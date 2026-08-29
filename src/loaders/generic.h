#ifndef FOOTIE_LIFE_GENERIC_H
#define FOOTIE_LIFE_GENERIC_H

#include <ints.h>


char *LoadFileText(const char *fileName, size_t *count);
char *LoadFileBinary(const char *fileName, size_t *count);

void FreeFileText(char *text);

#endif //FOOTIE_LIFE_GENERIC_H
