#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Error {
    ERROR_OK = 0,

    /* 100–199: Arguments and input */
    ERROR_NULL_POINTER       = 100,
    ERROR_INVALID_ARGUMENT   = 101,
    ERROR_OUT_OF_RANGE       = 102,
    ERROR_INVALID_FORMAT     = 103,

    /* 200–299: Memory */
    ERROR_ALLOCATION_FAILED  = 200,
    ERROR_BUFFER_TOO_SMALL   = 201,
    ERROR_BUFFER_OVERFLOW    = 202,

    /* 300–399: Files and I/O */
    ERROR_FILE_NOT_FOUND     = 300,
    ERROR_FILE_OPEN_FAILED   = 301,
    ERROR_FILE_CLOSE_FAILED  = 302,
    ERROR_FILE_READ_FAILED   = 303,
    ERROR_FILE_WRITE_FAILED  = 304,
    ERROR_FILE_SEEK_FAILED   = 305,
    ERROR_END_OF_FILE        = 306,

    /* 400–499: Program state */
    ERROR_NOT_INITIALIZED    = 400,
    ERROR_ALREADY_EXISTS     = 401,
    ERROR_NOT_FOUND          = 402,
    ERROR_INVALID_STATE      = 403,

    /* 500–599: Calculations */
    ERROR_DIVIDE_BY_ZERO     = 500,
    ERROR_ARITHMETIC_OVERFLOW = 501,

    /* 900+: Unexpected/internal failures */
    ERROR_INTERNAL           = 900, // If returning this code a diagnostic should be supplied
    ERROR_NOT_IMPLEMENTED    = 901
} Error;

static inline const char *clib_error_message(const Error error) {
    switch (error) {
        case ERROR_OK:                  return "No error";
        case ERROR_NULL_POINTER:        return "Null pointer received";
        case ERROR_INVALID_ARGUMENT:    return "Invalid argument";
        case ERROR_OUT_OF_RANGE:        return "Value is out of range";
        case ERROR_INVALID_FORMAT:      return "Invalid input format";

        case ERROR_ALLOCATION_FAILED:   return "Memory allocation failed";
        case ERROR_BUFFER_TOO_SMALL:    return "Buffer is too small";
        case ERROR_BUFFER_OVERFLOW:     return "Buffer capacity exceeded";

        case ERROR_FILE_NOT_FOUND:      return "File was not found";
        case ERROR_FILE_OPEN_FAILED:    return "Could not open file";
		case ERROR_FILE_CLOSE_FAILED:   return "Could not close file";
        case ERROR_FILE_READ_FAILED:    return "Could not read from file";
        case ERROR_FILE_WRITE_FAILED:   return "Could not write to file";
		case ERROR_FILE_SEEK_FAILED:    return "Could not seek file";
        case ERROR_END_OF_FILE:         return "End of file reached";

        case ERROR_NOT_INITIALIZED:     return "Object is not initialized";
        case ERROR_ALREADY_EXISTS:      return "Item already exists";
        case ERROR_NOT_FOUND:           return "Requested item was not found";
        case ERROR_INVALID_STATE:       return "Operation is invalid in the current state";

        case ERROR_DIVIDE_BY_ZERO:      return "Division by zero";
        case ERROR_ARITHMETIC_OVERFLOW: return "Arithmetic overflow";

        case ERROR_INTERNAL:            return "Internal program error";
        case ERROR_NOT_IMPLEMENTED:     return "Feature is not implemented";
        default:                      return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif

#endif //ERROR_H
