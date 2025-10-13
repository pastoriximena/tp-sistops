#include "protocolo.h"

// Variables globales
pthread_mutex_t mutex_db = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;
int servidor_corriendo = 1;
int transaccion_activa = 0;
int cliente_con_transaccion = -1;
ClienteInfo clientes_conectados[MAX_CLIENTES_CONCURRENTES];
int num_clientes_activos = 0;
ConfigServidor config_global;

// Agregar las nuevas variables globales:
ClienteEnEspera cola_espera[MAX_CLIENTES_ESPERA];
int num_clientes_en_espera = 0;
pthread_mutex_t mutex_cola_espera = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_espacio_disponible = PTHREAD_COND_INITIALIZER;

void mostrar_ayuda_servidor() {
    printf("📋 USO: ./servidor [opciones]\n\n");
    printf("OPCIONES:\n");
    printf("  -h          Mostrar esta ayuda\n");
    printf("  -c archivo  Especificar archivo de configuración\n\n");
    printf("CONFIGURACIÓN:\n");
    printf("  El servidor lee la configuración desde 'config.txt'\n");
    printf("  Si no existe, usa valores por defecto\n\n");
    printf("CONTROLES:\n");
    printf("  Ctrl+C      Terminar servidor de forma limpia\n\n");
}

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n🛑 Recibida señal de terminación...\n");
        servidor_corriendo = 0;
        
        // Desconectar todos los clientes
        pthread_mutex_lock(&mutex_clientes);
        for (int i = 0; i < num_clientes_activos; i++) {
            if (clientes_conectados[i].socket_fd > 0) {
                enviar_respuesta(clientes_conectados[i].socket_fd, 
                    "SERVIDOR: Cerrando por mantenimiento. Desconectando...");
                close(clientes_conectados[i].socket_fd);
            }
        }
        pthread_mutex_unlock(&mutex_clientes);
        
        // Limpiar lock de base de datos si existe
        unlink(LOCK_FILENAME);
        
        printf("✅ Servidor terminado limpiamente\n");
        exit(0);
    }
}

