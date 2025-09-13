#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define MAX_CLIENTES_CONCURRENTES 10
#define MAX_CLIENTES_ESPERA 20
#define MAX_COMMAND_LENGTH 1024
#define MAX_RESPONSE_LENGTH 4096
#define CSV_FILENAME "../ejercicio-1/datos_generados.csv"
#define CONFIG_FILENAME "config.txt"
#define LOCK_FILENAME "database.lock"

// Tipos de comandos
typedef enum {
    CMD_SELECT,
    CMD_INSERT, 
    CMD_UPDATE,
    CMD_DELETE,
    CMD_BEGIN_TRANSACTION,
    CMD_COMMIT_TRANSACTION,
    CMD_ROLLBACK_TRANSACTION,
    CMD_QUIT,
    CMD_HELP,
    CMD_UNKNOWN
} TipoComando;

// Estados de transacción
typedef enum {
    TRANS_NONE,      // Sin transacción activa
    TRANS_ACTIVE,    // Transacción en progreso
    TRANS_COMMITTED  // Transacción confirmada
} EstadoTransaccion;

// Estructura para comando parseado
typedef struct {
    TipoComando tipo;
    char parametros[MAX_COMMAND_LENGTH];
    char tabla[64];  // Para futuras expansiones
} ComandoParsed;

// Estructura para cliente conectado
typedef struct {
    int socket_fd;
    int cliente_id;
    struct sockaddr_in direccion;
    EstadoTransaccion estado_trans;
    pthread_t thread;
    time_t tiempo_conexion;
    char ultimo_comando[MAX_COMMAND_LENGTH];
} ClienteInfo;

// Estructura para configuración del servidor
typedef struct {
    char ip[16];
    int puerto;
    int max_clientes;
    int max_espera;
    char csv_file[256];
} ConfigServidor;

// Variables globales del servidor
extern pthread_mutex_t mutex_db;
extern pthread_mutex_t mutex_clientes;
extern int servidor_corriendo;
extern int transaccion_activa;
extern int cliente_con_transaccion;
extern ClienteInfo clientes_conectados[MAX_CLIENTES_CONCURRENTES];
extern int num_clientes_activos;

// Funciones del protocolo
TipoComando parsear_comando(const char* comando, ComandoParsed* parsed);
int enviar_respuesta(int client_fd, const char* mensaje);
int recibir_comando(int client_fd, char* buffer, int max_len);
void construir_respuesta_error(char* respuesta, const char* error);
void construir_respuesta_exito(char* respuesta, const char* datos);

// Funciones de configuración
int cargar_configuracion(ConfigServidor* config);
void mostrar_configuracion(const ConfigServidor* config);

// Funciones de base de datos
int ejecutar_select(const char* parametros, char* respuesta);
int ejecutar_insert(const char* parametros, char* respuesta);
int ejecutar_update(const char* parametros, char* respuesta);
int ejecutar_delete(const char* parametros, char* respuesta);
int verificar_archivo_csv();

// Funciones de transacciones
int iniciar_transaccion(int cliente_id);
int confirmar_transaccion(int cliente_id);
int rollback_transaccion(int cliente_id);
int verificar_transaccion_activa(int cliente_id);

// Funciones de utilidad
void log_servidor(const char* mensaje);
void log_cliente(int cliente_id, const char* mensaje);
char* obtener_timestamp();
void limpiar_string(char* str);

#endif