# Makefile Principal - Trabajo Práctico de Sistemas Operativos
# Universidad Nacional de La Matanza - Segundo Cuatrimestre 2025

.PHONY: all clean test demo help ejercicio-1 ejercicio-2

# Compilar ambos ejercicios
all:
	@echo "🎓 TRABAJO PRÁCTICO - SISTEMAS OPERATIVOS"
	@echo "========================================="
	@echo ""
	@echo "📁 Compilando Ejercicio 1..."
	@cd ejercicio-1 && $(MAKE)
	@echo ""
	@echo "📁 Compilando Ejercicio 2..."
	@cd ejercicio-2 && $(MAKE)
	@echo ""
	@echo "✅ Ambos ejercicios compilados exitosamente"

# Ejercicio 1 individual
ejercicio-1:
	@echo "🏗️  Compilando Ejercicio 1..."
	@cd ejercicio-1 && $(MAKE)

# Ejercicio 2 individual
ejercicio-2:
	@echo "🏗️  Compilando Ejercicio 2..."
	@cd ejercicio-2 && $(MAKE)

# Limpiar ambos ejercicios
clean:
	@echo "🧹 Limpiando ambos ejercicios..."
	@cd ejercicio-1 && $(MAKE) clean 2>/dev/null || true
	@cd ejercicio-2 && $(MAKE) clean 2>/dev/null || true
	@echo "✅ Limpieza completada"

# Limpiar recursos IPC
clean-ipc:
	@echo "🧹 Limpiando recursos IPC..."
	@cd ejercicio-1 && $(MAKE) clean-ipc 2>/dev/null || true

# Prueba completa del sistema
test: all
	@echo "🧪 PRUEBA COMPLETA DEL SISTEMA"
	@echo "=============================="
	@echo ""
	@echo "1️⃣  Probando Ejercicio 1 (Generación de datos)..."
	@cd ejercicio-1 && $(MAKE) test
	@echo ""
	@echo "2️⃣  Verificando datos generados..."
	@cd ejercicio-1 && $(MAKE) verify
	@echo ""
	@echo "3️⃣  Probando Ejercicio 2 (Cliente-Servidor)..."
	@cd ejercicio-2 && $(MAKE) test
	@echo ""
	@echo "✅ Pruebas completadas"

# Demo interactivo completo
demo: all
	@echo "🎭 DEMO COMPLETO DEL TRABAJO PRÁCTICO"
	@echo "====================================="
	@echo ""
	@echo "📊 1. Generando datos con múltiples procesos..."
	@cd ejercicio-1 && ./generador_datos 4 100
	@echo ""
	@echo "🔍 2. Verificando integridad de los datos..."
	@cd ejercicio-1 && $(MAKE) verify
	@echo ""
	@echo "🌐 3. Iniciando sistema cliente-servidor..."
	@cd ejercicio-2 && $(MAKE) demo
	@echo ""
	@echo "🎉 Demo completado!"

# Monitoreo completo
monitor:
	@echo "🖥️  MONITOREO COMPLETO DEL SISTEMA"
	@echo "=================================="
	@echo ""
	@echo "📊 Ejercicio 1 - Procesos y Memoria Compartida:"
	@cd ejercicio-1 && $(MAKE) monitor 2>/dev/null || echo "   (Ejercicio 1 no compilado)"
	@echo ""
	@echo "🌐 Ejercicio 2 - Cliente-Servidor:"
	@cd ejercicio-2 && $(MAKE) monitor 2>/dev/null || echo "   (Ejercicio 2 no compilado)"

# Matar todos los procesos
kill-all:
	@echo "🔪 Terminando todos los procesos del TP..."
	@pkill generador_datos 2>/dev/null || true
	@pkill servidor 2>/dev/null || true
	@pkill cliente 2>/dev/null || true
	@cd ejercicio-1 && $(MAKE) clean-ipc 2>/dev/null || true
	@echo "✅ Procesos terminados"

# Ayuda completa
help:
	@echo "📋 TRABAJO PRÁCTICO - SISTEMAS OPERATIVOS"
	@echo "========================================="
	@echo ""
	@echo "👨‍🎓 Estudiante: [TU NOMBRE]"
	@echo "🏫 Universidad Nacional de La Matanza"
	@echo "📅 Segundo Cuatrimestre 2025"
	@echo ""
	@echo "🏗️  COMPILACIÓN:"
	@echo "   make              - Compilar ambos ejercicios"
	@echo "   make ejercicio-1  - Compilar solo Ejercicio 1"
	@echo "   make ejercicio-2  - Compilar solo Ejercicio 2"
	@echo "   make clean        - Limpiar archivos compilados"
	@echo ""
	@echo "🧪 PRUEBAS:"
	@echo "   make test         - Prueba completa del sistema"
	@echo "   make demo         - Demo interactivo completo"
	@echo "   make monitor      - Monitorear recursos"
	@echo "   make kill-all     - Terminar todos los procesos"
	@echo ""
	@echo "📁 EJERCICIO 1 (Procesos + Memoria Compartida):"
	@echo "   cd ejercicio-1"
	@echo "   ./generador_datos <generadores> <registros>"
	@echo ""
	@echo "📁 EJERCICIO 2 (Cliente-Servidor + Transacciones):"
	@echo "   cd ejercicio-2"
	@echo "   Terminal 1: make run-server"
	@echo "   Terminal 2: make run-client"
	@echo ""
	@echo "📚 Para ayuda específica:"
	@echo "   cd ejercicio-1 && make help"
	@echo "   cd ejercicio-2 && make help"

# Crear estructura de directorios si no existe
init:
	@echo "📁 Inicializando estructura del proyecto..."
	@mkdir -p ejercicio-1 ejercicio-2
	@echo "✅ Estructura creada"

# Información del proyecto
info:
	@echo "📊 INFORMACIÓN DEL PROYECTO"
	@echo "==========================="
	@echo ""
	@echo "📋 Ejercicio 1: Generador de Datos con Procesos"
	@echo "   • Múltiples procesos generadores"
	@echo "   • Memoria compartida (SHM)"
	@echo "   • Sincronización con semáforos"
	@echo "   • Generación de archivo CSV"
	@echo ""
	@echo "📋 Ejercicio 2: Cliente-Servidor con Transacciones"
	@echo "   • Servidor multi-thread"
	@echo "   • Sockets TCP/IP"
	@echo "   • Operaciones CRUD sobre CSV"
	@echo "   • Sistema de transacciones"
	@echo ""
	@echo "🔧 Tecnologías utilizadas:"
	@echo "   • Lenguaje: C (estándar C99)"
	@echo "   • IPC: Memoria compartida, semáforos, sockets"
	@echo "   • Threading: pthreads"
	@echo "   • Persistencia: Archivos CSV"

.PHONY: init info