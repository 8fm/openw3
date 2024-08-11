/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphComboNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphStateMachine.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/extAnimBehaviorEvents.h"
#include "animatedComponent.h"
#include "baseEngine.h"
#include "behaviorProfiler.h"

//#define BEH_COMBO_DEBUG

IMPLEMENT_RTTI_ENUM( EAttackDirection );
IMPLEMENT_RTTI_ENUM( EAttackDistance );

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComboStateNode );

CBehaviorGraphComboStateNode::CBehaviorGraphComboStateNode()
	: m_isConnected( false )
	, m_blendInternal( 0.f )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphComboStateNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_slotA = CreateSlot();
	m_slotB = CreateSlot();
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphAnimationNode* CBehaviorGraphComboStateNode::CreateSlot()
{
	GraphBlockSpawnInfo spawnInfo( CBehaviorGraphAnimationNode::GetStaticClass() );

	// Create node
	CBehaviorGraphAnimationNode* newNode = CreateObject< CBehaviorGraphAnimationNode >( spawnInfo.GetClass(), this );	

	// Inform node that it has been spawned
	newNode->OnSpawned( spawnInfo );

	// Rebuild sockets of new node
	newNode->OnRebuildSockets();

	// Don't play with loops
	newNode->SetLoopPlayback( false );

	return newNode;
}

#endif

void CBehaviorGraphComboStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_level;
	compiler << i_running;
	compiler << i_cooldownTimer;
	compiler << i_cooldownDuration;
	compiler << i_comboNextAttack;
	compiler << i_comboTimer;
	compiler << i_comboEvent;
	compiler << i_hasVarComboWay;
	compiler << i_hasVarComboDist;
	compiler << i_hasVarComboDir;
	compiler << i_internalRootTimer;
	compiler << i_rootState;

	m_slotA->OnBuildDataLayout( compiler );
	m_slotB->OnBuildDataLayout( compiler );
}

void CBehaviorGraphComboStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_level ] = 0;
	instance[ i_running ] = false;
	instance[ i_cooldownTimer ] = 0.f;
	instance[ i_cooldownDuration ] = m_cooldown;
	instance[ i_comboTimer ] = 0.f;
	instance[ i_comboEvent ] = instance.GetEventId( m_comboEvent );
	instance[ i_hasVarComboWay ] = instance.HasFloatValue( m_varComboWay );
	instance[ i_hasVarComboDist ] = instance.HasFloatValue( m_varComboDist );
	instance[ i_hasVarComboDir ] = instance.HasFloatValue( m_varComboDir );
	instance[ i_internalRootTimer ] = 0.f;
	instance[ i_rootState ] = ROOT_IDLE_STATE;

	ResetNextAttack( instance );

	m_slotA->OnInitInstance( instance );
	m_slotB->OnInitInstance( instance );
}

void CBehaviorGraphComboStateNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	m_slotA->OnReleaseInstance( instance );
	m_slotB->OnReleaseInstance( instance );
}

void CBehaviorGraphComboStateNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_level );
	INST_PROP( i_running );
	INST_PROP( i_cooldownTimer );
	INST_PROP( i_comboNextAttack );
	INST_PROP( i_comboTimer );

	m_slotA->OnBuildInstanceProperites( instance, builder );
	m_slotB->OnBuildInstanceProperites( instance, builder );
}

Bool CBehaviorGraphComboStateNode::HasCachedAllValues( CBehaviorGraphInstance& instance ) const
{
	return	instance[ i_comboEvent ] != CBehaviorEventsList::NO_EVENT &&
			instance[ i_hasVarComboWay ] &&
			instance[ i_hasVarComboDist ] &&
			instance[ i_hasVarComboDir ];
}

 Bool CBehaviorGraphComboStateNode::AutoStart() const
 {
 	return true;
 }

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphComboStateNode::GetCaption() const
{
	return String::EMPTY;
}

EGraphBlockShape CBehaviorGraphComboStateNode::GetBlockShape() const
{
	return GBS_Octagon;
}

Color CBehaviorGraphComboStateNode::GetBorderColor() const
{
	return Color( 75, 175, 210 );
}

Color CBehaviorGraphComboStateNode::GetClientColor() const
{
	return Color( 135, 142, 149 );
}

EGraphBlockDepthGroup CBehaviorGraphComboStateNode::GetBlockDepthGroup() const
{
	return GBDG_Foreground;
}

#endif

CBehaviorGraph* CBehaviorGraphComboStateNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

Float CBehaviorGraphComboStateNode::GetCooldownDuration( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_cooldownDuration ];
}

void CBehaviorGraphComboStateNode::SetCooldownDuration( CBehaviorGraphInstance& instance, Float duration ) const
{
	instance[ i_cooldownDuration ] = duration;
}

Float CBehaviorGraphComboStateNode::GetCooldownProgress( CBehaviorGraphInstance& instance ) const
{
	return GetCooldownDuration( instance ) > 0.f ? Clamp< Float >( instance[ i_cooldownTimer ] / GetCooldownDuration( instance ) , 0.f, 1.f ) : 0.f;
}

Bool CBehaviorGraphComboStateNode::IsRunning( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_running ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphComboStateNode::GetComboType() const
{
	return TXT("Combo");
}

String CBehaviorGraphComboStateNode::GetComboDesc( CBehaviorGraphInstance& instance ) const
{
	if ( IsRunning( instance ) )
	{
		Uint32 level = GetComboLevel( instance );
		Bool canStart = CanStartNextAttack( instance );
		Bool blending = IsExternalBlending( instance );
		Bool internalBlending = IsRootBlending( instance );
		Bool isRoot = IsPlayingRoot( instance );

		String desc = canStart? String::Printf( TXT("Next Level %d - READY\n"), level ) : String::Printf( TXT("Next Level %d\n"), level );

		if ( m_slotA->IsTempSlotActive( instance ) )
		{
			const CName& animName = m_slotA->GetAnimation( instance )->GetName();
			Float progress = m_slotA->GetAnimProgress( instance );
			Float alpha = GetAnimBlendWeight( instance );

			desc += String::Printf( TXT("A:%s [%.2f], [blend %.2f]\n"), animName.AsString().AsChar(), progress, alpha );
		}
		else
		{
			desc += TXT("A:<empty> [0.00], [blend 0.00]\n");
		}

		if ( m_slotB->IsTempSlotActive( instance ) )
		{
			const CName& animName = m_slotB->GetAnimation( instance )->GetName();
			Float progress = m_slotB->GetAnimProgress( instance );

			desc += String::Printf( TXT("B:%s [%.2f] \n"), animName.AsString().AsChar(), progress );
		}
		else
		{
			desc += TXT("B:<empty> [0.00]\n");
		}

		if ( HasNextAttack( instance ) )
		{
			const CName& nextAnimAttack = GetNextAttack( instance ).m_attackAnimation;
			const CName& nextAnimParry = GetNextAttack( instance ).m_parryAnimation;
			desc += String::Printf( TXT("N A:%s P:%s\n"), nextAnimAttack.AsString().AsChar(), nextAnimParry.AsString().AsChar() );
		}
		else
		{
			desc += TXT("N:<empty>\n");
		}

		desc += blending ? TXT("Blending [true]") : TXT("Blending [false]");

		desc += isRoot ?			TXT("Root [ON], ") : TXT("Root [OFF], ");
		desc += internalBlending ?	TXT("Blend [ON]\n") : TXT("Blend [OFF]\n");

		return desc;
	}
	else
	{
		Float cp = GetCooldownProgress( instance );
		if ( cp > 0.f )
		{
			return String::Printf( TXT("Cooldown [%.2f]"), cp );
		}
		else
		{
			return TXT("Sleeping");
		}
	}
}

#endif

namespace
{
	void CollectUsedAnimationsFromDist( const SBehaviorComboDistance& dir, TDynArray< CName >& anims )
	{
		const Uint32 size = dir.m_animations.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SBehaviorComboAnimation& anim = dir.m_animations[ i ];

			if ( anim.m_animationAttack != CName::NONE )
			{
				anims.PushBackUnique( anim.m_animationAttack );
			}
			
			if ( anim.m_animationParry != CName::NONE )
			{
				anims.PushBackUnique( anim.m_animationParry );
			}
		}
	}

	void CollectUsedAnimationsFromDir( const SBehaviorComboDirection& dir, TDynArray< CName >& anims )
	{
		CollectUsedAnimationsFromDist( dir.m_distLarge, anims );
		CollectUsedAnimationsFromDist( dir.m_distMedium, anims );
		CollectUsedAnimationsFromDist( dir.m_distSmall, anims );
	}
}

