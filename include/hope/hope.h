/* File: hope.h
 * Author: Janick Eicher
 * Date: 21.06.2022
 * Description: Single-Header-Library for argument parsing in C
 * License: MIT
 * Version: 1.0.0
 * 
 * To use this library, define HOPE_IMPLEMENTATION before including this file.
 * This will make this file compile.
 */


#ifndef HOPE_H_
#define HOPE_H_

// Header
#ifndef INCLUDE_HOPE_H_
#define INCLUDE_HOPE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HOPEDEF
#ifdef HOPE_STATIC
#define HOPEDEF static
#else
#define HOPEDEF extern
#endif
#endif

#define HOPE_VERSION "0.1.4"


/* individual types of arguments
 * all arguments will always be put into an array for streamlining
 */
enum hope_argtype_e {
    HOPE_TYPE_SWITCH = 0,
    HOPE_TYPE_INTEGER = 1, // these are long integers
    HOPE_TYPE_DOUBLE = 2,
    HOPE_TYPE_STRING = 3,
};

HOPEDEF const char *hope_argtype_str(enum hope_argtype_e argtype);

// Argument count special values
// Expect none: This is unused
#define HOPE_ARGC_NONE       0
// Expect one or more
#define HOPE_ARGC_MORE      -1
// Expect zero or more
#define HOPE_ARGC_OPTMORE   -2
// Expect zero or one
#define HOPE_ARGC_OPT       -3

/* Parameter struct
 * nargs: number of arguments
 *        (0 for no arguments)
 *        (-1 for 1 or more arguments)
 *        (-2 for 0 or more arguments)
 *        (-3 for an optional argument)
 * name: prefix for the parameter
 *      (NULL for no prefix, called collector, there can only be one of these)
 * help: help message
 * 
 */
typedef struct {
    const char *name;
    const char *help;
    enum hope_argtype_e type;
    int nargs; 
} hope_param_t;

/* Temporary struct for storing arguments parsed
 * First arg is always the prefix (NULL if no prefix)
 */
typedef struct {
    union {
        long int *integers;
        double *doubles;
        const char **strings;
        bool _switch;
    } value;
    const char *name;
    size_t count;
    enum hope_argtype_e type;
} hope_result_t;

/* A set of parameters. You can have multiple of these in one parser,
 * but only the first matching one will get parsed.
 */
typedef struct {
    const char *name;
    size_t nparams;
    size_t nresults;
    hope_param_t *params;
    hope_param_t *collector;
    hope_result_t *results;
} hope_set_t;


/* Main data structure, will contain the parameters and
 * further information about the arguments parsed
 * prog_name: Name of the program
 * prog_desc: Description of the program's function
 * sets: The parameter sets
 * nsets: The amount of sets
 * results: A pointer to the result array for the used set
 * nresults: A pointer to the amount of results for the used set
 */ 
typedef struct {
    const char *prog_name;
    const char *prog_desc;
    hope_set_t *sets;
    size_t nsets;
    hope_result_t *results;
    size_t nresults;
    const char *used_set_name;
} hope_t;


//
// hope_param_t functions
//

// Initialize the hope_param_t data structure
HOPEDEF hope_param_t hope_init_param(const char *name, const char *help, enum hope_argtype_e type, int nargs);

//
// hope_set_t functions
//

// Initialize a new set
HOPEDEF hope_set_t hope_init_set(const char *set_name);

// Add a new parameter to the set
HOPEDEF int hope_add_param(hope_set_t *set, hope_param_t param);

//
// hope_t functions
// 

// Initialize the hope data structure
HOPEDEF hope_t hope_init(const char *prog_name, const char *prog_desc);
// Free the params in the hope data structure
HOPEDEF void hope_free(hope_t *hope);
// Generate and write the help message to the sink
HOPEDEF void hope_print_help(hope_t *hope, FILE *sink); 
// Add a new parameter set to the hope data structure
HOPEDEF int hope_add_set(hope_t *hope, hope_set_t set);
// Parse the command line arguments for the given set
HOPEDEF int hope_parse_set(hope_set_t *set, char *args[]);
// Parse all sets and use the results from the first one
HOPEDEF int hope_parse(hope_t *hope, char *args[]);
// A helper function that allows to you just pass argv for parsing
HOPEDEF inline int hope_parse_argv(hope_t *hope, char *argv[]);

// All these getter functions return -1 on error, and print an error message to stderr
// Dest pointers will also be set to NULL on error

