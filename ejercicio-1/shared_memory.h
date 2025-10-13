#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wait.h>

#define MAX_RECORD_SIZE 65536
#define MAX_NOMBRE 50
#define MAX_CLASE 50
#define IDS_POR_LOTE 10

// Estructura para un registro de datos
typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    char clase[MAX_CLASE];
    int nivel;
    float oro;
} RegistroDatos;

// Estados para la comunicación
typedef enum {
    ESTADO_VACIO = 0,
    ESTADO_SOLICITUD_IDS,
    ESTADO_IDS_LISTOS,
    ESTADO_NUEVO_REGISTRO,
    ESTADO_REGISTRO_PROCESADO,
    ESTADO_FINALIZAR
} EstadoComunicacion;

// Estructura para solicitar lote de IDs
typedef struct {
    int generador_id;
    int cantidad_solicitada;
} SolicitudIDs;

// Estructura para respuesta de IDs
typedef struct {
    int ids[IDS_POR_LOTE];
    int cantidad;
    int generador_id;
} RespuestaIDs;


typedef struct {
    int ids[IDS_POR_LOTE];
    int usados[IDS_POR_LOTE]; // 0 = no usado, 1 = ya escrito
} LoteAsignado;

// Estructura principal de memoria compartida
typedef struct {
    EstadoComunicacion estado;
    int total_registros_objetivo;
    int registros_generados;
    int siguiente_id;
    
    // Para comunicación de IDs
    SolicitudIDs solicitud;
    RespuestaIDs respuesta;
    
    // Para envío de registros
    RegistroDatos registro_actual;
    int generador_actual;
    
    // Control de finalización
    int generadores_activos;
    int proceso_finalizando;
} MemoriaCompartida;


#define MAX_GENERADORES 10
// Claves para IPC
#define CLAVE_SHM 12345
#define CLAVE_SEM 12346

// Índices de semáforos
#define SEM_MUTEX 0      // Exclusión mutua
#define SEM_COORD 1      // Semáforo coordinador
#define SEM_GEN_BASE 2   // Base para semáforos de generadores (SEM_GEN_0, SEM_GEN_1, ...)
#define NUM_SEMAFOROS (2 + MAX_GENERADORES)

// Funciones principales
void proceso_coordinador(int shmid, int semid);
void proceso_generador(int generador_id, int shmid, int semid);

// Funciones auxiliares para semáforos
void sem_wait(int semid, int sem_num);
void sem_signal(int semid, int sem_num);
int crear_semaforos();
void eliminar_semaforos(int semid);

#endif