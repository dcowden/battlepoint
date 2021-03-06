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
L CONN_01X02 P5
U 1 1 5A288E13
P 2300 2150
F 0 "P5" H 2300 2300 50  0000 C CNN
F 1 "SPK" V 2400 2150 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B02B-XH-A_02x2.50mm_Straight" H 2300 2150 50  0001 C CNN
F 3 "" H 2300 2150 50  0000 C CNN
	1    2300 2150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 5A28A45E
P 1600 4950
F 0 "#PWR01" H 1600 4700 50  0001 C CNN
F 1 "GND" H 1600 4800 50  0000 C CNN
F 2 "" H 1600 4950 50  0000 C CNN
F 3 "" H 1600 4950 50  0000 C CNN
	1    1600 4950
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR02
U 1 1 5A28A493
P 1000 5150
F 0 "#PWR02" H 1000 5000 50  0001 C CNN
F 1 "+5V" H 1000 5290 50  0000 C CNN
F 2 "" H 1000 5150 50  0000 C CNN
F 3 "" H 1000 5150 50  0000 C CNN
	1    1000 5150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 5A28A532
P 2400 2650
F 0 "#PWR03" H 2400 2400 50  0001 C CNN
F 1 "GND" H 2400 2500 50  0000 C CNN
F 2 "" H 2400 2650 50  0000 C CNN
F 3 "" H 2400 2650 50  0000 C CNN
	1    2400 2650
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR04
U 1 1 5A28A567
P 2700 1550
F 0 "#PWR04" H 2700 1400 50  0001 C CNN
F 1 "+5V" H 2700 1690 50  0000 C CNN
F 2 "" H 2700 1550 50  0000 C CNN
F 3 "" H 2700 1550 50  0000 C CNN
	1    2700 1550
	1    0    0    -1  
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
NoConn ~ 4250 1800
NoConn ~ 4250 1900
NoConn ~ 4250 2000
NoConn ~ 4250 2100
NoConn ~ 4250 2200
NoConn ~ 4250 2300
NoConn ~ 4250 2400
NoConn ~ 4250 2500
Text GLabel 8250 3300 0    60   Input ~ 0
A0
Text GLabel 4500 1100 2    60   Input ~ 0
D5
Text GLabel 2650 1900 0    60   Input ~ 0
D8
Text GLabel 2650 2000 0    60   Input ~ 0
D9
NoConn ~ 3050 2100
NoConn ~ 3050 2200
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
L GND #PWR05
U 1 1 5A28FF9B
P 3150 6900
F 0 "#PWR05" H 3150 6650 50  0001 C CNN
F 1 "GND" H 3150 6750 50  0000 C CNN
F 2 "" H 3150 6900 50  0000 C CNN
F 3 "" H 3150 6900 50  0000 C CNN
	1    3150 6900
	1    0    0    -1  
$EndComp
Text GLabel 2900 6300 0    60   Input ~ 0
A2
Text GLabel 1600 3600 0    60   Input ~ 0
SCL
Text GLabel 1600 3700 0    60   Input ~ 0
SDA
Text GLabel 7200 3750 0    60   Input ~ 0
SCL
Text GLabel 7200 3850 0    60   Input ~ 0
SDA
$Comp
L GND #PWR06
U 1 1 5A29A259
P 10450 5400
F 0 "#PWR06" H 10450 5150 50  0001 C CNN
F 1 "GND" H 10450 5250 50  0000 C CNN
F 2 "" H 10450 5400 50  0000 C CNN
F 3 "" H 10450 5400 50  0000 C CNN
	1    10450 5400
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR07
U 1 1 5A29A294
P 8750 2250
F 0 "#PWR07" H 8750 2100 50  0001 C CNN
F 1 "+5V" H 8750 2390 50  0000 C CNN
F 2 "" H 8750 2250 50  0000 C CNN
F 3 "" H 8750 2250 50  0000 C CNN
	1    8750 2250
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
P 8750 3050
F 0 "R3" V 8830 3050 50  0000 C CNN
F 1 "2K" V 8750 3050 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Vertical_RM5mm" V 8680 3050 50  0001 C CNN
F 3 "" H 8750 3050 50  0000 C CNN
	1    8750 3050
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 5A29AEF5
P 8750 3550
F 0 "R4" V 8830 3550 50  0000 C CNN
F 1 "330" V 8750 3550 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Vertical_RM5mm" V 8680 3550 50  0001 C CNN
F 3 "" H 8750 3550 50  0000 C CNN
	1    8750 3550
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 5A29AF42
P 8750 4050
F 0 "R5" V 8830 4050 50  0000 C CNN
F 1 "620" V 8750 4050 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Vertical_RM5mm" V 8680 4050 50  0001 C CNN
F 3 "" H 8750 4050 50  0000 C CNN
	1    8750 4050
	1    0    0    -1  
