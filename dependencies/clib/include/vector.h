#ifndef VECTOR_H
#define VECTOR_H

#include <ints.h>
#include <error.h>

typedef struct Vector {
    size_t capacity;
    size_t length;
    size_t stride;
    void *data;
} Vector;

//----------------------------------------------------------------------------------
// Lifecycle
//----------------------------------------------------------------------------------
Error vector_init(Vector *vec, size_t stride);
Error vector_destroy(Vector *vec);
Error vector_clone(Vector *dst, const Vector *src);

//----------------------------------------------------------------------------------
// Properties
//----------------------------------------------------------------------------------
size_t vector_length(const Vector *vec);
size_t vector_capacity(const Vector *vec);
bool vector_empty(const Vector *vec);

//----------------------------------------------------------------------------------
// Memory management
//----------------------------------------------------------------------------------
Error vector_reserve(Vector *vec, size_t minimum_capacity);
Error vector_resize(Vector *vec, size_t new_length);
Error vector_shrink_to_fit(Vector *vec);

//----------------------------------------------------------------------------------
// Element access
//----------------------------------------------------------------------------------
Error vector_get(const Vector *vec, size_t index,  void *dest);
Error vector_set(const Vector *vec, size_t index, const void *src);

//----------------------------------------------------------------------------------
// Insertion
//----------------------------------------------------------------------------------
Error vector_push(Vector *vec, const void *data);
Error vector_insert(Vector *vec, size_t index, const void *element);
Error vector_append(Vector *vec, const void *elements, size_t count);

//----------------------------------------------------------------------------------
// Removal
//----------------------------------------------------------------------------------
Error vector_pop(Vector *vec, void *dst);
Error vector_remove(Vector *vec, size_t index, void *dst);
Error vector_swap_remove(Vector *vec, size_t index, void *dst);
void vector_clear(Vector *vec);

//----------------------------------------------------------------------------------
// Algorithms
//----------------------------------------------------------------------------------
Error vector_reverse(const Vector *vec);
Error vector_sort(Vector *vec, int (*compare)(void *, void *));

void *vector_find(Vector *vec, void *data, int (*compare)(void *, void *));

const void *vector_find_const(Vector *vec, void *data, int (*compare)(void *, void *));

#endif //VECTOR_H
