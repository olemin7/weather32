## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

https://esp-idf-lib.readthedocs.io/en/latest/groups/bmp180.html

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


https://github.com/DiegoPaezA/ESP32-freeRTOS/blob/master/i2c_scanner/main/i2c_scanner.c
https://github.com/espressif/esp-iot-solution/tree/dd15770d0f8c5fedea78809737f04b63d3765c83/examples/sensors/sensor_hub_monitor


. /home/oleksandr/ides/tools/esp-idf-v5.3/export.sh
export IOT_SOLUTION_PATH=/home/oleksandr/personal/repo/esp-iot-solution
idf.py set-target esp32c3
idf.py build
idf.py menuconfig

null in the env https://github.com/espressif/idf-eclipse-plugin/issues/535https://components.espressif.com/

Check the values in your sdkconfig for LOG_MAXIMUM_LEVEL. It defaults to matching LOG_DEFAULT_LEVEL which limits what values you can use by default.


[mqtt test]
one terminal
mosquitto_sub -d -t hello/world
mosquitto_sub -d -t stat/weather -h central.local
Connect to another ssh session and run
mosquitto_pub -d -t hello/world -m "Hello from terminal window 2!"
mosquitto_pub -h central.local -d -t stat/pr_clock -m "Hello from terminal window 2!"
sudo tail -f /var/log/mosquitto/mosquitto.log

https://github.com/nopnop2002/esp-idf-json/tree/master/json-basic-object
