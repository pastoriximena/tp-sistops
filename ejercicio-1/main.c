#include "shared_memory.h"
#include <signal.h>

// Variables globales para limpieza
int shmid = -1;
int semid = -1;
pid_t coordinador_pid = -1;
pid_t *generadores_pids = NULL;
int num_generadores = 0;

// Función para limpiar recursos al salir
void cleanup_handler(int sig) {
    (void)sig; // Evitar warning
    printf("\n🧹 Limpiando recursos del sistema...\n");
    
    // Terminar procesos hijos con SIGKILL si es necesario
    if (coordinador_pid > 0) {
        kill(coordinador_pid, SIGKILL);  // Usar SIGKILL para forzar
        waitpid(coordinador_pid, NULL, WNOHANG);
    }
    
    if (generadores_pids) {
        for (int i = 0; i < num_generadores; i++) {
            if (generadores_pids[i] > 0) {
                kill(generadores_pids[i], SIGKILL);
                waitpid(generadores_pids[i], NULL, WNOHANG);
            }
        }
        free(generadores_pids);
    }
    
    // Limpiar memoria compartida
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    
    // Limpiar semáforos
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID, 0);
    }
    
    printf("✅ Recursos liberados correctamente\n");
    exit(0);
}

void mostrar_ayuda(const char *programa) {
    printf("📋 USO: %s <num_generadores> <total_registros>\n\n", programa);
    printf("PARÁMETROS:\n");
    printf("  num_generadores  : Número de procesos generadores (1-10)\n");
    printf("  total_registros  : Cantidad total de registros a generar (>0)\n\n");
    printf("EJEMPLO:\n");
    printf("  %s 3 1000    # 3 generadores, 1000 registros totales\n\n", programa);
}

int main(int argc, char *argv[]) {
    // Validar parámetros
    if (argc != 3) {
        mostrar_ayuda(argv[0]);
        return 1;
    }
    
    int num_gen = atoi(argv[1]);
    int total_registros = atoi(argv[2]);
    
    if (num_gen <= 0 || num_gen > MAX_GENERADORES) {
        printf("❌ ERROR: Número de generadores debe estar entre 1 y 10\n");
        mostrar_ayuda(argv[0]);
        return 1;
    }
    
    if (total_registros <= 0) {
        printf("❌ ERROR: Total de registros debe ser mayor a 0\n");
        mostrar_ayuda(argv[0]);
        return 1;
    }
    
    printf("🚀 Iniciando sistema de generación de datos\n");
    printf("   📊 Generadores: %d\n", num_gen);
    printf("   📝 Registros objetivo: %d\n\n", total_registros);
    
    num_generadores = num_gen;
    generadores_pids = malloc(num_generadores * sizeof(pid_t));
    
    // Configurar manejador de señales para limpieza
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    signal(SIGALRM, cleanup_handler);  // Timeout de 30 segundos
    
    // Crear memoria compartida
    shmid = shmget(CLAVE_SHM, sizeof(MemoriaCompartida), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("❌ Error creando memoria compartida");
        return 1;
    }
    
    MemoriaCompartida *shm = (MemoriaCompartida*) shmat(shmid, NULL, 0);
    if (shm == (MemoriaCompartida*) -1) {
        perror("❌ Error attachando memoria compartida");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    
    // Crear semáforos
    semid = crear_semaforos();
    if (semid == -1) {
        printf("❌ Error creando semáforos\n");
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    
    // Inicializar memoria compartida
    memset(shm, 0, sizeof(MemoriaCompartida));
    shm->total_registros_objetivo = total_registros;
    shm->siguiente_id = 1;
    shm->generadores_activos = num_generadores;
    shm->estado = ESTADO_VACIO;
    
    printf("💾 Memoria compartida inicializada (ID: %d)\n", shmid);
    printf("🔒 Semáforos creados (ID: %d)\n", semid);
    
    // Crear proceso coordinador
    coordinador_pid = fork();
    if (coordinador_pid == 0) {
        proceso_coordinador(shmid, semid);
        exit(0);
    } else if (coordinador_pid < 0) {
        perror("❌ Error creando proceso coordinador");
        cleanup_handler(0);
        return 1;
    }
    
    printf("👑 Coordinador iniciado (PID: %d)\n", coordinador_pid);
    
    // Crear procesos generadores
    for (int i = 0; i < num_generadores; i++) {
        generadores_pids[i] = fork();
        if (generadores_pids[i] == 0) {
            proceso_generador(i + 1, shmid, semid);
            exit(0);
        } else if (generadores_pids[i] < 0) {
            perror("❌ Error creando proceso generador");
            cleanup_handler(0);
            return 1;
        }
        printf("🏭 Generador %d iniciado (PID: %d)\n", i + 1, generadores_pids[i]);
    }
    
    printf("\n🎯 Todos los procesos iniciados. Esperando finalización...\n\n");
    
    // Configurar timeout de 30 segundos para forzar terminación si hay deadlock
    alarm(30);
    
    // Esperar a que termine el coordinador
    int status;
    waitpid(coordinador_pid, &status, 0);
    
    // Esperar a que terminen todos los generadores
    for (int i = 0; i < num_generadores; i++) {
        waitpid(generadores_pids[i], &status, 0);
    }
    
    printf("\n🎉 Generación de datos completada!\n");
    printf("📄 Archivo 'datos_generados.csv' creado con %d registros\n", total_registros);
    
    // Limpiar recursos
    shmdt(shm);
    cleanup_handler(0);
    
    return 0;
}