$EndComp
$Comp
L R R6
U 1 1 5A29AFA4
P 8750 4500
F 0 "R6" V 8830 4500 50  0000 C CNN
F 1 "1K" V 8750 4500 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Vertical_RM5mm" V 8680 4500 50  0001 C CNN
F 3 "" H 8750 4500 50  0000 C CNN
	1    8750 4500
	1    0    0    -1  
$EndComp
$Comp
L R R7
U 1 1 5A29BC9C
P 8750 4900
F 0 "R7" V 8830 4900 50  0000 C CNN
F 1 "3.3K" V 8750 4900 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Vertical_RM5mm" V 8680 4900 50  0001 C CNN
F 3 "" H 8750 4900 50  0000 C CNN
	1    8750 4900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR08
U 1 1 5A29CEBE
P 8300 4700
F 0 "#PWR08" H 8300 4450 50  0001 C CNN
F 1 "GND" H 8300 4550 50  0000 C CNN
F 2 "" H 8300 4700 50  0000 C CNN
F 3 "" H 8300 4700 50  0000 C CNN
	1    8300 4700
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P14
U 1 1 5A2A2AC0
P 7550 3800
F 0 "P14" H 7550 4050 50  0000 C CNN
F 1 "OLED_OUT" V 7650 3800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 7550 3800 50  0001 C CNN
F 3 "" H 7550 3800 50  0000 C CNN
	1    7550 3800
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR09
U 1 1 5A2A2D40
P 7300 3300
F 0 "#PWR09" H 7300 3150 50  0001 C CNN
F 1 "+5V" H 7300 3440 50  0000 C CNN
F 2 "" H 7300 3300 50  0000 C CNN
F 3 "" H 7300 3300 50  0000 C CNN
	1    7300 3300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR010
U 1 1 5A2A41C0
P 7250 4100
F 0 "#PWR010" H 7250 3850 50  0001 C CNN
F 1 "GND" H 7250 3950 50  0000 C CNN
F 2 "" H 7250 4100 50  0000 C CNN
F 3 "" H 7250 4100 50  0000 C CNN
	1    7250 4100
	1    0    0    -1  
$EndComp
Text GLabel 1600 4100 0    60   Input ~ 0
A4
$Comp
L CONN_01X04 P13
U 1 1 5A2C71AF
P 9450 4150
F 0 "P13" H 9450 4400 50  0000 C CNN
F 1 "BTNS_IN" V 9550 4150 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x04" H 9450 4150 50  0001 C CNN
F 3 "" H 9450 4150 50  0000 C CNN
	1    9450 4150
	-1   0    0    1   
$EndComp
$Comp
L CONN_01X04 P17
U 1 1 5A2C776A
P 8100 3700
F 0 "P17" H 8100 3950 50  0000 C CNN
F 1 "BTNS_OUT" V 8200 3700 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 8100 3700 50  0001 C CNN
F 3 "" H 8100 3700 50  0000 C CNN
	1    8100 3700
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR011
U 1 1 5A2C78E7
P 9600 4450
F 0 "#PWR011" H 9600 4200 50  0001 C CNN
F 1 "GND" H 9600 4300 50  0000 C CNN
F 2 "" H 9600 4450 50  0000 C CNN
F 3 "" H 9600 4450 50  0000 C CNN
	1    9600 4450
	1    0    0    -1  
$EndComp
$Comp
L R R10
U 1 1 5A2EF399
P 2850 1900
F 0 "R10" V 2930 1900 50  0000 C CNN
F 1 "1K" V 2850 1900 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" V 2780 1900 50  0001 C CNN
F 3 "" H 2850 1900 50  0000 C CNN
	1    2850 1900
	0    1    1    0   
$EndComp
Text GLabel 1600 4300 0    60   Input ~ 0
A2
$Comp
L LM7805CT U4
U 1 1 5A3497DA
P 6150 4950
F 0 "U4" H 5950 5150 50  0000 C CNN
F 1 "L4940" H 6150 5150 50  0000 L CNN
F 2 "Power_Integrations:TO-220" H 6150 5050 50  0000 C CIN
F 3 "" H 6150 4950 50  0000 C CNN
	1    6150 4950
	1    0    0    -1  
