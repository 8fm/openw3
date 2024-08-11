
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animatedComponent.h"
#include "behaviorGraphStack.h"
#include "animatedComponentScripts.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "../core/scriptStackFrame.h"
#include "skeleton.h"
#include "entity.h"

#ifdef DEBUG_AC
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SAnimatedComponentSyncSettings );
IMPLEMENT_ENGINE_CLASS( SAnimatedComponentSlotAnimationSettings );

//////////////////////////////////////////////////////////////////////////
// Native functions
//////////////////////////////////////////////////////////////////////////
void CAnimatedComponent::funcSetBehaviorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Float, var, 0.0f );
	FINISH_PARAMETERS;
	Bool ret = false;
	ret = m_behaviorGraphStack ? m_behaviorGraphStack->SetBehaviorVariable( varName, var ) : false;
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior set variable error - %s %s"),
			GetName().AsChar(), GetEntity()->GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcGetBehaviorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;
	Float var = m_behaviorGraphStack ? m_behaviorGraphStack->GetBehaviorFloatVariable( varName ) : 0.f;
	RETURN_FLOAT( var );
}

void CAnimatedComponent::funcSetBehaviorVectorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Vector, var, Vector::ZEROS );
	FINISH_PARAMETERS;
	Bool ret = false;
	ret = m_behaviorGraphStack ? m_behaviorGraphStack->SetBehaviorVariable( varName, var ) : false;
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Set behavior variable error - %s %s"),
			GetName().AsChar(), GetEntity()->GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcGetBehaviorVectorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;
	Vector var = m_behaviorGraphStack ? m_behaviorGraphStack->GetBehaviorVectorVariable( varName ) : Vector::ZEROS;
	RETURN_STRUCT( Vector, var );
}

void CAnimatedComponent::funcActivateBehaviors( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;
	Bool ret = m_behaviorGraphStack ? m_behaviorGraphStack->ActivateBehaviorInstances( names ) : false;
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance activation fail - %s %s"),
			GetName().AsChar(), GetEntity()->GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcAttachBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	Bool ret = m_behaviorGraphStack ? m_behaviorGraphStack->AttachBehaviorInstance( name ) : false;
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance attach error - %s - %s %s"),
			name.AsString().AsChar(), GetName().AsChar(), GetEntity()->GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcDetachBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	Bool ret = m_behaviorGraphStack ? m_behaviorGraphStack->DetachBehaviorInstance( name ) : false;
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance detach error - %s - %s %s"),
			name.AsString().AsChar(), GetName().AsChar(), GetEntity()->GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcDisplaySkeleton( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, dispBone, false );
	GET_PARAMETER_OPT( Bool, dispAxis, false );
	GET_PARAMETER_OPT( Bool, dispNames, false );
	FINISH_PARAMETERS;

	SetDispSkeleton( ACDD_SkeletonBone, dispBone );
	SetDispSkeleton( ACDD_SkeletonAxis, dispAxis );
	SetDispSkeleton( ACDD_SkeletonName, dispNames );
}

void CAnimatedComponent::funcGetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float var = GetTimeMultiplier();
	RETURN_FLOAT( var );
}

void CAnimatedComponent::funcSetAnimationTimeMultiplier( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, mult, 1.f );
	FINISH_PARAMETERS;
	SetTimeMultiplier( mult );
}

void CAnimatedComponent::funcGetMoveSpeedAbs( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float var = GetAbsoluteMoveSpeed();
	RETURN_FLOAT( var );
}

void CAnimatedComponent::funcGetMoveDirWorldSpace( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float var = GetMoveDirectionWorldSpace();
	RETURN_FLOAT( var );
}

