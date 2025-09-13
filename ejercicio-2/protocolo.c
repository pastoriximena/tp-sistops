#include "protocolo.h"
#include <ctype.h>

TipoComando parsear_comando(const char* comando, ComandoParsed* parsed) {
    char comando_copia[MAX_COMMAND_LENGTH];
    strcpy(comando_copia, comando);
    
    // Convertir a mayúsculas para comparación
    char *token = strtok(comando_copia, " \t\n");
    if (!token) return CMD_UNKNOWN;
    
    // Convertir primer token a mayúsculas
    for (int i = 0; token[i]; i++) {
        token[i] = toupper(token[i]);
    }
    
    parsed->tipo = CMD_UNKNOWN;
    strcpy(parsed->parametros, "");
    strcpy(parsed->tabla, "registros"); // Por defecto
    
    // Identificar tipo de comando
    if (strcmp(token, "SELECT") == 0) {
        parsed->tipo = CMD_SELECT;
    } else if (strcmp(token, "INSERT") == 0) {
        parsed->tipo = CMD_INSERT;
    } else if (strcmp(token, "UPDATE") == 0) {
        parsed->tipo = CMD_UPDATE;
    } else if (strcmp(token, "DELETE") == 0) {
        parsed->tipo = CMD_DELETE;
    } else if (strcmp(token, "BEGIN") == 0) {
        parsed->tipo = CMD_BEGIN_TRANSACTION;
    } else if (strcmp(token, "COMMIT") == 0) {
        parsed->tipo = CMD_COMMIT_TRANSACTION;
    } else if (strcmp(token, "ROLLBACK") == 0) {
        parsed->tipo = CMD_ROLLBACK_TRANSACTION;
    } else if (strcmp(token, "QUIT") == 0 || strcmp(token, "EXIT") == 0) {
        parsed->tipo = CMD_QUIT;
    } else if (strcmp(token, "HELP") == 0) {
        parsed->tipo = CMD_HELP;
    }
    
    // Extraer parámetros (resto del comando original)
    const char* inicio_params = comando;
    while (*inicio_params && !isspace(*inicio_params)) inicio_params++; // Saltar comando
    while (*inicio_params && isspace(*inicio_params)) inicio_params++;  // Saltar espacios
    
    if (strlen(inicio_params) > 0) {
        strcpy(parsed->parametros, inicio_params);
        limpiar_string(parsed->parametros);
    }
    
    return parsed->tipo;
}

int enviar_respuesta(int client_fd, const char* mensaje) {
    char buffer[MAX_RESPONSE_LENGTH + 10];
    snprintf(buffer, sizeof(buffer), "%s\n> ", mensaje);
    
    int bytes_enviados = send(client_fd, buffer, strlen(buffer), MSG_NOSIGNAL);
    if (bytes_enviados == -1) {
        perror("Error enviando respuesta");
        return 0;
    }
    return 1;
}

int recibir_comando(int client_fd, char* buffer, int max_len) {
    int total_bytes = 0;
    int bytes;
    
    memset(buffer, 0, max_len);
    
    while (total_bytes < max_len - 1) {
        bytes = recv(client_fd, buffer + total_bytes, 1, 0);
        if (bytes <= 0) {
            return bytes; // Error o conexión cerrada
        }
        
        if (buffer[total_bytes] == '\n') {
            buffer[total_bytes] = '\0';
            break;
        }
        
        total_bytes += bytes;
    }
    
    return total_bytes;
}

void construir_respuesta_error(char* respuesta, const char* error) {
    snprintf(respuesta, MAX_RESPONSE_LENGTH, "❌ ERROR: %s", error);
}

void construir_respuesta_exito(char* respuesta, const char* datos) {
    snprintf(respuesta, MAX_RESPONSE_LENGTH, "✅ %s", datos);
}

int cargar_configuracion(ConfigServidor* config) {
    FILE* archivo = fopen(CONFIG_FILENAME, "r");
    
    // Valores por defecto
    strcpy(config->ip, "127.0.0.1");
    config->puerto = 8080;
    config->max_clientes = 5;
    config->max_espera = 10;
    strcpy(config->csv_file, CSV_FILENAME);
    
    if (!archivo) {
        printf("⚠️  No se encontró %s, usando valores por defecto\n", CONFIG_FILENAME);
        return 0;
    }
    
    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        limpiar_string(linea);
        
        if (strlen(linea) == 0 || linea[0] == '#') continue;
        
        char clave[64], valor[192];
        if (sscanf(linea, "%63[^=]=%191s", clave, valor) == 2) {
            limpiar_string(clave);
            limpiar_string(valor);
            
            if (strcmp(clave, "ip") == 0) {
                strcpy(config->ip, valor);
            } else if (strcmp(clave, "puerto") == 0) {
                config->puerto = atoi(valor);
            } else if (strcmp(clave, "max_clientes") == 0) {
                config->max_clientes = atoi(valor);
            } else if (strcmp(clave, "max_espera") == 0) {
                config->max_espera = atoi(valor);
            } else if (strcmp(clave, "csv_file") == 0) {
                strcpy(config->csv_file, valor);
            }
        }
    }
    
    fclose(archivo);
    return 1;
}

void mostrar_configuracion(const ConfigServidor* config) {
    printf("⚙️  CONFIGURACIÓN DEL SERVIDOR:\n");
    printf("   🌐 IP: %s\n", config->ip);
    printf("   🔌 Puerto: %d\n", config->puerto);
    printf("   👥 Máx clientes: %d\n", config->max_clientes);
    printf("   ⏳ Máx espera: %d\n", config->max_espera);
    printf("   📊 Archivo CSV: %s\n\n", config->csv_file);
}

char* obtener_timestamp() {
    static char timestamp[64];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    return timestamp;
}

void log_servidor(const char* mensaje) {
    printf("[%s] [SERVIDOR] %s\n", obtener_timestamp(), mensaje);
}

void log_cliente(int cliente_id, const char* mensaje) {
    printf("[%s] [CLIENTE-%d] %s\n", obtener_timestamp(), cliente_id, mensaje);
}

void limpiar_string(char* str) {
    if (!str) return;
    
    // Remover espacios al inicio
    char* inicio = str;
    while (isspace(*inicio)) inicio++;
    
    // Mover contenido al inicio
    if (inicio != str) {
        memmove(str, inicio, strlen(inicio) + 1);
    }
    
    // Remover espacios al final
    int len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[--len] = '\0';
    }
}