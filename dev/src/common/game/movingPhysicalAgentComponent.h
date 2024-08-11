/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "..\engine\characterControllerParam.h"
#include "actorsManager.h"

class CMRPhysicalCharacter;
class CCharacterControllersManager;

// CCharacterComponent = CMovingAgentComponent + possible fallback to physics capsule
class CMovingPhysicalAgentComponent : public CMovingAgentComponent, public IPhysicalCollisionTriggerCallback
{
	DECLARE_ENGINE_CLASS( CMovingPhysicalAgentComponent, CMovingAgentComponent, 0 );

protected:
	CMRPhysicalCharacter*					m_physRepresentation;

	CPhysicsWorld*							m_physWorld;

	InteractionPriorityType					m_pushPriorityOriginal;

	virtual void CreateCustomMoveRepresentations( CWorld* world ) override;
public:
	CMovingPhysicalAgentComponent();
	virtual ~CMovingPhysicalAgentComponent();

	//! Component was attached to world
	virtual void OnAttached( CWorld *world );

	//! Component was detached from world
	virtual void OnDetached( CWorld *world );

public:
	RED_INLINE const CMRPhysicalCharacter* GetPhysicalCharacter() const { return m_physRepresentation; }

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	
	RED_INLINE CPhysicsWorld* GetPhysicsWorld() { return m_physWorld; }

    void ForceMoveToPosition( const Vector& position, Bool resetZAxis );
    void SetCollisionType( const CName& typeName);

	// platforms
	const Bool IsOnPlatform();
	const Vector& GetPlatformLocalPos();
	const Float GetPlatformRotation();

	const Bool IsCollisionPredictionEnabled();
	const Bool IsNearWater();
	const Bool CanPush();
	const Bool IsShapeHit();
	const Bool IsSlidingEnabled();
	const Bool IsUpdatingVirtualRadius();
	const Float	GetPushingTime();


	// states
	void SetAnimatedMovement( Bool enable );	
	Bool IsAnimatedMovement();
	void SetGravity( Bool enable );
	Bool IsGravity();
	void SetNeedsBehaviorCallback( Bool enable );
	Bool IsBehaviorCallbackNeeded() const;
	void EnableAdditionalVerticalSlidingIteration( const Bool enable );
	Bool IsAdditionalVerticalSlidingIterationEnabled();
	const Bool CanPhysicalMove();
	const Bool ShouldPhysicalMove();
	const Bool IsFalling();
	const Bool IsTeleport();
	const Bool IsStandingOnDynamic();
	const Float GetRagdollPushMultiplier();
	Bool IsPhysicalMovementEnabled() const;

	// swimming
	void SetSwimming( Bool enable );
	Bool IsSwimming() const;
	Float GetWaterLevel();
	Float GetSubmergeDepth();
	void SetDiving( Bool diving );
	Bool IsDiving() const;
	void SetRagdollToSwimming( Bool enable );
	const Float GetEmergeSpeed();
	const Float GetSubmergeSpeed();

	// virtual controllers
	const Uint32 GetVirtualControllerCount();

	// sliding
	const Bool IsSliding();
	const Uint32 GetSlidingState();
	const Float GetSlideCoef();
	const Vector GetSlidingDir();

	// movement
	const Vector GetLastMoveVector();
	const Vector GetCurrentMovementVectorRef();
	const Vector GetInternalVelocity();
	const Vector GetExternalDisp();
	const Vector GetInputDisp();
	const Float GetSpeedMul();

	void CreateCharacterCapsuleWS( FixedCapsule& capsule ) const;

	// interactions priority
	InteractionPriorityType GetInteractionPriority() const;
	InteractionPriorityType SetInteractionPriority( InteractionPriorityType interactionPriority );
	void SetOriginalInteractionPriority( InteractionPriorityType interactionPriority );
	void RestoreOriginalInteractionPriority();
	InteractionPriorityType GetOriginalInteractionPriority();
	CMovingPhysicalAgentComponent* SetUnpushableTarget( CMovingPhysicalAgentComponent* targetMAC );
	InteractionPriorityType GetActualInteractionPriority() const;

