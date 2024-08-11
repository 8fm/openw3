
#include "build.h"
#include "animationReporter.h"
#include "../../common/game/actionPointComponent.h"
#include "../../common/game/jobTree.h"
#include "../../common/engine/behaviorGraph.h"
#include "../../common/core/gatheredResource.h"

Uint64 CEdAnimationReporterWindow::CalcHashForAC( CAnimatedComponent* ac ) const
{
	Uint64 defineHash = HASH64_BASE;

	const TDynArray< SBehaviorGraphInstanceSlot >& slots = ac->GetBehaviorSlots();
	const TDynArray< THandle< CSkeletalAnimationSet > >& animsets = ac->GetAnimationSets();

	for ( Uint32 i=0; i<slots.Size(); ++i )
	{
		const SBehaviorGraphInstanceSlot& s = slots[ i ];
		const String& graphPath = s.m_graph.Get()->GetDepotPath();

		defineHash = ACalcBufferHash64Merge( graphPath, defineHash );
	}

	for ( Uint32 i=0; i<animsets.Size(); ++i )
	{
		const CSkeletalAnimationSet* a = animsets[ i ].Get();
		if ( a )
		{
			defineHash = ACalcBufferHash64Merge( a->GetDepotPath(), defineHash );
		}
	}

	return defineHash;
}

void CEdAnimationReporterWindow::FillACNode( ACNode& node, Uint64 hash, CAnimatedComponent* ac, EdAnimationReporterTodoList& todo )
{
	node.m_hash = hash;

	const TDynArray< SBehaviorGraphInstanceSlot >& slots = ac->GetBehaviorSlots();
	const TDynArray< THandle< CSkeletalAnimationSet > >& animsets = ac->GetAnimationSets();

	node.m_animsets.Resize( animsets.Size() );
	node.m_behaviors.Resize( slots.Size() );

	TDynArray< CName > collectedBehSlots;
	TDynArray< String > collectedBehPath;

	for ( Uint32 i=0; i<slots.Size(); ++i )
	{
		const SBehaviorGraphInstanceSlot& s = slots[ i ];
		const String& graphPath = s.m_graph.Get()->GetDepotPath();

		node.m_behaviors[ i ] = graphPath;

		if ( graphPath.Empty() )
		{
			String entityTemplPath = ac->GetEntity()->GetEntityTemplate() ? ac->GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : ac->GetEntity()->GetFriendlyName().AsChar();
			String desc = String::Printf( TXT("Component name: %s, Behavior slot number: %d"), ac->GetName().AsChar(), i );
			todo.AddTask( new EdAnimationReporterEmptySlot( entityTemplPath, desc, ARTC_AnimComp ) );
			continue;
		}

		if ( collectedBehSlots.Exist( s.m_instanceName ) )
		{
			String entityTemplPath = ac->GetEntity()->GetEntityTemplate() ? ac->GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : ac->GetEntity()->GetFriendlyName().AsChar();
			todo.AddTask( new EdAnimationReporterDuplicatedBehaviorSlots( entityTemplPath, ac->GetName(), s.m_instanceName, graphPath ) );
			continue;
		}
		else
		{
			collectedBehSlots.PushBack( s.m_instanceName );
		}

		if ( collectedBehPath.Exist( graphPath ) )
		{
			String entityTemplPath = ac->GetEntity()->GetEntityTemplate() ? ac->GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : ac->GetEntity()->GetFriendlyName().AsChar();
			todo.AddTask( new EdAnimationReporterDuplicatedBehaviorSlots( entityTemplPath, ac->GetName(), s.m_instanceName, graphPath ) );
		}
		else
		{
			collectedBehPath.PushBack( graphPath );
		}
	}

	for ( Uint32 i=0; i<animsets.Size(); ++i )
	{
		const CSkeletalAnimationSet* a = animsets[ i ].Get();
		if ( a )
		{
			ASSERT( !a->GetDepotPath().Empty() );
			node.m_animsets[ i ] = a->GetDepotPath();
		}
		else
		{
			String entityTemplPath = ac->GetEntity()->GetEntityTemplate() ? ac->GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : ac->GetEntity()->GetFriendlyName().AsChar();
			String desc = String::Printf( TXT("Component name: %s, Animset slot number: %d"), ac->GetName().AsChar(), i );
			todo.AddTask( new EdAnimationReporterEmptySlot( entityTemplPath, desc, ARTC_AnimComp ) );
		}
	}
}

