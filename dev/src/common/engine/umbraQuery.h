/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/pool.h"
#include "../core/refCountPointer.h"

#ifdef USE_UMBRA

class UmbraQuery
{
	friend class UmbraQueryBatch;
private:

	typedef Red::Threads::CAtomic< Int32 > RefCount;

	UmbraQuery*			m_next;
	Box					m_bbox;
	Bool				m_isProcessed;
	Bool				m_isOccluded;
	RefCount			m_refCount;

	static CPool		s_queryPool;
	static void*		s_queryMemory;

	void*				operator new( size_t size );
	void				operator delete( void* ptr );

	UmbraQuery( const Box& bbox, UmbraQuery* nextQuery )
		: m_next( nextQuery )
		, m_bbox( bbox )
		, m_isProcessed( false )
		, m_isOccluded( false )
		, m_refCount( 0 )													{}
	~UmbraQuery()															{}

public:
	

	static void			InitializeQueriesPool( Uint32 maxQueriesCount );
	static void			ShutdownQueriesPool();
	
	void				AddRef()											{ m_refCount.Increment(); }
	void				Release()											{ if ( m_refCount.Decrement() == 0 ) { delete this; } }

	void				SetResult( Bool occluded )							{ m_isProcessed = true; m_isOccluded = occluded; }

	const Box&			GetQueryBox() const									{ return m_bbox; }
	Bool				WasProcessed() const								{ return m_isProcessed; }
	Bool				IsOccluded() const 									{ return m_isOccluded; }
};

typedef TRefCountPointer< UmbraQuery > UmbraQueryPtr;

class UmbraQueryBatch : public Red::System::NonCopyable
{
protected:
	UmbraQuery*			m_queryList;
public:
	struct Iterator
	{
	private:
		UmbraQuery*		m_query;
	public:
		Iterator( const UmbraQueryBatch& batch )
			: m_query( batch.m_queryList )									{}

		operator		Bool() const										{ return m_query != nullptr; }
		UmbraQuery&		operator*()											{ return *m_query; }
		UmbraQuery*		operator->()										{ return m_query; }
		void			operator++()										{ m_query = m_query->m_next; }
	};

	UmbraQueryBatch()
		: m_queryList( nullptr )											{}
	UmbraQueryBatch( UmbraQueryBatch&& batch )
		: m_queryList( batch.m_queryList )									{ batch.m_queryList = nullptr; }
	~UmbraQueryBatch()														{ Clear(); }

	void				operator=( UmbraQueryBatch&& batch )				{ m_queryList = batch.m_queryList; batch.m_queryList = nullptr; }
	
	UmbraQueryPtr		AddQuery( const Box& bbox );
	Bool				IsEmpty() const										{ return m_queryList == nullptr; }
	void				Clear();
	void				CutBefore( Iterator it );

	
	
};

#endif // USE_UMBRA
