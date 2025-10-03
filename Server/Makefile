# Makefile para el Servidor de Vehículo Autónomo

CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = server
OBJS = server.o protocol.o logger.o auth.o telemetry.o client_handler.o

# Regla principal
all: $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "✓ Compilación exitosa. Ejecutable: ./$(TARGET)"

# Compilar archivos objeto
server.o: server.c protocol.h logger.h auth.h telemetry.h client_handler.h
	$(CC) $(CFLAGS) -c server.c

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

auth.o: auth.c auth.h protocol.h
	$(CC) $(CFLAGS) -c auth.c

telemetry.o: telemetry.c telemetry.h protocol.h logger.h
	$(CC) $(CFLAGS) -c telemetry.c

client_handler.o: client_handler.c client_handler.h protocol.h logger.h auth.h telemetry.h
	$(CC) $(CFLAGS) -c client_handler.c

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "✓ Archivos limpiados"

# Limpiar y recompilar
rebuild: clean all

# Ejecutar el servidor con parámetros de ejemplo
run: $(TARGET)
	./$(TARGET) 8080 server.log

# Ayuda
help:
	@echo "Uso del Makefile:"
	@echo "  make          - Compilar el proyecto"
	@echo "  make clean    - Eliminar archivos compilados"
	@echo "  make rebuild  - Limpiar y recompilar"
	@echo "  make run      - Compilar y ejecutar con puerto 8080"
	@echo "  make help     - Mostrar esta ayuda"
	@echo ""
	@echo "Ejecución manual:"
	@echo "  ./server <puerto> <archivo_log>"
	@echo "  Ejemplo: ./server 8080 server.log"

.PHONY: all clean rebuild run help