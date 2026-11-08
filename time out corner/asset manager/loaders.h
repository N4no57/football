#ifndef FOOTIE_LIFE_LOADERS_H
#define FOOTIE_LIFE_LOADERS_H

void *LoadFontCPU(const char *path);
void FreeFontCPU(void *data);
void *LoadFontGPU(void *data);
void FreeFontGPU(void *data);

#endif //FOOTIE_LIFE_LOADERS_H
