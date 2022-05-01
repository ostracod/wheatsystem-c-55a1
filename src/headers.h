
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

#ifdef WHEATSYSTEM_AVR
#include "./constants_avr.h"
#endif
#ifdef WHEATSYSTEM_UNIX
#include "./constants_unix.h"
#endif
#include "./constants_common.h"

#ifdef WHEATSYSTEM_AVR
#include "./vars_avr.h"
#endif
#ifdef WHEATSYSTEM_UNIX
#include "./vars_unix.h"
#endif
#include "./vars_common.h"

#ifdef WHEATSYSTEM_AVR
#include "./funcs_avr.h"
#endif
#ifdef WHEATSYSTEM_UNIX
#include "./funcs_unix.h"
#endif
#include "./funcs_common.h"

// HEADER_FILE
#endif


