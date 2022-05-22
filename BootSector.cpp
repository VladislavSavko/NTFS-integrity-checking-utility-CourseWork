#pragma once
#include"BootSector.h"
#include"FR.h"
#include <string>
#include<iostream>


#define MAX_FILE_COUNT 100
#define MAX_OPEN_COUNT 1
#define SECTORS_PER_FR 2



using namespace std;

bootSector::bootSector(BYTE* _data)
{
    data = _data;

    memcpy(fileSystemID, data + 0x3, 0x8);
    bytesPerSector = *(WORD*)(data + 0xB);
    sectorsPerCluster = *(BYTE*)(data + 0xD);
    startOfMFT = *(UINT64*)(data + 0x30);

}


INT32 openDisk(string logicDriverName, HANDLE& hDisk)
{
    string add = "\\\\.\\" + logicDriverName + ":";
    UINT32 index = 0;
    for (; index < MAX_OPEN_COUNT; index++)
    {
        hDisk = CreateFileA(add.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, \
            NULL, OPEN_EXISTING, 0, NULL);
        if (hDisk != INVALID_HANDLE_VALUE)
        {
            break;
        }
        cerr << GetLastError() << endl;
    }
    if (index == MAX_OPEN_COUNT)
    {
        fprintf(stderr, "\nopen disk filed in openDisk(), have opened %lld times\nError ID: %lu", (INT64)MAX_OPEN_COUNT, GetLastError());
        return -1;
    }
    return 0;
}


INT32 getData(HANDLE hDisk, UINT64 offset, BYTE* dest, DWORD lenth)
{
    void* p = (void*)&offset;

    SetFilePointer(hDisk, *(LONG*)p, (PLONG)((BYTE*)p + 4), FILE_BEGIN);
    DWORD dwReadLenth = 0;
    INT32 res = 0;
    res = ReadFile(hDisk, dest, lenth, &dwReadLenth, NULL);
    if (res == 0)
    {
        fprintf(stderr, "\nread data failed in getData(), read %lu/%lu byte(s)", dwReadLenth, lenth);
        return -2;
    }
    return 0;
}




INT32 searchFile(HANDLE hDisk, string filename, INT64& offset,int& mftFilesCounter)
{
    mftFilesCounter = 0;
    if (filename[0] == '<')
        filename[0] == ' ';
    offset = 0;
    BYTE data[BYTES_PER_SECTOR * SECTORS_PER_FR];
    INT32 res = 0;
    res = getData(hDisk, 0, data, BYTES_PER_SECTOR);
    if (res != 0)
    {
        fprintf(stderr, "\nget boot sector data filed in searchFile()");
        return res;
    }
    bootSector bs(data);
    INT64 fileCount = 0;
    string sfsID((char*)bs.fileSystemID, 4);
    if (sfsID != "NTFS")
    {
        fprintf(stderr, "\nThis disk is not ntfs format");
        return -1;
    }
    FR* fr = NULL;
    INT64 offsetOfSector = bs.startOfMFT * 8;
    while (fileCount < MAX_FILE_COUNT)
    {
        fileCount++;
        res = getData(hDisk, offsetOfSector * BYTES_PER_SECTOR, data, BYTES_PER_SECTOR * SECTORS_PER_FR);
        if (res != 0)
        {
            fprintf(stderr, "\nget FR data filed in searchFile()");
            return res;
        }
        if (string((char*)data, 4) == "FILE")
        {
            FRHeader FRH(data);
            if (FRH.isExist && !FRH.isDIR)
            {
                fr = new FR(data);
                if (fr->aName != NULL)
                {
                    string fnameUNICODE((char*)fr->aName->content->fileName, fr->aName->content->nameLenth * 2);
                    string fnameASCII;
                    string::size_type index = 0;
                    for (; index < fnameUNICODE.length(); index++)
                    {
                        if (index % 2 == 0)
                        {
                            fnameASCII.push_back(fnameUNICODE[index]);
                        }
                    }
                    fprintf(stdout, "\nfound file %s", fnameASCII.c_str());
                    if (fnameASCII == "$AttrDef" || fnameASCII == "$BadClus" || fnameASCII == "$MFT" || fnameASCII == "$MFTMirr" ||fnameASCII == "$LogFile" ||
                        fnameASCII == "$Bitmap" || fnameASCII == "$Boot" || fnameASCII == "$UpCase" || fnameASCII == "$Repair" || fnameASCII == "$Tops" ||
                        fnameASCII == "$TxfLog.blf" || fnameASCII == "$TxfLogContainer00000000000000000001" ||fnameASCII == "$TxfLogContainer00000000000000000002"||
                        fnameASCII=="$Volume")
                        mftFilesCounter++;
                    if (filename == fnameASCII)
                    {
                        break;
                    }
                }
            }
        }
        offsetOfSector += SECTORS_PER_FR;
    }
    if (fileCount == MAX_FILE_COUNT)
    {
        cout << endl << "All files have been read" << endl;
        return 3;
    }
    offset = offsetOfSector;
    SAFE_RELEASE_SINGLE_POINTER(fr);
    return 0;
}

void bootSector::getInfoAboutNtfs(HANDLE disk, NTFS_VOLUME_DATA_BUFFER* infoBuffer, DWORD bytesRead)
{
    if (!DeviceIoControl(disk, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, infoBuffer, sizeof(NTFS_VOLUME_DATA_BUFFER), &bytesRead, NULL))
    {
        cout << "Error getting information about NTFS!\n";
        return;
    }
    cout << "Information about filesystem:" << endl;
    cout << "Size of one MFT record: " << infoBuffer->BytesPerFileRecordSegment <<" bytes"<< endl;
    cout << "Overall size of MFT: " << infoBuffer->MftValidDataLength.QuadPart << " bytes"<<endl;
    cout << "Maximum amount of MFT records: " << infoBuffer->MftValidDataLength.QuadPart/infoBuffer->BytesPerFileRecordSegment << endl;
    cout << "Total clusters: " << infoBuffer->TotalClusters.QuadPart << endl;
    cout << "Free clusters: " << infoBuffer->FreeClusters.QuadPart << endl;
    cout << "Total reserved clusters: " << infoBuffer->TotalReserved.QuadPart << endl;
    cout << "The starting logical cluster number of the MFT zone: " << infoBuffer->MftZoneStart.QuadPart << endl;
    cout << "The ending logical cluster number of the MFT zone " << infoBuffer->MftZoneEnd.QuadPart << endl;
}