ACNode& CEdAnimationReporterWindow::GetOrCreateACNodeByComp( CAnimatedComponent* ac, EdAnimationReporterTodoList& todo )
{
	Uint64 hash = CalcHashForAC( ac );

	for ( Uint32 i=0; i<m_acNodes.Size(); ++i )
	{
		ACNode& node = m_acNodes[ i ];
		if ( node.m_hash == hash )
		{
			return node;
		}
	}

	Uint32 index = m_acNodes.Grow( 1 );
	ACNode& node = m_acNodes[ index ];

	FillACNode( node, hash, ac, todo );

	return node;
}

void CEdAnimationReporterWindow::FillAPNode( APNode& node, Uint64 hash, CActionPointComponent* ap, EdAnimationReporterTodoList& todo )
{
	node.m_hash = hash;

	CJobTree* tree = ap->GetJobTreeRes().Get();
	if ( tree )
	{
		node.m_jobTreePath = tree->GetDepotPath();
	}
	else
	{
		String entityTemplPath = ap->GetEntity()->GetEntityTemplate() ? ap->GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : ap->GetEntity()->GetFriendlyName().AsChar();
		String desc = String::Printf( TXT("Component name: %s"), ap->GetName().AsChar() );
		todo.AddTask( new EdAnimationReporterEmptySlot( entityTemplPath, desc, ARTC_ApComp ) );
	}
}

Uint64 CEdAnimationReporterWindow::CalcHashForAP( CActionPointComponent* ap, EdAnimationReporterTodoList& todo ) const
{
	Uint64 defineHash = HASH64_BASE;

	CJobTree* tree = ap->GetJobTreeRes().Get();
	if ( tree )
	{
		String path = tree->GetDepotPath();
		defineHash = ACalcBufferHash64Merge( path, defineHash );
	}
	else
	{
		String entityTemplPath = ap->GetEntity()->GetEntityTemplate() ? ap->GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : ap->GetEntity()->GetFriendlyName().AsChar();
		String desc = String::Printf( TXT("JobTree in ap component: %s"), ap->GetName().AsChar() );
		todo.AddTask( new EdAnimationReporterMissingResource( entityTemplPath, desc, ARTC_ApComp ) );
	}

	return defineHash;
}

APNode& CEdAnimationReporterWindow::GetOrCreateAPNodeByComp( CActionPointComponent* ap, EdAnimationReporterTodoList& todo )
{
	Uint64 hash = CalcHashForAP( ap, todo );

	for ( Uint32 i=0; i<m_apNodes.Size(); ++i )
	{
		APNode& node = m_apNodes[ i ];
		if ( node.m_hash == hash )
		{
			return node;
		}
	}

	Uint32 index = m_apNodes.Grow( 1 );
	APNode& node = m_apNodes[ index ];

	FillAPNode( node, hash, ap, todo );

	return node;
}

void CEdAnimationReporterWindow::FillDependences( ACNode& node, EdAnimationReporterTodoList& todo )
{
	TDynArray< CName > anims;

	// Collect all used animations
	for ( Uint32 i=0; i<node.m_behaviors.Size(); ++i )
	{
		if ( node.m_behaviors[ i ].Empty() )
		{
			continue;
		}

		EdAnimReportBehavior* beh = FindRecordByPath< EdAnimReportBehavior >( node.m_behaviors[ i ], m_behaviorRecords );
		if ( !beh )
		{
			todo.AddTask( new EdAnimationReporterMissingResource( node.m_behaviors[ i ], TXT("Empty record"), ARTC_Behavior ) );
			continue;
		}

		anims.PushBackUnique( beh->m_usedAnimations );
	}

	// Find animsets
	TDynArray< EdAnimReportAnimset* > ass;
	ass.Resize( node.m_animsets.Size() );

	for ( Uint32 i=0; i<node.m_animsets.Size(); ++i )
	{
		ass[ i ] = FindRecordByPath< EdAnimReportAnimset >( node.m_animsets[ i ], m_animsetRecords );
	}

	// Try to find anims in animsets
	for ( Uint32 j=0; j<anims.Size(); ++j )
	{
		const CName& animName = anims[ j ];

		TDynArray< String > tempAnimsets;

		for ( Uint32 i=0; i<node.m_animsets.Size(); ++i )
		{
			EdAnimReportAnimset* as = ass[ i ];
			if ( as )
			{
				for ( Uint32 k=0; k<as->m_animations.Size(); ++k )
				{
					EdAnimReportAnimation* anim = as->m_animations[ k ];
					if ( anim->m_name == animName )
					{
						anim->m_used++;
						tempAnimsets.PushBackUnique( node.m_animsets[ i ] );
					}
				}
			}
		}

		if ( tempAnimsets.Size() > 1 )
		{
			todo.AddTask( new EdAnimationReporterDuplicatedAnimation( tempAnimsets, animName ) );
		}
	}

	node.m_usedAnims = anims.Size();
}

