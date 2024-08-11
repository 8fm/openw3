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

//////////////////////////////////////////////////////////////////////
//
// AkBitArray.h
//
// AudioKinetic Lock base class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_BIT_ARRAY_H_
#define _AK_BIT_ARRAY_H_

// Allowing to set an array of max 64 bits
// this one is way more performant, and use only 64 bits of space, nothing virtual...
// Use with AkUInt16, AkUInt32 or AkUInt64

template <class T>
class CAkBitArray 
{
public:
	CAkBitArray()
		: m_iBitArray(0)
	{
	}

	void SetBit( T in_BitIndex )
	{
		AKASSERT( in_BitIndex < sizeof(T)*8 );
		m_iBitArray |= (1ULL << in_BitIndex);
	}

	void UnsetBit( T in_BitIndex )
	{
		AKASSERT( in_BitIndex < sizeof(T)*8 );
		m_iBitArray &= (~(1ULL << in_BitIndex));
	}

	void Set( T in_BitIndex, bool in_value )
	{
		if( in_value )
		{
			SetBit( in_BitIndex );
		}
		else
		{
			UnsetBit( in_BitIndex );
		}
	}

	bool IsSet( T in_BitIndex )
	{
		AKASSERT( in_BitIndex < sizeof(T)*8 );
		return ( m_iBitArray & (1ULL << in_BitIndex) )?true:false;
	}

	bool IsEmpty()
	{
		return (m_iBitArray == 0);
	}

	void ClearBits()
	{
		m_iBitArray = 0;
	}

private:
	T m_iBitArray;
};

#endif //_AK_BIT_ARRAY_H_
