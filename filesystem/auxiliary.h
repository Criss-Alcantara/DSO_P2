
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	Last revision 01/04/2020
 *
 */

#include "crc.h"

int ialloc (void);
int alloc (void);
int blockmap(int fileDescriptor, int position);
int syncFS(void);
float ceil_number(float numero);
float floor_number(float numero);