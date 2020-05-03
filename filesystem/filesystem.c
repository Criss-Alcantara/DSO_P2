
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	Last revision 01/04/2020
 *
 */


#include "filesystem/filesystem.h" // Headers for the core functionality
#include "filesystem/auxiliary.h"  // Headers for auxiliary functions
#include "filesystem/metadata.h"   // Type and structure declaration of the file system
#include "filesystem/crc.h"        // Headers for the CRC functionality

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

superBlock SBlock;
blocksBitMap blocksBitMAP;
iNodeBitMap iNodeBitMAP;
iNodeAux infNodeAux [48];
iNodes iNodos;
bool isMount = false;

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	int i = 0;
    if(deviceSize <= MAX_SIZE_SYSTEM && deviceSize >= MIN_SIZE_SYSTEM){
        
        // inicializar a los valores por defecto del superbloque, mapas e i-nodos
        
        SBlock.magicNumber = 12345;
        SBlock.numINodes = 48;
        SBlock.numDataBlock = 300;
        SBlock.firstInodeBlock = 10;
        SBlock.firstDataBlock = 11; 
        SBlock.firstInodeMAP = 6;
        SBlock.firstDataMAP = 7;

        for (i = 0; i < SBlock.numINodes; i++){           
            bitmap_setbit(iNodeBitMAP.BitMap, i, 0);//free
        }
        for (i = 0; i < SBlock.numDataBlock; i++){            
            if (i > 10)bitmap_setbit(blocksBitMAP.BitMap, i, 0); // free
            else bitmap_setbit(blocksBitMAP.BitMap, i, 1);
        }        
        for (i = 0; i < NUMBER_OF_BLOCKS; i++)
            SBlock.mblocks[i] = -1;

        for (i = 0; i < SBlock.numINodes; i++){
	    infNodeAux[i].abierto = false;
            infNodeAux[i].position = 0;
            iNodos.INodos[i].fileSize = 0;
            iNodos.INodos[i].directBlock = 0;
        }
        // escribir los valores por defecto al disco
        syncFS();
        return 0;
    }
   
    else return -1;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */ 
int mountFS(void)
{ 
	if(isMount == true) return -1;
    // leer bloque 1 de disco en SBlock
    for (int i = 0; i < 6; i++){  
        bread(DEVICE_IMAGE, i,(char *) &(SBlock));
    } 
    // leer los bloques para el mapa de i-nodos
    for (int i = 0; i < SBlock.numBlocksINodeMap; i++){
        bread(DEVICE_IMAGE, SBlock.firstInodeMAP + i, ((char *)iNodeBitMAP.BitMap));//------------------????
    }
    // leer los bloques para el mapa de bloques de datos !!!!!!!!!!!!!!!!!! donde haya acabado el anterior
    for (int i=0; i < SBlock.numBlocksDatasMap; i++){
        bread(DEVICE_IMAGE, SBlock.firstDataMAP + i, ((char *)blocksBitMAP.BitMap));//????????????
    }
    // leer los i-nodos a memoria
    for (int i=0; i<(SBlock.numINodes*sizeof(iNode)/BLOCK_SIZE); i++){
        bread(DEVICE_IMAGE,SBlock.firstInodeBlock + i, ((char *)iNodos.INodos));
    }
    
    isMount = true; 
    return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	if(isMount == false) return -1;
    // asegurarse de que todos los ficheros están cerrados
    for (int i = 0 ; i < SBlock.numINodes; i++) {
        if (infNodeAux->abierto == true) {
            return -1;
        }
    }
    // escribir a disco los metadatos
    syncFS();
    
    isMount = false;
    return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
    if(isMount == false) {
	printf("Error 1");
	return -1;
    } 
    int b_id, inodo_id ;
    for(int i = 0; i < SBlock.numINodes; i++){   
        if(strcmp(iNodos.INodos[i].name,fileName) == 0) {
		printf("Error 2");
		return -1;
	}
    }
    inodo_id = ialloc();

    if (inodo_id < 0) {
	printf("Error 3");
	return -2;
    }
    b_id = alloc();
    if (b_id < 0) return b_id ;
    strcpy(iNodos.INodos[inodo_id].name, fileName);
    iNodos.INodos[inodo_id].directBlock = b_id;
    iNodos.INodos[inodo_id].fileSize = 0;
	iNodos.INodos[inodo_id].tipo = 0;
    infNodeAux[inodo_id].position = 0;
    return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	if(isMount == false) return -1;
    int previousBlock;//Bloque previo de datos.
    int actualBlock;
    int numBlocks;// Numero de bloques que tiene el fichero. 
    for(int i = 0; i < SBlock.numINodes; i++){   
        if (strcmp(iNodos.INodos[i].name,fileName) == 0){   
            numBlocks = (int) ceil_number(iNodos.INodos[i].fileSize/2048); 
            /*Borro el anterior despues de guardar el siguiente y actualizo el anterior al siguiente*/ 
            previousBlock = iNodos.INodos[i].directBlock;
            while(numBlocks != 0){// Devuelvo los valores de los bloques al valor inicial modifico el mapa de bits            
                actualBlock = SBlock.mblocks[previousBlock];//Guardo el actual     
                bitmap_setbit(blocksBitMAP.BitMap, previousBlock, 0); // Libero el mapa de bits        
                SBlock.mblocks[previousBlock] = -1;//Borro la referencia del anterior al actual             
                previousBlock = actualBlock;//Actualizo el previous al actual para la siguiente iteracion            
                numBlocks--;//Resto el bloque he que borrado al numero de bloques
            }
            //Devuelvo todos los atributos a su estado inicial.
            memset(iNodos.INodos[i].name, 0, 32);
            iNodos.INodos[i].fileSize = 0;
            iNodos.INodos[i].directBlock = -1;
            infNodeAux[i].abierto = false;
	    for(int j = 0; j < 5;j++) iNodos.INodos[i].crc[j] = 0;
            lseek(i, 0, FS_SEEK_BEGIN);            
            bitmap_setbit(iNodeBitMAP.BitMap, i, 0);//free
            return 0;
        }
    }
    return -1;  
}
   
