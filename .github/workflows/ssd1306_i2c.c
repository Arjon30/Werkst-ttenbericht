/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "raspberry26x32.h"
#include "ssd1306_font.h"
#include <time.h>

#define SSD1306_HEIGHT              32
#define SSD1306_WIDTH               128

#define SSD1306_I2C_ADDR            _u(0x3C)
#define SSD1306_I2C_CLK             400

#define TASTER1HOUR 2
#define TASTER2MIN 3
#define TASTER1SEK 22
#define TASTER_ALARM 21
#define LED_PIN 20

#define DEBOUNCE_MS 200  // Entprellzeit in Millisekunden

// commands (see datasheet)
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

#define IMG_WIDTH 26
#define IMG_HEIGHT 32

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int buflen;
};

void calc_render_area_buflen(struct render_area *area) {
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

#ifdef i2c_default

void SSD1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t *temp_buf = malloc(buflen + 1);
    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);
    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);
    free(temp_buf);
}

void SSD1306_init() {
    uint8_t cmds[] = {
        SSD1306_SET_DISP,
        SSD1306_SET_MEM_MODE,
        0x00,
        SSD1306_SET_DISP_START_LINE,
        SSD1306_SET_SEG_REMAP | 0x01,
        SSD1306_SET_MUX_RATIO,
        SSD1306_HEIGHT - 1,
        SSD1306_SET_COM_OUT_DIR | 0x08,
        SSD1306_SET_DISP_OFFSET,
        0x00,
        SSD1306_SET_COM_PIN_CFG,
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        SSD1306_SET_DISP_CLK_DIV,
        0x80,
        SSD1306_SET_PRECHARGE,
        0xF1,
        SSD1306_SET_VCOM_DESEL,
        0x30,
        SSD1306_SET_CONTRAST,
        0xFF,
        SSD1306_SET_ENTIRE_ON,
        SSD1306_SET_NORM_DISP,
        SSD1306_SET_CHARGE_PUMP,
        0x14,
        SSD1306_SET_SCROLL | 0x00,
        SSD1306_SET_DISP | 0x01,
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_scroll(bool on) {
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00,
        0x00,
        0x00,
        SSD1306_NUM_PAGES - 1,
        0x00,
        0xFF,
        SSD1306_SET_SCROLL | (on ? 0x01 : 0)
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area) {
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

static void SetPixel(uint8_t *buf, int x,int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);
    const int BytesPerRow = SSD1306_WIDTH;
    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}

static inline int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 1;
    }
    else if (ch >= '0' && ch <= '9') {
        return ch - '0' + 27;
    }
    else if (ch == ':') {
        return 37;
    }
    else {
        return 0;
    }
}

static void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    y = y/8;
    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (int i=0;i<8;i++) {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

static void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str) {
        WriteChar(buf, x, y, *str++);
        x+=8;
    }
}

#endif

// Globale Zeitvariablen
int manual_hours = 0;
int manual_minutes = 0;
int manual_seconds = 0;

// Wecker-Variablen
bool alarm_setting_mode = false;  // Wecker-Einstellmodus
bool alarm_enabled = false;       // Wecker aktiviert
int alarm_hours = 0;
int alarm_minutes = 0;
int alarm_seconds = 0;
bool alarm_triggered = false;     // Wecker ausgelöst
uint64_t alarm_trigger_time = 0;  // Zeitpunkt der Auslösung

// Letzte Tasterzustände für Entprellung
uint64_t last_press_time[4] = {0, 0, 0, 0};

