// Get gametime from *.w3g file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "zlib-master/zlib.h"

//#include "stdafx.h"   // not actually needed
//#define ZLIB_WINAPI   // actually actually needed (for linkage)

//#include "windows.h"  // get BYTE et al.
//#include "zlib.h"     // declare the external fns -- uses zconf.h, too
//#pragma comment(lib, "zlibwapi.lib") // for access to the DLL
/*
int getMaxCompressedLen( int nLenSrc ) 
{
    int n16kBlocks = (nLenSrc+16383) / 16384; // round up any fraction of a block
    return ( nLenSrc + 6 + (n16kBlocks*5) );
}
int CompressData( const BYTE* abSrc, int nLenSrc, BYTE* abDst, int nLenDst )
{
    z_stream zInfo ={0};
    zInfo.total_in=  zInfo.avail_in=  nLenSrc;
    zInfo.total_out= zInfo.avail_out= nLenDst;
    zInfo.next_in= (BYTE*)abSrc;
    zInfo.next_out= abDst;

    int nErr, nRet= -1;
    nErr= deflateInit( &zInfo, Z_DEFAULT_COMPRESSION ); // zlib function
    if ( nErr == Z_OK ) {
        nErr= deflate( &zInfo, Z_FINISH );              // zlib function
        if ( nErr == Z_STREAM_END ) {
            nRet= zInfo.total_out;
        }
    }
    deflateEnd( &zInfo );    // zlib function
    return( nRet );
}*/
unsigned char* compressed_data_buffer; // Bytes der komprimierten Daten; overall size of decompressed data (excluding header) 40-43
unsigned char* decompressed_data_buffer; // 44-47

int uncompressData(const char* abSrc, int nLenSrc, char* abDst, int nLenDst)
{
    z_stream zInfo ={0};
    zInfo.total_in=  zInfo.avail_in=  nLenSrc;
    zInfo.total_out= zInfo.avail_out= nLenDst;
    zInfo.next_in= (char*)abSrc;
    zInfo.next_out= abDst;

    int nErr, nRet= -1;
    nErr= inflateInit( &zInfo );               // zlib function    
    if ( nErr == Z_OK ) {
        nErr= inflate( &zInfo, Z_SYNC_FLUSH  );     // zlib function vorher: Z_FINISH      
        if ( nErr == Z_OK ) {  // vorher: Z_STREAM_END
            nRet= zInfo.total_out;
        }
    }
    inflateEnd( &zInfo );   // zlib function
    return( nRet ); // -1 or len of output
}

