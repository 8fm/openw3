
#include "build.h"
#include "storySceneLookAtController.h"

#include "../core/mathUtils.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/mimicComponent.h"

#include "storySceneEventsCollector_events.h"
#include "storySceneActorsEyesTracker.h"

RED_DEFINE_STATIC_NAME( DIALOG_LOOK_AT_A );
RED_DEFINE_STATIC_NAME( DIALOG_LOOK_AT_B );

RED_DEFINE_STATIC_NAME( lookatOn_scene );
RED_DEFINE_STATIC_NAME( lookAtTarget_scene );

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

const CName& CStorySceneLookAtController::LOOK_AT_NODE_NAME_A = CNAME( DIALOG_LOOK_AT_A );
const CName& CStorySceneLookAtController::LOOK_AT_NODE_NAME_B = CNAME( DIALOG_LOOK_AT_B );

const CName& CStorySceneLookAtController::LOOK_AT_ON_GMPL_ANIMAL = CNAME( lookatOn_scene );
const CName& CStorySceneLookAtController::LOOK_AT_TARGET_GMPL_ANIMAL = CNAME( lookAtTarget_scene );

CStorySceneLookAtController::CStorySceneLookAtController()
	: m_mode( M_PointCloud )
	, m_lookAtNodesAvailable( false )
	, m_useDeformationMS( true )
{
	Reset();
}

void CStorySceneLookAtController::Init( CEntity* e )
{
	m_entityLookAtOwner = e;
	m_actorLookAtOwner = Cast< CActor >( e );

	CacheSlots();
}

void CStorySceneLookAtController::Deinit()
{
	ResetGameplayLookAtNode();
}

void CStorySceneLookAtController::CacheSlots()
{
	CEntity* e = m_entityLookAtOwner.Get();
	CAnimatedComponent* ac = e ? e->GetRootAnimatedComponent() : nullptr;
	if ( ac && ac->GetBehaviorStack() )
	{
		ac->GetBehaviorStack()->GetLookAt( LOOK_AT_NODE_NAME_A, m_lookAtNodeA );
		ac->GetBehaviorStack()->GetLookAt( LOOK_AT_NODE_NAME_B, m_lookAtNodeB );

		m_lookAtEyesMediator.Init( e );

		m_lookAtNodesAvailable = true;
	}
}

void CStorySceneLookAtController::CheckSlots()
{
	if ( m_lookAtNodesAvailable && ( !m_lookAtNodeA.IsValid() || !m_lookAtNodeB.IsValid() ) )
	{
		CacheSlots();
	}
}

void CStorySceneLookAtController::ChangeMode( CStorySceneLookAtController::EMode mode )
{
	if ( m_mode != mode )
	{
		if ( m_mode == M_PointCloud )
		{
			ResetPointCloudLookAtNode();
		}
		else
		{
			SCENE_ASSERT( m_mode == M_Gameplay );

			ResetGameplayLookAtNode();
		}

		m_mode = mode;
	}
}

void CStorySceneLookAtController::ResetPointCloudLookAtNode()
{
	CheckSlots();

	if ( m_lookAtNodeA.IsValid() )
	{
		m_lookAtNodeA.ResetParams();
		m_lookAtNodeA.ResetTargetA();
		m_lookAtNodeA.ResetTargetB();
	}
	if ( m_lookAtNodeB.IsValid() )
	{
		m_lookAtNodeB.ResetParams();
		m_lookAtNodeB.ResetTargetA();
		m_lookAtNodeB.ResetTargetB();
	}
}

void CStorySceneLookAtController::ResetGameplayLookAtNode()
{
	SendDataToGameplayLookAt( 0.f, Vector::ZERO_3D_POINT );
}

void CStorySceneLookAtController::SyncWithIdle( const CName& lookAtBodyPrev, const CName& lookAtHeadPrev, const CName& lookAtBodyCurr, const CName& lookAtHeadCurr )
{
	CheckSlots();

	if ( m_lookAtNodeA.IsValid() )
	{
		m_lookAtNodeA.SetAnimations( lookAtBodyPrev, lookAtHeadPrev );
	}

	if ( m_lookAtNodeB.IsValid() )
	{
		m_lookAtNodeB.SetAnimations( lookAtBodyCurr, lookAtHeadCurr );
	}
}