/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	if(isMount == false) return -1;
    int inodo_id = -1;   
    for(int i = 0; i < SBlock.numINodes; i++){  
        if (strcmp(iNodos.INodos[i].name,fileName) == 0) {  
			if(iNodos.INodos[i].tipo == 0){
				if(infNodeAux[i].abierto == true) return -1;
				inodo_id = i;
				break;// Si existe el fichero con el nombre buscado, se guarda el numero de inodo y se sale del loop
			}
            
        }
    }
    // buscar el inodo asociado al nombre
    if (inodo_id < 0)
        return inodo_id ;
	
	

    // iniciar sesión de trabajo
    infNodeAux[inodo_id].position = 0;
    infNodeAux[inodo_id].abierto = true;
    return inodo_id;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	if(isMount == false) return -1;
    if(infNodeAux[fileDescriptor].abierto == true){ 
        infNodeAux[fileDescriptor].position = 0;
        infNodeAux[fileDescriptor].abierto = false;
        return 0;
    }
    return -1;
}
 
/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{   
    if(isMount == false)  return -1;
    if(infNodeAux[fileDescriptor].abierto == false) return -1;
    char b[BLOCK_SIZE];
    int b_id ;
    int highestPosition = BLOCK_SIZE;
    int numBytesInBlock = 0; //Numero de bytes que se lee de cada bloque dentro del while
    int numBytesToRead = numBytes;//Numero de bytes a que faltan por escribir;
    int numBytesRead = 0;
    if(numBytes < 0 && numBytes > MAX_NUM_FILE) return -1;//Si los bytes que se quieren leer son un numero valido
    if (infNodeAux[fileDescriptor].position == iNodos.INodos[fileDescriptor].fileSize){//Si el puntero esta al final del fichero
        return 0;
    }
    else if (infNodeAux[fileDescriptor].position+numBytes > iNodos.INodos[fileDescriptor].fileSize && iNodos.INodos[fileDescriptor].fileSize != 0){//Si lo que quieres leer sobrepasa el tamaño del fichero entonces leo todos los bytes que pueda.
        
        numBytesToRead = iNodos.INodos[fileDescriptor].fileSize - infNodeAux[fileDescriptor].position;
    } 
    else numBytesToRead = numBytes;
    while(numBytesToRead != 0){//Mientras queden bytes por leer      
        if(infNodeAux[fileDescriptor].position + numBytesToRead > highestPosition){//Si lo que quieres leer es mayor que el top del bloque entonces leo lo que puedo
            numBytesInBlock = highestPosition - infNodeAux[fileDescriptor].position;
        } 
        else if(numBytes > numBytesToRead) numBytesInBlock = numBytesToRead;
        else numBytesInBlock = numBytes;// Si no leo todo lo que quiero leer.
        
        b_id = blockmap(fileDescriptor, infNodeAux[fileDescriptor].position);//Calculo el bloque que tengo que leer       
        if(bread(DEVICE_IMAGE, b_id, b) == -1) { printf("Error"); return -1;}//Leemos un bloque entero      
        strncat(buffer,b,numBytesInBlock);//Añade b al final de buffer;  
        lseekFile(fileDescriptor, numBytesInBlock, FS_SEEK_CUR);//Actualizo el descriptor
        highestPosition += BLOCK_SIZE;//Pongo el top en el siguiente
        numBytesToRead -= numBytesInBlock;//Resto los bytes que he leido a los que me quedan
        numBytesRead += numBytesInBlock;//Sumo los bytes que he leido a los que habia leido anteriormentefileDescriptor
    }
    
    return numBytesRead;
}  

