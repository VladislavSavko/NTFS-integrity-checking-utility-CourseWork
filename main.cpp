#include"BootSector.h"
#include"FR.h"
#include<string>
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
using namespace std;
#define MAX_FILE_COUNT 100000
#define MAX_OPEN_COUNT 1
#define SECTORS_PER_FR 2

INT32 main(int argc, char** argv)
{
    bool fflag = false;
    if (argc==2)
        fflag = true;
    else if (argc != 3)
    {
        fprintf(stderr, "useage: %s <logicDriverName> <fileName>", argv[0]);
        return -1;
    }

    char* logicDriverName = argv[1];
    char* name = argv[2];
    if (fflag)
        name = new char('<');

    INT32 res = 0;
    INT64 sectorNum = 0;
    NTFS_VOLUME_DATA_BUFFER* infoBuffer=new NTFS_VOLUME_DATA_BUFFER;
    DWORD bytesRead=0;
    int mftFilesCounter = 0;

    fprintf(stdout, "Starting checking file system integrity utility...\n");

    HANDLE hDisk = NULL;
    res = openDisk(string(logicDriverName), hDisk);
    if (res != 0)
    {
        fprintf(stderr, "\nopen disk failed");
        return res;
    }
    bootSector::getInfoAboutNtfs(hDisk, infoBuffer, bytesRead);

    res = searchFile(hDisk, string(name), sectorNum,mftFilesCounter);
    if (res == 0)
    {
        BYTE data[BYTES_PER_SECTOR * SECTORS_PER_FR];
        printf("\nfile record(%s) found in sector %lld",argv[2], sectorNum);

        getData(hDisk, sectorNum * BYTES_PER_SECTOR, data, BYTES_PER_SECTOR * SECTORS_PER_FR);
        FR fr(data);
        if (fr.aData->header->isResident)
        {
            fprintf(stdout, "\n$FILE_NAME attribute in this file isResident\nso the data of this file is in sector %lld", sectorNum);
        }
        else
        {
            fprintf(stdout, "\ncluster(s):");
            vector<clasterFragments>* pcf = &fr.aData->header->run->cf;
            vector<clasterFragments>::iterator it = pcf->begin();
            for (; it != pcf->end(); it++)
            {
                fprintf(stdout, "\nfragment %lld: begin in cluster %llu, lenth %llu", (INT64)(it - pcf->begin()), it->begin, it->lenth);
            }
        }
    }
    else if(!fflag)
    {
        printf("\nfile not found");
    }
    if (mftFilesCounter == 14)
        cout << endl << endl << "All MFT files are found. Checking completed" << endl;
    else cout << endl << endl << "File system is corrupted. Not all MFT files have been found" << endl;

    return 0;
}