void CAnimatedComponent::funcRaiseBehaviorEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false;

	if ( m_behaviorGraphStack )
	{
		ret = m_behaviorGraphStack->GenerateBehaviorEvent( eventName );
	}

	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcRaiseBehaviorForceEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false;

	if ( m_behaviorGraphStack )
	{
		ret = m_behaviorGraphStack->GenerateBehaviorForceEvent( eventName );
	}

	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcFindNearestBoneWS( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, position, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	Int32 bestBone = -1;
	if(m_skeleton)
	{		
		Int32 endIndex = m_skeleton->GetBonesNum();
		if ( endIndex > 0 )
		{
			const Int32 lodBonesNum = m_skeleton->GetLodBoneNum_1();
			if( lodBonesNum > 0)
			{
				endIndex = lodBonesNum;
			}
		
			Float bestDistanceSquared( FLT_MAX ), currentDisanceSquared( FLT_MAX );
			for ( Int32 bone = 0; bone < endIndex; ++bone )
			{
				currentDisanceSquared = position.DistanceSquaredTo( m_skeletonWorldSpace[ bone ].GetTranslationRef() );
				if ( currentDisanceSquared < bestDistanceSquared )
				{
					bestBone = bone;
					bestDistanceSquared = currentDisanceSquared;
				}
			}

			ASSERT( bestBone >= 0 );
			position = m_skeletonWorldSpace[ bestBone ].GetTranslationRef();
		}
	}

	RETURN_INT( bestBone );
}

void CAnimatedComponent::funcFindNearestBoneToEdgeWS( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER_REF( Vector, b, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;


	Int32 bestBone = -1;
	if(m_skeleton)
	{		
		Int32 endIndex = m_skeleton->GetBonesNum();
		if ( endIndex > 0 )
		{
			const Int32 lodBonesNum = m_skeleton->GetLodBoneNum_1();
			if( lodBonesNum > 0)
			{
				endIndex = lodBonesNum;
			}

			Float bestDistance( FLT_MAX ), currentDisance( FLT_MAX );
			for ( Int32 bone = 0; bone < endIndex; ++bone )
			{
				const auto& transl = m_skeletonWorldSpace[ bone ].GetTranslationRef();
				currentDisance = transl.DistanceToEdge(a, b);
				if ( currentDisance < bestDistance )
				{
					bestBone = bone;
					bestDistance = currentDisance;
				}
			}

			ASSERT( bestBone >= 0 );
		}
	}

	RETURN_INT( bestBone );
}

void CAnimatedComponent::funcGetCurrentBehaviorState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, instanceName, CName::NONE );
	FINISH_PARAMETERS;

	String ret = String::EMPTY;

	if ( m_behaviorGraphStack )
	{
		if(instanceName == CName::NONE)
		{
			instanceName = m_behaviorGraphStack->GetActiveBottomInstance();
		}

		ret =  m_behaviorGraphStack->GetStateInDefaultStateMachine(instanceName);
	}

	RETURN_STRING( ret );
}

void CAnimatedComponent::funcFreezePose( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->FreezePose();
	}
}

void CAnimatedComponent::funcUnfreezePose( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->UnfreezePose();
	}
}

void CAnimatedComponent::funcFreezePoseFaded( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, fadeInTime, 1.0f );
	FINISH_PARAMETERS;

	FreezePoseFadeIn( fadeInTime );
}

void CAnimatedComponent::funcUnfreezePoseFaded( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, fadeOutTime, 1.0f );
	FINISH_PARAMETERS;

	UnfreezePoseFadeOut( fadeOutTime );
}

