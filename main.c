/*
 * File:   main.c
 * Author: wj
 *
 * Created on 2014 6 24
 *
 * MPLAB XC8 Compiler v1.32 install, but when installing, warning/error occured
 * MPLAB X IDE v2.10 download & install
 * picket3 + mplabx
 */


//**********************************************************************************
//                       PIC12F1501 Pinout
//                          ----------
//                      Vdd |1      8| GND
// out(led enable)<--   RA5 |2      7| RA0/ICSPDAT/AN0
//     tack s/w --> RA4/AN3 |3      6| RA1/ICSPCLK/AN1 / dacout1
//      reset---> RA3/MCLR  |4      5| RA2/AN2/DACOUT2 -> dac out
//                          ----------
//
// MPLAB X IDE -- PICKIT3[ICSPDAT,ICSPCLK, MCLR] -- TARGET BOARD
// RA3 is a special pin. if it is not configured as !MCLR , it can only be an input
// it is no possible to use RA3 as an outpu!
//***********************************************************************************


/* specification of product

 off state  led enable = high
            dac out  = 1.0v
 * tact s/w pressed  h->low : operation mode1
 * mode1(max)
 *  led enable = low
 *  dac out 4.6 v
 * mode2(mid)
 *  led enable = low
 *  dac out 1.8v
 * mode3(low)S
 *  led enable = low
 *  dac out 0.45v
 * SOS mode  4.3 sec(H) - 660(L)-/170/200*4 - 500(L)-300/330*4 -
 * short light duraation 3 times and then long light 3 times


 */
 
/*
 First check!     Target board working/debuging ? as RA3 setting
 Do not use debug mode. just program and then run so that use in-circuit debugger pin

 * if MCLRE =1 &&  LVP=0 , !MCLR  enabled
 * if LVP=1 , !MCLR is enabled
 *
 */

/*
    First headache pb: connection problem 
    hold the button on the pICKIT 3 for 30 seconds without power then connect while still holding the button. 
    the PICKIT should be re-programmed when switching from PIC18 to PIC16 visa versa.
    http://www.microchip.com/forums/m784148.aspx#784148

    for sleep mode
    http://www.gooligum.com.au/tutorials/midrange/PIC_Mid_C_4.pdf

*/



#include <stdio.h>
#include <stdlib.h>

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)


/*
 *
 */
#define _XTAL_FREQ  500000  //0.5Mhz

#define LED_ENABLE LATAbits.LATA5
#define TACT_SW PORTAbits.RA4

#define LED_ON 0
#define LED_OFF 1
#if 1
/*
  dac max = 31
  dac mid = 15
  dac min =  3

*/
#define DAC_MAX  0x1f     
#define DAC_MID  0x1f/2   
#define DAC_MIN  0x1f/10  //3
#else

#define DAC_MAX  (0x1f/5)
#define DAC_MID  (0x1f/10) 
#define DAC_MIN  (0x1f/30)
#endif

#define SWITCH_PRESSED 0
#define SWITCH_RELEASED 1

// Assume timer0 is running 10msec
#define KEY_CHECK_CNT  2  // 20msec key check 
#define SOS_START_READ_TIME 420  // after pressing over 4.2 sec, sos starts.

unsigned char timer0_10msec=0;
unsigned char timer0_1sec=0;
unsigned char timer0_sos=0;

unsigned char key_pressed_cnt=0;
unsigned char key_released_cnt=0;

unsigned char old_key=0;
unsigned char key_pressed=0;
unsigned char  key_sos_cnt=0;

unsigned char sos_s_cnt=0;
//unsigned char sos_o_cnt=0;
unsigned char sos_status=0;

unsigned char key_interrupt=0;


unsigned char timer0_led;
unsigned char dac_value;
unsigned char led_on_status=0;

