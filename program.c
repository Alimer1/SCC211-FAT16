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

typedef struct __attribute__((__packed__))
{
    uint8_t		LDIR_Ord;		    // Order/ position in sequence
	uint8_t     LDIR_Name1[ 10 ];	// First 5 UNICODE characters
	uint8_t		LDIR_Attr;		    // = ATTR_LONG_NAME
	uint8_t		LDIR_Type;		    // Should = 0
	uint8_t		LDIR_Chksum;		// Checksum of short name
	uint8_t     LDIR_Name2[ 12 ];	// Middle 6 UNICODE characters
	uint16_t	LDIR_FstClusLO;		// MUST be zero
	uint8_t     LDIR_Name3[ 4 ];	// Last 2 UNICODE characters
} LongDirectoryEntry;

typedef union Entry
{
    DirectoryEntry      dE;
    LongDirectoryEntry lDE;
} Entry;

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

typedef struct WholeFile
{
    int     clusterCount;   //Total number of clusters
    void*   data;           //Pointer to the all the clusters combined.
} WholeFile;

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
    printf("0x%04X 0x%04X",givenDE.DIR_FstClusHI,givenDE.DIR_FstClusLO);
    printf("    ");
    printf("%i bytes\n",givenDE.DIR_FileSize);
}

void printDirectory(DirectoryEntry *directoryEntryArray,int directoryElementCount)
{
    printf("    FILE-NAME      ATTR      WRT-TIME    WRT-DATE      FIRST-CLUSTER    FILE-SIZE\n");
    for(int i=0;i<directoryElementCount;i++)
    {
        Entry pastEntry;
        pastEntry.dE = directoryEntryArray[i-1];
        LongDirectoryEntry currentLDE = pastEntry.lDE;
        DirectoryEntry currentDE = directoryEntryArray[i];
        if(!(currentDE.DIR_Name[0] == 0x00 ||currentDE.DIR_Name[0] == 0xE5))
        {
            if((((currentDE.DIR_Attr >> 0) & 0x0F) == 0x0F) && (((currentDE.DIR_Attr >> 4) & 0x03) == 0x00))
            {}
            else
            {
                printDirectoryEntry(currentDE);
            }
        }
    }
}

//Returns the boot sector given an fd
BootSector readBootSector(int fd)
{
    BootSector bootSector;
    BootSector* buffer = malloc(sizeof(BootSector));
    read(fd,buffer,sizeof(BootSector));
    bootSector = *buffer;
    free(buffer);
    return (bootSector);
}

void addToList(LinkedListElement *head,int16_t value)
{
    LinkedListElement *newElement = malloc(sizeof(LinkedListElement));
    newElement->value = value;
    newElement->nextElement = NULL;
    while(head->nextElement != NULL)
    {
        head = head->nextElement;
    }
    head->nextElement = newElement;
}

void freeList(LinkedListElement *head)
{
    LinkedListElement *last;
    while(head != NULL)
    {
        last = head;
        head = head->nextElement;
        free(last);
    }
}

void printList(LinkedListElement *head)
{
    printf("Linked List Values:\n");
    LinkedListElement *last;
    while(head != NULL)
    {
        last = head;
        head = head->nextElement;
        printf("%i\n",last->value);
    }
}

//Returns the a pointer to the location of the given clusterID
uint8_t *getCluster(uint16_t clusterID,BootSector bS,int fd)
{
    int initalOffset = (bS.BPB_BytsPerSec * (bS.BPB_RsvdSecCnt + (bS.BPB_NumFATs * bS.BPB_FATSz16)) + (bS.BPB_RootEntCnt * sizeof(DirectoryEntry)));
    int clusterSize = bS.BPB_BytsPerSec * bS.BPB_SecPerClus;
    int extraOffset = (clusterID - 2) * clusterSize;
    lseek(fd,initalOffset + extraOffset,SEEK_SET);
    uint8_t *cluster = malloc(clusterSize);
    read(fd,cluster,clusterSize);
    return(cluster);
}

