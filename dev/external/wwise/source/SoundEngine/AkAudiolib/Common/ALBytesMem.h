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
// ALBytesMem.h
//
// IReadBytes / IWriteBytes implementation on a growing memory buffer.
// This version uses the AudioLib memory pools.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/IBytes.h>
#include "AkPrivateTypes.h"

namespace AK
{
	class ALWriteBytesMem
		: public AK::IWriteBytes
	{
	public:
	
		ALWriteBytesMem();
		virtual ~ALWriteBytesMem();
	
		// IWriteBytes implementation
	
		virtual bool WriteBytes( const void * in_pData, AkInt32 in_cBytes, AkInt32 & out_cWritten );
	
		// Public methods
	
		bool		Reserve( AkInt32 in_cBytes );
		inline AkInt32 Count() const { return m_cPos; }
		inline void SetCount( AkInt32 in_cBytes ) { m_cPos = in_cBytes; }

		inline AkUInt8 * Bytes() const { return m_pBytes; }
		AkUInt8 *	Detach();
	
		inline void Clear() { m_cPos = 0; }
	
		static inline void SetMemPool( AkMemPoolId in_pool ) { s_pool = in_pool; }
		static inline AkMemPoolId GetMemPool() { return s_pool; }
	
	private:
		AkInt32		m_cBytes;
		AkUInt8 *	m_pBytes;
	
		AkInt32	m_cPos;
	
		static AkMemPoolId s_pool;
	};
}