void CBehaviorGraphComboStateNode::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	for ( Uint32 i=0; i<m_comboWays.Size(); ++i )
	{
		const SBehaviorComboWay& way = m_comboWays[ i ];
		
		for ( Uint32 j=0; j<way.m_levels.Size(); ++j )
		{
			const SBehaviorComboLevel& level = way.m_levels[ j ];

			CollectUsedAnimationsFromDir( level.m_dirBack, anims );
			CollectUsedAnimationsFromDir( level.m_dirFront, anims );
			CollectUsedAnimationsFromDir( level.m_dirLeft, anims );
			CollectUsedAnimationsFromDir( level.m_dirRight, anims );
		}
	}
}

void CBehaviorGraphComboStateNode::StartCombo( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Start Combo"), GetClass()->GetName().AsChar() );
#endif

	instance[ i_running ] =			true;
	instance[ i_cooldownTimer ] =	0.f;

	if ( HasNextAttack( instance ) && AutoStart() )
	{
		StartNextAttack( instance, 0.f );
	}
	else if ( AutoStart() )
	{
		ASSERT( 0 );
	}
	else
	{
		StartPlayingRoot( instance, true );
	}
}

void CBehaviorGraphComboStateNode::StopCombo( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Stop Combo"), GetClass()->GetName().AsChar() );
#endif

	instance[ i_running ] =			false;
	instance[ i_cooldownTimer ] =	0.f;

	ResetNextAttack( instance );
}

Bool CBehaviorGraphComboStateNode::FireFinishEvent( CBehaviorGraphInstance& instance ) const
{
	if ( m_finishedEvent != CName::NONE )
	{
		return instance.GenerateEvent( m_finishedEvent );
	}

	return false;
}

void CBehaviorGraphComboStateNode::StartPlayingRoot( CBehaviorGraphInstance& instance, Bool force ) const
{
	ASSERT( IsPlayingRoot( instance ) == false );

	if ( force )
	{
		instance[ i_rootState ] = ROOT_WORK_STATE; 
	}
	else
	{
		instance[ i_rootState ] = BT_BlendIn;

		instance[ i_internalRootTimer ] = 0.f;
	}
}

void CBehaviorGraphComboStateNode::StopPlayingRoot( CBehaviorGraphInstance& instance ) const
{
	ASSERT( IsPlayingRoot( instance ) == true );

	instance[ i_rootState ] = ROOT_IDLE_STATE;
}

Bool CBehaviorGraphComboStateNode::IsPlayingRoot( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_rootState ] != ROOT_IDLE_STATE;
}

Bool CBehaviorGraphComboStateNode::IsRootBlending( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_rootState ] == BT_BlendIn;
}

Float CBehaviorGraphComboStateNode::GetWeightForRoot( CBehaviorGraphInstance& instance ) const
{
	return m_blendInternal > 0.f ? Clamp( 1.f - instance[ i_internalRootTimer ] / m_blendInternal, 0.f, 1.f ) : 0.f;
}

void CBehaviorGraphComboStateNode::UpdateRoot( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( IsRootBlending( instance ) )
	{
		instance[ i_internalRootTimer ] += timeDelta;

		if ( instance[ i_internalRootTimer ] >= m_blendInternal )
		{
			instance[ i_rootState ] = ROOT_WORK_STATE;
		}	
	}
	
	m_rootNode->Update( context, instance, timeDelta );
}

void CBehaviorGraphComboStateNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( ComboState );
	instance[ i_comboTimer ] = Min( instance[ i_comboTimer ] + timeDelta, m_blendForAnim );

	if ( IsRunning( instance ) )
	{
		Float restTime = UpdateSteps( context, instance, timeDelta );

		if ( HasNextAttack( instance ) && CanStartNextAttack( instance ) )
		{
			#ifdef BEH_COMBO_DEBUG
				BEH_LOG( TXT("%s - Start next attack"), GetClass()->GetName().AsChar() );
			#endif

			StartNextAttack( instance, restTime );
		}
		else if ( CurrAttackFinished( instance ) )
		{
			#ifdef BEH_COMBO_DEBUG
				BEH_LOG( TXT("%s - Current attack finished"), GetClass()->GetName().AsChar() );
			#endif

			StopCombo( instance );

			if ( FireFinishEvent( instance ) == false )
			{
				StartPlayingRoot( instance, false );
			}
		}
	}

	if ( IsPlayingRoot( instance ) )
	{
		UpdateRoot( context, instance, timeDelta );
	}
}

Float CBehaviorGraphComboStateNode::UpdateSteps( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( GetAnimBlendWeight( instance ) >= 1.f && m_slotA->IsTempSlotActive( instance ) )
	{
		m_slotA->ResetTempRuntimeAnimation( instance );
	}

	if ( m_slotA->IsTempSlotActive( instance ) )
	{
		m_slotA->Update( context, instance, timeDelta );
	}

	if ( m_slotB->IsTempSlotActive( instance ) )
	{
		Float time = m_slotB->GetAnimTime( instance );
		Float duration = m_slotB->GetAnimDuration( instance );

		Float restTime = time + timeDelta - duration;

		m_slotB->Update( context, instance, timeDelta );

		return Max( 0.f, restTime );
	}
	else
	{
		return 0.f;
	}
}

void CBehaviorGraphComboStateNode::ProcessMainPose( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
}

Bool CBehaviorGraphComboStateNode::CanStartNextAttack( CBehaviorGraphInstance& instance ) const
{
	return true;
}

Bool CBehaviorGraphComboStateNode::IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const
{
	return false;
}

void CBehaviorGraphComboStateNode::OnStartNextAttack( CBehaviorGraphInstance& instance ) const
{
}

void CBehaviorGraphComboStateNode::PlayNextAnimation( CBehaviorGraphInstance& instance, const CName& nextAnim, Float restTime ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Play next animation"), GetClass()->GetName().AsChar() );
#endif

	// Set animation to A slot
	if ( m_slotB->IsTempSlotActive( instance ) )
	{
		const CName& currAnim = m_slotB->GetAnimation( instance )->GetName();
		CSyncInfo info;
		m_slotB->GetSyncInfo( instance, info );

		m_slotA->SetTempRuntimeAnimationName( instance, currAnim );
		m_slotA->SynchronizeTo( instance, info );
	}
	else
	{
		m_slotA->ResetTempRuntimeAnimation( instance );
	}

	// Set new animation to B slot
	Bool ret = m_slotB->SetTempRuntimeAnimationName( instance, nextAnim );
	ASSERT( ret );
	CSyncInfo info;
	info.m_prevTime = 0.f;
	info.m_currTime = restTime;
	m_slotB->SynchronizeTo( instance, info );

	// Reset timer
	instance[ i_comboTimer ] = 0.f;
}

Bool CBehaviorGraphComboStateNode::SelectDefaultAnimation( CBehaviorGraphInstance& instance, const CName& animName ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Select deault animation"), GetClass()->GetName().AsChar() );
#endif

	const CSkeletalAnimationSetEntry* animation = FindAnimation( instance, animName );

	if ( !animation )
	{
		BEH_WARN( TXT("Behavior defensive combo: Default hit animation '%ls' is empty. Please check animset."), animName.AsString().AsChar() );
		return false;
	}

	SBehaviorComboAttack& attack = instance[ i_comboNextAttack ];

	// Level
	attack.m_level = 0;
	attack.m_type = 0;
	attack.m_distance = ADIST_Small;
	attack.m_direction = AD_Front;

	// Animations
	attack.m_attackAnimation = animation->GetName();
	attack.m_parryAnimation = CName::NONE;

	// Durations
	attack.m_attackTime = animation->GetDuration();
	attack.m_parryTime = 0.f;

	// Hit time and level
	//GetHitDataFromAnim( NULL, attack.m_attackHitTime, attack.m_attackHitLevel );
	//GetHitDataFromAnim( NULL, attack.m_parryHitTime, attack.m_parryHitLevel );

	GetHitDataFromAnim( NULL, 
		attack.m_attackHitTime, attack.m_attackHitTime1, attack.m_attackHitTime2, attack.m_attackHitTime3, 
		attack.m_attackHitLevel, attack.m_attackHitLevel1, attack.m_attackHitLevel2, attack.m_attackHitLevel3 );

	GetHitDataFromAnim( NULL, 
		attack.m_parryHitTime, attack.m_parryHitTime1, attack.m_parryHitTime2, attack.m_parryHitTime3, 
		attack.m_parryHitLevel, attack.m_parryHitLevel1, attack.m_parryHitLevel2, attack.m_parryHitLevel3 );

	return true;
}

void CBehaviorGraphComboStateNode::StartNextAttack( CBehaviorGraphInstance& instance, Float restTime ) const
{
	// Get next combo animation
	SBehaviorComboAttack nextComboAttack = ResetNextAttack( instance );

	CName nextAnim = IsAttackParried( instance, nextComboAttack) ? nextComboAttack.m_parryAnimation : nextComboAttack.m_attackAnimation;

	if ( IsPlayingRoot( instance ) )
	{
		StopPlayingRoot( instance );
	}

	PlayNextAnimation( instance, nextAnim, restTime );

	// Increase combo level
	IncComboLevel( instance );

	OnStartNextAttack( instance );
}

