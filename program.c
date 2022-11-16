#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct __attribute__((__packed__)) {
    uint8_t BS_jmpBoot[ 3 ];    // x86 jump instr. to boot code
    uint8_t BS_OEMName[ 8 ];    // What created the filesystem
    uint16_t BPB_BytsPerSec;    // Bytes per Sector
    uint8_t BPB_SecPerClus;     // Sectors per Cluster
    uint16_t BPB_RsvdSecCnt;    // Reserved Sector Count
    uint8_t BPB_NumFATs;        // Number of copies of FAT
    uint16_t BPB_RootEntCnt;    // FAT12/FAT16: size of root DIR
    uint16_t BPB_TotSec16;      // Sectors, may be 0, see below
    uint8_t BPB_Media;          // Media type, e.g. fixed
    uint16_t BPB_FATSz16;       // Sectors in FAT (FAT12 or FAT16)
    uint16_t BPB_SecPerTrk;     // Sectors per Track
    uint16_t BPB_NumHeads;      // Number of heads in disk
    uint32_t BPB_HiddSec;       // Hidden Sector count
    uint32_t BPB_TotSec32;      // Sectors if BPB_TotSec16 == 0
    uint8_t BS_DrvNum;          // 0 = floppy, 0x80 = hard disk
    uint8_t BS_Reserved1;       //
    uint8_t BS_BootSig;         // Should = 0x29
    uint32_t BS_VolID;          // 'Unique' ID for volume
    uint8_t BS_VolLab[ 11 ];    // Non zero terminated string
    uint8_t BS_FilSysType[ 8 ]; // e.g. 'FAT16 ' (Not 0 term.)
} BootSector;

int fd;
int data;
BootSector bootSector;
uint16_t *startOfFAT;

int main()
{
    printf("Hello World\n");

    fd = open("fat16.img",O_RDONLY);

    BootSector *buf = (BootSector*)malloc(sizeof(BootSector));
    read(fd,buf,sizeof(BootSector));
    bootSector = *buf;
    free(buf);

    printf("BPB_BytsPerSec: %i\n",  bootSector.BPB_BytsPerSec);
    printf("BPB_SecPerClus: %i\n",  bootSector.BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: %i\n",  bootSector.BPB_RsvdSecCnt);
    printf("BPB_NumFATs: %i\n",     bootSector.BPB_NumFATs);
    printf("BPB_RootEntCnt: %i\n",  bootSector.BPB_RootEntCnt);
    printf("BPB_TotSec16: %i\n",    bootSector.BPB_TotSec16);
    printf("BPB_FATSz16: %i\n",     bootSector.BPB_FATSz16);
    printf("BPB_TotSec32: %i\n",    bootSector.BPB_TotSec32);
    for(int i=0;i<11;i++)
    {
        printf("BS_VolLab[%i]: %i\n",i,bootSector.BS_VolLab[i]);
    }

    int offSetValue = bootSector.BPB_BytsPerSec * bootSector.BPB_RsvdSecCnt;

    lseek(fd,offSetValue,SEEK_SET);
    int *bufa = (int*)malloc(sizeof(int));
    read(fd,bufa,sizeof(int));
    printf("%i",*bufa);

    close(fd);

}

