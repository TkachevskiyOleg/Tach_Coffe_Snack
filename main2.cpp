#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#define DATA_PIN   25   // SER
#define LATCH_PIN  18   // RCLK
#define CLOCK_PIN  19   // SRCLK
#define OE_PIN     5    // OE
#define RESET_PIN  6    // SRCLR

#define WS2812_PIN 16
#define IS_RGBW false
#define NUM_PIXELS 1

// Відправка байта у 74HC595 (LSB first)
void shiftOut595(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_put(DATA_PIN, (data >> i) & 1);
        gpio_put(CLOCK_PIN, 1);
        sleep_us(1);
        gpio_put(CLOCK_PIN, 0);
    }
}

// Встановлення кольору WS2812
void set_led_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = ((uint32_t)(g) << 16) |
                     ((uint32_t)(r) << 8)  |
                     (uint32_t)(b);
    pio_sm_put_blocking(pio0, 0, color << 8u);
}

// Увімкнути конкретну кнопку (1..14)
void setButton(int btnNumber) {
    if (btnNumber < 1 || btnNumber > 14) return; // захист від некоректних номерів
    uint16_t mask = 1 << (btnNumber - 1);
    gpio_put(LATCH_PIN, 0);
    shiftOut595(mask & 0xFF);
    shiftOut595((mask >> 8) & 0xFF);
    gpio_put(LATCH_PIN, 1);
}

int main() {
    stdio_init_all();

    // Ініціалізація GPIO для 74HC595
    gpio_init(DATA_PIN);   gpio_set_dir(DATA_PIN, true);
    gpio_init(LATCH_PIN);  gpio_set_dir(LATCH_PIN, true);
    gpio_init(CLOCK_PIN);  gpio_set_dir(CLOCK_PIN, true);
    gpio_init(OE_PIN);     gpio_set_dir(OE_PIN, true);
    gpio_init(RESET_PIN);  gpio_set_dir(RESET_PIN, true);

    gpio_put(OE_PIN, 0);
    gpio_put(RESET_PIN, 1);

    // Ініціалізація WS2812
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Початковий стан — очікування
    set_led_color(0, 0, 255); // синій

    while (true) {
        int c = getchar_timeout_us(10000);
        if (c == 0xAA) {
            int btn = getchar_timeout_us(100000);
            int dur_hi = getchar_timeout_us(100000);
            int dur_lo = getchar_timeout_us(100000);
            int stop = getchar_timeout_us(100000);

            bool ok = (btn != PICO_ERROR_TIMEOUT)
                   && (dur_hi != PICO_ERROR_TIMEOUT)
                   && (dur_lo != PICO_ERROR_TIMEOUT)
                   && (stop == 0x55)
                   && (btn >= 1 && btn <= 14);

            int hold_ms = 1000;
            if (dur_hi != PICO_ERROR_TIMEOUT && dur_lo != PICO_ERROR_TIMEOUT)
                hold_ms = ((dur_hi & 0xFF) << 8) | (dur_lo & 0xFF);
            if (hold_ms < 100) hold_ms = 100;
            if (hold_ms > 30000) hold_ms = 30000;

            if (ok) {
                set_led_color(0, 255, 0);
                putchar(0xBB);
                fflush(stdout);
                setButton(btn);
                sleep_ms(hold_ms);
                setButton(0);
                set_led_color(0, 0, 255);
            } else {
                set_led_color(255, 0, 0);
                putchar(0xEE);
                fflush(stdout);
                sleep_ms(500);
                set_led_color(0, 0, 255);
            }
        }
    }
}
