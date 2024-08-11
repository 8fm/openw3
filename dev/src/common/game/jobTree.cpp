/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "jobTree.h"
#include "jobTreeLeaf.h"
#include "jobTreeNode.h"
#include "../core/factory.h"

IMPLEMENT_RTTI_ENUM( EJobMovementMode );
IMPLEMENT_ENGINE_CLASS( SJobTreeSettings );
IMPLEMENT_ENGINE_CLASS( CJobTree );

//////////////////////////////////////////////////////////////////////////

void CJobTree::ApplyJobTreeExitSettings( CNewNPC* npc, Bool fast, SJobTreeExecutionContext& context ) const
{
	CInventoryComponent* inventory = npc->GetInventoryComponent();
	if ( !inventory )
	{
		return;
	}

	if ( m_settings.m_leftRemoveAtEnd || m_settings.m_leftDropOnBreak )
	{
		SInventoryItem* lItem = inventory->GetItemHeldInSlot( CNAME( l_weapon ) );
		if( !lItem || ( lItem->GetName() != context.m_skippedEtryAnimItem && lItem->GetCategory() != context.m_skippedEtryAnimItem ) )
		{
			npc->EmptyHand( CNAME( l_weapon ), fast && m_settings.m_leftDropOnBreak );
		}		
	}
	if ( m_settings.m_rightRemoveAtEnd || m_settings.m_rightDropOnBreak )
	{
		SInventoryItem* rItem = inventory->GetItemHeldInSlot( CNAME( r_weapon ) );
		if( !rItem || ( rItem->GetName() != context.m_skippedEtryAnimItem && rItem->GetCategory() != context.m_skippedEtryAnimItem ) )
		{
			npc->EmptyHand( CNAME( r_weapon ), fast && m_settings.m_rightDropOnBreak );
		}
	}
}

void CJobTree::EnumUsedAnimationsForNode( const CJobTreeNode* node, TDynArray< SJobAnimation >& anims ) const
{
	if ( node )
	{
		EnumUsedAnimationsForAction( node->m_onEnterAction, anims );
		EnumUsedAnimationsForAction( node->m_onLeaveAction, anims );
		EnumUsedAnimationsForAction( node->m_onFastLeaveAction, anims );

		for ( Uint32 i=0; i<node->m_childNodes.Size(); ++i )
		{
			EnumUsedAnimationsForNode( node->m_childNodes[ i ], anims );
		}
	}
}

void CJobTree::EnumUsedAnimationsForAction( const CJobActionBase* action, TDynArray< SJobAnimation >& anims ) const
{
	if ( action )
	{
		SJobAnimation jobAnim;
		jobAnim.m_category = action->GetAnimCategory();
		jobAnim.m_animation = action->GetAnimName();

		for ( Uint32 i=0; i<anims.Size(); ++i )
		{
			if ( anims[ i ].IsEqual( jobAnim ) )
			{
				return;
			}
		}

		anims.PushBack( jobAnim );
	}
}

void CJobTree::EnumUsedAnimations( TDynArray< SJobAnimation >& anims ) const
{
	EnumUsedAnimationsForNode( m_jobTreeRootNode, anims );
}


//////////////////////////////////////////////////////////////////////////
/// Factory for the job tree
//////////////////////////////////////////////////////////////////////////

class CJobTreeFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CJobTreeFactory, IFactory, 0 );

public:
	CJobTreeFactory()
	{
		m_resourceClass = ClassID< CJobTree >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CJobTreeFactory )
PARENT_CLASS( IFactory )	
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CJobTreeFactory );

CResource* CJobTreeFactory::DoCreate( const FactoryOptions& options )
{
	CJobTree *jobTree = ::CreateObject< CJobTree >( options.m_parentObject );
	return jobTree;
}
