#include "shared_memory.h"

const char* nombres[] = {
    "Thaldrin", "Elenya", "Borak", "Lyraen", "Kaelith",
    "Druanor", "Selindra", "Veynor", "Aelthar", "Morgana"
};

// Apellidos/familias clánicas
const char* apellidos[] = {
    "de los Cuervos", "Martillo Rojo", "Sombras Negras", 
    "Llama Azul", "de la Luna", "Diente de Hierro",
    "Corazón de Roble", "Tormenta de Plata", "del Valle Sombrío", 
    "Escama de Dragón"
};


const char* clases[] = {
    "Guerreros", "Magos", "Arqueros", "Clérigos", "Bárbaros",
    "Asesinos", "Paladines", "Druidas", "Hechiceros", "Alquimistas"
};
void generar_registro_aleatorio(RegistroDatos *reg, int id) {
    reg->id = id;
    snprintf(reg->nombre, MAX_NOMBRE, "%s %s",
             nombres[rand() % 10], apellidos[rand() % 10]);
    snprintf(reg->clase, MAX_CLASE, "%s",
             clases[rand() % 10]);
    reg->nivel = 1 + rand() % 10;
    reg->oro = 1000 + (rand() % 9000);
}

void proceso_generador(int generador_id, int shmid, int semid) {
    printf("🏭 Generador %d: Iniciando...\n", generador_id);
    
    srand(time(NULL) + generador_id * 1000 + getpid());
    
    MemoriaCompartida *shm = (MemoriaCompartida*) shmat(shmid, NULL, 0);
    if (shm == (MemoriaCompartida*) -1) {
        perror("Generador: Error attachando memoria compartida");
        exit(1);
    }
    
    int ids_asignados[IDS_POR_LOTE];
    int cantidad_ids = 0;
    int idx_actual = 0;
    int registros_enviados = 0;
    
    while (1) {
        // Si no tengo IDs, solicitar
        if (idx_actual >= cantidad_ids) {
            sem_wait(semid, SEM_MUTEX);
            
            // Verificar si hay trabajo
            if (shm->registros_generados >= shm->total_registros_objetivo) {
                sem_signal(semid, SEM_MUTEX);
                break;
            }
            
            // Esperar turno para solicitar
            while (shm->estado != ESTADO_VACIO) {
                sem_signal(semid, SEM_MUTEX);
                usleep(1000 * generador_id); // Espera diferenciada por generador
                sem_wait(semid, SEM_MUTEX);
                
                if (shm->registros_generados >= shm->total_registros_objetivo) {
                    sem_signal(semid, SEM_MUTEX);
                    goto salir;
                }
            }
            
            // Solicitar IDs
            shm->solicitud.generador_id = generador_id;
            shm->solicitud.cantidad_solicitada = IDS_POR_LOTE;
            shm->estado = ESTADO_SOLICITUD_IDS;
            
            sem_signal(semid, SEM_COORD);
            sem_signal(semid, SEM_MUTEX);
            
            // Esperar respuesta
            int intentos = 0;
            while (intentos < 100) {
                sem_wait(semid, SEM_GEN_BASE + (generador_id - 1));
                sem_wait(semid, SEM_MUTEX);
                
                if (shm->estado == ESTADO_IDS_LISTOS && shm->respuesta.generador_id == generador_id) {
                    cantidad_ids = shm->respuesta.cantidad;
                    memcpy(ids_asignados, shm->respuesta.ids, cantidad_ids * sizeof(int));
                    idx_actual = 0;
                    shm->estado = ESTADO_VACIO;
                    
                    printf("🏭 Generador %d: Recibidos %d IDs\n", generador_id, cantidad_ids);
                    
                    sem_signal(semid, SEM_MUTEX);
                    break;
                }
                
                sem_signal(semid, SEM_MUTEX);
                usleep(10000); // Esperar 10ms antes de reintentar
                intentos++;
            }
            
            if (cantidad_ids == 0) {
                break;
            }
        }
        
        // Generar y enviar registro
        if (idx_actual < cantidad_ids) {
            RegistroDatos nuevo_registro;
            generar_registro_aleatorio(&nuevo_registro, ids_asignados[idx_actual]);
            
            sem_wait(semid, SEM_MUTEX);
            
            // Esperar turno para enviar
            while (shm->estado != ESTADO_VACIO) {
                sem_signal(semid, SEM_MUTEX);
                usleep(1000);
                sem_wait(semid, SEM_MUTEX);
            }
            
            // Enviar registro
            memcpy(&shm->registro_actual, &nuevo_registro, sizeof(RegistroDatos));
            shm->generador_actual = generador_id;
            shm->estado = ESTADO_NUEVO_REGISTRO;
            
            sem_signal(semid, SEM_COORD);
            sem_signal(semid, SEM_MUTEX);
            
            // Esperar confirmación con timeout
            int confirmado = 0;
            for (int i = 0; i < 100; i++) {
                sem_wait(semid, SEM_GEN_BASE + (generador_id - 1));
                sem_wait(semid, SEM_MUTEX);
                if (shm->estado == ESTADO_REGISTRO_PROCESADO) {
                    registros_enviados++;
                    shm->estado = ESTADO_VACIO;
                    confirmado = 1;
                    sem_signal(semid, SEM_MUTEX);
                    break;
                }
                sem_signal(semid, SEM_MUTEX);
                usleep(10000); // Esperar 10ms antes de reintentar
            }
            
            if (!confirmado) {
                printf("🏭 Generador %d: Timeout en confirmación de registro\n", generador_id);
            }
            
            idx_actual++;
            usleep(1000 + rand() % 2000);
        }
    }
    
    salir:
    // Notificar finalización
    sem_wait(semid, SEM_MUTEX);
    while (shm->estado != ESTADO_VACIO) {
        sem_signal(semid, SEM_MUTEX);
        usleep(1000);
        sem_wait(semid, SEM_MUTEX);
    }
    shm->estado = ESTADO_FINALIZAR;
    sem_signal(semid, SEM_COORD);
    sem_signal(semid, SEM_MUTEX);
    
    shmdt(shm);
    printf("🏭 Generador %d: Terminando. Registros: %d\n", generador_id, registros_enviados);
}