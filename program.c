#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct __attribute__((__packed__))
{
    uint8_t     BS_jmpBoot[ 3 ];    // x86 jump instr. to boot code
    uint8_t     BS_OEMName[ 8 ];    // What created the filesystem
    uint16_t    BPB_BytsPerSec;     // Bytes per Sector
    uint8_t     BPB_SecPerClus;     // Sectors per Cluster
    uint16_t    BPB_RsvdSecCnt;     // Reserved Sector Count
    uint8_t     BPB_NumFATs;        // Number of copies of FAT
    uint16_t    BPB_RootEntCnt;     // FAT12/FAT16: size of root DIR
    uint16_t    BPB_TotSec16;       // Sectors, may be 0, see below
    uint8_t     BPB_Media;          // Media type, e.g. fixed
    uint16_t    BPB_FATSz16;        // Sectors in FAT (FAT12 or FAT16)
    uint16_t    BPB_SecPerTrk;      // Sectors per Track
    uint16_t    BPB_NumHeads;       // Number of heads in disk
    uint32_t    BPB_HiddSec;        // Hidden Sector count
    uint32_t    BPB_TotSec32;       // Sectors if BPB_TotSec16 == 0
    uint8_t     BS_DrvNum;          // 0 = floppy, 0x80 = hard disk
    uint8_t     BS_Reserved1;       //
    uint8_t     BS_BootSig;         // Should = 0x29
    uint32_t    BS_VolID;           // 'Unique' ID for volume
    uint8_t     BS_VolLab[ 11 ];    // Non zero terminated string
    uint8_t     BS_FilSysType[ 8 ]; // e.g. 'FAT16 ' (Not 0 term.)
} BootSector;

typedef struct __attribute__((__packed__))
{
    uint8_t DIR_Name[ 11 ];     // Non zero terminated string
    uint8_t DIR_Attr;           // File attributes
    uint8_t DIR_NTRes;          // Used by Windows NT, ignore
    uint8_t DIR_CrtTimeTenth;   // Tenths of sec. 0...199
    uint16_t DIR_CrtTime;       // Creation Time in 2s intervals
    uint16_t DIR_CrtDate;       // Date file created
    uint16_t DIR_LstAccDate;    // Date of last read or write
    uint16_t DIR_FstClusHI;     // Top 16 bits file's 1st cluster
    uint16_t DIR_WrtTime;       // Time of last write
    uint16_t DIR_WrtDate;       // Date of last write
    uint16_t DIR_FstClusLO;     // Lower 16 bits file's 1st cluster
    uint32_t DIR_FileSize;      // File size in bytes
} File;

typedef struct LinkedListElement
{
    int16_t value;
    struct LinkedListElement *nextElement;
} LinkedListElement;

BootSector getBootSector(int fd)
{
    BootSector bootSector;
    BootSector *buffer = (BootSector*)malloc(sizeof(BootSector));
    read(fd,buffer,sizeof(BootSector));
    bootSector = *buffer;
    free(buffer);
    return (bootSector);
}

void addToList(struct LinkedListElement *head,int16_t value)
{
    struct LinkedListElement *newElement = NULL;
    newElement = malloc(sizeof(struct LinkedListElement));
    newElement->value = value;
    newElement->nextElement = NULL;
    while(head->nextElement != NULL)
    {
        head = head->nextElement;
    }
    head->nextElement = newElement;
}

void freeList(struct LinkedListElement *head)
{
    struct LinkedListElement *last;
    while(head != NULL)
    {
        last = head;
        head = head->nextElement;
        free(last);
    }
}

void printList(struct LinkedListElement *head)
{
    printf("Linked List Values:\n");
    struct LinkedListElement *last;
    while(head != NULL)
    {
        last = head;
        head = head->nextElement;
        printf("%i\n",last->value);
    }
}

int main()
{
    //printf("Hello World\n");

    int fd = open("fat16.img",O_RDONLY);

    BootSector bootSector = getBootSector(fd);

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

    int fATOffSet = bootSector.BPB_BytsPerSec * bootSector.BPB_RsvdSecCnt;
    int fATSectionSize = bootSector.BPB_BytsPerSec * bootSector.BPB_FATSz16;

    lseek(fd,fATOffSet,SEEK_SET);
    int16_t* firstFAT = (int16_t*)malloc(fATSectionSize);
    read(fd,firstFAT,fATSectionSize);

    int rootDirectoryOffSet = bootSector.BPB_BytsPerSec * (bootSector.BPB_RsvdSecCnt + (bootSector.BPB_NumFATs * bootSector.BPB_FATSz16));
    int rootDirectorySize = bootSector.BPB_BytsPerSec * sizeof(File);

    lseek(fd,rootDirectoryOffSet,SEEK_SET);
    File* rootDirectory = (File*)malloc(sizeof(rootDirectorySize));
    read(fd,rootDirectory,rootDirectorySize);

    printf("First File Name:");
    for(int i=0;i<11;i++)
    {
        printf("%c",rootDirectory[0].DIR_Name[i]);
    }
    printf("\n");

    /*

    Too much to display I think everything is correct.

    for(int i=0;i<600;i++)
    {
        printf("FAT Array[%i]:%i\n",i,firstFAT[i]);
    }
    */
    

    struct LinkedListElement *head = NULL;
    head = malloc(sizeof(struct LinkedListElement));

    head->value = 5;   //Search Head Value
    head->nextElement = NULL;

    int currentValue = head->value;
    while(currentValue>=0)
    {
        addToList(head,firstFAT[currentValue]);
        currentValue = firstFAT[currentValue];
    }
    printList(head);
    freeList(head);

    printf("Size of a file???: %li\n",sizeof(File));
    free(rootDirectory);
    free(firstFAT);
    close(fd);
}