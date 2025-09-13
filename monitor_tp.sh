#!/bin/bash

# monitor_tp.sh - Script de monitoreo para el TP de Sistemas Operativos
# Universidad Nacional de La Matanza

COLOR_RESET='\033[0m'
COLOR_BLUE='\033[1;34m'
COLOR_GREEN='\033[1;32m'
COLOR_YELLOW='\033[1;33m'
COLOR_RED='\033[1;31m'

print_header() {
    echo -e "${COLOR_BLUE}=================================${COLOR_RESET}"
    echo -e "${COLOR_BLUE}  MONITOREO TP SISTEMAS OPERATIVOS${COLOR_RESET}"
    echo -e "${COLOR_BLUE}=================================${COLOR_RESET}"
    echo ""
}

monitor_ejercicio1() {
    echo -e "${COLOR_GREEN}📊 EJERCICIO 1 - PROCESOS Y MEMORIA COMPARTIDA${COLOR_RESET}"
    echo "------------------------------------------------"
    
    echo -e "${COLOR_YELLOW}🧠 Memoria compartida (ipcs -m):${COLOR_RESET}"
    ipcs -m | head -1
    ipcs -m | grep $(whoami) || echo "   (Sin memoria compartida activa)"
    echo ""
    
    echo -e "${COLOR_YELLOW}🔒 Semáforos (ipcs -s):${COLOR_RESET}"
    ipcs -s | head -1  
    ipcs -s | grep $(whoami) || echo "   (Sin semáforos activos)"
    echo ""
    
    echo -e "${COLOR_YELLOW}⚡ Procesos generador_datos:${COLOR_RESET}"
    ps aux | grep generador_datos | grep -v grep || echo "   (Sin procesos generador_datos activos)"
    echo ""
    
    echo -e "${COLOR_YELLOW}📂 Archivos generados:${COLOR_RESET}"
    if [ -f "ejercicio-1/datos_generados.csv" ]; then
        echo "   ✅ datos_generados.csv existe"
        echo "   📝 Líneas: $(wc -l < ejercicio-1/datos_generados.csv)"
        echo "   📋 Encabezado: $(head -1 ejercicio-1/datos_generados.csv)"
    else
        echo "   ❌ datos_generados.csv no encontrado"
    fi
    echo ""
}

monitor_ejercicio2() {
    echo -e "${COLOR_GREEN}🌐 EJERCICIO 2 - CLIENTE-SERVIDOR${COLOR_RESET}"
    echo "-----------------------------------"
    
    echo -e "${COLOR_YELLOW}🔌 Puertos en escucha (netstat):${COLOR_RESET}"
    netstat -tlnp 2>/dev/null | grep :8080 || echo "   (Puerto 8080 libre)"
    echo ""
    
    echo -e "${COLOR_YELLOW}🔌 Puertos en escucha (ss - moderno):${COLOR_RESET}"
    ss -tlnp | grep :8080 || echo "   (Puerto 8080 libre)"
    echo ""
    
    echo -e "${COLOR_YELLOW}📡 Procesos servidor/cliente:${COLOR_RESET}"
    ps aux | grep -E "(servidor|cliente)" | grep -v grep || echo "   (Sin procesos servidor/cliente activos)"
    echo ""
    
    echo -e "${COLOR_YELLOW}🔗 Archivos abiertos en puerto 8080 (lsof):${COLOR_RESET}"
    lsof -i :8080 2>/dev/null || echo "   (Puerto 8080 no en uso)"
    echo ""
    
    echo -e "${COLOR_YELLOW}🗃️ Archivos del sistema:${COLOR_RESET}"
    if [ -f "ejercicio-2/database.lock" ]; then
        echo "   🔒 database.lock existe (transacción activa)"
    else
        echo "   ✅ Sin bloqueo de base de datos"
    fi
    
    ls ejercicio-2/temp_*.csv 2>/dev/null && echo "   ⚠️ Archivos temporales presentes" || echo "   ✅ Sin archivos temporales"
    echo ""
}

monitor_recursos_sistema() {
    echo -e "${COLOR_GREEN}💻 RECURSOS DEL SISTEMA${COLOR_RESET}"
    echo "------------------------"
    
    echo -e "${COLOR_YELLOW}📈 Memoria del sistema (vmstat):${COLOR_RESET}"
    vmstat 1 3 | tail -2
    echo ""
    
    echo -e "${COLOR_YELLOW}🏭 Procesos del usuario actual:${COLOR_RESET}"
    ps aux | grep $(whoami) | grep -E "(generador|servidor|cliente)" | wc -l | xargs echo "   Procesos activos:"
    echo ""
}

limpiar_recursos() {
    echo -e "${COLOR_RED}🧹 LIMPIANDO RECURSOS IPC${COLOR_RESET}"
    echo "-------------------------"
    
    # Limpiar semáforos
    echo "Eliminando semáforos..."
    ipcs -s | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -s 2>/dev/null || true
    
    # Limpiar memoria compartida  
    echo "Eliminando memoria compartida..."
    ipcs -m | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -m 2>/dev/null || true
    
    # Matar procesos
    echo "Terminando procesos..."
    pkill -f generador_datos 2>/dev/null || true
    pkill -f servidor 2>/dev/null || true
    pkill -f cliente 2>/dev/null || true
    
    echo -e "${COLOR_GREEN}✅ Recursos limpiados${COLOR_RESET}"
}

mostrar_ayuda() {
    echo "USO: ./monitor_tp.sh [opcion]"
    echo ""
    echo "OPCIONES:"
    echo "  -1, --ej1      Monitorear solo Ejercicio 1"
    echo "  -2, --ej2      Monitorear solo Ejercicio 2"
    echo "  -s, --sistema  Monitorear recursos del sistema"
    echo "  -c, --clean    Limpiar todos los recursos IPC"
    echo "  -w, --watch    Monitoreo continuo (cada 2 seg)"
    echo "  -h, --help     Mostrar esta ayuda"
    echo ""
    echo "Sin opciones: Monitoreo completo"
}

# Función principal
case "$1" in
    -1|--ej1)
        print_header
        monitor_ejercicio1
        ;;
    -2|--ej2)
        print_header
        monitor_ejercicio2
        ;;
    -s|--sistema)
        print_header
        monitor_recursos_sistema
        ;;
    -c|--clean)
        limpiar_recursos
        ;;
    -w|--watch)
        while true; do
            clear
            print_header
            monitor_ejercicio1
            monitor_ejercicio2
            monitor_recursos_sistema
            echo "Presiona Ctrl+C para salir..."
            sleep 2
        done
        ;;
    -h|--help)
        mostrar_ayuda
        ;;
    "")
        # Monitoreo completo por defecto
        print_header
        monitor_ejercicio1
        monitor_ejercicio2
        monitor_recursos_sistema
        ;;
    *)
        echo "Opción no reconocida: $1"
        mostrar_ayuda
        exit 1
        ;;
esac