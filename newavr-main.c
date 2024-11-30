/*
 * File:   newavr-main.c
 * Author: evanb
 *
 * Created on November 29, 2024, 8:04 PM
 */

#ifndef F_CPU
#define F_CPU 3333333
#endif

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

uint16_t getWordLen(uint8_t *_utf8);

typedef enum{
   eAlphabet,/**<Spell>*/
   eWord,/**<word>*/
  } eENpron_t;

typedef enum{
    eChinese,
    eEnglish,
    eNone,
  } eState_t;

volatile uint16_t gLength;
volatile eState_t curState = eNone;
volatile eState_t lastState = eNone;
volatile bool lanChange = false;

uint8_t readACK(){
    uint8_t data = 0;
    readBytes(data, 1);
    return data;
    
}

void wait(){
//    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    while (readACK() != 0x41)
    {
        
    }
    
    _delay_ms(100);
    
    while (1)
    {
      uint8_t check[4] = { 0xFD,0x00,0x01,0x21 };
      write(check, 4);
      if (readACK() == 0x4f) break;
      _delay_ms(20);
    }
}

void startTWIr(){
    TWI0.MADDR = (SPEECH_ADDRESS << 1) | 1;
    
//    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    wait();
    TWI0.MCTRLB = TWI_MCMD_REPSTART_gc;
}

void write(uint8_t* data, uint16_t length){
    uint8_t count = 0;
    while(count < length){
        TWI0.MDATA = data[count];
//        while (!(TWI0.MSTATUS & TWI_WIF_bm));
        wait();
        count++;
    }
}

void initTWI() {
    // TODO Set up TWI Peripheral
    PORTA.DIRSET = PIN2_bm | PIN3_bm; //i/o
    
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL = PORT_PULLUPEN_bm;
    
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    
    TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
            TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
    
    TWI0.MBAUD = 10;

    TWI0.MCTRLA = TWI_ENABLE_bm;

}

void begin() {
    TWI0.MADDR = (SPEECH_ADDRESS << 1) | 0;
    
//    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    wait();
    
    uint8_t init = 0xAA;
    writeBytes(init, 1);
    
     _delay_ms(100);
    
    uint8_t check[4] = { 0xFD,0x00,0x01,0x21 };
    
    writeBytes(check, 4);
    
}

void startTWIw(){
    TWI0.MADDR = (SPEECH_ADDRESS << 1) | 0;
    
//    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    wait();
    
    TWI0.MCTRLB = TWI_MCMD_REPSTART_gc;
}



void writeBytes(uint8_t* data, uint16_t length){
    startTWIw();
    
    write(data, length);
    
    
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    
    
}

void read(uint8_t* data, uint8_t len){
    uint8_t bCount = 0;
    TWI0.MSTATUS = TWI_CLKHOLD_bm;
    while (bCount < len)
    {
        //Wait...
//        while (!(TWI0.MSTATUS & TWI_RIF_bm));
        wait();
        
        //Store data
        data[bCount] = TWI0.MDATA;

        //Increment the counter
        bCount += 1;
        
        if (bCount != len)
        {
            //If not done, then ACK and read the next byte
            TWI0.MCTRLB = TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc;
        }
    }
    
    //NACK and STOP the bus
    TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
    
}

void readBytes(uint8_t* data, uint8_t len){
    startTWIr();
    read(data, len);
}

void setVol(uint8_t vol){
    char* str = "[v3]";
    if(vol > 9){
        vol = 9;
    }
    str[2] = 48 + vol;
    speakElish(str);
}

void setSpeed(uint8_t speed){
    char* str = "[s5]";
    if (speed > 9) speed = 9;
    str[2] = 48 + speed;
    speakElish(str);
}

void setTone(uint8_t tone){
    char* str = "[t5]";
    if (tone > 9) tone = 9;
    str[2] = 48 + tone;
    speakElish(str);
}

void setEnglishPron(eENpron_t pron){
    char* str;
    if (pron == eAlphabet) {
        str = "[h1]";
    } else if (pron == eWord) {
        str = "[h2]";
    }
    speakElish(str);
}

void speakElish(char* str){
    uint16_t point = 0;
    uint16_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    uint8_t unicode[len];
    for (uint8_t i = 0; i < len; i++) {
        unicode[i] = str[i] & 0x7F;
    }
    
    sendPack(START_SYNTHESIS1, unicode, len);

}

void sendPack(uint8_t cmd, uint8_t* data, uint16_t len){
    uint16_t length = 0;
    uint8_t head[5] = { 0 };
    head[0] = 0xfd;
    switch (cmd) {
  case START_SYNTHESIS: {
    length = 2 + len;
    head[1] = length >> 8;
    head[2] = length & 0xff;
    head[3] = START_SYNTHESIS;
    head[4] = 0x03;
    sendCommand(head, data, len);
  } break;
  case START_SYNTHESIS1: {
    length = 2 + len;
    head[1] = length >> 8;
    head[2] = length & 0xff;
    head[3] = START_SYNTHESIS;
    head[4] = 0x04;
    sendCommand(head, data, len);
  }
                       break;
  default: {
    length = 1;
    head[1] = length >> 8;
    head[2] = length & 0xff;
    head[3] = cmd;
    writeBytes(head, 4);
  }
         break;
  }
}

void sendCommand(uint8_t* head, uint8_t* data, uint16_t length){
    uint16_t lenTemp = 0;
    startTWIw();
    writeBytes(head, 5);

    startTWIw();
    writeBytes(data, length);
}

