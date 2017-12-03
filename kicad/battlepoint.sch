EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:switches
LIBS:relays
LIBS:motors
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:battlepoint-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L GND #PWR01
U 1 1 5A162680
P 3550 2250
F 0 "#PWR01" H 3550 2000 50  0001 C CNN
F 1 "GND" H 3550 2100 50  0000 C CNN
F 2 "" H 3550 2250 50  0001 C CNN
F 3 "" H 3550 2250 50  0001 C CNN
	1    3550 2250
	1    0    0    -1  
$EndComp
$Comp
L +VDC #PWR02
U 1 1 5A1626BE
P 3200 1200
F 0 "#PWR02" H 3200 1100 50  0001 C CNN
F 1 "+VDC" H 3200 1450 50  0000 C CNN
F 2 "" H 3200 1200 50  0001 C CNN
F 3 "" H 3200 1200 50  0001 C CNN
	1    3200 1200
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR03
U 1 1 5A1626D2
P 4400 1150
F 0 "#PWR03" H 4400 1000 50  0001 C CNN
F 1 "+5V" H 4400 1290 50  0000 C CNN
F 2 "" H 4400 1150 50  0001 C CNN
F 3 "" H 4400 1150 50  0001 C CNN
	1    4400 1150
	1    0    0    -1  
$EndComp
$Comp
L LM7805_TO220 U1
U 1 1 5A162705
P 3550 1500
F 0 "U1" H 3400 1625 50  0000 C CNN
F 1 "LM7805_TO220" H 3550 1625 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-220-3_Vertical" H 3550 1725 50  0001 C CIN
F 3 "" H 3550 1450 50  0001 C CNN
	1    3550 1500
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 5A1628A0
P 3000 1800
F 0 "C1" H 3025 1900 50  0000 L CNN
F 1 "0.1uf" H 3025 1700 50  0000 L CNN
F 2 "Capacitors_THT:C_Rect_L7.0mm_W2.5mm_P5.00mm" H 3038 1650 50  0001 C CNN
F 3 "" H 3000 1800 50  0001 C CNN
	1    3000 1800
	1    0    0    -1  
$EndComp
$Comp
L CP C2
U 1 1 5A162915
P 3950 1750
F 0 "C2" H 3975 1850 50  0000 L CNN
F 1 "22uf" H 3975 1650 50  0000 L CNN
F 2 "Capacitors_THT:CP_Radial_D4.0mm_P1.50mm" H 3988 1600 50  0001 C CNN
F 3 "" H 3950 1750 50  0001 C CNN
	1    3950 1750
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5A1629D6
P 2550 1550
F 0 "R1" V 2630 1550 50  0000 C CNN
F 1 "5k" V 2550 1550 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P2.54mm_Vertical" V 2480 1550 50  0001 C CNN
F 3 "" H 2550 1550 50  0001 C CNN
	1    2550 1550
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5A162A83
P 2550 1850
F 0 "R2" V 2630 1850 50  0000 C CNN
F 1 "5k" V 2550 1850 50  0000 C CNN
F 2 "Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P2.54mm_Vertical" V 2480 1850 50  0001 C CNN
F 3 "" H 2550 1850 50  0001 C CNN
	1    2550 1850
	1    0    0    -1  
$EndComp
$Comp
L Barrel_Jack J7
U 1 1 5A162B66
P 1700 1650
F 0 "J7" H 1700 1860 50  0000 C CNN
F 1 "Barrel_Jack" H 1700 1475 50  0000 C CNN
F 2 "Connectors:BARREL_JACK" H 1750 1610 50  0001 C CNN
F 3 "" H 1750 1610 50  0001 C CNN
	1    1700 1650
	1    0    0    -1  
$EndComp
$Comp
L RJ45 J5
U 1 1 5A162C63
P 6250 1950
F 0 "J5" H 6450 2450 50  0000 C CNN
F 1 "PANEL" H 6100 2450 50  0000 C CNN
F 2 "Connectors:RJ45_8" H 6250 1950 50  0001 C CNN
F 3 "" H 6250 1950 50  0001 C CNN
	1    6250 1950
	1    0    0    -1  