// Get the value of a switch parameter (Function format is same as others to keep uniformity,
// despite the fact that only one value is returned for switches)
HOPEDEF int hope_get_switch(hope_t *hope, const char *name, bool *dest);
// Get the values of an integer parameter
HOPEDEF int hope_get_integer(hope_t *hope, const char *name, long int **dest);
// Get the values of a double parameter
HOPEDEF int hope_get_double(hope_t *hope, const char *name, double **dest);
// Get the values of a string parameter
HOPEDEF int hope_get_string(hope_t *hope, const char *name, const char ***dest);

// All these getter functions will terminate the program with an assertion if an error occurs,
// so use them carefully.

// Get a single switch or return false if it wasn't set.
HOPEDEF bool hope_get_single_switch(hope_t *hope, const char *name);

// Get a single integer or return a default value if none were passed.
HOPEDEF long int hope_get_single_integer(hope_t *hope, const char *name);

// Get a single double or return a default value if none were passed.
HOPEDEF double hope_get_single_double(hope_t *hope, const char *name);

// Get a single string or return a default value if none were passed.
HOPEDEF const char *hope_get_single_string(hope_t *hope, const char *name);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_HOPE_H_
// End of Header

//#define HOPE_IMPLEMENTATION
#ifdef HOPE_IMPLEMENTATION

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

// Get the string representation of an argument type
HOPEDEF const char *hope_argtype_str(enum hope_argtype_e argtype) {
    switch (argtype) {
        case HOPE_TYPE_SWITCH:
            return "switch";
        case HOPE_TYPE_INTEGER:
            return "integer";
        case HOPE_TYPE_DOUBLE:
            return "double";
        case HOPE_TYPE_STRING:
            return "string";
        default:
            return "unknown";
    }
}

// generic format string

#define HOPE_SUCCESS_CODE 0x00
#define HOPE_FMT_DEFAULT "hope: %s; %s"

// Error codes for system failures
#define HOPE_ERR_CODE 0x10
#define HOPE_ERR_ALLOC_FAILED_CODE 0x11
#define HOPE_ERR_ALLOC_FAILED_MSG "Allocation of memory failed"
#define HOPE_ERR_INVALID_STRUCT_CODE 0x12
#define HOPE_ERR_INVALID_STRUCT_MSG "Invalid hope structure passed"

void hope_err_alloc(const char *msg) {
    fprintf(stderr, HOPE_FMT_DEFAULT "\n", msg, HOPE_ERR_ALLOC_FAILED_MSG);
}

void hope_err_invalid_struct(const char *msg) {
    fprintf(stderr, HOPE_FMT_DEFAULT "\n", msg, HOPE_ERR_INVALID_STRUCT_MSG);
}

void hope_sys_err_any(int err, const char *msg){
    switch(err){
        case HOPE_ERR_ALLOC_FAILED_CODE:
            hope_err_alloc(msg);
            return;
        case HOPE_ERR_INVALID_STRUCT_CODE:
            hope_err_invalid_struct(msg);
            return;
    }
    return;
}

// Error codes and messages for adding parameters
#define HOPE_PARAMADD_ERR_CODE 0x20
#define HOPE_PARAMADD_ERR_GENERIC_MSG "Error adding parameter"
#define HOPE_PARAMADD_ERR_HASCOLLECTOR_CODE 0x21
#define HOPE_PARAMADD_ERR_HASCOLLECTOR_MSG "Collector already exists"
#define HOPE_PARAMADD_ERR_DUPLICATE_CODE 0x22
#define HOPE_PARAMADD_ERR_DUPLICATE_MSG "Duplicate parameter name"

void hope_paramadd_err_hascollector(){
    fprintf(stderr, HOPE_FMT_DEFAULT "\n", 
            HOPE_PARAMADD_ERR_GENERIC_MSG, 
            HOPE_PARAMADD_ERR_HASCOLLECTOR_MSG);
}

void hope_paramadd_err_duplicate(const char *name) {
    fprintf(stderr, HOPE_FMT_DEFAULT ": %s\n", 
            HOPE_PARAMADD_ERR_GENERIC_MSG, 
            HOPE_PARAMADD_ERR_DUPLICATE_MSG,
            name);
}

void hope_paramadd_err_any(int err, const char *msg) {
    switch(err) {
        case HOPE_PARAMADD_ERR_DUPLICATE_CODE:
            hope_paramadd_err_duplicate(msg);
            return;
        case HOPE_PARAMADD_ERR_HASCOLLECTOR_CODE:
            hope_paramadd_err_hascollector();
            return;
    }
    return;
}


