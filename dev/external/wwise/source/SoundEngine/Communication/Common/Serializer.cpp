/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


#include "stdafx.h"

#include "Serializer.h"
#include "AkEndianByteSwap.h"

#include <string.h>

#ifdef DEBUG_NEW
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif

Serializer::Serializer( bool in_bSwapEndian )
	: m_pReadBytes( NULL )
	, m_readPos( 0 )
	, m_readPosBeforePeeking( 0 )
	, m_bSwapEndian( in_bSwapEndian )
{
}

Serializer::~Serializer()
{
	m_writer.Clear();
}

void Serializer::Deserializing( const AkUInt8* in_pData )
{
	m_pReadBytes = in_pData;
	m_readPos = 0;
	m_readPosBeforePeeking = 0;
}

AKRESULT Serializer::Reserve(AkUInt32 in_size)
{
	return m_writer.Reserve(in_size) ? AK_Success : AK_InsufficientMemory;
}

const AkUInt8* Serializer::GetReadBytes() const
{
	return m_pReadBytes + m_readPos;
}

void Serializer::Reset()
{
	m_writer.Clear();
	Deserializing( NULL );
}

bool Serializer::Put( AkUInt8 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool Serializer::Get( AkUInt8& out_rValue )
{
	out_rValue = Swap( *(AkUInt8*)(m_pReadBytes + m_readPos) );
	m_readPos += 1;

	return true;
}

bool Serializer::Put( AkInt16 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool Serializer::Get( AkInt16& out_rValue )
{
	out_rValue = Swap( *(AkInt16*)(m_pReadBytes + m_readPos) );
	m_readPos += 2;

	return true;
}

bool Serializer::Put( AkInt32 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool Serializer::Get( AkInt32& out_rValue )
{
	out_rValue = Swap( *(AkInt32*)(m_pReadBytes + m_readPos) );
	m_readPos += 4;

	return true;
}

bool Serializer::Put( AkUInt16 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool Serializer::Get( AkUInt16& out_rValue )
{
	out_rValue = Swap( *(AkUInt16*)(m_pReadBytes + m_readPos) );
	m_readPos += 2;

	return true;
}

bool Serializer::Put( AkUInt32 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool Serializer::Get( AkUInt32& out_rValue )
{
	out_rValue = Swap( *(AkUInt32*)(m_pReadBytes + m_readPos) );
	m_readPos += 4;

	return true;
}

bool Serializer::Put( AkInt64 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool Serializer::Get( AkInt64& out_rValue )
{
	out_rValue = Swap( *(AkInt64*)(m_pReadBytes + m_readPos) );
	m_readPos += 8;

	return true;
}

bool Serializer::Put( AkReal32 in_value )
{
	AkInt32 lDummy = 0;

	AkReal32 valToWrite = Swap( in_value );
	
	return m_writer.WriteBytes( &valToWrite, 4, lDummy );
}

bool Serializer::Get( AkReal32& out_rValue )
{
	// Align the float for consoles like Xbox 360.
	AkInt32 temp = *(AkInt32*)(m_pReadBytes + m_readPos);
	out_rValue = Swap( *reinterpret_cast<AkReal32*>( &temp ) );
	m_readPos += 4;

/*	out_rValue = 0;
	::memcpy( &out_rValue, m_pReadBytes + m_readPos, 4 );
	m_readPos += 4;
*/
	return true;
}

bool Serializer::Put( const void* in_pvData, AkInt32 in_size )
{
	AkInt32 lDummy = 0;
	
	return Put( in_size )
		&& m_writer.WriteBytes( in_pvData, in_size, lDummy );
}

bool Serializer::Get( void*& out_rpData, AkInt32& out_rSize )
{
	bool bRet = false;
	
	if( Get( out_rSize ) && out_rSize != 0 )
	{
		out_rpData = (void*)(m_pReadBytes + m_readPos);

		m_readPos += out_rSize;

		bRet = true;
	}

	return bRet;
}

bool Serializer::Put( const char* in_pszData )
{
	return Put( (void*)in_pszData, in_pszData != NULL ? (AkInt32)(::strlen( in_pszData ) + 1) : 0 );
}

bool Serializer::Get( char*& out_rpszData, AkInt32& out_rSize )
{
	return Get( (void*&)out_rpszData, out_rSize );
}

void Serializer::SetDataPeeking( bool in_bPeekData )
{
	if( in_bPeekData )
		m_readPosBeforePeeking = m_readPos;
	else
		m_readPos = m_readPosBeforePeeking;
}

AkUInt8 Serializer::Swap( const AkUInt8& in_rValue ) const
{
	return in_rValue;
}

AkInt16 Serializer::Swap( const AkInt16& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::WordSwap( in_rValue ) : in_rValue;
}

AkUInt16 Serializer::Swap( const AkUInt16& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::WordSwap( in_rValue ) : in_rValue;
}

AkInt32 Serializer::Swap( const AkInt32& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::DWordSwap( in_rValue ) : in_rValue;
}

AkUInt32 Serializer::Swap( const AkUInt32& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::DWordSwap( in_rValue ) : in_rValue;
}

AkInt64 Serializer::Swap( const AkInt64& in_rValue ) const
{
    AkInt64 liSwapped;
    liSwapped = m_bSwapEndian ? AK::EndianByteSwap::Int64Swap( in_rValue ) : in_rValue;
	return liSwapped;
}

AkReal32 Serializer::Swap( const AkReal32& in_rValue ) const
{
	AkReal32 retVal = in_rValue;
	
	if( m_bSwapEndian )
		AK::EndianByteSwap::Swap( (AkUInt8*)&in_rValue, 4, (AkUInt8*)&retVal );
	
	return retVal;
}
