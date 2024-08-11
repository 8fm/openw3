/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphDirectionalMovementNode.h"
#include "../engine/behaviorGraphBlendNode.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphValueNode.h"
#include "../engine/behaviorGraphContext.h"
#include "../engine/cacheBehaviorGraphOutput.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/behaviorGraphSocket.h"
#include "../engine/skeleton.h"
#include "../engine/graphConnectionRebuilder.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

#define DEBUG_DIRECTIONAL_MOVEMENT				0
#define DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME	0

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDirectionalMovementNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDirectionalMovementStartNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDirectionalMovementStopNode );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( MovementDirectionWS );
RED_DEFINE_STATIC_NAME( FacingDirectionWS );
RED_DEFINE_STATIC_NAME( InitialMovementDirectionWS );
RED_DEFINE_STATIC_NAME( InitialGroupDirMS );
RED_DEFINE_STATIC_NAME( AllowGroupDirChange );
RED_DEFINE_STATIC_NAME( AllowQuickTurn );
RED_DEFINE_STATIC_NAME( DirectionalMovement );
RED_DEFINE_STATIC_NAMED_NAME( RightFoot, "r_foot" )
RED_DEFINE_STATIC_NAMED_NAME( LeftFoot, "l_foot" )

RED_DEFINE_STATIC_NAME( requestedMovementDirection );
RED_DEFINE_STATIC_NAME( requestedFacingDirection );
RED_DEFINE_STATIC_NAME( MovementDirectionFromDirectionalMovement );
RED_DEFINE_STATIC_NAME( MovementDirectionGroupDir );


///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphDirectionalMovementNode::CBehaviorGraphDirectionalMovementNode()
	: m_groupCount( 2 )
	, m_groupCountFloat( 2.0f )
	, m_animsPerGroup( 3 )
	, m_groupDir( 180.0f )
	, m_firstGroupDirOffset( 0.0f )
	, m_keepInCurrentGroupAngle( 5.0f )
	, m_findGroupDirOffset( 0.0f )
	, m_extraOverlapAngle( 45.0f )
	, m_sideAngleRange( 135.0f )
	, m_singleAnimOnly( false )
	, m_doNotSwitchAnim( false )
	, m_movementDirBlendTime( 0.3f )
	, m_movementDirMaxSpeedChange( 165.0f )
	, m_facingDirBlendTime( 0.4f )
	, m_facingDirMaxSpeedChange( 130.0f )
	, m_groupsBlendTime( 0.25f )
	, m_quickTurnBlendTime( 0.4f )
	, m_fasterQuickTurnBlendTime( 0.2f )
	, m_angleThresholdForQuickTurn( 80.0f )
	, m_reverseSyncOnQuickTurnFwdBwd( true )
	, m_reverseSyncOnQuickTurnLeftRight( true )
	, m_syncBlendingOffsetPTLOnQuickTurn( -0.2f )
	// earlier to cover blend
	, m_startPTLRightFootInFront( 0.65f )
	, m_startPTLLeftFootInFront( 0.15f )
	, m_alwaysStartAtZero( false )
	, m_loopCount( 1 )
	, m_syncGroupOffsetPTL( 0.5f )
	, m_requestedMovementDirectionVariableName( CNAME( requestedMovementDirection ) )
	, m_requestedFacingDirectionVariableName( CNAME( requestedFacingDirection ) )
	, m_useDefinedVariablesAsRequestedInput( true )
	, m_useDefinedInternalVariablesAsInitialInput( false )
	, m_movementDirectionInternalVariableName( CNAME( MovementDirectionFromDirectionalMovement ) )
	, m_groupDirInternalVariableName( CNAME( MovementDirectionGroupDir ) )
	, m_syncMethod( NULL )
	, m_rightFootBone( CNAME( RightFoot ) )
	, m_leftFootBone( CNAME( LeftFoot ) )
{
	AutoCalculateValues();
}

void CBehaviorGraphDirectionalMovementNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_requestedQuickTurnTime;
	compiler << i_currentBlendTime;
	compiler << i_currentRotationYaw;
	compiler << i_requestedFacingDirection;
	compiler << i_facingDirectionChange;
	compiler << i_requestedMovementDirection;
	compiler << i_movementDirection;
	compiler << i_additionalMovementDirection;
	compiler << i_prevGroupMovementDirection;
	compiler << i_quickTurnIsAllowed;
	compiler << i_switchGroupIsAllowed;
	compiler << i_isBlending;
	compiler << i_blendLeft;
	compiler << i_isDoingQuickTurn;
	compiler << i_currGroupIdx;
	compiler << i_currGroupAnimIdx;
	compiler << i_currGroupAnimBlend;
	compiler << i_prevGroupIdx;
	compiler << i_prevGroupAnimIdx;
	compiler << i_prevGroupAnimBlend;
	compiler << i_rightFootBoneIdx;
	compiler << i_leftFootBoneIdx;

	// Variables
	compiler << i_hasRequestedMovementDirectionVariable;
	compiler << i_hasRequestedFacingDirectionVariable;
	// Internal variables
	compiler << i_hasMovementDirectionInternalVariable;
	compiler << i_hasGroupDirInternalVariable;
}

void CBehaviorGraphDirectionalMovementNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();

	instance[ i_requestedQuickTurnTime ] = 0.0f;
	instance[ i_currentBlendTime ] = 0.0f;
	instance[ i_currentRotationYaw ] = instance.GetAnimatedComponent()->GetWorldYaw();
	instance[ i_requestedFacingDirection ] = 0.0f;
	instance[ i_facingDirectionChange ] = 0.0f;
	instance[ i_requestedMovementDirection ] = 0.0f;
	instance[ i_movementDirection ] = 0.0f;
	instance[ i_additionalMovementDirection ] = 0.0f;
	instance[ i_prevGroupMovementDirection ] = 0.0f;
	instance[ i_quickTurnIsAllowed ] = false;
	instance[ i_switchGroupIsAllowed ] = false;
	instance[ i_isBlending ] = false;
	instance[ i_blendLeft ] = 0.0f;
	instance[ i_isDoingQuickTurn ] = false;
	instance[ i_currGroupIdx ] = 0;
	instance[ i_currGroupAnimIdx ] = 0;
	instance[ i_currGroupAnimBlend ] = 1.0f;
	instance[ i_prevGroupIdx ] = instance[ i_currGroupIdx ];
	instance[ i_prevGroupAnimIdx ] = 0;
	instance[ i_prevGroupAnimBlend ] = 1.0f;
	instance[ i_rightFootBoneIdx ] = skeleton->FindBoneByName( m_rightFootBone );
	instance[ i_leftFootBoneIdx ] = skeleton->FindBoneByName( m_leftFootBone );

	// Variables
	instance[ i_hasRequestedMovementDirectionVariable ] = instance.HasFloatValue( m_requestedMovementDirectionVariableName );
	instance[ i_hasRequestedFacingDirectionVariable ] = instance.HasFloatValue( m_requestedFacingDirectionVariableName );
	// Internal variables
	instance[ i_hasMovementDirectionInternalVariable ] = instance.HasInternalFloatValue( m_movementDirectionInternalVariableName );
	instance[ i_hasGroupDirInternalVariable ] = instance.HasInternalFloatValue( m_groupDirInternalVariableName );
}

void CBehaviorGraphDirectionalMovementNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_requestedQuickTurnTime );
	INST_PROP( i_currentBlendTime );
	INST_PROP( i_currentRotationYaw );
	INST_PROP( i_requestedFacingDirection );
	INST_PROP( i_facingDirectionChange );
	INST_PROP( i_requestedMovementDirection );
	INST_PROP( i_movementDirection );
	INST_PROP( i_additionalMovementDirection );
	INST_PROP( i_prevGroupMovementDirection );
	INST_PROP( i_quickTurnIsAllowed );
	INST_PROP( i_switchGroupIsAllowed );
	INST_PROP( i_isBlending );
	INST_PROP( i_blendLeft );
	INST_PROP( i_isDoingQuickTurn );
	INST_PROP( i_currGroupIdx );
	INST_PROP( i_currGroupAnimIdx );
	INST_PROP( i_currGroupAnimBlend );
	INST_PROP( i_prevGroupIdx );
	INST_PROP( i_prevGroupAnimIdx );
	INST_PROP( i_prevGroupAnimBlend );

	// Variables
	INST_PROP( i_hasRequestedMovementDirectionVariable );
	INST_PROP( i_hasRequestedFacingDirectionVariable );
	// Internal variables
	INST_PROP( i_hasMovementDirectionInternalVariable );
	INST_PROP( i_hasGroupDirInternalVariable );
}

void CBehaviorGraphDirectionalMovementNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	AutoCalculateValues();
}

void CBehaviorGraphDirectionalMovementNode::AutoCalculateValues()
{
	m_loopCount = Max( 1, m_loopCount );
	m_loopCountFloat = (Float) m_loopCount;
	m_groupCountFloat = (Float) m_groupCount;
	m_groupDir = 360.0f / Max( 1.0f, m_groupCountFloat );
	const Float halfGroupDir = m_groupDir / 2.0f;
	m_sideAngleRange = halfGroupDir + m_extraOverlapAngle;
	m_animsPerGroup = Max( 1, m_animsPerGroup );
	if ( m_animsPerGroup > 3 )
	{
		m_anglePerInsideAnim = m_groupDir / (Float)(m_animsPerGroup - 3);
	}
	else
	{
		m_anglePerInsideAnim = 0.0f;
	}
}

