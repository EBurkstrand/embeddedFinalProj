#include "SPEECH.h"

void USART2_INIT(void)
{
    /* Set TX pin as output and RX pin as input */
    PORTF.DIRSET = PIN0_bm;
    PORTF.DIRCLR = PIN1_bm;
    
    /* Set the BAUD Rate using the macro from the tutorial
     *      Use 9600 as the standard baud rate. You will need to match this in Putty
     */
    USART2.BAUD = (uint16_t)USART2_BAUD_VALUE(9600);
    
    /* Enable transmission for USART2 */
    USART2.CTRLB |= USART_TXEN_bm;
}

void USART2_PRINTF(char *str)
 {
    for(size_t i = 0; i < strlen(str); i++)
    {
        while (!(USART2.STATUS & USART_DREIF_bm));      
        USART2.TXDATAL = str[i];
    }
//    USART2.TXDATAL = 0x45;
 }

void USART2_PRINTF2(uint8_t data[])
 {
    uint8_t length = sizeof(data) / sizeof(data[0]);
    
    
    for(size_t i = 0; i < length; i++)
    {
        while (!(USART2.STATUS & USART_DREIF_bm));      
        USART2.TXDATAL = data[i];
    }
//    USART2.TXDATAL = 0x45;
 }

void initTWI() {
    // TODO Set up TWI Peripheral
//    PORTA.DIRSET = PIN2_bm | PIN3_bm; //i/o
    PORTA.DIRSET = PIN2_bm | PIN3_bm;
    
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL = PORT_PULLUPEN_bm;
    
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    
//    TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
//            TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
    
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
    
    TWI0.MBAUD = (uint8_t)TWI_BAUD(F_SCL);

    TWI0.MCTRLA = TWI_ENABLE_bm;

}

void startTWI(uint8_t addr, uint8_t read){
    TWI0.MADDR = (addr << 1) | read;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
}

void writeToTWI(uint8_t* data, uint8_t len){
    uint8_t count = 0;
    while(count < len){
        TWI0.MDATA = data[count];
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
        count += 1;
    }
}

void stopTWI(){
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}
#define CLOCK_ADDRESS 0x71
#define COLON_ADDRESS 0x77
#define TWI_READ 1
#define TWI_WRITE 0

void toggleColon(){
    uint8_t buffer[2];
    buffer[0] = COLON_ADDRESS;
    if(0){
        buffer[1] = 0x00;
    } else {
        buffer[1] = 0x10;
    }
    startTWI(CLOCK_ADDRESS, TWI_WRITE);
    writeToTWI(buffer, 2);
    stopTWI();
}

uint8_t readACK(){
    uint8_t data = 0;
    readBytes(&data, 1);
    return data;
    
}

void wait(){
//    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    while (readACK() != 0x41)
    {
        PORTA.OUT |= PIN5_bm;
    }
    
     _delay_ms(100);

//    
    while (1)
    {
      uint8_t check[4] = { 0xFD,0x00,0x01,0x21 };
      writeBytes(check, 4);
      if (readACK() == 0x4f) break;
      _delay_ms(20);
    }
}



void write(uint8_t* data, uint16_t length){
    uint8_t count = 0;
    while(count < length){
        TWI0.MDATA = data[count];
        while (!(TWI0.MSTATUS & TWI_WIF_bm));
        if(TWI0.MSTATUS & TWI_RXACK_bm){
            return;
        }
        count++;
    }
}



void begin() {
//    TWI0.MADDR = (SPEECH_ADDRESS << 1) | 0;
//    
//    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    
    uint8_t init = 0xAA;
    writeBytes(&init, 1);
    
     _delay_ms(100);
    
    uint8_t check[4] = { 0xFD,0x00,0x01,0x21 };
    
    writeBytes(check, 4);
    
}

void startTWIw(){
    TWI0.MADDR = (SPEECH_ADDRESS << 1) | 0;
    
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    
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
        while (!(TWI0.MSTATUS & TWI_RIF_bm));
        
        
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

void startTWIr(){
    TWI0.MADDR = (SPEECH_ADDRESS << 1) | 1;
    
    while (!(TWI0.MSTATUS & TWI_RIF_bm));
}

void readBytes(uint8_t* data, uint8_t len){
    startTWIr();
    read(data, len);
}

void setVol(uint8_t vol){
    char str[4] = "[v3]";
    if(vol > 9){
        vol = 9;
    }
    str[2] = 48 + vol;
    speakElish(str);
}

void setSpeed(uint8_t speed){
    char str[4] = "[s5]";
    if (speed > 9) speed = 9;
    str[2] = 48 + speed;
    speakElish(str);
}

void setTone(uint8_t tone){
    char str[4] = "[t5]";
    if (tone > 9) tone = 9;
    str[2] = 48 + tone;
    speakElish(str);
}

void setEnglishPron(eENpron_t pron){
    char* str = "[h2]";
    if (pron == eAlphabet) {
        str = "[h1]";
    } else if (pron == eWord) {
        str = "[h2]";
    }
    speakElish(str);
}

void speakElish(char* str){
//    uint16_t point = 0;   
    uint16_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    uint8_t unicode[len];
    for (uint8_t i = 0; i < len; i++) {
        unicode[i] = str[i] & 0x7F;
    }
    
    sendPack(START_SYNTHESIS1, unicode, len);
    
    //wait();)

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
//    uint16_t lenTemp = 0;
    
    startTWIw();
    
    write(head, 5);

    write(data, length);
    
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
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
    _len = len;
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

    if (lastState == eChinese) {
        sendPack(START_SYNTHESIS, _unicode, point);
        wait();
    } else if (lastState == eEnglish) {
        PORTA.DIR |= PIN6_bm;
        sendPack(START_SYNTHESIS1, _unicode, point);
        wait();
    }
    lastState = eNone;
    curState = eNone;
    point = 0;
    lanChange = false;

    _index = 0;
    _len = 0;
}





uint16_t getWordLen(uint8_t *_utf8){
    uint16_t index = 0;
//    uint32_t uni = 0;
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

void sayBop(){
    begin();
    speak("bop-it");
    _delay_ms(100);
}

void sayTwist(){
    begin();
    speak("twist-it");
    _delay_ms(100);
}

void sayPull(){
    begin();
    speak("pull-it");
    _delay_ms(100);
}

void sayShake(){
    begin();
    speak("shake-it");
    _delay_ms(100);
}
