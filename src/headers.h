
#ifndef HEADER_FILE
#define HEADER_FILE

#ifdef WHEATSYSTEM_AVR
#include "./types_avr.h"
#endif
#ifdef WHEATSYSTEM_UNIX
#include "./types_unix.h"
#endif
#include "./types_common.h"

#ifdef WHEATSYSTEM_AVR
#include "./structs_avr.h"
#endif
#ifdef WHEATSYSTEM_UNIX
#include "./structs_unix.h"
#endif
#include "./structs_common.h"

// HEADER_FILE
#endif


