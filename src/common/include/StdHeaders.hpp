// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

/**
 * @file StdHeaders.hpp
 * @brief Registry mapping standard/POSIX type and functions to their defining headers.
 *
 * Used by the transform step to re-inject the standard headers that include
 * stripping leaves missing.
 */

/**
 * @brief Maps a standard/POSIX type name to the header that defines it.
 */
inline const std::unordered_map<std::string, std::string> StdHeaders = {
    // <stdbool.h>
    // types
    {"bool", "stdbool.h"},
    // macros
    {"true", "stdbool.h"},
    {"false", "stdbool.h"},

    // <stddef.h>
    // types
    {"size_t", "stddef.h"},
    {"ptrdiff_t", "stddef.h"},
    {"wchar_t", "stddef.h"},
    {"max_align_t", "stddef.h"},
    // macros
    {"NULL", "stddef.h"},
    {"offsetof", "stddef.h"},

    // <stdint.h>
    // types
    {"int8_t", "stdint.h"},
    {"int16_t", "stdint.h"},
    {"int32_t", "stdint.h"},
    {"int64_t", "stdint.h"},
    {"uint8_t", "stdint.h"},
    {"uint16_t", "stdint.h"},
    {"uint32_t", "stdint.h"},
    {"uint64_t", "stdint.h"},
    {"intptr_t", "stdint.h"},
    {"uintptr_t", "stdint.h"},
    {"intmax_t", "stdint.h"},
    {"uintmax_t", "stdint.h"},
    {"int_least8_t", "stdint.h"},
    {"int_least16_t", "stdint.h"},
    {"int_least32_t", "stdint.h"},
    {"int_least64_t", "stdint.h"},
    {"uint_least8_t", "stdint.h"},
    {"uint_least16_t", "stdint.h"},
    {"uint_least32_t", "stdint.h"},
    {"uint_least64_t", "stdint.h"},
    {"int_fast8_t", "stdint.h"},
    {"int_fast16_t", "stdint.h"},
    {"int_fast32_t", "stdint.h"},
    {"int_fast64_t", "stdint.h"},
    {"uint_fast8_t", "stdint.h"},
    {"uint_fast16_t", "stdint.h"},
    {"uint_fast32_t", "stdint.h"},
    {"uint_fast64_t", "stdint.h"},

    // <stdio.h>
    // types
    {"FILE", "stdio.h"},
    {"fpos_t", "stdio.h"},
    // functions
    {"printf", "stdio.h"},
    {"fprintf", "stdio.h"},
    {"sprintf", "stdio.h"},
    {"snprintf", "stdio.h"},
    {"vprintf", "stdio.h"},
    {"vfprintf", "stdio.h"},
    {"vsprintf", "stdio.h"},
    {"vsnprintf", "stdio.h"},
    {"scanf", "stdio.h"},
    {"fscanf", "stdio.h"},
    {"sscanf", "stdio.h"},
    {"fopen", "stdio.h"},
    {"freopen", "stdio.h"},
    {"fclose", "stdio.h"},
    {"fflush", "stdio.h"},
    {"fread", "stdio.h"},
    {"fwrite", "stdio.h"},
    {"fseek", "stdio.h"},
    {"ftell", "stdio.h"},
    {"rewind", "stdio.h"},
    {"remove", "stdio.h"},
    {"rename", "stdio.h"},
    {"tmpfile", "stdio.h"},
    {"setbuf", "stdio.h"},
    {"setvbuf", "stdio.h"},
    {"fgetc", "stdio.h"},
    {"getc", "stdio.h"},
    {"getchar", "stdio.h"},
    {"fputc", "stdio.h"},
    {"putc", "stdio.h"},
    {"putchar", "stdio.h"},
    {"fgets", "stdio.h"},
    {"fputs", "stdio.h"},
    {"puts", "stdio.h"},
    {"ungetc", "stdio.h"},
    {"perror", "stdio.h"},
    {"feof", "stdio.h"},
    {"ferror", "stdio.h"},
    {"clearerr", "stdio.h"},
    // POSIX/GNU extensions that allocate their own buffer (caller must free)
    {"getline", "stdio.h"},
    {"getdelim", "stdio.h"},
    {"asprintf", "stdio.h"},
    {"vasprintf", "stdio.h"},

    // <stdlib.h>
    // types
    {"div_t", "stdlib.h"},
    {"ldiv_t", "stdlib.h"},
    {"lldiv_t", "stdlib.h"},
    // functions
    {"malloc", "stdlib.h"},
    {"calloc", "stdlib.h"},
    {"realloc", "stdlib.h"},
    {"reallocarray", "stdlib.h"},
    {"aligned_alloc", "stdlib.h"},
    {"posix_memalign", "stdlib.h"},
    {"free", "stdlib.h"},
    {"exit", "stdlib.h"},
    {"abort", "stdlib.h"},
    {"atexit", "stdlib.h"},
    {"system", "stdlib.h"},
    {"getenv", "stdlib.h"},
    {"atoi", "stdlib.h"},
    {"atol", "stdlib.h"},
    {"atoll", "stdlib.h"},
    {"atof", "stdlib.h"},
    {"strtol", "stdlib.h"},
    {"strtoul", "stdlib.h"},
    {"strtoll", "stdlib.h"},
    {"strtoull", "stdlib.h"},
    {"strtod", "stdlib.h"},
    {"strtof", "stdlib.h"},
    {"rand", "stdlib.h"},
    {"srand", "stdlib.h"},
    {"qsort", "stdlib.h"},
    {"bsearch", "stdlib.h"},
    {"abs", "stdlib.h"},
    {"labs", "stdlib.h"},
    {"llabs", "stdlib.h"},
    {"div", "stdlib.h"},
    {"ldiv", "stdlib.h"},
    {"lldiv", "stdlib.h"},
    // macros
    {"EXIT_SUCCESS", "stdlib.h"},
    {"EXIT_FAILURE", "stdlib.h"},
    {"RAND_MAX", "stdlib.h"},

    // <string.h>
    // functions (no types beyond size_t)
    {"memcpy", "string.h"},
    {"memmove", "string.h"},
    {"memset", "string.h"},
    {"memcmp", "string.h"},
    {"memchr", "string.h"},
    {"strcpy", "string.h"},
    {"strncpy", "string.h"},
    {"strcat", "string.h"},
    {"strncat", "string.h"},
    {"strcmp", "string.h"},
    {"strncmp", "string.h"},
    {"strchr", "string.h"},
    {"strrchr", "string.h"},
    {"strstr", "string.h"},
    {"strpbrk", "string.h"},
    {"strspn", "string.h"},
    {"strcspn", "string.h"},
    {"strtok", "string.h"},
    {"strerror", "string.h"},
    {"strlen", "string.h"},
    {"strdup", "string.h"},
    {"strndup", "string.h"},

    // <wchar.h> (memory-alloc-relevant subset; the rest of wchar.h is out of
    // scope for this registry until something else needs it)
    {"wcsdup", "wchar.h"},

    // glibc/BSD (declared in <malloc.h>, distinct from <stdlib.h>'s malloc family)
    {"memalign", "malloc.h"},
    {"valloc", "malloc.h"},
    {"malloc_usable_size", "malloc.h"},

    // <sys/mman.h>
    {"mmap", "sys/mman.h"},
    {"munmap", "sys/mman.h"},
    {"mremap", "sys/mman.h"},

    // <ctype.h>
    // functions
    {"isalpha", "ctype.h"},
    {"isdigit", "ctype.h"},
    {"isalnum", "ctype.h"},
    {"isspace", "ctype.h"},
    {"ispunct", "ctype.h"},
    {"isupper", "ctype.h"},
    {"islower", "ctype.h"},
    {"iscntrl", "ctype.h"},
    {"isprint", "ctype.h"},
    {"isgraph", "ctype.h"},
    {"isxdigit", "ctype.h"},
    {"toupper", "ctype.h"},
    {"tolower", "ctype.h"},

    // <time.h>
    // types
    {"time_t", "time.h"},
    {"clock_t", "time.h"},
    {"tm", "time.h"},
    {"timespec", "time.h"},
    {"clockid_t", "time.h"},
    {"timer_t", "time.h"},
    // functions
    {"time", "time.h"},
    {"difftime", "time.h"},
    {"mktime", "time.h"},
    {"asctime", "time.h"},
    {"ctime", "time.h"},
    {"gmtime", "time.h"},
    {"localtime", "time.h"},
    {"strftime", "time.h"},
    {"clock", "time.h"},
    // macros
    {"CLOCKS_PER_SEC", "time.h"},

    // <signal.h>
    // types
    {"sig_atomic_t", "signal.h"},
    // functions
    {"signal", "signal.h"},
    {"raise", "signal.h"},

    // <setjmp.h>
    // types
    {"jmp_buf", "setjmp.h"},
    // functions
    {"longjmp", "setjmp.h"},

    // <math.h>
    // types
    {"float_t", "math.h"},
    {"double_t", "math.h"},
    // functions
    {"sin", "math.h"},
    {"cos", "math.h"},
    {"tan", "math.h"},
    {"atan2", "math.h"},
    {"sinh", "math.h"},
    {"cosh", "math.h"},
    {"tanh", "math.h"},
    {"exp", "math.h"},
    {"log", "math.h"},
    {"log10", "math.h"},
    {"pow", "math.h"},
    {"sqrt", "math.h"},
    {"ceil", "math.h"},
    {"floor", "math.h"},
    {"fabs", "math.h"},
    {"fmod", "math.h"},
    {"ldexp", "math.h"},
    {"frexp", "math.h"},
    {"modf", "math.h"},
    {"round", "math.h"},
    {"trunc", "math.h"},
    // macros
    {"HUGE_VAL", "math.h"},
    {"NAN", "math.h"},
    {"INFINITY", "math.h"},

    // <threads.h> / <pthread.h>
    // types
    {"thrd_t", "threads.h"},
    {"mtx_t", "threads.h"},
    {"cnd_t", "threads.h"},
    {"tss_t", "threads.h"},
    {"pthread_t", "pthread.h"},
    {"pthread_mutex_t", "pthread.h"},
    {"pthread_cond_t", "pthread.h"},
    {"pthread_attr_t", "pthread.h"},
    {"pthread_rwlock_t", "pthread.h"},
    {"pthread_key_t", "pthread.h"},
    {"pthread_once_t", "pthread.h"},
    // functions
    {"thrd_create", "threads.h"},
    {"thrd_join", "threads.h"},
    {"thrd_exit", "threads.h"},
    {"mtx_init", "threads.h"},
    {"mtx_lock", "threads.h"},
    {"mtx_unlock", "threads.h"},
    {"mtx_destroy", "threads.h"},
    {"cnd_init", "threads.h"},
    {"cnd_wait", "threads.h"},
    {"cnd_signal", "threads.h"},
    {"cnd_broadcast", "threads.h"},
    {"pthread_create", "pthread.h"},
    {"pthread_join", "pthread.h"},
    {"pthread_mutex_lock", "pthread.h"},
    {"pthread_mutex_unlock", "pthread.h"},
    {"pthread_mutex_init", "pthread.h"},
    {"pthread_mutex_destroy", "pthread.h"},
    {"pthread_cond_wait", "pthread.h"},
    {"pthread_cond_signal", "pthread.h"},
    {"pthread_cond_broadcast", "pthread.h"},
    {"pthread_exit", "pthread.h"},
    {"pthread_detach", "pthread.h"},
    {"pthread_self", "pthread.h"},

    // <semaphore.h>
    // types
    {"sem_t", "semaphore.h"},
    // functions
    {"sem_init", "semaphore.h"},
    {"sem_wait", "semaphore.h"},
    {"sem_post", "semaphore.h"},
    {"sem_destroy", "semaphore.h"},

    // <stdarg.h>
    // types
    {"va_list", "stdarg.h"},

    // <errno.h>
    // macros
    {"errno", "errno.h"},

    // <limits.h>
    // macros
    {"CHAR_BIT", "limits.h"},
    {"CHAR_MIN", "limits.h"},
    {"CHAR_MAX", "limits.h"},
    {"SCHAR_MIN", "limits.h"},
    {"SCHAR_MAX", "limits.h"},
    {"UCHAR_MAX", "limits.h"},
    {"SHRT_MIN", "limits.h"},
    {"SHRT_MAX", "limits.h"},
    {"USHRT_MAX", "limits.h"},
    {"INT_MIN", "limits.h"},
    {"INT_MAX", "limits.h"},
    {"UINT_MAX", "limits.h"},
    {"LONG_MIN", "limits.h"},
    {"LONG_MAX", "limits.h"},
    {"ULONG_MAX", "limits.h"},
    {"LLONG_MIN", "limits.h"},
    {"LLONG_MAX", "limits.h"},
    {"ULLONG_MAX", "limits.h"},

    // <float.h>
    // macros
    {"FLT_MIN", "float.h"},
    {"FLT_MAX", "float.h"},
    {"FLT_EPSILON", "float.h"},
    {"DBL_MIN", "float.h"},
    {"DBL_MAX", "float.h"},
    {"DBL_EPSILON", "float.h"},
    {"LDBL_MIN", "float.h"},
    {"LDBL_MAX", "float.h"},
    {"LDBL_EPSILON", "float.h"},

    // <inttypes.h>
    // types
    {"imaxdiv_t", "inttypes.h"},

    // <wchar.h>
    // types
    {"wint_t", "wchar.h"},
    {"mbstate_t", "wchar.h"},

    // <locale.h>
    // types
    {"lconv", "locale.h"},
    {"locale_t", "locale.h"},
    // functions
    {"setlocale", "locale.h"},
    {"localeconv", "locale.h"},

    // POSIX common types
    {"ssize_t", "unistd.h"},
    {"pid_t", "sys/types.h"},
    {"uid_t", "sys/types.h"},
    {"gid_t", "sys/types.h"},
    {"off_t", "sys/types.h"},
    {"mode_t", "sys/types.h"},
    {"dev_t", "sys/types.h"},
    {"ino_t", "sys/types.h"},
    {"nlink_t", "sys/types.h"},
    {"key_t", "sys/types.h"},
    {"id_t", "sys/types.h"},
    {"useconds_t", "sys/types.h"},

    // <unistd.h>
    // functions
    {"read", "unistd.h"},
    {"write", "unistd.h"},
    {"close", "unistd.h"},
    {"lseek", "unistd.h"},
    {"unlink", "unistd.h"},
    {"rmdir", "unistd.h"},
    {"sleep", "unistd.h"},
    {"usleep", "unistd.h"},
    {"fork", "unistd.h"},
    {"execvp", "unistd.h"},
    {"getpid", "unistd.h"},
    {"getppid", "unistd.h"},
    {"access", "unistd.h"},
    {"dup", "unistd.h"},
    {"dup2", "unistd.h"},
    {"pipe", "unistd.h"},
    {"alarm", "unistd.h"},

    // <fcntl.h>
    // functions
    {"open", "fcntl.h"},
    {"fcntl", "fcntl.h"},

    // <sys/socket.h>
    // types
    {"socklen_t", "sys/socket.h"},
    {"sa_family_t", "sys/socket.h"},
    {"sockaddr", "sys/socket.h"},

    // <netinet/in.h>
    // types
    {"in_addr_t", "netinet/in.h"},
    {"in_port_t", "netinet/in.h"},

    // <dirent.h>
    // types
    {"DIR", "dirent.h"},
    // functions
    {"opendir", "dirent.h"},
    {"readdir", "dirent.h"},
    {"closedir", "dirent.h"},

    // <sys/stat.h>
    // types
    {"stat", "sys/stat.h"},
    // functions
    {"fstat", "sys/stat.h"},
    {"lstat", "sys/stat.h"},
    {"mkdir", "sys/stat.h"},
    {"chmod", "sys/stat.h"}

    // assert.h handles by AssertRewriter in TransformAction

};

/**
 * @brief Named heap alloc/free functions relevant to memory-safety detection
 */
inline const std::unordered_set<std::string> MemoryFunctions = {
    "malloc",  "calloc",         "realloc",  "reallocarray", "aligned_alloc",
    "posix_memalign", "memalign", "valloc",  "free",
    "strdup",  "strndup",        "wcsdup",
    "mmap",    "munmap",         "mremap",
    "getline", "getdelim",       "asprintf", "vasprintf",
};

/**
 * @brief Returns true if `name` is one of the functions in `MemoryFunctions`.
 */
inline bool isMemoryFunction(const std::string &name) { return MemoryFunctions.count(name) > 0; }
