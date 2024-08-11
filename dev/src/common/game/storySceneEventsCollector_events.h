
#pragma once

#include "storySceneIncludes.h"
#include "storySceneCameraDefinition.h"
#include "../core/engineQsTransform.h"
#include "../engine/lightComponent.h"
#include "../engine/dimmerComponent.h"
#include "../engine/videoPlayer.h"

namespace StorySceneEventsCollector
{
	//////////////////////////////////////////////////////////////////////////

	struct Event
	{
		const CStorySceneEvent*		m_event;
		Float						m_eventTimeAbs;
		Float						m_eventTimeLocal;

		RED_INLINE const CStorySceneEvent* GetId() const { return m_event; }

	protected:
		Event() : m_eventTimeAbs( 0.f ), m_eventTimeLocal( 0.f ), m_event( nullptr ) {}
		Event( const CStorySceneEvent* e ) : m_eventTimeAbs( 0.f ), m_eventTimeLocal( 0.f ), m_event( e ) {}

		void MergeEvt( const Event& rhs )
		{
			m_event = rhs.m_event;
			m_eventTimeAbs = rhs.m_eventTimeAbs;
			m_eventTimeLocal = rhs.m_eventTimeLocal;
		}
	};

	struct ActorEvent : public Event
	{
		CName		m_actorId;

	protected:
		ActorEvent() : Event(), m_actorId( CName::NONE ) {}
		ActorEvent( const CStorySceneEvent* e, const CName& actorId ) : Event( e ), m_actorId( actorId ) {}

		void MergeActorEvt( const ActorEvent& rhs )
		{
			SCENE_ASSERT( m_actorId == rhs.m_actorId );

			Event::MergeEvt( rhs );
		}
	};

	struct Fade : public Event
	{
		Bool m_isIn;
		Color m_color;
		Float m_time;
		Float m_duration;

		Fade() {}
		Fade( const CStorySceneEvent* e, Bool isIn = true, Float time = 0.0f, Float duration = 0.0f, const Color& color = Color::BLACK )
			: Event( e )
			, m_isIn( isIn )
			, m_time( time )
			, m_duration( duration )
			, m_color( color )
		{}
	};

	//////////////////////////////////////////////////////////////////////////

	struct BodyAnimation : public ActorEvent
	{
		Float				m_weight;
		EAnimationType		m_type;
		EAdditiveType		m_additiveType;
		SAnimationState		m_animationState;
		Bool				m_useMotion;
		Bool				m_useFakeMotion;
		Bool				m_gatherSyncTokens;
		Bool				m_muteSoundEvents;
		Bool				m_convertToAdditive;
		TDynArray< Int32 >	m_bonesIdx;
		TDynArray< Float >	m_bonesWeight;
		CGUID				m_ID;
		Bool				m_useLowerBodyPartsForLookAt;
		Bool				m_allowPoseCorrection;

		BodyAnimation() {}
		BodyAnimation( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_weight( 1.f ), m_type( EAT_Normal )
			, m_additiveType( AT_Local ), m_convertToAdditive( false ), m_useMotion( false ), m_useFakeMotion( false )
			, m_ID( CGUID::ZERO ), m_gatherSyncTokens( false ), m_muteSoundEvents( false )
			, m_useLowerBodyPartsForLookAt( true ), m_allowPoseCorrection( true ) {}
	};

	struct MimicsAnimation : public ActorEvent
	{
		Float				m_weight;
		Bool				m_fullEyesWeight;
		SAnimationState		m_animationState;
		CGUID				m_ID;
		Bool				m_allowPoseCorrection;

