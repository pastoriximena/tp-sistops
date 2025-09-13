# Makefile para Ejercicio 2: Cliente-Servidor de Micro Base de Datos con Transacciones

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LIBS = -lpthread
TARGET_SERVER = servidor
TARGET_CLIENT = cliente
OBJS_SERVER = servidor.o protocolo.o database.o
OBJS_CLIENT = cliente.o protocolo.o
CSV_PATH = ../ejercicio-1/datos_generados.csv

# Regla principal
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Compilar servidor
$(TARGET_SERVER): $(OBJS_SERVER)
	@echo "🔧 Enlazando ejecutable $(TARGET_SERVER)..."
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJS_SERVER) $(LIBS)
	@echo "✅ Servidor compilado: $(TARGET_SERVER)"

# Compilar cliente
$(TARGET_CLIENT): $(OBJS_CLIENT)
	@echo "🔧 Enlazando ejecutable $(TARGET_CLIENT)..."
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJS_CLIENT) $(LIBS)
	@echo "✅ Cliente compilado: $(TARGET_CLIENT)"

# Archivos objeto
servidor.o: servidor.c protocolo.h
	@echo "🔧 Compilando servidor.c..."
	$(CC) $(CFLAGS) -c servidor.c

cliente.o: cliente.c protocolo.h
	@echo "🔧 Compilando cliente.c..."
	$(CC) $(CFLAGS) -c cliente.c

protocolo.o: protocolo.c protocolo.h
	@echo "🔧 Compilando protocolo.c..."
	$(CC) $(CFLAGS) -c protocolo.c

database.o: database.c protocolo.h
	@echo "🔧 Compilando database.c..."
	$(CC) $(CFLAGS) -c database.c

# Limpiar archivos compilados
clean:
	@echo "🧹 Limpiando archivos compilados..."
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)
	rm -f temp_*.csv database.lock
	@echo "✅ Limpieza completada"

# Verificar datos del Ejercicio 1
check-data:
	@echo "🔍 Verificando datos del Ejercicio 1..."
	@if [ ! -f $(CSV_PATH) ]; then \
		echo "❌ No se encuentra $(CSV_PATH)"; \
		echo "💡 Ejecuta primero: cd ../ejercicio-1 && make test"; \
		exit 1; \
	else \
		echo "✅ Datos encontrados: $(CSV_PATH)"; \
		echo "📊 Total registros: $$(wc -l < $(CSV_PATH) | tr -d ' ')"; \
		echo "📋 Encabezados: $$(head -1 $(CSV_PATH))"; \
	fi

# Crear datos si no existen
create-data:
	@if [ ! -f $(CSV_PATH) ]; then \
		echo "📊 Generando datos de prueba..."; \
		cd ../ejercicio-1 && make && ./generador_datos 3 30; \
	else \
		echo "✅ Datos ya existen: $(CSV_PATH)"; \
	fi

# Crear configuración por defecto si no existe
config.txt:
	@if [ ! -f config.txt ]; then \
		echo "⚙️  Creando config.txt por defecto..."; \
		echo "ip=127.0.0.1" > config.txt; \
		echo "puerto=8080" >> config.txt; \
		echo "max_clientes=5" >> config.txt; \
		echo "max_espera=10" >> config.txt; \
		echo "csv_file=../ejercicio-1/datos_generados.csv" >> config.txt; \
		echo "✅ Configuración creada"; \
	fi

# Iniciar servidor
run-server: $(TARGET_SERVER) check-data config.txt
	@echo "🚀 Iniciando servidor..."
	@echo "🌐 Dirección: http://127.0.0.1:8080"
	@echo "🛑 Para detener: Ctrl+C"
	@echo ""
	./$(TARGET_SERVER)

# Iniciar cliente
run-client: $(TARGET_CLIENT)
	@echo "🚀 Iniciando cliente..."
	@echo "💡 Comandos útiles: SELECT, HELP, QUIT"
	@echo ""
	./$(TARGET_CLIENT)

# Servidor en background
start-server: $(TARGET_SERVER) check-data config.txt
	@echo "🚀 Iniciando servidor en background..."
	./$(TARGET_SERVER) &
	@echo "📡 Servidor iniciado. PID: $$!"
	@echo "🔗 Conéctate con: make run-client"
	@echo "🛑 Para detener: make stop-server"