String CBehaviorGraphDirectionalMovementNode::CreateSocketName( Int32 groupIdx, Int32 animInGroupIdx ) const
{
	String groupName = String::Printf( TXT("%i"), groupIdx );
	if ( m_groupCount == 2 )
	{
		groupName = groupIdx == 0? TXT("Fwd") : TXT("Bwd");
	}
	if ( m_groupCount == 4 )
	{
		switch ( groupIdx )
		{
		case 0: groupName = TXT("Fwd"); break;
		case 1: groupName = TXT("Left"); break;
		case 2: groupName = TXT("Bwd"); break;
		case 3: groupName = TXT("Right"); break;
		}
	}
	//
	String animInGroupName = String::Printf( TXT("%i"), animInGroupIdx );
	if ( m_animsPerGroup == 1 )
	{
		if ( m_groupCount == 4 )
		{
			switch ( groupIdx )
			{
			case 0:	animInGroupName = TXT("F"); break;
			case 1:	animInGroupName = TXT("L"); break;
			case 2:	animInGroupName = TXT("B"); break;
			case 3:	animInGroupName = TXT("R"); break;
			}
		}
	}
	if ( m_animsPerGroup == 2 )
	{
		if ( m_groupCount == 2 )
		{
			if ( groupIdx == 0 )
			{
				animInGroupName = animInGroupIdx == 0? TXT("R*") : TXT("L*");
			}
			if ( groupIdx == 1 )
			{
				animInGroupName = animInGroupIdx == 0? TXT("L*") : TXT("R*");
			}
		}
	}
	else if ( m_animsPerGroup == 3 )
	{
		if ( m_groupCount == 2 )
		{
			if ( groupIdx == 0 )
			{
				animInGroupName = animInGroupIdx == 0? TXT("R*") :
								( animInGroupIdx == 2? TXT("L*") :
													   TXT("C") );
			}
			if ( groupIdx == 1 )
			{
				animInGroupName = animInGroupIdx == 0? TXT("L*") :
								( animInGroupIdx == 2? TXT("R*") :
													   TXT("C") );
			}
		}
	}
	else if ( m_animsPerGroup == 5 )
	{
		if ( m_groupCount == 2 )
		{
			if ( groupIdx == 0 )
			{
				switch ( animInGroupIdx )
				{
				case 0:	animInGroupName = TXT("R*"); break;
				case 1:	animInGroupName = TXT("R"); break;
				case 2:	animInGroupName = TXT("C"); break;
				case 3:	animInGroupName = TXT("L"); break;
				case 4:	animInGroupName = TXT("L*"); break;
				}
			}
			if ( groupIdx == 1 )
			{
				switch ( animInGroupIdx )
				{
				case 0:	animInGroupName = TXT("L*"); break;
				case 1:	animInGroupName = TXT("L"); break;
				case 2:	animInGroupName = TXT("C"); break;
				case 3:	animInGroupName = TXT("R"); break;
				case 4:	animInGroupName = TXT("R*"); break;
				}
			}
		}
	}
	else if ( m_animsPerGroup == 7 )
	{
		if ( m_groupCount == 2 )
		{
			if ( groupIdx == 0 )
			{
				switch ( animInGroupIdx )
				{
				case 0:	animInGroupName = TXT("R*"); break;
				case 1:	animInGroupName = TXT("R"); break;
				case 2:	animInGroupName = TXT("RC"); break;
				case 3:	animInGroupName = TXT("C"); break;
				case 4:	animInGroupName = TXT("LC"); break;
				case 5:	animInGroupName = TXT("L"); break;
				case 6:	animInGroupName = TXT("L*"); break;
				}
			}
			if ( groupIdx == 1 )
			{
				switch ( animInGroupIdx )
				{
				case 0:	animInGroupName = TXT("L*"); break;
				case 1:	animInGroupName = TXT("L"); break;
				case 2:	animInGroupName = TXT("LC"); break;
				case 3:	animInGroupName = TXT("C"); break;
				case 4:	animInGroupName = TXT("RC"); break;
				case 5:	animInGroupName = TXT("R"); break;
				case 6:	animInGroupName = TXT("R*"); break;
				}
			}
		}
	}
	return String::Printf( TXT("Input %s : %s"), groupName.AsChar(), animInGroupName.AsChar() );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphDirectionalMovementNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT( "groupCount" ) ||
		 prop->GetName() == TXT( "animsPerGroup" ) )
	{
		AutoCalculateValues();
		OnRebuildSockets();
	}
	if ( prop->GetName() == TXT( "loopCount" ) ||
		 prop->GetName() == TXT( "groupDir" ) ||
		 prop->GetName() == TXT( "animsPerGroup" ) ||
		 prop->GetName() == TXT( "extraOverlapAngle" ) )
	{
		AutoCalculateValues();
	}
}

void CBehaviorGraphDirectionalMovementNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	

	for( Int32 i = 0; i < GetNumberOfAnimations(); ++ i )
	{
		Int32 groupIdx;
		Int32 animInGroupIdx;
		GetGroupAndAnimation( i, groupIdx, animInGroupIdx );
		const String socketName = CreateSocketName( groupIdx, animInGroupIdx );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CName( socketName ) ) );
	}

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MovementDirectionWS ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( FacingDirectionWS ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( InitialMovementDirectionWS ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( InitialGroupDirMS ), false ) );
}

#endif

void CBehaviorGraphDirectionalMovementNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Get control variables
	m_cachedRequestedMovementDirectionWSValueNode = CacheValueBlock( TXT("MovementDirectionWS") );
	m_cachedRequestedFacingDirectionWSValueNode = CacheValueBlock( TXT("FacingDirectionWS") );
	m_cachedInitialMovementDirectionWSValueNode = CacheValueBlock( TXT("InitialMovementDirectionWS") );
	m_cachedInitialGroupDirMSValueNode = CacheValueBlock( TXT("InitialGroupDirMS") );

	// Cache input nodes
	m_cachedInputNodes.Clear();
	const Uint32 numInputs = GetNumberOfAnimations();
	for ( Uint32 i=0; i<numInputs; i++ )
	{
		Int32 groupIdx;
		Int32 animInGroupIdx;
		GetGroupAndAnimation( i, groupIdx, animInGroupIdx );
		const String socketName = CreateSocketName( groupIdx, animInGroupIdx );
		m_cachedInputNodes.PushBack( CacheBlock( socketName ) );
	}

	// check if cached input nodes are valid;
	m_allInputsValid = true;
	for ( TDynArray< CBehaviorGraphNode* >::const_iterator iInput = m_cachedInputNodes.Begin(); iInput != m_cachedInputNodes.End(); ++ iInput )
	{
		if ( ! (*iInput) )
		{
			m_allInputsValid = false;
			break;
		}
	}

	if ( ! m_syncMethod )
	{
		m_syncMethod = CreateObject< CBehaviorSyncMethodDuration >( this );
	}
}

Float CBehaviorGraphDirectionalMovementNode::GetRawDirForGroup( Int32 groupIdx ) const
{
	// TODO some better way to deal with Int32 to Float conversion here?
	return (Float)groupIdx * m_groupDir + m_firstGroupDirOffset; // TODO Int32 -> Float conversion
}

Float CBehaviorGraphDirectionalMovementNode::GetDirForGroup( Int32 groupIdx ) const
{
	return EulerAngles::NormalizeAngle( GetRawDirForGroup( groupIdx ) );
}

Float CBehaviorGraphDirectionalMovementNode::GetDirForGroup180( Int32 groupIdx ) const
{
	return EulerAngles::NormalizeAngle180( GetRawDirForGroup( groupIdx ) );
}

Int32 CBehaviorGraphDirectionalMovementNode::FindClosestGroupForDirectionAvoiding( Float dir, Int32 tryToAvoidGroupIdx ) const
{
	// get into 0-360 relatively to first group's dir
	dir = EulerAngles::NormalizeAngle( dir - m_firstGroupDirOffset );

	Int32 bestGroupIdx = -1;
	Float bestGroupDist = 0.0f;
	Int32 bestGroupAvoidingIdx = -1;
	Float bestGroupAvoidingDist = 0.0f;

	// check each group and find closest to desired dir. try to avoid selecting group with index "tryToAvoidGroupIdx"
	for ( Int32 groupIdx = 0; groupIdx < m_groupCount; ++ groupIdx )
	{
		// group actually covers that dir
		Float groupDist = Abs( EulerAngles::NormalizeAngle180( dir ) );
		if ( groupDist <= m_sideAngleRange )
		{
			if ( groupDist < bestGroupDist ||
				 bestGroupIdx == -1 )
			{
				bestGroupIdx = groupIdx;
			}
			if ( groupIdx != tryToAvoidGroupIdx &&
				 ( groupDist < bestGroupAvoidingDist ||
				   bestGroupAvoidingIdx == -1 ) )
			{
				bestGroupAvoidingIdx = groupIdx;
			}
		}
		dir -= m_groupDir;
	}
	return bestGroupAvoidingIdx != -1 ? bestGroupAvoidingIdx : bestGroupIdx;
}

