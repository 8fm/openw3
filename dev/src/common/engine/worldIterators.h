
#pragma once

#include "component.h"

#include "world.h"
#include "layer.h"

class WorldAttachedComponentsIterator
{
	CComponent*					m_cur;

public:
	WorldAttachedComponentsIterator( const CWorld* world )
		: m_cur( world->m_allAttachedComponents )
	{

	}

	RED_INLINE operator Bool () const
	{
		return m_cur != NULL;
	}

	RED_INLINE void operator++ ()
	{
		m_cur = m_cur->GetNextAttachedComponent();
	}

	RED_INLINE CComponent* operator*()
	{
		ASSERT( m_cur );
		return m_cur;
	}
};

//////////////////////////////////////////////////////////////////////////
class WorldAttachedLayerIterator
{
	friend class WorldAttachedEntitiesIterator;

	THashSet< CLayer* >::iterator m_it;
	THashSet< CLayer* >::iterator m_end;

public:
	WorldAttachedLayerIterator( CWorld* world )
		: m_it( world->m_attachedLayers.Begin() )
		, m_end( world->m_attachedLayers.End() )
	{

	}

	RED_INLINE operator Bool () const
	{
		return m_it != m_end;
	}

	RED_INLINE void operator++ ()
	{
		++m_it;
	}

	RED_INLINE CLayer* operator*()
	{
		return *m_it;
	}
};

//////////////////////////////////////////////////////////////////////////

class WorldAttachedEntitiesIterator
{
	WorldAttachedLayerIterator		m_layerIt;
	Uint32							m_currEntity;			

public:
	WorldAttachedEntitiesIterator( CWorld* world )
		: m_layerIt( world )
		, m_currEntity( 0 )
	{
		if ( IsCurrLayerEmpty() )
		{
			Next();
		}
	}

	RED_INLINE operator Bool ()
	{
		if ( m_layerIt )
		{
			return m_currEntity < (*m_layerIt)->GetEntities().Size();
		}

		return false;
	}

	RED_INLINE void operator++ ()
	{
		Next();
	}

	RED_INLINE CEntity* operator*()
	{
		ASSERT( m_layerIt );
		ASSERT( *m_layerIt );
		ASSERT( (*m_layerIt)->GetEntities().Size() > m_currEntity );

		return ((*m_layerIt)->GetEntities())[ m_currEntity ];
	}

private:
	RED_INLINE Bool IsCurrLayerEmpty()
	{
		return m_layerIt && (*m_layerIt)->GetEntities().Size() == 0;
	}

	RED_INLINE void Next()
	{
		if ( m_layerIt )
		{
			ASSERT( m_layerIt );
			ASSERT( *m_layerIt );

			const Uint32 entNum = (*m_layerIt)->GetEntities().Size();

			if ( entNum > 0 && m_currEntity < entNum - 1 )
			{
				++m_currEntity;
			}
			else
			{
				++m_layerIt;
				m_currEntity = 0;

				if ( IsCurrLayerEmpty() )
				{
					Next();
				}
			}
		}
	}
};

// This function should be removed and replaced by an iterator class
template< class T >
void CWorld::GetAttachedComponentsOfClass( TDynArray< T* >& components ) const
{
	for ( CComponent* cur = m_allAttachedComponents; cur; cur=cur->GetNextAttachedComponent() )
	{
		if ( cur->IsA< T >() )
		{
			components.PushBack( Cast< T > ( cur ) );
		}
	}
}