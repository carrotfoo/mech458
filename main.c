 
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

// Global Variables =======================================
unsigned int direction; //direction of belt


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

    // Configure Interrupt 2
    EIMSK |= (_BV(INT2)); // enable INT2
    EICRA |= (_BV(ISC21) | _BV(ISC20)); // rising edge interrupt

    // Configure Interrupt 3
    EIMSK |= (_BV(INT3)); // enable INT3
    EICRA |= (_BV(ISC31)); // falling edge interrupt

    // Configure ADC

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
    OCR0A = 0x7F; // 50% duty cycle

    // Initialize LCD

    direction = 1; //set initial direction
    while(1){
        if (direction){ // 
            mot_CW();
            mTimer(5000);
            mot_CCW();
        }

    }

    return(0);    // this line should never execute

}

// ISRs ===================================================
// Interrupt 2 OR Trigger
ISR(INT2_vect){
    mTimer(20);
    PORTL = 0xF0;
    mTimer(500);
    PORTL = 0x00;
}

// kill switch on button pull down
ISR(INT3_vect){ //kill switch
    //PORTA = 0b00001111; // break high
    mot_stop(); // break high
	cli(); //disable interrupts
    while(1){ // infinite loop of flashing leds
        PORTL = 0xF0;
        mTimer(500);
        PORTL = 0x00;
        mTimer(500);
    }
}

// Functions ++++++++++++++++++++++++++++++++++++++++++++++
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
