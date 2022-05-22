#pragma once
#include<Windows.h>
#include<string>
using namespace std;

#define BYTES_PER_SECTOR 512

class bootSector
{
public:
    BYTE* data;

    BYTE fileSystemID[8];
    WORD bytesPerSector;
    BYTE sectorsPerCluster;
    UINT64 startOfMFT;

    bootSector(BYTE* _data);
    static void getInfoAboutNtfs(HANDLE disk, NTFS_VOLUME_DATA_BUFFER* infoBuffer, DWORD bytesRead);
   
}; 
INT32 openDisk(string logicDriverName, HANDLE& hDisk);
INT32 getData(HANDLE hDisk, UINT64 offset, BYTE* dest, DWORD lenth);
INT32 searchFile(HANDLE hDisk, string filename, INT64& offset,int& mftFilesCounter);