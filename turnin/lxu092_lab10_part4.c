/*	Author: lab
 *  Partner(s) Name: Luofeng Xu
 *	Lab Section:022
 *	Assignment: Lab 10  Exercise 4
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *
 *	Demo Link: Youtube URL>https://youtu.be/kaq_nkBJKJI
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


const unsigned char pwd[5]={'1','2','3','4','5'};

unsigned char GetKeypadKey() {

	// Check keys in col 4  
        PORTC = 0x7F;
        asm("nop"); // add a delay to allow PORTC to stabilize before checking
        if (GetBit(PINC,0)==0) { return('A'); }
        if (GetBit(PINC,1)==0) { return('B'); }
        if (GetBit(PINC,2)==0) { return('C'); }
        if (GetBit(PINC,3)==0) { return('D'); }

	// Check keys in col 3
        PORTC = 0xBF; // Enable col 6 with 0, disable others with 1’s
        asm("nop"); // add a delay to allow PORTC to stabilize before checking
        if (GetBit(PINC,0)==0) { return('3'); }
        if (GetBit(PINC,1)==0) { return('6'); }
        if (GetBit(PINC,2)==0) { return('9'); }
        if (GetBit(PINC,3)==0) { return('#'); }

	// Check keys in col 2
        PORTC = 0xDF; // Enable col 5 with 0, disable others with 1’s
        asm("nop"); // add a delay to allow PORTC to stabilize before checking
        if (GetBit(PINC,0)==0) { return('2'); }
        if (GetBit(PINC,1)==0) { return('5'); }
        if (GetBit(PINC,2)==0) { return('8'); }
        if (GetBit(PINC,3)==0) { return('0'); }


	PORTC = 0xEF; // Enable col 4 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('1'); }
	if (GetBit(PINC,1)==0) { return('4'); }
	if (GetBit(PINC,2)==0) { return('7'); }
	if (GetBit(PINC,3)==0) { return('*'); }

	return('\0'); // default value

}



typedef struct task{
        int state;
        unsigned long period;
        unsigned long elapsedTime;
        int(*TickFct)(int);
}task;

task tasks[5];
const unsigned short tasksNum=5;
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

void set_PWM(double frequency) {
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08;
		else TCCR3B |= 0x03; 
		if (frequency < 0.954) OCR3A = 0xFFFF;
		else if (frequency > 31250) OCR3A = 0x0000;
		
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; 
		current_frequency = frequency;
	}
}
void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}
void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

unsigned char newpwd[4]={'g','g','g','g'};


unsigned char change=0;
unsigned char task1=1;
unsigned char task2=1;
unsigned char t;
unsigned char flag;
enum Respondtopad{Rstart,wait,act,rdy,rdy_c,pass};
int Tick_R(int state){
	unsigned char x;
	unsigned char tmp;
	x=GetKeypadKey();
	switch(state){
		case Rstart:
			t=0;
			state=wait;
			break;
		case wait:
			if(x!='#'){
				state=wait;
			}
			else if(x=='#'){
				t=0;
				flag=0;
				state=act;
			}
			break;
		case act:
			if(x=='#'){
                                state=act;
                        }
                        else if((x=='\0')&&(change==0)){
                                state=rdy;
                        }
			else if((x=='\0')&&(change==1)){
				state=rdy_c;
			}
			break;
		case rdy:
			if(x=='#'){
				state=act;
				t=0;
				flag=0;
			}
			else if((x=='\0')&&(t<5)){
				state=rdy;
			}
			else if((x!='\0')&&(t<5)){
				tmp=x;
				state=pass;
				if(tmp==pwd[t]){
					flag=flag+1;
				}
				t=t+1;
			}
			else if((x=='\0')&&(t>=5)){
				state=wait;
			}
			break;
		case pass:
			if(x!='\0'){
				state=pass;
			}
			else if((x=='\0')&&(change==0)){
				state=rdy;
			}
			else if((x=='\0')&&(change==1)){
                                state=rdy_c;
			}
			break;
		case rdy_c:
			if(x=='#'){
                                state=act;
                                t=0;
                                flag=0;
                        }
                        else if((x=='\0')&&(t<4)){
                                state=rdy_c;
                        }
                        else if((x!='\0')&&(t<4)){
                                tmp=x;
                                state=pass;
                                if(tmp==newpwd[t]){
					task2=!task2;
                                        flag=flag+1;
                                }
                                t=t+1;
                        }
                        else if((x=='\0')&&(t>=4)){
                                state=wait;
                        }
			break;
		default:
			break;
	}
	switch(state){
		case wait:
			if (((flag==5)&&(change==0))||((flag==4)&&(change==1))){
				task1=0;
				flag=0;
			}
			break;
		case act:
			flag=0;
			t=0;
			break;
		default:
			break;

	}
	return state;
}



enum LockSys{Lstart,lockr,lockp};
int Tick_L(int state){
	switch(state){
		case Lstart:	
			state=lockr;
			break;
		case lockr:
			if((~PINB & 0x80)==0x80){
				task1=1;
				state=lockp;
			}
			else{
				state=lockr;
			}
			break;
		case lockp:
			if((~PINB & 0x80)==0x80){
				state=lockp;
			}
			else{
				state=lockr;
			}
			break;
		default:
			break;
	}
	switch(state){
		case lockr:
			break;
		default:
			break;
	}
	return state;
}

const unsigned short tone[8]={261.63,293.66,329.63,349.23,392.00,440.00,493.88,523.25};
const unsigned char melody[9]={8,8,8,8,5,7,8,7,8};

unsigned char m_i;
unsigned char t_i;
enum Doorbell{DStart,Wait,Play,Finish};
int Tick_D(int state){
	switch(state) 
	{
		case DStart:
			state = Wait;
			break;
		case Wait:
			if((~PINA & 0x80)==0x00){
				state = Wait;
			}
			else if((~PINA&0x80)==0x80){
				state = Play;
				m_i=0;
				t_i=0;
			}
			break;
		case Play:
			if(t_i<18){
				state = Play;
			}
			else if(t_i>=18){
				state=Finish;
			}
			break;
		case Finish:
			if((~PINA & 0x80)!=0x00){
				state=Finish;
			}
			else if((~PINA & 0x80)==0x00){
				state=Wait;
			}
			break;
		default:
			break;
	}
	switch(state)
	{
		case DStart:
			set_PWM(0);
			break;
		case Wait:
			m_i=0;
			t_i=0;
			set_PWM(0);
			break;
		case Play:
			m_i=t_i/2;
			if (t_i%2!=1){
				set_PWM(tone[melody[m_i]-1]);
			}
			else if(t_i%2==1){
				set_PWM(0);
			}
			t_i++;
			break;
		case Finish:
			set_PWM(0);
			break;
		default:
			break;
		
	}
	return state;
}
unsigned char pflag;
unsigned char cnt;
unsigned char t_p;
enum Changepassword{Pstart,pwait,pact,prdy,verify,verify_p,done};
int Tick_P(int state){
	unsigned char x;
        x=GetKeypadKey();
	switch(state){
		case Pstart:
			state=pwait;
			break;
		case pwait:
			if((x=='*')&&((~PINB & 0x80)==0x80)){
				state=pact;
				t_p=0;
				cnt=0;
			}
			else {
				state=pwait;
			}
			break;
		case pact:
			if ((x=='*')&&((~PINB&0x80)==0x80)){
				state=pact;
			}
			else if((x!='*')&&((~PINB&0x80)==0x80)&&(x!='\0')&&(t_p<4)){
				state=prdy;
				newpwd[t_p]=x;
				t_p++;
			}
			else if((x=='\0')&&((~PINB&0x80)==0x00)&&(t_p>=4)){
				state=verify;
				cnt=0;
				t_p=0;
				pflag=0;
			}
			break;
		case prdy:
			if(((~PINB&0x80)==0x80)&&(x!='*')){
				state=prdy;
			}
			else if((x=='*')&&((~PINB&0x80)==0x80)){
				state=pact;
			}
			break;
		case verify:
			if((cnt<50)&&(t_p<4)&&(x!='\0')){
				state=verify_p;
				if(newpwd[t_p]==x){
					pflag++;
					task2=!task2;
				}
				t_p++;
			}
			else if((cnt<50)&&(t_p<4)&&(x=='\0')){
				state=verify;
			}
			else if((cnt<50)&&(pflag==4)){
				change=1;
				state=done;
			}
			else if(((cnt>50)||(t_p>=4))&&(pflag!=4)){
				state=pwait;
			}
			break;
		case verify_p:
			if(cnt>=50){
				state=pwait;
			}
			else if((cnt<50)&&(x!='\0')){
				state=verify_p;
			}
			else if((cnt<50)&&(x=='\0')){
				state=verify;
			}
			break;
		case done:
			state=done;
			break;
		default:
			break;
	}
	switch(state){
		case verify:
			cnt++;
			break;
		case verify_p:
			cnt++;
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
			PORTB=task1|(change<<1)|(task2<<2);
			break;
		default:
			break;
	}
	return state;
}

int main(void) {
	DDRA=0x00;PORTA=0xFF;
	DDRB=0x7F;PORTB=0x80;
	DDRC=0xF0;PORTC=0x0F;
	PWM_on();
	unsigned char i=0;
	tasks[i].state=Rstart;
	tasks[i].period=100;
	tasks[i].elapsedTime=0;
	tasks[i].TickFct=&Tick_R;
        i++;
        tasks[i].state=Lstart;
        tasks[i].period=100;
        tasks[i].elapsedTime=0;
        tasks[i].TickFct=&Tick_L;
	i++;
	tasks[i].state=DStart;
	tasks[i].period=200;
	tasks[i].elapsedTime=0;
	tasks[i].TickFct=&Tick_D;
	i++;
	tasks[i].state=Pstart;
	tasks[i].period=100;
	tasks[i].elapsedTime=0;
	tasks[i].TickFct=&Tick_P;
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