void CStorySceneLookAtController::SetLowerBodyPartsWeight( Float w )
{
	CheckSlots();

	if ( m_lookAtNodeA.IsValid() )
	{
		m_lookAtNodeA.SetLowerBodyPartsWeight( w );
	}

	if ( m_lookAtNodeB.IsValid() )
	{
		m_lookAtNodeB.SetLowerBodyPartsWeight( w );
	}
}

Bool CStorySceneLookAtController::SetPointCloud( const StorySceneEventsCollector::ActorLookAt& evt, const CEntity* bodyTarget, const CEntity* eyesTarget, const Vector& bodyStaticPointWS )
{
	Bool ret = false;

	ChangeMode( M_PointCloud );

	if ( evt.m_reset )
	{
		//Reset();
		MarkReset( true );
	}
	else
	{
		MarkReset( false );

		const Bool isDiffEvt = m_bodyTargetB.m_id != evt.GetId();
		const Bool isStaticPointDiff = m_bodyTargetB.m_targetPoint.m_node.Get() == nullptr && bodyTarget == nullptr && !Vector::Near3( m_bodyTargetB.m_targetPoint.m_point, bodyStaticPointWS );

		if ( isDiffEvt || isStaticPointDiff )
		{
			m_bodyTargetA = m_bodyTargetB;
			SetupTarget( m_bodyTargetB, evt, bodyTarget, bodyStaticPointWS );
			ret = true;
		}

		const Bool isTargetABTheSame_static = m_bodyTargetA.m_targetPoint.m_node.Get() == nullptr && m_bodyTargetB.m_targetPoint.m_node.Get() == nullptr && Vector::Near3( m_bodyTargetA.m_targetPoint.m_point, m_bodyTargetB.m_targetPoint.m_point );
		const Bool isTargetABTheSame_dynamic = m_bodyTargetA.m_targetPoint.m_node.Get() != nullptr && m_bodyTargetA.m_targetPoint.m_node == m_bodyTargetB.m_targetPoint.m_node;

		if ( isTargetABTheSame_static || isTargetABTheSame_dynamic )
		{
			m_progress = 1.f;
		}
		else
		{
			m_progress = evt.m_duration > 0.f && !evt.m_bodyInstant ? evt.m_eventTimeLocal / evt.m_duration : 1.f;
		}
		m_duration = evt.m_duration;
		m_useDeformationMS = evt.m_useDeformationMS;
		m_bodyDataWasChanged = true;

		//m_bodyWeight = evt.m_bodyInstant ? ( evt.m_bodyEnabled ? evt.m_bodyWeight : 0.f ) : CalcBodyWeight( m_bodyTargetA, m_bodyTargetB, evt.m_curveValue );
		//m_bodyTargetWeight = evt.m_bodyInstant ? 1.f : CalcTargetBlendWeight( m_bodyTargetA, m_bodyTargetB, evt.m_curveValue );
		//m_bodyHeadBlendWeight = evt.m_bodyInstant ? ( m_bodyTargetB.m_headLevel ? 1.f : 0.f ) : CalcHeadBlendWeight( m_bodyTargetA, m_bodyTargetB, progress );
		//m_bodyTransitionWeight = evt.m_bodyTransitionWeight;

		if ( evt.m_level == LL_Eyes )
		{
			//m_bodyWeight = 0.f;
			m_progress = 0.f;
			m_duration = 0.f;
			//m_bodyHeadBlendWeight = 0.f;
		}

		// Eyes
		m_eyesTransitionFactor = evt.m_eyesTransitionFactor;

		RED_UNUSED( eyesTarget );
	}

	return ret;
}