void* manejar_cliente(void* arg) {
    ClienteInfo* cliente = (ClienteInfo*)arg;
    char buffer[MAX_COMMAND_LENGTH];
    char respuesta[MAX_RESPONSE_LENGTH];
    ComandoParsed comando_parsed;
    
    log_cliente(cliente->cliente_id, "Cliente conectado");
    
    // Mensaje de bienvenida
    snprintf(respuesta, sizeof(respuesta),
        "🎉 Bienvenido al servidor de Base de Datos CSV\n"
        "📊 Base de datos: %s\n"
        "💡 Escribe 'HELP' para ver comandos disponibles\n"
        "🔚 Escribe 'QUIT' para desconectar\n", config_global.csv_file);
    enviar_respuesta(cliente->socket_fd, respuesta);
    
    while (servidor_corriendo) {
        // Recibir comando del cliente
        int bytes = recibir_comando(cliente->socket_fd, buffer, MAX_COMMAND_LENGTH);
        if (bytes <= 0) {
            log_cliente(cliente->cliente_id, "Cliente desconectado inesperadamente");
            break;
        }
        
        limpiar_string(buffer);
        strcpy(cliente->ultimo_comando, buffer);
        
        log_cliente(cliente->cliente_id, buffer);
        
        // Parsear comando
        TipoComando tipo = parsear_comando(buffer, &comando_parsed);
        
        // Procesar según tipo de comando
        switch (tipo) {
            case CMD_QUIT:
                enviar_respuesta(cliente->socket_fd, "👋 Hasta luego!");
                log_cliente(cliente->cliente_id, "Desconexión voluntaria");
                goto salir_cliente;
                
            case CMD_HELP:
                snprintf(respuesta, sizeof(respuesta),
                    "📋 COMANDOS DISPONIBLES:\n"
                    "🔍 SELECT <criterio>     - Buscar registros\n"
                    "➕ INSERT <datos>        - Insertar nuevo registro\n"
                    "✏️  UPDATE <id> <datos>   - Modificar registro\n"
                    "❌ DELETE <id>           - Eliminar registro\n"
                    "🔒 BEGIN TRANSACTION     - Iniciar transacción\n"
                    "✅ COMMIT TRANSACTION    - Confirmar transacción\n"
                    "🔄 ROLLBACK TRANSACTION  - Revertir transacción\n"
                    "❓ HELP                  - Mostrar esta ayuda\n"
                    "🔚 QUIT                  - Desconectar\n\n"
                    "💡 Durante transacción tienes bloqueo exclusivo\n"
                    "💡 Usa ROLLBACK para deshacer cambios no confirmados\n");
                enviar_respuesta(cliente->socket_fd, respuesta);
                break;
                
            case CMD_BEGIN_TRANSACTION:
                if (iniciar_transaccion(cliente->cliente_id)) {
                    cliente->estado_trans = TRANS_ACTIVE;
                    enviar_respuesta(cliente->socket_fd, 
                        "🔒 Transacción iniciada. Tienes bloqueo exclusivo de la BD");
                } else {
                    enviar_respuesta(cliente->socket_fd, 
                        "❌ ERROR: Ya hay una transacción activa. Intenta más tarde");
                }
                break;
                
            case CMD_COMMIT_TRANSACTION:
                if (cliente->estado_trans == TRANS_ACTIVE) {
                    if (confirmar_transaccion(cliente->cliente_id)) {
                        cliente->estado_trans = TRANS_NONE;
                        enviar_respuesta(cliente->socket_fd, 
                            "✅ Transacción confirmada y liberada");
                    } else {
                        enviar_respuesta(cliente->socket_fd, 
                            "❌ ERROR: No se pudo confirmar transacción");
                    }
                } else {
                    enviar_respuesta(cliente->socket_fd, 
                        "❌ ERROR: No tienes transacción activa");
                }
                break;
                
            case CMD_ROLLBACK_TRANSACTION:
                if (cliente->estado_trans == TRANS_ACTIVE) {
                    if (rollback_transaccion(cliente->cliente_id)) {
                        cliente->estado_trans = TRANS_NONE;
                        enviar_respuesta(cliente->socket_fd, 
                            "🔄 Transacción revertida - BD restaurada");
                    } else {
                        enviar_respuesta(cliente->socket_fd, 
                            "❌ ERROR: No se pudo hacer rollback");
                    }
                } else {
                    enviar_respuesta(cliente->socket_fd, 
                        "❌ ERROR: No tienes transacción activa");
                }
                break;
                
            case CMD_SELECT:
                if (transaccion_activa && cliente_con_transaccion != cliente->cliente_id) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⏳ ERROR: Hay transacción activa. Reintenta luego");
                } else {
                    if (ejecutar_select(comando_parsed.parametros, respuesta)) {
                        enviar_respuesta(cliente->socket_fd, respuesta);
                    } else {
                        enviar_respuesta(cliente->socket_fd, 
                            "❌ ERROR: No se pudo ejecutar consulta");
                    }
                }
                break;
                
            case CMD_INSERT:
                if (transaccion_activa && cliente_con_transaccion != cliente->cliente_id) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⏳ ERROR: Hay transacción activa. Reintenta luego");
                } else if (!transaccion_activa) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⚠️  ADVERTENCIA: Modificaciones requieren transacción para seguridad");
                } else {
                    if (ejecutar_insert(comando_parsed.parametros, respuesta)) {
                        enviar_respuesta(cliente->socket_fd, respuesta);
                    } else {
                        enviar_respuesta(cliente->socket_fd, 
                            "❌ ERROR: No se pudo insertar registro");
                    }
                }
                break;
                
            case CMD_UPDATE:
                if (transaccion_activa && cliente_con_transaccion != cliente->cliente_id) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⏳ ERROR: Hay transacción activa. Reintenta luego");
                } else if (!transaccion_activa) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⚠️  ADVERTENCIA: Modificaciones requieren transacción para seguridad");
                } else {
                    if (ejecutar_update(comando_parsed.parametros, respuesta)) {
                        enviar_respuesta(cliente->socket_fd, respuesta);
                    } else {
                        enviar_respuesta(cliente->socket_fd, 
                            "❌ ERROR: No se pudo actualizar registro");
                    }
                }
                break;
                
            case CMD_DELETE:
                if (transaccion_activa && cliente_con_transaccion != cliente->cliente_id) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⏳ ERROR: Hay transacción activa. Reintenta luego");
                } else if (!transaccion_activa) {
                    enviar_respuesta(cliente->socket_fd, 
                        "⚠️  ADVERTENCIA: Modificaciones requieren transacción para seguridad");
                } else {
                    if (ejecutar_delete(comando_parsed.parametros, respuesta)) {
                        enviar_respuesta(cliente->socket_fd, respuesta);
                    } else {
                        enviar_respuesta(cliente->socket_fd, 
                            "❌ ERROR: No se pudo eliminar registro");
                    }
                }
                break;
                
            default:
                enviar_respuesta(cliente->socket_fd, 
                    "❓ Comando no reconocido. Escribe 'HELP' para ver opciones");
                break;
        }
    }
    
    salir_cliente:
    // Limpiar transacción si la tenía (rollback automático)
    if (cliente->estado_trans == TRANS_ACTIVE) {
        rollback_transaccion(cliente->cliente_id);
        log_cliente(cliente->cliente_id, "Rollback automático por desconexión");
    }
    
    close(cliente->socket_fd);
    
    // CORRECCIÓN: Remover cliente de la lista
    pthread_mutex_lock(&mutex_clientes);
    int cliente_encontrado = 0;
    for (int i = 0; i < num_clientes_activos; i++) {
        if (clientes_conectados[i].cliente_id == cliente->cliente_id) {
            for (int j = i; j < num_clientes_activos - 1; j++) {
                clientes_conectados[j] = clientes_conectados[j + 1];
            }
            num_clientes_activos--;
            cliente_encontrado = 1;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_clientes);
    
    if (cliente_encontrado) {
        printf("👋 Cliente %d desconectado (%d/%d activos restantes)\n",
               cliente->cliente_id, num_clientes_activos, config_global.max_clientes);
        
        // Notificar al procesador de cola que hay espacio disponible
        pthread_cond_signal(&cond_espacio_disponible);
    }
    
    log_cliente(cliente->cliente_id, "Thread de cliente terminado");
    return NULL;
}

