EESchema Schematic File Version 2
LIBS:power
LIBS:device
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
LIBS:arduino
LIBS:rur
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
L Arduino_Uno_Shield ARD1
U 1 1 5A288319
P 3200 4350
F 0 "ARD1" V 3300 4350 60  0000 C CNN
F 1 "UNO_SHIELD" V 3100 4350 60  0000 C CNN
F 2 "Arduino:Arduino_Uno_Shield" H 5000 8100 60  0001 C CNN
F 3 "" H 5000 8100 60  0001 C CNN
	1    3200 4350
	1    0    0    -1  
$EndComp
$Comp
L DFPLAYER_MINI U1
U 1 1 5A288A03
P 3650 2150
F 0 "U1" H 3650 1900 60  0000 C CNN
F 1 "DFPLAYER_MINI" H 3650 2600 60  0000 C CNN
F 2 "Arduino:dfplayer_mini" H 3650 2150 60  0001 C CNN
F 3 "" H 3650 2150 60  0000 C CNN
	1    3650 2150
	1    0    0    -1  
$EndComp
$Comp
L USB_A P1
U 1 1 5A288AA4
P 6350 1700
F 0 "P1" H 6550 1500 50  0000 C CNN
F 1 "TMR1_LED" H 6300 1900 50  0000 C CNN
F 2 "Connect:USB_A" V 6300 1600 50  0001 C CNN
F 3 "" V 6300 1600 50  0000 C CNN
	1    6350 1700
	1    0    0    -1  
$EndComp
$Comp
L USB_A P2
U 1 1 5A288B61
P 7450 1750
F 0 "P2" H 7650 1550 50  0000 C CNN
F 1 "TMR2_LED" H 7400 1950 50  0000 C CNN
F 2 "Connect:USB_A" V 7400 1650 50  0001 C CNN
F 3 "" V 7400 1650 50  0000 C CNN
	1    7450 1750
	1    0    0    -1  
$EndComp
$Comp
L USB_A P3
U 1 1 5A288BC4
P 5500 1700
F 0 "P3" H 5700 1500 50  0000 C CNN
F 1 "CAP_LED" H 5450 1900 50  0000 C CNN
F 2 "Connect:USB_A" V 5450 1600 50  0001 C CNN
F 3 "" V 5450 1600 50  0000 C CNN
	1    5500 1700
	1    0    0    -1  
$EndComp
$Comp
L USB_A P4
U 1 1 5A288C1F
P 8300 1750
F 0 "P4" H 8500 1550 50  0000 C CNN
F 1 "OWN_LED" H 8250 1950 50  0000 C CNN
F 2 "Connect:USB_A" V 8250 1650 50  0001 C CNN
F 3 "" V 8250 1650 50  0000 C CNN
	1    8300 1750
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X02 P5
U 1 1 5A288E13
P 1800 2450
F 0 "P5" H 1800 2600 50  0000 C CNN
F 1 "SPK" V 1900 2450 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B02B-XH-A_02x2.50mm_Straight" H 1800 2450 50  0001 C CNN
F 3 "" H 1800 2450 50  0000 C CNN
	1    1800 2450
	-1   0    0    1   
$EndComp
$Comp
L RJ45 J1
U 1 1 5A288E8E
P 1150 1100
F 0 "J1" H 1350 1600 50  0000 C CNN
F 1 "BTNS" H 1000 1600 50  0000 C CNN
F 2 "Connect:RJ45_8" H 1150 1100 50  0001 C CNN
F 3 "" H 1150 1100 50  0000 C CNN
	1    1150 1100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 5A28A349
P 5800 5000
F 0 "#PWR01" H 5800 4750 50  0001 C CNN
F 1 "GND" H 5800 4850 50  0000 C CNN
F 2 "" H 5800 5000 50  0000 C CNN
F 3 "" H 5800 5000 50  0000 C CNN
	1    5800 5000
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR02
U 1 1 5A28A37E
P 5600 2250
F 0 "#PWR02" H 5600 2000 50  0001 C CNN
F 1 "GND" H 5600 2100 50  0000 C CNN
F 2 "" H 5600 2250 50  0000 C CNN
F 3 "" H 5600 2250 50  0000 C CNN
	1    5600 2250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 5A28A3B3
