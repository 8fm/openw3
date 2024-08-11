#include "build.h"
#include "questGraphSocket.h"
#include "../engine/entityMotion.h"
#include "questEntityMotionBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/tagManager.h"
#include "../engine/layerInfo.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestEntityMotionBlock )

// CQuestEntityMotion - quest-provided entity motion
class CQuestEntityMotion : public CEntityMotion
{
	friend class CQuestEntityMotionBlock;

	const CQuestEntityMotionBlock*	m_block;

public:
	CQuestEntityMotion( const CQuestEntityMotionBlock* block ) : m_block( block ) {}
	virtual void UpdateMotion();
};

void CQuestEntityMotion::UpdateMotion()
{
	// Apply the curve to the time
	Float timeInCurve = m_block->m_animationCurve.GetValue( m_time ).W;

	// For identity transforms use only the deltas
	if ( m_block->m_targetTransform.IsIdentity() )
	{
		if ( m_block->m_positionDelta != Vector::ZEROS )
		{
			m_entity->SetPosition( m_initialPosition + m_block->m_positionDelta*timeInCurve );
		}
		if ( m_block->m_rotationDelta != EulerAngles::ZEROS )
		{
			m_entity->SetRotation( m_initialRotation + m_block->m_rotationDelta*timeInCurve );
		}
		if ( m_block->m_scaleDelta != Vector::ZEROS )
		{
			m_entity->SetScale( m_initialScale + m_block->m_scaleDelta*timeInCurve );
		}
	}
	else // otherwise use the transform as the target
	{
		const Vector& position = m_block->m_targetTransform.GetPosition();
		const EulerAngles& rotation = m_block->m_targetTransform.GetRotation();
		const Vector& scale = m_block->m_targetTransform.GetScale();
		
		Vector rawPosition = m_initialPosition*( 1.0f - timeInCurve ) + position*timeInCurve;
		EulerAngles rawRotation = EulerAngles::Interpolate( m_initialRotation, rotation, timeInCurve );
		Vector rawScale = m_initialScale*( 1.0f - timeInCurve ) + scale*timeInCurve;

		m_entity->SetRawPlacement( &rawPosition, &rawRotation, &rawScale );
	}
}


//////////////////////////
// CQuestEntityMotionBlock

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestEntityMotionBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestEntityMotionBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_motion;
}

void CQuestEntityMotionBlock::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_motion ] = NULL;
}

void CQuestEntityMotionBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	// Find the entity referenced by the block
	CNode* entityNode = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_entityTag );
	if ( !entityNode || !entityNode->IsA< CEntity >() )
	{
		LOG_GAME( TXT("QuestEntityMotionBlock: Failed to find the entity node with tag '%ls'"), m_entityTag.AsString().AsChar() );
		return;
	}
	CEntity* entity = entityNode->AsEntity();
	if ( entity->GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		LOG_GAME( TXT("QuestEntityMotionBlock: The entity tagged '%ls' lies on an environment layer, make sure you use non static layer"), m_entityTag.AsString().AsChar() );
		return;
	}

	// Create an entity motion that will handle the actual motion
	// (note: the motion manager will destroy the object when it is done)
	CQuestEntityMotion* motion = new CQuestEntityMotion( this );
	data[ i_motion ] = reinterpret_cast< TGenericPtr >( motion );
	GGame->GetActiveWorld()->GetEntityMotionManager()->AddMotion( motion, entity, m_duration );
}

void CQuestEntityMotionBlock::OnExecute( InstanceBuffer& data ) const
{
	CEntityMotion* motion = reinterpret_cast< CEntityMotion* >( data[ i_motion ] );

	// If the motion is finished, activate the Out output
	if ( !GGame->GetActiveWorld()->GetEntityMotionManager()->IsRunning( motion ) )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestEntityMotionBlock::OnDeactivate( InstanceBuffer& data ) const
{
	data[ i_motion ] = NULL;
}
