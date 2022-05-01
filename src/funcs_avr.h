
// Retrieves a value within the given fixed array.
// "name" is a fixed array.
// "index" is the offset of byte from which to start reading.
#define readFixedArrayValue(name, index, type) ({int8_t result[sizeof(type)]; memcpy_P(&result, (int8_t *)name + index, sizeof(type)); *(type *)result;})


