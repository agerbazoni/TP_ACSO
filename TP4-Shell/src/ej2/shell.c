#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h> // Necesario para isspace

#define MAX_COMMANDS 200
#define MAX_ARGS 65 // Cambiado de 64 a 65 para 64 argumentos + NULL


static void internal_parse_arguments(char *linea_comando_actual, char *argumentos[], int *contador_argumentos) {
    int count = 0;
    char *p = linea_comando_actual;
    *contador_argumentos = 0;

    while (*p && count < MAX_ARGS - 1) { // MAX_ARGS - 1 para dejar espacio para NULL
        while (*p == ' ' || *p == '\t') p++; // Saltar espacios en blanco
        if (*p == '\0') break;
        
        char *inicio_argumento = p;
        if (*p == '"' || *p == '\'') {
            char comilla = *p;
            p++; // Saltar comilla inicial
            inicio_argumento = p; 
            while (*p && *p != comilla) {
                p++;
            }
            if (*p == comilla) {
                *p = '\0'; // Terminar el argumento en la comilla
                p++;       // Moverse más allá de la comilla (ahora nula)
            } else {
                // Comilla sin cerrar
                fprintf(stderr, "Error: comillas sin cerrar\n");
                *contador_argumentos = -1; // Indicar error
                return;
            }
            // Si no hay comilla de cierre, el argumento es desde inicio_argumento hasta el actual *p 
            // (que podría ser '\0' si se llegó al final de la cadena)
        } else {
            // Argumento sin comillas
            while (*p && *p != ' ' && *p != '\t') {
                p++;
            }
            if (*p == ' ' || *p == '\t') {
                *p = '\0'; // Terminar el argumento
                p++;       // Moverse más allá del delimitador (ahora nulo)
            }
        }
        argumentos[count++] = inicio_argumento;
        
        // Verificar si se excede el límite de argumentos
        if (count >= MAX_ARGS - 1) {
            // Verificar si hay más argumentos después de este
            while (*p == ' ' || *p == '\t') p++;
            if (*p != '\0') {
                fprintf(stderr, "Error: demasiados argumentos\n");
                *contador_argumentos = -1; // Indicar error
                return;
            }
        }
    }
    argumentos[count] = NULL;
    *contador_argumentos = count;
}

