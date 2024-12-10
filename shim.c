#include <stdio.h>
#include <string.h>

#define DYLD_INTERPOSE(_replacement,_replacee) \
    __attribute__((used)) static struct{ const void *replacement; const void *replacee; } _interpose_##_replacee \
    __attribute__((section ("__DATA,__interpose"))) = { (const void *)(unsigned long)&_replacement, (const void *)(unsigned long)&_replacee };

int my_puts(const char *s) {
    char buf[200] = {0};

    strcpy(buf, "Hooked: ");
    strncpy(buf + strlen("Hooked: "), s, 200 - strlen("Hooked: "));
    return puts(buf);
}

DYLD_INTERPOSE(my_puts, puts)
