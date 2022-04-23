
#include "./headers.h"
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    DDRB |= (1 << DDB0);
    while (1) {
        PORTB |= (1 << PORTB0);
        _delay_ms(250);
        PORTB &= ~(1 << PORTB0);
        _delay_ms(250);
    }
    return 0;
}


