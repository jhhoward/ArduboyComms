#pragma once

class Comms
{
public:
	static constexpr bool useStopBit = true;
	static constexpr bool stopBitValue = false;
	static constexpr int sendBitCount = 7;

	void reset();
	void begin();
	void tick();
	
	bool availableToRead();
	char read();
	bool availableToWrite();
	void write(char value);

	enum class SignalState
	{
		Read,
		PostRead,
		Write
	};
	
	enum class CommsState
	{
		Searching,
		Syncing,
		Synchronised
	};
	
	SignalState signalState = SignalState::Read;
	CommsState commsState = CommsState::Searching;
	
	static constexpr int updateFrequency = 1;
	static constexpr int requiredSyncCount = 50;
	static constexpr int maxReadBufferSize = 8;
	int counter = 0;
	int syncCounter = 0;
	int commsId;
	
	char readBuffer[maxReadBufferSize];
	int readBufferIndex = 0;
	int readBufferSize = 0;
	char readBit = 0;
	bool isReading = false;
	char writeBuffer = 0;
	char writeBit = 0;
	bool isWriting = false;
	
	int minAdcReading = 32767;
	int maxAdcReading = 0;
	int lastAdcReading = 0;
};