// Taster-Handler mit Entprellung
void handle_buttons() {
    uint64_t now = time_us_64();
    
    // Taster für Wecker-Modus (GP21)
    if (!gpio_get(TASTER_ALARM)) {
        if ((now - last_press_time[3]) > (DEBOUNCE_MS * 1000)) {
            if (alarm_triggered) {
                // Wecker ausschalten wenn er läutet
                alarm_triggered = false;
                gpio_put(LED_PIN, 0);
            } else if (alarm_setting_mode) {
                // Wecker-Einstellung beenden und aktivieren
                alarm_setting_mode = false;
                alarm_enabled = true;
            } else {
                // Wecker-Einstellmodus starten
                alarm_setting_mode = true;
                alarm_enabled = false;
                alarm_triggered = false;
                alarm_hours = 0;
                alarm_minutes = 0;
                alarm_seconds = 0;
            }
            last_press_time[3] = now;
        }
    }
    
    // Im Wecker-Einstellmodus die Wecker-Zeit ändern
    if (alarm_setting_mode) {
        // Taster für Stunden (GP2)
        if (!gpio_get(TASTER1HOUR)) {
            if ((now - last_press_time[0]) > (DEBOUNCE_MS * 1000)) {
                alarm_hours++;
                if (alarm_hours >= 24) {
                    alarm_hours = 0;
                }
                last_press_time[0] = now;
            }
        }
        
        // Taster für Minuten (GP3)
        if (!gpio_get(TASTER2MIN)) {
            if ((now - last_press_time[1]) > (DEBOUNCE_MS * 1000)) {
                alarm_minutes++;
                if (alarm_minutes >= 60) {
                    alarm_minutes = 0;
                }
                last_press_time[1] = now;
            }
        }
        
        // Taster für Sekunden (GP22)
        if (!gpio_get(TASTER1SEK)) {
            if ((now - last_press_time[2]) > (DEBOUNCE_MS * 1000)) {
                alarm_seconds++;
                if (alarm_seconds >= 60) {
                    alarm_seconds = 0;
                }
                last_press_time[2] = now;
            }
        }
    } else {
        // Normale Modus: Aktuelle Zeit ändern
        // Taster für Stunden (GP2)
        if (!gpio_get(TASTER1HOUR)) {
            if ((now - last_press_time[0]) > (DEBOUNCE_MS * 1000)) {
                manual_hours++;
                if (manual_hours >= 24) {
                    manual_hours = 0;
                }
                last_press_time[0] = now;
            }
        }
        
        // Taster für Minuten (GP3)
        if (!gpio_get(TASTER2MIN)) {
            if ((now - last_press_time[1]) > (DEBOUNCE_MS * 1000)) {
                manual_minutes++;
                if (manual_minutes >= 60) {
                    manual_minutes = 0;
                }
                last_press_time[1] = now;
            }
        }
        
        // Taster für Sekunden (GP22)
        if (!gpio_get(TASTER1SEK)) {
            if ((now - last_press_time[2]) > (DEBOUNCE_MS * 1000)) {
                manual_seconds++;
                if (manual_seconds >= 60) {
                    manual_seconds = 0;
                }
                last_press_time[2] = now;
            }
        }
    }
}

// Wecker überprüfen und LED blinken lassen
void check_alarm(int current_hours, int current_minutes, int current_seconds) {
    if (alarm_enabled && !alarm_triggered) {
        // Prüfen ob die Wecker-Zeit erreicht wurde
        if (current_hours == alarm_hours && 
            current_minutes == alarm_minutes && 
            current_seconds == alarm_seconds) {
            alarm_triggered = true;
            alarm_trigger_time = time_us_64();
        }
    }
    
    // LED blinken lassen wenn Wecker ausgelöst
    if (alarm_triggered) {
        uint64_t now = time_us_64();
        uint64_t elapsed = (now - alarm_trigger_time) / 1000000; // in Sekunden
        
        // Blinken mit 2Hz (alle 500ms wechseln)
        if ((elapsed % 1) == 0 && ((now / 500000) % 2) == 0) {
            gpio_put(LED_PIN, 1);
        } else {
            gpio_put(LED_PIN, 0);
        }
    }
}

