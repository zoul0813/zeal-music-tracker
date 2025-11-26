#include <stdint.h>

// Helper function to convert an integer to a string
void itoa(uint16_t num, char* str, uint16_t base, char alpha) {
    uint16_t i = 0, j, k, rem;

    // safety checks
    if(base < 2 || base > 36) {
        str[0] = '\0';
        return;
    }

    // Handle 0 explicitly, otherwise empty string is printed
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Process individual digits
    while (num != 0) {
        rem = num % base;
        str[i++] = (char)((rem > 9) ? (rem - 10) + alpha : rem + '0');
        num = num / base;
    }

    str[i] = '\0';

    // Reverse the string
    for (j = 0, k = i - 1; j < k; j++, k--) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}