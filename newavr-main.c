/*
 * File:   newavr-main.c
 * Author: lucas
 *
 * Created on November 14, 2024, 12:09 AM
 */


//#define F_CPU 3333333

#include "speech.h"
//#include <avr/io.h>
#include <avr/interrupt.h>
//#include <util/delay.h>
//#include <stdlib.h>
#include <avr/cpufunc.h>
//#include "stdio.h"
//#include "stdlib.h"

#define TWI_WAIT_R() while (!(TWI0.MSTATUS & TWI_RIF_bm))
#define TWI_WAIT_W() while (!(TWI0.MSTATUS & TWI_WIF_bm))
#define ACCELEROMETER_ADDRESS 0x19
#define ACCELEROMETER_START_REGISTER 0x02

enum States {
    INIT = -1,
    BOP,
    PULL,
    TWIST,
    SHAKE,
    OVER
};

volatile enum States state = INIT;
volatile int score = 0;
volatile int delay = 0;

typedef struct {
    register8_t *direction;
    register8_t *output;
    register8_t *ctrl;
    uint8_t bit_map;
} output_t;

output_t SDA = {
    .direction = &PORTA.DIR,
    .output = &PORTA.OUT,
    .ctrl = &PORTA.PIN2CTRL,
    .bit_map = PIN2_bm
};

output_t SCL = {
    .direction = &PORTA.DIR,
    .output = &PORTA.OUT,
    .ctrl = &PORTA.PIN3CTRL,
    .bit_map = PIN3_bm,
};

output_t twist = {
    .direction = &PORTA.DIR,
    .output = &PORTA.OUT,
    .ctrl = &PORTA.PIN4CTRL,
    .bit_map = PIN4_bm,
};

output_t bop = {
    .direction = &PORTA.DIR,
    .output = &PORTA.OUT,
    .ctrl = &PORTA.PIN5CTRL,
    .bit_map = PIN5_bm,
};

output_t pull = {
    .direction = &PORTA.DIR,
    .output = &PORTA.OUT,
    .ctrl = &PORTA.PIN6CTRL,
    .bit_map = PIN6_bm,
};

void initTWI() {
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
                   TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
    TWI0.MBAUD = 10;
    //TWI0.DBGCTRL &= ~TWI_DBGRUN_bm;
    TWI0.MCTRLA = TWI_ENABLE_bm;
    
    *SDA.direction |= SDA.bit_map;
    *SCL.direction |= SCL.bit_map;
    *SDA.ctrl |= PORT_PULLUPEN_bm;
    *SCL.ctrl |= PORT_PULLUPEN_bm;
}

void initInputs() {
    *twist.direction &= ~twist.bit_map;
    *twist.ctrl |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    *bop.direction &= ~bop.bit_map;
    *bop.ctrl |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    *pull.direction &= ~pull.bit_map;
    *pull.ctrl |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
}