$EndComp
$Comp
L C C5
U 1 1 5A3498C1
P 6900 5250
F 0 "C5" H 6925 5350 50  0000 L CNN
F 1 "22uf" H 6925 5150 50  0000 L CNN
F 2 "Capacitors_ThroughHole:C_Radial_D7.5_L11.2_P2.5" H 6938 5100 50  0001 C CNN
F 3 "" H 6900 5250 50  0000 C CNN
	1    6900 5250
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X02 P20
U 1 1 5A349E9A
P 5150 4950
F 0 "P20" H 5150 5100 50  0000 C CNN
F 1 "BATT" V 5250 4950 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B02B-XH-A_02x2.50mm_Straight" H 5150 4950 50  0001 C CNN
F 3 "" H 5150 4950 50  0000 C CNN
	1    5150 4950
	-1   0    0    1   
$EndComp
$Comp
L +5V #PWR012
U 1 1 5A349FFE
P 6450 4500
F 0 "#PWR012" H 6450 4350 50  0001 C CNN
F 1 "+5V" H 6450 4640 50  0000 C CNN
F 2 "" H 6450 4500 50  0000 C CNN
F 3 "" H 6450 4500 50  0000 C CNN
	1    6450 4500
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR013
U 1 1 5A34A05D
P 6150 5650
F 0 "#PWR013" H 6150 5400 50  0001 C CNN
F 1 "GND" H 6150 5500 50  0000 C CNN
F 2 "" H 6150 5650 50  0000 C CNN
F 3 "" H 6150 5650 50  0000 C CNN
	1    6150 5650
	1    0    0    -1  
$EndComp
$Comp
L C C7
U 1 1 5A3538CC
P 5700 5300
F 0 "C7" H 5725 5400 50  0000 L CNN
F 1 "0.1uf" H 5725 5200 50  0000 L CNN
F 2 "Capacitors_ThroughHole:C_Rect_L4_W2.5_P2.5" H 5738 5150 50  0001 C CNN
F 3 "" H 5700 5300 50  0000 C CNN
	1    5700 5300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR014
U 1 1 5A3547E8
P 5600 5500
F 0 "#PWR014" H 5600 5250 50  0001 C CNN
F 1 "GND" H 5600 5350 50  0000 C CNN
F 2 "" H 5600 5500 50  0000 C CNN
F 3 "" H 5600 5500 50  0000 C CNN
	1    5600 5500
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X02 P40
U 1 1 5A470255
P 5550 4350
F 0 "P40" H 5550 4500 50  0000 C CNN
F 1 "PWR_SWITCH" V 5650 4350 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B02B-XH-A_02x2.50mm_Straight" H 5550 4350 50  0001 C CNN
F 3 "" H 5550 4350 50  0000 C CNN
	1    5550 4350
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR015
U 1 1 5A4773C4
P 4450 1000
F 0 "#PWR015" H 4450 750 50  0001 C CNN
F 1 "GND" H 4450 850 50  0000 C CNN
F 2 "" H 4450 1000 50  0001 C CNN
F 3 "" H 4450 1000 50  0001 C CNN
	1    4450 1000
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR016
U 1 1 5A47741A
P 6950 1000
F 0 "#PWR016" H 6950 750 50  0001 C CNN
F 1 "GND" H 6950 850 50  0000 C CNN
F 2 "" H 6950 1000 50  0001 C CNN
F 3 "" H 6950 1000 50  0001 C CNN
	1    6950 1000
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR017
U 1 1 5A477470
P 6100 1000
F 0 "#PWR017" H 6100 750 50  0001 C CNN
F 1 "GND" H 6100 850 50  0000 C CNN
F 2 "" H 6100 1000 50  0001 C CNN
F 3 "" H 6100 1000 50  0001 C CNN
	1    6100 1000
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR018
U 1 1 5A4774C6
P 5350 1000
F 0 "#PWR018" H 5350 750 50  0001 C CNN
F 1 "GND" H 5350 850 50  0000 C CNN
F 2 "" H 5350 1000 50  0001 C CNN
F 3 "" H 5350 1000 50  0001 C CNN
	1    5350 1000
	0    -1   -1   0   
$EndComp
$Comp
L Conn_01x05 J1
U 1 1 5A485E51
P 1700 2300
F 0 "J1" H 1700 2600 50  0000 C CNN
F 1 "AUDIO" H 1700 2000 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B05B-XH-A_05x2.50mm_Straight" H 1700 2300 50  0001 C CNN
F 3 "" H 1700 2300 50  0001 C CNN
	1    1700 2300
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR019
U 1 1 5A4862A6
P 2900 2400
F 0 "#PWR019" H 2900 2150 50  0001 C CNN
F 1 "GND" H 2900 2250 50  0000 C CNN
F 2 "" H 2900 2400 50  0001 C CNN
F 3 "" H 2900 2400 50  0001 C CNN
	1    2900 2400
	0    1    1    0   
