
#pragma once

#include "kdTreeTypes.h"

template< class TTree >
class kdTreeNnPool
{
	struct ItemNode
	{
		typename TTree::Dist	m_key;
		Int32						m_info;
	};

	Int32			m_maxKeys;
	Int32			m_usedKeys;
	ItemNode*	m_items;			// Sorted! array of keys

public:
	kdTreeNnPool< TTree >( Int32 max )
		: m_maxKeys( max )
		, m_usedKeys( 0 )
	{
		m_items = new ItemNode[ m_maxKeys+1 ];
	}

	~kdTreeNnPool()
	{ 
		delete [] m_items;
	}

	RED_INLINE typename TTree::Dist MinKey() const
	{ 
		return ( m_usedKeys > 0 ? m_items[0].m_key : FLT_MAX); 
	}

	RED_INLINE typename TTree::Dist MaxKey() const
	{ 
		return ( m_usedKeys == m_maxKeys ? m_items[m_maxKeys-1].m_key : FLT_MAX ); 
	}

	RED_INLINE typename TTree::Dist IthSmallestKey( Int32 i ) const
	{ 
		return ( i < m_usedKeys ? m_items[i].m_key : FLT_MAX ); 
	}

	RED_INLINE Int32 IthSmallestInfo( Int32 i ) const
	{ 
		return ( i < m_usedKeys ? m_items[i].m_info : -1 ); 
	}

	RED_INLINE void Insert( typename TTree::Dist kv, Int32 inf )
	{
		Int32 i;

		for ( i=m_usedKeys; i>0; i-- ) 
		{
			if ( m_items[i-1].m_key > kv )
			{
				m_items[i] = m_items[i-1];
			}
			else
			{
				break;
			}
		}

		m_items[i].m_key = kv;
		m_items[i].m_info = inf;

		if ( m_usedKeys<m_maxKeys ) 
		{
			m_usedKeys++;
		}
	}
};

//////////////////////////////////////////////////////////////////////////

template< class TTree >
class kdTreeNnPrioQueue 
{
	struct ItemNode 
	{
		typename TTree::Dist				m_key;
		const typename TTree::kdTreeNode*	m_info;
	};

	Int32			m_maxKeys;
	Int32			m_usedKeys;
	ItemNode*	m_items;			// the priority queue! (array of nodes)

public:
	kdTreeNnPrioQueue( Int32 max )
		: m_usedKeys( 0 )
		, m_maxKeys( max )
	{
		m_items = new ItemNode[ m_maxKeys+1 ];
	}

	~kdTreeNnPrioQueue()
	{ 
		delete [] m_items; 
	}

	RED_INLINE Bool Empty()
	{ 
		return m_usedKeys == 0; 
	}

	RED_INLINE void Reset()
	{ 
		m_usedKeys = 0; 
	}

	RED_INLINE void Insert( typename TTree::Dist kv, const typename TTree::kdTreeNode* inf )
	{
		if (++m_usedKeys > m_maxKeys)
		{
			ASSERT( m_usedKeys < m_maxKeys );
			return;
		}

		Int32 r = m_usedKeys;

		while (r > 1) 
		{
			Int32 p = r/2;

			if ( m_items[p].m_key <= kv )
			{
				break;
			}

			m_items[r] = m_items[p];
			r = p;
		}

		m_items[r].m_key = kv;
		m_items[r].m_info = inf;
	}

	RED_INLINE void ExtractMin( typename TTree::Dist& kv, const typename TTree::kdTreeNode*& inf )
	{
		kv = m_items[1].m_key;
		inf = m_items[1].m_info;

		typename TTree::Dist kn = m_items[m_usedKeys--].m_key;

		int p = 1;
		int r = p<<1;

		while ( r <= m_usedKeys ) 
		{
			if ( r < m_usedKeys  && m_items[r].m_key > m_items[ r+1 ].m_key ) 
			{
				r++;
			}

			if (kn <= m_items[r].m_key)
			{
				break;
			}

			m_items[p] = m_items[r];
			p = r;
			r = p<<1;
		}

		m_items[p] = m_items[ m_usedKeys+1 ];
	}
};
