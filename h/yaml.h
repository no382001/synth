#ifndef YAML_H
#define YAML_H

#include "hash.h"

/*
originally from https://github.com/pbrandt1/yaml with some fixes and changes
*/

hash_t * yaml_read(char *file);

#endif
