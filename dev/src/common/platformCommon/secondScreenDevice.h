/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if !defined( NO_SECOND_SCREEN )

#include "../../common/core/types.h"
#include "../../common/core/coreInternal.h"
#include "../../common/core/dynarray.h"
class ISecondScreenDeviceDelegate;
class CSecondScreenDeviceWatcher;

//! CSecondScreenDevice class represents second screen device unified for all platforms 
//! Platform dependent implementation of CSecondScreenDevice:
//! Xbox One: CSecondScreenDeviceDurango
//! PC		: CSecondScreenDevicePC
class CSecondScreenDevice
{
public:
	CSecondScreenDevice( ISecondScreenDeviceDelegate* _delegate )
	{ 
		m_delegate = _delegate;
		m_sendConnectionEstablish = true;
	}
	virtual ~CSecondScreenDevice()
	{

	}

	Bool SendConnectionEstablish() const { return m_sendConnectionEstablish; }
	void ConnectionEstablishSent() { m_sendConnectionEstablish = false; }
	void SendMessage( const Char* message ){ m_messagesToSend.PushBack( message );	}

	void Tick( Float timeDelta )
	{
#ifdef SECOND_SCREEN_PROFILE_CODE
		PC_SCOPE( SS_Device_Tick);
#endif
		while( (m_messagesToSend.Begin() != m_messagesToSend.End()) )
		{
			Bool eraseMessage = false;
			if( m_messagesToSend.Begin()->GetLength() > s_maxMessageLength )
			{
				RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Message which you try to send is to big: %i." ), m_messagesToSend.Begin()->GetLength() );
				eraseMessage = true;
			}
			else if( m_messagesToSend.Begin()->GetLength() == 0 )
			{
				RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Message which you try to send is empty." ) );
				eraseMessage = true;
			}
			else if( SendMessageAsync( m_messagesToSend.Begin()->AsChar() ) )
			{
				eraseMessage = true;
			}
			if( eraseMessage )
			{
				m_messagesToSend.Erase( m_messagesToSend.Begin() );				
			}
			else
			{
				break;
			}
		}
	}


	virtual bool operator==( const CSecondScreenDevice& rhs ) = 0;

public:
	//! implemented per platform
	static const size_t s_maxMessageLength; 

protected:

	// return false if message must be resend
	virtual Bool SendMessageAsync( const Char* message ) const = 0;

protected:
	ISecondScreenDeviceDelegate*	m_delegate;
	TDynArray<String>				m_messagesToSend;
	Bool							m_sendConnectionEstablish;
};

class ISecondScreenDeviceDelegate
{
public:
	virtual void OnMessageReceived( const CSecondScreenDevice& device, const Char* message ) = 0;
	virtual void OnDeviceAdded( CSecondScreenDeviceWatcher* sender, CSecondScreenDevice* device ) = 0;
	virtual void OnDeviceRemoved( CSecondScreenDeviceWatcher* sender, const CSecondScreenDevice& device) = 0;
};

#endif