Bool CBehaviorGraphComboStateNode::CurrAttackFinished( CBehaviorGraphInstance& instance ) const
{
	if ( m_slotB->IsTempSlotActive( instance ) == false )
	{
		return false;
	}

	Float time = m_slotB->GetAnimTime( instance );
	Float duration = m_slotB->GetAnimDuration( instance );
	return time >= duration;
}

Uint32 CBehaviorGraphComboStateNode::GetComboLevel( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_level ];
}

Uint32 CBehaviorGraphComboStateNode::GetMaxComboLevel() const
{
	return m_maxLevel;
}

void CBehaviorGraphComboStateNode::SetComboLevel( CBehaviorGraphInstance& instance, Uint32 level ) const
{
	if ( level >= m_maxLevel )
	{
		ResetComboLevel( instance );
	}
	else
	{
		instance[ i_level ] = level;
	}
}

void CBehaviorGraphComboStateNode::IncComboLevel( CBehaviorGraphInstance& instance ) const
{
	instance[ i_level ] += 1;

	if ( instance[ i_level ] >= m_maxLevel )
	{
		ResetComboLevel( instance );
	}
}

void CBehaviorGraphComboStateNode::ResetComboLevel( CBehaviorGraphInstance& instance ) const
{
	instance[ i_level ] = 0;
}

Float CBehaviorGraphComboStateNode::GetAnimBlendWeight( CBehaviorGraphInstance& instance ) const
{
	return m_blendForAnim > 0.f ? Clamp( instance[ i_comboTimer ] / m_blendForAnim, 0.f, 1.f ) : 1.f;
}

void CBehaviorGraphComboStateNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	Bool running = IsRunning( instance );

	if ( m_slotA->IsTempSlotActive( instance ) && m_slotB->IsTempSlotActive( instance ) )
	{
		CCacheBehaviorGraphOutput cachePoseA( context );
		CCacheBehaviorGraphOutput cachePoseB( context );

		SBehaviorGraphOutput* poseA = cachePoseA.GetPose();
		SBehaviorGraphOutput* poseB = cachePoseB.GetPose();

		if ( poseA && poseB )
		{
			m_slotA->Sample( context, instance, *poseA );
			m_slotB->Sample( context, instance, *poseB );

			Float alpha = GetAnimBlendWeight( instance );

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				output.SetInterpolate( *poseA, *poseB, alpha );
			}
			else
			{
				output.SetInterpolateME( *poseA, *poseB, alpha );
			}
#else
			output.SetInterpolate( *poseA, *poseB, alpha );
#endif

			output.MergeEventsAndUsedAnims( *poseA, *poseB, 1.0f - alpha, alpha );

			if ( running )
			{
				ProcessMainPose( instance, *poseB );
			}
		}
	}
	else if ( m_slotB->IsTempSlotActive( instance ) )
	{
		m_slotB->Sample( context, instance, output );

		if ( running )
		{
			ProcessMainPose( instance, output );
		}
	}

	if ( IsPlayingRoot( instance ) )
	{
		if ( IsRootBlending( instance ) )
		{
			CCacheBehaviorGraphOutput cachedRootPose( context );
			SBehaviorGraphOutput* poseRoot = cachedRootPose.GetPose();
			if ( poseRoot )
			{
				Float weight = GetWeightForRoot( instance );

				m_rootNode->Sample( context, instance, *poseRoot );

#ifdef DISABLE_SAMPLING_AT_LOD3
				if ( context.GetLodLevel() <= BL_Lod2 )
				{
					output.SetInterpolate( *poseRoot, output, weight );
				}
				else
				{
					output.SetInterpolateME( *poseRoot, output, weight );
				}
#else
				output.SetInterpolate( *poseRoot, output, weight );
#endif

				output.MergeEventsAndUsedAnims( *poseRoot, output, 1.0f - weight, weight );
			}
		}
		else
		{
			m_rootNode->Sample( context, instance, output );
		}
	}
}

Bool CBehaviorGraphComboStateNode::IsConnected() const
{
	return m_isConnected;
}

Bool CBehaviorGraphComboStateNode::ReadyToStart( CBehaviorGraphInstance& instance ) const
{
	return IsConnected() && HasNextAttack( instance );
}

Bool CBehaviorGraphComboStateNode::IsExternalBlending( CBehaviorGraphInstance& instance ) const
{
	CBehaviorGraphStateMachineNode* sm = SafeCast< CBehaviorGraphStateMachineNode >( GetParent() );
	return IsActive( instance ) && sm->GetCurrentState( instance ) != this;
}

void CBehaviorGraphComboStateNode::ProcessCooldown( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( !IsRunning( instance ) && GetComboLevel( instance ) > 0 )
	{
		// Cooldown process
		instance[ i_cooldownTimer ] += timeDelta;

		if ( instance[ i_cooldownTimer ] > GetCooldownDuration( instance ) )
		{
			// Cooldown finished
			ResetComboLevel( instance );

			// Reset cooldown
			SetCooldownDuration( instance, m_cooldown );
			instance[ i_cooldownTimer ] = 0.f;
		}
	}
}

const CSkeletalAnimationSetEntry* CBehaviorGraphComboStateNode::FindAnimation( CBehaviorGraphInstance& instance, const CName& animName ) const
{
	if ( instance.GetAnimatedComponent()->GetAnimationContainer() )
	{
		return instance.GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( animName );
	}
	else
	{
		return NULL;
	}
}

Bool CBehaviorGraphComboStateNode::GetHitDataFromAnim( const CSkeletalAnimationSetEntry* anim, 
													   Vector& hitTimes, 
													   Vector& hitLevels ) const
{
	Bool ret = false;

	hitTimes = -Vector::ONES;
	hitLevels = -Vector::ONES;

	if ( anim && anim->GetAnimation() )
	{
		// Find first hit event
		TDynArray< const CExtAnimHitEvent* > hitEvents;
		anim->GetEventsOfType( hitEvents );

		if ( hitEvents.Size() > 0 )
		{
			SortEvents( hitEvents );

			Uint32 size = Min( hitEvents.Size(), MAX_COMBO_PARTS );

			for ( Uint32 i=0; i<size; ++i )
			{
				const CExtAnimHitEvent* temp = hitEvents[i];

				hitTimes.A[i] = temp->GetStartTime();
				hitLevels.A[i] = (Float)temp->GetHitLevel();

				ret = true;
			}

			if ( hitEvents.Size() > MAX_COMBO_PARTS )
			{
				BEH_ERROR( TXT("Behavior assset error: Combo animation '%ls' has got more then 3 events!"), anim->GetName().AsString().AsChar() );
			}

			if ( hitTimes.A[3] > 0.f ) hitTimes.A[3] -= hitTimes.A[2];
			if ( hitTimes.A[2] > 0.f ) hitTimes.A[2] -= hitTimes.A[1];
			if ( hitTimes.A[1] > 0.f ) hitTimes.A[1] -= hitTimes.A[0];
		}
	}

	return ret;
}

Bool CBehaviorGraphComboStateNode::GetHitDataFromAnim( const CSkeletalAnimationSetEntry* anim, 
													   Float& hitTimes0, Float& hitTimes1, Float& hitTimes2, Float& hitTimes3, 
													   Float& hitLevels0, Float& hitLevels1, Float& hitLevels2, Float& hitLevels3 ) const
{
	Vector hitTimes = -Vector::ONES;
	Vector hitLevels = -Vector::ONES;

	Bool ret = GetHitDataFromAnim( anim, hitTimes, hitLevels );

	hitTimes0 = hitTimes.A[0];
	hitTimes1 = hitTimes.A[1];
	hitTimes2 = hitTimes.A[2];
	hitTimes3 = hitTimes.A[3];

	hitLevels0 = hitLevels.A[0];
	hitLevels1 = hitLevels.A[1];
	hitLevels2 = hitLevels.A[2];
	hitLevels3 = hitLevels.A[3];

	return ret;
}

void CBehaviorGraphComboStateNode::SortEvents( TDynArray< const CExtAnimHitEvent* >& events ) const
{
	// TODO
	//...
}

void CBehaviorGraphComboStateNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Activation"), GetClass()->GetName().AsChar() );
#endif

	TBaseClass::OnActivated( instance );

	m_slotA->Activate( instance );
	m_slotB->Activate( instance );

	StartCombo( instance );
}

void CBehaviorGraphComboStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Deactivation"), GetClass()->GetName().AsChar() );
#endif

	if ( IsPlayingRoot( instance ) )
	{
		StopPlayingRoot( instance );
	}

	ResetSlots( instance );

	m_slotA->Deactivate( instance );
	m_slotB->Deactivate( instance );

	if ( IsRunning( instance ) )
	{
		StopCombo( instance );
	}

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphComboStateNode::OnReset( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Reset"), GetClass()->GetName().AsChar() );
#endif

	TBaseClass::OnReset( instance );

	if ( IsPlayingRoot( instance ) )
	{
		StopPlayingRoot( instance );
	}

	if ( IsRunning( instance ) )
	{
		StopCombo( instance );
		ResetSlots( instance );
	}
}

void CBehaviorGraphComboStateNode::ResetSlots( CBehaviorGraphInstance& instance ) const
{
	m_slotA->ResetTempRuntimeAnimation( instance );
	m_slotB->ResetTempRuntimeAnimation( instance );
}

Bool CBehaviorGraphComboStateNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return ProcessComboEvent( instance, event );
}

Bool CBehaviorGraphComboStateNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	//if ( !IsRunning( instance ) )
	{
		return ProcessComboEvent( instance, event );
	}

	//return false;
}

void CBehaviorGraphComboStateNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	CBehaviorGraphNode::ProcessActivationAlpha( instance, alpha );
}

void CBehaviorGraphComboStateNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	// Check if combo is connected to global transition
	CBehaviorGraphStateMachineNode* sm = FindParent< CBehaviorGraphStateMachineNode >();
	m_isConnected = sm ? sm->HasGlobalTransitionConnectedTo( this ) : false;
}

Bool CBehaviorGraphComboStateNode::HasNextAttack( CBehaviorGraphInstance& instance ) const
{
	return GetNextAttack( instance ).IsValid();
}

const SBehaviorComboAttack& CBehaviorGraphComboStateNode::GetNextAttack( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_comboNextAttack ];
}

SBehaviorComboAttack CBehaviorGraphComboStateNode::ResetNextAttack( CBehaviorGraphInstance& instance ) const
{
	SBehaviorComboAttack temp = instance[ i_comboNextAttack ];
	instance[ i_comboNextAttack ].Reset();
	return temp;
}

void CBehaviorGraphComboStateNode::InjectNextAnimation( CBehaviorGraphInstance& instance, const CName& animationAttack, const CName& animationParry ) const
{
	instance[ i_comboNextAttack ].m_attackAnimation = animationAttack;
	instance[ i_comboNextAttack ].m_parryAnimation = animationParry;
}

Bool CBehaviorGraphComboStateNode::SelectNextAttack( CBehaviorGraphInstance& instance, const Uint32 currLevel, const CBehaviorGraphComboStateNode::SComboDef& def ) const
{
	if ( def.m_way >= 0 && def.m_way < (Int32)m_comboWays.Size() )
	{
		const SBehaviorComboWay& comboWay =	m_comboWays[ def.m_way ];

		if ( comboWay.m_levels.Size() == 0 )
		{
			return false;
		}

		const Uint32						level =			GetNextComboLevel( comboWay, currLevel );
		const SBehaviorComboLevel&		comboLevel =	comboWay.m_levels[ level ];
		const SBehaviorComboDirection&	comboDir =		GetComboDir( comboLevel, def.m_dir );
		const SBehaviorComboDistance&	comboDist =		GetComboDist( comboDir, def.m_dist );
		const SBehaviorComboAnimation	comboAnim =		GetComboAnim( comboDist );

		if ( comboAnim.m_animationAttack.Empty() == false )
		{
			// Find animations
			const CSkeletalAnimationSetEntry* attackAnim = FindAnimation( instance, comboAnim.m_animationAttack );
			const CSkeletalAnimationSetEntry* parryAnim = FindAnimation( instance, comboAnim.m_animationParry );

			if ( !attackAnim )
			{
				return false;
			}

			SBehaviorComboAttack& attack = instance[ i_comboNextAttack ];

			// Level
			attack.m_level = level;
			attack.m_type = def.m_way;
			attack.m_distance = def.m_dist;
			attack.m_direction = def.m_dir;

			// Animations
			attack.m_attackAnimation = comboAnim.m_animationAttack;
			attack.m_parryAnimation = parryAnim ? comboAnim.m_animationParry : CName::NONE;

			// Durations
			attack.m_attackTime = attackAnim->GetDuration();
			attack.m_parryTime = parryAnim ? parryAnim->GetDuration() : 0.f;

			// Hit time and level

			//GetHitDataFromAnim( attackAnim, attack.m_attackHitTime, attack.m_attackHitLevel );
			//GetHitDataFromAnim( parryAnim, attack.m_parryHitTime, attack.m_parryHitLevel );

			GetHitDataFromAnim( attackAnim, 
				attack.m_attackHitTime, attack.m_attackHitTime1, attack.m_attackHitTime2, attack.m_attackHitTime3, 
				attack.m_attackHitLevel, attack.m_attackHitLevel1, attack.m_attackHitLevel2, attack.m_attackHitLevel3 );

			GetHitDataFromAnim( parryAnim, 
				attack.m_parryHitTime, attack.m_parryHitTime1, attack.m_parryHitTime2, attack.m_parryHitTime3, 
				attack.m_parryHitLevel, attack.m_parryHitLevel1, attack.m_parryHitLevel2, attack.m_parryHitLevel3 );

			return true;
		}
	}

	return false;
}

Uint32 CBehaviorGraphComboStateNode::GetNextComboLevel( const SBehaviorComboWay& way, Uint32 level ) const
{
	Uint32 maxLevel = Min( way.m_levels.Size() - 1, level );
	ASSERT( maxLevel >= 0 );

	Uint32 selLevel = maxLevel;

	/*for ( selLevel=maxLevel; i>=0; --i )
	{
		if ( HasAbility( way.m_levels[ selLevel ].m_abilityRequired ) )
		{
			break;
		}
	}*/

	return selLevel;
}

const SBehaviorComboDirection& CBehaviorGraphComboStateNode::GetComboDir( const SBehaviorComboLevel& level, EAttackDirection dir ) const
{
	switch ( dir )
	{
	case AD_Front:
		return level.m_dirFront;
	case AD_Back:
		return level.m_dirBack;
	case AD_Left:
		return level.m_dirLeft;
	case AD_Right:
		return level.m_dirRight;
	default:
		ASSERT( 0 );
		return level.m_dirFront;
	};
}

const SBehaviorComboDistance& CBehaviorGraphComboStateNode::GetComboDist( const SBehaviorComboDirection& dir, EAttackDistance dist ) const
{
	switch ( dist )
	{
	case ADIST_Small :
		return dir.m_distSmall;
	case ADIST_Medium :
		return dir.m_distMedium;
	case ADIST_Large :
		return dir.m_distLarge;
	default:
		ASSERT( 0 );
		return dir.m_distSmall;
	}
}

const SBehaviorComboAnimation CBehaviorGraphComboStateNode::GetComboAnim( const SBehaviorComboDistance& dist ) const
{
	if ( dist.m_animations.Size() > 0 )
	{
		// Rand
		Float totalWeight = 0;

		for ( Uint32 i=0; i<dist.m_animations.Size(); ++i )
		{
			totalWeight += Max( dist.m_animations[i].m_weight, 0.f );
		}

		Float randVal = GEngine->GetRandomNumberGenerator().Get< Float >( totalWeight );

		for ( Uint32 i=0; i<dist.m_animations.Size(); ++i )
		{
			Float weight = dist.m_animations[i].m_weight;

			if ( randVal <= weight )
			{
				return dist.m_animations[i];
			}

			randVal -= weight;
		}

		ASSERT( 0 );
	}

	return SBehaviorComboAnimation();
}

Bool CBehaviorGraphComboStateNode::HasAbility( const CName& /*ability*/, CBehaviorGraphInstance& /*instance*/ ) const
{
	return true;
}

Bool CBehaviorGraphComboStateNode::CanProcessEvent( CBehaviorGraphInstance& /*instance*/ ) const
{
	return false;
}

Bool CBehaviorGraphComboStateNode::ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return HasCachedAllValues( instance ) && event.GetEventID() == instance[ i_comboEvent ];
}

Bool CBehaviorGraphComboStateNode::FillComboDefinition( CBehaviorGraphInstance& instance, SComboDef& def ) const
{
	// Combo way
	Float wayVar = instance.GetFloatValue( m_varComboWay );
	if ( m_comboWays.Size() == 0 || wayVar < 0.f || wayVar > (Float)m_comboWays.Size() )
	{
		return false;
	}

	// Combo direction
	Float dirVar = instance.GetFloatValue( m_varComboDir );
	if ( dirVar < (Float)AD_Front || dirVar > (Float)AD_Back )
	{
		return false;
	}

	// Combo distance
	Float distVar = instance.GetFloatValue( m_varComboDist );
	if ( distVar < (Float)ADIST_Small || distVar > (Float)ADIST_Large )
	{
		return false;
	}

	def.m_way = (Int32)wayVar;
	def.m_dir = (EAttackDirection)(Int32)dirVar;
	def.m_dist = (EAttackDistance)(Int32)distVar;

	return true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphOffensiveComboStateNode );

void CBehaviorGraphOffensiveComboStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_allowAttack;
	compiler << i_comboEventTime;
}

void CBehaviorGraphOffensiveComboStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_comboEventTime ] = false;
	instance[ i_allowAttack ] = false;
}

void CBehaviorGraphOffensiveComboStateNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// Reset event flags
	instance[ i_comboEventTime ] = false;
	instance[ i_allowAttack ] = false;

	TBaseClass::Sample( context, instance, output );
}

void CBehaviorGraphOffensiveComboStateNode::StartCombo( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::StartCombo( instance );
}

void CBehaviorGraphOffensiveComboStateNode::StopCombo( CBehaviorGraphInstance& instance ) const
{
	instance[ i_allowAttack ] =		false;
	instance[ i_comboEventTime ] =	false;

	TBaseClass::StopCombo( instance );
}

Bool CBehaviorGraphOffensiveComboStateNode::CanStartNextAttack( CBehaviorGraphInstance& instance ) const
{
	Float time = m_slotB->GetAnimTime( instance );
	Float duration = m_slotB->GetAnimDuration( instance );

	return time + m_blendForAnim >= duration || IsAllowAttackEventOccurred( instance );
}

Bool CBehaviorGraphOffensiveComboStateNode::IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const
{
	return false;
}

Bool CBehaviorGraphOffensiveComboStateNode::CanProcessEvent( CBehaviorGraphInstance& instance ) const
{
	return !IsRunning( instance );
}

Bool CBehaviorGraphOffensiveComboStateNode::ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( TBaseClass::ProcessComboEvent( instance, event ) == false )
	{
		return false;
	}

	if ( HasNextAttack( instance ) )
	{
		return false;
	}

	if ( IsRunning( instance ) && !IsComboEventOccurred( instance ) )
	{
		return false;
	}

	SComboDef comboDef;

	if ( FillComboDefinition( instance, comboDef ) )
	{
#ifdef BEH_COMBO_DEBUG
		BEH_LOG( TXT("%s - Process combo event"), GetClass()->GetName().AsChar() );
#endif

		return SelectNextAttack( instance, GetComboLevel( instance ), comboDef );	
	}

	return false;
}

void CBehaviorGraphOffensiveComboStateNode::ProcessMainPose( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const
{
	// Combo event
	instance[ i_comboEventTime ] = ComboEventOccurred( pose );

	// Allow attack event
	instance[ i_allowAttack ] = AllowAttackEventOccurred( pose );
}

Bool CBehaviorGraphOffensiveComboStateNode::IsAllowAttackEventOccurred( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_allowAttack ];
}

Bool CBehaviorGraphOffensiveComboStateNode::IsComboEventOccurred( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_comboEventTime ];
}

Bool CBehaviorGraphOffensiveComboStateNode::ComboEventOccurred( const SBehaviorGraphOutput& pose ) const
{
	for ( Uint32 i=0; i<pose.m_numEventsFired; ++i )
	{
		if ( IsType< CExtAnimComboEvent >( pose.m_eventsFired[i].m_extEvent ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphOffensiveComboStateNode::AllowAttackEventOccurred( const SBehaviorGraphOutput& pose ) const
{
	if ( m_allowAttackEvent != CName::NONE )
	{
		for ( Uint32 i=0; i<pose.m_numEventsFired; ++i )
		{
			if ( pose.m_eventsFired[i].GetEventName() == m_allowAttackEvent )
			{
				return true;
			}
		}
	}

	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphOffensiveComboStateNode::GetComboType() const
{
	return TXT("Offensive");
}

String CBehaviorGraphOffensiveComboStateNode::GetComboDesc( CBehaviorGraphInstance& instance ) const
{
	String desc = TBaseClass::GetComboDesc( instance );

	if ( IsRunning( instance ) )
	{
		Bool isEvent = IsComboEventOccurred( instance );
		Bool isAllowAttack = IsAllowAttackEventOccurred( instance );

		desc += isEvent ? TXT("Event time\n") : TXT("Waiting for event\n");
		desc += isAllowAttack ? TXT("Allow attack ON\n") : TXT("Allow attack OFF\n");
	}

	return desc;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDefensiveComboStateNode );

void CBehaviorGraphDefensiveComboStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_hasVarHitTime;
	compiler << i_hasVarLevel;
	compiler << i_hasVarParry;
	compiler << i_timeToNextAttack;
}

void CBehaviorGraphDefensiveComboStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_hasVarHitTime ] = instance.HasFloatValue( m_varHitTime );
	instance[ i_hasVarLevel ] = instance.HasFloatValue( m_varLevel );
	instance[ i_hasVarParry ] = instance.HasFloatValue( m_varParry );
	
	ResetTimeToNextAttack( instance );
}

void CBehaviorGraphDefensiveComboStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	Float time = instance[ i_timeToNextAttack ];
	ASSERT( time <= 0.f );
#endif

	TBaseClass::OnDeactivated( instance );
}

Bool CBehaviorGraphDefensiveComboStateNode::HasCachedAllValues( CBehaviorGraphInstance& instance ) const
{
	return	TBaseClass::HasCachedAllValues( instance ) &&
			instance[ i_hasVarHitTime ] &&
			instance[ i_hasVarLevel ] &&
			instance[ i_hasVarParry ];
}

void CBehaviorGraphDefensiveComboStateNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( DefensiveComboState );
	if ( instance[ i_timeToNextAttack ] > 0.f )
	{
		instance[ i_timeToNextAttack ] -= timeDelta;
	}

	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphDefensiveComboStateNode::StartCombo( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::StartCombo( instance );
}

void CBehaviorGraphDefensiveComboStateNode::StopCombo( CBehaviorGraphInstance& instance ) const
{
	ResetTimeToNextAttack( instance );

	TBaseClass::StopCombo( instance );
}

Bool CBehaviorGraphDefensiveComboStateNode::CanProcessEvent( CBehaviorGraphInstance& instance ) const
{
	return true;
}

Bool CBehaviorGraphDefensiveComboStateNode::ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( TBaseClass::ProcessComboEvent( instance, event ) == false )
	{
		return false;
	}

	Float attackHitTime = GetAttackHitTime( instance );
	Uint32 level = GetLevelFromVariable( instance );
	Bool isHit = IsHit( instance );

	SComboDef comboDef;

	if ( attackHitTime >= 0.f && FillComboDefinition( instance, comboDef ) )
	{
		if ( SelectNextAttack( instance, level, comboDef ) )
		{
			Float blockHitTime = GetBlockHitTime( instance, isHit );
			if ( blockHitTime >= 0.f )
			{
				Float timeToNextAnim = attackHitTime - blockHitTime;
				if ( timeToNextAnim >= 0.f )
				{
					SetTimeToNextAttack( instance, timeToNextAnim );
					//StartPlayingRoot( instance, true );

					#ifdef BEH_COMBO_DEBUG
						BEH_LOG( TXT("%s - Process combo event"), GetClass()->GetName().AsChar() );
					#endif

					return true;
				}
#ifdef BEH_COMBO_DEBUG
				else
				{
					BEH_WARN( TXT("%s - FAIL: attackHitTime - blockHitTime < 0.0 "), GetClass()->GetName().AsChar() );	
				}
#endif				
			}
		}
	}

	ResetNextAttack( instance );
	ResetTimeToNextAttack( instance );
	
	if ( HasDefaultHitAnimation( instance ) && SelectDefaultHitAnimation( instance ) )
	{
		if ( attackHitTime > 0.f )
		{
			SetTimeToNextAttack( instance, attackHitTime );

			#ifdef BEH_COMBO_DEBUG
				BEH_LOG( TXT("%s - Process combo event - default"), GetClass()->GetName().AsChar() );
			#endif
		}
		return true;
	}

	return false;
}

void CBehaviorGraphDefensiveComboStateNode::ResetTimeToNextAttack( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Reset time to next anim"), GetClass()->GetName().AsChar() );
#endif

	SetTimeToNextAttack( instance, 0.f );
}

void CBehaviorGraphDefensiveComboStateNode::SetTimeToNextAttack( CBehaviorGraphInstance& instance, Float time ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Set time to next anim %f"), GetClass()->GetName().AsChar(), time );
#endif

	instance[ i_timeToNextAttack ] = time;
}

Bool CBehaviorGraphDefensiveComboStateNode::IsHit( CBehaviorGraphInstance& instance ) const
{
	return instance.GetFloatValue( m_varParry ) < 1.f;
}