P 6450 2250
F 0 "#PWR03" H 6450 2000 50  0001 C CNN
F 1 "GND" H 6450 2100 50  0000 C CNN
F 2 "" H 6450 2250 50  0000 C CNN
F 3 "" H 6450 2250 50  0000 C CNN
	1    6450 2250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR04
U 1 1 5A28A45E
P 1600 4950
F 0 "#PWR04" H 1600 4700 50  0001 C CNN
F 1 "GND" H 1600 4800 50  0000 C CNN
F 2 "" H 1600 4950 50  0000 C CNN
F 3 "" H 1600 4950 50  0000 C CNN
	1    1600 4950
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR05
U 1 1 5A28A493
P 1000 5150
F 0 "#PWR05" H 1000 5000 50  0001 C CNN
F 1 "+5V" H 1000 5290 50  0000 C CNN
F 2 "" H 1000 5150 50  0000 C CNN
F 3 "" H 1000 5150 50  0000 C CNN
	1    1000 5150
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR06
U 1 1 5A28A4C8
P 900 2100
F 0 "#PWR06" H 900 1950 50  0001 C CNN
F 1 "+5V" H 900 2240 50  0000 C CNN
F 2 "" H 900 2100 50  0000 C CNN
F 3 "" H 900 2100 50  0000 C CNN
	1    900  2100
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR07
U 1 1 5A28A4FD
P 800 1950
F 0 "#PWR07" H 800 1700 50  0001 C CNN
F 1 "GND" H 800 1800 50  0000 C CNN
F 2 "" H 800 1950 50  0000 C CNN
F 3 "" H 800 1950 50  0000 C CNN
	1    800  1950
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR08
U 1 1 5A28A532
P 2750 2450
F 0 "#PWR08" H 2750 2200 50  0001 C CNN
F 1 "GND" H 2750 2300 50  0000 C CNN
F 2 "" H 2750 2450 50  0000 C CNN
F 3 "" H 2750 2450 50  0000 C CNN
	1    2750 2450
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR09
U 1 1 5A28A567
P 2700 1550
F 0 "#PWR09" H 2700 1400 50  0001 C CNN
F 1 "+5V" H 2700 1690 50  0000 C CNN
F 2 "" H 2700 1550 50  0000 C CNN
F 3 "" H 2700 1550 50  0000 C CNN
	1    2700 1550
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR010
U 1 1 5A28A59C
P 5300 2200
F 0 "#PWR010" H 5300 2050 50  0001 C CNN
F 1 "+5V" H 5300 2340 50  0000 C CNN
F 2 "" H 5300 2200 50  0000 C CNN
F 3 "" H 5300 2200 50  0000 C CNN
	1    5300 2200
	-1   0    0    1   
$EndComp
Text GLabel 1600 4500 0    60   Input ~ 0
A0
Text GLabel 4750 3500 2    60   Input ~ 0
D4
Text GLabel 4750 3600 2    60   Input ~ 0
D5
Text GLabel 4750 3700 2    60   Input ~ 0
D6
Text GLabel 4750 3800 2    60   Input ~ 0
D7
Text GLabel 4750 3900 2    60   Input ~ 0
D8
Text GLabel 4750 4000 2    60   Input ~ 0
D9
Text GLabel 4750 4100 2    60   Input ~ 0
D10
Text GLabel 4750 4200 2    60   Input ~ 0
D11
Text GLabel 4750 4300 2    60   Input ~ 0
D12
Text GLabel 4750 3400 2    60   Input ~ 0
D3
Text GLabel 4750 3300 2    60   Input ~ 0
D2
Text GLabel 4750 4400 2    60   Input ~ 0
D13
$Comp
L GND #PWR012
U 1 1 5A28ACCE
P 7550 2250
F 0 "#PWR012" H 7550 2000 50  0001 C CNN
F 1 "GND" H 7550 2100 50  0000 C CNN
F 2 "" H 7550 2250 50  0000 C CNN
F 3 "" H 7550 2250 50  0000 C CNN
	1    7550 2250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR013