int main() {
    stdio_init_all();

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c / SSD1306_i2d example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("SSD1306 OLED driver I2C example for the Raspberry Pi Pico"));

    printf("Hello, SSD1306 OLED display! Look at my raspberries..\n");

    // I2C initialisieren
    i2c_init(i2c_default, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Taster initialisieren (als Eingänge mit Pull-up)
    gpio_init(TASTER1HOUR);
    gpio_set_dir(TASTER1HOUR, GPIO_IN);
    gpio_pull_up(TASTER1HOUR);

    gpio_init(TASTER2MIN);
    gpio_set_dir(TASTER2MIN, GPIO_IN);
    gpio_pull_up(TASTER2MIN);

    gpio_init(TASTER1SEK);
    gpio_set_dir(TASTER1SEK, GPIO_IN);
    gpio_pull_up(TASTER1SEK);

    gpio_init(TASTER_ALARM);
    gpio_set_dir(TASTER_ALARM, GPIO_IN);
    gpio_pull_up(TASTER_ALARM);

    // LED initialisieren (als Ausgang)
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // LED aus

    SSD1306_init();

    struct render_area frame_area = {
        .start_col = 0,
        .end_col = SSD1306_WIDTH - 1,
        .start_page = 0,
        .end_page = SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // Intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        SSD1306_send_cmd(SSD1306_SET_ALL_ON);
        sleep_ms(500);
        SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON);
        sleep_ms(500);
    }

    struct render_area area = {
        .start_page = 0,
        .end_page = (IMG_HEIGHT / SSD1306_PAGE_HEIGHT) - 1
    };

    area.start_col = 0;
    area.end_col = IMG_WIDTH - 1;
    calc_render_area_buflen(&area);

    uint8_t offset = 5 + IMG_WIDTH;

    for (int i = 0; i < 3; i++) {
        render(raspberry26x32, &area);
        area.start_col += offset;
        area.end_col += offset;
    }

    SSD1306_scroll(true);
    sleep_ms(5000);
    SSD1306_scroll(false);

    uint64_t start_time = time_us_64();

    while (true) {
        // Taster überprüfen
        handle_buttons();
        
        uint64_t now = time_us_64();
        double elapsed_s = (now - start_time) / 1e6;

        int hours = ((int)(elapsed_s / 3600) + manual_hours) % 24;
        int minutes = ((int)(elapsed_s / 60) + manual_minutes) % 60;
        int seconds = ((int)(elapsed_s) + manual_seconds) % 60;

        // Wecker überprüfen
        check_alarm(hours, minutes, seconds);

        char laufzeit_str[9];
        snprintf(laufzeit_str, sizeof(laufzeit_str), "%02d:%02d:%02d", hours, minutes, seconds);

        char cruz[] = "CRUZ";
        char arjon[] = "ARJON";
        
        char alarm_str[16];
        if (alarm_setting_mode) {
            // Wecker-Einstellmodus: Zeige "SET:" mit der eingestellten Zeit
            snprintf(alarm_str, sizeof(alarm_str), "SET:%02d:%02d:%02d", alarm_hours, alarm_minutes, alarm_seconds);
        } else if (alarm_enabled) {
            // Wecker aktiv: Zeige "ALM:" mit der Wecker-Zeit
            snprintf(alarm_str, sizeof(alarm_str), "ALM:%02d:%02d:%02d", alarm_hours, alarm_minutes, alarm_seconds);
        } else {
            // Kein Wecker: Zeige nichts oder "NO ALARM"
            snprintf(alarm_str, sizeof(alarm_str), "        ");
        }

        char *text[] = { cruz, arjon, laufzeit_str, alarm_str };

        memset(buf, 0, SSD1306_BUF_LEN);

        int y = 0;
        for (int i = 0; i < 4; i++) {
            WriteString(buf, 5, y, text[i]);
            y += 8;
        }

        render(buf, &frame_area);

        // Optional: invert display alle 300 Sekunden (5 Minuten)
        if (elapsed_s > 300) {
            SSD1306_send_cmd(SSD1306_SET_INV_DISP);
            sleep_ms(3000);
            SSD1306_send_cmd(SSD1306_SET_NORM_DISP);
            start_time = time_us_64();
            manual_hours = 0;
            manual_minutes = 0;
            manual_seconds = 0;
        }

        sleep_ms(100);  // Kürzerer Sleep für bessere Taster-Reaktion
    }

#endif
    return 0;
}