/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "component.h"

///////////////////////////////////////////////////////////////////////////

/// Component iterator
class BaseComponentIterator
{
private:
	const CEntity*						m_entity;				//!< Entity being iterated
	const TDynArray< CComponent* >&		m_components;			//!< Components table
	Int32								m_componentIndex;		//!< Index of component
	CClass*								m_class;				//!< Filter class

public:
	//! Constructor
	BaseComponentIterator( const CEntity* entity, CClass* filterClass = ClassID< CComponent >() );
	
	//! Copy constructor
	BaseComponentIterator( const BaseComponentIterator& other );

	//! Is current component valid
	RED_INLINE operator Bool () const
	{
		return IsValid();
	}

	//! Advance to next
	RED_INLINE void operator++ ()
	{
		Next();
	}

	//! Get current
	RED_INLINE CComponent* operator*()
	{
		ASSERT( m_componentIndex >= 0 && m_componentIndex < (Int32)m_components.Size() );
		return m_components[ m_componentIndex ];
	}

protected:
	//! Is the iterator valid ?
	RED_INLINE Bool IsValid() const{ return ( m_componentIndex >= 0 ) && ( m_componentIndex < (Int32)m_components.Size() ); }

	//! Advance to next component
	void Next()
	{
		while ( ++m_componentIndex < (Int32)m_components.Size() )
		{
			if ( m_components[ m_componentIndex ] && m_components[ m_componentIndex ]->IsA( m_class ) )
			{
				break;
			}
		}
	}



private:
	//! Assignment is illegal
	RED_INLINE BaseComponentIterator& operator=( const BaseComponentIterator& /*other*/ )
	{
		return *this;
	}
};

///////////////////////////////////////////////////////////////////////////

/// Iterate over entity components of given class
template< class T >
class ComponentIterator : private BaseComponentIterator
{
public:
	//! Constructor
	RED_INLINE ComponentIterator( const CEntity* entity )
		: BaseComponentIterator( entity, ClassID< T >() )
	{};

	//! Copy constructor
	RED_INLINE ComponentIterator( const ComponentIterator& other )
		: BaseComponentIterator( other )
	{};

	//! Is current component valid
	RED_INLINE operator Bool() const
	{
		return BaseComponentIterator::IsValid();
	}

	//! Advance to next
	RED_INLINE void operator++ ()
	{
		BaseComponentIterator::Next();
	}

	//! Get current
	RED_INLINE T* operator*()
	{
		return static_cast< T* >( BaseComponentIterator::operator *() );
	}

private:
	//! Assignment is illegal
	RED_INLINE ComponentIterator& operator=( const ComponentIterator& other )
	{
		return *this;
	}
};

///////////////////////////////////////////////////////////////////////////
