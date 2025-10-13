#include "protocolo.h"
#include <termios.h>
#include <signal.h>
#include <sys/time.h>

void mostrar_ayuda_cliente() {
    printf("📋 USO: ./cliente [opciones]\n\n");
    printf("OPCIONES:\n");
    printf("  -h               Mostrar esta ayuda\n");
    printf("  -s servidor      IP del servidor (default: 127.0.0.1)\n");
    printf("  -p puerto        Puerto del servidor (default: 8080)\n\n");
    printf("EJEMPLOS:\n");
    printf("  ./cliente                    # Conectar a localhost:8080\n");
    printf("  ./cliente -s 192.168.1.100   # Conectar a IP específica\n");
    printf("  ./cliente -p 9090            # Conectar a puerto específico\n\n");
    printf("COMANDOS EN EL CLIENTE:\n");
    printf("  SELECT [criterio]            # Buscar registros\n");
    printf("  INSERT datos                 # Insertar registro\n");
    printf("  UPDATE id datos              # Modificar registro\n");
    printf("  DELETE id                    # Eliminar registro\n");
    printf("  BEGIN TRANSACTION            # Iniciar transacción\n");
    printf("  COMMIT TRANSACTION           # Confirmar transacción\n");
    printf("  ROLLBACK TRANSACTION         # Revertir transacción\n");
    printf("  HELP                         # Ayuda en servidor\n");
    printf("  QUIT                         # Desconectar\n\n");
}

void mostrar_ejemplos() {
    printf("💡 EJEMPLOS DE COMANDOS:\n\n");
    printf("🔍 CONSULTAS:\n");
    printf("  SELECT                       # Mostrar todos los registros\n");
    printf("  SELECT Thaldrin              # Buscar registros que contengan 'Thaldrin'\n");
    printf("  SELECT Guerrero              # Buscar registros de la clase Guerrero\n\n");
    printf("➕ INSERTAR:\n");
    printf("  INSERT Carlos Guerrero,Guerrero,15,25000\n");
    printf("  INSERT Ana Lopez,Maga,8,18500\n\n");
    printf("✏️  ACTUALIZAR:\n");
    printf("  UPDATE 5 Elena Sombras,Asesina,25,45000\n");
    printf("  UPDATE 10 Borak Martillo,Paladin,30,78000\n\n");
    printf("❌ ELIMINAR:\n");
    printf("  DELETE 15                    # Eliminar registro con ID 15\n\n");
    printf("🔒 TRANSACCIONES:\n");
    printf("  BEGIN TRANSACTION            # Iniciar\n");
    printf("  INSERT ...                   # Hacer cambios\n");
    printf("  UPDATE ...                   # Más cambios\n");
    printf("  COMMIT TRANSACTION           # Confirmar todo\n");
    printf("  ROLLBACK TRANSACTION         # Deshacer cambios\n\n");
}

