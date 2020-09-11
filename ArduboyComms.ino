#include <Arduboy2.h>
#include "Comms.h"

Arduboy2 arduboy;
Comms comms;

int timer1_counter;

void setup() 
{
	arduboy.begin();
	comms.begin();
  
	noInterrupts();
	TCCR1A = 0;
	TCCR1B = 0;

	timer1_counter = 64286;

	TCNT1 = timer1_counter;
	//TCCR1B |= (1 << CS12);
	TCCR1B |= (1 << CS11) | (1 << CS10);
	TIMSK1 |= (1 << TOIE1);
	interrupts();
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = timer1_counter;
	comms.tick();
}

char recvMessage[50] = "";
//char sendMessage[] = "Hello world!";
char sendMessage[] = "A test message!";
int sendIndex = 0;
int recvIndex = 0;

void loop() 
{
	arduboy.clear();
	
	arduboy.setCursor(0, 0);
	arduboy.println(recvMessage);
	
	switch(comms.commsState)
	{
		case Comms::CommsState::Searching:
		arduboy.println("Searching");
		break;
		case Comms::CommsState::Syncing:
		arduboy.println("Syncing");
		break;
		case Comms::CommsState::Synchronised:
		arduboy.println("Synchronised");
		break;
	}
	arduboy.println(comms.syncCounter);
	arduboy.print("[");
	arduboy.print(comms.minAdcReading);
	arduboy.print("-");
	arduboy.print(comms.maxAdcReading);
	arduboy.print("] ");
	arduboy.println(comms.lastAdcReading);

	arduboy.println("Message to send:");
	arduboy.println(sendMessage);

	
	if(comms.availableToWrite() && sendIndex < sizeof(sendMessage))
	{
		comms.write(sendMessage[sendIndex]);
		sendIndex++;
	}
	if(comms.availableToRead() && recvIndex < sizeof(recvMessage) - 1)
	{
		recvMessage[recvIndex] = comms.read();
		recvMessage[recvIndex + 1] = '\0';
		recvIndex++;
	}
	
	if(arduboy.pressed(A_BUTTON))
	{
		comms.reset();
		recvIndex = 0;
		sendIndex = 0;
		recvMessage[0] = '\0';
	}
	
	arduboy.display();
}
