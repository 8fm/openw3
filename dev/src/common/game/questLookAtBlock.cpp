#include "build.h"
#include "questGraphSocket.h"
#include "questLookAtBlock.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestLookAtBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

	void CQuestLookAtBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif
void CQuestLookAtBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	OnActivateImpl( data, inputName, parentThread );
	ActivateOutput( data, CNAME( Out ) );
}

void CQuestLookAtBlock::OnActivateImpl( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CNode* lookAtTarget = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_target );

	if( !lookAtTarget && m_enabled )
	{		
		return;
	}
	
	struct Functor : public Red::System::NonCopyable
	{		
		CNode* m_lookAtTarget;
		const CQuestLookAtBlock* m_parentBlock;

		Functor( CNode* lookAtTarger, const CQuestLookAtBlock* parentBlock ) : 
			m_lookAtTarget( lookAtTarger ), 
			m_parentBlock( parentBlock )
		{}

		RED_INLINE Bool EarlyTest( CNode* node )
		{						
			return true;			
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			m_parentBlock->PerformLookAt( node, m_lookAtTarget );		
		}		
	} functor( lookAtTarget, this );

	GGame->GetActiveWorld()->GetTagManager()->IterateTaggedNodes( m_actor, functor );

	ActivateOutput( data, CNAME( Out ) );
}

void CQuestLookAtBlock::PerformLookAt( CNode* lookingNode, CNode* m_lookAtTarget ) const
{
	CActor* lookingActor = Cast< CActor >( lookingNode );
	
	if ( lookingActor )
	{
		if( m_enabled )
		{
			switch ( m_type )
			{
			case DLT_Dynamic		: PerformLookAtDynamic( lookingActor, m_lookAtTarget ); break;
			case DLT_Static			: PerformLookAtStatic( lookingActor, m_lookAtTarget ); break;
			case DLT_StaticPoint	: PerformLookAtStaticPoint( lookingActor ); break;
			}
		}
		else
		{
			lookingActor->DisableDialogsLookAts( m_speed );
		}
	}

}

void CQuestLookAtBlock::PerformLookAtDynamic( CActor* lookingActor, CNode* m_lookAtTarget ) const
{
	CAnimatedComponent* targetAnimatedComponent;
	Int32 targetHeadBoneName;
	CActor* targetActor = Cast< CActor >( m_lookAtTarget );
	if( targetActor )
	{
		targetAnimatedComponent	= const_cast< CAnimatedComponent* >( targetActor->GetRootAnimatedComponent() );
		targetHeadBoneName		= targetActor->GetHeadBone();
	}
	else
	{			
		return;
	}

	if ( targetAnimatedComponent && targetHeadBoneName != -1 )
	{
		SLookAtQuestBoneInfo info;
		info.m_boneIndex					= targetHeadBoneName;
		info.m_targetOwner					= targetAnimatedComponent;
		info.m_level						= m_level;
		info.m_speedOverride				= m_speed;
		info.m_instant						= m_instant;
		info.m_range						= m_gameplayRange;
		info.m_autoLimitDeact				= m_limitDeact;
		//info.m_desc							= desc;
		info.m_headRotationRatio			= m_headRotationRatio;
		info.m_eyesLookAtConvergenceWeight	= m_eyesLookAtConvergenceWeight;
		info.m_eyesLookAtIsAdditive			= m_eyesLookAtIsAdditive;
		info.m_eyesLookAtDampScale			= m_eyesLookAtDampScale;
		info.m_duration						= m_duration;

		lookingActor->EnableLookAt( info );		
	}
}

void CQuestLookAtBlock::PerformLookAtStatic( CActor* lookingNode, CNode* m_lookAtTarget ) const
{
	CActor* targetActor = Cast< CActor >( m_lookAtTarget );
	if( !targetActor )
		return;

	Vector staticTarget = targetActor->GetHeadPosition();
	SLookAtQuestStaticInfo info;
	//additional offset makes sure that actors are looking at the eyes instead of base of the head
	info.m_target						= staticTarget + Vector(0.f, 0.f, 0.09f );
	info.m_level						= m_level;
	info.m_speedOverride				= m_speed;
	info.m_instant						= m_instant;
	info.m_range						= m_range;
	info.m_autoLimitDeact				= m_limitDeact;	
	info.m_headRotationRatio			= m_headRotationRatio;
	info.m_eyesLookAtConvergenceWeight	= m_eyesLookAtConvergenceWeight;
	info.m_eyesLookAtIsAdditive			= m_eyesLookAtIsAdditive;
	info.m_eyesLookAtDampScale			= m_eyesLookAtDampScale;
	info.m_duration						= m_duration;

	lookingNode->EnableLookAt( info );
}

void CQuestLookAtBlock::PerformLookAtStaticPoint( CActor* lookingNode ) const
{
	Vector staticTarget = lookingNode->GetLocalToWorld().TransformPoint( m_staticPoint );

	SLookAtQuestStaticInfo info;
	info.m_target						= staticTarget;
	info.m_level						= m_level;
	info.m_speedOverride				= m_speed;
	info.m_instant						= m_instant;
	info.m_range						= m_range;
	info.m_autoLimitDeact				= m_limitDeact;	
	info.m_headRotationRatio			= m_headRotationRatio;
	info.m_eyesLookAtConvergenceWeight	= m_eyesLookAtConvergenceWeight;
	info.m_eyesLookAtIsAdditive			= m_eyesLookAtIsAdditive;
	info.m_eyesLookAtDampScale			= m_eyesLookAtDampScale;
	info.m_duration						= m_duration;

	lookingNode->EnableLookAt( info );
}