int main(int argc, char* argv[]){

int filedesc = 0;
unsigned char* header_wsubh_buffer = malloc(69); // Bytes des Headers mit Subheader
//unsigned char* compressed_data_buffer; // Bytes der komprimierten Daten; overall size of decompressed data (excluding header) 40-43
//unsigned char* decompressed_data_buffer; // 44-47
unsigned char* tmp_buffer_compressed_data;
unsigned char word[2];
unsigned char dword[4];
ssize_t counts;
size_t i;

if(argc != 2){
    printf("Bitte Replay als Parameter angeben!\n");
    return 0;
}
filedesc = open(argv[1], O_RDONLY);
if(filedesc == -1){
    perror("open");
    exit(EXIT_FAILURE);    
}
counts = read(filedesc, header_wsubh_buffer, 68);
if(counts == -1){
    perror("read");
    exit(EXIT_FAILURE);    
}

// Größe (Anzahl der Bytes) der komprimierten Daten in der Datei
// ===================================================================================================
for(i = 32; i < 36; i++){       
    dword[i-32] = (unsigned)header_wsubh_buffer[i];
}
int size_compressed_data_in_file = (int)(dword[3] << 24 | dword[2] << 16 | dword[1] << 8 | dword[0]);
// ===================================================================================================

// Größe (Anzahl der Bytes) der komprimierten Datenblöcke (alle) ohne Header
// ===================================================================================================
for(i = 40; i < 44; i++){       
    dword[i-40] = (unsigned)header_wsubh_buffer[i];
}
int size_compressed_data = (int)(dword[3] << 24 | dword[2] << 16 | dword[1] << 8 | dword[0]);
// ===================================================================================================

// Anzahl der komprimierten Datenblöcke in der Datei
// ===================================================================================================
for(i = 44; i < 48; i++){       
    dword[i-44] = (unsigned)header_wsubh_buffer[i];
}
int datablock_number = (int)(dword[3] << 24 | dword[2] << 16 | dword[1] << 8 | dword[0]);
// ===================================================================================================

// Lese die Gametime aus
// ===================================================================================================
for(i = 60; i < 64; i++){       
    dword[i-60] = (unsigned)header_wsubh_buffer[i];
}
int gametime = (int)(dword[3] << 24 | dword[2] << 16 | dword[1] << 8 | dword[0]);
double gametimed = (gametime/1000.0)/60.0;
// ===================================================================================================

compressed_data_buffer = malloc(size_compressed_data);  

/* read compressed data */
int bytes_count_compressed_data;
bytes_count_compressed_data = read(filedesc, compressed_data_buffer, size_compressed_data);
if(counts == -1){
    perror("read");
    exit(EXIT_FAILURE);    
}
// Einzelne komprimierte Datenblöcke auslesen
int size_compressed_block;
for(i = 0; i < 2; i++){
    word[i] = (unsigned)compressed_data_buffer[i];
}
size_compressed_block = (int)(word[1] << 8 | word[0]);
int size_decompressed_block_from_header;
int u = 0;
for(i = 2; i < 4; i++){
    word[u] = (unsigned)compressed_data_buffer[i];
    u++;
}
size_decompressed_block_from_header = (int)(word[1] << 8 | word[0]);
tmp_buffer_compressed_data = malloc(size_compressed_block);
for(i = 8; i < size_compressed_block; i++){
    tmp_buffer_compressed_data[i-8] = compressed_data_buffer[i];
}
decompressed_data_buffer = malloc(8192);
int size_decompressed_block = uncompressData(tmp_buffer_compressed_data, size_compressed_block, decompressed_data_buffer, 8192);
char playername[16];
char playername2[16];
int j;
for(j = 0; j < 16; j++){
    if(decompressed_data_buffer[j+6] == '\0'){
        break;
    }
    playername[j] = decompressed_data_buffer[j+6];
}

// Blockgröße dekomprimierter Daten ermitteln für Header und Replay Aktionen & beides zusammen
int decompressed_block_replay_length = size_compressed_data_in_file/datablock_number;
int decompressed_block_header_length = (size_compressed_data/datablock_number)-decompressed_block_replay_length;
int decompressed_block_length = decompressed_block_header_length + decompressed_block_replay_length;



printf("%d, %d\n", size_compressed_block, size_decompressed_block_from_header);

printf("Anzahl Bytes: %zd\n",counts);
printf("Anzahl komprimierte Daten in Datei: %d (+68) %d\n", size_compressed_data_in_file, bytes_count_compressed_data);
printf("Größe der komprimierten Daten (ohne Header): %d\n", size_compressed_data);
printf("Anzahl komprimierte Datenblöcke: %d\n", datablock_number);
printf("Größe komprimierter Block: %d\nGröße dekomprimierter Block: %d\n%d\n\n",size_compressed_block, size_decompressed_block,decompressed_block_length);
printf("Gametime: %f Minuten --->%d, %d, %s\n", gametimed, tmp_buffer_compressed_data[0],decompressed_data_buffer[0], playername);

int o = 0;
for(i = (size_compressed_block*2+8); i < (size_compressed_block*3+8); i++){
    tmp_buffer_compressed_data[o-8] = compressed_data_buffer[i];
    o++;
}
size_decompressed_block = uncompressData(tmp_buffer_compressed_data, size_compressed_block, decompressed_data_buffer, 8192);

free(header_wsubh_buffer);
free(compressed_data_buffer);
free(decompressed_data_buffer);
free(tmp_buffer_compressed_data);

close(filedesc);

return 1;
}

/*
unsigned char byte = *((unsigned char *)&buffer[i-60]);
test[i-60] = (unsigned)byte;
*/
