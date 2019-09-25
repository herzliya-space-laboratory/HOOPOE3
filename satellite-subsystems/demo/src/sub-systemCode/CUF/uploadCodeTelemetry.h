#include "../COMM/GSC.h"
#include "CUF.h"

#define fbackupName "CUFback.cuf"
#define fboolBackupName "BAback.cuf"
#define fheaderName "headCUF.cuf"

#define uploadNameLength 10
#define uploadName "UPLOAD.cuf"

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