	void InvalidatePhysicsCache();

	// collisions
	void EnableStaticCollisions( Bool enable );		//! Allows to ignore static colliders
	Bool IsStaticCollisionsEnabled();
	void EnableDynamicCollisions( Bool enable );		//! Allows to ignore dynamic colliders
	Bool IsDynamicCollisionsEnabled();
	const Bool GetGroundGridCollisionOn( const Uint32 dir );

	virtual void EnableCombatMode( Bool combat );

	// collision data
	void ResetCollisionObstaclesData();
	void ResetCollisionCharactersData();

	// virtual controllers
	virtual void EnableVirtualController( const CName& virtualControllerName, const Bool enabled ) override;

	// virtual radius
	virtual void SetVirtualRadius( const CName& radiusName, Bool immediately, const CName& virtualControllerName ) override;
	virtual void ResetVirtualRadius( const CName& virtualControllerName ) override;
	const Float GetVirtualRadius();

	// height
	virtual void SetHeight( const Float Height );
	virtual void ResetHeight();
	const Float GetHeight();

	// Z-Test
	Bool DoTraceZTest( const Vector& pointWS, Vector& outPosition ) const;

	// slope pitch
	Float GetSlopePitch() const;
	Float GetPhysicalPitch() const;

	// terrain
	Bool IsCollidingDown() const;
	Bool IsCollidingUp() const;
	Bool IsCollidingSide() const;
	Vector GetTerrainNormal( Bool damped = false ) const;

	// radius
	Float GetPhysicalRadius() const;
	const Float GetCurrentRadius();

	virtual void onCharacterTouch( THandle< IScriptable > m_triggeredComponent, SActorShapeIndex& m_triggeredBodyIndex );

	virtual const SPhysicalMaterial* GetCurrentStandPhysicalMaterial() const override;
	virtual enum ECharacterPhysicsState GetCurrentPhysicsState() const override;

	struct ResolveSeparationContext
	{
 		typedef TPair< CMovingPhysicalAgentComponent*, CMovingPhysicalAgentComponent* > AgentsPair;

		struct ControlPairHasher
		{
			static Uint32 GetHash( const AgentsPair& p )
			{
				return GetPtrHash( p.m_first ) ^ GetPtrHash( p.m_second );
			}
		};

		THashSet< AgentsPair, ControlPairHasher > testedPairs;

		void Clear() 
		{ 
			testedPairs.ClearFast(); 
		}
	};


	Uint32 ResolveSeparation( ResolveSeparationContext& context );

protected:

	virtual void OnSkeletonChanged() override;

	IMovableRepresentation* DetermineMoveRepresentation() const override;

private:

	struct SeparateFunctor : public CQuadTreeStorage< CActor, CActorsManagerMemberData >::DefaultFunctor
	{
		SeparateFunctor( CMovingPhysicalAgentComponent* self, CCharacterControllersManager* mgr, ResolveSeparationContext& context );
		
		enum { SORT_OUTPUT = false };
		
		Bool operator()( const CActorsManagerMemberData& element );

		Uint32 GetColliders() const { return m_colliders; }

		const Vector& GetSeparation() const { return m_separation; }

	private:
		Vector							m_separation;
		CMovingPhysicalAgentComponent*	m_self;
		CCharacterControllersManager*	m_mgr;
		ResolveSeparationContext&		m_context;
		Uint32							m_colliders;
	};