// Error codes and messages for parsing arguments
#define HOPE_PARSE_ERR_CODE 0x30
#define HOPE_PARSE_ERR_GENERIC_MSG "Error parsing arguments"
#define HOPE_PARSE_ERR_PARAM_MISCOUNT_CODE 0x31
#define HOPE_PARSE_ERR_PARAM_MISCOUNT_MSG "Invalid amount of parameters provided"
#define HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE 0x32
#define HOPE_PARSE_ERR_PARAM_UNPARSABLE_MSG "Parameter could not be parsed"
#define HOPE_PARSE_ERR_PARAM_ARG_MISCOUNT_CODE 0x33
#define HOPE_PARSE_ERR_PARAM_ARG_MISCOUNT_MSG "Invalid amount of arguments passed for parameter"

void hope_parse_err(const char *msg){
    fprintf(stderr, HOPE_FMT_DEFAULT "\n",
            HOPE_PARSE_ERR_GENERIC_MSG,
            msg);
}

void hope_parse_err_param_miscount(const char *msg) {
    fprintf(stderr, HOPE_FMT_DEFAULT "; %s\n",
            HOPE_PARSE_ERR_GENERIC_MSG,
            HOPE_PARSE_ERR_PARAM_MISCOUNT_MSG,
            msg);
}

void hope_parse_err_param_unparsable(const char *msg) {
    fprintf(stderr, HOPE_FMT_DEFAULT ": %s\n",
            HOPE_PARSE_ERR_GENERIC_MSG,
            HOPE_PARSE_ERR_PARAM_UNPARSABLE_MSG,
            msg);
}

void hope_parse_err_param_arg_miscount(const char *name) {
    fprintf(stderr, HOPE_FMT_DEFAULT " %s\n",
                HOPE_PARSE_ERR_GENERIC_MSG,
                HOPE_PARSE_ERR_PARAM_ARG_MISCOUNT_MSG,
                name);
}

void hope_parse_err_any(int err, const char *msg){
    switch(err){
        case HOPE_PARSE_ERR_CODE:
            hope_parse_err(msg);
            return;
        case HOPE_PARSE_ERR_PARAM_MISCOUNT_CODE:
            hope_parse_err_param_miscount(msg);
            return;
        case HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE:
            hope_parse_err_param_unparsable(msg);
            return;
        case HOPE_PARSE_ERR_PARAM_ARG_MISCOUNT_CODE:
            hope_parse_err_param_arg_miscount(msg);
            return;
    }
}

// Error messages for types
#define HOPE_TYPED_ERR_SWITCH_MSG "Argument is not a switch, but a"
#define HOPE_TYPED_ERR_INTEGER_MSG "Argument is not an integer, but a"
#define HOPE_TYPED_ERR_DOUBLE_MSG "Argument is not a double, but a"
#define HOPE_TYPED_ERR_STRING_MSG "Argument is not a string, but a"


// Error messages and codes for getter
#define HOPE_GET_ERR_CODE 0x40
#define HOPE_GET_ERR_GENERIC_MSG "Failed to get parameter"
#define HOPE_GET_ERR_NOEXIST_CODE 0x41
#define HOPE_GET_ERR_NOEXIST_MSG "Parameter does not exist"
#define HOPE_GET_ERR_TYPE_MISMATCH_CODE 0x42
#define HOPE_GET_ERR_TYPE_MISMATCH_MSG "Type mismatch;"

void hope_get_err_noexist(const char *msg){
    fprintf(stderr, HOPE_FMT_DEFAULT ": %s\n",
            HOPE_GET_ERR_GENERIC_MSG,
            HOPE_GET_ERR_NOEXIST_MSG,
            msg);
}

void hope_get_err_type_mismatch(const char *msg) {
    fprintf(stderr, HOPE_FMT_DEFAULT " %s\n",
            HOPE_GET_ERR_GENERIC_MSG,
            HOPE_GET_ERR_TYPE_MISMATCH_MSG,
            msg);
}

void hope_get_err_any(int err, const char *msg){
    switch(err){
    case HOPE_GET_ERR_NOEXIST_CODE:
        hope_get_err_noexist(msg);
        return;
    case HOPE_GET_ERR_TYPE_MISMATCH_CODE:
        hope_get_err_type_mismatch(msg);
        return;
    }
    return;
}


// Error codes and messages for adding sets
#define HOPE_SETADD_ERR_CODE 0x50
#define HOPE_SETADD_ERR_GENERIC_MSG "Error adding set"
#define HOPE_SETADD_ERR_DUPLICATE_CODE 0x51
#define HOPE_SETADD_ERR_DUPLICATE_MSG "A set with the same name already exists"

void hope_setadd_err_duplicate(const char *msg) {
    fprintf(stderr, HOPE_FMT_DEFAULT ": %s\n", 
            HOPE_SETADD_ERR_GENERIC_MSG, 
            HOPE_SETADD_ERR_DUPLICATE_MSG,
            msg);
}

