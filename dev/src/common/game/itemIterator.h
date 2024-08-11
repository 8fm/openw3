
#pragma once

#include "itemEntity.h"
#include "inventoryComponent.h"
#include "actor.h"

template< class T >
class EntityWithItemsComponentIterator
{
	ComponentIterator< T >						m_iterator;
	Int32										m_itemIndex;
	const TDynArray< SInventoryItem, MC_Inventory >* m_items;

	const TList< IAttachment* >*				m_attachmentsEnt;
	TList< IAttachment* >::const_iterator		m_atEntIter;

	const TList< IAttachment* >*				m_attachmentsAnim;
	TList< IAttachment* >::const_iterator		m_atAnimIter;

public:
	EntityWithItemsComponentIterator( const CActor* actor )
		: m_items				( nullptr )
		, m_itemIndex			( -1 )
		, m_iterator			( actor )
		, m_attachmentsEnt		( nullptr )
		, m_attachmentsAnim		( nullptr )
	{
		if ( const CInventoryComponent* inv = actor->GetInventoryComponent() )
		{
			m_items = &(inv->GetItems());
		}

		if ( !m_iterator )
		{
			Next();
		}
	}

	RED_INLINE operator Bool() const
	{
		return m_iterator;
	}

	RED_INLINE void operator++ ()
	{
		Next();
	}

	RED_INLINE T* operator*()
	{
		return *m_iterator;
	}

private:
	void MarkEnd()
	{
		m_items = nullptr;
	}

	Bool IsEnd() const
	{
		return m_items == nullptr;
	}



	void IterateOverAttachments( const TList< IAttachment* >*& list, TList< IAttachment* >::const_iterator& iter )
	{
		if ( !m_iterator && list )
		{
			for ( ; iter != list->End(); ++iter )
			{
				if( CHardAttachment* ha = Cast< CHardAttachment >( *iter ) )
				{
					if( CEntity* ent = Cast< CEntity >( ha->GetChild() ) )
					{
						InitComponentIterator( ent );
						++iter;
						break;
					}
				}
			}

			if ( iter == list->End() )
			{
				list = nullptr;
			}
		}
	}
		

	void Next()
	{
		if ( m_iterator )
		{
			++m_iterator;
		}
	
		IterateOverAttachments( m_attachmentsEnt, m_atEntIter );
		IterateOverAttachments( m_attachmentsAnim, m_atAnimIter );

		if ( !m_iterator )
		{
			GoToNextItem();

			if ( !IsEnd() && !m_iterator )
			{
				Next();
			}
		}
	}

	void GoToNextItem()
	{
		if ( !m_items )
		{
			return;
		}

		m_itemIndex++;

		if ( m_itemIndex >= m_items->SizeInt() )
		{
			MarkEnd();
		}
		else
		{
			const SInventoryItem& item = (*m_items)[ m_itemIndex ];			

			if ( CEntity* itemEnt = item.GetItemEntity() )
			{
				m_attachmentsEnt = &itemEnt->GetChildAttachments();
				m_atEntIter = m_attachmentsEnt->Begin();

				if ( itemEnt->GetRootAnimatedComponent() )
				{
					m_attachmentsAnim = &itemEnt->GetRootAnimatedComponent()->GetChildAttachments();					
					m_atAnimIter = m_attachmentsAnim->Begin();
				}

				InitComponentIterator( itemEnt );
			}
			else
			{
				GoToNextItem();
			}
		}
	}

	void InitComponentIterator( CEntity* e )
	{
		new ( &m_iterator ) ComponentIterator< T >( e );
	}
};