/*   
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */ 
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
    if(isMount == false) return -1;
    if(infNodeAux[fileDescriptor].abierto == false) return -1;
    char b[BLOCK_SIZE] ;
    int b_id = 0;
    int b_id_last;//anterior b_id
    int highestPosition = BLOCK_SIZE;
    int numBytesInBlock = 0; //Numero de bytes que se lee de cada bloque dentro del while
    int numBytesToWrite = numBytes;//Numero de bytes a que faltan por escribir;
    int numBytesWrite = 0;
    int actualBlockNumber = 0;//Comienza en el primero
    bool other = false;
    if(numBytes < 0) return -1;// Si no el numero de Bytes no es valido se devuelve -1.
    else if(numBytes > MAX_NUM_FILE) numBytesToWrite = MAX_NUM_FILE;// Si supera el maximo se excribe el maximo  
    if (infNodeAux[fileDescriptor].position == MAX_NUM_FILE && iNodos.INodos[fileDescriptor].fileSize != 0){//Si el puntero esta al final, no podemos escribir nada, porque no hay mas especio.     
        return 0;
    }

    while(numBytesToWrite != 0){   
        if(infNodeAux[fileDescriptor].position + numBytesToWrite > highestPosition){
            numBytesInBlock = highestPosition - infNodeAux[fileDescriptor].position;
            other = true;// Como el numero de bytes para escribir supera el limite del bloque lo marcamos
        }
        else numBytesInBlock = numBytesToWrite;  
        if(b_id == 0)b_id = blockmap(fileDescriptor, infNodeAux[fileDescriptor].position);//Si es la primera vez, busco el primer bloque.
        if (b_id == -1) return -1;//Si hay algun error devuelvo -1 
        if(bread(DEVICE_IMAGE, b_id, b) == -1) return -1;//Leemos un bloque entero 
        if(infNodeAux[fileDescriptor].position%2048 == 0)memmove(b,buffer+(BLOCK_SIZE*(actualBlockNumber)),numBytesInBlock);//Copiamos en el buffer lo que queremos escribir.
        else memmove(b+(infNodeAux[fileDescriptor].position-(actualBlockNumber*BLOCK_SIZE)),buffer+(BLOCK_SIZE*(actualBlockNumber)),numBytesInBlock);
        if(bwrite(DEVICE_IMAGE, b_id, b) == -1) return -1;//Lo escribimos
        lseekFile(fileDescriptor, numBytesInBlock, FS_SEEK_CUR);//Actualizamos el puntero
        highestPosition += BLOCK_SIZE;//Actualizamos el top al del siguiente bloque
        numBytesToWrite -= numBytesInBlock;//Restamos los escritos a los que quedan por escribir
        numBytesWrite += numBytesInBlock;//Actualizamos la variable de bytes que hemos leido
        actualBlockNumber++;
        if (other == true){//Como necesitamos otro bloque, lo pedimos y lo conectamos dentro del array   
            b_id_last = b_id;//Guardo el numero del bloque que acabo de escribir
            b_id = alloc();//Calculo el siguiente
            SBlock.mblocks[b_id_last] = b_id;//los conecto
            other = false;
        }
        if(numBytesToWrite == 0)  iNodos.INodos[fileDescriptor].fileSize += numBytesWrite;
    }
    return numBytesWrite;
}
 
/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{  
	if(isMount == false) return -1;
    switch (whence) {
        case FS_SEEK_CUR:    
            infNodeAux[fileDescriptor].position += offset;
            return 0;    
        case FS_SEEK_BEGIN:   
            infNodeAux[fileDescriptor].position = 0;
            return 0;   
        case FS_SEEK_END:    
            infNodeAux[fileDescriptor].position = iNodos.INodos[fileDescriptor].fileSize;;
            return 0;    
        default:
            return -1;
    }
} 

