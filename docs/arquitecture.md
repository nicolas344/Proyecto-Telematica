# Arquitectura del Sistema

## 1. Diagrama General

```
┌─────────────────────────────────────────────┐
│            SERVIDOR (C - Puerto 8080)        │
│                                              │
│  ┌────────────┐  ┌──────────┐  ┌─────────┐ │
│  │ Protocol   │  │  Logger  │  │  Auth   │ │
│  │ Handler    │  │  System  │  │ Manager │ │
│  └────────────┘  └──────────┘  └─────────┘ │
│                                              │
│  ┌──────────────────────────────────────┐  │
│  │   Client Handler (1 thread/client)   │  │
│  └──────────────────────────────────────┘  │
│                                              │
│  ┌──────────────────────────────────────┐  │
│  │   Telemetry Broadcast Thread         │  │
│  │   (broadcast cada 10s)               │  │
│  └──────────────────────────────────────┘  │
└──────────────────┬───────────────────────────┘
                   │ TCP/IP
        ┌──────────┼──────────┐
        │          │          │
   ┌────▼───┐ ┌────▼───┐ ┌───▼────┐
   │Cliente │ │Cliente │ │Cliente │
   │Admin   │ │Observer│ │Observer│
   │(Python)│ │(Java)  │ │(Java)  │
   └────────┘ └────────┘ └────────┘
```

---

## 2. Componentes del Servidor

### server.c - Núcleo Principal
```c
main()
├── Validar argumentos (puerto, log)
├── Inicializar sistemas
│   ├── logger_init()
│   ├── auth_init()
│   └── telemetry_init()
├── Crear socket TCP + bind + listen
├── Crear thread de telemetry_broadcast
└── Loop: accept() → crear thread handle_client()
```

### protocol.c/h - Manejo del Protocolo
**Funciones clave:**
- `parse_message()`: Raw string → Estructura Message
- `build_response()`: Construir respuesta VATP
- `build_telemetry_message()`: Formatear telemetría

**Estructuras:**
```c
typedef struct {
    char version[16];      // "VATP/1.0"
    MessageType type;      // MSG_CONNECT, MSG_COMMAND, etc.
    int length;            
    char username[32];
    char auth_token[128];
    char command[32];
    char data[2048];
} Message;
```

### client_handler.c/h - Gestión de Clientes
**Thread por cliente:**
```c
void* handle_client(void* arg) {
    while (1) {
        recv() mensaje
        parse_message()
        switch (msg.type) {
            case MSG_CONNECT: registrar tipo usuario
            case MSG_AUTH: autenticar y dar token
            case MSG_COMMAND: validar y ejecutar
            case MSG_LIST_USERS: listar conectados
        }
        send() respuesta
    }
    cleanup()
}
```

**Gestión de lista:**
- `add_client()`: Agregar a array (mutex protected)
- `remove_client()`: Remover y cerrar socket

### auth.c/h - Autenticación
```c
// Base de usuarios (en memoria)
typedef struct {
    char username[32];
    char password[64];
    char token[128];
    time_t token_expiry;  // timestamp + 3600s
} UserCredential;

// Funciones
authenticate_user()  // Validar user/pass, generar token
validate_token()     // Verificar token válido y no expirado
revoke_token()       // Logout
```

### telemetry.c/h - Sistema de Telemetría
```c
typedef struct {
    float speed;          // 0-100 km/h
    float battery;        // 0-100 %
    float temperature;    // 15-45 °C
    char direction[16];   // NORTH/SOUTH/EAST/WEST
    int is_moving;
} VehicleState;

// Thread de broadcast
void* telemetry_broadcast_thread() {
    while (1) {
        sleep(10);
        simulate_vehicle_changes();  // Consumir batería, variar temp
        build_telemetry_message();
        // Enviar a TODOS los clientes activos (mutex protected)
    }
}

// Validación
can_execute_command()  // Batería >= 10%, límites velocidad
update_vehicle_state() // Aplicar comando al estado
```

### logger.c/h - Sistema de Logging
**Características:**
- Thread-safe (mutex)
- Escribe en consola + archivo simultáneamente
- Formato: `[TIMESTAMP] CLIENT[IP:PORT] TYPE: MESSAGE`

---

## 3. Concurrencia y Sincronización

### Modelo de Threading
```
Main Thread
├── accept() loop
│   └── spawn thread per client
└── Telemetry Broadcast Thread (permanente)

Client Threads (hasta 50)
├── Cliente 1
├── Cliente 2
└── Cliente N
```

