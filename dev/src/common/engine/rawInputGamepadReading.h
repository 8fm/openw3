#pragma once
#include "../redSystem/types.h"
#include "../redSystem/crt.h"

#include "../redThreads/redThreadsThread.h"

struct SRawMouseReading
{
	Float x;
	Float y;
	Float dx;
	Float dy;
	Bool leftButtonDown;
	Bool rightButtonDown;
	Bool middleButtonDown;

	SRawMouseReading()
		: dx( 0 )
		, dy( 0 )
		, leftButtonDown( false )
		, rightButtonDown( false )
		, middleButtonDown( false )
	{}
};

struct SRawKeyboardEvent
{
	Uint32	m_key;
	Bool	m_down;
};

// Could've been a ring buffer, but w/e
// Might not ultimately need the mutex either, but better safe
class CRawKeyboardReadingBuffer
{
public:
	static const Uint32 READBUFSIZE = 1024;

private:

	SRawKeyboardEvent	m_readBuf[ READBUFSIZE ];
	::Red::Threads::CMutex m_mutex;
	Uint32				m_numEvents;
	Uint32				m_startIndex;

public:
	CRawKeyboardReadingBuffer()
		: m_numEvents( 0 )
		, m_startIndex( 0 )
	{
		Reset_NoSync();
	}

	~CRawKeyboardReadingBuffer()
	{
	}

	void AddKeyEvent( Uint32 key, Bool down )
	{
		::Red::Threads::CScopedLock< ::Red::Threads::CMutex > lock( m_mutex );

		if ( IsFull_NoSync() )
		{
			Reset_NoSync();
		}

		SRawKeyboardEvent event = { key, down };
		m_readBuf[ m_numEvents++] = event;
	}

	void GetKeyEvents( SRawKeyboardEvent elems[], Uint32* /*[inout]*/ numElems )
	{
		::Red::Threads::CScopedLock< ::Red::Threads::CMutex > lock( m_mutex );

		const Uint32 numUnread = m_numEvents - m_startIndex;
		const Uint32 numCopy = *numElems > numUnread ? numUnread: *numElems;
		*numElems = numCopy;

		::Red::System::MemoryCopy( elems, m_readBuf + m_startIndex, numCopy * sizeof( m_readBuf[0] ) );
		m_startIndex += numCopy;
	}

private:
	Bool IsFull_NoSync() const
	{
		return m_numEvents == READBUFSIZE;
	}

	void Reset_NoSync()
	{
		m_numEvents = 0;
		m_startIndex = 0;
		::Red::System::MemoryZero( m_readBuf, sizeof( m_readBuf ) );
	}
};

struct SRawInputGamepadReading
{
public:
	::Red::System::Bool IsValid;

	::Red::System::Bool IsAPressed;
	::Red::System::Bool IsBPressed;
	::Red::System::Bool IsXPressed;
	::Red::System::Bool IsYPressed;

	::Red::System::Bool IsMenuPressed;
	::Red::System::Bool IsViewPressed;

	::Red::System::Bool IsDPadUpPressed;
	::Red::System::Bool IsDPadDownPressed;
	::Red::System::Bool IsDPadLeftPressed;
	::Red::System::Bool IsDPadRightPressed;

	::Red::System::Bool IsLeftShoulderPressed;
	::Red::System::Bool IsRightShoulderPressed;

	::Red::System::Bool IsLeftThumbstickPressed;
	::Red::System::Bool IsRightThumbstickPressed;

	::Red::System::Float LeftThumbstickX;
	::Red::System::Float LeftThumbstickY;
	::Red::System::Float RightThumbstickX;
	::Red::System::Float RightThumbstickY;

	::Red::System::Float LeftTrigger;
	::Red::System::Float RightTrigger;

	// 100ns since Jan 1, 1601...
	::Red::System::Uint64 Timestamp;
};