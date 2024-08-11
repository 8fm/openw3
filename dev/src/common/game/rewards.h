/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/directory.h"

#define REWARDS_DIR				TXT("gameplay\\rewards\\")

class CRewardGroup;

struct SItemReward
{
	DECLARE_RTTI_STRUCT( SItemReward );

	SItemReward()
		: m_amount( 0 )
	{}

	CName	m_item;
	Int32	m_amount;
};

BEGIN_CLASS_RTTI( SItemReward );
	PROPERTY_CUSTOM_EDIT( m_item, TXT("Name of the rewarded item"), TXT("ChooseItem") );
	PROPERTY_EDIT( m_amount, TXT("Amount of specified items.") );
END_CLASS_RTTI();

struct SReward
{
	DECLARE_RTTI_STRUCT( SReward );

	CRewardGroup*				m_group;

	SReward( CRewardGroup* group, const SReward* reward = nullptr )
		: m_group( group )
		, m_experience( 0 )
		, m_level( 0 )
		, m_gold( 0 )
	{
		if ( reward )
		{
			m_name			= reward->m_name;
			m_experience	= reward->m_experience;
			m_level			= reward->m_level;
			m_gold			= reward->m_gold;
			m_items			= reward->m_items;
			m_achievement	= reward->m_achievement;
			m_script		= reward->m_script;
			m_comment		= reward->m_comment;
		}
	}

	SReward() 
		: m_group( NULL )
		, m_experience( 0 )
		, m_level( 0 )
		, m_gold( 0 )
		, m_achievement( 0 )
	{};

	CName						m_name;
	Int32						m_experience;
	Int32						m_level;
	Int32						m_gold;
	TDynArray< SItemReward >	m_items;
	Int32						m_achievement;
	CName						m_script;
	String						m_comment;

	static const SReward EMPTY;
};

BEGIN_CLASS_RTTI( SReward );
	PROPERTY( m_name );
	PROPERTY_EDIT( m_experience, TXT("Rewarded experience points.") ); 
	PROPERTY_EDIT( m_level, TXT("Recommended level for completing this quest.") );
	PROPERTY_EDIT( m_gold, TXT("Rewarded amount of gold.") );
	PROPERTY_EDIT( m_items, TXT("Rewarded items.") );
	PROPERTY_CUSTOM_EDIT( m_achievement, TXT("Achievement to unlock."), TXT("ScriptedEnum_EAchievement") );
	PROPERTY_CUSTOM_EDIT( m_script, TXT("Custom script called when reward is given."), TXT("RewardFunctionList") );
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("Comment") );
END_CLASS_RTTI();

class CRewardGroup : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CRewardGroup, CResource, "w2rewards", "Rewards" );

	friend class CEdRewardEditor;

protected:
	typedef TDynArray< THandle< CRewardGroup > >	TRewardGroups;

	TDynArray< SReward >	m_rewards;
	TRewardGroups			m_subGroups;
	Bool					m_isSubGroup;

public:
	CRewardGroup();

	SReward* CreateReward( const SReward* reward = nullptr );
	void	 RemoveReward( SReward* );
	Bool	 MoveReward( const CName& rewardToMove, CRewardGroup* toGroup, const CName& toPosition );

	RED_INLINE const TDynArray< SReward >&					GetRewards() const { return m_rewards; }
	RED_INLINE const TDynArray< THandle< CRewardGroup > >&	GetSubGroups() const { return m_subGroups; }
	RED_INLINE Bool											IsSubGroup() const { return m_isSubGroup; }
};

BEGIN_CLASS_RTTI( CRewardGroup );
	PARENT_CLASS( CResource );
	PROPERTY( m_rewards );
	PROPERTY_EDIT( m_subGroups, TXT("Sub Groups") );
	PROPERTY_NOT_COOKED( m_isSubGroup );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////

/// Reward group iterator
class CRewardGroupIterator
{
private:
	const CRewardGroup&							m_root;			//!< Group being iterated
	TDynArray< THandle< CRewardGroup > >::const_iterator	m_groupIter;	//!< Index of current group
	CRewardGroupIterator*						m_subGroupIter;	//!< Recursive sub group iterator
	const Bool									m_recursive;	//!< Recursive flag

public:
	//! Constructor
	//! If recursive is set to true, will iterate through groups of subgroups
	RED_INLINE CRewardGroupIterator( const CRewardGroup& group, Bool recursive = false )
		: m_root( group )
		, m_groupIter( group.GetSubGroups().Begin() )
		, m_subGroupIter( NULL )
		, m_recursive( recursive )
	{
		//! Subgroups can be NULL if referenced resource has been deleted, so get first non-NULL iterator
		while( m_groupIter != m_root.GetSubGroups().End() && (*m_groupIter).Get() == NULL ) ++m_groupIter;
	};

	//! Copy constructor
	RED_INLINE CRewardGroupIterator( const CRewardGroupIterator& other )
		: m_root( other.m_root )
		, m_groupIter( other.m_groupIter )
		, m_subGroupIter( other.m_subGroupIter ? new CRewardGroupIterator( *other.m_subGroupIter ) : NULL )
		, m_recursive( other.m_recursive )
	{};

	//! Destructor
	RED_INLINE ~CRewardGroupIterator()
	{
		if( m_subGroupIter )
		{
			delete m_subGroupIter;
			m_subGroupIter = NULL;
		}
	}

