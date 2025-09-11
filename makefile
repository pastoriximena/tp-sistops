# Makefile para el Trabajo Práctico de Sistemas Operativos
# Ejercicio 1: Generador de Datos con Procesos y Memoria Compartida

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LIBS = 
TARGET = generador_datos
OBJS = main.o coordinador.o generador.o semaforos.o

# Regla principal
all: $(TARGET)

# Compilar el ejecutable principal
$(TARGET): $(OBJS)
	@echo "🔧 Enlazando ejecutable $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@echo "✅ Compilación completada: $(TARGET)"

# Compilar archivos objeto
main.o: main.c shared_memory.h
	@echo "🔧 Compilando main.c..."
	$(CC) $(CFLAGS) -c main.c

coordinador.o: coordinador.c shared_memory.h
	@echo "🔧 Compilando coordinador.c..."
	$(CC) $(CFLAGS) -c coordinador.c

generador.o: generador.c shared_memory.h
	@echo "🔧 Compilando generador.c..."
	$(CC) $(CFLAGS) -c generador.c

semaforos.o: semaforos.c shared_memory.h
	@echo "🔧 Compilando semaforos.c..."
	$(CC) $(CFLAGS) -c semaforos.c

# Limpiar archivos compilados
clean:
	@echo "🧹 Limpiando archivos compilados..."
	rm -f $(OBJS) $(TARGET) datos_generados.csv
	@echo "✅ Limpieza completada"

# Limpiar recursos IPC (por si quedaron colgados)
clean-ipc:
	@echo "🧹 Limpiando recursos IPC..."
	@ipcs -s | grep $(USER) | awk '{print $$2}' | xargs -r ipcrm -s 2>/dev/null || true
	@ipcs -m | grep $(USER) | awk '{print $$2}' | xargs -r ipcrm -m 2>/dev/null || true
	@echo "✅ Recursos IPC limpiados"

# Ejecutar pruebas
test: $(TARGET)
	@echo "🧪 Ejecutando prueba con 3 generadores y 100 registros..."
	./$(TARGET) 3 100
	@echo ""
	@echo "📊 Verificando resultado:"
	@if [ -f datos_generados.csv ]; then \
		echo "   • Registros generados: $$(wc -l < datos_generados.csv | tr -d ' ')"; \
		echo "   • Primeras 5 líneas:"; \
		head -5 datos_generados.csv; \
	else \
		echo "   ❌ No se generó el archivo CSV"; \
	fi

# Verificar IDs con AWK (como pide el ejercicio)
verify: datos_generados.csv
	@echo "🔍 Verificando IDs con script AWK..."
	@awk -F',' ' \
		NR == 1 { next } \
		{ \
			id = $$1; \
			if (id in ids) { \
				print "❌ ID duplicado encontrado: " id; \
				duplicates++; \
			} else { \
				ids[id] = 1; \
			} \
			if (NR == 2) min_id = max_id = id; \
			if (id < min_id) min_id = id; \
			if (id > max_id) max_id = id; \
		} \
		END { \
			total = NR - 1; \
			expected_total = max_id - min_id + 1; \
			print "📈 Estadísticas de IDs:"; \
			print "   • Total registros: " total; \
			print "   • ID mínimo: " min_id; \
			print "   • ID máximo: " max_id; \
			print "   • Rango esperado: " expected_total; \
			if (duplicates > 0) { \
				print "   ❌ IDs duplicados: " duplicates; \
			} else { \
				print "   ✅ Sin IDs duplicados"; \
			} \
			if (total == expected_total) { \
				print "   ✅ IDs correlativos correctos"; \
			} else { \
				print "   ❌ Faltan IDs en la secuencia"; \
			} \
		}' datos_generados.csv

# Monitoreo del sistema (como pide el ejercicio)
monitor:
	@echo "🖥️  Monitoreando recursos del sistema..."
	@echo ""
	@echo "📊 Memoria compartida (ipcs -m):"
	@ipcs -m | head -1
	@ipcs -m | grep $(USER) || echo "   (Sin memoria compartida activa)"
	@echo ""
	@echo "🔒 Semáforos (ipcs -s):"
	@ipcs -s | head -1
	@ipcs -s | grep $(USER) || echo "   (Sin semáforos activos)"
	@echo ""
	@echo "⚡ Procesos relacionados (ps):"
	@ps aux | grep -E "(generador_datos|PID)" | grep -v grep || echo "   (Sin procesos activos)"

# Ayuda
help:
	@echo "📋 Comandos disponibles:"
	@echo "   make          - Compilar el proyecto"
	@echo "   make clean    - Limpiar archivos compilados"
	@echo "   make clean-ipc- Limpiar recursos IPC colgados"
	@echo "   make test     - Ejecutar prueba básica"
	@echo "   make verify   - Verificar IDs con AWK"
	@echo "   make monitor  - Mostrar recursos del sistema"
	@echo "   make help     - Mostrar esta ayuda"
	@echo ""
	@echo "💡 Uso del programa:"
	@echo "   ./$(TARGET) <num_generadores> <total_registros>"
monitor:
	@echo "=== Procesos coordinador/generador ==="
	@ps -fC coordinador || true
	@ps -fC generador || true
	@echo
	@echo "=== Recursos IPC (memoria y semáforos) ==="
	@ipcs -m
	@ipcs -s
	@echo
	@echo "=== /dev/shm ==="
	@ls -lh /dev/shm
	@echo
	@echo "=== Estadísticas vmstat (5 seg) ==="
	@vmstat 1 5

# Declarar targets que no son archivos
.PHONY: all clean clean-ipc test verify monitor help