Int32 CBehaviorGraphDirectionalMovementNode::FindGroupForDirection( Float dir, Int32 currentGroupIdx ) const
{
	const Float halfGroupDir = m_groupDir * 0.5f;

	dir += m_findGroupDirOffset;

	if ( currentGroupIdx >= 0 )
	{
		Float dirInCurrentGroup = GetDirForGroup( currentGroupIdx );
		if ( Abs( EulerAngles::NormalizeAngle180( dir - dirInCurrentGroup ) ) < halfGroupDir + m_keepInCurrentGroupAngle )
		{
			return currentGroupIdx;
		}
	}

	dir = EulerAngles::NormalizeAngle180( dir - m_firstGroupDirOffset );

	// cover half of first group
	if ( dir <= 0.0f && dir >= -halfGroupDir )
	{
		return 0;
	}

	// get into 0-360 relatively to first group's dir
	dir = EulerAngles::NormalizeAngle( dir );

	// check if we are within range (just check against maximum, half of group) and then modify dir,
	// don't check last group, just assume that if we didn't fell into others, we should be in last
	for ( Int32 groupIdx = 0; groupIdx < m_groupCount - 1; ++ groupIdx )
	{
		if ( dir <= halfGroupDir )
		{
			return groupIdx;
		}
		dir -= m_groupDir;
	}
	return m_groupCount - 1;
}

Int32 CBehaviorGraphDirectionalMovementNode::NormalizeGroupIndex( Int32 groupIdx ) const
{
	while ( groupIdx < 0.0f )
	{
		groupIdx += m_groupCount;
	}
	while ( groupIdx >= m_groupCount )
	{
		groupIdx -= m_groupCount;
	}
	return groupIdx;
}

Float CBehaviorGraphDirectionalMovementNode::TransformPTLToPT( Float ptl ) const
{
	// change into pt
	Float pt = ptl / m_loopCountFloat;
	// get into [0-1) range
	return pt >= 0.0f ? fmodf( pt, 1.0f ) : 1.0f - fmodf( -pt, 1.0f );
}

Bool CBehaviorGraphDirectionalMovementNode::AreAllInputsValid() const
{
	return m_allInputsValid;
}

void CBehaviorGraphDirectionalMovementNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	if ( ! AreAllInputsValid() )
	{
		return;
	}

	// update variables
	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->Update( context, instance, timeDelta );
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->Update( context, instance, timeDelta );
	}

	// update yaw and directions
	UpdateRotationAndDirections( instance );

	// facing update!
	// get references to variables in instance buffer
	Float& requestedFacingDirection = instance[ i_requestedFacingDirection ];
	Float& facingDirectionChange = instance[ i_facingDirectionChange ];

	Float facingDirection = 0.0f; // we want to rotate whole character
	Float prevFacingDirection = facingDirection;
	AdvanceFacingDirection( facingDirection, requestedFacingDirection, timeDelta );
	facingDirectionChange = EulerAngles::NormalizeAngle180( facingDirection - prevFacingDirection );

	// movement update!
	// get references to variables in instance buffer
	Float& requestedQuickTurnTime = instance[ i_requestedQuickTurnTime ];
	Float& currentBlendTime = instance[ i_currentBlendTime ];
	Float& requestedMovementDirection = instance[ i_requestedMovementDirection ];
	Float& movementDirection = instance[ i_movementDirection ];
	Float& additionalMovementDirection = instance[ i_additionalMovementDirection ];
	Float& prevGroupMovementDirection = instance[ i_prevGroupMovementDirection ];
	Bool& quickTurnIsAllowed = instance[ i_quickTurnIsAllowed ];
	Bool& switchGroupIsAllowed = instance[ i_switchGroupIsAllowed ];
	Bool& isBlending = instance[ i_isBlending ];
	Float& blendLeft = instance[ i_blendLeft ];
	Bool& isDoingQuickTurn = instance[ i_isDoingQuickTurn ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];
	Float& currGroupAnimBlend = instance[ i_currGroupAnimBlend ];
	Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
	Int32& prevGroupAnimIdx = instance[ i_prevGroupAnimIdx ];
	Float& prevGroupAnimBlend = instance[ i_prevGroupAnimBlend ];

	if ( ! m_doNotSwitchAnim )
	{
		// process group switching
		Bool justStartedBlending = false;
		Bool startBlendWithPredefinedPT = false;
		Bool reverseSync = false;
		Float syncBlendingOffset = 0.0f;

		// blend immediately to new diff
		Float angleDiff = Abs( EulerAngles::NormalizeAngle180( requestedMovementDirection - movementDirection ) );
		if ( angleDiff > m_angleThresholdForQuickTurn &&
			 ! isBlending &&
			 m_quickTurnBlendTime != 0.0f )
		{
			if ( requestedQuickTurnTime > 0.05f )
			{
				currentBlendTime = m_quickTurnBlendTime;
				// try to find group that will handle that movement direction and that will be different than our current group
				Int32 requestedGroupIdx = FindClosestGroupForDirectionAvoiding( requestedMovementDirection, currGroupIdx );
				ASSERT( requestedGroupIdx >= 0, TXT("Something went terribly wrong, as FindClosestGroupForDirectionAvoiding should return result in range") );
#if DEBUG_DIRECTIONAL_MOVEMENT
				RED_LOG(DirectionalMovement, TXT("DirMov:%s: do quick turn! %.3f' -> %.3f' group %i -> %i"), GetName().AsChar(), movementDirection, requestedMovementDirection, currGroupIdx, requestedGroupIdx);
#endif
				prevGroupIdx = currGroupIdx;				// store previous
				prevGroupAnimIdx = currGroupAnimIdx;
				prevGroupAnimBlend = currGroupAnimBlend;
				prevGroupMovementDirection = movementDirection;
				currGroupIdx = requestedGroupIdx;			// switch to desired
				isBlending = true;
				blendLeft = 1.0f;
				isDoingQuickTurn = true;
				if ( currGroupIdx != prevGroupIdx )
				{
					ActivateGroup( instance, currGroupIdx );
				}
				if ( quickTurnIsAllowed )
				{
					// just sync without offset
					startBlendWithPredefinedPT = false;
					syncBlendingOffset = 0.0f;
				}
				else
				{
					// if we're changing direction not to opposite, start from predefined value
					startBlendWithPredefinedPT = angleDiff < 150.0f;
					// if we're syncing, then use offset only if we're changing groups
					if ( currGroupIdx != prevGroupIdx )
					{
						// TODO this is not best solution but if it works, I'll leave it. other solution may require setting lots of things in animation
						Float absRequestedMovementDirection = Abs( requestedMovementDirection );
						reverseSync = absRequestedMovementDirection < 45.0f || absRequestedMovementDirection > 135.0f ? m_reverseSyncOnQuickTurnFwdBwd : m_reverseSyncOnQuickTurnLeftRight;
						syncBlendingOffset = m_syncBlendingOffsetPTLOnQuickTurn;
					}
					else
					{
						reverseSync = false;
						syncBlendingOffset = 0.0f;
					}
				}
				//
				movementDirection = requestedMovementDirection; // immediate!
				angleDiff = 0.0f; // current diff is 0
				justStartedBlending = true;
			}
			else
			{
				requestedMovementDirection = movementDirection; // keep moving in same direction and wait for quick turn possibility
			}
			requestedQuickTurnTime += timeDelta;
		}
		else
		{
			requestedQuickTurnTime = 0.0f;
		}

		// update blend time for quick turns (to speed up when it requires another turn
		if ( isDoingQuickTurn )
		{
			currentBlendTime = angleDiff > m_angleThresholdForQuickTurn && m_fasterQuickTurnBlendTime != 0.0f ? m_fasterQuickTurnBlendTime : m_quickTurnBlendTime;
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: currentBlendTime %.3f"), GetName().AsChar(), currentBlendTime);
#endif
		}

		// advance movement direction and clamp it to remain within group
		// store required group idx change, as sometimes (for two groups fwd:bwd) we may change movement direction from -135' to 60
		// and fastest way would be to switch group, go shorter route and switch back to group we originally started
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: movDir %.3f' -> %.3f'"), GetName().AsChar(), movementDirection, requestedMovementDirection);
#endif
		Float clampedYawDifference = 0.0f;
		Int32 requiredGroupIdxChangeDueToClamping = AdvanceMovementDirection( movementDirection, requestedMovementDirection, currGroupIdx, timeDelta, clampedYawDifference );
		additionalMovementDirection = clampedYawDifference;
		facingDirectionChange += additionalMovementDirection; // add to facing direction change, to not go off path
		//facingDirectionChange = 20.0f * timeDelta;
		if ( isBlending )
		{
			AdvanceMovementDirection( prevGroupMovementDirection, requestedMovementDirection, prevGroupIdx, timeDelta, clampedYawDifference, isDoingQuickTurn ); // allow reversing when doing quick turn
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: prevGroup movDir %.3f' -> %.3f'"), GetName().AsChar(), prevGroupMovementDirection, requestedMovementDirection);
#endif
		}

		// find requested group dir (if we know about clamping, switch to clamped one
		Int32 requestedGroupIdx = requiredGroupIdxChangeDueToClamping != 0? NormalizeGroupIndex( currGroupIdx + requiredGroupIdxChangeDueToClamping ) : FindGroupForDirection( requestedMovementDirection, currGroupIdx );
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: requested group %i (clamping? %i)"), GetName().AsChar(), requestedGroupIdx, requiredGroupIdxChangeDueToClamping);
#endif
		if ( requestedGroupIdx != currGroupIdx )
		{
			Float reqGroupDir = GetDirForGroup( requestedGroupIdx );
			Float reqRelMovDir = EulerAngles::NormalizeAngle180( movementDirection - reqGroupDir );
			Float reqRelReqMovDir = EulerAngles::NormalizeAngle180( requestedMovementDirection - reqGroupDir );
			const Float halfGroupDir = m_groupDir * 0.5f;
																										// revert to current group
			if ( Abs( reqRelMovDir ) > m_sideAngleRange ||												// if current movement is outside of requested group's coverage
				 ( reqRelMovDir * reqRelReqMovDir < 0.0f && Abs( reqRelMovDir ) > halfGroupDir ) )		// we want to switch to other group but using way too long path
			{
				requestedGroupIdx = currGroupIdx;
			}
		}

#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: %s"), GetName().AsChar(), switchGroupIsAllowed? TXT("switching group is now allowed") : TXT("switching group is not available"));
#endif

		if ( requestedGroupIdx != currGroupIdx &&		// if we want to change
			 ! isBlending &&							// if we are not blending right now
			 switchGroupIsAllowed &&					// if we are allowed to blend now
			 m_groupsBlendTime != 0.0f )				// and we are allowed to blend
		{
			currentBlendTime = m_groupsBlendTime;
			prevGroupIdx = currGroupIdx;				// store previous
			prevGroupAnimIdx = currGroupAnimIdx;
			prevGroupAnimBlend = currGroupAnimBlend;
			prevGroupMovementDirection = movementDirection;
			currGroupIdx = requestedGroupIdx;			// switch to desired
			isBlending = true;
			blendLeft = 1.0f;
			isDoingQuickTurn = false;
			if ( currGroupIdx != prevGroupIdx )
			{
				ActivateGroup( instance, currGroupIdx );
			}
			justStartedBlending = true;
#if DEBUG_DIRECTIONAL_MOVEMENT
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: switch group! group %i -> %i, blendtime %.3f "), GetName().AsChar(), prevGroupIdx, currGroupIdx, currentBlendTime);
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: mov dir %.3f -> req mov dir %.3f "), GetName().AsChar(), movementDirection, requestedMovementDirection);
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: requiredGroupIdxChangeDueToClamping %i "), GetName().AsChar(), requiredGroupIdxChangeDueToClamping);
#endif
		}

#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: %s : left %.3f"), GetName().AsChar(), isBlending? TXT("blending") : TXT("flat"), blendLeft);
#endif

		// blend group

		if ( isBlending && currentBlendTime != 0.0f )
		{
			// linear blend!
			Float blendSpeed = 1.0f / currentBlendTime;
			blendLeft = BlendToWithSpeed( blendLeft, 0.0f, blendSpeed, timeDelta );
			if ( blendLeft < 0.01f )
			{
				// we finished blending
				isBlending = false;
				isDoingQuickTurn = false;
				if ( prevGroupIdx != currGroupIdx )
				{
					DeactivateGroup( instance, prevGroupIdx );
				}
			}
		}

		// store for reference to know if we switched anims
		Int32 prevCurrGroupAnimIdx = currGroupAnimIdx;
		Float prevCurrGroupAnimBlend = currGroupAnimBlend;

		// calculate current active anims
		UpdateAnimBlendForGroup( instance, movementDirection, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: current %i : %i ~ %.3f"), GetName().AsChar(), currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
#endif

		if ( justStartedBlending )
		{
#if DEBUG_DIRECTIONAL_MOVEMENT
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: just started blending"), GetName().AsChar() );
#endif
			if ( isDoingQuickTurn && currGroupIdx != prevGroupIdx && startBlendWithPredefinedPT ) // don't sync for quick turn that wasn't synced (unless we're within same group)
			{
				// base starting time on current stance
				StartAnimsGroupAt( instance, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend, ChooseStartingPTLBasingOnPreviousPose( instance, currGroupIdx ) );
			}
			else
			{
				// synchronize current to prev
				SynchronizeAnimsBetweenGroups( instance, prevGroupIdx, prevGroupAnimIdx, prevGroupAnimBlend, currGroupIdx, currGroupAnimIdx, reverseSync, syncBlendingOffset );
			}
		}
		else if ( prevCurrGroupAnimIdx != currGroupAnimIdx && ! isDoingQuickTurn )
		{
#if DEBUG_DIRECTIONAL_MOVEMENT
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: changed anims within group"), GetName().AsChar() );
#endif
			// synchronize current anims to previously used current anims
			SynchronizeAnimsBetweenGroups( instance, currGroupIdx, prevCurrGroupAnimIdx, prevCurrGroupAnimBlend, currGroupIdx, currGroupAnimIdx );
		}
		else
		{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: sync?"), GetName().AsChar() );
#endif
			SynchronizeAnimsGroup( instance, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
		}

		// calculate prev anims, sync them to current anims and update
		if ( isBlending )
		{
			UpdateAnimBlendForGroup( instance, prevGroupMovementDirection, prevGroupIdx, prevGroupAnimIdx, prevGroupAnimBlend );
			if ( ! isDoingQuickTurn ) // don't sync
			{
				SynchronizeAnimsBetweenGroups( instance, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend, prevGroupIdx, prevGroupAnimIdx );
			}
			UpdateAnims( context, instance, timeDelta, prevGroupIdx, prevGroupAnimIdx, prevGroupAnimBlend );
		}
	}

	UpdateAnims( context, instance, timeDelta, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );

	// store internal variables
	if ( instance[ i_hasMovementDirectionInternalVariable ] )
	{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: provide internal mov dir %.3f => %.3f"), GetName().AsChar(), movementDirection, EulerAngles::NormalizeAngle( movementDirection ) );
#endif
		instance.SetInternalFloatValue( m_movementDirectionInternalVariableName, EulerAngles::NormalizeAngle( movementDirection ) ); // 0-360'
	}
	if ( instance[ i_hasGroupDirInternalVariable ] )
	{
		ASSERT( GetDirForGroup( currGroupIdx ) >= 0.0f && GetDirForGroup( currGroupIdx ) <= 360.0f, TXT("GetDirForGroup should always return value 0-360'") );
		instance.SetInternalFloatValue( m_groupDirInternalVariableName, GetDirForGroup( currGroupIdx ) ); // GetDirForGroup gives result in 0-360'
	}
}

void CBehaviorGraphDirectionalMovementNode::AdvanceFacingDirection( Float& refFacingDirection, const Float requestedFacingDirection, Float timeDelta ) const
{
	if ( m_facingDirBlendTime == 0.0f )
	{
		return;
	}
	// blend direction (as requested), do it in space relative to current facing direction and then move it back to -180:180 range
	Float facingDirection = refFacingDirection;
	const Float localRequestedFacingDirection = EulerAngles::NormalizeAngle180( requestedFacingDirection - facingDirection );
	facingDirection = EulerAngles::NormalizeAngle180( facingDirection + BlendToWithBlendTimeAndSpeed( 0.0f, localRequestedFacingDirection, m_facingDirBlendTime, m_facingDirMaxSpeedChange, timeDelta ) );
	refFacingDirection = facingDirection;
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: advance facing direction %.3f => %.3f"), GetName().AsChar(), facingDirection, requestedFacingDirection );
#endif
}

Int32 CBehaviorGraphDirectionalMovementNode::AdvanceMovementDirection( Float& refMovementDirection, const Float requestedMovementDirection, const Int32 groupIdx, Float timeDelta, Float& outClampDifference, Bool allowReversing ) const
{
	if ( m_movementDirBlendTime == 0.0f )
	{
		return 0;
	}
	// blend direction (as requested), do it in space relative to current movement direction and then move it back to -180:180 range
	Float movementDirection = refMovementDirection;
	Float localRequestedMovementDirection = EulerAngles::NormalizeAngle180( requestedMovementDirection - movementDirection );
	if ( allowReversing )
	{
		// reverse
		if ( Abs( localRequestedMovementDirection ) > 90.0f )
		{
			localRequestedMovementDirection = EulerAngles::NormalizeAngle180( localRequestedMovementDirection + 180.0f );
		}
	}
	movementDirection = EulerAngles::NormalizeAngle180( movementDirection + BlendToWithBlendTimeAndSpeed( 0.0f, localRequestedMovementDirection, m_movementDirBlendTime, m_movementDirMaxSpeedChange, timeDelta ) );
	const Float currGroupDir = GetDirForGroup( groupIdx );
	const Float notClampedMovementDirection = movementDirection;
	const Float localMovementDirection = EulerAngles::NormalizeAngle180( movementDirection - currGroupDir );
	movementDirection = EulerAngles::NormalizeAngle180( currGroupDir + Clamp( localMovementDirection, -m_sideAngleRange, m_sideAngleRange ) );
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: movDir (clamped) %.3f' -> %.3f'"), GetName().AsChar(), notClampedMovementDirection, movementDirection);
#endif
	refMovementDirection = movementDirection;
	outClampDifference = EulerAngles::NormalizeAngle180( notClampedMovementDirection - movementDirection );
	return outClampDifference != 0.0f ? ( localMovementDirection < 0.0f? -1 : 1 ) : 0;
}

void CBehaviorGraphDirectionalMovementNode::UpdateAnims( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const
{
	CBehaviorGraphNode* rightInput;
	CBehaviorGraphNode* leftInput;
	GetInputs( groupIdx, groupAnimIdx, rightInput, leftInput );

	if ( rightInput != leftInput && m_syncMethod )
	{
		m_syncMethod->Synchronize( instance, rightInput, leftInput, groupAnimBlend, timeDelta );
	}

	rightInput->Update( context, instance, timeDelta );
	if ( rightInput != leftInput )
	{
		leftInput->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphDirectionalMovementNode::StartAnimsGroupAt( CBehaviorGraphInstance& instance, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend, Float startAtPTL ) const
{
	if ( m_syncMethod )
	{
		CBehaviorGraphNode* moreImportantInput;
		CBehaviorGraphNode* lessImportantInput;
		GetInputsBasedOnImportance( groupIdx, groupAnimIdx, groupAnimBlend, moreImportantInput, lessImportantInput );
		CSyncInfo syncInfo;
		moreImportantInput->GetSyncInfo( instance, syncInfo );
		syncInfo.m_currTime = TransformPTLToPT( startAtPTL ) * syncInfo.m_totalTime;
		syncInfo.m_prevTime = syncInfo.m_currTime;
		m_syncMethod->SynchronizeTo( instance, moreImportantInput, syncInfo );
		if ( moreImportantInput != lessImportantInput )
		{
			m_syncMethod->SynchronizeTo( instance, lessImportantInput, syncInfo );
		}
	}
}

void CBehaviorGraphDirectionalMovementNode::SynchronizeAnimsGroup( CBehaviorGraphInstance& instance, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const
{
	if ( m_syncMethod && m_animsPerGroup != 1 )
	{
		CBehaviorGraphNode* moreImportantInput;
		CBehaviorGraphNode* lessImportantInput;
		GetInputsBasedOnImportance( groupIdx, groupAnimIdx, groupAnimBlend, moreImportantInput, lessImportantInput );
		CSyncInfo syncInfo;
		moreImportantInput->GetSyncInfo( instance, syncInfo );
		m_syncMethod->SynchronizeTo( instance, lessImportantInput, syncInfo );
	}
}

void CBehaviorGraphDirectionalMovementNode::SynchronizeAnimsBetweenGroups( CBehaviorGraphInstance& instance, Int32 srcGroupIdx, Int32 srcGroupAnimIdx, Float srcGroupAnimBlend, Int32 destGroupIdx, Int32 destGroupAnimIdx, Bool reverseSync, Float offsetPTL ) const
{
	if ( m_syncMethod )
	{
		CBehaviorGraphNode* moreImportantSrcInput = GetMoreImportantInput( srcGroupIdx, srcGroupAnimIdx, srcGroupAnimBlend );
		CSyncInfo syncInfo;
		moreImportantSrcInput->GetSyncInfo( instance, syncInfo );
		CBehaviorGraphNode* rightDestInput;
		CBehaviorGraphNode* leftDestInput;
		GetInputs( destGroupIdx, destGroupAnimIdx, rightDestInput, leftDestInput );
		if ( reverseSync || offsetPTL != 0.0f )
		{
#if DEBUG_DIRECTIONAL_MOVEMENT
			RED_LOG(DirectionalMovement, TXT("Sync from %.3f of %.3f (%s, offset PTL %.3f)"), syncInfo.m_currTime, syncInfo.m_totalTime, reverseSync? TXT("reverse sync") : TXT("don't reverse"), offsetPTL);
#endif
			if ( reverseSync )
			{
				syncInfo.m_currTime = fmodf( ( 1.0f - syncInfo.m_currTime / syncInfo.m_totalTime + TransformPTLToPT( offsetPTL ) ) * syncInfo.m_totalTime, syncInfo.m_totalTime );
			}
			else
			{
				syncInfo.m_currTime = fmodf( syncInfo.m_currTime + TransformPTLToPT( offsetPTL ) * syncInfo.m_totalTime, syncInfo.m_totalTime );
			}
			syncInfo.m_prevTime = syncInfo.m_currTime;
#if DEBUG_DIRECTIONAL_MOVEMENT
			RED_LOG(DirectionalMovement, TXT("Sync to %.3f of %.3f"), syncInfo.m_currTime, syncInfo.m_totalTime);
#endif
		}
		m_syncMethod->SynchronizeTo( instance, rightDestInput, syncInfo );
		if ( rightDestInput != leftDestInput )
		{
			m_syncMethod->SynchronizeTo( instance, leftDestInput, syncInfo );
		}
	}
}

CBehaviorGraphNode* CBehaviorGraphDirectionalMovementNode::GetMoreImportantInput( Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const
{
	const Int32 index = m_animsPerGroup == 1? groupIdx : groupIdx * m_animsPerGroup + groupAnimIdx + ( groupAnimBlend < 0.5f ? 0 : 1 );
	return m_cachedInputNodes[ index ];
}

void CBehaviorGraphDirectionalMovementNode::GetInputsBasedOnImportance( Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend, CBehaviorGraphNode*& outMoreImportant, CBehaviorGraphNode*& outLessImportant ) const
{
	const Int32 index = m_animsPerGroup == 1? groupIdx : groupIdx * m_animsPerGroup + groupAnimIdx;
	const Int32 moreImportantOffset = ( groupAnimBlend < 0.5f ? 0 : 1 );
	outMoreImportant = m_cachedInputNodes[ index + moreImportantOffset ];
	outLessImportant = m_animsPerGroup == 1 ? outMoreImportant : m_cachedInputNodes[ index + 1 - moreImportantOffset ];
	ASSERT( outMoreImportant && outLessImportant, TXT("Not all inputs are defined and check for that was not done!") );
}

void CBehaviorGraphDirectionalMovementNode::GetInputs( Int32 groupIdx, Int32 groupAnimIdx, CBehaviorGraphNode*& outRight, CBehaviorGraphNode*& outLeft ) const
{
	const Int32 index = m_animsPerGroup == 1? groupIdx : groupIdx * m_animsPerGroup + groupAnimIdx;
	outLeft = m_cachedInputNodes[ index ];
	outRight = m_animsPerGroup == 1? outLeft : m_cachedInputNodes[ index + 1 ];
	ASSERT( outRight && outLeft, TXT("Not all inputs are defined and check for that was not done!") );
}

void CBehaviorGraphDirectionalMovementNode::UpdateAnimBlendForGroup( CBehaviorGraphInstance& instance, const Float movementDirection, const Int32 groupIdx, Int32& outGroupAnimIdx, Float& outGroupAnimBlend ) const
{
	Float groupDir = GetDirForGroup( groupIdx );
	Float relMovDir = EulerAngles::NormalizeAngle180( movementDirection - groupDir );

	// we need some defaults here
	Int32 groupAnimIdx = 0;
	Float groupAnimBlend = 0.0f;

	Float rightDir = -m_sideAngleRange;
	Float leftDir = -rightDir;

	const Float halfGroupDir = m_groupDir * 0.5f;

#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: in md:%.3f gd:%.3f rel:%.3f"), GetName().AsChar(), movementDirection, groupDir, relMovDir );
#endif

	if ( m_animsPerGroup > 3 )
	{
		if ( relMovDir < -halfGroupDir )
		{
			// right side
			groupAnimIdx = 0;
			rightDir = -m_sideAngleRange;
			leftDir = -halfGroupDir;
		}
		else if ( relMovDir >= halfGroupDir )
		{
			// left side
			groupAnimIdx = m_animsPerGroup - 2;
			rightDir = halfGroupDir;
			leftDir = m_sideAngleRange;
		}
		else
		{
			// between
			groupAnimIdx = 1;
			rightDir = -halfGroupDir;
			leftDir = rightDir + m_anglePerInsideAnim;
			while ( leftDir < halfGroupDir )
			{
				if ( relMovDir <= leftDir )
				{
					break;
				}
				++ groupAnimIdx;
				rightDir = leftDir;
				leftDir += m_anglePerInsideAnim;
			}
		}
	}
	else if ( m_animsPerGroup == 1 )
	{
		// just one anim
		groupAnimIdx = 0;
		rightDir = -m_sideAngleRange;
		leftDir = -m_sideAngleRange;
	}
	else if ( m_animsPerGroup == 2 )
	{
		// just side anims
		groupAnimIdx = 0;
		rightDir = -m_sideAngleRange;
		leftDir = -m_sideAngleRange;
	}
	else if ( m_animsPerGroup == 3 )
	{
		// right forward left
		if ( relMovDir < 0.0f )
		{
			groupAnimIdx = 0;
			rightDir = -m_sideAngleRange;
			leftDir = 0.0f;
		}
		else
		{
			groupAnimIdx = 1;
			rightDir = 0.0f;
			leftDir = m_sideAngleRange;
		}
	}

	// calculate blend between dirs
	const Float rightToLeft = leftDir - rightDir;
	if ( rightToLeft == 0.0f )
	{
		groupAnimBlend = 0.0f;
	}
	else
	{
		groupAnimBlend = Clamp( ( relMovDir - rightDir ) / ( leftDir - rightDir ), 0.0f, 1.0f);
		if ( m_singleAnimOnly )
		{
			groupAnimBlend = groupAnimBlend < 0.5f? 0.0f : 1.0f;
		}
	}

#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: out %i:%.3f"), GetName().AsChar(), groupAnimIdx, groupAnimBlend );
#endif

	outGroupAnimIdx = groupAnimIdx;
	outGroupAnimBlend = groupAnimBlend;
}

void CBehaviorGraphDirectionalMovementNode::ActivateGroup( CBehaviorGraphInstance& instance, Int32 groupIdx ) const
{
	Int32 groupStart = groupIdx * m_animsPerGroup;
	Int32 groupEnd = ( groupIdx + 1 ) * m_animsPerGroup;
	for ( Int32 inGroup = groupStart; inGroup < groupEnd; ++ inGroup )
	{
		m_cachedInputNodes[ inGroup ]->Activate( instance );
	}
}

void CBehaviorGraphDirectionalMovementNode::DeactivateGroup( CBehaviorGraphInstance& instance, Int32 groupIdx ) const
{
	Int32 groupStart = groupIdx * m_animsPerGroup;
	Int32 groupEnd = ( groupIdx + 1 ) * m_animsPerGroup;
	for ( Int32 inGroup = groupStart; inGroup < groupEnd; ++ inGroup )
	{
		m_cachedInputNodes[ inGroup ]->Deactivate( instance );
	}
}

void CBehaviorGraphDirectionalMovementNode::UpdateRotationAndDirections( CBehaviorGraphInstance& instance, Bool initial ) const
{
	// update yaw
	Float& currentYaw = instance[ i_currentRotationYaw ];
	const Float prevYaw = currentYaw;
	currentYaw = instance.GetAnimatedComponent()->GetWorldYaw();
	// apply yaw change
	Float yawChange = EulerAngles::NormalizeAngle180( currentYaw - prevYaw );
	instance[ i_movementDirection ] -= yawChange;
	// update variables
	// movement direction
	bool movementDirectionRead = false;
	if ( initial )
	{
		if ( m_cachedInitialMovementDirectionWSValueNode )
		{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: use initial variable provided for initial movement direction (connected)"), GetName().AsChar() );
#endif
			instance[ i_requestedMovementDirection ] = EulerAngles::NormalizeAngle180( m_cachedInitialMovementDirectionWSValueNode->GetValue( instance ) - currentYaw );
			movementDirectionRead = true;
		}
		else if ( m_useDefinedInternalVariablesAsInitialInput )
		{
			if ( instance[ i_hasMovementDirectionInternalVariable ] )
			{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
				RED_LOG(DirectionalMovement, TXT("DirMov:%s: use initial variable provided for initial movement direction (own access)"), GetName().AsChar() );
#endif
				instance[ i_requestedMovementDirection ] = EulerAngles::NormalizeAngle180( instance.GetInternalFloatValue( m_movementDirectionInternalVariableName ) );
				movementDirectionRead = true;
			}
		}
	}
	if ( ! movementDirectionRead )
	{
		if ( m_cachedRequestedMovementDirectionWSValueNode )
		{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: use variable provided for movement direction (connected)"), GetName().AsChar() );
#endif
			instance[ i_requestedMovementDirection ] = EulerAngles::NormalizeAngle180( m_cachedRequestedMovementDirectionWSValueNode->GetValue( instance ) - currentYaw );
		}
		else
		{
			if ( m_useDefinedVariablesAsRequestedInput && instance[ i_hasRequestedMovementDirectionVariable ] )
			{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
				RED_LOG(DirectionalMovement, TXT("DirMov:%s: use variable provided for movement direction (own access)"), GetName().AsChar() );
#endif
				instance[ i_requestedMovementDirection ] = EulerAngles::NormalizeAngle180( instance.GetFloatValue( m_requestedMovementDirectionVariableName ) - currentYaw );
				movementDirectionRead = true;
			}
			else
			{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
				RED_LOG(DirectionalMovement, TXT("DirMov:%s: clear movement direction"), GetName().AsChar() );
#endif
				instance[ i_requestedMovementDirection ] = 0.0f;
			}
		}
	}
	// facing direction
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		instance[ i_requestedFacingDirection ] = EulerAngles::NormalizeAngle180( m_cachedRequestedFacingDirectionWSValueNode->GetValue( instance ) - currentYaw );
	}
	else
	{
		if ( m_useDefinedVariablesAsRequestedInput && instance[ i_hasRequestedFacingDirectionVariable ] )
		{
			instance[ i_requestedFacingDirection ] = EulerAngles::NormalizeAngle180( instance.GetFloatValue( m_requestedFacingDirectionVariableName ) - currentYaw );
		}
		else
		{
			instance[ i_requestedFacingDirection ] = 0.0f;
		}
	}
}

void CBehaviorGraphDirectionalMovementNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( ! AreAllInputsValid() )
	{
		return;
	}

	Bool& quickTurnIsAllowed = instance[ i_quickTurnIsAllowed ];
	Bool& switchGroupIsAllowed = instance[ i_switchGroupIsAllowed ];
	Bool& isBlending = instance[ i_isBlending ];
	Float& blendLeft = instance[ i_blendLeft ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];
	Float& currGroupAnimBlend = instance[ i_currGroupAnimBlend ];

	if ( isBlending )
	{
		Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
		Int32& prevGroupAnimIdx = instance[ i_prevGroupAnimIdx ];
		Float& prevGroupAnimBlend = instance[ i_prevGroupAnimBlend ];

		CCacheBehaviorGraphOutput cachePoseCurr( context );
		CCacheBehaviorGraphOutput cachePosePrev( context );

		SBehaviorGraphOutput* tempCurr = cachePoseCurr.GetPose();
		SBehaviorGraphOutput* tempPrev = cachePosePrev.GetPose();
		if ( tempCurr && tempPrev )
		{
			SamplePosesForGroup( context, instance, *tempCurr, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
			//AdjustTranslation( *tempCurr, instance[ i_additionalMovementDirection ] ); // TODO?
			SamplePosesForGroup( context, instance, *tempPrev, prevGroupIdx, prevGroupAnimIdx, prevGroupAnimBlend );

			BlendPoses( context, instance, output, *tempCurr, *tempPrev, blendLeft, true );
		}
	}
	else
	{
		SamplePosesForGroup( context, instance, output, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
		//AdjustTranslation( output, instance[ i_additionalMovementDirection ] ); // TODO?
	}

	// go through events to learn if we can switch group now or do quick turn
	quickTurnIsAllowed = false;
	switchGroupIsAllowed = false;
	CAnimationEventFired* firedEvent = output.m_eventsFired;
	for ( Int32 idx = output.m_numEventsFired; idx > 0; -- idx, ++ firedEvent )
	{
		if ( firedEvent->GetEventName() == CNAME( AllowGroupDirChange ) )
		{
			switchGroupIsAllowed = true;
		}
		if ( firedEvent->GetEventName() == CNAME( AllowQuickTurn ) && firedEvent->m_alpha > 0.7f )
		{
			quickTurnIsAllowed = true;
		}
	}

	Float facingDirectionChange = instance[ i_facingDirectionChange ];
	AddYawRotationToAnimQsTransform( output.m_deltaReferenceFrameLocal, facingDirectionChange );
	//AdjustTranslation( output, -facingDirectionChange ); // TODO?
	AdjustTranslation( output, instance[ i_additionalMovementDirection ] ); // TODO
}

void CBehaviorGraphDirectionalMovementNode::SamplePosesForGroup( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const Int32 groupIdx, const Int32 groupAnimIdx, const Float groupAnimBlend ) const
{
	CBehaviorGraphNode* rightInput;
	CBehaviorGraphNode* leftInput;
	GetInputs( groupIdx, groupAnimIdx, leftInput, rightInput );

	if ( groupAnimBlend == 0.0f || m_animsPerGroup == 1 )
	{
		rightInput->Sample( context, instance, output );
	}
	else if ( groupAnimBlend == 1.0f )
	{
		leftInput->Sample( context, instance, output );
	}
	else
	{
		CCacheBehaviorGraphOutput cachePoseRight( context );
		CCacheBehaviorGraphOutput cachePoseLeft( context );

		SBehaviorGraphOutput* tempRight = cachePoseRight.GetPose();
		SBehaviorGraphOutput* tempLeft = cachePoseLeft.GetPose();

		if ( tempRight && tempLeft )
		{
			rightInput->Sample( context, instance, *tempRight );
			leftInput->Sample( context, instance, *tempLeft );

			BlendPoses( context, instance, output, *tempRight, *tempLeft, groupAnimBlend );
		}
	}
}

void CBehaviorGraphDirectionalMovementNode::AdjustTranslation( SBehaviorGraphOutput &output, Float byYaw ) const
{
	// maintain length of translation and blend with yaw
	Vector outputTranslation = AnimVectorToVector( GetTranslation( output.m_deltaReferenceFrameLocal ) );
	// we always have non zero translations as input
	if ( outputTranslation.SquareMag2() != 0.0f )
	{
		// Inputs for directional movement should have non zero translation, unless this is stop anim
		Float outputTranslationYaw = EulerAngles::YawFromXY( outputTranslation.X, outputTranslation.Y ) + byYaw;
		outputTranslation = EulerAngles::YawToVector( outputTranslationYaw ) * outputTranslation.Mag3();
		SetTranslation( output.m_deltaReferenceFrameLocal, VectorToAnimVector( outputTranslation ) );
	}
}

void CBehaviorGraphDirectionalMovementNode::BlendPoses( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorGraphOutput &first, const SBehaviorGraphOutput &second, const Float alpha, const Bool takeMEfromFirstInput ) const
{
#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() <= BL_Lod2 )
	{
		// Interpolate poses
		output.SetInterpolate( first, second, alpha );
	}
	else
	{
		// Interpolate poses
		output.SetInterpolateME( first, second, alpha );
	}
#else
	output.SetInterpolate( first, second, alpha );
#endif

	const Float invAlpha = 1.0f - alpha;

	if ( takeMEfromFirstInput )
	{
		if ( ! instance[ i_isDoingQuickTurn ] )
		{
			// not doing quick turn, just take first input as current one
			output.m_deltaReferenceFrameLocal = first.m_deltaReferenceFrameLocal;
		}
		// otherwise leave it as blend
	}
	else if ( ! m_useSimpleBlendForMovementDelta ) // for simple blend, just leave what there is
	{
		// maintain length of translation and blend with yaw
		Vector firstTranslation = AnimVectorToVector( GetTranslation( first.m_deltaReferenceFrameLocal ) );
		Vector secondTranslation = AnimVectorToVector( GetTranslation( second.m_deltaReferenceFrameLocal ) );
		// we always have non zero translations as input
		if ( firstTranslation.SquareMag2() != 0.0f && secondTranslation.SquareMag2() != 0.0f )
		{
			// Inputs for directional movement should have non zero translation, unless this is stop anim
			Float firstTranslationYaw = EulerAngles::YawFromXY( firstTranslation.X, firstTranslation.Y );
			Float secondTranslationYaw = EulerAngles::YawFromXY( secondTranslation.X, secondTranslation.Y );
			// blend yaw to have actual range covered exactly as we want it (0.5 between 0' and 135' should result in 67.5', 0.25 in same case should give 33.75')
			Float outputTranslationYaw = firstTranslationYaw + EulerAngles::NormalizeAngle180( secondTranslationYaw - firstTranslationYaw ) * alpha;
			Vector outputTranslation = EulerAngles::YawToVector( outputTranslationYaw ) * ( firstTranslation.Mag3() * invAlpha + secondTranslation.Mag3() * alpha );
			SetTranslation( output.m_deltaReferenceFrameLocal, VectorToAnimVector( outputTranslation ) );
		}
	}

	// Merge events and used anims
	output.MergeEventsAndUsedAnims( first, second, invAlpha, alpha );
}

void CBehaviorGraphDirectionalMovementNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( ! AreAllInputsValid() )
	{
		return;
	}

	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];
	Float& currGroupAnimBlend = instance[ i_currGroupAnimBlend ];

	CBehaviorGraphNode* moreImportantCurrInput = GetMoreImportantInput( currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
	moreImportantCurrInput->GetSyncInfo( instance, info );
}

void CBehaviorGraphDirectionalMovementNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( ! AreAllInputsValid() )
	{
		return;
	}

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->SynchronizeTo( instance, info );
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->SynchronizeTo( instance, info );
	}
	if ( m_cachedInitialMovementDirectionWSValueNode )
	{
		m_cachedInitialMovementDirectionWSValueNode->SynchronizeTo( instance, info );
	}
	if ( m_cachedInitialGroupDirMSValueNode )
	{
		m_cachedInitialGroupDirMSValueNode->SynchronizeTo( instance, info );
	}

	Bool& isBlending = instance[ i_isBlending ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];

	SynchronizeGroupTo( instance, info, currGroupIdx, currGroupAnimIdx );
	if ( isBlending )
	{
		Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
		Int32& prevGroupAnimIdx = instance[ i_prevGroupAnimIdx ];
		SynchronizeGroupTo( instance, info, prevGroupIdx, prevGroupAnimIdx );
	}
}

void CBehaviorGraphDirectionalMovementNode::SynchronizeGroupTo( CBehaviorGraphInstance& instance, const CSyncInfo &info, Int32 groupIdx, Int32 groupAnimIdx ) const
{
	CBehaviorGraphNode* rightInput;
	CBehaviorGraphNode* leftInput;
	GetInputs( groupIdx, groupAnimIdx, leftInput, rightInput );

	if ( m_syncMethod )
	{
		m_syncMethod->SynchronizeTo( instance, rightInput, info );
		if ( rightInput != leftInput )
		{
			m_syncMethod->SynchronizeTo( instance, leftInput, info );
		}
	}
	else
	{
		rightInput->SynchronizeTo( instance, info );
		if ( rightInput != leftInput )
		{
			leftInput->SynchronizeTo( instance, info );
		}
	}
}

Bool CBehaviorGraphDirectionalMovementNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( ! AreAllInputsValid() )
	{
		return false;
	}

	Bool retVal = false;

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		retVal = m_cachedRequestedMovementDirectionWSValueNode->ProcessEvent( instance, event ) || retVal;
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		retVal = m_cachedRequestedFacingDirectionWSValueNode->ProcessEvent( instance, event ) || retVal;
	}
	if ( m_cachedInitialMovementDirectionWSValueNode )
	{
		retVal = m_cachedInitialMovementDirectionWSValueNode->ProcessEvent( instance, event ) || retVal;
	}
	if ( m_cachedInitialGroupDirMSValueNode )
	{
		retVal = m_cachedInitialGroupDirMSValueNode->ProcessEvent( instance, event ) || retVal;
	}

	Bool& isBlending = instance[ i_isBlending ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];

	retVal = ProcessEventInGroup( instance, event, currGroupIdx, currGroupAnimIdx ) || retVal;
	if ( isBlending )
	{
		Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
		Int32& prevGroupAnimIdx = instance[ i_prevGroupAnimIdx ];
		retVal = ProcessEventInGroup( instance, event, prevGroupIdx, prevGroupAnimIdx ) || retVal;
	}
	return retVal;
}

Bool CBehaviorGraphDirectionalMovementNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( ! AreAllInputsValid() )
	{
		return false;
	}

	Bool retVal = false;

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		retVal = m_cachedRequestedMovementDirectionWSValueNode->ProcessForceEvent( instance, event ) || retVal;
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		retVal = m_cachedRequestedFacingDirectionWSValueNode->ProcessForceEvent( instance, event ) || retVal;
	}
	if ( m_cachedInitialMovementDirectionWSValueNode )
	{
		retVal = m_cachedInitialMovementDirectionWSValueNode->ProcessForceEvent( instance, event ) || retVal;
	}
	if ( m_cachedInitialGroupDirMSValueNode )
	{
		retVal = m_cachedInitialGroupDirMSValueNode->ProcessForceEvent( instance, event ) || retVal;
	}

	Bool& isBlending = instance[ i_isBlending ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];

	retVal = ProcessForceEventInGroup( instance, event, currGroupIdx, currGroupAnimIdx ) || retVal;
	if ( isBlending )
	{
		Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
		Int32& prevGroupAnimIdx = instance[ i_prevGroupAnimIdx ];
		retVal = ProcessForceEventInGroup( instance, event, prevGroupIdx, prevGroupAnimIdx ) || retVal;
	}
	return retVal;
}

