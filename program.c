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
	uint8_t		DIR_Name[ 11 ];		// Non zero terminated string
	uint8_t		DIR_Attr;           // File attributes
	uint8_t		DIR_NTRes;          // Used by Windows NT, ignore
	uint8_t		DIR_CrtTimeTenth;   // Tenths of sec. 0..199
	uint16_t	DIR_CrtTime;        // Creation Time in 2s intervals
	uint16_t	DIR_CrtDate;
	uint16_t	DIR_LstAccDate;     // Date of last read or write
	uint16_t	DIR_FstClusHI;      // Top bits file's 1st cluster
	uint16_t	DIR_WrtTime;        // Time of last write
	uint16_t	DIR_WrtDate;        // Date of last write
	uint16_t	DIR_FstClusLO;      // Lower bits file's 1st cluster
	uint32_t	DIR_FileSize;       // File size in bytes
} DirectoryEntry;

typedef struct LinkedListElement
{
    int16_t value;
    struct LinkedListElement *nextElement;
} LinkedListElement;

typedef struct Date
{
    int day;
    int month;
    int year;
} Date;

typedef struct Time
{
    int seconds;
    int minutes;
    int hours;
} Time;

Date getDateInfo(uint16_t date)
{
    Date resultDate;
    resultDate.day = 0;
    resultDate.month = 0;
    resultDate.year = 1980;

    resultDate.day += ((date >> 0) & 0x1F);     // 00011111
    resultDate.month += ((date >> 5) & 0x0F);   // 00001111
    resultDate.year += ((date >> 9) & 0x7F);    // 01111111

    return(resultDate);
}

Time getTimeInfo(uint16_t time)
{
    Time resultTime;
    resultTime.seconds = 0;
    resultTime.minutes = 0;
    resultTime.hours = 0;

    resultTime.seconds += ((time >> 0) & 0x1F); // 00011111
    resultTime.minutes += ((time >> 5) & 0x3F); // 00111111
    resultTime.hours += ((time >> 11) & 0x1F);  // 00011111

    resultTime.seconds *= 2;

    return(resultTime);
}

//Prints a the given directory entries properties in a row
void printDirectoryEntry(DirectoryEntry givenDE)
{
    Time writeTime = getTimeInfo(givenDE.DIR_WrtTime);
    Date writeDate = getDateInfo(givenDE.DIR_WrtDate);
    printf("    ");
    for(int i=0;i<11;i++)
    {
        printf("%c",givenDE.DIR_Name[i]);
    }
    printf("    ");
    if((givenDE.DIR_Attr >> 0) & 0x01 == 1) printf("R");
    else(printf("-"));
    if((givenDE.DIR_Attr >> 1) & 0x01 == 1) printf("H");
    else(printf("-"));
    if((givenDE.DIR_Attr >> 2) & 0x01 == 1) printf("S");
    else(printf("-"));
    if((givenDE.DIR_Attr >> 3) & 0x01 == 1) printf("V");
    else(printf("-"));
    if((givenDE.DIR_Attr >> 4) & 0x01 == 1) printf("D");
    else(printf("-"));
    if((givenDE.DIR_Attr >> 5) & 0x01 == 1) printf("A");
    else(printf("-"));
    printf("    ");
    printf("%02d:%02d:%02d",writeTime.hours,writeTime.minutes,writeTime.seconds);
    printf("    ");
    printf("%02d/%02d/%04d",writeDate.day,writeDate.month,writeDate.year);
    printf("    ");
    printf("%04X %04X",givenDE.DIR_FstClusHI,givenDE.DIR_FstClusLO);
    printf("    ");
    printf("%i bytes\n",givenDE.DIR_FileSize);
}

BootSector readBootSector(int fd)
{
    BootSector bootSector;
    BootSector* buffer = malloc(sizeof(BootSector));
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
    //Openning the Volume
    int fd = open("fat16.img",O_RDONLY);

    //Reading the BootSector
    BootSector bootSector = readBootSector(fd);

    //FAT OffSet and Size Calculation
    int fATOffSet = bootSector.BPB_BytsPerSec * bootSector.BPB_RsvdSecCnt;
    int fATSectionSize = bootSector.BPB_BytsPerSec * bootSector.BPB_FATSz16;

    //RootDirectory OffSet and Size Calculation
    int rootDirectoryOffSet = fATOffSet + (fATSectionSize * bootSector.BPB_NumFATs);
    int rootDirectorySize = bootSector.BPB_RootEntCnt * sizeof(DirectoryEntry);

    //Data OffSet Calculation
    int dataOffSet = rootDirectoryOffSet + rootDirectorySize;

    //Reading FAT[0]
    lseek(fd,fATOffSet,SEEK_SET);
    int16_t* firstFAT = malloc(fATSectionSize);
    read(fd,firstFAT,fATSectionSize);

    //Reading the RootDirectory
    lseek(fd,rootDirectoryOffSet,SEEK_SET);
    DirectoryEntry *rootDirectory = malloc(rootDirectorySize);
    read(fd,rootDirectory,rootDirectorySize);

    //Printing the Important BootSector Values
    printf("BPB_BytsPerSec: %i\n",  bootSector.BPB_BytsPerSec);
    printf("BPB_SecPerClus: %i\n",  bootSector.BPB_SecPerClus);
    printf("BPB_RsvdSecCnt: %i\n",  bootSector.BPB_RsvdSecCnt);
    printf("BPB_NumFATs: %i\n",     bootSector.BPB_NumFATs);
    printf("BPB_RootEntCnt: %i\n",  bootSector.BPB_RootEntCnt);
    printf("BPB_TotSec16: %i\n",    bootSector.BPB_TotSec16);
    printf("BPB_FATSz16: %i\n",     bootSector.BPB_FATSz16);
    printf("BPB_TotSec32: %i\n",    bootSector.BPB_TotSec32);
    printf("BS_VolLab: {%i",        bootSector.BS_VolLab[0]);
    for(int i=1;i<11;i++)
    {
        printf(",%i",bootSector.BS_VolLab[i]);
    }
    printf("}\n\n");

    //Printing the Root Directory
    printf("    FILE-NAME      ATTR      WRT-TIME    WRT-DATE      FIRST-CLSTR  FILE-SIZE\n");
    for(int i=0;i<bootSector.BPB_RootEntCnt;i++)
    {
        DirectoryEntry currentDE = rootDirectory[i];
        if(
            !(currentDE.DIR_Name[0] == 0x00 ||currentDE.DIR_Name[0] == 0xE5)                             //Don't print in case the first byte of the name is 0x00 or 0xE5
            &&
            !(((currentDE.DIR_Attr >> 0) & 0x0F) == 0x0F && ((currentDE.DIR_Attr >> 4) & 0x03) == 0x00)  //Don't print if the first 4 bits of the attribute are 1 and last 2 bits of the attribute are 0
        )
        {
            printDirectoryEntry(currentDE);
        }

    }

    //Too much to display I think everything is correct.
    /*
    for(int i=0;i<600;i++)
    {
        printf("FAT Array[%i]:%i\n",i,firstFAT[i]);
    }
    */

    //Linked List Test
    /*
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
    */

    free(rootDirectory);
    free(firstFAT);
    close(fd);
}