	void funcIsPhysicalMovementEnabled( CScriptStackFrame& stack, void* result );	
	void funcSetAnimatedMovement( CScriptStackFrame& stack, void* result );	
	void funcIsAnimatedMovement( CScriptStackFrame& stack, void* result );
	void funcGetPhysicalState( CScriptStackFrame& stack, void* result );
	void funcSetGravity( CScriptStackFrame& stack, void* result );
	void funcSetBehaviorCallbackNeed( CScriptStackFrame& stack, void* result );
	void funcSetSwimming( CScriptStackFrame& stack, void* result );
	void funcGetWaterLevel( CScriptStackFrame& stack, void* result );
	void funcGetSubmergeDepth( CScriptStackFrame& stack, void* result );
	void funcSetDiving( CScriptStackFrame& stack, void* result );
	void funcIsDiving( CScriptStackFrame& stack, void* result );
	void funcSetEmergeSpeed( CScriptStackFrame& stack, void* result );
	void funcGetEmergeSpeed( CScriptStackFrame& stack, void* result );
	void funcSetSubmergeSpeed( CScriptStackFrame& stack, void* result );
	void funcGetSubmergeSpeed( CScriptStackFrame& stack, void* result );
	void funcSetRagdollPushingMul( CScriptStackFrame& stack, void* result );
	void funcGetRagdollPushingMul( CScriptStackFrame& stack, void* result );
    void funcApplyVelocity( CScriptStackFrame& stack, void* result );
	void funcRegisterEventListener( CScriptStackFrame& stack, void* result );
    void funcUnregisterEventListener( CScriptStackFrame& stack, void* result );
	void funcSetPushable( CScriptStackFrame& stack, void* result );
	void funcIsOnGround( CScriptStackFrame& stack, void* result );
	void funcIsCollidesWithCeiling( CScriptStackFrame& stack, void* result );
	void funcIsCollidesOnSide( CScriptStackFrame& stack, void* result );
	void funcIsFalling( CScriptStackFrame& stack, void* result );
	void funcIsSliding( CScriptStackFrame& stack, void* result );
	void funcGetSlideCoef( CScriptStackFrame& stack, void* result );
	void funcGetSlideDir( CScriptStackFrame& stack, void* result );
	void funcSetSlidingSpeed( CScriptStackFrame& stack, void* result );
	void funcSetSlidingLimits( CScriptStackFrame& stack, void* result );
	void funcSetSliding( CScriptStackFrame& stack, void* result );
	void funcEnableAdditionalVerticalSlidingIteration( CScriptStackFrame& stack, void* result );
	void funcIsAdditionalVerticalSlidingIterationEnabled( CScriptStackFrame& stack, void* result );
	void funcSetTerrainLimits( CScriptStackFrame& stack, void* result );
	void funcSetTerrainInfluence( CScriptStackFrame& stack, void* result );
	void funcGetCapsuleHeight( CScriptStackFrame& stack, void* result );
	void funcGetCapsuleRadius( CScriptStackFrame& stack, void* result );
	void funcGetSlopePitch( CScriptStackFrame& stack, void* result );
	void funcGetTerrainNormal( CScriptStackFrame& stack, void* result );
	void funcGetTerrainNormalWide(  CScriptStackFrame& stack, void* result );
	void funcSetVirtualControllersPitch( CScriptStackFrame& stack, void* result );
	void funcGetCollisionDataCount( CScriptStackFrame& stack, void* result );
	void funcGetCollisionData( CScriptStackFrame& stack, void* result );
	void funcGetCollisionCharacterDataCount( CScriptStackFrame& stack, void* result );
	void funcGetCollisionCharacterData( CScriptStackFrame& stack, void* result );
	void funcGetGroundGridCollisionOn( CScriptStackFrame& stack, void* result );
	void funcEnableCollisionPrediction( CScriptStackFrame& stack, void* result );
	void funcEnableVirtualControllerCollisionResponse( CScriptStackFrame& stack, void* result );
	void funcGetMaterialName( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CMovingPhysicalAgentComponent );
	PARENT_CLASS( CMovingAgentComponent );
	NATIVE_FUNCTION( "IsPhysicalMovementEnabled", funcIsPhysicalMovementEnabled );
	NATIVE_FUNCTION( "SetAnimatedMovement", funcSetAnimatedMovement );
	NATIVE_FUNCTION( "IsAnimatedMovement", funcIsAnimatedMovement );
	NATIVE_FUNCTION( "GetPhysicalState", funcGetPhysicalState );
	NATIVE_FUNCTION( "SetGravity", funcSetGravity );
	NATIVE_FUNCTION( "SetBehaviorCallbackNeed", funcSetBehaviorCallbackNeed );	
	NATIVE_FUNCTION( "SetSwimming", funcSetSwimming );
	NATIVE_FUNCTION( "GetWaterLevel", funcGetWaterLevel );
	NATIVE_FUNCTION( "GetSubmergeDepth", funcGetSubmergeDepth );
	NATIVE_FUNCTION( "SetDiving", funcSetDiving );
	NATIVE_FUNCTION( "IsDiving", funcIsDiving );
	NATIVE_FUNCTION( "SetEmergeSpeed", funcSetEmergeSpeed );
	NATIVE_FUNCTION( "GetEmergeSpeed", funcGetEmergeSpeed );
	NATIVE_FUNCTION( "SetSubmergeSpeed", funcSetSubmergeSpeed );
	NATIVE_FUNCTION( "GetSubmergeSpeed", funcGetSubmergeSpeed );
	NATIVE_FUNCTION( "SetRagdollPushingMul", funcSetRagdollPushingMul );
	NATIVE_FUNCTION( "GetRagdollPushingMul", funcGetRagdollPushingMul );
	NATIVE_FUNCTION( "ApplyVelocity", funcApplyVelocity );
	NATIVE_FUNCTION( "RegisterEventListener", funcRegisterEventListener );
    NATIVE_FUNCTION( "UnregisterEventListener", funcUnregisterEventListener );
	NATIVE_FUNCTION( "SetPushable", funcSetPushable );
	NATIVE_FUNCTION( "IsOnGround", funcIsOnGround );
	NATIVE_FUNCTION( "IsCollidesWithCeiling", funcIsCollidesWithCeiling );
	NATIVE_FUNCTION( "IsCollidesOnSide", funcIsCollidesOnSide );
	NATIVE_FUNCTION( "IsFalling", funcIsFalling );
	NATIVE_FUNCTION( "IsSliding", funcIsSliding );
	NATIVE_FUNCTION( "GetSlideDir", funcGetSlideDir );
	NATIVE_FUNCTION( "GetSlideCoef", funcGetSlideCoef );
	NATIVE_FUNCTION( "SetSlidingSpeed", funcSetSlidingSpeed );
	NATIVE_FUNCTION( "SetSlidingLimits", funcSetSlidingLimits );
	NATIVE_FUNCTION( "SetSliding", funcSetSliding );
	NATIVE_FUNCTION( "EnableAdditionalVerticalSlidingIteration", funcEnableAdditionalVerticalSlidingIteration );
	NATIVE_FUNCTION( "IsAdditionalVerticalSlidingIterationEnabled", funcIsAdditionalVerticalSlidingIterationEnabled );
	NATIVE_FUNCTION( "SetTerrainLimits", funcSetTerrainLimits );
	NATIVE_FUNCTION( "SetTerrainInfluence", funcSetTerrainInfluence );
	NATIVE_FUNCTION( "GetCapsuleHeight", funcGetCapsuleHeight );
	NATIVE_FUNCTION( "GetCapsuleRadius", funcGetCapsuleRadius );
	NATIVE_FUNCTION( "GetSlopePitch", funcGetSlopePitch );
	NATIVE_FUNCTION( "GetTerrainNormal", funcGetTerrainNormal );
	NATIVE_FUNCTION( "GetTerrainNormalWide", funcGetTerrainNormalWide );	
	NATIVE_FUNCTION( "SetVirtualControllersPitch", funcSetVirtualControllersPitch );
	NATIVE_FUNCTION( "GetCollisionDataCount", funcGetCollisionDataCount );
	NATIVE_FUNCTION( "GetCollisionData", funcGetCollisionData );
	NATIVE_FUNCTION( "GetCollisionCharacterDataCount", funcGetCollisionCharacterDataCount );
	NATIVE_FUNCTION( "GetCollisionCharacterData", funcGetCollisionCharacterData );
	NATIVE_FUNCTION( "GetGroundGridCollisionOn", funcGetGroundGridCollisionOn );
	NATIVE_FUNCTION( "EnableCollisionPrediction", funcEnableCollisionPrediction );
	NATIVE_FUNCTION( "EnableVirtualControllerCollisionResponse", funcEnableVirtualControllerCollisionResponse );
	NATIVE_FUNCTION( "GetMaterialName", funcGetMaterialName );
END_CLASS_RTTI();