volatile struct
{
    unsigned Keyfound:1;  // flag used to indicate that pulse value has been grabbed
    unsigned KeySosFound :1;
    unsigned KeySosAction :1;
    unsigned LedToggle :1;
    unsigned led_progressive :1;
    unsigned tmp5 :1;
    unsigned tmp6 :1;
    unsigned tmp7 :1;
}flags;

void interrupt isr(void)
{
    /* 10msec timer0  */
    if(INTCONbits.TMR0IF)
    {
        INTCONbits.TMR0IF =0 ; /* clear interrupt flag */
        TMR0 = 256-39;
        timer0_10msec++;
        timer0_1sec++;
        timer0_sos++;
        timer0_led++;            
    }
    #if 0
    if(INTCONbits.IOCIF)
    {
        INTCONbits.IOCIF=0;
        
        #if 0
        if(!TACT_SW)
        {
            //do something
            
            DACCON1 = DAC_MIN;   // DAC output
             LED_ENABLE=LED_ON;
             key_interrupt++;
        }
        else
        {
            DACCON1 = 0;   // DAC output
             LED_ENABLE=LED_OFF;

        }
        #endif


    }
    #endif
}

#if 0
void interrupt Timer1_Gate_ISR()
{
}

void init_adc(void)
{
    DACCON0bits.DACEN=0; // turn DAC off

    ANSELAbits.ANSA0=1; // Select A0 as analog input
    ADCON0bits.CHS=0x00;  // analog input to use for the ADC conversioin
    ADCON0bits.ADON=1; // ADC on
    ADCON1bits.ADCS=0x01;  // ADC clock select as Fosc/8
    ADCON1bits.ADFM=1; // results are right justified


}
unsigned int Read_ADC_Value(void)
{
    unsigned int ADCValue;

    ADCON0bits.GO = 1;              // start conversion
    while (ADCON0bits.GO);          // wait for conversion to finish
    ADCValue = ADRESH << 8;         // get the 2 msbs of the result and rotate 8 bits to the left
    ADCValue = ADCValue + ADRESL;   // now add the low 8 bits of the resut into our return variable
    return (ADCValue);              // return the 10bit result in a single variable
}
#endif

void led_toggle(unsigned char v)
{
    if(flags.LedToggle==0)
    {
        //DACCON1 = v;
        //LED_ENABLE=LED_ON;
        flags.LedToggle=1;
        dac_value=v;
        flags.led_progressive=1;
    }
    else
    {
        DACCON1 = 0;
        LED_ENABLE=LED_OFF;
        flags.LedToggle=0;

    }
}


