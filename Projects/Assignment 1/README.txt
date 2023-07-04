Assignment 1

During the tutorial periods in Week 4 (Jan 30th) or Week 5 (Feb 6th), 
you need to demonstrate to the Lecturer, or one of the TAs that you have successfully 
installed the CC2650 development tool on your personal laptop (or LattePanda) by running 
the “Hello-World” program.

Important: In your demonstration, you should change the output to print your “Name” 
instead of the string “Hello World”. Furthermore, you should also blink one of the 
onboard LED (actuator) on the sensor tag platform.


Windows Setup

Install Windows Subsystem for Linux (WSL) Ubuntu 22.04.1 LTS

Launch Bash Ubuntu Shell with username = imacellist, password = p5hrNIpu@6034

Install Contiki OS via git clone into contiki-ng directory 

Install ARM GCC compiler vis sudo apt-get, check installed with arm-none-eabi-gcc --version

Download and install UniFlash

Open file explorer in current directory using explorer.exe . for GUI

cd into contiki-ng/examples/hello-world

Open hello-world.c in editor to change output to print "Hello, Jianning!"

Compile hello-world.c program 

sudo make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 PORT=/dev/ttyACM0 hello-world

Binary Image is hello-world.cc26x0-cc13x0

Open UniFlash and detect board

Choose CC2650F128 device and Texas Instruments XDS110 USB Debug Probe

Copy path from file explorer \\wsl.localhost\Ubuntu-22.04\home\imacellist\contiki-ng\examples\hello-world
in browse file name and select hello-world.cc26x0-cc13x0

Need to select All Files to see hello-world.cc26x0-cc13x0 (default Custom Files)

Load Image and check success message

To erase flash if necessary:
https://www.zigbee2mqtt.io/guide/adapters/flashing/flashing_via_uniflash.html

To update sensortag firmware:
https://software-dl.ti.com/ccs/esd/documents/xdsdebugprobes/emu_xds_software_package_download.html

Install Realterm and follow settings (every time) to see output

Display: Choose Ansi (default All Chars of Ascii), check newLine mode, change Rows to 40
Port: baudrate = 115200 and choose same UART port in Device Manager (COM6 for now)
Press Change and select Open

Press reset button on SensorTag and verify output

To make own app:
1. create new directory in /examples
2. make .c file
3. copy Makefile
4. compile

LEDS_RED 	1 (01 bitmask)
LEDS_GREEN 	2 (10 bitmask)
LEDS_ALL 	3 (11 bitmask)