#include "shader_utils.h"
#include "glib.h"
#include "log_utils.h"
#include <stdlib.h>
#include <stdio.h>

char* load_shader(const char* shader_path) {
    char* buf = NULL;
    size_t file_size = 0;

    FILE* file = fopen(shader_path, "r");
    if (!file) {
        ERROR_FMT("Failed to open shader file %s \n", shader_path);
        return NULL;
    }

    /* Read entire file contents into memory */
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    buf = malloc(file_size + 1);
    if (!buf) {
        ERROR_FMT("Failed to alloc %ld bytes\n", file_size);
        return NULL;
    }

    fread(buf, 1, file_size, file);
    buf[file_size] = '\0';
    fclose(file);
    
    return buf;
}