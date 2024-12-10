#include <stdio.h>
#include <string.h>

int my_puts(const char *s) {
    char buf[200] = {0};

    strcpy(buf, "Hooked: ");
    strcpy(buf + strlen("Hooked: "), s);
    return puts(buf);
}

__attribute__((used)) static struct { const void *replacement; const void *replacee; } _interpose_puts
__attribute__((section ("__DATA,__interpose"))) = { (const void *)(unsigned long)&my_puts, (const void *)(unsigned long)&puts };