U 1 1 5A28AD03
P 8400 2250
F 0 "#PWR013" H 8400 2000 50  0001 C CNN
F 1 "GND" H 8400 2100 50  0000 C CNN
F 2 "" H 8400 2250 50  0000 C CNN
F 3 "" H 8400 2250 50  0000 C CNN
	1    8400 2250
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR014
U 1 1 5A28AE34
P 6150 2250
F 0 "#PWR014" H 6150 2100 50  0001 C CNN
F 1 "+5V" H 6150 2390 50  0000 C CNN
F 2 "" H 6150 2250 50  0000 C CNN
F 3 "" H 6150 2250 50  0000 C CNN
	1    6150 2250
	-1   0    0    1   
$EndComp
$Comp
L +5V #PWR015
U 1 1 5A28AE69
P 7250 2250
F 0 "#PWR015" H 7250 2100 50  0001 C CNN
F 1 "+5V" H 7250 2390 50  0000 C CNN
F 2 "" H 7250 2250 50  0000 C CNN
F 3 "" H 7250 2250 50  0000 C CNN
	1    7250 2250
	-1   0    0    1   
$EndComp
$Comp
L +5V #PWR016
U 1 1 5A28AE9E
P 8100 2250
F 0 "#PWR016" H 8100 2100 50  0001 C CNN
F 1 "+5V" H 8100 2390 50  0000 C CNN
F 2 "" H 8100 2250 50  0000 C CNN
F 3 "" H 8100 2250 50  0000 C CNN
	1    8100 2250
	-1   0    0    1   
$EndComp
NoConn ~ 4250 1800
NoConn ~ 4250 1900
NoConn ~ 4250 2000
NoConn ~ 4250 2100
NoConn ~ 4250 2200
NoConn ~ 4250 2300
NoConn ~ 4250 2400
NoConn ~ 4250 2500
Text GLabel 1000 1650 3    60   Input ~ 0
D2
Text GLabel 1400 1650 3    60   Input ~ 0
D3
Text GLabel 5800 4700 0    60   Input ~ 0
A0
Text GLabel 5400 2100 3    60   Input ~ 0
D5
Text GLabel 1500 1650 3    60   Input ~ 0
D10
Text GLabel 1100 1650 3    60   Input ~ 0
D11
Text GLabel 2850 1900 0    60   Input ~ 0
D8
Text GLabel 2850 2000 0    60   Input ~ 0
D9
NoConn ~ 3050 2100
NoConn ~ 3050 2200
$Comp
L CONN_01X04 P8
U 1 1 5A28E955
P 2200 1950
F 0 "P8" H 2200 2200 50  0000 C CNN
F 1 "AUDIO_JACK" V 2300 1950 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 2200 1950 50  0001 C CNN
F 3 "" H 2200 1950 50  0000 C CNN
	1    2200 1950
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR019
U 1 1 5A28F0DC
P 1200 1950
F 0 "#PWR019" H 1200 1700 50  0001 C CNN
F 1 "GND" H 1200 1800 50  0000 C CNN
F 2 "" H 1200 1950 50  0000 C CNN
F 3 "" H 1200 1950 50  0000 C CNN
	1    1200 1950
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR020
U 1 1 5A28F29D
P 1300 2100
F 0 "#PWR020" H 1300 1950 50  0001 C CNN
F 1 "+5V" H 1300 2240 50  0000 C CNN
F 2 "" H 1300 2100 50  0000 C CNN
F 3 "" H 1300 2100 50  0000 C CNN
	1    1300 2100
	-1   0    0    1   
$EndComp
$Comp
L R R1
U 1 1 5A28FE68
P 3150 6100
F 0 "R1" V 3230 6100 50  0000 C CNN
F 1 "5k" V 3150 6100 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 3080 6100 50  0001 C CNN
F 3 "" H 3150 6100 50  0000 C CNN
	1    3150 6100
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5A28FED5
P 3150 6550
F 0 "R2" H 3230 6550 50  0000 C CNN
F 1 "5k" V 3150 6550 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 3080 6550 50  0001 C CNN
F 3 "" H 3150 6550 50  0000 C CNN
	1    3150 6550
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR021
U 1 1 5A28FF9B
P 3150 6900
F 0 "#PWR021" H 3150 6650 50  0001 C CNN
F 1 "GND" H 3150 6750 50  0000 C CNN
F 2 "" H 3150 6900 50  0000 C CNN
F 3 "" H 3150 6900 50  0000 C CNN
	1    3150 6900
	1    0    0    -1  
