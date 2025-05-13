#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * TODO
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode in;
    
    // Obtener el inodo
    if (inode_iget(fs, inumber, &in) < 0) {
        return -1;
    }
    
    // Verificar que el inodo esté asignado
    if (!(in.i_mode & IALLOC)) {
        return -1;
    }
    
    // Obtener el tamaño del archivo
    int filesize = inode_getsize(&in);
    
    // Calcular el primer byte de este bloque
    int firstByteOfBlock = blockNum * DISKIMG_SECTOR_SIZE;
    
    // Si el primer byte está más allá del final del archivo, error
    if (firstByteOfBlock >= filesize) {
        return -1;
    }
    
    // Buscar el número de bloque físico usando el índice lógico
    int physicalBlockNum = inode_indexlookup(fs, &in, blockNum);
    if (physicalBlockNum < 0) {
        return -1;
    }
    
    // Leer el bloque físico del disco
    int bytesRead = diskimg_readsector(fs->dfd, physicalBlockNum, buf);
    if (bytesRead < 0) {
        return -1;
    }
    
    // Calcular cuántos bytes son válidos en este bloque
    int validBytes = filesize - firstByteOfBlock;
    if (validBytes > DISKIMG_SECTOR_SIZE) {
        validBytes = DISKIMG_SECTOR_SIZE;
    }
    
    return validBytes;
}
