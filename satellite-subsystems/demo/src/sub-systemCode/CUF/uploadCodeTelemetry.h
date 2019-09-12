#include "../COMM/GSC.h"
#include "CUF.h"

#define fbackupName "CUFbackup"
#define fboolBackupName "boolArraybackup"
#define fheaderName "headerCUF"

#define uploadNameLength 10
#define uploadName "UPLOAD"

#define uploadCodeLength 1024
#define frameLength 256
#define boolArrayLength (uploadCodeLength/frameLength)

void addToArray(TC_spl decode, int framePlace);
void loadBackup();
void saveBackup();
void saveAck(char* err);
void initializeUpload();
void startCUFintegration();
void headerHandle(TC_spl decode);
void removeFiles();
