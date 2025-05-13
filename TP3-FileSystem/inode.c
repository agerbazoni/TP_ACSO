#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"
#include "diskimg.h"


/**
 * TODO
 */
// int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
//     if (inumber < 1) {
//         // Inodos válidos empiezan en 1
//         return -1;
//     }

//     // Tamaño de un bloque
//     const int BLOCK_SIZE = 512;

//     // Cuántos inodos caben en un bloque
//     int inodes_per_block = BLOCK_SIZE / sizeof(struct inode);

//     // Total de inodos en el sistema de archivos
//     int total_inodes = fs->superblock.s_isize * inodes_per_block;

//     if (inumber > total_inodes) {
//         return -1;
//     }

//     // Número de bloque donde se encuentra el inodo
//     int block_num = ((inumber - 1) / inodes_per_block) + 2;  // +2 por boot block (0) y superbloque (1)

//     // Offset del inodo dentro del bloque
//     int offset = (inumber - 1) % inodes_per_block;

//     // Leer el bloque del disco que contiene los inodos
//     struct inode block[BLOCK_SIZE / sizeof(struct inode)];
//     int bytes_read = diskimg_readsector(fs->dfd, block_num, block); // le pasas bloc_num porque asumimos que un sector es un bloque
//     if (bytes_read == -1) {
//         return -1;  // error al leer del disco
//     }

//     // Copiar el inodo deseado a la estructura de salida
//     memcpy(inp, &block[offset], sizeof(struct inode));

//     return 0;  // éxito
// }

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1) {
        // Inodos válidos empiezan en 1
        return -1;
    }

    // Tamaño de un bloque
    const int BLOCK_SIZE = DISKIMG_SECTOR_SIZE;

    // Cuántos inodos caben en un bloque
    int inodes_per_block = BLOCK_SIZE / sizeof(struct inode);

    // Total de inodos en el sistema de archivos
    int total_inodes = fs->superblock.s_isize * inodes_per_block;

    if (inumber > total_inodes) {
        return -1;
    }

    // Número de bloque donde se encuentra el inodo
    int block_num = ((inumber - 1) / inodes_per_block) + INODE_START_SECTOR;

    // Offset del inodo dentro del bloque
    int offset = (inumber - 1) % inodes_per_block;

    // Leer el bloque del disco que contiene los inodos
    struct inode block[inodes_per_block];
    int bytes_read = diskimg_readsector(fs->dfd, block_num, block);
    if (bytes_read == -1) {
        return -1;  // error al leer del disco
    }

    // Copiar el inodo deseado a la estructura de salida
    *inp = block[offset];

    return 0;  // éxito
}


/**
 * TODO
 */
// int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
//     if (blockNum < 0 || blockNum >= fs->superblock.s_fsize) {
//         return -1;
//     }

//     if ((inp->i_mode & IFMT) == IFDIR) {
//         return -1;
//     }

//     const int N = 512/ sizeof(uint16_t);  // cantidad de direcciones por bloque

//     if (inp->i_mode & ILARG) {
//         // BLOQUES INDIRECTOS SIMPLES
//         if (blockNum < 7 * N) {
//             int indirect_index = blockNum / N;
//             int offset = blockNum % N;

//             int indirect_block = inp->i_addr[indirect_index];
//             if (indirect_block == 0) return -1;

//             uint16_t buffer[N];
//             if (diskimg_readsector(fs->dfd, indirect_block, buffer) == -1) {
//                 return -1;
//             }

//             return buffer[offset];
//         }

//         // BLOQUE DE DOBLE INDIRECCIÓN
//         blockNum -= 7 * N; //que bloque mas alla de los 7*N de indireccion simple quiero

//         int double_indirect_block = inp->i_addr[7];
//         if (double_indirect_block == 0) return -1;

//         uint16_t level1[N];
//         if (diskimg_readsector(fs->dfd, double_indirect_block, level1) == -1) {
//             return -1;
//         }

//         int level1_index = blockNum / N; //que bloque de indireccion simple quiero
//         int level2_index = blockNum % N; //que bloque de datos quiero

//         if (level1_index >= N) return -1;

//         int indirect_block = level1[level1_index]; // bloque de indireccion simple de las N direcciones que tiene level1
//         if (indirect_block == 0) return -1;

//         uint16_t level2[N];
//         if (diskimg_readsector(fs->dfd, indirect_block, level2) == -1) {
//             return -1;
//         }

//         return level2[level2_index]; //level2 es un array que tiene los bloques con los datos
//     }

//     // CASO BÁSICO: acceso directo
//     if (blockNum >= 8) {
//         return -1;
//     }

//     return inp->i_addr[blockNum];
// }
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (blockNum < 0) {
        return -1;
    }

    // Número de entradas en un bloque indirecto
    const int ADDRS_PER_BLOCK = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);

    // Para archivos grandes (con flag ILARG activado)
    if (inp->i_mode & ILARG) {
        // Bloques indirectos simples (primeros 7 punteros)
        if (blockNum < 7 * ADDRS_PER_BLOCK) {
            int indirect_index = blockNum / ADDRS_PER_BLOCK;
            int offset = blockNum % ADDRS_PER_BLOCK;

            int indirect_block = inp->i_addr[indirect_index];
            if (indirect_block == 0) return -1;

            uint16_t buffer[ADDRS_PER_BLOCK];
            if (diskimg_readsector(fs->dfd, indirect_block, buffer) == -1) {
                return -1;
            }

            return buffer[offset];
        }
        
        // Bloque de doble indirección (octavo puntero)
        blockNum -= 7 * ADDRS_PER_BLOCK;
        
        if (blockNum >= ADDRS_PER_BLOCK * ADDRS_PER_BLOCK) {
            return -1;  // Fuera de rango para doble indirección
        }

        int double_indirect_block = inp->i_addr[7];
        if (double_indirect_block == 0) return -1;

        // Primer nivel de indirección
        uint16_t level1[ADDRS_PER_BLOCK];
        if (diskimg_readsector(fs->dfd, double_indirect_block, level1) == -1) {
            return -1;
        }

        int level1_index = blockNum / ADDRS_PER_BLOCK;
        int level2_index = blockNum % ADDRS_PER_BLOCK;

        int indirect_block = level1[level1_index];
        if (indirect_block == 0) return -1;

        // Segundo nivel de indirección
        uint16_t level2[ADDRS_PER_BLOCK];
        if (diskimg_readsector(fs->dfd, indirect_block, level2) == -1) {
            return -1;
        }

        return level2[level2_index];
    } 
    else {
        // Acceso directo (sin ILARG)
        if (blockNum >= 8) {
            return -1;  // Fuera de rango para bloques directos
        }

        return inp->i_addr[blockNum];
    }
}


int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}