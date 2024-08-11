/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behaviorGraphOutput.h"

RED_INLINE CPoseHandle::CPoseHandle()
	:	m_pose( nullptr ),
		m_owner( nullptr )
{}


RED_INLINE CPoseHandle::CPoseHandle( const CPoseHandle & copyFrom )
	:	m_pose( copyFrom.m_pose ),
		m_owner( copyFrom.m_owner )
{
	if( m_pose )
	{
		m_pose->AddRef();
	}
}

// ctremblay circular dependency hack. This function is called quite a lot. need to be inline ....
RED_INLINE CPoseHandle::~CPoseHandle()
{
	if( m_pose && m_pose->Release() == 0 )
	{
		if( m_owner )
		{
			SignalPoseAvailable();
		}
		else
		{
			delete m_pose;
		}
	}
}

RED_INLINE CPoseHandle::CPoseHandle( CPoseHandle && rvalue )
	:	m_pose( rvalue.m_pose ),
		m_owner( rvalue.m_owner )
{
	rvalue.m_pose = nullptr;
	rvalue.m_owner = nullptr;
}


RED_INLINE CPoseHandle::CPoseHandle( SBehaviorGraphOutput * pose, CPoseBlock * owner )
	:	m_pose( pose ),
		m_owner( owner )
{}

RED_INLINE SBehaviorGraphOutput * CPoseHandle::Get() const
{
	return m_pose;
}

RED_INLINE SBehaviorGraphOutput * CPoseHandle::operator->() const
{
	RED_FATAL_ASSERT( m_pose, "null pointer access is illegal!" );
	return m_pose;
}

RED_INLINE SBehaviorGraphOutput & CPoseHandle::operator*() const
{
	RED_FATAL_ASSERT( m_pose, "null pointer access is illegal!" );
	return *m_pose;
}

RED_INLINE void CPoseHandle::Reset()
{
	CPoseHandle().Swap( *this );
}

RED_INLINE void CPoseHandle::Swap( CPoseHandle & swapWith )
{
	::Swap( m_pose, swapWith.m_pose );
	::Swap( m_owner, swapWith.m_owner );
}

RED_INLINE CPoseHandle & CPoseHandle::operator=( const CPoseHandle & copyFrom )
{
	CPoseHandle( copyFrom ).Swap( *this );
	return *this;
}

RED_INLINE CPoseHandle & CPoseHandle::operator=( CPoseHandle && rvalue )
{
	CPoseHandle( std::move( rvalue ) ).Swap( *this );
	return *this;
}

RED_INLINE CPoseHandle::operator CPoseHandle::bool_operator() const
{
	return m_pose != nullptr ? &BoolConversion::valid : 0;
}

RED_INLINE bool CPoseHandle::operator!() const
{
	return !m_pose;
}

RED_INLINE bool operator==( const CPoseHandle & leftPtr, const CPoseHandle & rightPtr )
{
	return leftPtr.Get() == rightPtr.Get();
}

RED_INLINE bool operator!=( const CPoseHandle & leftPtr, const CPoseHandle & rightPtr )
{
	return leftPtr.Get() != rightPtr.Get();
}

RED_INLINE bool operator<( const CPoseHandle & leftPtr, const CPoseHandle & rightPtr )
{
	return leftPtr.Get() < rightPtr.Get();
}
