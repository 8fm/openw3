/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/inputUtils.inl"

#include "inputDeviceKeyboardDurangoDebug.h"

#if !defined( RED_FINAL_BUILD )

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardDurangoDebug
//////////////////////////////////////////////////////////////////////////
extern void DurangoHackGetKeyboardReading( SRawKeyboardEvent events[], Uint32* /*[inout]*/ numEvents );

CInputDeviceKeyboardDurangoDebug::CInputDeviceKeyboardDurangoDebug()
	: m_numEvents( 0 )
{
	Red::MemoryZero( m_rawKeyboardEvents, sizeof(m_rawKeyboardEvents) );
}

CInputDeviceKeyboardDurangoDebug::~CInputDeviceKeyboardDurangoDebug()
{
}

void CInputDeviceKeyboardDurangoDebug::Reset()
{
	// Do nothing
}

void CInputDeviceKeyboardDurangoDebug::Update( BufferedInput& outBufferedInput )
{
	m_numEvents = ARRAY_COUNT_U32( m_rawKeyboardEvents );
	::DurangoHackGetKeyboardReading(m_rawKeyboardEvents, &m_numEvents );

	for ( Uint32 i = 0; i < m_numEvents; ++i )
	{
		const SRawKeyboardEvent& evt = m_rawKeyboardEvents[ i ];
		if ( evt.m_down )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( static_cast< EInputKey >( evt.m_key ), IACT_Press, 1.0f ) );
		}
		else
		{
			outBufferedInput.PushBack( SBufferedInputEvent( static_cast< EInputKey >( evt.m_key ), IACT_Release, 0.f ) );
		}
	}
}

#endif // !defined( RED_FINAL_BUILD ) || defined( DEMO_BUILD )