Uint32 CBehaviorGraphDefensiveComboStateNode::GetLevelFromVariable( CBehaviorGraphInstance& instance ) const
{
	Float fVar = instance.GetFloatValue( m_varLevel );
	return fVar > 0.f ? (Uint32)fVar : 0;
}

Float CBehaviorGraphDefensiveComboStateNode::GetAttackHitTime( CBehaviorGraphInstance& instance ) const
{
	return instance.GetFloatValue( m_varHitTime );
}

Float CBehaviorGraphDefensiveComboStateNode::GetBlockHitTime( CBehaviorGraphInstance& instance, Bool isHit ) const
{
	CName animation = isHit ? GetNextAttack( instance ).m_attackAnimation : GetNextAttack( instance ).m_parryAnimation;
	const CSkeletalAnimationSetEntry* animEntry = FindAnimation( instance, animation );
	if ( animEntry == NULL )
	{
		return -1.f;
	}

	Vector hitTime;
	Vector hitLevel;
	if ( GetHitDataFromAnim( animEntry, 
		hitTime.A[0], hitTime.A[1], hitTime.A[2], hitTime.A[3], 
		hitLevel.A[0], hitLevel.A[1], hitLevel.A[2], hitLevel.A[3] ) )
	{
		return hitTime.A[0];
	}
	else
	{
		return -1.f;
	}
}

Bool CBehaviorGraphDefensiveComboStateNode::HasDefaultHitAnimation( CBehaviorGraphInstance& instance ) const
{
	return m_defaultHits.Size() > 0;
}

Bool CBehaviorGraphDefensiveComboStateNode::SelectDefaultHitAnimation( CBehaviorGraphInstance& instance ) const
{
	Uint32 randValue = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_defaultHits.Size() );
	CName animName = m_defaultHits[ randValue ];

	return SelectDefaultAnimation( instance, animName );
}

Bool CBehaviorGraphDefensiveComboStateNode::CanStartNextAttack( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_timeToNextAttack ] <= 0.f;
}

Bool CBehaviorGraphDefensiveComboStateNode::CurrAttackFinished( CBehaviorGraphInstance& instance ) const
{
	return TBaseClass::CurrAttackFinished( instance ) && instance[ i_timeToNextAttack ] <= 0;
}

Bool CBehaviorGraphDefensiveComboStateNode::IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const
{
	Bool canBeParried = !attack.m_parryAnimation.Empty();
	return canBeParried && !IsHit( instance );
}

Bool CBehaviorGraphDefensiveComboStateNode::AutoStart() const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphDefensiveComboStateNode::GetComboType() const
{
	return TXT("Defensive");
}

String CBehaviorGraphDefensiveComboStateNode::GetComboDesc( CBehaviorGraphInstance& instance ) const
{
	String desc = TBaseClass::GetComboDesc( instance );

	if ( IsRunning( instance ) )
	{
		Float timeToNextAnim = instance[ i_timeToNextAttack ];
		desc += timeToNextAnim >= 0.f ? String::Printf( TXT("Waiting [%.2f]"), timeToNextAnim ) : TXT("Waiting [-]");
	}

	return desc;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSimpleDefensiveComboStateNode );

void CBehaviorGraphSimpleDefensiveComboStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_hasVarHitTime;
	compiler << i_hasVarHitLevel;
	compiler << i_hasVarHit;
	compiler << i_hasVarParry;
	compiler << i_timeToNextAttack;
	compiler << i_animationMap;
	compiler << i_attackTimesTable;
	compiler << i_attackLevelTable;
	compiler << i_attackHitFlag;
	compiler << i_attackType;
}

void CBehaviorGraphSimpleDefensiveComboStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_hasVarHitTime ] = instance.HasFloatValue( m_varHitTime );
	instance[ i_hasVarHitLevel ] = instance.HasFloatValue( m_varHitLevel );

	instance[ i_hasVarHit ] = instance.HasFloatValue( m_varElemEnum );
	instance[ i_hasVarParry ] = instance.HasFloatValue( m_varParry );

	FillAnimationIndexTable( instance );

	ResetTimeToNextAttack( instance );
}

void CBehaviorGraphSimpleDefensiveComboStateNode::FillAnimationIndexTable( CBehaviorGraphInstance& instance ) const
{
	CEnum* enumType = SRTTI::GetInstance().FindEnum( m_enum );
	if ( enumType == NULL )
	{
		return;
	}

	const TDynArray< CName >& options = enumType->GetOptions();

	TDynArray< Int32 >& animationMap = instance[ i_animationMap ];
	animationMap.Resize( m_animElems.Size() );

	for ( Uint32 i=0; i<m_animElems.Size(); ++i )
	{
		const CName& enumName = m_animElems[i].m_enum;

		Bool found = false;

		for ( Uint32 j=0; j<options.Size() && !found; ++j )
		{
			if ( enumName == options[j] )
			{
				animationMap[i] = j;
				found = true;
			}
		}

		if ( found == false )
		{
			animationMap[i] = -1;
		}
	}
}

void CBehaviorGraphSimpleDefensiveComboStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	Float time = instance[ i_timeToNextAttack ];
	ASSERT( time <= 0.f );
#endif

	TBaseClass::OnDeactivated( instance );
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::HasCachedAllValues( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_hasVarHitTime ] &&
		instance[ i_hasVarHitLevel ] &&
		instance[ i_hasVarHit ] &&
		instance[ i_hasVarParry ];
}

void CBehaviorGraphSimpleDefensiveComboStateNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SimpleDefensiveComboState );
	if ( instance[ i_timeToNextAttack ] > 0.f )
	{
		instance[ i_timeToNextAttack ] -= timeDelta;
	}

	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphSimpleDefensiveComboStateNode::StartCombo( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::StartCombo( instance );
}

