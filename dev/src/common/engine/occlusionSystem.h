/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "umbraQuery.h"

#ifdef USE_UMBRA

struct COcclusionQueryPtr 
{
	DECLARE_RTTI_STRUCT( COcclusionQueryPtr );
	UmbraQueryPtr			m_query;

	COcclusionQueryPtr() {}
	COcclusionQueryPtr( UmbraQueryPtr&& query );
	COcclusionQueryPtr( COcclusionQueryPtr&& ptr );
	COcclusionQueryPtr( const COcclusionQueryPtr& ptr );
	
	RED_INLINE operator Bool() const						{ return m_query; }
	RED_INLINE operator UmbraQueryPtr() const				{ return m_query; }
	RED_INLINE const UmbraQueryPtr& operator->() const	{ return m_query; }
	RED_INLINE COcclusionQueryPtr& operator=( COcclusionQueryPtr&& ptr )
	{
		m_query = Move( ptr.m_query );
		return *this;
	}
	RED_INLINE COcclusionQueryPtr& operator=( const COcclusionQueryPtr& ptr )
	{
		m_query = ptr.m_query;
		return *this;
	}
};

BEGIN_NODEFAULT_CLASS_RTTI( COcclusionQueryPtr )
END_CLASS_RTTI()

class COcclusionSystem
{
	
protected:
	UmbraQueryBatch							m_pendingQueue;
public:
	COcclusionSystem();
	void				Init();
	void				Deinit();

	COcclusionQueryPtr	AddQuery( const Box& bbox );
	void				Tick();
};

typedef TSingleton< COcclusionSystem > SOcclusionSystem;

#endif