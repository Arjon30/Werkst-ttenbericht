/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include "pico/stdlib.h"

 // Definiere die GPIO-Pins für die Ampel
 #define RED_PIN     20
 #define YELLOW_PIN  19
 #define GREEN_PIN   18
 
 // Initialisiere alle drei Pins als Ausgang
 void ampel_init(void) {
     gpio_init(RED_PIN);
     gpio_set_dir(RED_PIN, GPIO_OUT);
 
     gpio_init(YELLOW_PIN);
     gpio_set_dir(YELLOW_PIN, GPIO_OUT);
 
     gpio_init(GREEN_PIN);
     gpio_set_dir(GREEN_PIN, GPIO_OUT);
 }
 
 // Hilfsfunktion zum Setzen der Ampelzustände
 void ampel_set(bool red, bool yellow, bool green) {
     gpio_put(RED_PIN, red);
     gpio_put(YELLOW_PIN, yellow);
     gpio_put(GREEN_PIN, green);
 }
 
 int main() {
     stdio_init_all();
     ampel_init();
     while (true) {
         // Rot an, Gelb & Grün aus
         ampel_set(true, false, false);
         sleep_ms(2000);
 
         // Rot und Gelb an
         ampel_set(true, true, false);
         sleep_ms(1500);
 
         // Grün an
         ampel_set(false, false, true);
         sleep_ms(1000);
        
         ampel_set(false,false,true);
         ampel_set(false,false,true);
         sleep_ms(100);
         ampel_set(false,false,true);
         sleep_ms(100);
         ampel_set(false,false,true);
         sleep_ms(100);
         ampel_set(false,false,true);
         sleep_ms(100);

           
        
         // Gelb an (Übergang)
         ampel_set(false, true, false);
         sleep_ms(1500);
     }
 }
 