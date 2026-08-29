#include "generic.h"
#include "../logging.h"

#include <stdio.h>
#include <stdlib.h>

char *LoadFileText(const char *fileName, size_t *count) {
    if (fileName == nullptr) {
        TraceLog(LOG_WARNING, "FILEIO: File name provided is not valid");
        return nullptr;
    }

    FILE *file = fopen(fileName, "rt");

    if (file == nullptr) {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to open text file", fileName);
        return nullptr;
    }

    // WARNING: When reading a file as 'text' file,
    // text mode causes carriage return-linefeed translation...
    // ...but using fseek() should return correct byte-offset
    fseek(file, 0, SEEK_END);
    const unsigned int size = (unsigned int)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to read text file", fileName);
        return nullptr;
    }

    char *text = (char *)malloc((size + 1) * sizeof(char));

    if (text == nullptr) {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to allocated memory for file reading", fileName);
        return nullptr;
    }

    const unsigned int _count = (unsigned int)fread(text, sizeof(char), size, file);

    // WARNING: \r\n is converted to \n on reading, so,
    // read bytes count gets reduced by the number of lines
    if (_count < size) text = (char *)realloc(text, _count + 1);

    // Zero-terminate the string
    text[_count] = '\0';

    if (count != nullptr) *count = _count;

    TraceLog(LOG_INFO, "FILEIO: [%s] Text file loaded successfully", fileName);

    fclose(file);

    return text;
}

char *LoadFileBinary(const char *fileName, size_t *count) {
    if (fileName == nullptr) {
        TraceLog(LOG_WARNING, "FILEIO: File name provided is not valid");
        return nullptr;
    }

    FILE *file = fopen(fileName, "rb");

    if (file == nullptr) {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to open text file", fileName);
        return nullptr;
    }

    // WARNING: When reading a file as 'text' file,
    // text mode causes carriage return-linefeed translation...
    // ...but using fseek() should return correct byte-offset
    fseek(file, 0, SEEK_END);
    const unsigned int size = (unsigned int)ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to read text file", fileName);
        return nullptr;
    }

    char *text = (char *)malloc((size + 1) * sizeof(char));

    if (text == nullptr) {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to allocated memory for file reading", fileName);
        return nullptr;
    }

    fread(text, sizeof(char), size, file);

    // Zero-terminate the string
    text[size] = '\0';

    if (count != nullptr) *count = size;

    TraceLog(LOG_INFO, "FILEIO: [%s] Text file loaded successfully", fileName);

    fclose(file);

    return text;
}

void FreeFileText(char *text) {
    free(text);
}
