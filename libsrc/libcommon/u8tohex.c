#include <stdint.h>

void u8tohex(uint8_t value, char* buffer, char alpha) {
    // Convert high nibble
    uint8_t high = (value >> 4) & 0x0F;
    buffer[0] = (high < 10) ? ('0' + high) : (alpha + high - 10);

    // Convert low nibble
    uint8_t low = value & 0x0F;
    buffer[1] = (low < 10) ? ('0' + low) : (alpha + low - 10);

    buffer[2] = 0; // Null terminator
}