namespace
{
	void CollectJobAnimsets( TDynArray< TJobAnimset >& jobAnimsets )
	{
		const C2dArray& animsets2dArray = SAnimationsCategoriesResourcesManager::GetInstance().Get2dArray();

		Uint32 colSize, rowSize;
		animsets2dArray.GetSize( colSize, rowSize );

		for ( Uint32 rowNum = 0; rowNum < rowSize; rowNum++ )
		{
			Uint32 index = jobAnimsets.Grow( 1 );
			TJobAnimset& jobAnimset = jobAnimsets[ index ];

			jobAnimset.m_first = animsets2dArray.GetValue( 0, rowNum );
			jobAnimset.m_second = animsets2dArray.GetValue( 1, rowNum );
		}
	}

	Int32 FindAnimsetByCategory( const String& cat, const TDynArray< TJobAnimset >& jobAnimsets )
	{
		for ( Uint32 i=0; i<jobAnimsets.Size(); ++i )
		{
			if ( jobAnimsets[ i ].m_first == cat )
			{
				return (Int32)i;
			}
		}
		return -1;
	}
}

void CEdAnimationReporterWindow::FillDependences( APNode& node, EdAnimationReporterTodoList& todo )
{
	TDynArray< TJobAnimset > jobAnimsets;
	CollectJobAnimsets( jobAnimsets );

	TDynArray< EdAnimReportAnimset* > animsetsRes;
	animsetsRes.Resize( jobAnimsets.Size() );

	for ( Uint32 i=0; i<jobAnimsets.Size(); ++i )
	{
		animsetsRes[ i ] = FindRecordByPath< EdAnimReportAnimset >( jobAnimsets[ i ].m_second, m_animsetRecords );
		
		if ( !animsetsRes[ i ] && !jobAnimsets[ i ].m_second.Empty() )
		{
			animsetsRes[ i ] = AddAnimset( jobAnimsets[ i ].m_second );

			if ( !animsetsRes[ i ] )
			{
				todo.AddTask( new EdAnimationReporterMissingResource( jobAnimsets[ i ].m_second, TXT("Empty record"), ARTC_Animset ) );
			}
		}
	}

	EdAnimReportJobTree* jt = FindRecordByPath< EdAnimReportJobTree >( node.m_jobTreePath, m_jobRecords );
	if ( jt )
	{
		const CJobTree* jobTree = jt->GetResource();
		if ( jobTree )
		{
			TDynArray< SJobAnimation > jobAnims;
			jobTree->EnumUsedAnimations( jobAnims );

			for ( Uint32 i=0; i<jobAnims.Size(); ++i )
			{
				const SJobAnimation& jobAnim = jobAnims[ i ];

				Int32 index = FindAnimsetByCategory( jobAnim.m_category, jobAnimsets );
				if ( index != -1 )
				{
					ASSERT( animsetsRes.SizeInt() > index );

					EdAnimReportAnimset* as = animsetsRes[ index ];
					if ( as )
					{
						Bool found = false;

						for ( Uint32 k=0; k<as->m_animations.Size(); ++k )
						{
							EdAnimReportAnimation* anim = as->m_animations[ k ];
							if ( anim->m_name == jobAnim.m_animation )
							{
								anim->m_used++;
								found = true;
							}
						}

						if ( !found )
						{
							String desc = String::Printf( TXT("Couldn't find job animation '%s' for category '%s' "), jobAnim.m_animation.AsString().AsChar(), jobAnim.m_category.AsChar() );
							todo.AddTask( new EdAnimationReporterMissingResource( as->m_path, desc, ARTC_JobTree ) );
						}
					}
				}
				else
				{
					String desc = String::Printf( TXT("Couldn't find job animset for category '%s' "), jobAnim.m_category.AsChar() );
					todo.AddTask( new EdAnimationReporterMissingResource( jobAnim.m_category, desc, ARTC_JobTree ) );
				}
			}
		}
		else
		{
			todo.AddTask( new EdAnimationReporterMissingResource( node.m_jobTreePath, String::EMPTY, ARTC_JobTree ) );
		}
	}
	else if ( !node.m_jobTreePath.Empty() )
	{
		todo.AddTask( new EdAnimationReporterMissingResource( node.m_jobTreePath, TXT("Empty record"), ARTC_JobTree ) );
	}
}
