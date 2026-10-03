#ifndef BOARD_H
#define BOARD_H

// How the bridge is wired, for the Axiometa Genesis Mini (ESP32-S3).
// Change these values to run the bridge on a different board.

// On-board RGB LED
#define LED_GPIO 21

// UART link to the MCU
#define UART_PORT   UART_NUM_1  // from driver/uart.h, included where it is used
#define UART_TX_PIN 7
#define UART_RX_PIN 6
#define UART_BAUD   115200

#endif