void CAnimatedComponent::funcHasFrozenPose( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Bool ret = m_behaviorGraphStack && m_behaviorGraphStack->HasFrozenPose();

	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcSyncTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CAnimatedComponent >, slaveComponent, NULL );
	GET_PARAMETER( SAnimatedComponentSyncSettings, ass, SAnimatedComponentSyncSettings() )
	FINISH_PARAMETERS;

	Bool ret = false;

	CAnimatedComponent* slave = slaveComponent.Get();

	if ( slave && slave->GetBehaviorStack() && m_behaviorGraphStack )
	{
		CSyncInfo sync;
		if ( m_behaviorGraphStack->GetSyncInfo( sync ) )
		{
			ret = slave->GetBehaviorStack()->SynchronizeTo( sync, ass );
		}

		if ( ass.m_syncEngineValueSpeed )
		{
			const Float val = GetRelativeMoveSpeed();
			slave->ForceSetRelativeMoveSpeed( val );
		}
	}

	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcUseExtractedMotion( CScriptStackFrame& stack, void* result )
{	
	FINISH_PARAMETERS;

	RETURN_BOOL( m_useExtractedMotion );
}

void CAnimatedComponent::funcSetUseExtractedMotion( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, use, false );
	FINISH_PARAMETERS;

	SetUseExtractedMotion( use );
}

void CAnimatedComponent::funcStickRagdollToCapsule( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, stick, false );
	FINISH_PARAMETERS;

	m_stickRagdollToCapsule = stick;
}


void CAnimatedComponent::funcHasRagdoll( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_ragdoll.IsValid() );
}

void CAnimatedComponent::funcGetRagdollBoneName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, actorIndex, 0 );			
	FINISH_PARAMETERS;

	if( !m_ragdollPhysicsWrapper )
	{
		RETURN_NAME( CNAME( Unknown ) );
		return;
	}

	CName name = CName::NONE;
	Int32 boneIndex = m_ragdollPhysicsWrapper->GetBoneIndex( actorIndex );
	if ( boneIndex >= 0 && m_skeleton != nullptr )
	{
		name = m_skeleton->GetBoneNameAsCName( boneIndex );
	}
	RETURN_NAME( name );
}

void CAnimatedComponent::funcPlaySlotAnimationAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animation, CName::NONE );
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER_OPT( SAnimatedComponentSlotAnimationSettings, ass, SAnimatedComponentSlotAnimationSettings() );
	FINISH_PARAMETERS;

	Bool ret = false;

	CBehaviorGraphStack* behStack = GetBehaviorStack();
	if ( behStack )
	{
		SBehaviorSlotSetup setup;
		setup.m_blendIn = ass.m_blendIn;
		setup.m_blendOut = ass.m_blendOut;

		ret = behStack->PlaySlotAnimation( slotName, animation, &setup );
	}

	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcPlaySkeletalAnimationAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animation, CName::NONE );
	GET_PARAMETER_OPT( Bool, looped, false );
	FINISH_PARAMETERS;

	Bool ret = PlayAnimationOnSkeleton( animation, true, looped );

	RETURN_BOOL( ret );
}

void CAnimatedComponent::funcGetMoveSpeedRel( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetRelativeMoveSpeed() );
}

void CAnimatedComponent::funcGetBoneMatrixMovementModelSpaceInAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, boneIndex, 0 );
	GET_PARAMETER( CName, animation, CName::NONE );
	GET_PARAMETER( Float, time, 0.0f );
	GET_PARAMETER( Float, deltaTime, 0.1f );
	GET_PARAMETER_REF( Matrix, refAtTime, Matrix::IDENTITY );
	GET_PARAMETER_REF( Matrix, refWithDeltaTime, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	GetBoneMatrixMovementModelSpaceInAnimation( boneIndex, animation, time, deltaTime, refAtTime, refWithDeltaTime );
}

void CAnimatedComponent::funcDontUpdateByOtherAnimatedComponent(CScriptStackFrame & stack, void * result)
{
	FINISH_PARAMETERS;

	DontUpdateByOtherAnimatedComponent();
}

void CAnimatedComponent::funcUpdateByOtherAnimatedComponent(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( THandle< CAnimatedComponent >, masterComponent, NULL );
	FINISH_PARAMETERS;

	UpdateByOtherAnimatedComponent( masterComponent.Get() );
}

#ifdef DEBUG_AC
#pragma optimize("",on)
#endif
