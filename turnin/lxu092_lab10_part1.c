/*	Author: lab
 *  Partner(s) Name: Luofeng Xu
 *	Lab Section:022
 *	Assignment: Lab 10  Exercise 1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif


unsigned char SetBit(unsigned char pin, unsigned char number, unsigned char bin_value) 
{
	return (bin_value ? pin | (0x01 << number) : pin & ~(0x01 << number));
}
unsigned char GetBit(unsigned char port, unsigned char number) 
{
	return ( port & (0x01 << number) );
}



unsigned char GetKeypadKey() {

	PORTC = 0xEF; // Enable col 4 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('1'); }
	if (GetBit(PINC,1)==0) { return('4'); }
	if (GetBit(PINC,2)==0) { return('7'); }
	if (GetBit(PINC,3)==0) { return('*'); }

	// Check keys in col 2
	PORTC = 0xDF; // Enable col 5 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('2'); }
	if (GetBit(PINC,1)==0) { return('5'); }
	if (GetBit(PINC,2)==0) { return('8'); }
	if (GetBit(PINC,3)==0) { return('0'); }

	// Check keys in col 3
	PORTC = 0xBF; // Enable col 6 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('3'); }
	if (GetBit(PINC,1)==0) { return('6'); }
	if (GetBit(PINC,2)==0) { return('9'); }
	if (GetBit(PINC,3)==0) { return('#'); }

	// Check keys in col 4	
	PORTC = 0x7F;
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('A'); }
	if (GetBit(PINC,1)==0) { return('B'); }
	if (GetBit(PINC,2)==0) { return('C'); }
	if (GetBit(PINC,3)==0) { return('D'); }

	return('\0'); // default value

}



typedef struct task{
        int state;
        unsigned long period;
        unsigned long elapsedTime;
        int(*TickFct)(int);
}task;

task tasks[2];
const unsigned short tasksNum=2;
const unsigned long tasksPeriod=100;

volatile unsigned char TimerFlag = 0; 
unsigned long _avr_timer_M = 1; 
unsigned long _avr_timer_cntcurr = 0; 
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
void TimerOn() {
	TCCR1B 	= 0x0B;	
	OCR1A 	= 125;	
	TIMSK1 	= 0x02; 
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;	
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B 	= 0x00; 
}

void TimerISR() {
	unsigned char i;
	for(i=0;i<tasksNum;++i){
		if(tasks[i].elapsedTime>=tasks[i].period){
			tasks[i].state=tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime=0;
		}
		tasks[i].elapsedTime+=tasksPeriod;
	}
}
ISR(TIMER1_COMPA_vect)
{
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) { 	
		TimerISR(); 				
		_avr_timer_cntcurr = _avr_timer_M;
	}
}







unsigned char task1;
enum Respondtopad{Rstart,wait,led};
int Tick_R(int state){
	unsigned char x;
	x=GetKeypadKey();
	switch(state){
		case Rstart:
			state=wait;
			break;
		case wait:
			if(x=='\0'){
				state=wait;
			}
			else{
				state=led;
			}
			break;
		case led:
			if(x=='\0'){
				state=wait;
			}
			else{
				state=led;
			}
			break;
		default:
			break;
	}
	switch(state){
		case Rstart:
			break;
		case wait:
			task1=0x00;
			break;
		case led:
			task1=0x80;
			break;
		default:
			break;
	}
	return state;
}

enum CombineLEDsSM{start,C};
int Tick_C(int state){
	switch(state){
		case start:
			state=C;
			break;
		case C:
			state=C;
			break;
		default:
			break;
	}
	switch(state){
		case start:
			break;
		case C:
			PORTB=task1;
			break;
		default:
			break;
	}
	return state;
}

int main(void) {
	DDRB=0xFF;PORTB=0x00;
	DDRC=0xF0;PORTC=0x0F;
	unsigned char i=0;
	tasks[i].state=start;
	tasks[i].period=100;
	tasks[i].elapsedTime=0;
	tasks[i].TickFct=&Tick_R;
	i++;
	tasks[i].state=start;
        tasks[i].period=100;
        tasks[i].elapsedTime=0;
        tasks[i].TickFct=&Tick_C;
	TimerSet(100);
	TimerOn();
	while(1){
	}
	return 1;
}