int main() {

    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        if (isatty(STDIN_FILENO)) { 
            printf("Shell> ");
            fflush(stdout); 
        }
        /*Reads a line of input from the user from the standard input (stdin) and stores it in the variable command */

        if (fgets(command, sizeof(command), stdin) == NULL) {
            if(isatty(STDIN_FILENO)) printf("\n"); // Salto de línea en modo interactivo antes de salir
            break; // EOF o error
        }

        /* Removes the newline character (\n) from the end of the string stored in command, if present. 
           This is done by replacing the newline character with the null character ('\0').
           The strcspn() function returns the length of the initial segment of command that consists of 
           characters not in the string specified in the second argument ("\n" in this case). */
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "exit") == 0) break;
        /* Tokenizes the command string using the pipe character (|) as a delimiter using the strtok() function. 
           Each resulting token is stored in the commands[] array. 
           The strtok() function breaks the command string into tokens (substrings) separated by the pipe character |. 
           In each iteration of the while loop, strtok() returns the next token found in command. 
           The tokens are stored in the commands[] array, and command_count is incremented to keep track of the number of tokens found. */

        int solo_espacios = 1;
        for(int i=0; command[i] != '\0'; ++i) {
            if (!isspace((unsigned char)command[i])) {
                solo_espacios = 0;
                break;
            }
        }
        if (solo_espacios) {
            command_count = 0;
            memset(commands, 0, sizeof(commands));
            continue;
        }

        // Verificar si hay comillas sin cerrar antes del parsing
        int comillas_abiertas = 0;
        char comilla_actual = '\0';
        for (int i = 0; command[i] != '\0'; i++) {
            if ((command[i] == '"' || command[i] == '\'') && command[i-1] != '\\') {
                if (!comillas_abiertas) {
                    comillas_abiertas = 1;
                    comilla_actual = command[i];
                } else if (command[i] == comilla_actual) {
                    comillas_abiertas = 0;
                    comilla_actual = '\0';
                }
            }
        }
        if (comillas_abiertas) {
            fprintf(stderr, "Error: comillas sin cerrar\n");
            goto end_of_command_processing;
        }

        // Verificar errores de pipe antes del parsing
        // Pipe al inicio
        char *trimmed_start = command;
        while (isspace((unsigned char)*trimmed_start)) trimmed_start++;
        if (*trimmed_start == '|') {
            fprintf(stderr, "Error: comando vacío en pipeline\n");
            goto end_of_command_processing;
        }

        // Pipe al final
        char *trimmed_end = command + strlen(command) - 1;
        while (trimmed_end > command && isspace((unsigned char)*trimmed_end)) trimmed_end--;
        if (trimmed_end >= command && *trimmed_end == '|') {
            fprintf(stderr, "Error: comando vacío en pipeline\n");
            goto end_of_command_processing;
        }

        // Verificar pipes dobles o múltiples
        if (strstr(command, "||") != NULL || strstr(command, "|||") != NULL) {
            fprintf(stderr, "Error: comando vacío en pipeline\n");
            goto end_of_command_processing;
        }

        char *token = strtok(command, "|");
        command_count = 0; 
        while (token != NULL && command_count < MAX_COMMANDS) 
        {
            char *inicio_token_real = token;
            while(isspace((unsigned char)*inicio_token_real)) inicio_token_real++;
            
            char *fin_token_real = inicio_token_real + strlen(inicio_token_real) - 1;
            while(fin_token_real > inicio_token_real && isspace((unsigned char)*fin_token_real)) fin_token_real--;
            *(fin_token_real + 1) = '\0';

            commands[command_count++] = inicio_token_real;
            token = strtok(NULL, "|");
        }

        /* You should start programming from here... */
        
        if (command_count == 0) { 
            goto end_of_command_processing; 
        }

        if (command_count == 1) {
            if (commands[0] == NULL || strlen(commands[0]) == 0) { 
                fprintf(stderr, "Error: comando vacío\n");
                goto end_of_command_processing;
            }

            char command_copy[256];
            strcpy(command_copy, commands[0]);

            char *args[MAX_ARGS]; 
            int arg_count = 0;
            internal_parse_arguments(command_copy, args, &arg_count);

            if (arg_count == -1) { 
                goto end_of_command_processing;
            }

            if (arg_count == 0) {
                fprintf(stderr, "Error: comando vacío\n");
                goto end_of_command_processing;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                goto end_of_command_processing;
            }

            if (pid == 0) { 
                execvp(args[0], args);
                fprintf(stderr, "%s: comando no encontrado\n", args[0]);
                exit(127);
            } else { 
                wait(NULL);
            }

        } else { 
            int pipes[command_count - 1][2]; 
            int error_en_pipeline = 0;

            for (int i = 0; i < command_count - 1; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    error_en_pipeline = 1;
                    for(int k=0; k<i; ++k) {
                        close(pipes[k][0]);
                        close(pipes[k][1]);
                    }
                    break; 
                }
            }

            if (error_en_pipeline) {
                goto end_of_command_processing;
            }
            
            
            int hijos_creados_exitosamente = 0; 

            for (int i = 0; i < command_count; i++) {
                if (isatty(STDIN_FILENO)) { 
                    printf("Command %d: %s\n", i, commands[i]);
                                                              
                }
            
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    error_en_pipeline = 1; 

                    break; 
                }
                
                hijos_creados_exitosamente++; 

                if (pid == 0) { 
                    if (i == 0) { 
                        if (dup2(pipes[i][1], 1) == -1) { perror("dup2"); exit(1); } // Usando 1 para stdout
                    } else if (i == command_count - 1) { 
                        if (dup2(pipes[i-1][0], 0) == -1) { perror("dup2"); exit(1); } // Usando 0 para stdin
                    } else { 
                        if (dup2(pipes[i-1][0], 0) == -1) { perror("dup2"); exit(1); }
                        if (dup2(pipes[i][1], 1) == -1) { perror("dup2"); exit(1); }
                    }

                    
                    for (int j = 0; j < command_count - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    if (commands[i] == NULL || strlen(commands[i]) == 0) {
                        fprintf(stderr, "Error: comando vacío en pipeline\n");
                        exit(1);
                    }
                    
                    char command_copy[256];
                    strcpy(command_copy, commands[i]);

                    char *args[MAX_ARGS]; 
                    int arg_count = 0;    
                    internal_parse_arguments(command_copy, args, &arg_count);

                    if (arg_count == -1) { 
                        exit(1);
                    }

                    if (arg_count == 0) { 
                        fprintf(stderr, "Error: comando vacío en pipeline\n");
                        exit(1);
                    }

                    execvp(args[0], args);
                    fprintf(stderr, "%s: comando no encontrado\n", args[0]); 
                    exit(127); 
                }
            } 

            
            if (!error_en_pipeline || hijos_creados_exitosamente > 0) { 
                 for (int j = 0; j < command_count - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }


            for (int j = 0; j < hijos_creados_exitosamente; j++) {
                wait(NULL);
            }
        }

    end_of_command_processing:;
        command_count = 0;
        memset(commands, 0, sizeof(commands)); 
    
    }
    return 0;
}