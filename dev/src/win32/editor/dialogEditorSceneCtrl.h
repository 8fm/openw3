
#pragma once

#include "../../common/game/storySceneIncludes.h"
#include "storyScenePreviewPlayer.h"

class CEdSceneEditor;
class CStoryScene;
struct ScenePlayerInputState;
class IStorySceneElementInstanceData;
class CCutsceneEmptyWorldInterface;

class CEdSceneCtrl
{
	CEdSceneEditor*				m_editor;
	const CStoryScene*			m_scene;

	CStorySceneController*		m_sceneController;
	CStoryScenePreviewPlayer*	m_player;

	CCutsceneEmptyWorldInterface* m_csWorldInterface;

public:
	CEdSceneCtrl();

	void Init( CEdSceneEditor* e, const CStoryScene* s );
	void Destroy();

	Bool IsValid() const;

	void Tick( Float dt );

	Bool ForceState( const ScenePlayerInputState& state );
	Bool SetState( const ScenePlayerInputState& state );

	Float GetSectionTime() const;

	void TogglePause();
	void Pause();
	Bool IsPaused() const;

	void Freeze();

	void RestartSection();
	void PlayOneFrame();

	ScenePlayerInputState Rebuild( const CStorySceneSection* startFromSection );
	void Refresh();

	Bool IsChangingSection() const;

	void SetCameraAdjustedDebugFrame( Bool adjusted );

public:
	Bool CalculateCamera( IViewport* view, CRenderCamera &camera ) const;
	
	EngineTransform	GetCurrentScenePlacement() const;

	const CStorySceneSection* GetCurrentSection() const;
	CStorySceneSection* GetCurrentSection();
	const CStorySceneDialogsetInstance* GetCurrentDialogsetInstance() const;
	const CStorySceneDialogsetInstance* GetCurrentDialogsetInstanceIfValid() const;


	//See EDialogActorType for $types values
	TDynArray< CName > GetActorIds( Int32 types ) const; 

	CActor* GetSceneActor( const CName& actorName );
	CEntity* GetSceneEntity( const CName& actorName );
	const CEntity* GetSceneEntity( const CName& actorName ) const;
	CCamera* GetCamera();

	const CActor* AsSceneActor( const CEntity* e ) const;
	const CEntity* AsSceneProp( const CEntity* e ) const;
	const CEntity* AsSceneLight( const CEntity* e ) const;

	StorySceneCameraState GetCameraState() const;

	Bool GetCurrentLightState( const CName& actor, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const;
	Bool GetCurrentActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	Bool GetPreviousActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	Bool GetCurrentActorAnimationMimicState( CName actor, CName& mimicEmoState, CName& mimicLayerEyes, CName& mimicLayerPose, CName& mimicLayerAnim, Float& poseWeight ) const;
	void GetActorAnimationNames( const CName& actor, TDynArray< CName >& out ) const;
	Bool GetActorCurrIdleAnimationNameAndTime( CName actorId, CName& animName, Float& animTime ) const;
	Bool GetActorCurrAnimationTime( CName actorId, CName animName, Float& animTime ) const;
	void GetVoiceTagsForCurrentSetting( TDynArray< CName >& vt ) const;

	Matrix GetActorPosition( const CName& actor ) const;
	Matrix GetDialogPosition() const;

	CName GetPrevSpeakerName( const CStorySceneElement* currElement ) const;

	const CStorySceneLine* GetPrevLine( const CStorySceneElement* currElement );

	Bool ReloadCamera( const StorySceneCameraDefinition* cameraDefinition );
	void PlaySpeechSound( Bool flag );
	void SetAutoGoToNextSection( Bool flag );

	const THashMap< CName, THandle< CEntity > >& GetActorMap() const;

	void GetEventAbsTime( const CStorySceneEvent* e, Float& time, Float& duration ) const;
	Float GetEventScalingFactor( const CStorySceneEvent& e ) const;
	Float GetEventDuration( const CStorySceneEvent& e ) const;
	void SetEventDuration( const CStorySceneEvent& e, Float duration ) const;
	Float GetEventStartTime( const CStorySceneEvent& e ) const;
	void SetEventStartTime( const CStorySceneEvent& e, Float startTime ) const;

	void SetPreviewGrid( SSPreviewPlayerCompGrid grid );

	template< class T >
	Bool FindEventsByTime( Float time, TDynArray< const T* >& out ) const
	{
		return m_player->FindEventsByTime( time, out );
	}

	void DeactivateCustomEnv()
	{
		return m_player->DeactivateCustomEnv();
	}

	Bool FindEventsByTime( const CClass* c, Float time, TDynArray< CStorySceneEvent* >& out );
	Bool FindEventsByType( const CClass* c, TDynArray< CStorySceneEvent* >& out );

	// If you really need to have it...
	const CStoryScenePreviewPlayer* GetPlayer() const;
	CStoryScenePreviewPlayer* HACK_GetPlayer();

	// If you really need to have it...
	const IStorySceneElementInstanceData* FindElementInstance( const CStorySceneElement* element ) const;
	Bool LocalVoMatchApprovedVoInCurrentSectionVariant() const;
	Bool UseApprovedDurations() const;

private:
	CStoryScenePreviewPlayer* CreatePlayer( const CStorySceneInput* input = nullptr);
};
