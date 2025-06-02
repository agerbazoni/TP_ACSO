#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int start, status, pid, n;
    int buffer[1];

    if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}

    n = atoi(argv[1]);
    int valor = atoi(argv[2]);
    start = atoi(argv[3]);
    
    buffer[0] = valor;

    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);

    int pipes[n][2];
    int result_pipe[2];
    
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
    
    if (pipe(result_pipe) == -1) {
        perror("pipe result");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            for (int j = 0; j < n; j++) {
                if (j != i) close(pipes[j][1]); 
                if (j != (i + n - 1) % n) close(pipes[j][0]); 
            }
            
            if ((i + 1) % n != start) {
                close(result_pipe[0]);
                close(result_pipe[1]);
            } else {
                close(result_pipe[0]); 
            }
            
            read(pipes[(i + n - 1) % n][0], buffer, sizeof(int));
            buffer[0] += 1;
            
            printf("Proceso %d recibió %d, incrementó a %d\n", i, buffer[0]-1, buffer[0]);

            if ((i + 1) % n == start) {
                write(result_pipe[1], buffer, sizeof(int));
            } else {
                write(pipes[i][1], buffer, sizeof(int));
            }
            
            exit(0);
        }
    }

    for (int i = 0; i < n; i++) {
        close(pipes[i][0]);
        if (i != (start + n - 1) % n) close(pipes[i][1]);
    }
    close(result_pipe[1]); 

    write(pipes[(start + n - 1) % n][1], buffer, sizeof(int));
    close(pipes[(start + n - 1) % n][1]);

    read(result_pipe[0], buffer, sizeof(int));
    printf("El padre recibió el valor final: %d\n", buffer[0]);

    for (int i = 0; i < n; i++) {
        wait(&status);
    }

    return 0;
}