void writeTWI_speak(char *data, int len) {
    TWI0.MADDR = 0x40;
    TWI_WAIT_W();
    for (int i = 0; i < len; i++) {
        TWI0.MDATA = data[i];
        TWI_WAIT_W();
    }
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

void writeTWI_display(char *data, int len) {
    TWI0.MADDR = 0b11100010;
    TWI_WAIT_W();
    for (int i = 0; i < len; i++) {
        TWI0.MDATA = data[i];
        TWI_WAIT_W();
    }
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

void readTWI_accel(uint8_t *dest, uint8_t len) {
    TWI0.MADDR = (ACCELEROMETER_ADDRESS << 1) | 0;
    TWI_WAIT_W();
    TWI0.MDATA = ACCELEROMETER_START_REGISTER;
    TWI_WAIT_W();
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    //Reading
    TWI0.MADDR = (ACCELEROMETER_ADDRESS << 1) | 1;
    TWI_WAIT_R();
    
    uint8_t count = 0;
    TWI0.MSTATUS = TWI_CLKHOLD_bm;
    
    while (count < len) {
        TWI_WAIT_R();
        dest[count] = TWI0.MDATA;
        count++;
        if (count != len) {
            TWI0.MCTRLB = TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc;
        }
    }
    TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
}

void RTC_init(void) {
    uint8_t temp;
    
    /* Initialize 32.768kHz Oscillator: */
    /* Disable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_ENABLE_bm;
    /* Writing to protected register */
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    
    while(CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm)
    {
        ; /* Wait until XOSC32KS becomes 0 */
    }
    
    /* SEL = 0 (Use External Crystal): */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_SEL_bm;
    /* Writing to protected register */
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    
    /* Enable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp |= CLKCTRL_ENABLE_bm;
    /* Writing to protected register */
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    
    /* Initialize RTC: */
    while (RTC.STATUS > 0)
    {
        ; /* Wait for all register to be synchronized */
    }

    /* Set period */
    RTC.PER = 32767;
    RTC.CMP = 16383;

    /* 32.768kHz External Crystal Oscillator (XOSC32K) */
    RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;

    /* Run in debug: enabled */
    RTC.DBGCTRL |= RTC_DBGRUN_bm;
    
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc
    | RTC_RTCEN_bm            /* Enable: enabled */
    | RTC_RUNSTDBY_bm;        /* Run In Standby: enabled */
    
    /* Enable Overflow Interrupt */
    RTC.INTCTRL |= RTC_OVF_bm;
    //RTC.INTCTRL |= RTC_CMP_bm;
}

void update_score() {
    char buf[5];
    sprintf(buf, "%04d", score);
    writeTWI_display((char *) &buf, 4);
}

int random_input(int current_input) {
    int inputs[] = {BOP, PULL, TWIST};
    if (current_input == INIT) {
        return inputs[rand() % (2 + 1 - 0) + 0];
    }
    int shift = rand() % (2 + 1 - 1) + 1;
    int new = inputs[(current_input + shift) % 3];
    switch (new) {
        case BOP:
            sayBop();
            break;
        case PULL:
            sayPull();
            break;
        case TWIST:
            sayTwist();
            break;
        case SHAKE:
            sayShake();
            break;
    }
    return new;
}

ISR(RTC_CNT_vect) {
    if (RTC.INTFLAGS & RTC_OVF_bm) {
        if (state == OVER) {
            writeTWI_display((char *) "LOSE", 4);
            delay++;
            if (delay > 5) {
                writeTWI_display((char *) "pLAY", 4);
                delay = 0;
                state = INIT;
            }
        } else if (state == BOP || state == TWIST || state == PULL || state == SHAKE) {
            if (delay > 5) {
                delay = 0;
                state = OVER;
            } else {
                delay++;
            }
        }
        RTC.INTFLAGS &= RTC_OVF_bm;
    }
} 

ISR(PORTA_PORT_vect) {
    if (PORTA.INTFLAGS & twist.bit_map) {
        if (state == TWIST) {
            delay = 0;
            score++;
            update_score();
            state = random_input(TWIST);
        } else if (state == BOP || state == PULL || state == SHAKE) {
            //state = OVER;
        }
        PORTA.INTFLAGS &= twist.bit_map;
    } else if (PORTA.INTFLAGS & bop.bit_map) {
        if (state == INIT) {
            score = 0;
            update_score();
            state = random_input(-1);
        } else if (state == BOP) {
            delay = 0;
            score++;
            update_score();
            state = random_input(BOP);
        } else if (state == TWIST || state == PULL || state == SHAKE) {
            //state = OVER;
        }
        PORTA.INTFLAGS &= bop.bit_map;
    } else if (PORTA.INTFLAGS & pull.bit_map) {
        if (state == PULL) {
            delay = 0;
            score++;
            update_score();
            state = random_input(PULL);
        } else if (state == TWIST || state == BOP || state == SHAKE) {
            //state = OVER;
        }
        PORTA.INTFLAGS &= pull.bit_map;
    }
}

int main(void) {
    /* Replace with your application code */
    initTWI();
    initInputs();
    RTC_init();
    writeTWI_display((char *) "pLAY", 4);
    sei();
//    int16_t accel[3];
//    char str[5];
    while (1) {
//        readTWI_accel((uint8_t *) accel, 6);
//        accel[0] >>= 4;
//        accel[1] >>= 4;
//        accel[2] >>= 4;
//        if (abs(accel[0]) > 2000) {
//            sprintf(str, "%04d", abs(accel[0]));
//            writeTWI_display((char *)&str, 4);
//        } else if (abs(accel[1]) > 2000) {
//            sprintf(str, "%04d", abs(accel[1]));
//            writeTWI_display((char *)&str, 4);
//        } else if (abs(accel[2]) > 2000) {
//            sprintf(str, "%04d", abs(accel[2]));
//            writeTWI_display((char *)&str, 4);
//        }
//        //sprintf(str, "%04d", abs(accel[2]));
//        //writeTWI_display((char *)&str, 4);
//        _delay_ms(200);
    }
}
