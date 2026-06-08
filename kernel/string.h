/* kernel/string.h — Luna OS freestanding string library */
#ifndef LUNA_STRING_H
#define LUNA_STRING_H

#include "../include/types.h"

/* Memory */
void* luna_memset(void* dst, int val, size_t n);
void* luna_memcpy(void* dst, const void* src, size_t n);
void* luna_memmove(void* dst, const void* src, size_t n);
int   luna_memcmp(const void* a, const void* b, size_t n);
void* luna_memchr(const void* s, int c, size_t n);

/* Length */
size_t luna_strlen(const char* s);
size_t luna_strnlen(const char* s, size_t max);

/* Copy / concat */
char* luna_strcpy(char* dst, const char* src);
char* luna_strncpy(char* dst, const char* src, size_t n);
char* luna_strcat(char* dst, const char* src);
char* luna_strncat(char* dst, const char* src, size_t n);

/* Compare */
int luna_strcmp(const char* a, const char* b);
int luna_strncmp(const char* a, const char* b, size_t n);
int luna_strcasecmp(const char* a, const char* b);

/* Search */
char* luna_strchr(const char* s, int c);
char* luna_strrchr(const char* s, int c);
char* luna_strstr(const char* haystack, const char* needle);
char* luna_strtok(char* str, const char* delim, char** saveptr);

/* Numeric conversion */
int       luna_atoi(const char* s);
long long luna_atoll(const char* s);
uint32_t  luna_strtoul(const char* s, char** end, int base);

/* Formatting */
int luna_snprintf(char* buf, size_t size, const char* fmt, ...);
int luna_sprintf(char* buf, const char* fmt, ...);

#endif /* LUNA_STRING_H */