int main(int argc, char** argv) {


    /* setup oscillator */
    OSCCONbits.SCS1 =1; // select internal clock
    OSCCONbits.IRCF=0b0111; // osc = 500khz

    /* port assignment input or output*/
    //TRISA=0b001000;  /*input(1) output(0) configure RA1 as an output */
    TRISAbits.TRISA0=0;  // RA0 = nc
    TRISAbits.TRISA1=0;  // RA1 = NC
    TRISAbits.TRISA2=0;  // RA2 = dac out TEST LED
    TRISAbits.TRISA3=1;  // RA3 = nc
    TRISAbits.TRISA4=1;  // RA4 = tack s/w AN4 input
    TRISAbits.TRISA5=0;  // RA5 = TEST LED

    //pull up RA4
    //WPUAbits.WPUA4=1;
    ANSELAbits.ANSA4=0; // RA4 = tack s/w AN4 input

    //LATA=0;  //output pins are all low
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;
    LATAbits.LATA2 = 0;
    //LATAbits.LATA3 = 0;
    ////LATAbits.LATA4 = 0;
    LATAbits.LATA5 = 0;


    //Interrupt-On-Change
#if 0
    
    IOCANbits.IOCAN4 = 1; // enable IOC on RA4 input
    //IOCAFbits.IOCAF4 =1 ;
    INTCONbits.IOCIF=1; // interrupt-on-change interrupt flag bit enable // input interrupt okay
    //INTCONbits.IOCIE=1; // interrupt-on-change enable bit  // dead! not working
   //INTCONbits.INTF=1;
#endif    

    //setup DAC
    DACCON0bits.DACEN=1;
    DACCON0bits.DACOE1= 0; //output on the DACOUT1 pin
    DACCON0bits.DACOE2 =1;  ////output on the DACOUT2 pin
    DACCON0bits.DACPSS= 0; // DAC positive source select bi is VDD or Vref+pin
    DACCON1 = 0;  //0x1f;  // voltages


    //configure timer0 to 10msec
    OPTION_REGbits.TMR0CS = 0; // select timer mode
    OPTION_REGbits.PSA = 0; //assign prescaler to timer0
    OPTION_REGbits.PS = 0b100; //prescale 32   8usec x 32 =256usec x 39 = 9.98msec

    //enable interrupts
    INTCONbits.TMR0IE =1; // enable timer0 interrupt
    ei(); // enable global interrupts

    __delay_ms(10);  //let everything settle

    // initialize
    DACCON1 = 0;   // DAC output
    LED_ENABLE=LED_OFF;
    flags.LedToggle=0;
#if 0  
        for(;;) 
        {
        
            if(timer0_10msec >= KEY_CHECK_CNT)
            {
                /* key check */
                timer0_10msec=0;
                if(key_interrupt) 
                {
                    key_interrupt=0;
                    key_released_cnt++;
                 }   
            }    
        
            if(old_key != key_released_cnt)
            {
                 old_key=key_released_cnt;
                if(key_released_cnt == 1 )
                {
                
                    DACCON1 = DAC_MAX;  // DAC output
                   LED_ENABLE=LED_ON;
                }
                else if(key_released_cnt ==2 )
                {
                
                    DACCON1 = DAC_MID;   // DAC output
                    LED_ENABLE=LED_ON;
                }
                else if(key_released_cnt ==3)
                {
                
                    DACCON1 = DAC_MIN;   // DAC output
                     LED_ENABLE=LED_ON;
                }
                else
                {
                
                    key_released_cnt=0;
                    old_key=0;
                    DACCON1 = 0;   // DAC output
                    LED_ENABLE=LED_OFF;

   // INTCONbits.IOCIF=0;
   // SLEEP();
                    
                }
            }
        }
#endif    

   
    for(;;)
    {
    
        NOP();
        NOP();
        if(timer0_10msec >= KEY_CHECK_CNT)
        {
            /* key check */
            timer0_10msec=0;
            
            if(TACT_SW==SWITCH_PRESSED && key_pressed==0)  // key pressed
            {
                key_pressed_cnt++;
                if(key_pressed_cnt >= 2)
                {
                key_pressed=1;
                key_pressed_cnt=0;
                }
                
                key_sos_cnt=0;
            }
            else if(key_pressed==1)
            {
               if(TACT_SW==SWITCH_RELEASED)  // key_release
               {
                    if(flags.KeySosFound==1)
                    {
                        flags.KeySosFound=0;
                    }
                    else
                    {
                        key_released_cnt++;
                        if(flags.KeySosAction)
                        {
                            flags.KeySosAction=0;
                            key_released_cnt=4;
                        }
                    }

                    key_pressed=0;
                    key_sos_cnt=0;
                    
                    key_pressed_cnt=0;

               }

               if(++key_sos_cnt >= SOS_START_READ_TIME/KEY_CHECK_CNT)
               {
                    if(flags.KeySosAction==0)
                    {
                        flags.KeySosFound=1;
                        flags.KeySosAction=1;
                        sos_s_cnt=0;
                        sos_status=0;

                        key_released_cnt=old_key=0;
                        DACCON1 = 0;   // DAC output
                        LED_ENABLE=LED_OFF;
                        flags.LedToggle=0;


                        key_sos_cnt=0;
                        //key_pressed=0;  //Strange !!
                    }
               }

               
            }
        }
        if(flags.KeySosAction==1)
        {
            switch(sos_status)
            {
                case 0: // init
                    timer0_sos=0;
                    sos_status=1;
                    sos_s_cnt=0;
                    break;
                case 1: // 's' signal toggle as 200msec interval
                    if(timer0_sos>=20)
                    {
                        timer0_sos=0;
                        led_toggle(DAC_MAX);
                        ///LED_ENABLE = ~ LED_ENABLE;
                        if(++sos_s_cnt >= 6)
                        {
                            sos_s_cnt=0;
                            sos_status=2;
                        }
                    }

                    break;
                case 2: // led off 660msec

                    if(timer0_sos>=66)
                    {
                      timer0_sos=0;
                      sos_status=3;
                    }
                    break;
                case 3:  // 'o' signal toggle as 330msec interval

                    if(timer0_sos>=33)
                    {
                        timer0_sos=0;
                        //LED_ENABLE = ~ LED_ENABLE;
                        led_toggle(DAC_MAX);
                        if(++sos_s_cnt >= 6)
                        {
                            sos_s_cnt=0;
                            sos_status=4;
                        }

                    }

                    break;
                case 4: // led off 660 msec
                    if(timer0_sos>=66)
                    {
                      sos_status=0;
                    }
                    break;

                default:
                    break;

            }


        }
        if(old_key != key_released_cnt)
        {
             old_key=key_released_cnt;
            if(key_released_cnt == 1 )
            {
            
               // DACCON1 = DAC_MAX;  // DAC output
               //LED_ENABLE=LED_ON;
               dac_value=DAC_MAX;
               flags.led_progressive=1;
            }
            else if(key_released_cnt ==2 )
            {

                
                //DACCON1 = DAC_MID;   // DAC output
                //LED_ENABLE=LED_ON;
                
                dac_value=DAC_MID;
                flags.led_progressive=1;
            }
            else if(key_released_cnt ==3)
            {
            
                //DACCON1 = DAC_MIN;   // DAC output
                // LED_ENABLE=LED_ON;
                 
                 dac_value=DAC_MIN;
                 flags.led_progressive=1;
#if 0// test for wake up from sleep, but not working
                 
                 DACCON1 = 0;   // DAC output
                 LED_ENABLE=LED_OFF;
                 INTCONbits.IOCIF=0;
                 //INTCONbits.INTF=0;
                  //INTCONbits.IOCIE=1;
                 SLEEP();
                 NOP();
#endif
                 
            }
            else
            {
                key_released_cnt=0;
                old_key=0;
                DACCON1 = 0;   // DAC output
                LED_ENABLE=LED_OFF;
            }
        }
        
        if(flags.led_progressive==1 && dac_value==DAC_MAX )
        {
            switch(led_on_status)
            {
                case 0:
                    DACCON1 = (dac_value*2/10) ;  // DAC output
                    LED_ENABLE=LED_ON;
                    led_on_status=1;
                    timer0_led=0;
            
                    break;
                case 1:
                    
                    if(timer0_led >=3)
                    {
                        timer0_led=0;
                        DACCON1 = (dac_value*4/10);  // DAC output
                        led_on_status=2;
                    }
                    break;
            
                case 2:
                    if(timer0_led >=3)
                    {
                        timer0_led=0;
                        DACCON1 = (dac_value*6/10);  // DAC output
                        led_on_status=3;
                    }
                    break;
                case 3:
                    if(timer0_led >=3)
                    {
                        timer0_led=0;
                        DACCON1 = (dac_value*8/10);  // DAC output
                        led_on_status=4;
                    }
                    break;
                case 4:
                    if(timer0_led >=3)
                    {
                        timer0_led=0;
                        DACCON1 = dac_value;  // DAC output
                        led_on_status=5;
                    }
                    break;
                    
                case 5:
                    NOP();
                    led_on_status=0;
                    flags.led_progressive=0;
                    break;
            
            
            }

        }
        
        else if(flags.led_progressive==1 && dac_value != DAC_MAX)
        {
            DACCON1 = dac_value;
        
        }

    }

}








