/*
 * File:   main.c
 * Author: Adriano Arcadipane
 *
 * Created on December 25, 2024, 6:51 PM
 */

#include <xc.h>

// PIC10F222 Configuration Bit Settings

// CONFIG
#pragma config IOSCFS = 4MHZ    // Internal Oscillator Frequency Select bit (4 MHz)
#pragma config MCPU = OFF       // Master Clear Pull-up Enable bit (Pull-up disabled)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config CP = OFF         // Code protection bit (Code protection off)
#pragma config MCLRE = OFF      // GP3/MCLR Pin Function Select bit (GP3/MCLR pin function is digital I/O, MCLR internally tied to VDD)

#define _XTAL_FREQ 4000000      // Internal frequency 4 MHz (for delay function)

#define OBSERVATION_TIME_MS 1000 // After first brake pressure, the software counts the pressures within thi time
#define CYCLE_TIME_MS 2         // Cycle time
#define REMOTE_TIME_ON_MS_1 1000   // Time the remote control is kept in transmission
#define REMOTE_TIME_ON_MS_2 500   // Time the remote control is kept in transmission

#define TRUE  1
#define FALSE 0

// I/O definitions
#define BRAKE_LIGHT GP3       // Brake light input
#define OUT1 GP0              // Output 1
#define OUT2 GP1              // Output 2
#define OUT3 GP2              // Output 3

void init(){
    //REQUIREMENTS TO MAKE PINS AVAILABLE IN DIGITAL MODE
    ADCON0bits.ANS0 = 0;  //Disable analog input 
    ADCON0bits.ANS1 = 0;  //Disable analog input 
    OSCCALbits.FOSC4 = 0; //Disable TMR input from GP2
    
    //0 GPWU: Enable Wake-up On Pin Change bit (GP0, GP1, GP3)
    //1 GPPU: Disable Weak Pull-ups bit (GP0, GP1, GP3)
    //0 T0CS: Timer0 Clock Source Select bit - Transition on internal instruction cycle clock, FOSC/4
    OPTION = 0b01011111;

    // Set GP0, GP1, GP2 as digital output, GP3 as input
    TRISGPIO = 0b11111000;    // (0 = output, 1 = input)
    GPIO = 0b00000000;        // Init outputs low (0)
}

unsigned char debouncePin(unsigned char raw_state){
    // Change the state when the raw state has the same value 8 consecutive
    // samples
    static unsigned char debounced_state = FALSE;
    static unsigned char candidate_state = 0;

    // candidate_state contains the last 8 raw readings.
    // Each time the function is called, candidate_state content is shifted 
    // left of one bit and the new raw value is added as LSB
    candidate_state = (unsigned char)((candidate_state << 1) | raw_state);
    
    if  (candidate_state == 0xff){ //When input is TRUE for 8 times
        debounced_state = TRUE;
    }
    else  if (candidate_state == 0x00){ //When input is FALSE for 8 times
        debounced_state = FALSE;
    }
    return  debounced_state;
}


void main(void) {
    unsigned char rising_edges_cnt = 0;
    unsigned int cycle = 0;
    unsigned char prev_state = FALSE;
    unsigned char state = FALSE;
    
    init();
    
    while(1){
        //Read the raw input and debounce it
        state = debouncePin(BRAKE_LIGHT);
        
        //Detect rising edge of the input
        if (prev_state == 0 && state == 1) {
            if (rising_edges_cnt < 255) {  //Guard against overflow
                rising_edges_cnt++;
            }
                cycle = 0; //Restart the timer every time the brake lever is pressed
        }
        
        //The timer ticks only when the brake has been pressed at least once
        if (rising_edges_cnt){
            cycle++;
        }
        
        //When the timer expires, activate the output
        if (cycle >= OBSERVATION_TIME_MS/CYCLE_TIME_MS){
            if (rising_edges_cnt == 1){
            ; //Do nothing. We don't want to trigger the output when braking just once
            } else if (rising_edges_cnt == 2){
                OUT1 = 1;
                __delay_ms(REMOTE_TIME_ON_MS_1); //Keep output high for some time
            } else if (rising_edges_cnt == 3){
                OUT2 = 1;
                __delay_ms(REMOTE_TIME_ON_MS_1); //Keep outputs high for some time
            } else if (rising_edges_cnt == 4){
                OUT3 = 1;
                __delay_ms(REMOTE_TIME_ON_MS_2); //Keep outputs high for some time
            }else {
                ; //Do nothing
            }
            
            cycle = 0; //Reset timer
            rising_edges_cnt = 0; //Reset input counts
        }
        
        //Update the previous state for the next cycle
        prev_state = state;
        //Set all outputs low
        OUT1 = 0;
        OUT2 = 0;
        OUT3 = 0;
        //Sleep
        __delay_ms(CYCLE_TIME_MS);
    }// end while(1)

}//end main