	//! Is current element valid
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
	RED_INLINE CRewardGroup* operator*()
	{
		if( m_subGroupIter )
		{
			return *(*m_subGroupIter);
		}
		else
		{
			ASSERT( IsValid() );
			return (*m_groupIter).Get();
		}
	}

protected:
	//! Is the iterator valid ?
	Bool IsValid() const;

	//! Advance to next element
	void Next();

private:
	//! Assignment is illegal
	RED_INLINE CRewardGroupIterator& operator=( const CRewardGroupIterator& other )
	{
		return *this;
	}
};

/// Reward directory iterator
class CRewardDirectoryIterator
{
private:
	CDirectory&					m_root;			//!< Directory being iterated
	TDynArray< CDiskFile* >		m_files;		//!< Files to iterate over
	Uint32						m_index;		//!< File index

public:
	//! Constructor
	RED_INLINE CRewardDirectoryIterator( CDirectory& directory, Bool recursive = false )
		: m_root( directory )
		, m_index( 0 )
	{
		m_root.CollectFiles( m_files, TXT("w2rewards"), recursive, false );
	}

	//! Copy constructor
	RED_INLINE CRewardDirectoryIterator( const CRewardDirectoryIterator& other )
		: m_root( other.m_root )
		, m_files( other.m_files )
		, m_index( other.m_index )
	{};

	//! Destructor
	RED_INLINE ~CRewardDirectoryIterator()
	{
	}

	//! Is current element valid
	RED_INLINE operator Bool () const
	{
		return m_index < m_files.Size();
	}

	//! Advance to next
	RED_INLINE void operator++ ()
	{
		++m_index;
	}

	//! Get current
	RED_INLINE THandle< CRewardGroup > operator*()
	{
		return SafeCast< CRewardGroup >( m_files[ m_index ]->Load() );
	}

private:
	//! Assignment is illegal
	RED_INLINE CRewardDirectoryIterator& operator=( const CRewardDirectoryIterator& other )
	{
		return *this;
	}
};

/// Reward iterator
class CRewardIterator
{
private:
	union
	{
		CRewardDirectoryIterator*	m_dirIter;
		CRewardGroupIterator*		m_groupIter;
	};
	TDynArray< SReward >::const_iterator	m_iterCurrent;	//!< Index of current reward
	TDynArray< SReward >::const_iterator	m_iterEnd;		//!< End of current group
	const Bool								m_isDirectory;	//!< Type of iterator used

	Bool									m_hackNeedToDoItFast;	//!< Hack to finish prototyping asap

public:
	//! Constructor
	CRewardIterator( CDirectory& directory, Bool recursive = true )
		: m_dirIter( new CRewardDirectoryIterator( directory, recursive ) )
		, m_isDirectory( true )
		, m_hackNeedToDoItFast( false )
	{
		// First check if there is any group in directory
		if( *m_dirIter )
		{
			// If yes, then find first reward ( groups may be empty )
			FindFirstReward( *m_dirIter );
		}
	};

	//! Constructor
	CRewardIterator( const CRewardGroup& group, Bool recursive = true )
		: m_groupIter( new CRewardGroupIterator( group, recursive ) )
		, m_isDirectory( false )
		, m_hackNeedToDoItFast( true )
	{
		// In this case we must iterate through our root group first
		m_iterCurrent = group.GetRewards().Begin();
		m_iterEnd = group.GetRewards().End();

		// If its not empty then we're done
		if( m_iterCurrent != m_iterEnd )
		{
			return;
		}
		// First check if there is any group in root group
		else if( *m_groupIter )
		{
			m_hackNeedToDoItFast = false;

			// If yes, then find first reward ( groups may be empty )
			FindFirstReward( *m_groupIter );
		}
	};

	//! Copy constructor
	RED_INLINE CRewardIterator( const CRewardIterator& other )
		: m_dirIter( other.m_dirIter )
		, m_iterCurrent( other.m_iterCurrent )
		, m_isDirectory( other.m_isDirectory )
	{};

	//! Destructor
	RED_INLINE ~CRewardIterator()
	{
		m_isDirectory ? delete m_dirIter : delete m_groupIter;
	}

	//! Is current element valid
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
	RED_INLINE const SReward& operator*()
	{
		ASSERT( IsValid() );
		return *m_iterCurrent;
	}

protected:
	//! Is the iterator valid ?
	Bool IsValid() const;

	//! Advance to next element
	void Next();

private:
	//! Assignment is illegal
	RED_INLINE CRewardIterator& operator=( const CRewardIterator& other )
	{
		return *this;
	}

	//! Find first valid reward in specified iterator
	template< class TIterator >
	void FindFirstReward( TIterator it )
	{
		ASSERT( it );
		CRewardGroup* currentGroup = *it;
		m_iterCurrent = currentGroup->GetRewards().Begin();
		m_iterEnd = currentGroup->GetRewards().End();
		while( m_iterCurrent == m_iterEnd )
		{
			// Current group is empty, move to the next one
			++it;
			if( it )
			{
				currentGroup = *it;
				m_iterCurrent = currentGroup->GetRewards().Begin();
				m_iterEnd = currentGroup->GetRewards().End();
			}
			else
			{
				// No more groups
				break;
			}
		}
	}
};