$EndComp
$Comp
L RJ45 J6
U 1 1 5A162E03
P 6200 3450
F 0 "J6" H 6400 3950 50  0000 C CNN
F 1 "SWITCHES" H 6050 3950 50  0000 C CNN
F 2 "Connectors:RJ45_8" H 6200 3450 50  0001 C CNN
F 3 "" H 6200 3450 50  0001 C CNN
	1    6200 3450
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x06_Female J1
U 1 1 5A162ECE
P 8450 1800
F 0 "J1" H 8450 2100 50  0000 C CNN
F 1 "Arduino1" H 8450 1400 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x06_Pitch2.54mm" H 8450 1800 50  0001 C CNN
F 3 "" H 8450 1800 50  0001 C CNN
	1    8450 1800
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x06_Female J2
U 1 1 5A162FB5
P 7700 3350
F 0 "J2" H 7700 3650 50  0000 C CNN
F 1 "Arduino2" H 7700 2950 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x06_Pitch2.54mm" H 7700 3350 50  0001 C CNN
F 3 "" H 7700 3350 50  0001 C CNN
	1    7700 3350
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x08_Female J4
U 1 1 5A163044
P 3100 2950
F 0 "J4" H 3100 3350 50  0000 C CNN
F 1 "Arduino4" H 3100 2450 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x08_Pitch2.54mm" H 3100 2950 50  0001 C CNN
F 3 "" H 3100 2950 50  0001 C CNN
	1    3100 2950
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x08_Female J3
U 1 1 5A1631D7
P 3100 4000
F 0 "J3" H 3100 4400 50  0000 C CNN
F 1 "Arduino3" H 3100 3500 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x08_Pitch2.54mm" H 3100 4000 50  0001 C CNN
F 3 "" H 3100 4000 50  0001 C CNN
	1    3100 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 1200 2550 1200
Wire Wire Line
	2550 1200 2550 1400
Wire Wire Line
	2000 1550 2150 1550
Wire Wire Line
	2150 1550 2150 1400
Wire Wire Line
	2150 1400 3250 1400
Wire Wire Line
	3250 1400 3250 1500
Connection ~ 2550 1400
Wire Wire Line
	2550 1700 2550 1850
Wire Wire Line
	3850 1500 4400 1500
Wire Wire Line
	4400 1500 4400 1150
Wire Wire Line
	3850 1500 3850 1600
Wire Wire Line
	3850 1600 3950 1600
Wire Wire Line
	3250 1500 3000 1500
Wire Wire Line
	3000 1500 3000 1650
Wire Wire Line
	3000 1950 3000 2250
Wire Wire Line
	3000 2250 3950 2250
Wire Wire Line
	3550 1800 3550 2250
Wire Wire Line
	3950 2250 3950 1900
Connection ~ 3550 2250
Wire Wire Line
	2000 1750 2000 2150
Wire Wire Line
	2000 2150 3550 2150
Connection ~ 2550 2150
Text GLabel 8000 1600 0    60   Input ~ 0
A5
Text GLabel 8000 1700 0    60   Input ~ 0
A4
Text GLabel 8000 1800 0    60   Input ~ 0
A3
Text GLabel 8000 1900 0    60   Input ~ 0
A2
Text GLabel 8000 2000 0    60   Input ~ 0
A1
Text GLabel 8000 2100 0    60   Input ~ 0
A0
Text GLabel 2600 2650 0    60   Input ~ 0
P0
Text GLabel 2600 2750 0    60   Input ~ 0
P1
Text GLabel 2600 2850 0    60   Input ~ 0
P2
Text GLabel 2600 2950 0    60   Input ~ 0
P3
Text GLabel 2600 3050 0    60   Input ~ 0
P4
Text GLabel 2600 3150 0    60   Input ~ 0
P5
Text GLabel 2600 3250 0    60   Input ~ 0
P6
Text GLabel 2600 3350 0    60   Input ~ 0
P7
Text GLabel 2600 3700 0    60   Input ~ 0
P8
Text GLabel 2600 3800 0    60   Input ~ 0
P9
Text GLabel 2600 3900 0    60   Input ~ 0
P10
Text GLabel 2600 4000 0    60   Input ~ 0
P11
Text GLabel 2600 4100 0    60   Input ~ 0
P12
Text GLabel 2600 4200 0    60   Input ~ 0
P13
Text GLabel 2600 4300 0    60   Input ~ 0
P14
Text GLabel 2600 4400 0    60   Input ~ 0
P15
Text GLabel 7350 3150 0    60   Input ~ 0
VIN
Text GLabel 7350 3250 0    60   Input ~ 0
GND
Text GLabel 7350 3350 0    60   Input ~ 0
GND
Text GLabel 7350 3450 0    60   Input ~ 0
5V
Text GLabel 7350 3550 0    60   Input ~ 0
3.3V
Text GLabel 7350 3650 0    60   Input ~ 0
RESET
Wire Wire Line
	2550 2150 2550 2000