void CStorySceneLookAtController::SetGameplay( const StorySceneEventsCollector::ActorGameplayLookAt& evt, const CEntity* bodyTarget, const Vector& bodyStaticPointWS )
{
	ChangeMode( M_Gameplay );

	if ( evt.m_reset )
	{
		//Reset();
		MarkReset( true );
	}
	else
	{
		MarkReset( false );

		const Bool isDiffEvt = m_bodyTargetB.m_id != evt.GetId();
		const Bool isStaticPointDiff = m_bodyTargetB.m_targetPoint.m_node.Get() == nullptr && bodyTarget == nullptr && !Vector::Near3( m_bodyTargetB.m_targetPoint.m_point, bodyStaticPointWS );

		if ( isDiffEvt )
		{
			m_bodyTargetA = m_bodyTargetB; // store previous look at target
		}

		if ( isDiffEvt || isStaticPointDiff )
		{
			// setup current look at target
			SetupTarget( m_bodyTargetB, evt, bodyTarget, bodyStaticPointWS );
		}
			
		m_progress = evt.m_duration > 0.f ? evt.m_eventTimeLocal / evt.m_duration : 1.f;
		m_duration = evt.m_duration;
		m_useDeformationMS = true;
		m_bodyDataWasChanged = true;

		if ( evt.m_instant )
		{
			m_progress = 1.f;
		}

		// Eyes
		m_eyesTransitionFactor = 0.f;

		m_behaviorVarWeight = evt.m_behaviorVarWeight;
		m_behaviorVarTarget = evt.m_behaviorVarTarget;
	}
}

void CStorySceneLookAtController::ConsumeReset()
{
	m_resetRequestConsumed = true;
}

void CStorySceneLookAtController::MarkReset( Bool flag )
{
	m_resetRequestConsumed = false;
	m_resetRequestMarker = flag;
}

void CStorySceneLookAtController::Reset()
{
	ChangeMode( M_PointCloud );

	ResetPointCloudLookAtNode();
	ResetGameplayLookAtNode();

	m_bodyTargetA.Reset();
	m_bodyTargetB.Reset();

	m_bodyTargetA.ResetId();
	m_bodyTargetB.ResetId();

	m_bodyDataWasChanged = true;
	//m_bodyWeight = 0.f;
	//m_bodyTargetWeight = 0.f;
	//m_bodyHeadBlendWeight = 0.f;
	//m_bodyTransitionWeight = 1.f;
	m_progress = 0.f;
	m_duration = 0.f;
	m_useDeformationMS = true;

	m_eyesLookAtConvergenceWeight = 0.f;
	m_eyesLookAtIsAdditive = true;
	m_eyesLookAtDampScale = 1.f;
	m_eyesTransitionFactor = 1.f;

	UnblockBlink();

	MarkReset( false );
}

void CStorySceneLookAtController::Update()
{
	if ( m_entityLookAtOwner && m_actorLookAtOwner )
	{
		// No one should add other lookats requests than scenes
		m_actorLookAtOwner->RemoveAllNonDialogsLookAts();
	}

	if ( m_resetRequestConsumed && m_resetRequestMarker )
	{
		Reset();
	}
	else if ( !m_resetRequestConsumed && m_resetRequestMarker )
	{
		ConsumeReset();
		return;
	}

	if ( m_mode == M_PointCloud )
	{
		CheckSlots();

		UpdateTarget( m_bodyTargetA.m_targetPoint, m_lookAtNodeA, false );
		UpdateTarget( m_bodyTargetB.m_targetPoint, m_lookAtNodeA, true );
		UpdateTarget( m_bodyTargetA.m_targetPoint, m_lookAtNodeB, false );
		UpdateTarget( m_bodyTargetB.m_targetPoint, m_lookAtNodeB, true );

		if ( m_bodyDataWasChanged )
		{
			SendDataToLookAt( m_lookAtNodeA );
			SendDataToLookAt( m_lookAtNodeB );

			m_bodyDataWasChanged = false;
		}

		SendDataToEyesLookAt();
	}
	else
	{
		Bool needRefresh = false;
		needRefresh |= UpdateTarget( m_bodyTargetA.m_targetPoint );
		needRefresh |= UpdateTarget( m_bodyTargetB.m_targetPoint );
		
		if ( needRefresh || m_bodyDataWasChanged )
		{
			SendDataToGameplayLookAt();

			m_bodyDataWasChanged = false;
		}
	}
}

Float CStorySceneLookAtController::CalcBodyWeight( const Target& targetA, const Target& targetB, Float progress ) const
{
	return Clamp( targetA.m_bodyWeight + ( targetB.m_bodyWeight - targetA.m_bodyWeight ) * progress, 0.f, 1.f );
}

