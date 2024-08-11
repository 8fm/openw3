/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "rewards.h"

#include "../core/factory.h"

const SReward SReward::EMPTY;

IMPLEMENT_ENGINE_CLASS( SItemReward );
IMPLEMENT_ENGINE_CLASS( SReward );

IMPLEMENT_ENGINE_CLASS( CRewardGroup );

CRewardGroup::CRewardGroup()
	: m_isSubGroup( false )
{

}

SReward* CRewardGroup::CreateReward( const SReward* reward /*= nullptr*/ )
{
	if( MarkModified() )
	{
		SReward* rew = ::new (m_rewards) SReward( this, reward );
		return rew;
	}

	return NULL;
}

void	CRewardGroup::RemoveReward( SReward* reward)
{
	if( MarkModified() )
	{
		m_rewards.Erase(TDynArray< SReward >::iterator( reward ));
	}
}

Bool CRewardGroup::MoveReward( const CName& rewardToMove, CRewardGroup* toGroup, const CName& toPosition )
{
	// the same reward group
	if ( !toGroup )
	{
		Int32 rewardToMoveIndex = -1;
		Int32 toPositionIndex = -1;

		// look for both rewards in current group
		for ( Int32 i = 0; i < m_rewards.SizeInt(); i++ )
		{
			if ( m_rewards[ i ].m_name == rewardToMove )
			{
				rewardToMoveIndex = i;
			}
			if ( toPosition != CName::NONE )
			{
				if ( m_rewards[ i ].m_name == toPosition )
				{
					toPositionIndex = i;
				}
			}
		}
		if ( rewardToMoveIndex == -1 || toPositionIndex == -1 )
		{
			return false;
		}
		if ( rewardToMoveIndex == toPositionIndex )
		{
			// the same position, ignore
			return false;
		}
		if ( rewardToMoveIndex == toPositionIndex + 1 )
		{
			// trying to move to the same position, ignore
			return false;
		}
		if ( !MarkModified() )
		{
			return false;
		}

		// store reward
		SReward movedReward = m_rewards[ rewardToMoveIndex ];
		// remove from current position (no RemoveAtFast, we need to preserve order)
		m_rewards.RemoveAt( rewardToMoveIndex );
		// insert at next position than dropped one
		toPositionIndex++;
		// if current position is smaller than intended position, decrease index because of removed object
		if ( rewardToMoveIndex < toPositionIndex )
		{
			toPositionIndex--;
		}
		// and insert at new position
		m_rewards.Insert( toPositionIndex, movedReward );
	}
	// different reward group
	else
	{
		Int32 rewardToMoveIndex = -1;
		Int32 toPositionIndex = -1;

		// look for reward in current group
		for ( Int32 i = 0; i < m_rewards.SizeInt(); i++ )
		{
			if ( m_rewards[ i ].m_name == rewardToMove )
			{
				rewardToMoveIndex = i;
				break;
			}
		}
		// look for reward in destination group
		if ( toPosition != CName::NONE )
		{
			for ( Int32 i = 0; i < toGroup->m_rewards.SizeInt(); i++ )
			{
				if ( toGroup->m_rewards[ i ].m_name == toPosition )
				{
					toPositionIndex = i;
					// insert at next position than dropped one
					toPositionIndex++;
					break;
				}
			}
		}
		else
		{
			toPositionIndex = toGroup->m_rewards.Size();
		}

		if ( rewardToMoveIndex == -1 || toPositionIndex == -1 )
		{
			return false;
		}
		if ( !MarkModified() || !toGroup->MarkModified() )
		{
			return false;
		}

		// store reward
		SReward movedReward = m_rewards[ rewardToMoveIndex ];
		// remove from current position (no RemoveAtFast, we need to preserve order)
		m_rewards.RemoveAt( rewardToMoveIndex );

		// and insert at new position in different group
		toGroup->m_rewards.Insert( toPositionIndex, movedReward );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

class CRewardGroupFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CRewardGroupFactory, IFactory, 0 );

public:
	CRewardGroupFactory()
	{
		m_resourceClass = ClassID< CRewardGroup >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CRewardGroupFactory );
PARENT_CLASS( IFactory );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CRewardGroupFactory );

CResource* CRewardGroupFactory::DoCreate( const FactoryOptions& options )
{
	CRewardGroup *rewGroup = ::CreateObject< CRewardGroup >( options.m_parentObject );
	return rewGroup;
}

//////////////////////////////////////////////////////////////////////////

Bool CRewardGroupIterator::IsValid() const
{
	return m_subGroupIter ? m_subGroupIter->IsValid() : m_groupIter != m_root.GetSubGroups().End();
}

void CRewardGroupIterator::Next()
{
	// First check if we are already at the end of the array.
	if( m_groupIter != m_root.GetSubGroups().End() )
	{
		// If recursive mode, we must go inside the sub groups
		if( m_recursive )
		{
			// If we already are inside a sub group, just increment it
			if( m_subGroupIter )
			{
				++(*m_subGroupIter);

				// If the new sub group is not valid, increment the main group iterator
				if( !*m_subGroupIter )
				{
					delete m_subGroupIter;
					m_subGroupIter = NULL;

					do
					{
						++m_groupIter;
					}
					while( m_groupIter != m_root.GetSubGroups().End() && (*m_groupIter).Get() == NULL );
				}
			}
			else
			{
				// Check if current group iterator consists of any subgroups
				const TDynArray< THandle< CRewardGroup > >& subGroups = (*m_groupIter)->GetSubGroups();
				if( subGroups.Size() > 0 )
				{
					// If yes, create a new sub group iterator
					m_subGroupIter = new CRewardGroupIterator( *(*m_groupIter).Get(), true );
					if( !*m_subGroupIter )
					{
						// If there are no valid sub groups, remove the iterator
						delete m_subGroupIter;
						m_subGroupIter = NULL;

						do
						{
							++m_groupIter;
						}
						while( m_groupIter != m_root.GetSubGroups().End() && (*m_groupIter).Get() == NULL );
					}
				}
				else
				{
					// If there are no sub groups for current iterator, just increment to next non-NULL
					do
					{
						++m_groupIter;
					}
					while( m_groupIter != m_root.GetSubGroups().End() && (*m_groupIter).Get() == NULL );
				}
			}
		}
		else
		{
			do
			{
				++m_groupIter;
			}
			while( m_groupIter != m_root.GetSubGroups().End() && (*m_groupIter).Get() == NULL );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CRewardIterator::IsValid() const
{
	return m_iterCurrent != m_iterEnd;
}

void CRewardIterator::Next()
{
	if( m_iterCurrent != m_iterEnd )
	{
		++m_iterCurrent;
	}

	if( m_iterCurrent == m_iterEnd )
	{
		// Do not increment iterators if we are in root group
		if( m_hackNeedToDoItFast )
		{
			m_hackNeedToDoItFast = false;
		}
		else if( m_isDirectory )
		{
			++*m_dirIter;
		}
		else
		{
			++*m_groupIter;
		}

		// Find first reward ( groups may be empty )
		if( m_isDirectory )
		{
			if( *m_dirIter )
			{
				FindFirstReward( *m_dirIter );
			}
		}
		else if( *m_groupIter )
		{
			FindFirstReward( *m_groupIter );
		}
	}
}