$Comp
L GND #PWR04
U 1 1 5A16B85B
P 5550 4200
F 0 "#PWR04" H 5550 3950 50  0001 C CNN
F 1 "GND" H 5550 4050 50  0000 C CNN
F 2 "" H 5550 4200 50  0001 C CNN
F 3 "" H 5550 4200 50  0001 C CNN
	1    5550 4200
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR05
U 1 1 5A16B893
P 5650 2600
F 0 "#PWR05" H 5650 2350 50  0001 C CNN
F 1 "GND" H 5650 2450 50  0000 C CNN
F 2 "" H 5650 2600 50  0001 C CNN
F 3 "" H 5650 2600 50  0001 C CNN
	1    5650 2600
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR06
U 1 1 5A16B8CB
P 6900 2250
F 0 "#PWR06" H 6900 2100 50  0001 C CNN
F 1 "+5V" H 6900 2390 50  0000 C CNN
F 2 "" H 6900 2250 50  0001 C CNN
F 3 "" H 6900 2250 50  0001 C CNN
	1    6900 2250
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR07
U 1 1 5A16B90F
P 6900 4100
F 0 "#PWR07" H 6900 3950 50  0001 C CNN
F 1 "+5V" H 6900 4240 50  0000 C CNN
F 2 "" H 6900 4100 50  0001 C CNN
F 3 "" H 6900 4100 50  0001 C CNN
	1    6900 4100
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 2650 2900 2650
Wire Wire Line
	2900 2750 2600 2750
Wire Wire Line
	2600 2850 2900 2850
Wire Wire Line
	2900 2950 2600 2950
Wire Wire Line
	2600 3050 2900 3050
Wire Wire Line
	2900 3150 2600 3150
Wire Wire Line
	2600 3700 2900 3700
Wire Wire Line
	2900 3800 2600 3800
Wire Wire Line
	2600 3900 2900 3900
Wire Wire Line
	2900 4000 2600 4000
Wire Wire Line
	2600 4100 2900 4100
Wire Wire Line
	2900 4200 2600 4200
Wire Wire Line
	2600 4300 2900 4300
Wire Wire Line
	2900 4400 2600 4400
Wire Wire Line
	2600 3350 2900 3350
Wire Wire Line
	2900 3250 2600 3250
Wire Wire Line
	7350 3150 7500 3150
Wire Wire Line
	7500 3250 7350 3250
Wire Wire Line
	7350 3550 7500 3550
Wire Wire Line
	7500 3650 7350 3650
Wire Wire Line
	8000 1600 8250 1600
Wire Wire Line
	8250 1700 8000 1700
Wire Wire Line
	8000 1800 8250 1800
Wire Wire Line
	8250 1900 8000 1900
Wire Wire Line
	8000 2000 8250 2000
Wire Wire Line
	8250 2100 8000 2100
Wire Wire Line
	6600 2400 6900 2400
Wire Wire Line
	6900 2400 6900 2250
Wire Wire Line
	5650 2600 5900 2600
Wire Wire Line
	5900 2600 5900 2400
Wire Wire Line
	6550 3900 6550 4100
Wire Wire Line
	6150 4100 6900 4100
Wire Wire Line
	6150 3900 6150 4100
Wire Wire Line
	6250 3900 6250 4200
Wire Wire Line
	6250 4200 5550 4200
Wire Wire Line
	5850 3900 5550 3900
Wire Wire Line
	5550 3900 5550 4200
Text GLabel 5950 4000 3    60   Input ~ 0
P2
Text GLabel 6050 4000 3    60   Input ~ 0
P10
Text GLabel 6350 4000 3    60   Input ~ 0
P3
Text GLabel 6450 4000 3    60   Input ~ 0
P11
Wire Wire Line
	6450 3900 6450 4000
Wire Wire Line
	6350 3900 6350 4000
