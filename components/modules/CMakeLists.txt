# The bridge's modules, one folder each: LED, UART link, Wi-Fi link, MQTT link.
# The certificates mqtt_link.c uses are embedded by src/CMakeLists.txt.
idf_component_register(
    SRCS "led/led.c" "uart_link/uart_link.c" "wifi_link/wifi_link.c" "mqtt_link/mqtt_link.c"
    INCLUDE_DIRS "led" "uart_link" "wifi_link" "mqtt_link"  # the modules' headers
    PRIV_INCLUDE_DIRS "../../include"                         # board.h and secrets/
    PRIV_REQUIRES esp_driver_rmt esp_driver_uart esp_wifi esp_netif esp_event esp_timer mqtt
)