$EndComp
$Comp
L GND #PWR020
U 1 1 5A487DBF
P 1150 1400
F 0 "#PWR020" H 1150 1150 50  0001 C CNN
F 1 "GND" H 1150 1250 50  0000 C CNN
F 2 "" H 1150 1400 50  0001 C CNN
F 3 "" H 1150 1400 50  0001 C CNN
	1    1150 1400
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR021
U 1 1 5A487E15
P 2100 1400
F 0 "#PWR021" H 2100 1150 50  0001 C CNN
F 1 "GND" H 2100 1250 50  0000 C CNN
F 2 "" H 2100 1400 50  0001 C CNN
F 3 "" H 2100 1400 50  0001 C CNN
	1    2100 1400
	1    0    0    -1  
$EndComp
Text GLabel 1300 1000 2    60   Input ~ 0
D2
Text GLabel 1300 1100 2    60   Input ~ 0
D10
Text GLabel 2300 1000 2    60   Input ~ 0
D3
Text GLabel 2300 1100 2    60   Input ~ 0
D11
$Comp
L +5VA #PWR022
U 1 1 5A491CD8
P 1300 800
F 0 "#PWR022" H 1300 650 50  0001 C CNN
F 1 "+5VA" H 1300 940 50  0000 C CNN
F 2 "" H 1300 800 50  0001 C CNN
F 3 "" H 1300 800 50  0001 C CNN
	1    1300 800 
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR023
U 1 1 5A491D2C
P 2200 750
F 0 "#PWR023" H 2200 600 50  0001 C CNN
F 1 "+5VA" H 2200 890 50  0000 C CNN
F 2 "" H 2200 750 50  0001 C CNN
F 3 "" H 2200 750 50  0001 C CNN
	1    2200 750 
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR024
U 1 1 5A491D80
P 4350 700
F 0 "#PWR024" H 4350 550 50  0001 C CNN
F 1 "+5VA" H 4350 840 50  0000 C CNN
F 2 "" H 4350 700 50  0001 C CNN
F 3 "" H 4350 700 50  0001 C CNN
	1    4350 700 
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR025
U 1 1 5A491DD4
P 5250 700
F 0 "#PWR025" H 5250 550 50  0001 C CNN
F 1 "+5VA" H 5250 840 50  0000 C CNN
F 2 "" H 5250 700 50  0001 C CNN
F 3 "" H 5250 700 50  0001 C CNN
	1    5250 700 
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR026
U 1 1 5A4924A8
P 6000 700
F 0 "#PWR026" H 6000 550 50  0001 C CNN
F 1 "+5VA" H 6000 840 50  0000 C CNN
F 2 "" H 6000 700 50  0001 C CNN
F 3 "" H 6000 700 50  0001 C CNN
	1    6000 700 
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR027
U 1 1 5A4924FC
P 6800 700
F 0 "#PWR027" H 6800 550 50  0001 C CNN
F 1 "+5VA" H 6800 840 50  0000 C CNN
F 2 "" H 6800 700 50  0001 C CNN
F 3 "" H 6800 700 50  0001 C CNN
	1    6800 700 
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR028
U 1 1 5A4951F7
P 7050 4600
F 0 "#PWR028" H 7050 4450 50  0001 C CNN
F 1 "+5VA" H 7050 4740 50  0000 C CNN
F 2 "" H 7050 4600 50  0001 C CNN
F 3 "" H 7050 4600 50  0001 C CNN
	1    7050 4600
	1    0    0    -1  
$EndComp
Text GLabel 6350 4300 3    60   Input ~ 0
VIN
$Comp
L Conn_01x04 J6
U 1 1 5A498E09
P 6350 4000
F 0 "J6" H 6350 4200 50  0000 C CNN
F 1 "PWR_OPTION" H 6350 3700 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x04_Pitch2.54mm" H 6350 4000 50  0001 C CNN
F 3 "" H 6350 4000 50  0001 C CNN
	1    6350 4000
	0    -1   -1   0   
$EndComp
$Comp
L +5VA #PWR029
U 1 1 5A49947B
P 6600 4300
F 0 "#PWR029" H 6600 4150 50  0001 C CNN
F 1 "+5VA" H 6600 4440 50  0000 C CNN
F 2 "" H 6600 4300 50  0001 C CNN
F 3 "" H 6600 4300 50  0001 C CNN
	1    6600 4300
	-1   0    0    1   
