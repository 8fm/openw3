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

////////////////////////////////////////////////////////////////////////////////
//
// ALBytesMem.cpp
//
// IReadBytes / IWriteBytes implementation on a memory buffer.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <AK/Tools/Common/AkObject.h>
#include "ALBytesMem.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

namespace AK
{

AkMemPoolId ALWriteBytesMem::s_pool = AK_INVALID_POOL_ID;

//------------------------------------------------------------------------------
// ALWriteBytesMem

ALWriteBytesMem::ALWriteBytesMem()
	: m_cBytes( 0 )
	, m_pBytes( NULL )
	, m_cPos( 0 )
{
	AKASSERT( s_pool != AK_INVALID_POOL_ID );
}

ALWriteBytesMem::~ALWriteBytesMem()
{
	if ( m_pBytes )
		AkFree( s_pool, m_pBytes );
}

bool ALWriteBytesMem::WriteBytes( const void * in_pData, AkInt32 in_cBytes, AkInt32& out_cWritten )
{
	if( Reserve( m_cPos + in_cBytes ) )
	{
		AKPLATFORM::AkMemCpy( m_pBytes + m_cPos, (void *)in_pData, in_cBytes );

		m_cPos += in_cBytes;
		out_cWritten = in_cBytes;

		return true;
	}
	else
		return false;
}

bool ALWriteBytesMem::Reserve( AkInt32 in_cBytes )
{
	static const AkInt kGrowBy = 1024;

	if ( m_cBytes < in_cBytes )
	{
		AkInt32 cBytesOld = m_cBytes;

		AkInt cGrowBlocks = ( in_cBytes + kGrowBy-1 ) / kGrowBy;
		m_cBytes = cGrowBlocks * kGrowBy;

		if ( m_pBytes )
		{
			AkUInt8 * m_pBytesOld = m_pBytes;

			m_pBytes = (AkUInt8 *) AkAlloc( s_pool, m_cBytes );

			if( m_pBytes )
			{
				AKPLATFORM::AkMemCpy( m_pBytes, m_pBytesOld, cBytesOld );
				AkFree( s_pool, m_pBytesOld );
			}
			else
			{
				// reverting changes and returning false.
				m_pBytes = m_pBytesOld;
				m_cBytes = cBytesOld;
				return false;
			}
		}
		else
		{
			m_pBytes = (AkUInt8 *) AkAlloc( s_pool, m_cBytes );
			if( !m_pBytes )
			{
				// reverting changes and returning false.
				m_cBytes = cBytesOld;
				return false;
			}
		}
	}

	return true;
}

AkUInt8 * ALWriteBytesMem::Detach()
{
    AkUInt8* pByte = m_pBytes;

    m_pBytes = NULL;
    m_cBytes = 0;
    m_cPos = 0;

    return pByte;
}

}