Wire Wire Line
	6050 3900 6050 4000
Wire Wire Line
	5950 3900 5950 4000
Text GLabel 2350 1700 0    60   Input ~ 0
A2
Wire Wire Line
	2350 1700 2550 1700
$Comp
L RJ45 J8
U 1 1 5A1684AA
P 4650 2850
F 0 "J8" H 4850 3350 50  0000 C CNN
F 1 "LIGHTS" H 4500 3350 50  0000 C CNN
F 2 "Connectors:RJ45_8" H 4650 2850 50  0001 C CNN
F 3 "" H 4650 2850 50  0001 C CNN
	1    4650 2850
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR08
U 1 1 5A16928A
P 4100 3650
F 0 "#PWR08" H 4100 3400 50  0001 C CNN
F 1 "GND" H 4100 3500 50  0000 C CNN
F 2 "" H 4100 3650 50  0001 C CNN
F 3 "" H 4100 3650 50  0001 C CNN
	1    4100 3650
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR09
U 1 1 5A1692C5
P 5250 3500
F 0 "#PWR09" H 5250 3350 50  0001 C CNN
F 1 "+5V" H 5250 3640 50  0000 C CNN
F 2 "" H 5250 3500 50  0001 C CNN
F 3 "" H 5250 3500 50  0001 C CNN
	1    5250 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	4300 3300 4100 3300
Wire Wire Line
	4100 3300 4100 3650
Wire Wire Line
	4500 3300 4500 3500
Wire Wire Line
	4500 3500 5250 3500
NoConn ~ 5000 3300
NoConn ~ 4600 3300
Wire Wire Line
	4900 3300 4900 3500
Wire Wire Line
	4700 3300 4700 3650
Wire Wire Line
	4700 3650 4100 3650
Text GLabel 4400 3400 3    60   Input ~ 0
P5
Text GLabel 4800 3650 3    60   Input ~ 0
P6
Wire Wire Line
	4800 3300 4800 3650
Wire Wire Line
	4400 3300 4400 3400
Text GLabel 6000 2550 3    60   Input ~ 0
P8
Text GLabel 6100 2550 3    60   Input ~ 0
P9
Text GLabel 6500 2550 3    60   Input ~ 0
A4
Text GLabel 6400 2550 3    60   Input ~ 0
A5
Text GLabel 6300 2550 3    60   Input ~ 0
A0
Wire Wire Line
	6500 2400 6500 2550
Wire Wire Line
	6400 2400 6400 2550
Wire Wire Line
	6300 2400 6300 2550
Wire Wire Line
	6100 2400 6100 2550
Wire Wire Line
	6000 2400 6000 2550
$Comp
L GND #PWR010
U 1 1 5A16B23A
P 8800 3600
F 0 "#PWR010" H 8800 3350 50  0001 C CNN
F 1 "GND" H 8800 3450 50  0000 C CNN
F 2 "" H 8800 3600 50  0001 C CNN
F 3 "" H 8800 3600 50  0001 C CNN
	1    8800 3600
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR011
U 1 1 5A16B268
P 8000 3450
F 0 "#PWR011" H 8000 3300 50  0001 C CNN
F 1 "+5V" H 8000 3590 50  0000 C CNN
F 2 "" H 8000 3450 50  0001 C CNN
F 3 "" H 8000 3450 50  0001 C CNN
	1    8000 3450
	1    0    0    -1  
$EndComp
Connection ~ 7500 3450
Wire Wire Line
	8800 2800 8800 3600
Connection ~ 7500 3350
Wire Wire Line
	3550 2250 2000 2250
Wire Wire Line
	2000 2250 2000 1650
NoConn ~ 6200 2400
$Comp
L GND #PWR012
U 1 1 5A16C0FC
P 4200 2000
F 0 "#PWR012" H 4200 1750 50  0001 C CNN
F 1 "GND" H 4200 1850 50  0000 C CNN
F 2 "" H 4200 2000 50  0001 C CNN
F 3 "" H 4200 2000 50  0001 C CNN
	1    4200 2000
	1    0    0    -1  
$EndComp
Wire Wire Line
	3950 1900 4200 1900
Wire Wire Line
	4200 1900 4200 2000
Wire Wire Line
	7350 3350 7500 3350
$EndSCHEMATC
