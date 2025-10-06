#  Proyecto Vehículo Autónomo de Telemetría

Sistema de comunicación cliente-servidor para un vehículo autónomo que transmite telemetría en tiempo real y recibe comandos de control remotos. Implementado con el protocolo personalizado **VATP (Vehicle Autonomous Telemetry Protocol)** sobre TCP/IP.


##  Video Demostrativo

**[Ver Video Explicativo](https://youtu.be/TU_ENLACE_AQUI)**

El video de 12 minutos incluye:
- Demostración del funcionamiento completo
- Explicación del código del servidor
- Explicación del código de los clientes
- Pruebas de concurrencia y robustez

---

##  Equipo de Desarrollo

- **Nicolas Rico Montesino** 
- **Santiago Alvarez Diaz** 
- **Jhon Fredy Alzate Duque** 
- **Tomas Gañan Rivera**  

##  Descripción

Este proyecto implementa un sistema de comunicación para un vehículo autónomo terrestre que:

- **Transmite telemetría** (velocidad, batería, temperatura, dirección) cada 10 segundos a múltiples usuarios
- **Recibe comandos de control** de administradores autenticados
- **Soporta múltiples clientes** simultáneos con gestión de concurrencia mediante hilos
- **Implementa autenticación** basada en tokens para administradores
- **Diferencia tipos de usuarios**: Administradores (control total) y Observadores (solo lectura)

---

## Características

### Servidor (C)
- ✅ API de Sockets Berkeley
- ✅ Manejo concurrente de múltiples clientes con `pthread`
- ✅ Protocolo personalizado VATP/1.0 en formato texto
- ✅ Sistema de logging completo (consola + archivo)
- ✅ Autenticación por tokens con expiración
- ✅ Broadcast automático de telemetría cada 10 segundos
- ✅ Validación de comandos según estado del vehículo

### Clientes
- ✅ **Python**: Cliente administrador y observador con GUI (Tkinter)
- ✅ **Java**: Cliente administrador y observador con GUI (Swing)
- ✅ Visualización en tiempo real de telemetría
- ✅ Interfaz gráfica intuitiva y responsiva

---

##  Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────┐
│                    SERVIDOR (C - Puerto 8080)                │
│  ┌────────────┐  ┌─────────────┐  ┌──────────────────┐     │
│  │  Protocol  │  │   Logger    │  │  Telemetry       │     │
│  │  Handler   │  │   System    │  │  Broadcast       │     │
│  └────────────┘  └─────────────┘  └──────────────────┘     │
│         │                │                   │               │
│         └────────────────┴───────────────────┘               │
│                          │                                   │
│              ┌───────────┴───────────┐                       │
│              │  Client Handler       │                       │
│              │  (Multi-threaded)     │                       │
│              └───────────────────────┘                       │
└─────────────────────────┬───────────────────────────────────┘
                          │ TCP/IP
          ┌───────────────┼───────────────┐
          │               │               │
    ┌─────▼─────┐   ┌─────▼─────┐  ┌─────▼─────┐
    │  Cliente  │   │  Cliente  │  │  Cliente  │
    │  Admin    │   │ Observer  │  │ Observer  │
    │  (Python) │   │  (Java)   │  │  (Java)   │
    └───────────┘   └───────────┘  └───────────┘
```

---

##  Requisitos

### Servidor
- **Sistema Operativo**: Linux / macOS / WSL (Windows Subsystem for Linux)
- **Compilador**: GCC (GNU Compiler Collection)
- **Bibliotecas**: pthread (incluida en sistemas POSIX)

### Cliente Python
- **Python**: 3.7 o superior
- **Módulos**: `tkinter` (generalmente incluido con Python)

### Cliente Java
- **Java**: JDK 8 o superior
- **Swing**: Incluido en el JDK estándar

---

##  Instalación

### 1. Clonar el Repositorio

```bash
git clone https://github.com/nicolas344/Proyecto-Telematica.git 
```

### 2. Compilar el Servidor

```bash
cd Server
make
```

Esto generará el ejecutable `server`.

### 3. Verificar Compilación

```bash
./server
# Debería mostrar: Uso: ./server <puerto> <archivo_log>
```

---

##  Uso

### Iniciar el Servidor

```bash
cd Server
./server 8080 server.log
```

**Parámetros:**
- `8080`: Puerto de escucha (puede ser cualquier puerto disponible entre 1024-65535)
- `server.log`: Archivo donde se guardarán los logs

**Salida esperada:**
```
==============================================
  SERVIDOR DE VEHÍCULO AUTÓNOMO DE TELEMETRÍA
==============================================

[2025-10-05 14:30:00] INFO: Servidor inicializado en puerto 8080
[2025-10-05 14:30:00] INFO: Servidor escuchando en puerto 8080
[2025-10-05 14:30:00] INFO: Thread de telemetría iniciado (broadcast cada 10 segundos)

✓ Servidor listo para recibir conexiones en puerto 8080
✓ Logs guardándose en: server.log

Presione Ctrl+C para detener el servidor
==============================================
```

### Ejecutar Cliente Python (Administrador)

```bash
cd clients/client_python
python3 admin_client_gui.py
```

**Credenciales por defecto:**
- Usuario: `admin`
- Contraseña: `admin123`

### Ejecutar Cliente Python (Observador)

```bash
cd clients/client_python
python3 observer_gui.py
```

Se conecta automáticamente al servidor y comienza a recibir telemetría.

### Ejecutar Cliente Java (Administrador)

```bash
cd clients/client_java
javac AdminClientGUI.java
java AdminClientGUI
```

### Ejecutar Cliente Java (Observador)

```bash
cd clients/client_java
javac ObserverClientGUI.java
java ObserverClientGUI
```

---

##  Estructura del Proyecto

```
vehiculo-autonomo-telemetria/
│
├── Server/                          # Servidor en C
│   ├── server.c                     # Punto de entrada del servidor
│   ├── protocol.c/.h                # Implementación del protocolo VATP
│   ├── logger.c/.h                  # Sistema de logging
│   ├── auth.c/.h                    # Autenticación y tokens
│   ├── telemetry.c/.h               # Gestión de telemetría
│   ├── client_handler.c/.h          # Manejo de clientes
│   ├── Makefile                     # Compilación automatizada
│   └── server.log                   # Logs del servidor (generado)
│
├── clients/
│   ├── client_python/               # Clientes en Python
│   │   ├── admin_client_gui.py      # Cliente administrador (Tkinter)
│   │   └── observer_gui.py          # Cliente observador (Tkinter)
│   │
│   └── client_java/                 # Clientes en Java
│       ├── AdminClientGUI.java      # Cliente administrador (Swing)
│       └── ObserverClientGUI.java   # Cliente observador (Swing)
│
├── docs/                            # Documentación
│   ├── PROTOCOL.md                  # Especificación del protocolo VATP
│   ├── ARCHITECTURE.md              # Arquitectura del sistema
│
├── .gitignore                       # Archivos ignorados por Git
├── README.md                        # Este archivo
```

---

##  Protocolo VATP

**VATP (Vehicle Autonomous Telemetry Protocol)** es un protocolo de capa de aplicación en formato texto diseñado específicamente para este proyecto.

### Formato General

```
VATP/1.0 <TIPO_MENSAJE> <LONGITUD>\r\n
<Header1>: <Valor1>\r\n
<Header2>: <Valor2>\r\n
\r\n
<Body (opcional)>
```

### Tipos de Mensajes

| Mensaje | Descripción | Enviado por |
|---------|-------------|-------------|
| `CONNECT` | Solicitud de conexión | Cliente |
| `AUTH` | Autenticación de administrador | Cliente Admin |
| `GET_TELEMETRY` | Solicitar telemetría inmediata | Cliente |
| `COMMAND` | Enviar comando de control | Cliente Admin |
| `LIST_USERS` | Listar usuarios conectados | Cliente Admin |
| `DISCONNECT` | Desconexión | Cliente |
| `RESPONSE_OK` | Respuesta exitosa | Servidor |
| `RESPONSE_ERROR` | Respuesta de error | Servidor |
| `TELEMETRY_DATA` | Datos de telemetría | Servidor |

### Comandos Disponibles

- `SPEED_UP`: Aumentar velocidad (+10 km/h)
- `SLOW_DOWN`: Reducir velocidad (-10 km/h)
- `TURN_LEFT`: Girar a la izquierda
- `TURN_RIGHT`: Girar a la derecha

---