# Detener servidor
stop-server:
	@echo "🛑 Deteniendo servidor..."
	@pkill servidor 2>/dev/null || echo "   (No había servidor corriendo)"

# Demo interactivo
demo: all create-data config.txt
	@echo "🎭 DEMO EJERCICIO 2"
	@echo "=================="
	@echo ""
	@echo "📊 Base de datos disponible:"
	@head -3 $(CSV_PATH) 2>/dev/null || echo "❌ Error leyendo CSV"
	@echo "..."
	@echo ""
	@echo "🚀 1. Iniciando servidor en background..."
	@./$(TARGET_SERVER) &
	@SERVER_PID=$$!; \
	echo "📡 Servidor iniciado (PID: $$SERVER_PID)"; \
	echo "⏱️  Esperando 2 segundos..."; \
	sleep 2; \
	echo ""; \
	echo "🧪 2. Ejecutando comandos de prueba:"; \
	echo "SELECT IT" | timeout 5 ./$(TARGET_CLIENT) 2>/dev/null || true; \
	echo ""; \
	echo "🛑 3. Deteniendo servidor..."; \
	kill $$SERVER_PID 2>/dev/null || true; \
	echo "✅ Demo completado"

# Prueba completa del sistema
test: all create-data config.txt
	@echo "🧪 PRUEBA COMPLETA DEL SISTEMA"
	@echo "=============================="
	@echo ""
	@echo "📊 Verificando datos..."
	@if [ ! -f $(CSV_PATH) ]; then \
		echo "❌ No hay datos. Generando..."; \
		cd ../ejercicio-1 && make test; \
	fi
	@echo "✅ Datos verificados: $$(wc -l < $(CSV_PATH) | tr -d ' ') líneas"
	@echo ""
	@echo "🔧 Compilación completada"
	@echo "🚀 Para usar el sistema:"
	@echo "   Terminal 1: make run-server"
	@echo "   Terminal 2: make run-client"

# Monitoreo del sistema
monitor:
	@echo "🖥️  Monitoreando Ejercicio 2..."
	@echo ""
	@echo "🌐 Conexiones de red:"
	@netstat -tlnp 2>/dev/null | grep :8080 || echo "   (Servidor no escuchando)"
	@echo ""
	@echo "⚡ Procesos activos:"
	@ps aux | grep -E "(servidor|cliente)" | grep -v grep || echo "   (Sin procesos activos)"
	@echo ""
	@echo "📁 Archivos del sistema:"
	@ls -la $(CSV_PATH) 2>/dev/null || echo "   ❌ CSV no encontrado: $(CSV_PATH)"
	@ls -la database.lock 2>/dev/null || echo "   ✅ Sin bloqueo de BD"
	@ls -la temp_*.csv 2>/dev/null || echo "   ✅ Sin archivos temporales"

help:
	@echo "📋 EJERCICIO 2 - Comandos disponibles:"
	@echo ""
	@echo "🏗️  COMPILACIÓN:"
	@echo "   make              - Compilar servidor y cliente"
	@echo "   make clean        - Limpiar archivos compilados"
	@echo ""
	@echo "🚀 EJECUCIÓN:"
	@echo "   make run-server   - Iniciar servidor (interactivo)"
	@echo "   make run-client   - Iniciar cliente"
	@echo "   make start-server - Servidor en background"
	@echo "   make stop-server  - Detener servidor"
	@echo ""
	@echo "🧪 PRUEBAS:"
	@echo "   make test         - Verificación completa"
	@echo "   make demo         - Demo automático"
	@echo "   make monitor      - Monitorear sistema"
	@echo ""
	@echo "📊 DATOS:"
	@echo "   make check-data   - Verificar CSV del Ejercicio 1"
	@echo "   make create-data  - Generar datos si no existen"
	@echo ""
	@echo "💡 USO MANUAL:"
	@echo "   ./servidor [-c config.txt]"
	@echo "   ./cliente [-s IP] [-p puerto]"
	@echo ""
	@echo "📁 Estructura esperada:"
	@echo "   ../ejercicio-1/datos_generados.csv"

.PHONY: all clean check-data create-data run-server run-client start-server stop-server demo test monitor help