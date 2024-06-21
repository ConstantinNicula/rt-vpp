#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>

#define PRINT_ERROR(msg) do { \
        perror("Error: " msg); \
        exit(EXIT_FAILURE); \
    } while (0)

#define PRINT_DEBUG(fmt) do {\
        printf("[%s] " fmt, __FUNCTION__);\
    } while(0)

#define PRINT_DEBUG_FMT(fmt, args...) do {\
        printf("[%s] " fmt, __FUNCTION__, args);\
    } while(0)

#define PRINT_DEBUG_EXPR(expr) do {\
        printf("[%s] " #expr  " = %d\n", __FUNCTION__, (expr));\
    } while(0)



#endif