void hope_setadd_err_any(int err, const char *msg) {
    switch(err) {
        case HOPE_SETADD_ERR_DUPLICATE_CODE:
            hope_setadd_err_duplicate(msg);
            return;
    }
    return;
}

// a function to concatenate strings given in varargs
char *varstrcat(int count, ...)
{
    va_list ap;
    size_t  len = 0;

    if (count < 1)
        return NULL;

    // First, measure the total length required.
    va_start(ap, count);
    for (int i=0; i < count; i++) {
        const char *s = va_arg(ap, char *);
        len += strlen(s);
    }
    va_end(ap);

    // Allocate return buffer.
    char *ret = malloc(len + 1);
    if (ret == NULL)
        return NULL;

    // Concatenate all the strings into the return buffer.
    char *dst = ret;
    va_start(ap, count);
    for (int i=0; i < count; i++) {
        const char *src = va_arg(ap, char *);

        // This loop is a strcpy.
        strcpy(dst, src);
    }
    va_end(ap);
    return ret;
}


// This is a bit ugly, but it can count the variadic function count
#ifdef _MSC_VER // Microsoft compilers

#   define GET_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#   define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define INTERNAL_EXPAND(x) x
#   define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#else // Non-Microsoft compilers

#   define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#endif

// This function will match any error encountered.
#define hope_err_any(err, ...) (_hope_err_any((err), GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__))
void _hope_err_any(int err, int count, ...){
    va_list ap;
    size_t  len = 0;

    // First, measure the total length required.
    va_start(ap, count);
    for (int i=0; i < count; i++) {
        const char *s = va_arg(ap, char *);
        len += strlen(s);
    }
    va_end(ap);

    // Allocate return buffer.
    char *msg = malloc(len + 1);
    assert(msg);

    // Concatenate all the strings into the return buffer.
    char *dst = msg;
    va_start(ap, count);
    for (int i=0; i < count; i++) {
        const char *src = va_arg(ap, char *);

        // This loop is a strcpy.
        strcpy(dst, src);
    }
    va_end(ap);

    switch(err & 0xF0){
        case HOPE_ERR_CODE:
            hope_sys_err_any(err, (const char*)msg);
            return;
        case HOPE_PARAMADD_ERR_CODE:
            hope_paramadd_err_any(err, (const char*)msg);
            return;
        case HOPE_PARSE_ERR_CODE:
            hope_parse_err_any(err, (const char*)msg);
            return;
        case HOPE_GET_ERR_CODE:
            hope_get_err_any(err, (const char*)msg);
            return;
        case HOPE_SETADD_ERR_CODE:
            hope_setadd_err_any(err, (const char*)msg);
            return;
    }
}


//
// hope_param_t functions
//

// Initialize the hope data structure
HOPEDEF hope_param_t hope_init_param(const char *name, const char *help, enum hope_argtype_e type, int nargs){
    hope_param_t param = {
        .name = name,
        .help = help,
        .type = type,
        .nargs = nargs,
    };
    return param;
}

//
// hope_set_t functions
//

HOPEDEF hope_set_t hope_init_set(const char *set_name){
    return (hope_set_t) {
        .name = set_name,
        .nparams = 0,
        .params = NULL,
        .collector = NULL,
        .results = NULL,
        .nresults = 0
    };
}

// Add a new parameter to the set
HOPEDEF int hope_add_param(hope_set_t *set, hope_param_t param){
    if(param.name == NULL){
        if(set->collector != NULL){
            hope_paramadd_err_hascollector();
            return HOPE_PARAMADD_ERR_HASCOLLECTOR_CODE;
        }
        set->collector = (hope_param_t*) malloc(sizeof(hope_param_t));
        if(!set->collector){
            hope_err_alloc(HOPE_PARAMADD_ERR_GENERIC_MSG);
            return HOPE_ERR_ALLOC_FAILED_CODE;
        }
        *set->collector = param;
    } else {
        // search for a parameter with the same name
        for(size_t i = 0; i < set->nparams; i++){
            hope_param_t *cur_param = set->params + i;
            if(strcmp(cur_param->name, param.name) == 0){
                hope_paramadd_err_duplicate(param.name);
                return HOPE_PARAMADD_ERR_DUPLICATE_CODE;
            }
        }
        set->params = (hope_param_t*) realloc(set->params, (set->nparams + 1) * sizeof(hope_param_t));
        if(!set->params){
            hope_err_alloc(HOPE_PARAMADD_ERR_GENERIC_MSG);
            return HOPE_ERR_ALLOC_FAILED_CODE;
        }
        set->params[set->nparams] = param;
        set->nparams++;
    }
    return HOPE_SUCCESS_CODE;
}

