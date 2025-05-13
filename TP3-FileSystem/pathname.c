
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Verificar que es una ruta absoluta
    if (pathname == NULL || pathname[0] != '/') {
        return -1;
    }
    
    // Empezar desde el directorio raíz
    int current_inumber = ROOT_INUMBER;
    
    // Si la ruta es solo "/", devolver el inodo raíz
    if (pathname[1] == '\0') {
        return current_inumber;
    }
    
    // Copiar la ruta para no modificar el original
    char path_copy[1024];
    strncpy(path_copy, pathname + 1, sizeof(path_copy) - 1); // Omitir el primer '/'
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    // Tokenizar la ruta
    char *token = strtok(path_copy, "/");
    
    while (token != NULL) {
        struct direntv6 dir_entry;
        
        // Buscar el componente actual en el directorio actual
        if (directory_findname(fs, token, current_inumber, &dir_entry) < 0) {
            return -1;  // No se encontró
        }
        
        // Actualizar el inumber actual
        current_inumber = dir_entry.d_inumber;
        
        // Pasar al siguiente componente
        token = strtok(NULL, "/");
    }
    
    return current_inumber;
}