Float CStorySceneLookAtController::CalcHeadBlendWeight( const CStorySceneLookAtController::Target& targetA, const CStorySceneLookAtController::Target& targetB, Float progress ) const
{
	if ( targetA.m_headLevel != targetB.m_headLevel )
	{
		if ( targetB.m_headLevel )
		{
			return progress;
		}
		else
		{
			return 1.f - progress;
		}
	}
	else
	{
		return targetB.m_headLevel ? 1.f : 0.f;
	}
}

Float CStorySceneLookAtController::CalcTargetBlendWeight( const Target& targetA, const Target& targetB, Float progress ) const
{
	if ( targetA.m_targetPoint.m_enabled != targetB.m_targetPoint.m_enabled || targetA.m_targetPoint.IsEqual( targetB.m_targetPoint ) )
	{
		if ( targetB.m_targetPoint.m_enabled )
		{
			return 1.f;
		}
		else
		{
			return 0.f;
		}
	}

	return progress;
}

void CStorySceneLookAtController::SetupTarget( CStorySceneLookAtController::Target& target, const StorySceneEventsCollector::ActorLookAt& evt, const CEntity* targetEntity, const Vector& bodyStaticPointWS ) const
{
	target.m_id = evt.GetId();
	target.m_targetPoint.m_enabled = evt.m_bodyEnabled;
	target.m_headLevel = evt.m_level >= LL_Head;
	target.m_bodyWeight = evt.m_bodyWeight;

	if ( evt.m_bodyEnabled )
	{
		SetupTargetPoint( target.m_targetPoint, bodyStaticPointWS, targetEntity );
	}
	else
	{
		target.Reset();
	}
}

void CStorySceneLookAtController::SetupTarget( CStorySceneLookAtController::Target& target, const StorySceneEventsCollector::ActorGameplayLookAt& evt, const CEntity* targetEntity, const Vector& bodyStaticPointWS ) const
{
	target.m_id = evt.GetId();
	target.m_targetPoint.m_enabled = evt.m_enabled;
	target.m_headLevel = false;
	target.m_bodyWeight = evt.m_weight;

	if ( evt.m_enabled )
	{
		SetupTargetPoint( target.m_targetPoint, bodyStaticPointWS, targetEntity );
	}
	else
	{
		target.Reset();
	}
}

void CStorySceneLookAtController::SetupTargetPoint( CStorySceneLookAtController::TargetPoint& target, const Vector& point, const CEntity* targetEntity ) const
{
	target.ResetTarget();

	if ( const CActor* actor = Cast< const CActor >( targetEntity ) )
	{
		target.m_bone = actor->GetHeadBone();
		target.m_node = actor;
	}
	else
	{
		target.m_point = point;
		target.m_pointWasChanged = true;
	}
}

Bool CStorySceneLookAtController::UpdateTarget( CStorySceneLookAtController::TargetPoint& target )
{
	if ( target.m_enabled )
	{
		const CNode* n = target.m_node.Get();

		if ( target.m_bone != -1 )
		{
			SCENE_ASSERT( n );
			SCENE_ASSERT( n->IsA< CEntity >() );

			if ( const CActor* actor = Cast< CActor >( n ) )
			{
				target.m_point = CStorySceneActorsEyesTracker::GetActorEyePosWS( actor );
				target.m_pointWasChanged = true;
			}
		}
		else if ( n )
		{
			target.m_point = n->GetWorldPosition();
			target.m_pointWasChanged = true;
		}

		return target.m_pointWasChanged;
	}

	return false;
}

void CStorySceneLookAtController::UpdateTarget( CStorySceneLookAtController::TargetPoint& target, CBehaviorPointCloudLookAtInterface& lookAt, Bool targetB )
{
	SCENE_ASSERT( m_mode == M_PointCloud );

	if ( lookAt.IsValid() )
	{
		if ( UpdateTarget( target ) )
		{
			if ( targetB )
			{
				lookAt.SetTargetB( target.m_point );
			}
			else
			{
				lookAt.SetTargetA( target.m_point );
			}
		}
		else if ( !target.m_enabled )
		{
			if ( targetB )
			{
				lookAt.ResetTargetB();
			}
			else
			{
				lookAt.ResetTargetA();
			}
		}
	}
}

