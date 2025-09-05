#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_timer.h"
#include "esp_log.h"

#define ADC_CHANNEL ADC1_CHANNEL_1 // GPIO 1
#define ADC_THRESHOLD 49           // ADC threshold for logical high
#define DOT_DURATION 100           // Dot duration in ms (0.1s)
#define DASH_DURATION 300          // Dash duration in ms (0.3s)
#define SYMBOL_PAUSE DOT_DURATION           // Pause between symbols in ms (0.1s)
#define LETTER_PAUSE DASH_DURATION           // Pause between letters in ms (0.3s)
#define WORD_PAUSE 500             // Pause between words in ms (0.5s)
#define POLLING_PERIOD 3333 // about 1/3 of dot duration so that 3 polls are taken per symbol
#define DOT 0x1
#define DASH 0x2

static const char *TAG = "morse_decoder";

static const uint16_t MORSE_CODE[] = {
    0b0110, 0b10010101, 0b10011001, 0b100101, 0b01, 0b01011001, 0b101001, 0b01010101, 0b0101, 0b01101010, 
    0b100110, 0b01100101, 0b1010, 0b1001, 0b101010, 0b01101001, 0b10100110, 0b011001, 0b010101, 0b10, 
    0b010110, 0b01010110, 0b011010, 0b10010110, 0b10011010, 0b10100101, 0b1010101010, 0b0110101010,
    0b0101101010, 0b0101011010, 0b0101010110, 0b0101010101, 0b1001010101, 0b1010010101, 0b1010100101,
    0b1010101001
};

static uint16_t morse_buffer;
static int morse_index = 0;
static uint8_t POLL_COUNTER = 0;
static uint8_t POLL_HISTORY = 0x0;
static uint8_t is_high = 1;
static int duration = 0;

void process_morse_code();
void decode_and_print_letter();
void morse_timer_callback(void *arg);
bool poll_end(void);
bool poll_decide(void);

void app_main() {
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_0);

    // Create a periodic timer
    const esp_timer_create_args_t timer_args = {
        .callback = &morse_timer_callback,
        .arg = NULL,
        .name = "morse_timer"
    };

    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, POLLING_PERIOD)); // Poll every 10ms
}

bool poll_end(){
    if (POLL_COUNTER % 3 == 0){
	return 1;
    }
    return 0;
}

bool poll_decide(){
    bool ret = 0;
    uint8_t parity = 0 | POLL_HISTORY;
    parity <<= 5;
    parity >>= 5;
//    printf("parity: %x\n", parity);
    if (parity == 3 || parity == 6 || parity == 5 || parity == 7) ret = 1;
//    printf("decision: %x\n", ret);
    POLL_COUNTER = 0;
    POLL_HISTORY = 0;
    return ret;
}

// Timer callback to poll ADC and handle Morse logic
void morse_timer_callback(void *arg) {
    int adc_value = adc1_get_raw(ADC_CHANNEL);
//    printf("adc_value: %d\n", adc_value);
    uint8_t current_state = adc_value < ADC_THRESHOLD;
    POLL_COUNTER++;
    POLL_HISTORY = (POLL_HISTORY << 1) | current_state;
    bool decision_time = poll_end();
    if (!decision_time) goto undecided;
    bool decision = poll_decide();	       
    if (decision == is_high) {	       
        duration += 10; // Add 10ms to the current duration
    } else {
        if (is_high) {
            // Handle high state duration
//	    ESP_LOGI(TAG, "was high for: %d\n", duration);
            if (duration >= DOT_DURATION*0.7 && duration <= DOT_DURATION*1.3) {
//		ESP_LOGI(TAG, "dot detected\n");
		morse_buffer = (morse_buffer << 2) | DOT;
		morse_index++;
            } else if (duration >= DASH_DURATION*0.7 && duration <= DASH_DURATION*1.3) {	
//		ESP_LOGI(TAG, "dash detected\n");
		morse_buffer = (morse_buffer << 2) | DASH;
		morse_index++;
            }
        } else {
	    
//            ESP_LOGI(TAG, "was low for: %d\n", duration);
            // Handle low state duration
            if (duration >= WORD_PAUSE*0.6 && duration <= WORD_PAUSE*1.4) {
//		ESP_LOGI(TAG, "word pause detected\n");
		decode_and_print_letter();
                ESP_LOGI(TAG, " "); // Print a space for a word pause
            } else if (duration >= LETTER_PAUSE*0.8 && duration <= LETTER_PAUSE*1.2) {
//		ESP_LOGI(TAG, "letter pause detected\n");
                decode_and_print_letter();
            }
        }

        // Reset duration and toggle state
        duration = 0;
        is_high = decision;
    }
    // handle end of tx (signal won't go high again)
    if (!decision && duration >= WORD_PAUSE){
	decode_and_print_letter();
    }
    undecided:
    // Handle buffer overflow
    if (morse_index >= 16) {
	//ESP_LOGI(TAG, "how often is this happening\n");
        decode_and_print_letter();
    }
}

// Decode the Morse code in the buffer and print the corresponding letter
void decode_and_print_letter() {
    if (morse_index == 0) return; // No symbol to decode
    //morse_buffer[morse_index] = '\0'; // Null-terminate the string
    for (int i = 0; i < 36; i++) {
        if (morse_buffer == MORSE_CODE[i]) {
		if (i <= 26) {
		    ESP_LOGI(TAG, "\n%c\n", 'A' + i);
		} else {
		    ESP_LOGI(TAG, "\n%c\n", '0' + i - 26);
		}
            break;
        }
    }

    // Clear the buffer
    morse_index = 0;
    morse_buffer = 0x0;
}