// Initialize the hope data structure
HOPEDEF hope_t hope_init(const char *prog_name, const char *prog_desc){
    assert(prog_name && "prog_name cannot be NULL");
    hope_t hope = {
        .prog_name = prog_name,
        .prog_desc = prog_desc,
        .sets = NULL,
        .results = NULL,
        .nsets = 0,
        .nresults = 0,
        .used_set_name = NULL
    };
    return hope;
}
// Free the params in the hope data structure
HOPEDEF void hope_free(hope_t *hope){
    if (hope->sets){
        for(size_t i = 0; i < hope->nsets; i++){
            hope_set_t *set = (hope_set_t*)(hope->sets + i);
            if(set->params) free(set->params);
            if(set->collector) free(set->collector);
            if (set->results){
                for (size_t i = 0; i < set->nresults; i++){
                    if(set->results[i].type != HOPE_TYPE_SWITCH)
                        free(set->results[i].value.strings);
                }
                free(set->results);
            }
        }
        free(hope->sets);
    }
    hope->nsets = 0;
    hope->nresults = 0;
}

// Generate and write the help message to the sink
HOPEDEF void hope_print_help(hope_t *hope, FILE *sink){
    const char *FMT_SWITCH_REQ = "%s ";
    const char *FMT_SWITCH_OPT = "(%s) ";
    const char *FMT_PARAM_REQ = "%s [%s] ";
    const char *FMT_PARAM_OPT = "(%s [%s])? ";
    const char *FMT_PARAM_OPTMORE = "(%s [%s]*) ";
    const char *FMT_PARAM_MORE = "%s [%s]+ ";
    const char *FMT_PARAM_REQ_SPECIFIC = "%s [%s]{%u} ";
    
    if(hope->prog_desc)
        fprintf(sink, "%s\n", hope->prog_desc);
    fprintf(sink, "Usage: %s ", hope->prog_name);

    char *fmt_string = NULL;
    for(size_t i = 0; i < hope->nsets; i++){
        hope_set_t *set = (hope_set_t*)(hope->sets + i);
        for(size_t j = 0; j < set->nparams; j++){
            hope_param_t *cur_param = (hope_param_t*)(set->params + j);
            if(cur_param->type == HOPE_TYPE_SWITCH){
                fmt_string = (char*)(cur_param->nargs == HOPE_ARGC_OPT ? FMT_SWITCH_OPT : FMT_SWITCH_REQ);
                printf(fmt_string, cur_param->name);
            } else {
                switch(cur_param->nargs){
                    case HOPE_ARGC_MORE:
                        fmt_string = (char*)FMT_PARAM_MORE;
                        break;
                    case HOPE_ARGC_OPTMORE:
                        fmt_string = (char*)FMT_PARAM_OPTMORE;
                        break;
                    case HOPE_ARGC_OPT:
                        fmt_string = (char*)FMT_PARAM_OPT;
                        break;
                    default:
                        assert(cur_param->nargs > 0);
                        fmt_string = (char*)(cur_param->nargs > 1 ? FMT_PARAM_REQ_SPECIFIC : FMT_PARAM_REQ);
                        break;
                }
                if(cur_param-> nargs > 1)
                    printf(fmt_string, cur_param->name, hope_argtype_str(cur_param->type), cur_param->nargs);  
                else
                    printf(fmt_string, cur_param->name, hope_argtype_str(cur_param->type));
            }
        }
        if(i + 1 < hope->nsets) {
            fprintf(sink, " | ");
        }
    }
    fprintf(sink, "\n");
    for(size_t i = 0; i < hope->nsets; i++){
        hope_set_t *set = (hope_set_t*)(hope->sets + i);
        fprintf(sink, "Parameter set %s:\n", set->name);
        for(size_t j = 0; j < set->nparams; j++){
            hope_param_t *cur_param = (hope_param_t *)(set->params + j);
            if(cur_param->help)
                fprintf(sink, "  %s: %s\n", cur_param->name, cur_param->help);
        }
    }
}

HOPEDEF int hope_add_set(hope_t *hope, hope_set_t set) {
    if(hope->sets){
        for(size_t i = 0; i < hope->nsets; i++){
            if(!strcmp(hope->sets[i].name, set.name)){
                hope_setadd_err_duplicate(set.name);
                return HOPE_SETADD_ERR_DUPLICATE_CODE;
            }
        }
    }
    hope->sets = (hope_set_t*) realloc(hope->sets, (hope->nsets + 1) * sizeof(hope_set_t));
    if(!hope->sets) {
        hope_err_alloc(HOPE_SETADD_ERR_GENERIC_MSG);
        return HOPE_ERR_ALLOC_FAILED_CODE;
    }
    hope->sets[hope->nsets] = set;
    hope->nsets++;
    return HOPE_SUCCESS_CODE;
}

