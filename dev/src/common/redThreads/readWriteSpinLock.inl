/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace Red
{
namespace Threads
{
	const Int32 c_rwSpinLockMaxSpinBeforeSleep = 32;

	RED_INLINE CRWSpinLock::CRWSpinLock()
	{
		AtomicOps::Exchange32( &m_lock, 0 );
	}

	RED_INLINE void CRWSpinLock::AcquireShared()
	{
		Int32 spinCount = 0;

		while( AtomicOps::Increment32( &m_lock ) & 0xffff0000 )
		{
			ReleaseShared();
			if( ++spinCount < c_rwSpinLockMaxSpinBeforeSleep )
			{
				YieldCurrentThread();
			}
			else
			{
				spinCount = 0;
				SleepOnCurrentThread( 1 );
			}
		}
	}

	RED_INLINE void CRWSpinLock::Acquire()
	{
		Int32 spinCount = 0;

		AtomicOps::Or32( &m_lock, 0x80000000 );
		while( ( AtomicOps::ExchangeAdd32( &m_lock, 0x00010000 ) & 0x7fffffff )  != 0 )
		{
			EndTryWrite();
			if( ++spinCount < c_rwSpinLockMaxSpinBeforeSleep )
			{
				YieldCurrentThread();
			}
			else
			{
				spinCount = 0;
				SleepOnCurrentThread( 1 );
			}
		}
	}

	RED_INLINE void CRWSpinLock::ReleaseShared()
	{
		AtomicOps::Decrement32( &m_lock );
	}

	RED_INLINE void CRWSpinLock::Release()
	{
		EndTryWrite(); 
		AtomicOps::And32( &m_lock, 0x7fffffff );
	}

	RED_INLINE void CRWSpinLock::EndTryWrite()
	{
		AtomicOps::ExchangeAdd32( &m_lock, -0x00010000 );
	}

}
}
