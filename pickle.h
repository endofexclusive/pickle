/**@file pickle.h
 * @brief Pickle interpreter header, a TCL like language based on 'picol'.
 *
 * BSD license, See <https://github.com/howerj/pickle/blob/master/LICENSE>
 * or pickle.c for more information.
 *
 * Copyright (c) 2007-2016, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2018-2019, Richard James Howe <howe.r.j.89@gmail.com> */

#ifndef PICKLE_H
#define PICKLE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define PICKLE_MAX_STRING    (512) /* Max string/Data structure size */
#define PICKLE_MAX_RECURSION (128) /* Recursion limit */
#define PICKLE_MAX_ARGS      (128) /* Maximum arguments to some internal functions */

#ifndef PREPACK
#define PREPACK /* Used to apply attributes to structures (like packing) */
#endif

#ifndef POSTPACK
#define POSTPACK /* Used to apply attributes to structures (like packing) */
#endif

#ifndef PICKLE_API
#define PICKLE_API /* Used to apply attributes to exported functions */
#endif

typedef PREPACK struct {
	void *(*malloc)  (void *arena, size_t bytes);            /* malloc equivalent */
	void *(*realloc) (void *arena, void *ptr, size_t bytes); /* realloc equivalent */
	int   (*free)    (void *arena, void *ptr);               /* free equivalent; can return error code */
	void *arena;  /* arena we are allocating in, if any */
} POSTPACK pickle_allocator_t; /* optional */

typedef PREPACK struct { /* TODO: Remove/Hide this if possible. */
	char *arg;   /* parsed argument */
	int error,   /* turn error reporting on/off */
	    index,   /* index into argument list */
	    option,  /* parsed option */
	    reset;   /* set to reset */
	char *place; /* internal use: scanner position */
	int  init;   /* internal use: initialized or not */
} POSTPACK pickle_getopt_t; /* getopt clone; with a few modifications */

struct pickle_interpreter;
typedef struct pickle_interpreter pickle_t;

typedef int (*pickle_command_func_t)(pickle_t *i, int argc, char **argv, void *privdata);

/* All the following functions return one of the pickle error statuses; PICKLE_OK,
 * PICKLE_ERROR, or one of the other pickle return values. All arguments are
 * asserted for not being NULL unless otherwise specified. */ 

enum { PICKLE_ERROR = -1, PICKLE_OK, PICKLE_RETURN, PICKLE_BREAK, PICKLE_CONTINUE };

PICKLE_API int pickle_getopt(pickle_getopt_t *opt, int argc, char *const argv[], const char *fmt); /* PICKLE_RETURN when done */

PICKLE_API int pickle_new(pickle_t **i, const pickle_allocator_t *a); /* if(a == NULL) default allocator used */
PICKLE_API int pickle_delete(pickle_t *i);
PICKLE_API int pickle_eval(pickle_t *i, const char *t);
PICKLE_API int pickle_register_command(pickle_t *i, const char *name, pickle_command_func_t f, void *privdata);
PICKLE_API int pickle_rename_command(pickle_t *i, const char *src, const char *dst); /* if 'dst' is "" then command is deleted */

PICKLE_API int pickle_set_result(pickle_t *i, const char *fmt, ...);
PICKLE_API int pickle_set_result_string(pickle_t *i, const char *s);
PICKLE_API int pickle_set_result_integer(pickle_t *i, long result);
PICKLE_API int pickle_get_result_string(pickle_t *i, const char **s);
PICKLE_API int pickle_get_result_integer(pickle_t *i, long *val);

PICKLE_API int pickle_set_result_error(pickle_t *i, const char *fmt, ...);
PICKLE_API int pickle_set_result_error_arity(pickle_t *i, int expected, int argc, char **argv);

PICKLE_API int pickle_set_var_string(pickle_t *i, const char *name, const char *val);
PICKLE_API int pickle_set_var_integer(pickle_t *i, const char *name, long r);
PICKLE_API int pickle_get_var_string(pickle_t *i, const char *name, const char **val);
PICKLE_API int pickle_get_var_integer(pickle_t *i, const char *name, long *val);

PICKLE_API int pickle_tests(void); /* returns: PICKLE_OK on success, PICKLE_ERROR on failure, PICKLE_OK if defined(NDEBUG) */

#ifdef __cplusplus
}
#endif
#endif
