
int8_t heapMemory[HEAP_MEMORY_SIZE];

// The Unix path of the file to use for the WheatSystem volume.
int8_t *unixVolumePath;

int8_t *storageSpace;
int8_t storageSpaceIsDirty;
storageOffset_t volumeFileSize;

WINDOW *window;

int8_t isIntegrationTest;
int32_t testSocketHandle;


