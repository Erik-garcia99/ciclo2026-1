idf_component_register(
    SRCS 
        "main.c" 
        "modules/MQTT/mqtt_lib.c" 
        "modules/WIIF/wifi_lib.c"
                    
    INCLUDE_DIRS 
        "." 
        "modules/MQTT"
        "modules/WIFI"
                    
    REQUIRES
        mqtt
)
