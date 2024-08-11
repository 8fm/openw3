/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/************************************************************************/
/* Simple Object iterator												*/
/************************************************************************/

/// Iterator to iterate over object list, thread safe (locks the object list)
class BaseObjectIterator
{
public:
	static const Uint32		DEFAULT_INCLUDE_FLAGS = ~0U;
	static const Uint32		DEFAULT_EXCLUDE_FLAGS = OF_DefaultObject | OF_Finalized | OF_Discarded;

	explicit BaseObjectIterator();
	explicit BaseObjectIterator( CClass* filterClass );
	explicit BaseObjectIterator( const Uint32 includeFlags, const Uint32 excludeFlags = DEFAULT_EXCLUDE_FLAGS );
	~BaseObjectIterator();

	//! Get object
	RED_INLINE CObject* operator *() { return m_currentObject; }

	//! Is object valid ?
	RED_INLINE operator Bool() const { return (NULL != m_currentObject); }

	//! Iterate to next object
	RED_INLINE void operator++() { FindNextObject(); }
	RED_INLINE void operator++(int) { FindNextObject(); }

private:
	Int32			m_index;
	const CClass*	m_filterClass;
	Uint32			m_includeFlags;
	Uint32			m_excludeFlags;

	CObject*		m_currentObject; // set in FindNextObject()

	BaseObjectIterator& operator =( const BaseObjectIterator& /*other*/ ); // no implementation (will cause linking error)

	void FindNextObject();
};

/************************************************************************/
/* Object iterator														*/
/************************************************************************/

template < class T >
class ObjectIterator : public BaseObjectIterator
{
public:
	//! Initialize filtering based on the template class
	RED_INLINE ObjectIterator()
		: BaseObjectIterator( ClassID< T >() )
	{};

	//! Get object
	RED_INLINE T* operator *()
	{
		return static_cast< T* >( BaseObjectIterator::operator *() );
	}
};