void CBehaviorGraphSimpleDefensiveComboStateNode::StopCombo( CBehaviorGraphInstance& instance ) const
{
	ResetTimeToNextAttack( instance );

	TBaseClass::StopCombo( instance );
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::InternalSelectNextAttackPart( CBehaviorGraphInstance& instance ) const
{
	const Int32 attackType = instance[ i_attackType ];
	const Bool attackHitFlag = instance[ i_attackHitFlag ];
	Vector& attackTimesTable = instance[ i_attackTimesTable ];
	Vector& attackLevelTable = instance[ i_attackLevelTable ];

	Float attackHitTime = attackTimesTable.A[0];
	Int32 attackHitLevel = (Int32)attackLevelTable.A[0];

	if ( attackHitTime >= 0.f && SelectNextAttack( instance, attackType, attackHitLevel ) )
	{
		Float blockHitTime = GetBlockHitTime( instance, attackHitFlag );
		if ( blockHitTime >= 0.f )
		{
			Float timeToNextAnim = attackHitTime - blockHitTime;
			if ( timeToNextAnim >= 0.f )
			{
				SetTimeToNextAttack( instance, timeToNextAnim );
			
				ShiftAttackTable( attackTimesTable, -1.f );
				ShiftAttackTable( attackLevelTable, 0.f );

#ifdef BEH_COMBO_DEBUG
				BEH_LOG( TXT("%s - Process combo event"), GetClass()->GetName().AsChar() );
#endif

				return true;
			}
#ifdef BEH_COMBO_DEBUG
			else
			{
				BEH_WARN( TXT("%s - FAIL: attackHitTime - blockHitTime < 0.0 "), GetClass()->GetName().AsChar() );	
			}
#endif				
		}
	}

	return false;
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::HasNextAttackPart( CBehaviorGraphInstance& instance ) const
{
	Vector& attackTimesTable = instance[ i_attackTimesTable ];
	Vector& attackLevelTable = instance[ i_attackLevelTable ];

	return attackTimesTable.A[0] >= 0.f && attackLevelTable.A[0] >= 0.f;
}

void CBehaviorGraphSimpleDefensiveComboStateNode::ShiftAttackTable( Vector& table, const Float defValue ) const
{
	table.A[0] = table.A[1];
	table.A[1] = table.A[2];
	table.A[2] = table.A[3];
	table.A[3] = defValue;
}

void CBehaviorGraphSimpleDefensiveComboStateNode::OnStartNextAttack( CBehaviorGraphInstance& instance ) const
{
	if ( HasNextAttackPart( instance ) )
	{
		Bool ret = InternalSelectNextAttackPart( instance );
		ASSERT( ret );
	}
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::SelectNextAttack( CBehaviorGraphInstance& instance, Int32 attackType, Int32 attackId ) const
{
	const TDynArray< Int32 >& animationMap = instance[ i_animationMap ];

	for ( Uint32 i=0; i<animationMap.Size(); ++i )
	{
		Int32 index = animationMap[i];

		if ( index == attackType )
		{
			const SBehaviorComboElem& comboAnimElem = m_animElems[ index ];

			Int32 selAttack = -1;
			Int32 animNum = (Int32)comboAnimElem.m_animations.Size();

			for ( Int32 j=0; j<animNum; ++j )
			{
				const SBehaviorComboAnim& comboAnim = comboAnimElem.m_animations[ j ];
				if ( comboAnim.m_id == attackId )
				{
					selAttack = j;
					break;
				}
			}

			if ( selAttack < 0 )
			{
				return false;
			}

			const SBehaviorComboAnim& comboAnim = comboAnimElem.m_animations[ selAttack ];

			if ( comboAnim.m_animationAttack.Empty() == false )
			{
				// Find animations
				const CSkeletalAnimationSetEntry* attackAnim = FindAnimation( instance, comboAnim.m_animationAttack );
				const CSkeletalAnimationSetEntry* parryAnim = FindAnimation( instance, comboAnim.m_animationParry );

				if ( !attackAnim )
				{
					return false;
				}

				SBehaviorComboAttack& attack = instance[ i_comboNextAttack ];

				// Level
				attack.m_level = 0;
				attack.m_type = 0;
				attack.m_distance = ADIST_Small;
				attack.m_direction = AD_Front;

				// Animations
				attack.m_attackAnimation = comboAnim.m_animationAttack;
				attack.m_parryAnimation = parryAnim ? comboAnim.m_animationParry : CName::NONE;

				// Durations
				attack.m_attackTime = attackAnim->GetDuration();
				attack.m_parryTime = parryAnim ? parryAnim->GetDuration() : 0.f;

				// Hit time and level
				//GetHitDataFromAnim( attackAnim, attack.m_attackHitTime, attack.m_attackHitLevel );
				//GetHitDataFromAnim( parryAnim, attack.m_parryHitTime, attack.m_parryHitLevel );

				GetHitDataFromAnim( attackAnim, 
					attack.m_attackHitTime, attack.m_attackHitTime1, attack.m_attackHitTime2, attack.m_attackHitTime3, 
					attack.m_attackHitLevel, attack.m_attackHitLevel1, attack.m_attackHitLevel2, attack.m_attackHitLevel3 );

				GetHitDataFromAnim( parryAnim, 
					attack.m_parryHitTime, attack.m_parryHitTime1, attack.m_parryHitTime2, attack.m_parryHitTime3, 
					attack.m_parryHitLevel, attack.m_parryHitLevel1, attack.m_parryHitLevel2, attack.m_parryHitLevel3 );

				return true;
			}

			return false;
		}
	}

	return false;
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::CanProcessEvent( CBehaviorGraphInstance& instance ) const
{
	return true;
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::ProcessComboEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( TBaseClass::ProcessComboEvent( instance, event ) == false )
	{
		return false;
	}

	instance[ i_attackType ] = GetAttackHit( instance );
	instance[ i_attackHitFlag ] = IsHit( instance );
	instance[ i_attackTimesTable ] = GetAttackHitTime( instance );
	instance[ i_attackLevelTable ] = GetAttackHitLevel( instance );

	{
		Vector temp = instance[ i_attackTimesTable ];
		
		//if ( temp.A[3] > 0.f )
		//{
		//	BEH_LOG( TXT("Four hits") );
		//}
		//else
		if ( temp.A[2] > 0.f )
		{
			BEH_LOG( TXT("Tree hits") );
		}
		else if ( temp.A[1] > 0.f )
		{
			BEH_LOG( TXT("Two hits") );
		}
		else if ( temp.A[0] > 0.f )
		{
			BEH_LOG( TXT("One hit") );
		}
		else
		{
			BEH_LOG( TXT("Zero hits") );
		}
	}

	if ( InternalSelectNextAttackPart( instance ) )
	{
		return true;
	}
	else
	{
		ResetNextAttack( instance );
		ResetTimeToNextAttack( instance );

		return false;
	}

	/*Float attackHitTime = attackHitTimeVec.A[0];

	if ( attackHitTime >= 0.f && SelectNextAttack( instance, attack ) )
	{
		Float blockHitTime = GetBlockHitTime( instance, isHit );
		if ( blockHitTime >= 0.f )
		{
			Float timeToNextAnim = attackHitTime - blockHitTime;
			if ( timeToNextAnim >= 0.f )
			{
				SetTimeToNextAttack( instance, timeToNextAnim );
				//StartPlayingRoot( instance, true );

#ifdef BEH_COMBO_DEBUG
				BEH_LOG( TXT("%s - Process combo event"), GetClass()->GetName().AsChar() );
#endif

				return true;
			}
#ifdef BEH_COMBO_DEBUG
			else
			{
				BEH_WARN( TXT("%s - FAIL: attackHitTime - blockHitTime < 0.0 "), GetClass()->GetName().AsChar() );	
			}
#endif				
		}

	}

	ResetNextAttack( instance );
	ResetTimeToNextAttack( instance );

	return false;*/
}

void CBehaviorGraphSimpleDefensiveComboStateNode::ResetTimeToNextAttack( CBehaviorGraphInstance& instance ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Reset time to next anim"), GetClass()->GetName().AsChar() );
#endif

	SetTimeToNextAttack( instance, 0.f );

	instance[ i_attackLevelTable ] = -Vector::ONES;
	instance[ i_attackTimesTable ] = -Vector::ONES;
	instance[ i_attackHitFlag ] = true;
	instance[ i_attackType ] = 0;
}

void CBehaviorGraphSimpleDefensiveComboStateNode::SetTimeToNextAttack( CBehaviorGraphInstance& instance, Float time ) const
{
#ifdef BEH_COMBO_DEBUG
	BEH_LOG( TXT("%s - Set time to next anim %f"), GetClass()->GetName().AsChar(), time );
#endif

	instance[ i_timeToNextAttack ] = time;
}

Int32 CBehaviorGraphSimpleDefensiveComboStateNode::GetAttackHit( CBehaviorGraphInstance& instance ) const
{
	return (Int32)instance.GetFloatValue( m_varElemEnum );
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::IsHit( CBehaviorGraphInstance& instance ) const
{
	return instance.GetFloatValue( m_varParry ) < 1.f;
}

Vector CBehaviorGraphSimpleDefensiveComboStateNode::GetAttackHitTime( CBehaviorGraphInstance& instance ) const
{
	return instance.GetVectorValue( m_varHitTime );
}

Vector CBehaviorGraphSimpleDefensiveComboStateNode::GetAttackHitLevel( CBehaviorGraphInstance& instance ) const
{
	return instance.GetVectorValue( m_varHitLevel );
}

Float CBehaviorGraphSimpleDefensiveComboStateNode::GetBlockHitTime( CBehaviorGraphInstance& instance, Bool isHit ) const
{
	CName animation = isHit ? GetNextAttack( instance ).m_attackAnimation : GetNextAttack( instance ).m_parryAnimation;
	const CSkeletalAnimationSetEntry* animEntry = FindAnimation( instance, animation );
	if ( animEntry == NULL )
	{
		return -1.f;
	}

	Vector hitTime;
	Vector hitLevel;

	if ( GetHitDataFromAnim( animEntry, hitTime, hitLevel ) )
	{
		return hitTime.A[0];
	}
	else
	{
		return -1.f;
	}
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::CanStartNextAttack( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_timeToNextAttack ] <= 0.f;
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::CurrAttackFinished( CBehaviorGraphInstance& instance ) const
{
	return TBaseClass::CurrAttackFinished( instance ) && instance[ i_timeToNextAttack ] <= 0;
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::IsAttackParried( CBehaviorGraphInstance& instance, SBehaviorComboAttack& attack ) const
{
	Bool canBeParried = !attack.m_parryAnimation.Empty();
	return canBeParried && !IsHit( instance );
}

Bool CBehaviorGraphSimpleDefensiveComboStateNode::AutoStart() const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphSimpleDefensiveComboStateNode::GetComboType() const
{
	return TXT("Simple defensive");
}

String CBehaviorGraphSimpleDefensiveComboStateNode::GetComboDesc( CBehaviorGraphInstance& instance ) const
{
	String desc = TBaseClass::GetComboDesc( instance );

	if ( IsRunning( instance ) )
	{
		Float timeToNextAnim = instance[ i_timeToNextAttack ];
		desc += timeToNextAnim >= 0.f ? String::Printf( TXT("Waiting [%.2f]"), timeToNextAnim ) : TXT("Waiting [-]");
	}

	return desc;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( IBehaviorGraphComboModifier );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComboLevelModifier );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComboStartingAnimationModifier );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComboCooldownModifier );

void CBehaviorGraphComboLevelModifier::Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const
{
	combo->SetComboLevel( instance, m_level );
}

void CBehaviorGraphComboStartingAnimationModifier::Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const
{
	if ( m_animationAttack.Empty() == false )
	{
		combo->InjectNextAnimation( instance, m_animationAttack, m_animationParry );
	}
}

void CBehaviorGraphComboCooldownModifier::Modify( CBehaviorGraphInstance& instance, CBehaviorGraphComboStateNode* combo ) const
{
	combo->SetCooldownDuration( instance, m_cooldown );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComboTransitionInterface );

Bool CBehaviorGraphComboTransitionInterface::CanProcessEvent( CBehaviorGraphInstance& instance ) const
{
	CBehaviorGraphComboStateNode* combo = GetCombo();
	return combo && combo->CanProcessEvent( instance );
}

Bool CBehaviorGraphComboTransitionInterface::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	CBehaviorGraphComboStateNode* combo = GetCombo();
	if ( combo )
	{
		return combo->ProcessEvent( instance, event );
	}
	return false;
}

Bool CBehaviorGraphComboTransitionInterface::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	CBehaviorGraphComboStateNode* combo = GetCombo();
	if ( combo )
	{
		return combo->ProcessForceEvent( instance, event );
	}
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CBehaviorGraphComboTransitionInterface::GetBorderColor( Bool enabled ) const
{
	return enabled ? Color( 75, 175, 210 ) : Color::RED;
}

Color CBehaviorGraphComboTransitionInterface::GetClientColor( Bool enabled ) const
{
	return enabled ? Color( 135, 142, 149 ) : Color( 200, 172, 172 );
}

#endif

void CBehaviorGraphComboTransitionInterface::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	CBehaviorGraphComboStateNode* combo = GetCombo();
	if ( combo )
	{
		combo->ProcessCooldown( instance, timeDelta );
	}
}

Bool CBehaviorGraphComboTransitionInterface::CheckTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	CBehaviorGraphComboStateNode* combo = GetCombo();
	if ( combo && !combo->IsRunning( instance ) )
	{
		return combo->ReadyToStart( instance );
	}
	return false;
}

Bool CBehaviorGraphComboTransitionInterface::TestTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	return CheckTransitionCondition( instance );
}

Bool CBehaviorGraphComboTransitionInterface::SteerToCombo() const
{
	CBehaviorGraphStateTransitionBlendNode* parentNode = FindParent< CBehaviorGraphStateTransitionBlendNode >();
	if ( parentNode )
	{
		CBehaviorGraphComboStateNode* combo = Cast< CBehaviorGraphComboStateNode >( parentNode->GetDestState() );
		return combo != NULL;
	}
	else
	{
		ASSERT( parentNode );
		return false;
	}
}

CBehaviorGraphComboStateNode* CBehaviorGraphComboTransitionInterface::GetCombo() const
{
	CBehaviorGraphStateTransitionBlendNode* parentNode = FindParent< CBehaviorGraphStateTransitionBlendNode >();
	if ( parentNode )
	{
		return SteerToCombo() ? 
			Cast< CBehaviorGraphComboStateNode >( parentNode->GetDestState() ) : 
			Cast< CBehaviorGraphComboStateNode >( parentNode->GetSourceState() );
	}
	else
	{
		ASSERT( parentNode );
		return NULL;
	}
}

void CBehaviorGraphComboTransitionInterface::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( SteerToCombo() == true )
	{
		ApplyComboModifications( instance );
	}
}

