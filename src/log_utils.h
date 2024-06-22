#ifndef __LOG_UTILS_H__
#define __LOG_UTILS_H__

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define ERROR(msg) do {\
        fprintf(stderr, "[%s] ERROR: " msg "%s\n", __FUNCTION__, (errno != 0)? strerror(errno) : "");\
    } while(0)

#define ERROR_FMT(fmt, args...) do {\
        fprintf(stderr, "[%s] ERROR: " fmt "%s\n", __FUNCTION__, args, (errno != 0) ? strerror(errno) : "");\
    } while(0)

#define DEBUG_PRINT(msg) do {\
        printf("[%s] DEBUG: " msg, __FUNCTION__);\
    } while(0)

#define DEBUG_PRINT_FMT(fmt, args...) do {\
        printf("[%s] DEBUG: " fmt, __FUNCTION__, args);\
    } while(0)

#define DEBUG_PRINT_EXPR(expr) do {\
        printf("[%s] DEBUG: " #expr  " = %d\n", __FUNCTION__, (expr));\
    } while(0)

#endif