Bool CBehaviorGraphDirectionalMovementNode::ProcessEventInGroup( CBehaviorGraphInstance& instance, const CBehaviorEvent &event, Int32 groupIdx, Int32 groupAnimIdx ) const
{
	CBehaviorGraphNode* rightInput;
	CBehaviorGraphNode* leftInput;
	GetInputs( groupIdx, groupAnimIdx, leftInput, rightInput );
	Bool retVal = false;
	retVal = rightInput->ProcessEvent( instance, event ) || retVal;
	retVal = ( rightInput != leftInput && leftInput->ProcessEvent( instance, event ) ) || retVal;
	return retVal;
}

Bool CBehaviorGraphDirectionalMovementNode::ProcessForceEventInGroup( CBehaviorGraphInstance& instance, const CBehaviorEvent &event, Int32 groupIdx, Int32 groupAnimIdx ) const
{
	CBehaviorGraphNode* rightInput;
	CBehaviorGraphNode* leftInput;
	GetInputs( groupIdx, groupAnimIdx, leftInput, rightInput );
	Bool retVal = false;
	retVal = rightInput->ProcessForceEvent( instance, event ) || retVal;
	retVal = ( rightInput != leftInput && leftInput->ProcessForceEvent( instance, event ) )|| retVal;
	return retVal;
}

void CBehaviorGraphDirectionalMovementNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( ! AreAllInputsValid() )
	{
		return;
	}

	// activate variables
	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->Activate( instance );
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->Activate( instance );
	}
	if ( m_cachedInitialMovementDirectionWSValueNode )
	{
		m_cachedInitialMovementDirectionWSValueNode->Activate( instance );
	}
	if ( m_cachedInitialGroupDirMSValueNode )
	{
		m_cachedInitialGroupDirMSValueNode->Activate( instance );
	}

	// update yaw and directions
	instance[ i_currentRotationYaw ] = instance.GetAnimatedComponent()->GetWorldYaw();
	UpdateRotationAndDirections( instance, true );

	// get references to variables in instance buffer
	//Float& requestedFacingDirection = instance[ i_requestedFacingDirection ];
	Float& facingDirectionChange = instance[ i_facingDirectionChange ];
	Float& requestedMovementDirection = instance[ i_requestedMovementDirection ];
	Float& movementDirection = instance[ i_movementDirection ];
	//Float& additionalMovementDirection = instance[ i_additionalMovementDirection ];
	Bool& quickTurnIsAllowed = instance[ i_quickTurnIsAllowed ];
	Bool& switchGroupIsAllowed = instance[ i_switchGroupIsAllowed ];
	Bool& isBlending = instance[ i_isBlending ];
	Bool& isDoingQuickTurn = instance[ i_isDoingQuickTurn ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];
	Float& currGroupAnimBlend = instance[ i_currGroupAnimBlend ];
	
	movementDirection = requestedMovementDirection;

	// don't rotate yet
	facingDirectionChange = 0.0f;

	// switch to new group
	Float lookForGroupInDir = movementDirection;
	if ( m_cachedInitialGroupDirMSValueNode )
	{
		lookForGroupInDir = m_cachedInitialGroupDirMSValueNode->GetValue( instance );
	}
	else if ( m_useDefinedInternalVariablesAsInitialInput )
	{
		if ( instance[ i_hasGroupDirInternalVariable ] )
		{
#if DEBUG_DIRECTIONAL_MOVEMENT && DEBUG_DIRECTIONAL_MOVEMENT_EVERY_FRAME
			RED_LOG(DirectionalMovement, TXT("DirMov:%s: use initial variable provided for initial movement direction (own access)"), GetName().AsChar() );
#endif
			lookForGroupInDir = EulerAngles::NormalizeAngle180( instance.GetInternalFloatValue( m_groupDirInternalVariableName ) );
		}
	}
	currGroupIdx = FindGroupForDirection( lookForGroupInDir );
	quickTurnIsAllowed = false;
	switchGroupIsAllowed = false;
	isBlending = false;
	isDoingQuickTurn = false;
	ActivateGroup( instance, currGroupIdx );