void configurar_terminal() {
    // Configurar terminal para entrada interactiva
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ECHO | ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void mostrar_prompt(int en_transaccion) {
    if (en_transaccion) {
        printf("🔒 [TRANSACCION]> ");
    } else {
        printf("📊 [CSV-DB]> ");
    }
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    char servidor_ip[64] = "127.0.0.1";
    int puerto = 8080;
    
    signal(SIGPIPE, SIG_IGN);  // Ignorar SIGPIPE para evitar terminación abrupta al enviar a socket cerrado
    
    // Procesar argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            mostrar_ayuda_cliente();
            return 0;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strcpy(servidor_ip, argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            puerto = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--examples") == 0) {
            mostrar_ejemplos();
            return 0;
        }
    }
    
    printf("🌐 Cliente CSV Database\n");
    printf("📡 Conectando a %s:%d...\n\n", servidor_ip, puerto);
    
    // Crear socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Error creando socket");
        return 1;
    }
    
    // Configurar dirección del servidor
    struct sockaddr_in servidor_addr;
    memset(&servidor_addr, 0, sizeof(servidor_addr));
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_port = htons(puerto);
    
    if (inet_pton(AF_INET, servidor_ip, &servidor_addr.sin_addr) <= 0) {
        printf("❌ Dirección IP inválida: %s\n", servidor_ip);
        close(socket_fd);
        return 1;
    }
    
    // Conectar al servidor
    if (connect(socket_fd, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr)) == -1) {
        printf("❌ No se pudo conectar al servidor %s:%d\n", servidor_ip, puerto);
        printf("💡 Asegúrate de que el servidor esté ejecutándose\n");
        close(socket_fd);
        return 1;
    }
    
    // Configurar timeouts para evitar bloqueos indefinidos
    struct timeval timeout;
    timeout.tv_sec = 30;  // 30 segundos timeout
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    printf("✅ Conectado exitosamente!\n\n");
    
    configurar_terminal();
    
    char comando[MAX_COMMAND_LENGTH];
    char respuesta[MAX_RESPONSE_LENGTH];
    int en_transaccion = 0;
    
    // Recibir mensaje de bienvenida
    int bytes = recv(socket_fd, respuesta, MAX_RESPONSE_LENGTH - 1, 0);
    if (bytes > 0) {
        respuesta[bytes] = '\0';
        printf("%s", respuesta);
    }
    
    printf("💡 Escribe '--examples' como comando para ver ejemplos\n");
    printf("💡 Escribe 'QUIT' para salir\n\n");
    
    // Loop principal del cliente
    while (1) {
        mostrar_prompt(en_transaccion);
        
        if (!fgets(comando, sizeof(comando), stdin)) {
            break; // EOF o error
        }
        
        // Remover salto de línea
        comando[strcspn(comando, "\n")] = '\0';
        
        // Comando vacío
        if (strlen(comando) == 0) {
            continue;
        }
        
        // Comandos locales
        if (strcmp(comando, "--examples") == 0) {
            mostrar_ejemplos();
            continue;
        }
        
        if (strcmp(comando, "--help") == 0) {
            mostrar_ayuda_cliente();
            continue;
        }
        
        // Enviar comando al servidor
        strcat(comando, "\n");
        if (send(socket_fd, comando, strlen(comando), 0) == -1) {
            printf("❌ Error enviando comando\n");
            break;
        }
        
        // Recibir respuesta
        bytes = recv(socket_fd, respuesta, MAX_RESPONSE_LENGTH - 1, MSG_DONTWAIT);
        if (bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - reintenta
                usleep(100000); // 100ms
                bytes = recv(socket_fd, respuesta, MAX_RESPONSE_LENGTH - 1, 0);
            }
        }
        
        if (bytes <= 0) {
            if (bytes == 0) {
                printf("🔌 Servidor cerró la conexión\n");
            } else {
                printf("❌ Error de comunicación con el servidor\n");
            }
            break;
        }
        
        respuesta[bytes] = '\0';
        
        // Verificar si es comando QUIT
        if (strstr(comando, "QUIT") == comando || strstr(comando, "quit") == comando) {
            printf("%s", respuesta);
            break;
        }
        
        // Actualizar estado de transacción
        if (strstr(comando, "BEGIN") == comando || strstr(comando, "begin") == comando) {
            if (strstr(respuesta, "iniciada")) {
                en_transaccion = 1;
            }
        } else if (strstr(comando, "COMMIT") == comando || strstr(comando, "commit") == comando) {
            if (strstr(respuesta, "confirmada")) {
                en_transaccion = 0;
            }
        }
        
        // Mostrar respuesta
        printf("%s", respuesta);
        
        // Agregar línea en blanco después de respuestas largas
        if (strlen(respuesta) > 100) {
            printf("\n");
        }
    }
    
    close(socket_fd);
    printf("\n👋 Desconectado del servidor\n");
    return 0;
}