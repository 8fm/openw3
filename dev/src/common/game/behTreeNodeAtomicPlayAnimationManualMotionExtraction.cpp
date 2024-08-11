/*
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicPlayAnimationManualMotionExtraction.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "../engine/animatedComponent.h"
#include "../engine/behaviorGraphStack.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAtomicPlayAnimationManualMotionExtractionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance::CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_slotName( def.m_slotName )
	, m_animationName( def.m_animationName )
	, m_loopIterations( def.m_loopIterations )
	, m_isTransitionAnimation( def.m_isTransitionAnimation )
	, m_animationTime( 0.f )
	, m_anim( nullptr )
	, m_currLoopIteration( 1 )
	, m_initPos( Vector::ZERO_3D_POINT )
	, m_initRot( EulerAngles::ZEROS )
	, m_initTrans( AnimQsTransform::IDENTITY )
	, m_lastTeleport( false )
{
}

Bool CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance::Activate()
{
	m_animationTime = 0.f;
	m_currLoopIteration = 1;
	m_lastTeleport = false;

	if ( CActor* actor = m_owner->GetActor() )
	{
		if ( CAnimatedComponent* ac = actor->GetRootAnimatedComponent() )
		{
			if ( CBehaviorGraphStack* stack = ac->GetBehaviorStack() )
			{
				if ( !stack->GetSlot( m_slotName, m_slot ) )
				{
					DebugNotifyActivationFail();
					return false;
				}
			}

			m_anim = m_animationName ? ac->GetAnimationContainer()->FindAnimationRestricted( m_animationName ) : nullptr;
			if ( !m_anim )
			{
				DebugNotifyActivationFail();
				return false;
			}

			m_initPos = actor->GetWorldPositionRef();
			m_initRot = actor->GetWorldRotation();

			Matrix currMat = m_initRot.ToMatrix();
			currMat.SetTranslation( m_initPos );

			RedMatrix4x4 _conversionMatrix;
			_conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( currMat );
			m_initTrans.Set( _conversionMatrix );
		}
	}
	
	return Super::Activate();
}

void CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance::Update()
{
	if ( m_slot.IsValid() && m_anim )
	{
		if ( m_lastTeleport )
		{
			Complete( BTTO_SUCCESS );
			return;
		}

		Float m_prevTime = m_animationTime; 
		m_animationTime += m_owner->GetLocalTimeDelta();

		const Float animationDuration = m_anim->GetDuration();
		if ( m_animationTime >= animationDuration )
		{
			if ( m_currLoopIteration < m_loopIterations )
			{
				m_currLoopIteration++;
				m_animationTime -= animationDuration;
			}
			else
			{
				m_lastTeleport = true;
				m_animationTime = animationDuration;
			}
		}

		SAnimationState state;
		state.m_animation = m_animationName;
		state.m_prevTime = m_prevTime;
		state.m_currTime = m_animationTime;
		
		m_slot.PlayAnimation( state );

		AnimQsTransform currTrans;
		if ( m_lastTeleport && !m_isTransitionAnimation )
		{
			currTrans = m_initTrans;
		}
		else
		{
			AnimQsTransform movement = m_anim->GetAnimation()->GetMovementAtTime( m_animationTime );
			currTrans.SetMul( m_initTrans, movement );
		}

		RedMatrix4x4 convertedMatrix = currTrans.ConvertToMatrixNormalized();
		Matrix matrix = reinterpret_cast< const Matrix& >( convertedMatrix );
		
		Vector currPos = matrix.GetTranslation();
		EulerAngles currRot = matrix.ToEulerAngles();

		m_owner->GetActor()->Teleport( currPos, currRot );
	}

	Super::Update();
}

void CBehTreeNodeAtomicPlayAnimationManualMotionExtractionInstance::Deactivate()
{
	m_slot.Stop();
	m_animationTime = 0.f;
	m_anim = nullptr;
	m_currLoopIteration = 1;
	m_lastTeleport = false;

 	Super::Deactivate();
}