		MimicsAnimation() {}
		MimicsAnimation( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_weight( 1.f ), m_fullEyesWeight( true ), m_ID( CGUID::ZERO ), m_allowPoseCorrection( true ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct MimicPose : public ActorEvent
	{
		Bool						m_reset;
		Float						m_weight;
		Uint32						m_poseId;
		Bool						m_enable;
		CGUID						m_correctionID;
		Bool						m_linkToDialogset;
		const CStorySceneDialogsetInstance* m_linkToDialogsetPtr;
		Bool						m_linkToChangePose;
		CName						m_linkToChangePoseState[4];

		TDynArray< Int32 >			m_trackIndices;
		TDynArray< Float >			m_trackValues;

		MimicPose() {}
		MimicPose( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_reset( false ), m_weight( 1.f ), m_enable( false ), m_poseId( 0 ), m_correctionID( CGUID::ZERO ), m_linkToDialogset( false ), m_linkToDialogsetPtr( nullptr ), m_linkToChangePose( false ) {}
	};

	struct BodyPose : public ActorEvent
	{
		Bool						m_reset;
		Float						m_weight;
		Uint32						m_poseId;
		Bool						m_enable;
		CGUID						m_correctionID;
		Bool						m_linkToDialogset;
		const CStorySceneDialogsetInstance* m_linkToDialogsetPtr;
		Bool						m_linkToChangePose;
		CName						m_linkToChangePoseState[4];

		TDynArray< Int32 >				m_boneIndices;
		TEngineQsTransformArray			m_boneTransforms;

		BodyPose() {}
		BodyPose( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_reset( false ), m_weight( 1.f ), m_poseId( 0 ), m_enable( false ), m_correctionID( CGUID::ZERO ), m_linkToDialogset( false ), m_linkToDialogsetPtr( nullptr ), m_linkToChangePose( false ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct ActorPlacement : public ActorEvent
	{
		EngineTransform		m_placementSS;			// Scene Space
		EngineTransform		m_sceneTransformWS;		// World Space
		Float				m_weight;

		ActorPlacement() {}
		ActorPlacement( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_weight( 1.f ) {}
	};

	struct ActorMotion : public ActorEvent
	{
		const CSkeletalAnimationSetEntry*	m_animation;
		Float								m_eventTimeAbsStart;
		Float								m_eventTimeAbsEnd;
		Float								m_clipFront;
		//Float								m_animEnd;
		Float								m_stretch;
		Float								m_blendIn;
		Float								m_blendOut;
		Float								m_weight;
		Bool								m_supportsClipFront;

		ActorMotion() {}
		ActorMotion( const CStorySceneEvent* e, const CName& actorId ) 
			: ActorEvent( e, actorId )
			, m_animation( nullptr )
			, m_eventTimeAbsStart( 0.f )
			, m_eventTimeAbsEnd( 1.f )
			, m_stretch( 1.f )
			, m_clipFront( 0.f )
			, m_weight( 1.f )
			, m_supportsClipFront( false ) 
		{}
	};

	//////////////////////////////////////////////////////////////////////////

	struct CameraShot : public Event
	{
		StorySceneCameraDefinition			m_definition;
		Bool								m_enableCameraNoise;

		CameraShot() : Event() {}
		CameraShot( const CStorySceneEvent* e ) : Event( e ), m_enableCameraNoise( false ) {}
	};

	struct CameraBlend : public Event
	{
		StorySceneCameraDefinition			m_currentCameraState;

		CameraBlend() {}
		CameraBlend( const CStorySceneEvent* e ) : Event( e ) {}
	};

	struct CameraStartBlendToGameplay : public Event
	{
		Float m_blendTime;
		Float m_lightsBlendTime;

		CameraStartBlendToGameplay() {}
		CameraStartBlendToGameplay( const CStorySceneEvent* e ) : Event( e ), m_blendTime( 1.0f ), m_lightsBlendTime( 1.0f ) {}
	};

	struct CameraAnimation : public Event
	{
		Bool				m_isIdle;
		Float				m_blendWeight;
		Float				m_animationWeight;
		Bool				m_reset;
		SAnimationState		m_animationState;

		CameraAnimation() {}
		CameraAnimation( const CStorySceneEvent* e ) : Event( e ), m_blendWeight( 0.f ), m_animationWeight( 1.f ), m_isIdle( false ), m_reset( false ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct ActorChangeState : public ActorEvent
	{
		SStorySceneActorAnimationState	m_state;
		Bool							m_bodyBlendSet;
		Float							m_bodyBlendWeight;
		Bool							m_mimicsBlendSet;
		Float							m_mimicsBlendWeight;
		Float							m_mimicsPoseWeight;
		CName							m_forceBodyIdleAnimation;
		CName							m_forceMimicsIdleEyesAnimation;
		CName							m_forceMimicsIdlePoseAnimation;
		CName							m_forceMimicsIdleAnimAnimation;
		Bool							m_reset;
		CGUID							m_ID;

		ActorChangeState() {}
		ActorChangeState( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_bodyBlendSet( false ), m_mimicsBlendSet( false ), m_bodyBlendWeight( 0.f ), m_mimicsBlendWeight( 0.f ), m_reset( false ), m_mimicsPoseWeight( 1.f ), m_ID( CGUID::ZERO ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct ActorChangeGameState : public ActorEvent
	{
		Bool	m_fullAutoBlend;
		Bool	m_snapToTerrain;
		Float	m_snapToTerrainDuration;
		Float	m_blendPoseDuration;
		Bool	m_switchToGameplayPose;
		CName	m_gameplayPoseTypeName;
		CName	m_raiseGlobalBehaviorEvent;
		Int32	m_activateBehaviorGraph;
		Int32	m_startGameplayAction;
		Bool	m_forceResetClothAndDangles;

		ActorChangeGameState() {}
		ActorChangeGameState( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_snapToTerrain( false ), m_snapToTerrainDuration( 0.f ), m_switchToGameplayPose( false ), m_fullAutoBlend( false ), m_blendPoseDuration( 0.f ), m_forceResetClothAndDangles( true ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct ActorVisibility : public ActorEvent
	{
		Bool m_showHide;

		ActorVisibility() {}
		ActorVisibility( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_showHide( true ) {}
	};

	struct ActorApplyAppearance : public ActorEvent
	{
		CName m_appearance;

		ActorApplyAppearance() {}
		ActorApplyAppearance( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ) {}
	};

	struct ActorMimicLod : public ActorEvent
	{
		Bool m_setMimicOn;

		ActorMimicLod() {}
		ActorMimicLod( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_setMimicOn( true ) {}
	};

	struct ActorLodOverride : public ActorEvent
	{
		TagList m_actorsByTag;									// Target actors, specified using tags - all actors with any of those tags will become targets.
																// Both scene and non-scene actors can be targeted this way. Remember that m_actorId is also used
																// to specify target actor (but it specifies target actor by voicetag and only scene actors can be
																// targeted this way).

		Bool m_forceHighestLod;									// Controls whether highest LOD is forced or not.
		Bool m_disableAutoHide;									// Controls whether autohide is disabled or not.

		ActorLodOverride() {}
		ActorLodOverride( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_forceHighestLod( false ), m_disableAutoHide( false ) {}
	};

	struct ActorUseHiresShadows : public ActorEvent
	{
		Bool m_useHiresShadows;

		ActorUseHiresShadows() {}
		ActorUseHiresShadows( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_useHiresShadows( true ) {}
	};

	struct ActorItem : public ActorEvent
	{
		CName	m_leftItem;
		CName	m_rightItem;

		Bool	m_instant;
		Bool	m_useMountInstead;
		Bool	m_useUnmountInstead;

		ActorItem() : m_useMountInstead( false ), m_useUnmountInstead( false ) {}
		ActorItem( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_instant( true ), m_useMountInstead( false ), m_useUnmountInstead( false ) {}
	};

	struct ActorItemVisibility : public ActorEvent
	{		
		SItemUniqueId	m_item;
		Bool			m_showHide;

		Bool			m_reset;

		ActorItemVisibility() {}
		ActorItemVisibility( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_showHide( true ), m_reset( false ) {}
	};

	struct ActorMorph : public ActorEvent
	{
		CName m_morphComponentId;
		Float m_weight;

		ActorMorph() {}
		ActorMorph( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_weight( 0.f ) {}
	};

	struct ActorDisablePhysicsCloth : public ActorEvent
	{
		Float m_weight;
		Float m_blendTime;

		ActorDisablePhysicsCloth() {}
		ActorDisablePhysicsCloth( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_weight( 0.f ), m_blendTime( 1.f ) {}
	};

	struct ActorDisableDangle : public ActorEvent
	{
		Float m_weight;

		ActorDisableDangle() {}
		ActorDisableDangle( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_weight( 0.f ) {}
	};

	struct ActorDanglesShake : public ActorEvent
	{
		Float m_factor;

		ActorDanglesShake() {}
		ActorDanglesShake( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_factor( 0.f ) {}
	};

	struct ActorResetClothAndDangles : public ActorEvent
	{
		Bool	m_forceRelaxedState;
		Bool	m_dangle;
		Bool	m_cloth;

		ActorResetClothAndDangles() {}
		ActorResetClothAndDangles( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_forceRelaxedState( false ), m_dangle( true ), m_cloth( true ) {}

		void Merge( const ActorResetClothAndDangles& rhs )
		{
			ActorEvent::MergeActorEvt( rhs );

			m_forceRelaxedState |= rhs.m_forceRelaxedState;
			m_dangle |= rhs.m_dangle;
			m_cloth |= rhs.m_cloth;
		}
	};


	struct AttachPropToBone : public ActorEvent
	{
		Bool	m_isAttached;
		Bool	m_snapAtStart;

		CName   m_targetEntity;
		CName	m_targetSlot;

		EngineTransform	m_offset;

		AttachPropToBone() {}
		AttachPropToBone( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_isAttached( false ), m_snapAtStart( false ) {}
	};

	struct PlayEffect : public ActorEvent
	{
		CName	m_effectName;
		Bool	m_startStop;
		Bool	m_persistAcrossSections;

		PlayEffect() : m_startStop( true ), m_persistAcrossSections( false ) {}
		PlayEffect( const CStorySceneEvent* e, const CName& actorId, Bool startStop, Bool persistAcrossSections = false ) : ActorEvent( e, actorId ), m_startStop( startStop ), m_persistAcrossSections( persistAcrossSections ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct PlayDialogLine : public ActorEvent
	{
		const CStorySceneLine*			m_line;
		IStorySceneDisplayInterface*	m_display;

		PlayDialogLine() {}
		PlayDialogLine( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_line( nullptr ), m_display( nullptr ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct ActorLookAtTick : public ActorEvent
	{
		Bool						m_enable;
		Float						m_disableSpeed;

		EDialogLookAtType			m_type;

		SLookAtDialogBoneInfo		m_infoA;		
		SLookAtDialogStaticInfo		m_infoB;
		SLookAtDialogDynamicInfo	m_infoC;

		Vector						m_staticPoint;
		CName						m_targetId;

		Bool						m_isSetByNewLookat;
		EngineTransform				m_sceneTransformWS;

		ActorLookAtTick() {}
		ActorLookAtTick( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_enable( true ), m_disableSpeed( 0.f ), m_type( DLT_StaticPoint ), m_isSetByNewLookat( false ) {}
	};

	struct ActorLookAt : public ActorEvent
	{
		Float		m_duration;
		Float		m_curveValue;
		Bool		m_useTwoTargets;
		Bool		m_reset;

		CName		m_bodyTarget;
		Bool		m_bodyEnabled;
		Bool		m_bodyInstant;
		Float		m_bodyWeight;
		Bool		m_bodyUseWeightCurve;
		Float		m_bodyTransitionWeight;
		Vector		m_bodyStaticPointWS;

		CName		m_eyesTarget;
		Bool		m_eyesEnabled;
		Bool		m_eyesInstant;
		Float		m_eyesWeight;
		Vector		m_eyesStaticPointWS;
		Float		m_eyesLookAtConvergenceWeight;
		Bool		m_eyesLookAtIsAdditive;
		Float		m_eyesTransitionFactor;

		EDialogLookAtType	m_type;
		ELookAtLevel		m_level;

		Float				m_sceneRange;
		Float				m_gameplayRange;
		Bool				m_limitDeact;

		EngineTransform		m_sceneTransformWS;

		Float		m_oldLookAtEyesSpeed;
		Float		m_blinkHorAngleDeg;

		CGUID		m_id;

		Bool		m_useDeformationMS;

		ActorLookAt() {}
		ActorLookAt( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_reset( false ), m_bodyEnabled( false ), m_bodyInstant( true ), m_eyesEnabled( false ), m_eyesInstant( true ), m_oldLookAtEyesSpeed( 0.f ), m_blinkHorAngleDeg( 0.f ), m_useDeformationMS( true ) {}
	};

	struct ActorGameplayLookAt : public ActorEvent
	{
		Float				m_duration;
		Float				m_curveValue;
		Bool				m_reset;
		CName				m_target;
		Bool				m_enabled;
		Bool				m_instant;
		Float				m_weight;
		EDialogLookAtType	m_type;
		Vector				m_staticPointSS;
		EngineTransform		m_sceneTransformWS;
		CName				m_behaviorVarWeight;
		CName				m_behaviorVarTarget;

		ActorGameplayLookAt() {}
		ActorGameplayLookAt( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_reset( false ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct PropPlacement : public ActorEvent
	{
		EngineTransform			m_placementWS;			// World Space

		PropPlacement() {}
		PropPlacement( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ) {}
	};

	struct PropVisibility : public ActorEvent
	{
		Bool m_showHide;

		PropVisibility() {}
		PropVisibility( const CStorySceneEvent* e, const CName& propId ) : ActorEvent( e, propId ), m_showHide( true ) {}
	};

	//////////////////////////////////////////////////////////////////////////

	struct LightProperty : public ActorEvent
	{
		Bool					m_reset;
		Bool					m_enabled;
		Bool					m_additiveChanges;

		struct // lights
		{
			Color				m_color;
			Float				m_radius;
			Float				m_brightness;
			Float				m_attenuation;
			Float				m_innerAngle;
			Float				m_outerAngle;
			Float				m_softness;
			SLightFlickering	m_flickering;
		};
		struct //dimmers
		{
			Float				m_ambientLevel;
			Float				m_marginFactor;
		};

		EngineTransform			m_placementSS;
		EngineTransform			m_sceneTransformWS;

		SStorySceneAttachmentInfo		m_attachment;
		SStorySceneLightTrackingInfo    m_lightTracker;

		LightProperty() {}
		LightProperty( const CStorySceneEvent* e, const CName& actorId ) : ActorEvent( e, actorId ), m_reset( false ), m_enabled( true ), m_additiveChanges( true ), m_color( 255, 128, 0 ), m_radius( 5.0f ), m_attenuation( 0.5 ), m_brightness( 1.0f ), m_innerAngle( 30.0f ), m_outerAngle( 45.0f ), m_softness( 2.0f ) {}
	};

	struct CameraLightProp : public Event
	{
		SCameraLightsModifiersSetup cameraSetup;
	};

	struct TimeMultiplier : public Event
	{
		Bool	m_enable;
		Float	m_multiplier;
	};

	struct VideoOverlay : public Event
	{
		VideoOverlay( const String& fileName = String(), Uint8 videoParamFlags  = eVideoParamFlag_None, EVideoBuffer videoBuffer = eVideoBuffer_Default ) : m_params( fileName, videoParamFlags, videoBuffer )
		{}

		SVideoParams	m_params;
	};

	//////////////////////////////////////////////////////////////////////////

	struct DisplayDebugComment : public Event
	{
		IStorySceneDisplayInterface*	m_display;
		CGUID							m_commentId;
		String							m_comment;

		DisplayDebugComment() {}
		DisplayDebugComment( const CStorySceneEvent* e ) : Event( e ), m_display( nullptr ) {}
	};

	struct HideDebugComment : public Event
	{
		IStorySceneDisplayInterface*	m_display;
		CGUID							m_commentId;

		HideDebugComment() {}
		HideDebugComment( const CStorySceneEvent* e ) : Event( e ), m_display( nullptr ) {}
	};

	struct SyncItemInfo : public Event
	{
		SyncItemInfo() : m_activate( false ){} 
		SyncItemInfo( const CStorySceneEvent* e ) : Event( e ), m_activate( false ) {}

		Bool							m_activate;
		CName							m_itemName;
		CName							m_actorTag;
	};

	//////////////////////////////////////////////////////////////////////////

	#define ActorPrepareToTalk_PRE_TIMEOFFSET 3.0f

	struct ActorPrepareToTalk : public ActorEvent
	{
		ActorPrepareToTalk() {}
		ActorPrepareToTalk( const CName& actorId ) : ActorEvent( nullptr, actorId ) {}
	};

	struct ActorFinishTalk : public ActorEvent
	{
		ActorFinishTalk() {}
		ActorFinishTalk( const CName& actorId ) : ActorEvent( nullptr, actorId ) {}
	};
	
	struct EnvChange : public Event
	{
		const CEnvironmentDefinition* m_environmentDefinition;
		Bool m_activate;
		Int32 m_priority;
		Float m_blendFactor;
		Float m_blendInTime;

		EnvChange() {}
	};

	struct DoorChangeState : public Event
	{
		DoorChangeState() : Event(), m_openClose( false ), m_instant( true ), m_resetAll( false )
		{}
		CName m_doorTag;
		Bool  m_instant;
		Bool  m_openClose;
		Bool  m_resetAll;
		Bool  m_flipDirection;
	};


	// How far ahead to prefetch a camera event.
	// TODO : Could maybe add some sort of lock, so that we can maintain texture distances until the event happens?
	#define CameraPrefetch_PRE_TIMEOFFSET 2.0f
	struct CameraPrefetch : public Event
	{
		CameraPrefetch() : Event() {}

		Matrix m_camMatrixSceneLocal;
		Float m_camFov;
	};
}
