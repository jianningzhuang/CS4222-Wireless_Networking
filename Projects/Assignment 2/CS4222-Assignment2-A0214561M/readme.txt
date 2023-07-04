From the output of etimer-buzzer.c, note down the value of CLOCK_SECOND. 
Find out how many clock ticks corresponds to 1s in real time.

The value of CLOCK_SECOND is 128

The number of clock ticks per 1s is 128

refer to screenshot of etimer-buzzer resolution


From the output of rtimer-lightSensor.c. note down the value of RTIMER_SECOND. 
Find out how many clock ticks corresponds to 1s in real time.

The value of RTIMER_SECOND is 65536

The number of clock ticks per 1s is 128

refer to screenshot of rtimer-lightSensor resolution


Instructions to run buzz.c

1.  create new directory in contiki-ng/examples
2.  copy Makefile and change CONTIKI_PROJECT = etimer-buzzer rtimer-lightSensor rtimer-IMUSensor
3.  copy buzz.c into directory
4.  sudo make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 PORT=/dev/ttyACM0 buzz (or just use buzz.cc26x0-cc13x0 provided)
5.  binary image is buzz.cc26x0-cc13x0
6.  open UniFlash and detect board
7.  choose CC2650F128 device and Texas Instruments XDS110 USB Debug Probe
8.  select All Files to see buzz.cc26x0-cc13x0 (default Custom Files)
9.  load image and check success message
10. open Realterm and change settings

Display: Choose Ansi (default All Chars of Ascii), check newLine mode, change Rows to 40
Port: baudrate = 115200 and choose same UART port in Device Manager (COM6 for now)
Press Change and select Open

11. press reset button on SensorTag and verify output


IMU data analysis

flat on table:
Gyro X = -1.40 to -1.10 deg/s
Gyro Y = 1.30 to 1.80 deg/s
Gyro Z = -0.20 to 0.10 deg/s

AccX = 0.01 to 0.03 G
AccY = 0.01 to 0.03 G
AccZ = -0.95 to -0.90 G

up down movement: (Z)

AccX = 0.20 to 0.40 G
AccY = -0.40 to -0.20 G
AccZ = -2.00 to 1.20 G

left right movement: cable to right (X)

AccX = -1.50 to 1.50 G
AccY = -0.40 to -0.20 G
AccZ = -0.90 to -0.50 G

forward back movement: (Y)

AccX = 0.20 to 0.40 G
AccY = -1.50 to 1.50 G
AccZ = -0.90 to -0.50 G

determine significant movement in each direction as higher acceleration than range specified for particular axis