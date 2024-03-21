
/*
* project.c
    Course      : UVic Mechatronics 458
    Milestone   : 5
    Title       : Final Project

    Name 1: Blake Baldwin    Student ID: V00917567
    Name 2: Hamza Siddique  Student ID: V00998771

 */

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h" 

// Global Variables =======================================
volatile unsigned int direction; //direction of belt
volatile unsigned int ADC_result;
volatile unsigned int ADC_result_flag;
volatile unsigned int min_refl;


// Function declerations ==================================
    //basic timer (such as mTimer)
    void mTimer(int count);
    void mot_CCW();     //motor forward
    void mot_CW();      //motor backwards
    void mot_stop();    //motor break


int main(){
    // Initial Configeruation ==============================

    // Define clock speed as required
    CLKPR = 0x80;
    CLKPR = 0x01; // sets CPU clock to 8MHz

    // Configure Interrupts
    cli(); // disable global interrupts
    sei(); // enable global interrupts

    // Configure Interrupt 1
    EIMSK |= (_BV(INT1)); // enable INT1
    EICRA |= (_BV(ISC11)); // falling edge interrupt

    // Configure Interrupt 2
    EIMSK |= (_BV(INT2)); // enable INT2
    EICRA |= (_BV(ISC21) | _BV(ISC20)); // rising edge interrupt

    // Configure Interrupt 3
    EIMSK |= (_BV(INT3)); // enable INT3
    EICRA |= (_BV(ISC31)); // falling edge interrupt

    // Configure ADC
    // by default, the ADC input (analog input is set to be ADC0 / PORTF0
    ADCSRA |= _BV(ADEN); // enable ADC
    ADCSRA |= _BV(ADIE); // enable interrupt of ADC
    ADMUX |= _BV(REFS0); // select reference voltage to AVCC w/cap at AREF


    // Configure Pin I/O
    DDRA = 0x3f; // set six lsb of port a as output, controls motor
    DDRB = 0xff; // set port b as output
    DDRC = 0xff; // set port c as output
    DDRL = 0xf0; // set 4 msb of port l as output

    // Configure PWM TODO change below to use _BV function
    //Set Timer 0 to Fast PWM
    TCCR0A = TCCR0A | 0b00000011; //enable WGM01 & WGM00
    //Set compare match output mode clear on compare match
    TCCR0A = TCCR0A | 0b10000000; //set COM0A1 & COM0A0
    //Set prescale value in TCCR0B
    TCCR0B = TCCR0B | 0b00000010; //set CS02, CS01 , CS00
    //Set OCRA to desired duty cycle
    OCR0A = 0xBF; // DC motor duty cycle

    // Initialize LCD
	InitLCD(LS_ULINE);
	//Clear the screen
	LCDClear();
	//Simple string printing
	LCDWriteString("Forward");


    mot_CCW(); //set initial belt direction
	
	// start one conversion at the beginning ==========
	//ADCSRA |= _BV(ADSC);
	
	
    while(1){
		if (ADC_result_flag){

			ADC_result_flag = 0x00;
			if(ADC_result < min_refl){
				min_refl = ADC_result;
			}

			// Update LCD
			LCDClear();
			
			if(direction)		//Print Direction
			{
				LCDWriteString("Forward");
			}
			else
			{
				LCDWriteString("Reverse");
			}
			
			//Write reflectivity of item to LCD
			//LCDWriteIntXY(1,1,(ADC_result*100/255),3);
			//LCDWriteStringXY(1,4,"%");
			LCDWriteIntXY(0,1,min_refl,4);

			if(ADC_result < 1023){
				// start ADC conversion
				ADCSRA |= _BV(ADSC);
			}
			else{
				PORTL = 0xF0;
			}
		}

    }

    return(0);    // this line should never execute
}

// ISRs ===================================================
// Interrupt 1 EX
ISR(INT1_vect){
    //mTimer(20);
    //PORTL = 0xF0;
    //mTimer(500);
    //PORTL = 0x00;
}

// Interrupt 2 OR
ISR(INT2_vect){
    mTimer(20);
	PORTL = 0xF0;
	min_refl = 1022;
    ADCSRA |= _BV(ADSC); // start and ADC conversion
}

// kill switch on button pull down
ISR(INT3_vect){ //kill switch
    mot_stop(); // break high
	cli(); //disable interrupts
    while(1){ // infinite loop of flashing leds
        PORTL = 0xF0;
        mTimer(500);
        PORTL = 0x00;
        mTimer(500);
    }
}

// the interrupt will be trigured if the ADC is done ========================
ISR(ADC_vect){
	ADC_result = ADCL;
	ADC_result = ADC_result | (ADCH << 8);
	ADC_result_flag = 1;
}

// Functions ==============================================
// mTimer, basic polling method timer, not interrupt driven
void mTimer (int count) {

   int i;

   i = 0;

   TCCR1B |= _BV (CS11);  // Set prescaler (/8) clock 16MHz/8 -> 2MHz
   /* Set the Waveform gen. mode bit description to clear
     on compare mode only */
   TCCR1B |= _BV(WGM12);

   OCR1A = 0x03E8; //set output compare register for 1ms
   TCNT1 = 0x0000; //sets initial timer value
   //TIMSK1 = TIMSK1 | 0b00000010; //enable output compare interrupt enable
   TIFR1 |= _BV(OCF1A); //clear timer interrupt flag and begin new timing

   //poll the timer
   while(i<count){
       if((TIFR1 & 0x02) == 0x02){
           TIFR1 |= _BV(OCF1A);//clear interrupt by writing a ONE to the bit
           i++;

       }//if

   }//while
   return;
}

void mot_CCW() {
    PORTA = 0b00001111; // Initially brake
    mTimer(20); // Wait a bit
    PORTA = 0b00001110; // Drive forward (CCW)
	direction = 1;
}

void mot_CW() {
    PORTA = 0b00001111; // Initially brake
    mTimer(20); // Wait a bit
    PORTA = 0b00001101; // Drive reverse (CW)
	direction = 0;
}

void mot_stop() {
    PORTA = 0b00001111; // Brake high to stop the motor
}