### Recursos Compartidos (Mutex Protected)

| Recurso | Mutex | Acceso |
|---------|-------|--------|
| `ClientInfo clients[50]` | `clients_mutex` | add/remove/iterate |
| `VehicleState vehicle_state` | `vehicle_mutex` | read/write estado |
| `FILE* log_file` | `log_mutex` | write logs |

**Patrón de uso:**
```c
pthread_mutex_lock(&clients_mutex);
// ... operación crítica ...
pthread_mutex_unlock(&clients_mutex);
```

**Prevención de deadlocks:**
- Lock único por operación
- Tiempo mínimo dentro del lock
- Orden consistente de locks

---

## 4. Componentes del Cliente

### Cliente Python (Tkinter)

**admin_client_gui.py:**
```python
class AdminClientApp:
    def __init__(self):
        # GUI: botones conexión + comandos + log
    
    def connect_to_server(self):
        socket.connect()
        send CONNECT
        send AUTH → guardar token
    
    def send_command(self, cmd):
        build COMMAND message
        send()
```

**observer_gui.py:**
```python
class ObserverClientGUI:
    def __init__(self):
        # Auto-connect al iniciar
        threading.Thread(listen_server)
    
    def listen_server(self):
        while True:
            recv() telemetría
            update GUI
```

### Cliente Java (Swing)

**Estructura similar:**
- Main thread: GUI (EventDispatchThread)
- Network thread: Recibir mensajes continuamente
- `SwingUtilities.invokeLater()` para actualizar GUI thread-safe

---

## 5. Flujo de Datos

### Broadcast de Telemetría
```
Telemetry Thread (cada 10s)
    │
    ├─ simulate_vehicle_changes()
    ├─ build_telemetry_message()
    │
    └─ lock(clients_mutex)
       FOR cada cliente activo:
           send(telemetry)
       unlock(clients_mutex)
```

### Comando de Control
```
Cliente Admin
    │
    └─ COMMAND (token + comando)
            ↓
    Server: handle_client thread
            │
            ├─ validate_token()
            ├─ can_execute_command()
            ├─ update_vehicle_state()
            │
            └─ RESPONSE_OK
```

---

## 6. Decisiones de Diseño

### ¿Por qué TCP?
✅ Comandos críticos no pueden perderse  
✅ Autenticación requiere confiabilidad  
✅ Detección automática de desconexiones

### ¿Por qué Protocolo de Texto?
✅ Debugging fácil (telnet, Wireshark)  
✅ Desarrollo más rápido  
✅ Educativo para proyecto académico

### ¿Por qué 1 Thread por Cliente?
✅ Simplicidad de código  
✅ Aislamiento de errores  
✅ Suficiente para 50 clientes (~400MB RAM)

### ¿Por qué Broadcast cada 10s?
✅ Balance tiempo real / eficiencia  
✅ Reduce carga de red  
✅ Suficiente para monitoreo

---

## 7. Limitaciones y Escalabilidad

### Límites Actuales
- **50 clientes máx**: Array estático
- **Buffer 2KB**: Suficiente para protocolo texto
- **1 hora token**: Balance seguridad/usabilidad

### Para Escalar a Producción (1000+ clientes)
1. **Thread pool** en vez de thread por cliente
2. **epoll/kqueue** para event-driven I/O
3. **Lista dinámica** (linked list)
4. **Redis** para tokens distribuidos
5. **Protocol Buffers** para eficiencia

---

## 8. Seguridad (Advertencias)

⚠️ **NO usar en producción:**
- Sin cifrado (TLS/SSL)
- Contraseñas en texto plano
- Tokens simples (no JWT)
- Sin rate limiting
- Sin validación robusta de inputs

**Para producción se requiere:**
- TLS obligatorio
- bcrypt para passwords
- JWT con firma
- RBAC (Role-Based Access Control)
- WAF (Web Application Firewall)

---

## 9. Diagrama de Despliegue

```
┌────────────────────┐         ┌────────────────────┐
│   Máquina Servidor │         │   Máquina Cliente  │
│                    │         │                    │
│  ./server 8080 ←──────TCP────→  python3 client.py│
│  server.log        │         │  java Client.java  │
└────────────────────┘         └────────────────────┘
        0.0.0.0:8080                192.168.1.X
```

**Requisitos de red:**
- Puerto TCP abierto (default 8080)
- Firewall configurado
- Misma red o rutas configuradas

---

**Patrones aplicados:**
- Producer-Consumer (telemetría)
- Observer (broadcast a clientes)
- State Pattern (estados del cliente)