$EndComp
$Comp
L C C2
U 1 1 5A49C433
P 5150 2550
F 0 "C2" H 5175 2650 50  0000 L CNN
F 1 "1000uf" H 5175 2450 50  0000 L CNN
F 2 "Capacitors_THT:CP_Radial_D7.5mm_P2.50mm" H 5188 2400 50  0001 C CNN
F 3 "" H 5150 2550 50  0001 C CNN
	1    5150 2550
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 5A49C5C8
P 4700 2550
F 0 "C1" H 4725 2650 50  0000 L CNN
F 1 "1000uf" H 4725 2450 50  0000 L CNN
F 2 "Capacitors_THT:CP_Radial_D7.5mm_P2.50mm" H 4738 2400 50  0001 C CNN
F 3 "" H 4700 2550 50  0001 C CNN
	1    4700 2550
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR030
U 1 1 5A49C7CC
P 4950 2850
F 0 "#PWR030" H 4950 2600 50  0001 C CNN
F 1 "GND" H 4950 2700 50  0000 C CNN
F 2 "" H 4950 2850 50  0001 C CNN
F 3 "" H 4950 2850 50  0001 C CNN
	1    4950 2850
	1    0    0    -1  
$EndComp
$Comp
L +5VA #PWR031
U 1 1 5A49C824
P 4950 2300
F 0 "#PWR031" H 4950 2150 50  0001 C CNN
F 1 "+5VA" H 4950 2440 50  0000 C CNN
F 2 "" H 4950 2300 50  0001 C CNN
F 3 "" H 4950 2300 50  0001 C CNN
	1    4950 2300
	1    0    0    -1  
$EndComp
Text GLabel 3350 5700 2    60   Input ~ 0
VBATT
Text GLabel 5800 4450 2    60   Input ~ 0
VBATT
Text GLabel 1700 5400 0    60   Input ~ 0
VIN
$Comp
L Conn_01x04 J3
U 1 1 5A482798
P 1900 1100
F 0 "J3" H 1900 1300 50  0000 C CNN
F 1 "BLU_SWITCH" H 1900 800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 1900 1100 50  0001 C CNN
F 3 "" H 1900 1100 50  0001 C CNN
	1    1900 1100
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x04 J4
U 1 1 5A48289D
P 4150 1100
F 0 "J4" H 4150 1300 50  0000 C CNN
F 1 "LED1" H 4150 800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 4150 1100 50  0001 C CNN
F 3 "" H 4150 1100 50  0001 C CNN
	1    4150 1100
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x04 J2
U 1 1 5A482948
P 950 1100
F 0 "J2" H 950 1300 50  0000 C CNN
F 1 "RED_SWITCH" H 950 800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 950 1100 50  0001 C CNN
F 3 "" H 950 1100 50  0001 C CNN
	1    950  1100
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x04 J8
U 1 1 5A4829D2
P 6600 1100
F 0 "J8" H 6600 1300 50  0000 C CNN
F 1 "LED4" H 6600 800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 6600 1100 50  0001 C CNN
F 3 "" H 6600 1100 50  0001 C CNN
	1    6600 1100
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x04 J5
U 1 1 5A483623
P 5050 1100
F 0 "J5" H 5050 1300 50  0000 C CNN
F 1 "LED2" H 5050 800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 5050 1100 50  0001 C CNN
F 3 "" H 5050 1100 50  0001 C CNN
	1    5050 1100
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x04 J7
U 1 1 5A4836AF
P 5800 1100
F 0 "J7" H 5800 1300 50  0000 C CNN
F 1 "LED3" H 5800 800 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 5800 1100 50  0001 C CNN
F 3 "" H 5800 1100 50  0001 C CNN
	1    5800 1100
	-1   0    0    1   