//read = 1, write = 0
//void startTWI(){
//    TWI0.MADDR = (addr << 1) | 1;
//}

void speak(char* word){
    uint32_t uni = 0;
    uint8_t utf8State = 0;
    uint16_t point = 0;
    uint16_t len = 0;
    uint16_t _index = 0;
    uint16_t _len = 0;
    while (word[len] != '\0') {
      len++;
    }
    gLength = len;
    uint8_t _utf8[len];
    for (uint32_t i = 0;i <= len;i++) {
        _utf8[i] = word[i];
    }
    word = "";
    uint16_t len1 = getWordLen(_utf8);
    uint8_t _unicode[len1];
    while (_index < _len) {
        if (_utf8[_index] >= 0xfc) {
          utf8State = 5;
          uni = _utf8[_index] & 1;
          _index++;
          for (uint8_t i = 1;i <= 5;i++) {
            uni <<= 6;
            uni |= (_utf8[_index] & 0x3f);
            utf8State--;
            _index++;
          }

        } else if (_utf8[_index] >= 0xf8) {
          utf8State = 4;
          uni = _utf8[_index] & 3;
          _index++;
          for (uint8_t i = 1;i <= 4;i++) {
            uni <<= 6;
            uni |= (_utf8[_index] & 0x03f);
            utf8State--;
            _index++;
          }

        } else if (_utf8[_index] >= 0xf0) {
          utf8State = 3;
          uni = _utf8[_index] & 7;
          _index++;
          for (uint8_t i = 1;i <= 3;i++) {
            uni <<= 6;
            uni |= (_utf8[_index] & 0x03f);
            utf8State--;
            _index++;
          }

        } else if (_utf8[_index] >= 0xe0) {
          curState = eChinese;
          if ((curState != lastState) && (lastState != eNone)) {
            lanChange = true;
          } else {
            utf8State = 2;
            uni = _utf8[_index] & 15;
            _index++;
            for (uint8_t i = 1;i <= 2;i++) {
              uni <<= 6;
              uni |= (_utf8[_index] & 0x03f);
              utf8State--;
              _index++;
            }
            if (_utf8[_index] == 239) {
              lanChange = true;
            }
            lastState = eChinese;
            _unicode[point++] = uni & 0xff;
            _unicode[point++] = uni >> 8;
            //if(point ==  24) lanChange = true;
          }
        } else if (_utf8[_index] >= 0xc0) {
          curState = eChinese;
          if ((curState != lastState) && (lastState != eNone)) {
            lanChange = true;

          } else {
            utf8State = 1;
            uni = _utf8[_index] & 0x1f;
            _index++;
            for (uint8_t i = 1;i <= 1;i++) {
              uni <<= 6;
              uni |= (_utf8[_index] & 0x03f);
              utf8State--;
              _index++;
            }
            lastState = eChinese;
            _unicode[point++] = uni & 0xff;
            _unicode[point++] = uni >> 8;
            //if(point ==  24) lanChange = true;
          }
        } else if (_utf8[_index] <= 0x80) {
          curState = eEnglish;
          if ((curState != lastState) && (lastState != eNone)) {
            lanChange = true;

          } else {
            _unicode[point++] = (_utf8[_index] & 0x7f);
            _index++;
            lastState = eEnglish;
            if (/*(point ==  24) || */(_utf8[_index] == 0x20) || (_utf8[_index] == 0x2c)) lanChange = true;
          }
        }
        if (lanChange == true) {
          if (lastState == eChinese) {
            sendPack(START_SYNTHESIS, _unicode, point);
            wait();
          } else if (lastState == eEnglish) {
            sendPack(START_SYNTHESIS1, _unicode, point);
            wait();
          }
          lastState = eNone;
          curState = eNone;
          point = 0;
          lanChange = false;
        }
      }
    
}





uint16_t getWordLen(uint8_t *_utf8){
    uint16_t index = 0;
    uint32_t uni = 0;
    uint16_t length = 0;
    while (index < gLength) {
    if (_utf8[index] >= 0xfc) {
      index++;
      for (uint8_t i = 1;i <= 5;i++) {
        index++;
      }
      length += 4;
    } else if (_utf8[index] >= 0xf8) {
      index++;
      for (uint8_t i = 1;i <= 4;i++) {
        index++;
      }
      length += 3;
    } else if (_utf8[index] >= 0xf0) {
      index++;
      for (uint8_t i = 1;i <= 3;i++) {
        index++;
      }
      length += 3;
    } else if (_utf8[index] >= 0xe0) {
      index++;
      for (uint8_t i = 1;i <= 2;i++) {
        index++;
      }
      length += 2;
    } else if (_utf8[index] >= 0xc0) {

      index++;
      for (uint8_t i = 1;i <= 1;i++) {
        index++;
      }
      length += 2;
    } else if (_utf8[index] <= 0x80) {
      index++;
      length++;
    }
  }
  return length;
}

void setup() {
  //Init speech synthesis sensor
  begin();
  //Set voice volume to 5
  setVol(9);
  //Set playback speed to 5
  setSpeed(5);
  //Set tone to 5
  setTone(5);
  //For English, speak word 
  setEnglishPron(eWord);
}

int main(void) {
    /* Replace with your application code */
    initTWI();
    setup();
    speak("Hello World");
    while (1) {
        _delay_ms(10000);
        speak("Hello World");
    }
}
