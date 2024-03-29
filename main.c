/*

* project.c
    Course      : UVic Mechatronics 458
    Milestone   : 5
    Title       : Final Project

    Name 1: Blake Baldwin   Student ID: V00917567
    Name 2: Hamza Siddique  Student ID: V00998771

*/

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h" 

// Global Variables =======================================
volatile unsigned char direction; //direction of belt
volatile unsigned int ADC_result;
volatile unsigned char ADC_result_flag;
volatile unsigned int min_refl;

volatile unsigned char home_flag;
volatile unsigned char step = 1;
volatile unsigned char loc_stepper;


// Function declerations ==================================
void mTimer(int count);
void mot_CCW();     //DC motor forward
void mot_CW();      //DC motor backwards
void mot_stop();    //DC motor break

void executeStepSequence(); //follows defined stepper motor sequence
void stepperhome(); //goes to stepper home (black)
void stepper90();   //rotates stepper 90 degrees CW
void stepperCW();   //rotates stepper 45 degrees CW
void stepperCCW();  //rotates stepper 45 degrees CCW


int main(){
    // Initial Configeruation ==============================

    // Define clock speed as required
    CLKPR = 0x80;
    CLKPR = 0x01; // sets CPU clock to 8MHz

    // Configure Interrupts
    cli(); // disable global interrupts
    sei(); // enable global interrupts

    // Configure Interrupt 0
    EIMSK |= (_BV(INT0));   // enable INT0
    EICRA |= (_BV(ISC01));  // falling edge interrupt

    // Configure Interrupt 1
    EIMSK |= (_BV(INT1));   // enable INT1
    EICRA |= (_BV(ISC11));  // falling edge interrupt

    // Configure Interrupt 2
    EIMSK |= (_BV(INT2));   // enable INT2
    EICRA |= (_BV(ISC21) | _BV(ISC20)); // rising edge interrupt

    // Configure Interrupt 3
    EIMSK |= (_BV(INT3));   // enable INT3
    EICRA |= (_BV(ISC31));  // falling edge interrupt

    // Configure Interrupt 4
    EIMSK |= (_BV(INT4));   // enable INT4
    EICRB |= (_BV(ISC41));  // falling edge interrupt

    // Configure Interrupt 5
    EIMSK |= (_BV(INT5));   // enable INT5
    EICRB |= (_BV(ISC51));  // falling edge interrupt

    // Configure ADC
    // by default, the ADC input (analog input is set to be ADC0 / PORTF0
    ADCSRA |= _BV(ADEN); // enable ADC
    ADCSRA |= _BV(ADIE); // enable interrupt of ADC
    ADMUX |= _BV(REFS0); // select reference voltage to AVCC w/cap at AREF


    // Configure Pin I/O
    DDRA = 0x3f; // set six lsb of port a as output, controls motor
    DDRB = 0x80; // set MSB of port b as output, controls PWM
    DDRC = 0xff; // set port c as output, controls LCD
    DDRK = 0x3f; // set six lsb of port k as output, controls stepper
    DDRL = 0xf0; // set four msb of port l as output, controls LEDs

    // Configure PWM (TODO change below to use _BV function)
    //Set Timer 0 to Fast PWM
    TCCR0A = TCCR0A | 0b00000011; //enable WGM01 & WGM00
    //Set compare match output mode clear on compare match
    TCCR0A = TCCR0A | 0b10000000; //set COM0A1 & COM0A0
    //Set prescale value in TCCR0B
    TCCR0B = TCCR0B | 0b00000010; //set CS02, CS01 , CS00
    //Set OCRA to desired duty cycle
    OCR0A = 0x7F; // DC motor duty cycle

    // Initialize LCD
	InitLCD(LS_ULINE);
	LCDClear();
	LCDWriteString("Forward");

    stepperhome();  //start stepper on black
    mot_CCW(); //set initial belt direction
	
	
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

            LCDWriteIntXY(6,1,loc_stepper,1);

			if(ADC_result < 1023){
				// start ADC conversion
				ADCSRA |= _BV(ADSC);
			}
			else{
				PORTL = 0xf0;
			}
		}

    }

    return(0);    // this line should never execute
}

// ISRs ===================================================
// Interrupt 0 HL
ISR(INT0_vect){
    home_flag = 1;
}

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
    /*mot_stop(); //break high
	cli();      //disable interrupts
    while(1){   // infinite loop of flashing leds
        PORTL = 0xF0;
        mTimer(500);
        PORTL = 0x00;
        mTimer(500);
    }*/
}

// kill switch on button pull down
ISR(INT4_vect){ //kill switch
    mot_stop(); //break high
	cli();      //disable interrupts
    while(1){   // infinite loop of flashing leds
        PORTL = 0xF0;
        mTimer(500);
        PORTL = 0x00;
        mTimer(500);
    }
}

// Test button
ISR(INT5_vect){
    mTimer(20); //debounce
    sei();      //enable interrupts 
    stepperCW();
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
    mTimer(20); // Wait
    PORTA = 0b00001110; // Drive forward (CCW)
	direction = 1;
}

void mot_CW() {
    PORTA = 0b00001111; // Initially brake
    mTimer(20); // Wait
    PORTA = 0b00001101; // Drive reverse (CW)
	direction = 0;
}

void mot_stop() {
    PORTA = 0b00001111; // Brake high to stop the motor
}

void executeStepSequence() {
	// Assuming stepNumber ranges from 0 to 3, corresponding to steps 1 to 4
	switch (step) {
		case 0: // Step 1
		PORTK = 0b00011011;
		break;
		case 1: // Step 2
		PORTK = 0b00011101;
		break;
		case 2: // Step 3
		PORTK = 0b00101101;
		break;
		case 3: // Step 4
		PORTK = 0b00101011;
		break;
	}
}

void stepperhome(){
    home_flag = 0;

	while (home_flag == 0) {
		step = (step + 1) % 4;
		executeStepSequence();
		mTimer(20); // Delay between steps for timing control
	}
    loc_stepper = 0;
}

void stepper90(){
    unsigned int volatile i = 0;

    while(i < 100){
        executeStepSequence();
        step = (step + 1) % 4;
        mTimer(20); // Delay between steps for timing control
        i++;
    }
    loc_stepper = (loc_stepper + 2) % 4;
}

void stepperCW(){
    unsigned int volatile i = 0;

    while(i < 50){
        executeStepSequence();
        step = (step + 1) % 4;
        mTimer(20); // Delay between steps for timing control
        i++;
    }
    loc_stepper = (loc_stepper + 1) % 4;
}

void stepperCCW(){
    unsigned int volatile i = 0;

    while(i < 50){
        executeStepSequence();        
        if(step << 0){
            step--;
        }
        else{
            step = 3;
        }

        mTimer(20); // Delay between steps for timing control
        i++;
    }

    if(loc_stepper << 0){
        loc_stepper--;
    }
    else{
        loc_stepper = 3;
    }
}