$EndComp
$Comp
L CONN_01X04 P1
U 1 1 5A494423
P 7850 1100
F 0 "P1" H 7850 1350 50  0000 C CNN
F 1 "A_LED1" V 7950 1100 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 7850 1100 50  0001 C CNN
F 3 "" H 7850 1100 50  0000 C CNN
	1    7850 1100
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P2
U 1 1 5A494552
P 8750 1100
F 0 "P2" H 8750 1350 50  0000 C CNN
F 1 "A_LED2" V 8850 1100 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 8750 1100 50  0001 C CNN
F 3 "" H 8750 1100 50  0000 C CNN
	1    8750 1100
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P3
U 1 1 5A4945AF
P 9450 1100
F 0 "P3" H 9450 1350 50  0000 C CNN
F 1 "A_LED3" V 9550 1100 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 9450 1100 50  0001 C CNN
F 3 "" H 9450 1100 50  0000 C CNN
	1    9450 1100
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P4
U 1 1 5A494610
P 10200 1100
F 0 "P4" H 10200 1350 50  0000 C CNN
F 1 "A_LED_4" V 10300 1100 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 10200 1100 50  0001 C CNN
F 3 "" H 10200 1100 50  0000 C CNN
	1    10200 1100
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X04 P6
U 1 1 5A49466F
P 11050 1100
F 0 "P6" H 11050 1350 50  0000 C CNN
F 1 "A_LED5" V 11150 1100 50  0000 C CNN
F 2 "Connectors_JST:JST_XH_B04B-XH-A_04x2.50mm_Straight" H 11050 1100 50  0001 C CNN
F 3 "" H 11050 1100 50  0000 C CNN
	1    11050 1100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR032
U 1 1 5A494FD8
P 7600 1150
F 0 "#PWR032" H 7600 900 50  0001 C CNN
F 1 "GND" H 7600 1000 50  0000 C CNN
F 2 "" H 7600 1150 50  0000 C CNN
F 3 "" H 7600 1150 50  0000 C CNN
	1    7600 1150
	0    1    1    0   
$EndComp
$Comp
L GND #PWR033
U 1 1 5A495036
P 8450 1150
F 0 "#PWR033" H 8450 900 50  0001 C CNN
F 1 "GND" H 8450 1000 50  0000 C CNN
F 2 "" H 8450 1150 50  0000 C CNN
F 3 "" H 8450 1150 50  0000 C CNN
	1    8450 1150
	0    1    1    0   
$EndComp
$Comp
L GND #PWR034
U 1 1 5A495094
P 9150 1150
F 0 "#PWR034" H 9150 900 50  0001 C CNN
F 1 "GND" H 9150 1000 50  0000 C CNN
F 2 "" H 9150 1150 50  0000 C CNN
F 3 "" H 9150 1150 50  0000 C CNN
	1    9150 1150
	0    1    1    0   
$EndComp
$Comp
L GND #PWR035
U 1 1 5A4950F2
P 9900 1150
F 0 "#PWR035" H 9900 900 50  0001 C CNN
F 1 "GND" H 9900 1000 50  0000 C CNN
F 2 "" H 9900 1150 50  0000 C CNN
F 3 "" H 9900 1150 50  0000 C CNN
	1    9900 1150
	0    1    1    0   
$EndComp
$Comp
L GND #PWR036
U 1 1 5A495150
P 10750 1150
F 0 "#PWR036" H 10750 900 50  0001 C CNN
F 1 "GND" H 10750 1000 50  0000 C CNN
F 2 "" H 10750 1150 50  0000 C CNN
F 3 "" H 10750 1150 50  0000 C CNN
	1    10750 1150
	0    1    1    0   
$EndComp
$Comp
L +5VA #PWR037
U 1 1 5A4951AE
P 10850 1350
F 0 "#PWR037" H 10850 1200 50  0001 C CNN
F 1 "+5VA" H 10850 1490 50  0000 C CNN
F 2 "" H 10850 1350 50  0000 C CNN
F 3 "" H 10850 1350 50  0000 C CNN
	1    10850 1350
	0    -1   -1   0   
$EndComp
$Comp
L +5VA #PWR038
U 1 1 5A49520C
P 10000 1350
F 0 "#PWR038" H 10000 1200 50  0001 C CNN
F 1 "+5VA" H 10000 1490 50  0000 C CNN
F 2 "" H 10000 1350 50  0000 C CNN
F 3 "" H 10000 1350 50  0000 C CNN
	1    10000 1350
	0    -1   -1   0   
$EndComp
$Comp
L +5VA #PWR039
U 1 1 5A49526A
P 9250 1350
F 0 "#PWR039" H 9250 1200 50  0001 C CNN
F 1 "+5VA" H 9250 1490 50  0000 C CNN
F 2 "" H 9250 1350 50  0000 C CNN
F 3 "" H 9250 1350 50  0000 C CNN
	1    9250 1350
	0    -1   -1   0   
$EndComp
$Comp
L +5VA #PWR040
U 1 1 5A4952C8
P 8500 1350
F 0 "#PWR040" H 8500 1200 50  0001 C CNN
F 1 "+5VA" H 8500 1490 50  0000 C CNN
F 2 "" H 8500 1350 50  0000 C CNN
F 3 "" H 8500 1350 50  0000 C CNN
	1    8500 1350
	0    -1   -1   0   