// Search for the parameter with the given name
hope_param_t *hope_search_param(hope_set_t *set, const char *name){
    for(size_t i = 0; i < set->nparams; i++){
        hope_param_t *cur_param = set->params + i;
        if(name == NULL){
            if(cur_param->name == NULL)
                return cur_param;
        } else if(strcmp(cur_param->name, name) == 0){
            return cur_param;
        }
    }
    return NULL;
}

// Search for the result with the given name
hope_result_t *hope_search_result(hope_result_t *results, size_t nresults, const char *name){
    for(size_t i = 0; i < nresults; i++){
        hope_result_t *cur_result =(hope_result_t*)(results + i);
        if(name == NULL){
            if(cur_result->name == NULL){
                return cur_result;
            }
        } else if(strcmp(cur_result->name, name) == 0){
            return cur_result;
        }
    }
    return NULL;
}

// parse an integer, allocate memory for the new item and push it to the result
int hope_parse_integer_into_result(const char *str, hope_result_t *result){
    char *endptr;
    long int next_val = strtol(str, &endptr, 10);
    if(endptr == str){
        //hope_err_any(HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE, result->name);
        return HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE;
    }
    result->value.integers = (long int*) realloc(result->value.integers, (result->count + 1) * sizeof(long int));
    if(!result->value.integers){
        hope_err_any(HOPE_ERR_ALLOC_FAILED_CODE, HOPE_PARSE_ERR_GENERIC_MSG);
        return HOPE_ERR_ALLOC_FAILED_CODE;
    }
    result->value.integers[result->count] = next_val;
    result->count++;
    return HOPE_SUCCESS_CODE;
}

// parse a double, allocate memory for the new item and push it to the result
int hope_parse_double_into_result(const char *str, hope_result_t *result){
    char *endptr;
    double next_val = strtod(str, &endptr);
    if(endptr == str){
        //hope_err_any(HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE, result->name);
        return HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE;
    }
    result->value.doubles = (double*) realloc(result->value.doubles, (result->count + 1) * sizeof(double));
    if(!result->value.doubles){
        hope_err_any(HOPE_ERR_ALLOC_FAILED_CODE, HOPE_PARSE_ERR_GENERIC_MSG);
        return HOPE_ERR_ALLOC_FAILED_CODE;
    }
    result->value.doubles[result->count] = next_val;
    result->count++;
    return HOPE_SUCCESS_CODE;
}

// allocate memory for the new string and push it to the result
int hope_parse_string_into_result(const char *str, hope_result_t *result){
    result->value.strings = (const char**) realloc(result->value.strings, (result->count + 1) * sizeof(char*));
    if(!result->value.strings){
        return HOPE_ERR_ALLOC_FAILED_CODE;
    }
    result->value.strings[result->count] = str;
    result->count++;
    return HOPE_SUCCESS_CODE;
}

// evaluate the required parsing method based on the parameter type, then parse and push to the result
int hope_parse_into_result(const char *str, hope_param_t *param, hope_result_t *result){
    switch(param->type){
        case HOPE_TYPE_INTEGER:
            return hope_parse_integer_into_result(str, result);
        case HOPE_TYPE_DOUBLE: {
            return hope_parse_double_into_result(str, result);
        }
        case HOPE_TYPE_STRING:{
            return hope_parse_string_into_result(str, result);
        }
        default:
            printf("Unknown type %d\n", param->type);
            assert(0 && "Unreachable"); // TODO: error msg for invalid type
    }
}

int hope_push_parsed_result(hope_set_t *set, hope_result_t result){
    set->results = (hope_result_t*) realloc(set->results, (set->nresults + 1) * sizeof(hope_result_t));
    if(!set->results){ 
        hope_err_any(HOPE_ERR_ALLOC_FAILED_CODE, HOPE_PARSE_ERR_GENERIC_MSG);
        return HOPE_ERR_ALLOC_FAILED_CODE;
    }   
    set->results[set->nresults] = result;
    set->nresults++;
    return HOPE_SUCCESS_CODE;
}

