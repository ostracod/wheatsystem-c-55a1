
int8_t heapMem[HEAP_MEM_SIZE];

// The Unix path of the file to use for the WheatSystem volume.
int8_t *unixVolumePath;

int8_t storageSpace[STORAGE_SIZE];
int8_t storageIsDirty;
storageOffset_t volumeFileSize;

WINDOW *window;

int8_t isIntegrationTest;
int32_t testSocketHandle;
int8_t systemShouldHalt;