$EndComp
$Comp
L +5VA #PWR041
U 1 1 5A495326
P 7600 1350
F 0 "#PWR041" H 7600 1200 50  0001 C CNN
F 1 "+5VA" H 7600 1490 50  0000 C CNN
F 2 "" H 7600 1350 50  0000 C CNN
F 3 "" H 7600 1350 50  0000 C CNN
	1    7600 1350
	0    -1   -1   0   
$EndComp
NoConn ~ 6800 1200
Wire Wire Line
	2700 1550 2700 1800
Wire Wire Line
	2700 1800 3050 1800
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
	3150 6250 3150 6400
Wire Wire Line
	3150 6700 3150 6900
Wire Wire Line
	3150 6250 2950 6250
Wire Wire Line
	2950 6250 2950 6300
Wire Wire Line
	2950 6300 2900 6300
Wire Wire Line
	10450 3450 10450 5400
Connection ~ 10450 4250
Connection ~ 10450 5000
Wire Wire Line
	8750 2250 8750 2900
Wire Wire Line
	8750 3200 8750 3400
Wire Wire Line
	8750 3700 8750 3900
Wire Wire Line
	8750 4200 8750 4350
Wire Wire Line
	8750 4650 8750 4750
Connection ~ 8750 2250
Wire Wire Line
	7300 3300 7300 3650
Wire Wire Line
	7300 3650 7350 3650
Wire Wire Line
	7350 3950 7250 3950
Wire Wire Line
	7250 3950 7250 4100
Wire Wire Line
	1600 3600 1900 3600
Wire Wire Line
	1600 3700 1900 3700
Connection ~ 8750 3200
Wire Wire Line
	7200 3750 7350 3750
Wire Wire Line
	7200 3850 7350 3850
Wire Wire Line
	9650 4000 9650 3450
Wire Wire Line
	9650 3450 9850 3450
Wire Wire Line
	9850 4250 9850 4100
Wire Wire Line
	9850 4100 9650 4100
Wire Wire Line
	9650 4300 9650 4450
Wire Wire Line
	9650 4450 9600 4450
Wire Wire Line
	9650 4200 9750 4200
Wire Wire Line
	9750 4200 9750 5000
Wire Wire Line
	9750 5000 9850 5000
Wire Wire Line
	8300 3850 8300 4700
Wire Wire Line
	8250 3300 8250 3200
Wire Wire Line
	8250 3200 8750 3200
Connection ~ 8750 3700
Connection ~ 8750 4200
Wire Wire Line
	8300 3550 8500 3550
Wire Wire Line
	8500 3550 8500 3700
Wire Wire Line
	8500 3700 8750 3700
Wire Wire Line
	8300 3650 8400 3650
Wire Wire Line
	8400 3650 8400 4200
Wire Wire Line
	8400 4200 8750 4200
Wire Wire Line
	8300 3750 8350 3750
Wire Wire Line
	8350 3750 8350 4650
Wire Wire Line
	8350 4650 8550 4650
Wire Wire Line
	8550 4650 8550 5050
Wire Wire Line
	8550 5050 8750 5050
Wire Wire Line
	3000 1900 3050 1900
Wire Wire Line
	2650 1900 2700 1900
Wire Wire Line
	2650 2000 3050 2000
Wire Wire Line
	1600 4300 1900 4300
Connection ~ 6150 5650
Wire Wire Line
	6150 5650 6150 5200
Wire Wire Line
	6900 5400 6300 5400
Wire Wire Line
	6300 5400 6300 5650
Wire Wire Line
	6300 5650 5700 5650
Wire Wire Line
	5350 5000 5350 5650
Wire Wire Line
	5350 5650 6150 5650
Wire Wire Line
	5700 5450 5600 5450
Wire Wire Line
	5600 5450 5600 5500
Wire Wire Line
	1900 2500 2400 2500
Wire Wire Line
	2400 2500 2400 2650
Wire Wire Line
	1900 2400 2550 2400
Wire Wire Line
	2550 2400 2550 2500
Wire Wire Line
	2550 2500 3050 2500
Wire Wire Line
	1900 2300 3050 2300
Wire Wire Line
	1900 2100 2100 2100
Wire Wire Line
	1900 2200 2100 2200
Wire Wire Line
	5500 4550 5500 4900
Wire Wire Line
	5500 4900 5350 4900
Wire Wire Line
	6350 4200 6350 4300
Wire Wire Line
	6450 4500 6450 4200
