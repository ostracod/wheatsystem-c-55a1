
// Retrieves the number of elements in the given array.
#define getArrayLength(name) (int32_t)(sizeof(name) / sizeof(*name))

// Convenience function to create a system application. systemAppFunctionArray must be a fixed array of systemAppFunction_t.
#define createSystemApp(globalFrameSize, systemAppFunctionArray) (systemApp_t){globalFrameSize, systemAppFunctionArray, getArrayLength(systemAppFunctionArray)}

// Implements the "init" WheatSystem function.
void initializeTermApp();
// Implements the "listenTerm" WheatSystem function.
void setTermObserver();
// Implements the "termSize" WheatSystem function.
void getTermSize();
// Implements the "wrtTerm" WheatSystem function.
void writeTermText();