$EndComp
Text GLabel 2900 6300 0    60   Input ~ 0
A4
Text GLabel 1600 3600 0    60   Input ~ 0
SCL
Text GLabel 1600 3700 0    60   Input ~ 0
SDA
$Comp
L +5V #PWR022
U 1 1 5A2921E1
P 5750 4150
F 0 "#PWR022" H 5750 4000 50  0001 C CNN
F 1 "+5V" H 5750 4290 50  0000 C CNN
F 2 "" H 5750 4150 50  0000 C CNN
F 3 "" H 5750 4150 50  0000 C CNN
	1    5750 4150
	1    0    0    -1  
$EndComp
Text GLabel 5800 4500 0    60   Input ~ 0
SCL
Text GLabel 5800 4600 0    60   Input ~ 0
SDA
$Comp
L GND #PWR023
U 1 1 5A29A259
P 10450 5400
F 0 "#PWR023" H 10450 5150 50  0001 C CNN
F 1 "GND" H 10450 5250 50  0000 C CNN
F 2 "" H 10450 5400 50  0000 C CNN
F 3 "" H 10450 5400 50  0000 C CNN
	1    10450 5400
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR024
U 1 1 5A29A294
P 9100 2400
F 0 "#PWR024" H 9100 2250 50  0001 C CNN
F 1 "+5V" H 9100 2540 50  0000 C CNN
F 2 "" H 9100 2400 50  0000 C CNN
F 3 "" H 9100 2400 50  0000 C CNN
	1    9100 2400
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW1
U 1 1 5A29A2CF
P 10150 3450
F 0 "SW1" H 10300 3560 50  0000 C CNN
F 1 "BTN_UP" H 10150 3370 50  0000 C CNN
F 2 "Buttons_Switches_ThroughHole:SW_PUSH_6mm" H 10150 3450 50  0001 C CNN
F 3 "" H 10150 3450 50  0000 C CNN
	1    10150 3450
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW2
U 1 1 5A29A3B2
P 10150 4250
F 0 "SW2" H 10300 4360 50  0000 C CNN
F 1 "BTN_DWN" H 10150 4170 50  0000 C CNN
F 2 "Buttons_Switches_ThroughHole:SW_PUSH_6mm" H 10150 4250 50  0001 C CNN
F 3 "" H 10150 4250 50  0000 C CNN
	1    10150 4250
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW3
U 1 1 5A29A3FA
P 10150 5000
F 0 "SW3" H 10300 5110 50  0000 C CNN
F 1 "BTN_SEL" H 10150 4920 50  0000 C CNN
F 2 "Buttons_Switches_ThroughHole:SW_PUSH_6mm" H 10150 5000 50  0001 C CNN
F 3 "" H 10150 5000 50  0000 C CNN
	1    10150 5000
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 5A29AE03
P 9100 3200
F 0 "R3" V 9180 3200 50  0000 C CNN
F 1 "2K" V 9100 3200 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 9030 3200 50  0001 C CNN
F 3 "" H 9100 3200 50  0000 C CNN
	1    9100 3200
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 5A29AEF5
P 9100 3700
F 0 "R4" V 9180 3700 50  0000 C CNN
F 1 "330" V 9100 3700 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 9030 3700 50  0001 C CNN
F 3 "" H 9100 3700 50  0000 C CNN
	1    9100 3700
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 5A29AF42
P 9100 4200
F 0 "R5" V 9180 4200 50  0000 C CNN
F 1 "620" V 9100 4200 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 9030 4200 50  0001 C CNN
F 3 "" H 9100 4200 50  0000 C CNN
	1    9100 4200
	1    0    0    -1  
$EndComp
$Comp
L R R6
U 1 1 5A29AFA4
P 9100 4650
F 0 "R6" V 9180 4650 50  0000 C CNN
F 1 "1K" V 9100 4650 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 9030 4650 50  0001 C CNN
F 3 "" H 9100 4650 50  0000 C CNN
	1    9100 4650
	1    0    0    -1  
