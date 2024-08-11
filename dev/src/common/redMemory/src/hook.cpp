/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hook.h"

namespace red
{
namespace memory
{
	Hook::Hook()
		:	m_preCallback( nullptr ),
			m_postCallback( nullptr ),
			m_userData( nullptr ),
			m_nextHook( nullptr )
	{}

	Hook::~Hook()
	{}

	void Hook::Initialize( const HookCreationParameter & param )
	{
		m_preCallback = param.preCallback;
		m_postCallback = param.postCallback;
		m_userData = param.userData;
	}

	void Hook::PreAllocation( HookPreParameter & param )
	{
		if( m_preCallback )
		{
			m_preCallback( param, m_userData );
		}

		if( m_nextHook )
		{
			m_nextHook->PreAllocation( param );
		}
	}

	void Hook::PostAllocation( HookPostParameter & param )
	{
		if( m_postCallback )
		{
			m_postCallback( param, m_userData );
		}

		if( m_nextHook )
		{
			m_nextHook->PostAllocation( param );
		}
	}

	Hook * Hook::GetNext() const
	{
		return m_nextHook;
	}

	void Hook::SetNext( Hook * hook )
	{
		m_nextHook = hook;
	}

	const void * Hook::InternalGetUserData() const
	{
		return m_userData;
	}

	const HookPostCallback Hook::InternalGetPostCallback() const
	{
		return m_postCallback;
	}

	bool operator==( const Hook & left, const Hook & right )
	{
		return left.InternalGetPostCallback() == right.InternalGetPostCallback() 
			&& left.InternalGetUserData() == right.InternalGetUserData()
			&& left.GetNext() == right.GetNext();
	}
}
}