// Parse the command line arguments and store the results in the hope data structure
HOPEDEF int hope_parse_set(hope_set_t *set, char *args[]){
    // look for the collector first
    hope_result_t result = {0};
    int parse_code = 0;
    char *error_msg = "";
    hope_result_t collector_result = {0};
    hope_param_t *param = NULL;

    if(args[0] == NULL){
        if(set->nparams > 0) {
            // check if there are any required parameters
            for(size_t i = 0; i < set->nparams; i++){
                hope_param_t *cur_param = set->params + i;
                if(cur_param->nargs >= HOPE_ARGC_MORE && cur_param){
                    parse_code = HOPE_PARSE_ERR_PARAM_MISCOUNT_CODE;
                    goto defer;
                }
            }
        } else if(set->collector){
            if(set->collector->nargs >= HOPE_ARGC_MORE && set->collector->nargs != 0){ 
                parse_code = HOPE_PARSE_ERR_PARAM_MISCOUNT_CODE;
                goto defer;
            }
        } 
    }
 
    for(size_t i = 0; args[i] != NULL; i++){
        if(strcmp(args[i], "--") == 0)
            continue; // skip the -- separator
        param = hope_search_param(set, args[i]);
        if(param){
            result.name = param->name;
            result.type = param->type;
            if(param->type == HOPE_TYPE_SWITCH){
                result.value._switch = 1;
            } else if(param->nargs == HOPE_ARGC_NONE){
                continue;
            } else {
                while(args[i+1] != NULL){
                    if(hope_search_param(set, args[i+1]) || strcmp(args[i+1], "--") == 0)
                        break;

                    parse_code = hope_parse_into_result(args[i+1], param, &result);
                    if(parse_code != HOPE_SUCCESS_CODE){
                        error_msg = (char*)param->name;
                        goto defer;
                    }
                    i++;
                    if(param->nargs == HOPE_ARGC_OPT || param->nargs == (int)result.count)
                        break;
                }
            }
            hope_push_parsed_result(set, result);
            result = (hope_result_t){0};
        } else {
            if (set->collector){
                if((set->collector->nargs == HOPE_ARGC_OPT && collector_result.count != 0) ||
                    set->collector->nargs == (int)collector_result.count){
                    break;
                }
                parse_code = hope_parse_into_result(args[i], set->collector, &collector_result);
                if(parse_code != HOPE_SUCCESS_CODE){
                    error_msg = "<collector>";
                    goto defer;
                }
            } else {
                parse_code = HOPE_PARSE_ERR_PARAM_MISCOUNT_CODE;
                error_msg = "Too many arguments without a defined collector";
                goto defer;
            }
        }
        if(args[i] == NULL) 
            break;
    }
    // add empty entries for optional params, or error out if not enough arguments were provided earlier
    for(size_t i = 0; i < set->nparams; i++){
        param = set->params + i;
        hope_result_t *param_result = hope_search_result(set->results, set->nresults, param->name);
        if(param->nargs == HOPE_ARGC_MORE || param->nargs > HOPE_ARGC_NONE){
            if(param_result == NULL){
                error_msg = (char*)param->name;
                parse_code = HOPE_PARSE_ERR_PARAM_ARG_MISCOUNT_CODE;
                goto defer;
            }
        } else if((param->nargs == HOPE_ARGC_OPTMORE || 
                   param->nargs == HOPE_ARGC_OPT || 
                   param->nargs == HOPE_ARGC_NONE) && 
                  param_result == NULL){
            result = (hope_result_t){
                .name = param->name, 
                .type = param->type,
                .count = 0,
                .value = {0}
            };
            hope_push_parsed_result(set, result);
        }
    }
    if(set->collector){
        collector_result.type = set->collector->type;
        if ((set->collector->nargs == HOPE_ARGC_MORE && collector_result.count <= 0) ||
            (set->collector->nargs > HOPE_ARGC_NONE && set->collector->nargs != (int)collector_result.count)){
            error_msg = (char*)set->collector->name;
            parse_code = HOPE_PARSE_ERR_PARAM_ARG_MISCOUNT_CODE;
            goto defer;
        }
        hope_push_parsed_result(set, collector_result);
    }
    return HOPE_SUCCESS_CODE;
defer:
    #ifdef HOPE_DEBUG
    hope_err_any(parse_code, error_msg);
    #else
    (void)error_msg;
    #endif
    // deallocate the results, if they were allocated.
    if(set->results) free(set->results);
    set->results = NULL;
    return parse_code;
}

HOPEDEF int hope_parse(hope_t *hope, char *args[]) {
    for(size_t i = 0; i < hope->nsets; i++){
        hope_set_t *set = (hope_set_t*)(hope->sets + i);
        int parse_result = hope_parse_set(set, args);
        if(parse_result == HOPE_SUCCESS_CODE){
            hope->results = set->results;
            hope->nresults = set->nresults;
            hope->used_set_name = set->name;
            return HOPE_SUCCESS_CODE;
        }
    }
    hope_parse_err("No matching set found for the given parameters");
    return HOPE_PARSE_ERR_PARAM_UNPARSABLE_CODE;
}

