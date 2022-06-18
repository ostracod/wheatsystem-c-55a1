
#include "./headers.h"

int8_t heapMemory[HEAP_MEMORY_SIZE];

int8_t *unixVolumePath;

int8_t *storageSpace;
int8_t storageSpaceIsDirty = false;
storageOffset_t volumeFileSize;

WINDOW *window = NULL;


