#include "protocolo.h"
#include <ctype.h>

int verificar_archivo_csv() {
    FILE* archivo = fopen(CSV_FILENAME, "r");
    if (!archivo) {
        return 0;
    }
    fclose(archivo);
    return 1;
}

int ejecutar_select(const char* parametros, char* respuesta) {
    FILE* archivo = fopen(CSV_FILENAME, "r");
    if (!archivo) {
        construir_respuesta_error(respuesta, "No se puede abrir archivo CSV");
        return 0;
    }
    
    char linea[512];
    char resultado[MAX_RESPONSE_LENGTH] = "📊 RESULTADOS:\n";
    int contador = 0;
    int linea_num = 0;
    
    // Leer encabezados
    if (fgets(linea, sizeof(linea), archivo)) {
        strcat(resultado, "📋 ");
        strcat(resultado, linea);
        if (resultado[strlen(resultado)-1] != '\n') strcat(resultado, "\n");
        strcat(resultado, "─────────────────────────────────────────\n");
        linea_num++;
    }
    
    // Si no hay parámetros, mostrar todos los registros
    if (!parametros || strlen(parametros) == 0) {
        while (fgets(linea, sizeof(linea), archivo) && contador < 20) {
            snprintf(resultado + strlen(resultado), 
                    MAX_RESPONSE_LENGTH - strlen(resultado), 
                    "%3d│ %s", ++contador, linea);
            if (resultado[strlen(resultado)-1] != '\n') strcat(resultado, "\n");
        }
    } else {
        // Buscar por criterio simple (contiene texto)
        char criterio[256];
        strcpy(criterio, parametros);
        
        // Convertir criterio a minúsculas para búsqueda case-insensitive
        for (int i = 0; criterio[i]; i++) {
            criterio[i] = tolower(criterio[i]);
        }
        
        while (fgets(linea, sizeof(linea), archivo) && contador < 20) {
            char linea_lower[512];
            strcpy(linea_lower, linea);
            
            // Convertir línea a minúsculas
            for (int i = 0; linea_lower[i]; i++) {
                linea_lower[i] = tolower(linea_lower[i]);
            }
            
            // Verificar si el criterio está en la línea
            if (strstr(linea_lower, criterio) != NULL) {
                snprintf(resultado + strlen(resultado), 
                        MAX_RESPONSE_LENGTH - strlen(resultado), 
                        "%3d│ %s", ++contador, linea);
                if (resultado[strlen(resultado)-1] != '\n') strcat(resultado, "\n");
            }
        }
    }
    
    fclose(archivo);
    
    if (contador == 0) {
        strcat(resultado, "🔍 No se encontraron registros que coincidan\n");
    } else {
        char footer[64];
        snprintf(footer, sizeof(footer), "─────────────────────────────────────────\n📈 Total: %d registros\n", contador);
        strcat(resultado, footer);
    }
    
    strcpy(respuesta, resultado);
    return 1;
}

int obtener_proximo_id() {
    FILE* archivo = fopen(CSV_FILENAME, "r");
    if (!archivo) return 1;
    
    char linea[512];
    int max_id = 0;
    
    // Saltar encabezado
    fgets(linea, sizeof(linea), archivo);
    
    while (fgets(linea, sizeof(linea), archivo)) {
        int id;
        if (sscanf(linea, "%d,", &id) == 1) {
            if (id > max_id) max_id = id;
        }
    }
    
    fclose(archivo);
    return max_id + 1;
}

int ejecutar_insert(const char* parametros, char* respuesta) {
    if (!parametros || strlen(parametros) == 0) {
        construir_respuesta_error(respuesta, 
            "Formato: INSERT nombre,clase,nivel,oro");
        return 0;
    }
    
    // Parsear parámetros
    char params_copia[512];
    strcpy(params_copia, parametros);
    
    char *nombre = strtok(params_copia, ",");
    char *clase = strtok(NULL, ",");
    char *nivel_str = strtok(NULL, ",");
    char *oro_str = strtok(NULL, ",");
    
    if (!nombre || !clase || !nivel_str || !oro_str) {
        construir_respuesta_error(respuesta, 
            "Faltan campos. Formato: INSERT nombre,clase,nivel,oro");
        return 0;
    }
    
    // Validar tipos
    int nivel = atoi(nivel_str);
    float oro = atof(oro_str);
    
    if (nivel <= 0 || nivel > 100) {
        construir_respuesta_error(respuesta, "nivel debe estar entre 1 y 100");
        return 0;
    }
    
    if (oro <= 0) {
        construir_respuesta_error(respuesta, "Oro debe ser mayor a 0");
        return 0;
    }
    
    // Obtener próximo ID
    int nuevo_id = obtener_proximo_id();
    
    // Abrir archivo para append
    FILE* archivo = fopen(CSV_FILENAME, "a");
    if (!archivo) {
        construir_respuesta_error(respuesta, "No se puede escribir en archivo CSV");
        return 0;
    }
    
    // Escribir nuevo registro (formato: ID,Nombre,Clase,Nivel,Oro)
    fprintf(archivo, "%d,%s,%s,%d,%.2f\n", 
            nuevo_id, nombre, clase, nivel, oro);
    
    fclose(archivo);
    
    snprintf(respuesta, MAX_RESPONSE_LENGTH, 
        "Registro insertado exitosamente con ID %d", nuevo_id);
    
    return 1;
}

