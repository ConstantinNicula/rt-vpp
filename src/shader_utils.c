#include "shader_utils.h"
#include "gst/gstplugin.h"
#include "log_utils.h"
#include "hmap.h"

#include <complex.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

static int is_shader(const char* file_name, char** out_shader_name); 
static char* path_join(const char* file_part1, const char* file_part2); 
static char* load_shader_code(const char* shader_path);

/* Hash map which will store all loaded shader (mainly used for deduplication)
   Must be initialized via a call to init_shader_store();
*/
static HashMap_t* shader_store = NULL;

int init_shader_store() { 
    shader_store = create_hash_map();
    CHECK(shader_store != NULL, "Failed to create shader store", RET_ERR);
    return RET_OK;
}

int add_shaders_to_store(const char* shader_folder_path) {
    DIR *dir = NULL;
    struct dirent *dir_entry = NULL;
    int count_loaded = 0;
    dir = opendir(shader_folder_path);
    if (!dir) {
        ERROR_FMT("Failed to open shader folder %s\n", shader_folder_path);
        return RET_ERR;
    }

    /* Load each shader and store the contents in the shader hmap.*/
    while ((dir_entry = readdir(dir)) != NULL) {
        char* shader_name; 
        if (is_shader(dir_entry->d_name, &shader_name) != RET_ERR) {
            /* Compute full path. */
            char *full_path = path_join(shader_folder_path, dir_entry->d_name);
            DEBUG_PRINT_FMT("Found shader [%s] at path: %s\n",shader_name, full_path);

            /* Load shader code and add to store. */
            char *shader_code = load_shader_code(full_path); 
            if (shader_code) { 
                hash_map_insert(shader_store, shader_name, shader_code);
                count_loaded++;
            } 
            /* Free malloc'd data, hashmap dups keys by default*/
            free(shader_name);
            free(full_path);
        }
        is_shader(dir_entry->d_name, NULL);
    }

    closedir(dir);

    /* Log something when we failed to load any shaders...*/
    if (count_loaded == 0) {
        ERROR_FMT("Failed to find any .glsl shaders in folder: %s\n", shader_folder_path);
        return RET_ERR;
    }

    return RET_OK;
}

const char* get_shader_code(const char* shader_name) {
    return hash_map_get(shader_store, shader_name);
}


static void _free_shader_code(void** elem) {
    if (elem) free(*elem);
}

void cleanup_shader_store() {
    cleanup_hash_map_elements(shader_store, _free_shader_code);
}


static char* load_shader_code(const char* shader_path) {
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


#define SHADER_EXT ".glsl"
static int is_shader(const char* file_name, char** out_shader_name) {
    size_t len = strlen(file_name);
    size_t len_ext = strlen(SHADER_EXT);
    if ((len <= len_ext) || (strcmp(file_name + len - len_ext, SHADER_EXT) != 0)) 
        return RET_ERR;
    if (out_shader_name != NULL)
        *out_shader_name = strndup(file_name, len - len_ext);
    return RET_OK;
}

static char* path_join(const char* file_part1, const char* file_part2) {
    size_t out_len = strlen(file_part1) + strlen(file_part2) + 2;
    char* ret = malloc(out_len); *ret = '\0'; 
    strcat(ret, file_part1);
    strcat(ret, "/");
    strcat(ret, file_part2); 
    return ret;
}