#include <Arduboy2.h>
#include "Comms.h"

#define ADC_PIN RED_LED
#define ADC_MAX 1023

void Comms::reset()
{
	commsState = CommsState::Searching;
	syncCounter = 0;
	isReading = false;
	isWriting = false;
	readBufferSize = 0;
	readBit = 0;
	minAdcReading = 32767;
	maxAdcReading = 0;
}

void Comms::begin()
{
	power_adc_enable();  // The library turns the ADC off, so power it on
	analogReference(DEFAULT); // Use VCC for the ADC reference
}

bool Comms::availableToRead()
{
	return commsState == CommsState::Synchronised && readBufferSize > 0;
}

char Comms::read()
{
	if(availableToRead())
	{
		int index = readBufferIndex - readBufferSize;
		if(index < 0)
			index += maxReadBufferSize;
		char result = readBuffer[index];
		readBufferSize--;
		return result;
	}
	return 0;
}

bool Comms::availableToWrite()
{
	return commsState == CommsState::Synchronised && syncCounter == 0 && !isWriting;
}

void Comms::write(char value)
{
	if(availableToWrite())
	{
		writeBuffer = value;
		writeBit = 0;
		isWriting = true;
	}
}

void Comms::tick()
{
	if(counter > 0)
	{
		counter--;
		return;
	}

	if(signalState == SignalState::Read)
	{
		counter = updateFrequency / 2;

		int adcReading = ADC_MAX - analogRead(ADC_PIN);
		if(adcReading > maxAdcReading)
		{
			maxAdcReading = adcReading;
		}
		if(adcReading < minAdcReading)
		{
			minAdcReading = adcReading;
		}
		int threshold = maxAdcReading - (maxAdcReading - minAdcReading) / 2;
		threshold = 290;
		
		lastAdcReading = adcReading;
		
		bool value = adcReading > threshold;
		
		Serial.println(adcReading);
		
		if(value)
		{
			TXLED1;
		}
		else
		{
			TXLED0;
		}
		
		// Process received bit
		switch(commsState)
		{
			case CommsState::Searching:
				if(value)
				{
					syncCounter++;
					if(syncCounter >= requiredSyncCount)
					{
						commsState = CommsState::Syncing;
						syncCounter = 0;
					}
				}
				else
				{
					syncCounter = 0;
					// Randomly adjust phase to try get in sync with other device
					counter += random(3);
				}
				break;
			case CommsState::Syncing:
				if(syncCounter < requiredSyncCount)
				{
					syncCounter++;
				}
				if(!value)
				{
					commsState = CommsState::Synchronised;
					readBufferSize = 0;
					readBit = 0;
					isReading = false;
					isWriting = false;
					syncCounter = requiredSyncCount;
				}
				break;
			case CommsState::Synchronised:
				if (syncCounter > 0)
				{
					syncCounter--;
				}
				if(isReading)
				{
					if(useStopBit && readBit == sendBitCount)
					{
						if(value != stopBitValue)
						{
							// We expected a stop bit here so there must be a sync issue
							commsState = CommsState::Searching;
							syncCounter = 0;
						}
						else
						{
							// Finished reading data, increment buffer position
							readBufferIndex++;
							if(readBufferIndex >= maxReadBufferSize)
							{
								readBufferIndex = 0;
							}
							readBufferSize++;
							isReading = false;
						}
					}
					else
					{
						if(value)
						{
							readBuffer[readBufferIndex] |= (1 << readBit);
						}
						readBit++;
						
						if(!useStopBit && readBit == sendBitCount)
						{
							// Finished reading data, increment buffer position
							readBufferIndex++;
							if(readBufferIndex >= maxReadBufferSize)
							{
								readBufferIndex = 0;
							}
							readBufferSize++;
							isReading = false;
						}
					}
				}
				else if(value)
				{
					if(readBufferSize >= maxReadBufferSize)
					{
						// Buffer is full : something has gone wrong so assume sync issue
						commsState = CommsState::Searching;
						syncCounter = 0;
					}
					else
					{
						readBuffer[readBufferIndex] = 0;
						readBit = 0;
						isReading = true;
					}
				}
				break;
		}

		signalState = SignalState::PostRead;
	}
	else if(signalState == SignalState::PostRead)
	{
		bool bitToSend = false;
		
		// Send our bit
		switch(commsState)
		{
			case CommsState::Searching:
				bitToSend = true;
				break;
			case CommsState::Syncing:
				bitToSend = syncCounter < requiredSyncCount;
				break;
			case CommsState::Synchronised:
				if(isWriting)
				{
					if(writeBit == 0)
					{
						// Start bit
						bitToSend = true;
						writeBit++;
					}
					else if(writeBit == sendBitCount + 1)
					{
						// Stop bit
						bitToSend = stopBitValue;
						isWriting = false;
					}
					else
					{
						bitToSend = (writeBuffer & (1 << (writeBit - 1))) != 0;
						writeBit++;
						
						if(!useStopBit && writeBit == sendBitCount + 1)
						{
							isWriting = false;
						}
					}
				}
				break;
		}

		// Set LED state
		pinMode(ADC_PIN, OUTPUT);
		digitalWrite(ADC_PIN, bitToSend ? LOW : HIGH);
		
		signalState = SignalState::Write;
		counter = updateFrequency;
	}
	else if(signalState == SignalState::Write)
	{
		// Switch off LED, set up for input
		pinMode(ADC_PIN, INPUT);
		
		signalState = SignalState::Read;
		counter = updateFrequency / 2;
	}
}