void CBehaviorGraphComboTransitionInterface::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( SteerToCombo() == false )
	{
		ApplyComboModifications( instance );
	}
}

void CBehaviorGraphComboTransitionInterface::ApplyComboModifications( CBehaviorGraphInstance& instance ) const
{
	CBehaviorGraphComboStateNode* combo = GetCombo();
	if ( combo )
	{
		for ( Uint32 i=0; i<m_modifiers.Size(); ++i )
		{
			if ( m_modifiers[i] )
			{
				m_modifiers[i]->Modify( instance, combo );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphComboTransitionNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphComboTransitionNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_comboInterface = CreateObject< CBehaviorGraphComboTransitionInterface >( this );
}

Color CBehaviorGraphComboTransitionNode::GetBorderColor() const
{
	return m_comboInterface->GetBorderColor( m_isEnabled );
}

Color CBehaviorGraphComboTransitionNode::GetClientColor() const
{
	return m_comboInterface->GetClientColor( m_isEnabled );
}

#endif

Bool CBehaviorGraphComboTransitionNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_comboInterface->CanProcessEvent( instance ) )
	{
		Bool ret = TBaseClass::ProcessEvent( instance, event );
		ret |= m_comboInterface->ProcessEvent( instance, event );
		return ret;
	}
	
	return false;
}

Bool CBehaviorGraphComboTransitionNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_comboInterface->CanProcessEvent( instance ) )
	{
		Bool ret = TBaseClass::ProcessForceEvent( instance, event );
		ret |= m_comboInterface->ProcessForceEvent( instance, event );
		return ret;
	}

	return false;
}

void CBehaviorGraphComboTransitionNode::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnStartBlockUpdate( context, instance, timeDelta );

	m_comboInterface->OnStartBlockUpdate( context, instance, timeDelta );
}

Bool CBehaviorGraphComboTransitionNode::CheckTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	if ( !m_isEnabled )
	{
		return false;
	}

	if ( m_comboInterface->SteerToCombo() )
	{
		if ( !m_transitionCondition || ( m_transitionCondition && TBaseClass::CheckTransitionCondition( instance ) ) )
		{
			return m_comboInterface->CheckTransitionCondition( instance );
		}
	}
	else
	{
		return TBaseClass::CheckTransitionCondition( instance );
	}

	return false;
}

Bool CBehaviorGraphComboTransitionNode::TestTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	if ( !m_isEnabled )
	{
		return false;
	}

	if ( m_comboInterface->SteerToCombo() )
	{
		if ( !m_transitionCondition || ( m_transitionCondition && TBaseClass::TestTransitionCondition( instance ) ) )
		{
			return m_comboInterface->TestTransitionCondition( instance );
		}
	}
	else
	{
		return TBaseClass::TestTransitionCondition( instance );
	}

	return false;
}

void CBehaviorGraphComboTransitionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	m_comboInterface->OnActivated( instance );

	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphComboTransitionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	m_comboInterface->OnDeactivated( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphGlobalComboTransitionNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphGlobalComboTransitionNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_comboInterface = CreateObject< CBehaviorGraphComboTransitionInterface >( this );
}

Color CBehaviorGraphGlobalComboTransitionNode::GetBorderColor() const
{
	return m_comboInterface->GetBorderColor( m_isEnabled );
}

Color CBehaviorGraphGlobalComboTransitionNode::GetClientColor() const
{
	return m_comboInterface->GetClientColor( m_isEnabled );
}

#endif

Bool CBehaviorGraphGlobalComboTransitionNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_comboInterface->CanProcessEvent( instance ) )
	{
		Bool ret = TBaseClass::ProcessEvent( instance, event );
		ret |= m_comboInterface->ProcessEvent( instance, event );
		return ret;
	}

	return false;
}

Bool CBehaviorGraphGlobalComboTransitionNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_comboInterface->CanProcessEvent( instance ) )
	{
		Bool ret = TBaseClass::ProcessForceEvent( instance, event );
		ret |= m_comboInterface->ProcessForceEvent( instance, event );
		return ret;
	}

	return false;
}

void CBehaviorGraphGlobalComboTransitionNode::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnStartBlockUpdate( context, instance, timeDelta );

	m_comboInterface->OnStartBlockUpdate( context, instance, timeDelta );
}

Bool CBehaviorGraphGlobalComboTransitionNode::CheckTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	if ( !m_isEnabled )
	{
		return false;
	}

	if ( m_comboInterface->SteerToCombo() )
	{
		if ( !m_transitionCondition || ( m_transitionCondition && TBaseClass::CheckTransitionCondition( instance ) ) )
		{
			return m_comboInterface->CheckTransitionCondition( instance );
		}
	}
	else
	{
		ASSERT( m_comboInterface->SteerToCombo() );
		return TBaseClass::CheckTransitionCondition( instance );
	}

	return false;
}

Bool CBehaviorGraphGlobalComboTransitionNode::TestTransitionCondition( CBehaviorGraphInstance& instance ) const
{
	if ( !m_isEnabled )
	{
		return false;
	}

	if ( m_comboInterface->SteerToCombo() )
	{
		if ( !m_transitionCondition || ( m_transitionCondition && TBaseClass::TestTransitionCondition( instance ) ) )
		{
			return m_comboInterface->TestTransitionCondition( instance );
		}
	}
	else
	{
		ASSERT( m_comboInterface->SteerToCombo() );
		return TBaseClass::TestTransitionCondition( instance );
	}

	return false;
}

void CBehaviorGraphGlobalComboTransitionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	m_comboInterface->OnActivated( instance );

	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphGlobalComboTransitionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	m_comboInterface->OnDeactivated( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBehaviorComboAnimation );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboDistance );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboDirection );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboLevel );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboWay );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboAttack );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboAnim );
IMPLEMENT_ENGINE_CLASS( SBehaviorComboElem );
