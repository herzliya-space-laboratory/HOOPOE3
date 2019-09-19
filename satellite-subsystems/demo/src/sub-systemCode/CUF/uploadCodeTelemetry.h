#include "../COMM/GSC.h"
#include "CUF.h"

#define fbackupName "CUFback"
#define fboolBackupName "BAback"
#define fheaderName "headCUF"

#define uploadNameLength 10
#define uploadName "UPLOAD"

#define frameLength 184
#define boolArrayLength (uploadCodeLength/frameLength)

void addToArray(TC_spl decode, int framePlace);
void loadBackup();
void saveBackup();
void saveAck(char* err);
void initializeUpload();
void startCUFintegration();
void headerHandle(TC_spl decode);
void removeFiles();