int ejecutar_update(const char* parametros, char* respuesta) {
    if (!parametros || strlen(parametros) == 0) {
        construir_respuesta_error(respuesta, 
            "Formato: UPDATE id nombre,clase,nivel,oro");
        return 0;
    }
    
    // Parsear ID
    char params_copia[512];
    strcpy(params_copia, parametros);
    
    char *id_str = strtok(params_copia, " ");
    char *resto = strtok(NULL, "");
    
    if (!id_str || !resto) {
        construir_respuesta_error(respuesta, 
            "Formato: UPDATE id nombre,clase,nivel,oro");
        return 0;
    }
    
    int id_objetivo = atoi(id_str);
    
    // Parsear campos nuevos
    char *nombre = strtok(resto, ",");
    char *clase = strtok(NULL, ",");
    char *nivel_str = strtok(NULL, ",");
    char *oro_str = strtok(NULL, ",");
    
    if (!nombre || !clase || !nivel_str || !oro_str) {
        construir_respuesta_error(respuesta, "Faltan campos para actualizar");
        return 0;
    }
    
    // Validar tipos
    int nivel = atoi(nivel_str);
    float oro = atof(oro_str);
    
    if (nivel <= 0 || nivel > 100) {
        construir_respuesta_error(respuesta, "nivel debe estar entre 1 y 100");
        return 0;
    }
    
    if (oro <= 0) {
        construir_respuesta_error(respuesta, "Oro debe ser mayor a 0");
        return 0;
    }
    
    // Crear archivo temporal
    FILE* original = fopen(CSV_FILENAME, "r");
    FILE* temporal = fopen("temp_update.csv", "w");
    
    if (!original || !temporal) {
        construir_respuesta_error(respuesta, "Error accediendo archivos");
        if (original) fclose(original);
        if (temporal) fclose(temporal);
        return 0;
    }
    
    char linea[512];
    int encontrado = 0;
    int linea_num = 0;
    
    while (fgets(linea, sizeof(linea), original)) {
        if (linea_num == 0) {
            // Copiar encabezado
            fputs(linea, temporal);
        } else {
            int id_actual;
            if (sscanf(linea, "%d,", &id_actual) == 1 && id_actual == id_objetivo) {
                // Reemplazar línea con formato correcto: ID,Nombre,Clase,Nivel,Oro
                fprintf(temporal, "%d,%s,%s,%d,%.2f\n",
                        id_objetivo, nombre, clase, nivel, oro);
                encontrado = 1;
            } else {
                // Copiar línea original
                fputs(linea, temporal);
            }
        }
        linea_num++;
    }
    
    fclose(original);
    fclose(temporal);
    
    if (encontrado) {
        rename("temp_update.csv", CSV_FILENAME);
        snprintf(respuesta, MAX_RESPONSE_LENGTH, 
            "Registro ID %d actualizado exitosamente", id_objetivo);
        return 1;
    } else {
        unlink("temp_update.csv");
        snprintf(respuesta, MAX_RESPONSE_LENGTH, 
            "No se encontró registro con ID %d", id_objetivo);
        return 0;
    }
}

