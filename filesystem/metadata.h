
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	Last revision 01/04/2020
 *
 */

#include <stdbool.h>

#define MAX_SIZE_SYSTEM 614400 // Tamaño máximo de los discos que usará el sistema de ficheros
#define MIN_SIZE_SYSTEM 471040 // Tamaño mínimo de los discos que usará el sistema de ficheros
#define MAX_NUM_FILE 10240 // Tamaño máximo de un fichero en el sistema
#define NUMBER_OF_BLOCKS 300
#define NUM_BLOCKS_METADATA 11

typedef struct iNode{
	char name [32]; /* Nombre del fichero/directorio */
	char name_enlace [32]; /* Nombre del fichero/directorio */
	unsigned int tipo; /* 0: Fichero -- 1: Enlace */
	unsigned int directBlock; /* Numero del bloque directo */
	unsigned int fileSize;
	uint32_t crc[5]; //Codigo de redundancia del fichero.
}iNode;

typedef struct iNodes{
	iNode INodos[48];
	char padding[128]; /* Relleno del bloque */
}iNodes;

typedef struct iNodeAux{//Esto está en memoria principal
	bool abierto; /* false = cerrado y 1 = abierto */
	unsigned int position; /* Posicion del puntero de posicion en el fichero */
}iNodeAux;

typedef struct superBlock{
	unsigned int magicNumber; // Número mágico del superbloque: 0x000D5500    
  unsigned int numBlocksINodeMap; // Número de bloques del mapa inodos
  unsigned int numBlocksDatasMap; // Número de bloques del mapa datos
  unsigned int numINodes; // Número de inodos en el dispositivo
  unsigned int firstInodeBlock; //Número de bloque del 1º INodo (Inodo raiz)
	unsigned int numDataBlock; // Número de bloques de datos en el disp.
  unsigned int firstDataBlock; // Número de bloque del 1º bloque de datos 
	unsigned int deviceSize; // Tamaño total del disp. (en bytes)	
  unsigned int firstInodeMAP; //Número de bloque del mapa de bits te los inodos
  unsigned int firstDataMAP; //Número de bloque del mapa de bits de los bloques 	
  short mblocks[NUMBER_OF_BLOCKS];
  char padding[2008]; /* Relleno del bloque */	
}superBlock;

typedef struct iNodeBitMap{//Ocupa 1 bloque    
    	char BitMap[48];//Se reservan 48 bloques porque hay un máximo de 48 ficheros
    	char padding[2000];
}iNodeBitMap;

typedef struct blocksBitMap{//Ocupa 3 bloques   
    	char BitMap[300];
    	char padding[1748];
}blocksBitMap;

#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}