$EndComp
$Comp
L R R7
U 1 1 5A29BC9C
P 9100 5050
F 0 "R7" V 9180 5050 50  0000 C CNN
F 1 "3.3K" V 9100 5050 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 9030 5050 50  0001 C CNN
F 3 "" H 9100 5050 50  0000 C CNN
	1    9100 5050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR025
U 1 1 5A29CEBE
P 9400 2800
F 0 "#PWR025" H 9400 2550 50  0001 C CNN
F 1 "GND" H 9400 2650 50  0000 C CNN
F 2 "" H 9400 2800 50  0000 C CNN
F 3 "" H 9400 2800 50  0000 C CNN
	1    9400 2800
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X05 P12
U 1 1 5A29E776
P 6450 4600
F 0 "P12" H 6450 4900 50  0000 C CNN
F 1 "PANEL_OUT" V 6550 4600 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B05B-XH-A_05x2.50mm_Straight" H 6450 4600 50  0001 C CNN
F 3 "" H 6450 4600 50  0000 C CNN
	1    6450 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	2700 1550 2700 1800
Wire Wire Line
	2700 1800 3050 1800
Wire Wire Line
	3050 2400 2750 2400
Wire Wire Line
	2750 2400 2750 2450
Wire Wire Line
	1900 5300 1000 5300
Wire Wire Line
	1000 5300 1000 5150
Wire Wire Line
	1900 4900 1600 4900
Wire Wire Line
	1600 4900 1600 4950
Wire Wire Line
	1900 4500 1600 4500
Wire Wire Line
	4750 3300 4500 3300
Wire Wire Line
	4500 3400 4750 3400
Wire Wire Line
	4750 3500 4500 3500
Wire Wire Line
	4500 3600 4750 3600
Wire Wire Line
	4750 3700 4500 3700
Wire Wire Line
	4500 3800 4750 3800
Wire Wire Line
	4750 3900 4500 3900
Wire Wire Line
	4500 4000 4750 4000
Wire Wire Line
	4750 4100 4500 4100
Wire Wire Line
	4500 4200 4750 4200
Wire Wire Line
	4750 4300 4500 4300
Wire Wire Line
	4500 4400 4750 4400
Wire Wire Line
	5600 2000 5600 2250
Wire Wire Line
	6450 2000 6450 2250
Wire Wire Line
	7550 2050 7550 2250
Wire Wire Line
	8400 2050 8400 2250
Wire Wire Line
	6150 2000 6150 2250
Wire Wire Line
	5300 2000 5300 2200
Wire Wire Line
	7250 2050 7250 2250
Wire Wire Line
	8100 2050 8100 2250
Wire Wire Line
	5400 2000 5400 2100
Wire Wire Line
	5500 2000 5500 2050
Wire Wire Line
	5500 2050 6250 2050
Wire Wire Line
	6250 2050 6250 2000
Wire Wire Line
	6350 2000 6350 2050
Wire Wire Line
	6350 2050 7350 2050
Wire Wire Line
	7450 2050 7450 2100
Wire Wire Line
	7450 2100 8200 2100
Wire Wire Line
	8200 2100 8200 2050
Wire Wire Line
	2850 1900 3050 1900
Wire Wire Line
	2850 2000 3050 2000
Wire Wire Line
	3050 2500 3050 2700
Wire Wire Line
	3050 2700 2000 2700
Wire Wire Line
	2000 2700 2000 2500
Wire Wire Line
	2000 2400 2000 2150
Wire Wire Line
	2000 2150 2050 2150
Wire Wire Line
	2350 2300 3050 2300
Wire Wire Line
	2600 2300 2600 2500
Wire Wire Line
	2600 2500 2150 2500
Wire Wire Line
	2150 2500 2150 2150
Wire Wire Line
	2350 2300 2350 2150
Wire Wire Line
	2000 2500 2100 2500
Wire Wire Line
	2100 2500 2100 2600
Wire Wire Line
	2100 2600 2250 2600
Wire Wire Line
	2250 2600 2250 2150
Wire Wire Line
	800  1550 800  1950
