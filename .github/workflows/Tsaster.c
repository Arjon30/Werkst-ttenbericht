#include <stdio.h>
#include "pico/stdlib.h"

#define RED_PIN     20
#define YELLOW_PIN  19
#define GREEN_PIN   18

#define TASTER1 22
#define TASTER2 3
#define TASTER3 21
#define TASTER4 2

void ampel_init(void) {
    gpio_init(RED_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);

    gpio_init(YELLOW_PIN);
    gpio_set_dir(YELLOW_PIN, GPIO_OUT);

    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
}

void ampel_set(bool red, bool yellow, bool green) {
    gpio_put(RED_PIN, red);
    gpio_put(YELLOW_PIN, yellow);
    gpio_put(GREEN_PIN, green);
}

void taster_init(void){
    gpio_init(TASTER1);
    gpio_set_dir(TASTER1, GPIO_IN);
    gpio_pull_up(TASTER1);  // Pull-Up aktivieren

    gpio_init(TASTER2);
    gpio_set_dir(TASTER2, GPIO_IN);
    gpio_pull_up(TASTER2);

    gpio_init(TASTER3);
    gpio_set_dir(TASTER3, GPIO_IN);
    gpio_pull_up(TASTER3);

    gpio_init(TASTER4);
    gpio_set_dir(TASTER4, GPIO_IN);
    gpio_pull_up(TASTER4);
}

int main() {
    stdio_init_all();
    ampel_init();
    taster_init();

    while (true) {
        if (gpio_get(TASTER1) == 0) {  // gedrückt = 0 wegen Pull-Up
            ampel_set(true, false, false);  // Rot
        } else if (gpio_get(TASTER2) == 0) {
            ampel_set(false, true, false);  // Gelb
        } else if (gpio_get(TASTER3) == 0) {
            ampel_set(false, false, true);  // Grün
        } else if (gpio_get(TASTER4) == 0) {
            ampel_set(true, true, true);    // Alle LEDs an
        } else {
            ampel_set(false, false, false); // Keine LED an
        }

    }
}
