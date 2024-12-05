/* 
 * File:   clock.h
 * Author: evanb
 *
 * Created on October 22, 2024, 5:22 PM
 */

#ifndef SPEECH_H
#define	SPEECH_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define F_CPU 3333333UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define SPEECH_ADDRESS 0x40
#define SPEECH_START_ADDRESS 0x01

#define I2C_ADDR               0x40  //i2c address
#define INQUIRYSTATUS          0x21  //Check status
#define ENTERSAVEELETRI        0x88   
#define WAKEUP                 0xFF  //Wake-up command

#define START_SYNTHESIS        0x01  //Start synthesis command 0
#define START_SYNTHESIS1       0x02  //Start synthesis command 1
#define STOP_SYNTHESIS         0x02  //End speech synthesis
#define PAUSE_SYNTHESIS        0x03  //pause speech synthesis command
#define RECOVER_SYNTHESIS      0x04  //Resume speech synthesis commands

#define F_SCL 100000UL
#define TWI_BAUD(F_SCL) ((F_CPU / (2 * F_SCL)) - 5)

typedef enum{
   eAlphabet,/**<Spell>*/
   eWord,/**<word>*/
  } eENpron_t;

typedef enum{
    eChinese,
    eEnglish,
    eNone,
  } eState_t;

uint16_t getWordLen(uint8_t *_utf8);
void begin();
void startTWIw();
void writeBytes(uint8_t* data, uint16_t length);
void read(uint8_t* data, uint8_t len);
void write(uint8_t* data, uint16_t length);
void startTWIr();
void readBytes(uint8_t* data, uint8_t len);
void setVol(uint8_t vol);
void setSpeed(uint8_t speed);
void setTone(uint8_t tone);
void setEnglishPron(eENpron_t pron);
void speakElish(char* str);
void sendPack(uint8_t cmd, uint8_t* data, uint16_t len);
void sendCommand(uint8_t* head, uint8_t* data, uint16_t length);
void speak(char* word);
uint16_t getWordLen(uint8_t *_utf8);
void setup();
void sayBop();
void sayTwist();
void sayPull();
void sayShake();

#define EXPECTED_ACK_VALUE 0x41

#define SAMPLES_PER_BIT 16
#define USART2_BAUD_VALUE(BAUD_RATE) (uint16_t) ((F_CPU << 6) / (((float) SAMPLES_PER_BIT) * (BAUD_RATE)) + 0.5)

void USART2_INIT(void);
void USART2_PRINTF(char *str);




#ifdef	__cplusplus
}
#endif

#endif

