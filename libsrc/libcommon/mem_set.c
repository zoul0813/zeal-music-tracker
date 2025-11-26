#include <stdint.h>
#include <stddef.h>

void* mem_set(void* ptr, uint8_t value, size_t size)
{
    uint8_t* p = ptr;
    while (size--) {
        *p++ = value;
    }
    return ptr;
}