// Función para procesar cola de espera
void* procesador_cola_espera(void* arg) {
    while (servidor_corriendo) {
        pthread_mutex_lock(&mutex_cola_espera);
        
        // Esperar hasta que haya clientes en cola y espacio disponible
        while ((num_clientes_en_espera == 0 || num_clientes_activos >= config_global.max_clientes) && servidor_corriendo) {
            pthread_cond_wait(&cond_espacio_disponible, &mutex_cola_espera);
        }
        
        if (!servidor_corriendo) {
            pthread_mutex_unlock(&mutex_cola_espera);
            break;
        }
        
        // Procesar el primer cliente de la cola
        if (num_clientes_en_espera > 0 && num_clientes_activos < config_global.max_clientes) {
            ClienteEnEspera cliente_espera = cola_espera[0];
            
            // Remover de la cola (mover todos hacia adelante)
            for (int i = 0; i < num_clientes_en_espera - 1; i++) {
                cola_espera[i] = cola_espera[i + 1];
            }
            num_clientes_en_espera--;
            
            pthread_mutex_unlock(&mutex_cola_espera);
            
            // Crear el cliente activo
            pthread_mutex_lock(&mutex_clientes);
            if (num_clientes_activos < config_global.max_clientes) {
                int cliente_id = (time(NULL) + rand()) % 100000;
                
                clientes_conectados[num_clientes_activos].socket_fd = cliente_espera.cliente_fd;
                clientes_conectados[num_clientes_activos].cliente_id = cliente_id;
                clientes_conectados[num_clientes_activos].direccion = cliente_espera.direccion;
                clientes_conectados[num_clientes_activos].estado_trans = TRANS_NONE;
                clientes_conectados[num_clientes_activos].tiempo_conexion = time(NULL);
                
                if (pthread_create(&clientes_conectados[num_clientes_activos].thread, NULL, 
                                  manejar_cliente, &clientes_conectados[num_clientes_activos]) == 0) {
                    pthread_detach(clientes_conectados[num_clientes_activos].thread);
                    num_clientes_activos++;
                    
                    printf("🎉 Cliente de la cola promovido (ID: %d) - %d/%d activos\n",
                           cliente_id, num_clientes_activos, config_global.max_clientes);
                } else {
                    close(cliente_espera.cliente_fd);
                }
            } else {
                close(cliente_espera.cliente_fd);
            }
            pthread_mutex_unlock(&mutex_clientes);
        } else {
            pthread_mutex_unlock(&mutex_cola_espera);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Procesar argumentos
    char config_file[256] = CONFIG_FILENAME;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            mostrar_ayuda_servidor();
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strcpy(config_file, argv[i + 1]);
            i++;
        }
    }
    
    // Cargar configuración
    if (!cargar_configuracion(&config_global)) {
        printf("⚠️  Usando configuración por defecto\n");
    }
    
    mostrar_configuracion(&config_global);
    
    // Verificar que existe el archivo CSV
    if (!verificar_archivo_csv()) {
        printf("❌ ERROR: No se encuentra el archivo CSV: %s\n", config_global.csv_file);
        printf("💡 Ejecuta primero el Ejercicio 1 para generar datos\n");
        return 1;
    }
    
    // Configurar manejador de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // Ignorar SIGPIPE
    
    // Crear socket servidor
    int servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_fd == -1) {
        perror("Error creando socket");
        return 1;
    }
    
    // Configurar reutilización de puerto
    int reuse = 1;
    setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Configurar dirección del servidor
    struct sockaddr_in servidor_addr;
    memset(&servidor_addr, 0, sizeof(servidor_addr));
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_port = htons(config_global.puerto);
    
    if (strlen(config_global.ip) > 0 && strcmp(config_global.ip, "0.0.0.0") != 0) {
        inet_pton(AF_INET, config_global.ip, &servidor_addr.sin_addr);
    } else {
        servidor_addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    // Bind del socket
    if (bind(servidor_fd, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr)) == -1) {
        perror("Error en bind");
        close(servidor_fd);
        return 1;
    }
    
    // Escuchar conexiones
    if (listen(servidor_fd, config_global.max_espera) == -1) {
        perror("Error en listen");
        close(servidor_fd);
        return 1;
    }
    
    // Crear thread para procesar cola de espera
    pthread_t thread_cola;
    pthread_create(&thread_cola, NULL, procesador_cola_espera, NULL);
    
    printf("🚀 Servidor iniciado exitosamente\n");
    printf("🌐 Escuchando en %s:%d\n", 
           (strlen(config_global.ip) > 0 && strcmp(config_global.ip, "0.0.0.0") != 0) ? 
           config_global.ip : "todas las interfaces", config_global.puerto);
    printf("👥 Clientes concurrentes: %d, En espera: %d\n", 
           config_global.max_clientes, config_global.max_espera);
    
    // Loop principal del servidor
    while (servidor_corriendo) {
        struct sockaddr_in cliente_addr;
        socklen_t cliente_addr_len = sizeof(cliente_addr);
        
        int cliente_fd = accept(servidor_fd, (struct sockaddr*)&cliente_addr, &cliente_addr_len);
        if (cliente_fd == -1) {
            if (servidor_corriendo) {
                perror("Error en accept");
            }
            continue;
        }
        
        pthread_mutex_lock(&mutex_clientes);
        int clientes_activos_actual = num_clientes_activos;
        pthread_mutex_unlock(&mutex_clientes);
        
        if (clientes_activos_actual >= config_global.max_clientes) {
            // Verificar si hay espacio en la cola de espera
            pthread_mutex_lock(&mutex_cola_espera);
            if (num_clientes_en_espera >= config_global.max_espera) {
                pthread_mutex_unlock(&mutex_cola_espera);
                
                // Cola de espera también llena
                char mensaje_rechazo[512];
                snprintf(mensaje_rechazo, sizeof(mensaje_rechazo), 
                    "❌ Servidor completamente lleno\n"
                    "👥 Clientes activos: %d/%d\n"
                    "⏳ Cola de espera: %d/%d\n"
                    "🔄 Intenta más tarde\n", 
                    clientes_activos_actual, config_global.max_clientes,
                    num_clientes_en_espera, config_global.max_espera);
                
                send(cliente_fd, mensaje_rechazo, strlen(mensaje_rechazo), 0);
                usleep(500000);
                close(cliente_fd);
                
                printf("🚫 Cliente rechazado: servidor y cola llenos (%s:%d)\n", 
                       inet_ntoa(cliente_addr.sin_addr), ntohs(cliente_addr.sin_port));
                continue;
            }
            
            // Agregar a cola de espera
            cola_espera[num_clientes_en_espera].cliente_fd = cliente_fd;
            cola_espera[num_clientes_en_espera].direccion = cliente_addr;
            cola_espera[num_clientes_en_espera].tiempo_llegada = time(NULL);
            num_clientes_en_espera++;
            
            pthread_mutex_unlock(&mutex_cola_espera);
            
            // Enviar mensaje de espera
            char mensaje_espera[512];
            snprintf(mensaje_espera, sizeof(mensaje_espera), 
                "⏳ En cola de espera (posición %d/%d)\n"
                "👥 Clientes activos: %d/%d\n"
                "💡 Mantén la conexión abierta, serás atendido cuando se libere un espacio\n"
                "🔄 Conectando automáticamente...\n", 
                num_clientes_en_espera, config_global.max_espera,
                clientes_activos_actual, config_global.max_clientes);
            
            send(cliente_fd, mensaje_espera, strlen(mensaje_espera), 0);
            
            printf("⏳ Cliente en cola de espera (posición %d) desde %s:%d\n",
                   num_clientes_en_espera, inet_ntoa(cliente_addr.sin_addr), ntohs(cliente_addr.sin_port));
            
        } else {
            // Hay espacio, conectar inmediatamente
            pthread_mutex_lock(&mutex_clientes);
            int cliente_id = (time(NULL) + rand()) % 100000;
            
            clientes_conectados[num_clientes_activos].socket_fd = cliente_fd;
            clientes_conectados[num_clientes_activos].cliente_id = cliente_id;
            clientes_conectados[num_clientes_activos].direccion = cliente_addr;
            clientes_conectados[num_clientes_activos].estado_trans = TRANS_NONE;
            clientes_conectados[num_clientes_activos].tiempo_conexion = time(NULL);
            
            if (pthread_create(&clientes_conectados[num_clientes_activos].thread, NULL, 
                              manejar_cliente, &clientes_conectados[num_clientes_activos]) != 0) {
                perror("Error creando thread para cliente");
                close(cliente_fd);
                pthread_mutex_unlock(&mutex_clientes);
                continue;
            }
            
            pthread_detach(clientes_conectados[num_clientes_activos].thread);
            num_clientes_activos++;
            pthread_mutex_unlock(&mutex_clientes);
            
            printf("✅ Cliente %d conectado directamente desde %s:%d (%d/%d espacios ocupados)\n",
                   cliente_id, inet_ntoa(cliente_addr.sin_addr), ntohs(cliente_addr.sin_port),
                   num_clientes_activos, config_global.max_clientes);
        }
    }
    
    // Limpiar al final
    pthread_cond_broadcast(&cond_espacio_disponible);
    pthread_join(thread_cola, NULL);
    close(servidor_fd);
    return 0;
}