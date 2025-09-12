#include "shared_memory.h"

// Operación wait en semáforo
void sem_wait(int semid, int sem_num) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = -1;  // Decrementar
    op.sem_flg = 0;
    
    if (semop(semid, &op, 1) == -1) {
        perror("Error en sem_wait");
        exit(1);
    }
}

// Operación signal en semáforo
void sem_signal(int semid, int sem_num) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = 1;   // Incrementar
    op.sem_flg = 0;
    
    if (semop(semid, &op, 1) == -1) {
        perror("Error en sem_signal");
        exit(1);
    }
}

// Crear conjunto de semáforos
int crear_semaforos() {
    int semid = semget(CLAVE_SEM, NUM_SEMAFOROS, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Error creando semáforos");
        return -1;
    }
    
    // Inicializar semáforos
    if (semctl(semid, SEM_MUTEX, SETVAL, 1) == -1) {  // Mutex en 1
        perror("Error inicializando SEM_MUTEX");
        return -1;
    }
    
    if (semctl(semid, SEM_COORD, SETVAL, 0) == -1) {  // Coordinador en 0
        perror("Error inicializando SEM_COORD");
        return -1;
    }
    
    // Inicializar semáforos de generadores en 0
    for (int i = 0; i < MAX_GENERADORES; i++) {
        if (semctl(semid, SEM_GEN_BASE + i, SETVAL, 0) == -1) {
            perror("Error inicializando semáforo de generador");
            return -1;
        }
    }
    
    return semid;
}

// Eliminar semáforos
void eliminar_semaforos(int semid) {
    if (semctl(semid, 0, IPC_RMID, 0) == -1) {
        perror("Error eliminando semáforos");
    }
}