Wire Wire Line
	900  1550 900  2100
Wire Wire Line
	1000 1550 1000 1650
Wire Wire Line
	1100 1550 1100 1650
Wire Wire Line
	1200 1550 1200 1950
Wire Wire Line
	1300 1550 1300 2100
Wire Wire Line
	1400 1550 1400 1650
Wire Wire Line
	1500 1550 1500 1650
Wire Wire Line
	3150 6250 3150 6400
Wire Wire Line
	3150 6700 3150 6900
Wire Wire Line
	1900 5900 3150 5900
Wire Wire Line
	3150 5900 3150 5950
Wire Wire Line
	3150 6250 2950 6250
Wire Wire Line
	2950 6250 2950 6300
Wire Wire Line
	2950 6300 2900 6300
Wire Wire Line
	1900 5900 1900 5400
Wire Wire Line
	10450 3450 10450 5400
Connection ~ 10450 4250
Connection ~ 10450 5000
Wire Wire Line
	9100 2400 9100 3050
Wire Wire Line
	9100 3350 9100 3550
Wire Wire Line
	9100 3850 9100 4050
Wire Wire Line
	9100 4350 9100 4500
Wire Wire Line
	9100 4800 9100 4900
Wire Wire Line
	9100 5200 9700 5200
Wire Wire Line
	9700 5200 9700 5000
Wire Wire Line
	9700 5000 9850 5000
Wire Wire Line
	9100 3850 9850 3850
Wire Wire Line
	9850 3850 9850 3450
Wire Wire Line
	9100 4350 9850 4350
Wire Wire Line
	9850 4350 9850 4250
Connection ~ 9100 2400
Wire Wire Line
	5800 5000 5800 4800
Wire Wire Line
	5800 4800 6250 4800
Wire Wire Line
	5750 4150 5750 4400
Wire Wire Line
	5750 4400 6250 4400
Wire Wire Line
	5800 4500 6250 4500
Wire Wire Line
	5800 4600 6250 4600
Wire Wire Line
	5800 4700 6250 4700
$Comp
L CONN_01X05 P13
U 1 1 5A2A2966
P 9750 2600
F 0 "P13" H 9750 2900 50  0000 C CNN
F 1 "PANEL_IN" V 9850 2600 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B05B-XH-A_05x2.50mm_Straight" H 9750 2600 50  0001 C CNN
F 3 "" H 9750 2600 50  0000 C CNN
	1    9750 2600
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P14
U 1 1 5A2A2AC0
P 10600 2650
F 0 "P14" H 10600 2900 50  0000 C CNN
F 1 "OLED_OUT" V 10700 2650 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 10600 2650 50  0001 C CNN
F 3 "" H 10600 2650 50  0000 C CNN
	1    10600 2650
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR026
U 1 1 5A2A2D40
P 10350 2150
F 0 "#PWR026" H 10350 2000 50  0001 C CNN
F 1 "+5V" H 10350 2290 50  0000 C CNN
F 2 "" H 10350 2150 50  0000 C CNN
F 3 "" H 10350 2150 50  0000 C CNN
	1    10350 2150
	1    0    0    -1  
$EndComp
Wire Wire Line
	9550 2800 9400 2800
Wire Wire Line
	9100 3350 9250 3350
Wire Wire Line
	9250 3350 9250 2500
Wire Wire Line
	9250 2500 9550 2500
Wire Wire Line
	10350 2150 10350 2500
Wire Wire Line
	10350 2500 10400 2500
$Comp
L GND #PWR027
U 1 1 5A2A41C0
P 10300 2950
F 0 "#PWR027" H 10300 2700 50  0001 C CNN
F 1 "GND" H 10300 2800 50  0000 C CNN
F 2 "" H 10300 2950 50  0000 C CNN
F 3 "" H 10300 2950 50  0000 C CNN
	1    10300 2950
	1    0    0    -1  
$EndComp
Wire Wire Line
	10400 2800 10300 2800
Wire Wire Line
	10300 2800 10300 2950
Wire Wire Line
	9550 2700 10400 2700
Wire Wire Line
	9550 2600 10400 2600
Wire Wire Line
	9100 2400 9550 2400
$EndSCHEMATC