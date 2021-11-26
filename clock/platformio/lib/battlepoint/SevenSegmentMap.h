#ifndef SevenSegmentAsciiMap_H
#define SevenSegmentAsciiMap_H

#include <Arduino.h>

// ASCII MAPPINGS from https://raw.githubusercontent.com/bremme/arduino-TM1637/master/src/SevenSegmentAsciiMap.h
#define SEG_CHAR_SPACE       B00000000 // 32  (ASCII)
#define SEG_CHAR_EXC         B00000110
#define SEG_CHAR_D_QUOTE     B00100010
#define SEG_CHAR_POUND       B01110110
#define SEG_CHAR_DOLLAR      B01101101
#define SEG_CHAR_PERC        B00100100
#define SEG_CHAR_AMP         B01111111
#define SEG_CHAR_S_QUOTE     B00100000
#define SEG_CHAR_L_BRACKET   B00111001
#define SEG_CHAR_R_BRACKET   B00001111
#define SEG_CHAR_STAR        B01011100
#define SEG_CHAR_PLUS        B01010000
#define SEG_CHAR_COMMA       B00010000
#define SEG_CHAR_MIN         B01000000
#define SEG_CHAR_DOT         B00001000
#define SEG_CHAR_F_SLASH     B00000110
#define SEG_CHAR_0           B00111111   // 48
#define SEG_CHAR_1           B00000110
#define SEG_CHAR_2           B01011011
#define SEG_CHAR_3           B01001111
#define SEG_CHAR_4           B01100110
#define SEG_CHAR_5           B01101101
#define SEG_CHAR_6           B01111101
#define SEG_CHAR_7           B00000111
#define SEG_CHAR_8           B01111111
#define SEG_CHAR_9           B01101111
#define SEG_CHAR_COLON       B00110000
#define SEG_CHAR_S_COLON     B00110000
#define SEG_CHAR_LESS        B01011000
#define SEG_CHAR_EQUAL       B01001000
#define SEG_CHAR_GREAT       B01001100
#define SEG_CHAR_QUEST       B01010011
#define SEG_CHAR_AT          B01011111
#define SEG_CHAR_A           B01110111 // 65  (ASCII)
#define SEG_CHAR_B           B01111111
#define SEG_CHAR_C           B00111001
#define SEG_CHAR_D           SEG_CHAR_d
#define SEG_CHAR_E           B01111001
#define SEG_CHAR_F           B01110001
#define SEG_CHAR_G           B00111101
#define SEG_CHAR_H           B01110110
#define SEG_CHAR_I           B00000110
#define SEG_CHAR_J           B00001110
#define SEG_CHAR_K           B01110101
#define SEG_CHAR_L           B00111000
#define SEG_CHAR_M           B00010101
#define SEG_CHAR_N           B00110111
#define SEG_CHAR_O           B00111111
#define SEG_CHAR_P           B01110011
#define SEG_CHAR_Q           B01100111
#define SEG_CHAR_R           B00110011
#define SEG_CHAR_S           B01101101
#define SEG_CHAR_T           SEG_CHAR_t
#define SEG_CHAR_U           B00111110
#define SEG_CHAR_V           B00011100
#define SEG_CHAR_W           B00101010
#define SEG_CHAR_X           SEG_CHAR_H
#define SEG_CHAR_Y           B01101110
#define SEG_CHAR_Z           B01011011
#define SEG_CHAR_L_S_BRACKET B00111001 // 91 (ASCII)
#define SEG_CHAR_B_SLASH     B00110000
#define SEG_CHAR_R_S_BRACKET B00001111
#define SEG_CHAR_A_CIRCUM    B00010011
#define SEG_CHAR_UNDERSCORE  B00001000
#define SEG_CHAR_A_GRAVE     B00010000
#define SEG_CHAR_a           B01011111 // 97 (ASCII)
#define SEG_CHAR_b           B01111100
#define SEG_CHAR_c           B01011000
#define SEG_CHAR_d           B01011110
#define SEG_CHAR_e           B01111011
#define SEG_CHAR_f           SEG_CHAR_F
#define SEG_CHAR_g           B01101111
#define SEG_CHAR_h           B01110100
#define SEG_CHAR_i           B00000100
#define SEG_CHAR_j           B00001100
#define SEG_CHAR_k           SEG_CHAR_K
#define SEG_CHAR_l           B00110000
#define SEG_CHAR_m           SEG_CHAR_M
#define SEG_CHAR_n           B01010100
#define SEG_CHAR_o           B01011100
#define SEG_CHAR_p           SEG_CHAR_P
#define SEG_CHAR_q           SEG_CHAR_Q
#define SEG_CHAR_r           B01010000
#define SEG_CHAR_s           SEG_CHAR_S
#define SEG_CHAR_t           B01111000
#define SEG_CHAR_u           B00011100
#define SEG_CHAR_v           B00011100
#define SEG_CHAR_w           SEG_CHAR_W
#define SEG_CHAR_x           SEG_CHAR_X
#define SEG_CHAR_y           B01100110
#define SEG_CHAR_z           SEG_CHAR_Z
#define SEG_CHAR_L_ACCON     B01111001 // 123 (ASCII)
#define SEG_CHAR_BAR         B00000110
#define SEG_CHAR_R_ACCON     B01001111
#define SEG_CHAR_TILDE       B01000000 // 126 (ASCII)

class AsciiMap {
public:
    const static uint8_t map[96];
};


#endif