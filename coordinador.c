#include "shared_memory.h"

void proceso_coordinador(int shmid, int semid) {
    printf("👑 Coordinador: Iniciando...\n");
    
    MemoriaCompartida *shm = (MemoriaCompartida*) shmat(shmid, NULL, 0);
    if (shm == (MemoriaCompartida*) -1) {
        perror("Coordinador: Error attachando memoria compartida");
        exit(1);
    }
    
    // Crear archivo CSV
    FILE *csv_file = fopen("datos_generados.csv", "w");
    if (!csv_file) {
        perror("Coordinador: Error creando archivo CSV");
        exit(1);
    }
    
    fprintf(csv_file, "ID,Nombre,Email,Edad,Salario,Departamento\n");
    fflush(csv_file);
    printf("👑 Coordinador: Archivo CSV creado\n");
    
    int registros_escritos = 0;
    
    while (1) {
        // Esperar trabajo
        sem_wait(semid, SEM_COORD);
        
        // Sección crítica
        sem_wait(semid, SEM_MUTEX);
        
        if (shm->estado == ESTADO_SOLICITUD_IDS) {
            // Asignar IDs
            int registros_faltantes = shm->total_registros_objetivo - shm->registros_generados;
            int cantidad_a_asignar = shm->solicitud.cantidad_solicitada;
            
            if (cantidad_a_asignar > registros_faltantes) {
                cantidad_a_asignar = registros_faltantes;
            }
            
            shm->respuesta.cantidad = cantidad_a_asignar;
            shm->respuesta.generador_id = shm->solicitud.generador_id;
            
            for (int i = 0; i < cantidad_a_asignar; i++) {
                shm->respuesta.ids[i] = shm->siguiente_id++;
            }
            
            shm->registros_generados += cantidad_a_asignar;
            shm->estado = ESTADO_IDS_LISTOS;
            
            printf("👑 Coordinador: Asignados %d IDs al generador %d\n",
                   cantidad_a_asignar, shm->respuesta.generador_id);
            
            // Despertar TODOS los generadores (simplificado)
            for (int i = 0; i < 10; i++) {  // máximo 10 generadores
                sem_signal(semid, SEM_GEN);
            }
            
        } else if (shm->estado == ESTADO_NUEVO_REGISTRO) {
            // Escribir registro
            fprintf(csv_file, "%d,%s,%s,%d,%.2f,%s\n",
                    shm->registro_actual.id,
                    shm->registro_actual.nombre,
                    shm->registro_actual.email,
                    shm->registro_actual.edad,
                    shm->registro_actual.salario,
                    shm->registro_actual.departamento);
            fflush(csv_file);
            
            registros_escritos++;
            shm->estado = ESTADO_REGISTRO_PROCESADO;
            
            if (registros_escritos % 5 == 0) {
                printf("👑 Coordinador: %d/%d registros escritos\n",
                       registros_escritos, shm->total_registros_objetivo);
            }
            
            // Log cada registro procesado
            printf("📝 Registro ID:%d procesado (Gen:%d) - Total: %d/%d\n",
                   shm->registro_actual.id, shm->generador_actual, 
                   registros_escritos, shm->total_registros_objetivo);
            
            // Despertar generadores (una sola señal)
            sem_signal(semid, SEM_GEN);
            
        } else if (shm->estado == ESTADO_FINALIZAR) {
            shm->generadores_activos--;
            printf("👑 Coordinador: Un generador terminó. Activos: %d\n", shm->generadores_activos);
            
            if (shm->generadores_activos <= 0 || registros_escritos >= shm->total_registros_objetivo) {
                shm->estado = ESTADO_VACIO;
                sem_signal(semid, SEM_MUTEX);
                break;
            }
            shm->estado = ESTADO_VACIO;
        }
        
        sem_signal(semid, SEM_MUTEX);
        
        if (registros_escritos >= shm->total_registros_objetivo) {
            break;
        }
    }
    
    fclose(csv_file);
    shmdt(shm);
    printf("👑 Coordinador: Terminando con %d registros escritos\n", registros_escritos);
}