int ejecutar_delete(const char* parametros, char* respuesta) {
    if (!parametros || strlen(parametros) == 0) {
        construir_respuesta_error(respuesta, "Formato: DELETE id");
        return 0;
    }
    
    int id_objetivo = atoi(parametros);
    
    if (id_objetivo <= 0) {
        construir_respuesta_error(respuesta, "ID debe ser mayor a 0");
        return 0;
    }
    
    // Crear archivo temporal
    FILE* original = fopen(CSV_FILENAME, "r");
    FILE* temporal = fopen("temp_delete.csv", "w");
    
    if (!original || !temporal) {
        construir_respuesta_error(respuesta, "Error accediendo archivos");
        if (original) fclose(original);
        if (temporal) fclose(temporal);
        return 0;
    }
    
    char linea[512];
    int encontrado = 0;
    int linea_num = 0;
    
    while (fgets(linea, sizeof(linea), original)) {
        if (linea_num == 0) {
            // Copiar encabezado
            fputs(linea, temporal);
        } else {
            int id_actual;
            if (sscanf(linea, "%d,", &id_actual) == 1 && id_actual == id_objetivo) {
                // Omitir esta línea (eliminar)
                encontrado = 1;
            } else {
                // Copiar línea
                fputs(linea, temporal);
            }
        }
        linea_num++;
    }
    
    fclose(original);
    fclose(temporal);
    
    if (encontrado) {
        rename("temp_delete.csv", CSV_FILENAME);
        snprintf(respuesta, MAX_RESPONSE_LENGTH, 
            "Registro ID %d eliminado exitosamente", id_objetivo);
        return 1;
    } else {
        unlink("temp_delete.csv");
        snprintf(respuesta, MAX_RESPONSE_LENGTH, 
            "No se encontró registro con ID %d", id_objetivo);
        return 0;
    }
}

// Variables globales para transacciones
extern pthread_mutex_t mutex_db;
extern int transaccion_activa;
extern int cliente_con_transaccion;

// Estructura para backup de transacción
typedef struct {
    char backup_file[256];
    int cliente_id;
    time_t inicio;
} BackupTransaccion;

static BackupTransaccion backup_actual = {"", -1, 0};

int iniciar_transaccion(int cliente_id) {
    pthread_mutex_lock(&mutex_db);
    
    if (transaccion_activa) {
        pthread_mutex_unlock(&mutex_db);
        return 0; // Ya hay transacción activa
    }
    
    // Crear backup del archivo CSV antes de modificaciones
    char backup_filename[256];
    snprintf(backup_filename, sizeof(backup_filename), "backup_%d_%ld.csv", cliente_id, time(NULL));
    
    FILE* original = fopen(CSV_FILENAME, "r");
    FILE* backup = fopen(backup_filename, "w");
    
    if (!original || !backup) {
        if (original) fclose(original);
        if (backup) fclose(backup);
        pthread_mutex_unlock(&mutex_db);
        return 0;
    }
    
    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), original)) > 0) {
        fwrite(buffer, 1, bytes, backup);
    }
    
    fclose(original);
    fclose(backup);
    
    // Configurar backup
    strcpy(backup_actual.backup_file, backup_filename);
    backup_actual.cliente_id = cliente_id;
    backup_actual.inicio = time(NULL);
    
    // Crear archivo de bloqueo
    FILE* lock_file = fopen(LOCK_FILENAME, "w");
    if (lock_file) {
        fprintf(lock_file, "%d\n%ld\n", cliente_id, time(NULL));
        fclose(lock_file);
    }
    
    transaccion_activa = 1;
    cliente_con_transaccion = cliente_id;
    
    pthread_mutex_unlock(&mutex_db);
    
    log_cliente(cliente_id, "Transacción iniciada - BD bloqueada con backup");
    return 1;
}

int confirmar_transaccion(int cliente_id) {
    pthread_mutex_lock(&mutex_db);
    
    if (!transaccion_activa || cliente_con_transaccion != cliente_id) {
        pthread_mutex_unlock(&mutex_db);
        return 0;
    }
    
    // Remover archivo de bloqueo
    unlink(LOCK_FILENAME);
    
    // Limpiar backup (transacción exitosa)
    if (strlen(backup_actual.backup_file) > 0) {
        unlink(backup_actual.backup_file);
        strcpy(backup_actual.backup_file, "");
    }
    
    transaccion_activa = 0;
    cliente_con_transaccion = -1;
    
    pthread_mutex_unlock(&mutex_db);
    
    log_cliente(cliente_id, "Transacción confirmada - BD liberada");
    return 1;
}

int rollback_transaccion(int cliente_id) {
    pthread_mutex_lock(&mutex_db);
    
    if (!transaccion_activa || cliente_con_transaccion != cliente_id) {
        pthread_mutex_unlock(&mutex_db);
        return 0;
    }
    
    // Restaurar backup
    if (strlen(backup_actual.backup_file) > 0) {
        if (rename(backup_actual.backup_file, CSV_FILENAME) == 0) {
            log_cliente(cliente_id, "Rollback realizado - BD restaurada");
        } else {
            log_cliente(cliente_id, "ERROR: No se pudo hacer rollback");
        }
        strcpy(backup_actual.backup_file, "");
    }
    
    // Remover archivo de bloqueo
    unlink(LOCK_FILENAME);
    
    transaccion_activa = 0;
    cliente_con_transaccion = -1;
    
    pthread_mutex_unlock(&mutex_db);
    
    return 1;
}