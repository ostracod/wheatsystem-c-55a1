
#include "./headers.h"

int8_t heapMem[HEAP_MEM_SIZE];

int8_t *unixVolumePath;

int8_t storageSpace[STORAGE_SIZE];
int8_t storageIsDirty;
storageOffset_t volumeFileSize;

WINDOW *window = NULL;

int8_t isIntegrationTest = false;
int32_t testSocketHandle;