void CStorySceneLookAtController::SendDataToLookAt( CBehaviorPointCloudLookAtInterface& lookAt )
{
	SCENE_ASSERT( m_mode == M_PointCloud );

	if ( lookAt.IsValid() )
	{
		CBehaviorGraphPointCloudLookAtNode::Params params;
		params.m_progress = m_progress;
		params.m_weightA = m_bodyTargetA.m_bodyWeight;
		params.m_useSecBlendCloudA = m_bodyTargetA.m_headLevel;
		params.m_weightB = m_bodyTargetB.m_bodyWeight;
		params.m_useSecBlendCloudB = m_bodyTargetB.m_headLevel;
		params.m_duration = m_duration;
		params.m_useDeformationMS = m_useDeformationMS;

		lookAt.SetParams( params );

		//lookAt.SetWeight( m_progress, m_bodyWeight );
		//lookAt.SetTargetWeight( m_bodyTargetWeight );
		//lookAt.SetBlendWeight( m_bodyHeadBlendWeight );
		//lookAt.SetTransitionWeight( m_bodyTransitionWeight );
	}
}

void CStorySceneLookAtController::SendDataToGameplayLookAt( Float weight, const Vector& targetWS )
{	
	if ( m_entityLookAtOwner )
	{
		if ( CAnimatedComponent* ac = m_entityLookAtOwner->GetRootAnimatedComponent() )
		{
			if ( CBehaviorGraphStack* stack = ac->GetBehaviorStack() )
			{
				if ( stack->HasBehaviorFloatVariable( LOOK_AT_ON_GMPL_ANIMAL ) )
				{
					stack->SetBehaviorVariable( LOOK_AT_ON_GMPL_ANIMAL, weight );
					stack->SetBehaviorVariable( LOOK_AT_TARGET_GMPL_ANIMAL, targetWS );
				}
				else if ( m_behaviorVarWeight && m_behaviorVarTarget )
				{
					stack->SetBehaviorVariable( m_behaviorVarWeight, weight );
					stack->SetBehaviorVariable( m_behaviorVarTarget, targetWS );
				}
			}
		}
	}
}

void CStorySceneLookAtController::CalcApproximateWeightAndTarget( Float& w, Vector& vec ) const
{
	Float weight( 0.f );
	Vector target( Vector::ZERO_3D_POINT );

	if ( m_bodyTargetA.m_targetPoint.m_enabled && m_bodyTargetB.m_targetPoint.m_enabled )
	{
		weight = Lerp( m_progress, m_bodyTargetA.m_bodyWeight, m_bodyTargetB.m_bodyWeight );
		target = Vector::Interpolate( m_bodyTargetA.m_targetPoint.m_point, m_bodyTargetB.m_targetPoint.m_point, m_progress );
	}
	else if ( m_bodyTargetB.m_targetPoint.m_enabled )
	{
		weight = m_bodyTargetB.m_bodyWeight;
		target = m_bodyTargetB.m_targetPoint.m_point;
	}
	else if ( m_bodyTargetB.m_targetPoint.m_enabled )
	{
		weight = m_bodyTargetA.m_bodyWeight;
		target = m_bodyTargetA.m_targetPoint.m_point;
	}
	else
	{
		weight = Lerp( m_progress, m_bodyTargetA.m_bodyWeight, m_bodyTargetB.m_bodyWeight );
		target = Vector::Interpolate( m_bodyTargetA.m_targetPoint.m_point, m_bodyTargetB.m_targetPoint.m_point, m_progress );
		//SCENE_ASSERT( 0 );
	}

	w = weight;
	vec = target;
}

void CStorySceneLookAtController::SendDataToGameplayLookAt()
{
	Float weight( 0.f );
	Vector target( Vector::ZERO_3D_POINT );
	CalcApproximateWeightAndTargetForGameplay( weight, target );

	SendDataToGameplayLookAt( weight, target );
}

void CStorySceneLookAtController::SendDataToEyesLookAt()
{
	SCENE_ASSERT( m_mode == M_PointCloud );

	if ( m_lookAtEyesMediator.IsValid() )
	{
		Float weight( 0.f );
		Vector target( Vector::ZERO_3D_POINT );
		CalcApproximateWeightAndTarget( weight, target );

		m_lookAtEyesMediator.SetData( m_progress, m_bodyTargetA.m_targetPoint.m_point, m_bodyTargetB.m_targetPoint.m_point, weight );
	}
}

void CStorySceneLookAtController::NotifyTargetWasChanged()
{
	if ( m_lookAtEyesMediator.IsValid() )
	{
		m_lookAtEyesMediator.NotifyTargetWasChanged();
	}
}

