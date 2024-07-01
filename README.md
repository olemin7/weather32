## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.



https://github.com/espressif/esp-iot-solution

# esp32c3_supermini
    Pin     | ADC | func |
GPIO_NUM_0  | A0  |      |
GPIO_NUM_1  | A1  |      |
GPIO_NUM_2  | A2  |      |
GPIO_NUM_3  | A3  |      |
GPIO_NUM_4  | A4  | SCK  |
3V3
GND
5V
GPIO_NUM_5  | A5  | MISO |
GPIO_NUM_6  |     | MOSI |
GPIO_NUM_7  |     | SS   |
GPIO_NUM_8  |     | SDA  | LED
GPIO_NUM_9  |     | SDL  |
GPIO_NUM_10 |     |      |
GPIO_NUM_20 |     | RX   |
GPIO_NUM_21 |     | TX   |




