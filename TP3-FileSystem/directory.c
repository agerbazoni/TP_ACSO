#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
  int dirinumber, struct direntv6 *dirEnt) {
struct inode dir_inode;

// Obtener el inodo del directorio
if (inode_iget(fs, dirinumber, &dir_inode) < 0) {
return -1;
}

// Verificar que es un directorio
if ((dir_inode.i_mode & IFMT) != IFDIR) {
return -1;
}

// Obtener el tamaño del directorio
int dir_size = inode_getsize(&dir_inode);

// Número total de entradas en el directorio
int num_entries = dir_size / sizeof(struct direntv6);

// Recorrer cada bloque del directorio
char buf[DISKIMG_SECTOR_SIZE];
struct direntv6 *dir_entries = (struct direntv6 *)buf;
int entries_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct direntv6);

for (int block = 0; block * DISKIMG_SECTOR_SIZE < dir_size; block++) {
// Leer el bloque
int bytes_read = file_getblock(fs, dirinumber, block, buf);
if (bytes_read < 0) {
return -1;
}

// Determinar cuántas entradas hay en este bloque
int entries_in_block = bytes_read / sizeof(struct direntv6);

// Buscar el nombre en las entradas de este bloque
for (int i = 0; i < entries_in_block; i++) {
if (strcmp(dir_entries[i].d_name, name) == 0) {
// Encontrado! Copiar la entrada al resultado
*dirEnt = dir_entries[i];
return 0;
}
}
}

// No se encontró el archivo
return -1;
}