Vector CStorySceneLookAtController::GetApproximateTargetWS() const
{
	Float weight( 0.f );
	Vector target( Vector::ZERO_3D_POINT );
	CalcApproximateWeightAndTarget( weight, target );

	return target;
}

Float CStorySceneLookAtController::GetApproximateHorAngleDegToTargetLS() const
{
	if ( m_bodyTargetB.m_targetPoint.m_enabled && m_entityLookAtOwner && m_actorLookAtOwner )
	{
		if ( CAnimatedComponent* ac = m_actorLookAtOwner->GetRootAnimatedComponent() )
		{
			const Int32 head = m_actorLookAtOwner->GetHeadBone();
			SCENE_ASSERT( head != -1 );
			if ( head != -1 )
			{
				Matrix headWS( Matrix::IDENTITY );
				if ( ac->GetThisFrameTempBoneMatrixWorldSpace( head, headWS ) )
				{
					Vector pointA( Vector::ZERO_3D_POINT );
					if ( m_bodyTargetA.m_targetPoint.m_enabled )
					{
						pointA = m_bodyTargetA.m_targetPoint.m_point;
					}
					else
					{
						pointA = headWS.GetTranslation() + headWS.GetAxisY(); // TODO GetAxisY
					}

					const Vector dirA = pointA - headWS.GetTranslation();
					const Vector dirB = m_bodyTargetB.m_targetPoint.m_point - headWS.GetTranslation();

					const Float angleDeg = MathUtils::VectorUtils::GetAngleDegAroundAxis( dirA, dirB, Vector::EZ );
					return angleDeg;
				}
			}
		}
	}

	return 0.f;
}

void CStorySceneLookAtController::BlockBlink( const CGUID& id )
{
	m_blockBlink = true;
	m_blockBlinkId = id;
}

void CStorySceneLookAtController::UnblockBlink()
{
	m_blockBlink = false;
	m_blockBlinkId = CGUID::ZERO;
}

Bool CStorySceneLookAtController::IsBlinkBlocked( const CGUID& id ) const
{
	return m_blockBlink;
}

void CStorySceneLookAtController::CalcApproximateWeightAndTargetForGameplay( Float& w, Vector& vec ) const
{
	Float weight( 0.f );
	Vector target( Vector::ZERO_3D_POINT );

	// Note that there is similar function 'CalcApproximateWeightAndTarget' that manages 
	// approximation for point cloud/skeleton look at.
	// When dealing with gameplay look at:
	// m_bodyTargetA - is previous look at target (so can be disabled)
	// m_bodyTargetB - is current look target

	if ( m_bodyTargetA.m_targetPoint.m_enabled && m_bodyTargetB.m_targetPoint.m_enabled )
	{
		// we have current and previous look at target so, interpolate between them,
		// e.g. user requested new look at target, when m_entityLookAtOwner was looking at the previous one.
		weight = Lerp( m_progress, m_bodyTargetA.m_bodyWeight, m_bodyTargetB.m_bodyWeight );
		target = Vector::Interpolate( m_bodyTargetA.m_targetPoint.m_point, m_bodyTargetB.m_targetPoint.m_point, m_progress );
	}
	else if ( m_bodyTargetB.m_targetPoint.m_enabled )
	{
		// we have only current look at target (e.g. first look at on the scene),
		// so weight will be interpolated here [0 - m_bodyWeight]. Note that this should
		// activate smoothly look at node in m_entityLookAtOwner's behavior graph.
		weight = m_bodyTargetB.m_bodyWeight*m_progress;
		target = m_bodyTargetB.m_targetPoint.m_point;
	}
	else if ( !m_bodyTargetB.m_targetPoint.m_enabled && m_bodyTargetA.m_targetPoint.m_enabled )
	{
		// user disabled current look at target - so weight here should smoothly deactivate look at node
		// in m_entityLookAtOwner's behavior graph, while looking at prev target.
		weight = m_bodyTargetA.m_bodyWeight * ( 1.0f - m_progress );
		target = m_bodyTargetA.m_targetPoint.m_point;
	}

	// In all other scenarios do not activate look at node.
	// E.g. situation when user sets disable event as first event in the track

	w = weight;
	vec = target;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif
