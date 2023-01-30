#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <stdint.h>

#include "error.h"

typedef uint32_t texture;

mrerror texture_new(texture *t, const char *file);

mrerror texture_find(texture *t, const char *name);

#endif