HOPEDEF int hope_get_switch(hope_t *hope, const char *name, bool *dest){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    if(!result){
        hope_get_err_noexist(name);
		dest = NULL;
		return -1;
    }
    if(result->type != HOPE_TYPE_SWITCH){
        hope_err_any(HOPE_GET_ERR_TYPE_MISMATCH_CODE,
            name,
            " was expected to be of type ",
            hope_argtype_str(HOPE_TYPE_SWITCH),
            ", but is of type ",
            hope_argtype_str(result->type)
        );
		dest = NULL;
		return -1;
    }
    *dest = result->value._switch;
    return 1;
}

HOPEDEF inline int hope_parse_argv(hope_t *hope, char *argv[]) {
    return hope_parse(hope, argv + 1);
}

HOPEDEF int hope_get_integer(hope_t *hope, const char *name, long int **dest){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    if(!result){
        hope_get_err_noexist(name);
        dest = NULL;
		return -1;
    }
    if(result->type != HOPE_TYPE_INTEGER){
        hope_err_any(HOPE_GET_ERR_TYPE_MISMATCH_CODE,
            name,
            " was expected to be of type ",
            hope_argtype_str(HOPE_TYPE_INTEGER),
            ", but is of type ",
            hope_argtype_str(result->type)
        );
        dest = NULL;
		return -1;
    }
    *dest = result->value.integers;
    return result->count;
}

HOPEDEF int hope_get_double(hope_t *hope, const char *name, double **dest){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    if(!result){
        hope_get_err_noexist(name);
        dest = NULL;
		return -1;
    }
    if(result->type != HOPE_TYPE_DOUBLE){
        hope_err_any(HOPE_GET_ERR_TYPE_MISMATCH_CODE,
            name,
            " was expected to be of type ",
            hope_argtype_str(HOPE_TYPE_DOUBLE),
            ", but is of type ",
            hope_argtype_str(result->type)
        );
        dest = NULL;
		return -1;
    }
    *dest = result->value.doubles;
    return result->count;
}

HOPEDEF int hope_get_string(hope_t *hope, const char *name, const char ***dest){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    if(!result){
        hope_get_err_noexist(name);
        dest = NULL;
		return -1;
    }
    if(result->type != HOPE_TYPE_STRING){
        hope_err_any(HOPE_GET_ERR_TYPE_MISMATCH_CODE,
            name,
            " was expected to be of type ",
            hope_argtype_str(HOPE_TYPE_STRING),
            ", but is of type ",
            hope_argtype_str(result->type)
        );
        dest = NULL;
		return -1;
    }
    *dest = result->value.strings;
    return result->count;
}

// Get a single switch or return false if it wasn't set.
HOPEDEF bool hope_get_single_switch(hope_t *hope, const char *name){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    assert(result && "Queried parameter could not be found.");
    assert(result->type == HOPE_TYPE_SWITCH && "Queried parameter is not of switch type");
    assert(result->count < 2 && "Queried parameter contained more than 1 value.");
    return result->value._switch;
}

// Get a single integer or return a default value if none were passed.
HOPEDEF long int hope_get_single_integer(hope_t *hope, const char *name){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    assert(result && "Queried parameter could not be found.");
    assert(result->type == HOPE_TYPE_INTEGER && "Queried parameter is not of integer type");
    assert(result->count < 2 && "Queried parameter contained more than 1 value.");
    return result->count ? result->value.integers[0] : 0;
}

// Get a single double or return a default value if none were passed.
HOPEDEF double hope_get_single_double(hope_t *hope, const char *name){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    assert(result && "Queried parameter could not be found.");
    assert(result->type == HOPE_TYPE_DOUBLE && "Queried parameter is not of double type");
    assert(result->count < 2 && "Queried parameter contained more than 1 value.");
    return result->count ? result->value.doubles[0] : 0.0;
}

// Get a single string or return a default value if none were passed.
HOPEDEF const char *hope_get_single_string(hope_t *hope, const char *name){
    hope_result_t *result = hope_search_result(hope->results, hope->nresults, name);
    assert(result && "Queried parameter could not be found.");
    assert(result->type == HOPE_TYPE_STRING && "Queried parameter is not of string type");
    assert(result->count < 2 && "Queried parameter contained more than 1 value.");
    return result->count ? result->value.strings[0] : NULL;
}
#endif // HOPE_IMPLEMENTATION
#endif // HOPE_H_
