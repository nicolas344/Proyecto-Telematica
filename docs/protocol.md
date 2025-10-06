# Especificación del Protocolo VATP v1.0

## 1. Visión General

**VATP (Vehicle Autonomous Telemetry Protocol)** es un protocolo de texto sobre TCP para comunicación entre un vehículo autónomo (servidor) y múltiples clientes.

### Características
- Protocolo de capa de aplicación sobre TCP
- Formato de texto legible
- Autenticación por tokens
- Dos tipos de usuarios: Admin (control) y Observer (solo lectura)
- Broadcast de telemetría cada 10 segundos

---

## 2. Formato de Mensaje

### Estructura General
```
VATP/1.0 <TIPO_MENSAJE> <LONGITUD>\r\n
<Header1>: <Valor1>\r\n
<Header2>: <Valor2>\r\n
\r\n
[Body opcional]
```

### Ejemplo
```
VATP/1.0 COMMAND 0\r\n
Username: admin\r\n
Auth-Token: TOKEN_1728145632_89234\r\n
Command: SPEED_UP\r\n
\r\n
```

---

## 3. Tipos de Mensajes

### Del Cliente → Servidor

| Mensaje | Propósito | Headers Requeridos | Requiere Auth |
|---------|-----------|-------------------|---------------|
| `CONNECT` | Iniciar conexión | `User-Type: ADMIN\|OBSERVER` | No |
| `AUTH` | Autenticar admin | `Username`, `Password` | No |
| `GET_TELEMETRY` | Pedir telemetría ahora | - | No |
| `COMMAND` | Enviar comando | `Username`, `Auth-Token`, `Command` | Sí |
| `LIST_USERS` | Listar conectados | `Username`, `Auth-Token` | Sí |
| `DISCONNECT` | Cerrar conexión | - | No |

### Del Servidor → Cliente

| Mensaje | Cuándo se envía |
|---------|-----------------|
| `RESPONSE_OK` | Operación exitosa |
| `RESPONSE_ERROR` | Error en operación |
| `TELEMETRY_DATA` | Automático cada 10s + bajo demanda |

---

## 4. Comandos Disponibles

| Comando | Efecto | Validación |
|---------|--------|------------|
| `SPEED_UP` | +10 km/h | Max 100 km/h, Batería ≥10% |
| `SLOW_DOWN` | -10 km/h | Min 0 km/h |
| `TURN_LEFT` | Rotar 90° izq | - |
| `TURN_RIGHT` | Rotar 90° der | - |

**Direcciones:** NORTH → WEST → SOUTH → EAST → NORTH (sentido horario)

---

## 5. Flujos de Comunicación

### Flujo Observer
```
Cliente                          Servidor
  |                                 |
  |--- CONNECT (OBSERVER) --------->|
  |<-- RESPONSE_OK -----------------|
  |                                 |
  |<-- TELEMETRY_DATA (cada 10s) ---|
  |<-- TELEMETRY_DATA --------------|
  |                                 |
```

### Flujo Admin
```
Cliente                          Servidor
  |                                 |
  |--- CONNECT (ADMIN) ------------>|
  |<-- RESPONSE_OK -----------------|
  |                                 |
  |--- AUTH (user, pass) ---------->|
  |<-- RESPONSE_OK + Token ---------|
  |                                 |
  |--- COMMAND + Token ------------>|
  |<-- RESPONSE_OK -----------------|
  |                                 |
  |<-- TELEMETRY_DATA (cada 10s) ---|
  |                                 |
```

---

## 6. Ejemplos Completos

### Conexión como Observer
```
→ VATP/1.0 CONNECT 0\r\n
  User-Type: OBSERVER\r\n
  \r\n

← VATP/1.0 RESPONSE_OK 58\r\n
  \r\n
  Conectado como OBSERVER. Recibirá telemetría automáticamente
```

### Autenticación Admin
```
→ VATP/1.0 AUTH 0\r\n
  Username: admin\r\n
  Password: admin123\r\n
  \r\n

← VATP/1.0 RESPONSE_OK 45\r\n
  \r\n
  Autenticación exitosa. Token: TOKEN_1728145632_89234
```

### Envío de Comando
```
→ VATP/1.0 COMMAND 0\r\n
  Username: admin\r\n
  Auth-Token: TOKEN_1728145632_89234\r\n
  Command: SPEED_UP\r\n
  \r\n

← VATP/1.0 RESPONSE_OK 62\r\n
  \r\n
  Comando SPEED_UP ejecutado. Speed: 10.00 km/h, Direction: NORTH
```

### Telemetría
```
← VATP/1.0 TELEMETRY_DATA 98\r\n
  \r\n
  Speed: 45.00 km/h
  Battery: 85.50%
  Temperature: 28.30 C
  Direction: NORTH
  Moving: Yes
```

---

## 7. Errores Comunes

| Error | Causa | Solución |
|-------|-------|----------|
| `Credenciales inválidas` | Usuario/pass incorrecto | Verificar credenciales |
| `Token inválido o expirado` | Token > 1 hora | Reautenticar |
| `Debe ser administrador autenticado` | Observer intenta comando | Conectar como ADMIN |
| `Batería demasiado baja` | Batería < 10% | Esperar simulación de recarga |
| `Límite de velocidad alcanzado` | Speed = 100 km/h | Usar SLOW_DOWN primero |
| `Formato de mensaje inválido` | Parsing falló | Revisar formato VATP |

---

## 8. Justificación TCP vs UDP

**Se usa TCP porque:**
- ✅ Comandos de control son críticos (no pueden perderse)
- ✅ Autenticación requiere confiabilidad
- ✅ Orden de mensajes importante
- ✅ Detección automática de desconexiones

**UDP sería inadecuado por:**
- ❌ Comandos podrían perderse
- ❌ Mensajes podrían llegar desordenados
- ❌ No detecta desconexiones

---

## 9. Seguridad (Limitaciones)

⚠️ **Este protocolo es académico. NO usar en producción sin:**
- TLS/SSL para cifrado
- Hash de contraseñas (bcrypt)
- JWT estándar para tokens
- Rate limiting
- Validación estricta de inputs

---

**Usuarios de prueba:**
- `admin` / `admin123`
- `admin2` / `pass456`

**Token válido por:** 1 hora (3600 segundos)