#if DEBUG_DIRECTIONAL_MOVEMENT
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: activated!"), GetName().AsChar() );
#endif

	// calculate current active anims
	UpdateAnimBlendForGroup( instance, movementDirection, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );

	// initial start
	StartAnimsGroupAt( instance, currGroupIdx, currGroupAnimIdx, currGroupAnimBlend, ChooseStartingPTLBasingOnPreviousPose( instance, currGroupIdx ) ); // TODO not at 0?

#if DEBUG_DIRECTIONAL_MOVEMENT
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: initial movement direction %.3f' -> %.3f'"), GetName().AsChar(), movementDirection, requestedMovementDirection);
	RED_LOG(DirectionalMovement, TXT("DirMov:%s: initial %i : %i ~ %.3f"), GetName().AsChar(), currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
#endif
}

void CBehaviorGraphDirectionalMovementNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->Deactivate( instance );
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->Deactivate( instance );
	}

	if ( ! AreAllInputsValid() )
	{
		return;
	}

	Bool& isBlending = instance[ i_isBlending ];
	const Int32& currGroupIdx = instance[ i_currGroupIdx ];
	
	DeactivateGroup( instance, currGroupIdx );
	if ( isBlending )
	{
		const Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
		DeactivateGroup( instance, prevGroupIdx );
	}
}

void CBehaviorGraphDirectionalMovementNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedRequestedMovementDirectionWSValueNode )
	{
		m_cachedRequestedMovementDirectionWSValueNode->ProcessActivationAlpha( instance, alpha );
	}
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( ! AreAllInputsValid() )
	{
		return;
	}

	Bool& isBlending = instance[ i_isBlending ];
	Int32& currGroupIdx = instance[ i_currGroupIdx ];
	Int32& currGroupAnimIdx = instance[ i_currGroupAnimIdx ];
	Float& currGroupAnimBlend = instance[ i_currGroupAnimBlend ];

	Float& blendLeft = instance[ i_blendLeft ];
	Float atPrevBlend = blendLeft;
	ProcessActivationAlphaInGroup( instance, alpha * ( 1.0f - atPrevBlend ), currGroupIdx, currGroupAnimIdx, currGroupAnimBlend );
	if ( isBlending )
	{
		Int32& prevGroupIdx = instance[ i_prevGroupIdx ];
		Int32& prevGroupAnimIdx = instance[ i_prevGroupAnimIdx ];
		Float& prevGroupAnimBlend = instance[ i_prevGroupAnimBlend ];
		ProcessActivationAlphaInGroup( instance, alpha * atPrevBlend, prevGroupIdx, prevGroupAnimIdx, prevGroupAnimBlend );
	}
}

void CBehaviorGraphDirectionalMovementNode::ProcessActivationAlphaInGroup( CBehaviorGraphInstance& instance, Float alpha, Int32 groupIdx, Int32 groupAnimIdx, Float groupAnimBlend ) const
{
	CBehaviorGraphNode* rightInput;
	CBehaviorGraphNode* leftInput;
	GetInputs( groupIdx, groupAnimIdx, leftInput, rightInput );
	rightInput->ProcessActivationAlpha( instance, alpha * ( 1.0f - groupAnimBlend ) );
	if ( rightInput != leftInput )
	{
		leftInput->ProcessActivationAlpha( instance, alpha * groupAnimBlend );
	}
}

void CBehaviorGraphDirectionalMovementNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	TBaseClass::GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
	var.PushBack( m_requestedMovementDirectionVariableName );
	var.PushBack( m_requestedFacingDirectionVariableName );
	intVar.PushBack( m_movementDirectionInternalVariableName );
	intVar.PushBack( m_groupDirInternalVariableName );
}

Float CBehaviorGraphDirectionalMovementNode::ChooseStartingPTLBasingOnPreviousPose( CBehaviorGraphInstance& instance, Int32 forGroupIdx ) const
{
	if ( m_alwaysStartAtZero )
	{
		return 0.0f;
	}
	const Int32 rightFootBoneIdx = instance[ i_rightFootBoneIdx ];
	const Int32 leftFootBoneIdx = instance[ i_leftFootBoneIdx ];
	if ( rightFootBoneIdx != -1 && leftFootBoneIdx != -1 )
	{
		// check location of foot in local space, which one is in front
		Vector rightFootLocMS = instance.GetAnimatedComponent()->GetBoneMatrixModelSpace( rightFootBoneIdx ).GetTranslation();
		Vector leftFootLocMS = instance.GetAnimatedComponent()->GetBoneMatrixModelSpace( leftFootBoneIdx ).GetTranslation();
		Float startingPTL = leftFootLocMS.Y > rightFootLocMS.Y ? m_startPTLLeftFootInFront : m_startPTLRightFootInFront;
		// offset by group
		startingPTL = startingPTL + ( (Float)forGroupIdx ) * m_syncGroupOffsetPTL;
#if DEBUG_DIRECTIONAL_MOVEMENT
		RED_LOG(DirectionalMovement, TXT("DirMov:%s: choose starting ptl %.3f (l: %.3f r: %.3f)"), GetName().AsChar(), startingPTL, leftFootLocMS.Y, rightFootLocMS.Y);
#endif
		return startingPTL;
	}
	else
	{
		return m_startPTLLeftFootInFront;
	}
}

CSkeleton* CBehaviorGraphDirectionalMovementNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetSkeleton();
}

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphDirectionalMovementStartNode::CBehaviorGraphDirectionalMovementStartNode()
{
	DisableBlending();
	m_alwaysStartAtZero = true;
}

void CBehaviorGraphDirectionalMovementStartNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();
}

void CBehaviorGraphDirectionalMovementStartNode::DisableBlending()
{
	m_movementDirBlendTime = 0.8f;
	m_movementDirMaxSpeedChange = 20.0f;
	m_facingDirBlendTime = 1.5f;
	m_facingDirMaxSpeedChange = 20.0f;
	m_groupsBlendTime = 0.0f;
	m_quickTurnBlendTime = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphDirectionalMovementStopNode::CBehaviorGraphDirectionalMovementStopNode()
{
	m_useDefinedInternalVariablesAsInitialInput = true;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