WholeFile openFile(char* shortFileName,WholeFile directory,int16_t* fAT,BootSector bS,int fd)
{
    DirectoryEntry *directoryData = directory.data;
    int directorySize = (directory.clusterCount * bS.BPB_SecPerClus * bS.BPB_BytsPerSec)/sizeof(DirectoryEntry);

    int currentValue;
    for(int i=0;i<directorySize;i++)
    {
        int match = 0;
        for(int j=0;j<11;j++)
        {
            if(directoryData[i].DIR_Name[j] == shortFileName[j]) match++;
        }
        if(match == 11)
        {
            currentValue = directoryData[i].DIR_FstClusLO;
        }
    }
    int clusterCount = 0;
    uint8_t *totalclusters = malloc(bS.BPB_SecPerClus*bS.BPB_BytsPerSec);
    while(currentValue>-1)
    {
        printf("Cluster[%i] is: 0x%04X\n",clusterCount,currentValue);
        clusterCount ++;
        totalclusters = realloc(totalclusters,bS.BPB_SecPerClus*bS.BPB_BytsPerSec*clusterCount);
        uint8_t *newCluster = getCluster(currentValue,bS,fd);
        for(int i=0;i<bS.BPB_SecPerClus*bS.BPB_BytsPerSec;i++)
        {
            totalclusters[(totalclusters,bS.BPB_SecPerClus*bS.BPB_BytsPerSec*(clusterCount-1))+i] = newCluster[i];
        }
        currentValue = fAT[currentValue];
    }
    WholeFile output;
    output.clusterCount = clusterCount;
    output.data = totalclusters;
    return(output);
}

int main()
{
    //Openning the Volume
    int fd = open("fat16.img",O_RDONLY);

    //Reading the BootSector
    BootSector bootSector = readBootSector(fd);

    //Calculating the cluster size (this is important for later)
    int clusterSize = bootSector.BPB_SecPerClus * bootSector.BPB_BytsPerSec;

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
    printf("BPB_NumFATs:    %i\n",  bootSector.BPB_NumFATs);
    printf("BPB_RootEntCnt: %i\n",  bootSector.BPB_RootEntCnt);
    printf("BPB_TotSec16:   %i\n",  bootSector.BPB_TotSec16);
    printf("BPB_FATSz16:    %i\n",  bootSector.BPB_FATSz16);
    printf("BPB_TotSec32:   %i\n",  bootSector.BPB_TotSec32);
    printf("BS_VolLab:      {%i",   bootSector.BS_VolLab[0]);
    for(int i=1;i<11;i++)
    {
        printf(",%i",bootSector.BS_VolLab[i]);
    }
    printf("}\n\n");

    //Printing the Root Directory
    printDirectory(rootDirectory,bootSector.BPB_RootEntCnt);

    WholeFile rootFolder;
    rootFolder.data = rootDirectory;
    rootFolder.clusterCount = bootSector.BPB_RootEntCnt*sizeof(DirectoryEntry)/clusterSize;

    //Opening and Printing A Txt File From the root directory
    WholeFile fileA = openFile("SESSIONSTXT",rootFolder,firstFAT,bootSector,fd);
    char *textData = fileA.data;
    for(int i=0;i<fileA.clusterCount*clusterSize;i++)
    {
        printf("%c",textData[i]);
    }
    free(fileA.data);
    
    //Opening a folder and then Printing the folder (hopefully)
    WholeFile folderA = openFile("MAN        ",rootFolder,firstFAT,bootSector,fd);
    DirectoryEntry *dirData = folderA.data;
    printDirectory(dirData,(folderA.clusterCount*clusterSize)/sizeof(DirectoryEntry));

    //Opening a folder inside another folder
    WholeFile folderB = openFile("MAN2       ",folderA,firstFAT,bootSector,fd);
    DirectoryEntry *dirData2 = folderB.data;
    printDirectory(dirData2,(folderB.clusterCount*clusterSize)/sizeof(DirectoryEntry));


    free(folderA.data);
    free(folderB.data);
    free(rootDirectory);
    free(firstFAT);
    close(fd);
}