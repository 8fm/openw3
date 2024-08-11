/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "renderProxyInterface.h"
#include "renderProxy.h"
#include "entity.h"

// Render Proxy Iterator
// This iterator allows you to traverse through all the IRenderProxy from IRenderProxyInterface object.
// IRenderProxies can be filtered by type ( RPT_None == all )
//
// for ( RenderProxyIterator it( obj ); it; ++it )
// {
//		IRenderProxy* proxy = *it;
//		//...
// }

class RenderProxyIterator
{
	IRenderProxyInterface*	m_obj;
	Uint32					m_it;
	Uint32					m_size;
	ERenderProxyType		m_proxyType;

public:
	RenderProxyIterator()
		: m_obj( NULL )
		, m_it ( 0 )
		, m_size( 0 )
		, m_proxyType( RPT_Mesh )
	{}

	RenderProxyIterator( IRenderProxyInterface* obj, ERenderProxyType proxyType = RPT_Mesh )
		: m_obj( obj )
		, m_it( 0 )
		, m_size( 0 )
		, m_proxyType( proxyType )
	{
		m_size = obj ? obj->GetNumberOfRenderProxies() : 0;

		FindFirst();
	}


	RED_INLINE operator Bool() const
	{
		return ( m_it < m_size ); 
	}

	RED_INLINE Bool operator!() const
	{
		return ( m_it >= m_size ); 
	}

	RED_INLINE void operator++()
	{
		if ( m_proxyType != RPT_None )
		{
			while ( 1 )
			{
				++m_it;

				if ( !FilterProxy() )
				{
					break;
				}
			}
		}
		else
		{
			++m_it;
		}
	}

	RED_INLINE IRenderProxy* operator*()
	{
		ASSERT( m_obj && m_obj->GetRenderProxy( m_it ) );
		return m_obj ? m_obj->GetRenderProxy( m_it ) : NULL;
	}

	RED_INLINE Bool Init( IRenderProxyInterface* inter, ERenderProxyType proxyType = RPT_Mesh )
	{
		m_obj = inter;
		m_it = 0;
		m_size = inter->GetNumberOfRenderProxies();
		m_proxyType = proxyType;

		FindFirst();

		return *this;
	}

private:
	RED_INLINE Bool FilterProxy()
	{
		if ( m_it >= m_size )
		{
			return false;
		}

		ASSERT( m_obj );

		IRenderProxy* proxy = m_obj ? m_obj->GetRenderProxy( m_it ) : NULL;
		if ( proxy && proxy->GetType() == m_proxyType )
		{
			return false;
		}

		return true;
	}

	RED_INLINE void FindFirst()
	{
		if ( m_proxyType != RPT_None && FilterProxy() )
		{
			// Next
			operator ++();
		}
	}
};


// Render Proxy Entity Iterator
// This iterator allows you to traverse through all IRenderProxies from entity.
// IRenderProxies can be filtered by type ( RPT_None == all )
//
// for ( RenderProxyEntityIterator it( entity ); it; ++it )
// {
//		IRenderProxy* proxy = *it;
// }

class RenderProxyEntityIterator
{
	const TDynArray< CComponent* >* m_components;
	Uint32							m_numOfComponents;
	ERenderProxyType				m_proxyType;
	Uint32							m_nextComponent;
	RenderProxyIterator				m_iterator;

public:
	RenderProxyEntityIterator( const CEntity* entity, ERenderProxyType proxyType = RPT_Mesh )
		: m_components( &entity->GetComponents() )
		, m_proxyType( proxyType )
		, m_nextComponent( 0 )
	{
		m_numOfComponents = m_components->Size();
		Next();
	}

	RED_INLINE operator Bool() const
	{
		return m_iterator;
	}

	RED_INLINE void operator++()
	{
		Next();
	}

	RED_INLINE IRenderProxy* operator*()
	{
		return *m_iterator;
	}

private:
	void Next()
	{
		if ( m_iterator )
		{
			// Next render proxy
			++m_iterator;
		}

		// If there is no proxy, advance the component iterator.
		if ( !m_iterator )
		{
			while ( m_nextComponent < m_numOfComponents )
			{
				IRenderProxyInterface* proxyInter = (*m_components)[ m_nextComponent ]->QueryRenderProxyInterface();
				++m_nextComponent;

				if ( proxyInter && proxyInter->GetNumberOfRenderProxies() > 0 )
				{
					// Initialize proxy iterator for this component. If Init() succeeds, we can return, we've reached
					// the next. If it fails, then none of the proxies matched the type we're looking for, and we need
					// to keep looking in the next component.
					if ( m_iterator.Init( proxyInter, m_proxyType ) )
					{
						return;
					}
				}
			}
		}
	}
};

// Render Proxy Entity Hierarchical Iterator

class RenderProxyEntityHierarchicalIterator
{
	TDynArray< CComponent* >		m_components;
	Uint32							m_numOfComponents;
	ERenderProxyType				m_proxyType;
	Uint32							m_nextComponent;
	RenderProxyIterator				m_iterator;

public:
	RenderProxyEntityHierarchicalIterator( const CEntity* entity, ERenderProxyType proxyType = RPT_Mesh )
		: m_proxyType( proxyType )
		, m_nextComponent( 0 )
	{
		CollectComponents( entity );

		m_numOfComponents = m_components.Size();

		Next();
	}

	RED_INLINE operator Bool() const
	{
		return m_iterator;
	}

	RED_INLINE void operator++()
	{
		Next();
	}

	RED_INLINE IRenderProxy* operator*()
	{
		return *m_iterator;
	}

private:
	void Next()
	{
		if ( m_iterator )
		{
			// Next render proxy
			++m_iterator;
		}

		// If there is no proxy, advance the component iterator.
		if ( !m_iterator )
		{
			while ( m_nextComponent < m_numOfComponents )
			{
				IRenderProxyInterface* proxyInter = m_components[ m_nextComponent ]->QueryRenderProxyInterface();
				++m_nextComponent;

				if ( proxyInter && proxyInter->GetNumberOfRenderProxies() > 0 )
				{
					// Initialize proxy iterator for this component. If Init() succeeds, we can return, we've reached
					// the next. If it fails, then none of the proxies matched the type we're looking for, and we need
					// to keep looking in the next component.
					if ( m_iterator.Init( proxyInter, m_proxyType ) )
					{
						return;
					}
				}
			}
		}
	}

	void CollectComponents( const CEntity* entity )
	{
		const TDynArray< CComponent* >& comps = entity->GetComponents();
		for ( Uint32 i=0; i<comps.Size(); ++i )
		{
			CComponent* comp = comps[ i ];

			// Roots
			if ( comp && comp->GetParentAttachments().Size() == 0 )
			{
				m_components.PushBack( comp );

				comp->GetAllChildAttachedComponents( m_components );
			}
		}
	}
};

typedef RenderProxyEntityHierarchicalIterator RenderProxyActorWithItemsIterator;