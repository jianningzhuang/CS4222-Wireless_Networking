Steps to compile and execute programs:

1. copy source-code directory into contiki-ng/examples/
2. open a terminal and cd into contiki-ng/examples/source-code
3. run make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 nbr_light_sensor
4. run make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 nbr_relay
5. flash nbr_light_sensor.cc26x0-cc13x0 onto light sensor SensorTag
6. flash nbr_relay.cc26x0-cc13x0 onto relay SensorTags