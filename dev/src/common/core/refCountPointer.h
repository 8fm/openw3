/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_REF_COUNT_PTR_H_
#define _CORE_REF_COUNT_PTR_H_

template < class T >
class TRefCountPointer
{
protected:
	T*								m_ptr;
public:
	TRefCountPointer()
		: m_ptr( NULL )																			{}
	TRefCountPointer( T* ptr )
		: m_ptr( ptr )																			{ if ( m_ptr ) { m_ptr->AddRef(); } }
	TRefCountPointer( const TRefCountPointer& p )
		: m_ptr( p.m_ptr )																		{ if ( m_ptr ) { m_ptr->AddRef(); } }
	TRefCountPointer( TRefCountPointer&& p )
		: m_ptr( p.m_ptr )																		{ p.m_ptr = NULL; }
	~TRefCountPointer()																			{ if ( m_ptr ) { m_ptr->Release(); } }

	TRefCountPointer& operator=( const TRefCountPointer& ptr )
	{
		if ( ptr.m_ptr != m_ptr )
		{
			if ( m_ptr )
			{
				m_ptr->Release();
			}
			m_ptr = ptr.m_ptr;
			if ( m_ptr )
			{
				m_ptr->AddRef();
			}
		}
		return *this;
	}

	TRefCountPointer& operator=( TRefCountPointer&& ptr )
	{
		if ( ptr.m_ptr != m_ptr )
		{
			if ( m_ptr )
			{
				m_ptr->Release();
			}
			m_ptr = ptr.m_ptr;
			ptr.m_ptr = NULL;
		}
		return *this;
	}

	RED_FORCE_INLINE void Clear()
	{
		if ( m_ptr )
		{
			m_ptr->Release();
			m_ptr = NULL;
		}
	}

	RED_FORCE_INLINE void Assign( T* p )
	{
		if ( p != m_ptr )
		{
			if ( m_ptr )
			{
				m_ptr->Release();
			}
			m_ptr = p;
			if ( m_ptr )
			{
				m_ptr->AddRef();
			}
		}
	}

	RED_FORCE_INLINE T* Get() const
	{
		return m_ptr;
	}

	RED_INLINE operator Bool() const														{ return m_ptr != nullptr; }
	RED_INLINE T* operator->() const														{ return m_ptr; }
	RED_INLINE T& operator*() const														{ return *m_ptr; }

	RED_FORCE_INLINE Bool operator<( const TRefCountPointer& other ) const
	{
		return m_ptr < other->m_ptr;
	}

	RED_FORCE_INLINE Bool operator==( const TRefCountPointer& other ) const
	{
		return m_ptr == other->m_ptr;
	}

	// Evil: we should not have operator like this
	//RED_INLINE operator T* () const														{ return m_ptr; }
};



#endif