Wire Wire Line
	6550 4200 6550 4300
Wire Wire Line
	6550 4300 6600 4300
Wire Wire Line
	4700 2700 4700 2850
Wire Wire Line
	4700 2850 5150 2850
Wire Wire Line
	5150 2850 5150 2700
Connection ~ 4950 2850
Wire Wire Line
	2900 2400 3050 2400
Connection ~ 6900 4900
Wire Wire Line
	6550 4900 7050 4900
Wire Wire Line
	7050 4900 7050 4600
Wire Wire Line
	6900 5100 6900 4900
Connection ~ 5600 4700
Wire Wire Line
	5600 4550 5600 4900
Wire Wire Line
	5600 4900 5750 4900
Wire Wire Line
	5600 4700 6250 4700
Wire Wire Line
	5600 5150 5700 5150
Wire Wire Line
	5600 4450 5600 5150
Wire Wire Line
	6250 4700 6250 4200
Wire Wire Line
	1700 5400 1900 5400
Wire Wire Line
	1150 900  1300 900 
Wire Wire Line
	1300 900  1300 800 
Wire Wire Line
	2100 900  2200 900 
Wire Wire Line
	2200 900  2200 750 
Wire Wire Line
	4350 900  4350 700 
Wire Wire Line
	2100 1200 2100 1400
Wire Wire Line
	1150 1200 1150 1400
Wire Wire Line
	1150 1100 1300 1100
Wire Wire Line
	1150 1000 1300 1000
Wire Wire Line
	2100 1000 2300 1000
Wire Wire Line
	2100 1100 2300 1100
Wire Wire Line
	5250 700  5250 900 
Wire Wire Line
	6800 700  6800 900 
Wire Wire Line
	6000 700  6000 900 
Wire Wire Line
	3150 5950 3150 5700
Wire Wire Line
	3150 5700 3350 5700
Wire Wire Line
	5800 4450 5600 4450
Connection ~ 4950 2400
Wire Wire Line
	4700 2400 5150 2400
Wire Wire Line
	4950 2300 4950 2400
Wire Wire Line
	4350 1100 4500 1100
Wire Wire Line
	4350 1200 4350 1450
Wire Wire Line
	4350 1450 5350 1450
Wire Wire Line
	5350 1450 5350 1100
Wire Wire Line
	5350 1100 5250 1100
Wire Wire Line
	5350 1000 5250 1000
Wire Wire Line
	5250 1200 5250 1650
Wire Wire Line
	5250 1650 6200 1650
Wire Wire Line
	6200 1650 6200 1100
Wire Wire Line
	6200 1100 6000 1100
Wire Wire Line
	6100 1000 6000 1000
Wire Wire Line
	6000 1200 6000 1500
Wire Wire Line
	6000 1500 6950 1500
Wire Wire Line
	6950 1500 6950 1100
Wire Wire Line
	6950 1100 6800 1100
Wire Wire Line
	6950 1000 6800 1000
Wire Wire Line
	8450 1150 8550 1150
Wire Wire Line
	8550 1250 8500 1250
Wire Wire Line
	8500 1250 8500 1350
Wire Wire Line
	9150 1150 9250 1150
Wire Wire Line
	9250 1250 9250 1350
Wire Wire Line
	10000 1250 10000 1350
Wire Wire Line
	9900 1150 10000 1150
Wire Wire Line
	10750 1150 10850 1150
Wire Wire Line
	10850 1250 10850 1350
Wire Wire Line
	8550 950  8550 700 
Wire Wire Line
	8550 700  9200 700 
Wire Wire Line
	9200 700  9200 1050
Wire Wire Line
	9200 1050 9250 1050
Wire Wire Line
	9250 950  9250 750 
Wire Wire Line
	9250 750  9950 750 
Wire Wire Line
	9950 750  9950 1050
Wire Wire Line
	9950 1050 10000 1050
Wire Wire Line
	10000 950  10000 750 
Wire Wire Line
	10000 750  10750 750 
Wire Wire Line
	10750 750  10750 1050
Wire Wire Line
	10750 1050 10850 1050
Wire Wire Line
	7650 1150 7600 1150
Wire Wire Line
	7650 1250 7600 1250
Wire Wire Line
	7600 1250 7600 1350
Wire Wire Line
	7650 950  7650 750 
Wire Wire Line
	7650 750  8400 750 
Wire Wire Line
	8400 750  8400 1050
Wire Wire Line
	8400 1050 8550 1050
Wire Wire Line
	4350 1000 4450 1000
$EndSCHEMATC