/* 
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile (char * fileName) 
{   
    if(isMount == false) return -1; 
    iNode nodeToTest_CF;
    uint32_t crc_CF = 0;
    int numBlocks_CF;// Numero de bloques que ocupa el fichero.
    char b_CF[BLOCK_SIZE];  
    int blockToRead_CF;
    bool find_file_CF = false;
    for(int i = 0; i < SBlock.numINodes; i++){    
        if(strcmp(iNodos.INodos[i].name, fileName) == 0) {  
	    if(infNodeAux[i].abierto == true) return -2;   
            nodeToTest_CF = iNodos.INodos[i];
	    find_file_CF = true;
            break;
        } 
    }   
    if(find_file_CF == false) return -2;
    numBlocks_CF = (int) ceil_number((float)(nodeToTest_CF.fileSize)/BLOCK_SIZE);
    blockToRead_CF = nodeToTest_CF.directBlock;
    for(int i = 0; i < numBlocks_CF; i++){
        bread(DEVICE_IMAGE, blockToRead_CF, b_CF);
 	crc_CF = CRC32((const unsigned char *)b_CF, BLOCK_SIZE);
	if(nodeToTest_CF.crc[i] != crc_CF) return -1;
        blockToRead_CF = SBlock.mblocks[blockToRead_CF];
    }
   
    return 0; 
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity (char * fileName)
{  
    if(isMount == false) return -1; 
    iNode nodeToTest;    
    uint32_t crc = 0;
    int numBlocks;// Numero de bloques que ocupa el fichero.
    char b[BLOCK_SIZE];
    int blockToRead; 
    bool find_file = false;
    int valor_i = 0;
    for(int i = 0; i < SBlock.numINodes; i++){    
        if(strcmp(iNodos.INodos[i].name, fileName) == 0) {  
            nodeToTest = iNodos.INodos[i];
		valor_i = i;
	    for(int j = 0; j < 5; j++){
		if(nodeToTest.crc[j] > 0) return -2;
 	    }
	    find_file = true;
            break;
        } 
    }   
    if(find_file == false) return -1;
    numBlocks = (int) ceil_number((float)(nodeToTest.fileSize)/BLOCK_SIZE);
    blockToRead = nodeToTest.directBlock;
    for(int i = 0; i < numBlocks; i++){
        bread(DEVICE_IMAGE, blockToRead, b);
	crc = CRC32((const unsigned char *)b, BLOCK_SIZE);
	iNodos.INodos[valor_i].crc[i] = crc;
        blockToRead = SBlock.mblocks[blockToRead];
    }
    return 0;
}
  
/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{
    if(isMount == false) return -1;
    int inodo_id = -1;
    int integridad = 0;
    for(int i = 0; i < SBlock.numINodes; i++){           
        if (strcmp(iNodos.INodos[i].name,fileName) == 0) {    
            if(infNodeAux[i].abierto == true) return -1;
            inodo_id = i;
            break;// Si existe el fichero con el nombre buscado, se guarda el numero de inodo y se sale del loop
        }
    }
    // buscar el inodo asociado al nombre
    if (inodo_id < 0)
        return -1 ;

    // comprobar integridad
    if(checkFile(fileName) == -1) return -2;
    for(int i=0;i<5;i++){
    	integridad += iNodos.INodos[i].crc[i];	
    }
    if(integridad == 0) return -3;
    
    infNodeAux[inodo_id].position = 0;
    infNodeAux[inodo_id].abierto = true;
    return inodo_id;
	return 0;
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
     
    if(isMount == false) return -1;
    iNode nodeToTest;
    uint32_t crc = 0;
    int numBlocks;// Numero de bloques que ocupa el fichero.
    char b[BLOCK_SIZE];
    int blockToRead;
    int valor_i = 0;
    bool find_file = false;
    if(infNodeAux[fileDescriptor].abierto == true){
        for(int i = 0; i < SBlock.numINodes; i++){    
		if(strcmp(iNodos.INodos[fileDescriptor].name,"") != 0) {  
		    nodeToTest = iNodos.INodos[i];
		    valor_i = i;
		    find_file = true;
		    break;
		} 
    	}
    	if(find_file == false) return -1;  
    	numBlocks = (int) ceil_number((float)(nodeToTest.fileSize)/BLOCK_SIZE);
    	blockToRead = nodeToTest.directBlock;
    	for(int i = 0; i < numBlocks; i++){
        	bread(DEVICE_IMAGE, blockToRead, b);
        	blockToRead = SBlock.mblocks[blockToRead];
		crc = CRC32((const unsigned char *)b, BLOCK_SIZE);
		iNodos.INodos[valor_i].crc[i] = crc;
    	}
        infNodeAux[fileDescriptor].position = 0;
        infNodeAux[fileDescriptor].abierto = false;
        return 0;
    }
    return -1;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
    return -1;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
    return -2;
}

// Funciones Auxiliares
int ialloc(){
    
    iNode * itNode;
    int bit = 1;
    
    // buscar un i-nodo libre
    for (int i = 0; i <SBlock.numINodes; i++)
    {
        bit = bitmap_getbit(iNodeBitMAP.BitMap, i);// Devuelve el valor del bit i

        if (bit == 0) {
            // inodo ocupado ahora
            bitmap_setbit(iNodeBitMAP.BitMap, i, 1);//Pone a 1 el bit indicado i
            
            itNode = &iNodos.INodos[i];// Lo saco
            // valores por defecto en el i-nodo
            memset(itNode->name,' ', 32);
            itNode->fileSize = 0;
            itNode->directBlock = 0;
            
            // devolver identificador de i-nodo
            return i;
        }
    }
    return -2;
}

int alloc(){
    
    char b[BLOCK_SIZE];
    int bit = 1;
    // buscar un bloque libre
    for (int i = 0; i <SBlock.numDataBlock; i++)
    {
        bit = bitmap_getbit(blocksBitMAP.BitMap, i);
        if (bit == 0) {
            // bloque ocupado ahora
            bitmap_setbit(blocksBitMAP.BitMap, i, 1);
            // valores por defecto en el bloque
            memset(b, 0, BLOCK_SIZE);
            bwrite(DEVICE_IMAGE, i+NUM_BLOCKS_METADATA, b);//---------------------------?????????????????????
            // devolver identificador del bloque
            return i;
        }
    }
    return -2;
}

int blockmap(int fileDescriptor, int position){//Nos da el bloque donde se encuentra el puntero
    
    int blocksNumber = ceil_number(position+1/BLOCK_SIZE);//Numero de bloques que ocupa el fichero
    int highestPosition = BLOCK_SIZE;
    int block = 0;//Numero del bloque en el que se encuentra el puntero dentro del fichero
    int blockToReturn = iNodos.INodos[fileDescriptor].directBlock;//Bloque de datos que se devuelve.
    
    if(fileDescriptor >= 0 && fileDescriptor < 40 ){
        
        for(int i = 0; i < blocksNumber; i++){
            
            if(position < highestPosition) {
                block = i+1;
                break;
            }
            else highestPosition += BLOCK_SIZE;
        }

        for(int i = 1; i < block; i++){//Nos vamos al array de bloques y recorremos block-1 para saber el bloque que necesitamos.
            
            blockToReturn = SBlock.mblocks[blockToReturn];
        }
        return blockToReturn;
    }
    
    return -1;
}
    
int syncFS(){
    
    // leer bloque 1 de disco en SBlock
    for (int i = 0; i < 6; i++){
        bwrite(DEVICE_IMAGE, i,(char *) &(SBlock));
    }
    // leer los bloques para el mapa de i-nodos
    for (int i = 0; i < SBlock.numBlocksINodeMap; i++){
        bwrite(DEVICE_IMAGE, SBlock.firstInodeMAP + i, ((char *)iNodeBitMAP.BitMap));//------------------????
    }
    // leer los bloques para el mapa de bloques de datos !!!!!!!!!!!!!!!!!! donde haya acabado el anterior
    for (int i=0; i < SBlock.numBlocksDatasMap; i++){
        bwrite(DEVICE_IMAGE, SBlock.firstDataMAP + i, ((char *)blocksBitMAP.BitMap));//????????????
    }
    // leer los i-nodos a memoria
    for (int i=0; i<(SBlock.numINodes*sizeof(iNode)/BLOCK_SIZE); i++){
        bwrite(DEVICE_IMAGE,SBlock.firstInodeBlock + i, ((char *)iNodos.INodos));//---------????????????
    }
    return 0;
    
}

float ceil_number (float number){
    
    if(number > (int)number) number = (int)number+1;
    
    return number;
}

float floor_number (float number){
    
    number = (int) number;
    
    return number;
}
