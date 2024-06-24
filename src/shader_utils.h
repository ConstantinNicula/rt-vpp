#ifndef __SHADER_UTILS_H__
#define __SHADER_UTILS_H__

#ifndef DEFAULT_SHADER_VERSION
#define DEFAULT_SHADER_VERSION "#version 130\n"
#endif

int init_shader_store();
int add_shaders_to_store(const char* shader_folder_path);
const char* get_shader_code(const char* shader_name);
void cleanup_shader_store();

#endif