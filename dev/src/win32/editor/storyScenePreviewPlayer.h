
#pragma once

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storySceneVoicetagMapping.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneSectionPlayingPlan.h"
#include "../../common/engine/cutsceneInstance.h"
#include "dialogEditorFlowCtrl.h"

enum SSPreviewPlayerCompGrid
{
	SSPPCG_None,
	SSPPCG_33,
	SSPPCG_FIB_LT,
	SSPPCG_FIB_RT,
	SSPPCG_FIB_LB,
	SSPPCG_FIB_RB,
	SSPPCG_FIB_RATIO,
	SSPPCG_DYN_SYMMETRY,
};

class CStoryScenePreviewPlayer	: public CStoryScenePlayer
								, public IStorySceneDisplayInterface
{
	DECLARE_ENGINE_CLASS( CStoryScenePreviewPlayer, CStoryScenePlayer, 0 );

	struct SDialogLine
	{
		ISceneActorInterface*	m_actor;
		String					m_text;
		String					m_textWrapped;
		EStorySceneLineType		m_type;
		Bool					m_isTextWrapped;
	};

	struct SDebugComment
	{
		CGUID m_id;
		String m_comment;
	};

private:
	CCamera*					m_camera;
								
	SSPreviewPlayerCompGrid		m_drawCompGrid;
	Bool						m_autoGoToNextSection;
	Bool						m_playSpeechSound;

private:
	Uint32						m_textMaxWidth;
	TDynArray< SDialogLine >	m_lines;
	TDynArray< SSceneChoice >	m_choices;
	Int32						m_selectedChoice;
	TDynArray< SDebugComment>	m_debugComments;

public:
	// HACK
	CEdSceneFlowCtrl*			m_hackFlowCtrl;

public:
	CStoryScenePreviewPlayer();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

public:	// IStorySceneDisplayInterface
	virtual void ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI) override;
	virtual void HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType ) override;
	virtual void HideAllDialogTexts() override;
	virtual void ShowPreviousDialogText( const String& text ) override {}
	virtual void HidePreviousDialogText() override {}
	const String& GetLastDialogText() override;

	virtual void SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI ) override;
	virtual void ShowChoiceTimer( Float timePercent ) override {}
	virtual void HideChoiceTimer() override {}

	virtual void ShowDebugComment( CGUID commentId, const String& comment ) override;
	virtual void HideDebugComment( CGUID commentId ) override;
	virtual void HideAllDebugComments() override;

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;

public:
	void SetAutoGoToNextSection( Bool flag );
	Bool GetAutoGoToNextSection() const;

	void PlaySpeechSound( Bool flag );

	void SetPreviewGrid( SSPreviewPlayerCompGrid grid );
	SSPreviewPlayerCompGrid GetPreviewGrid() const;

	CStorySceneSectionPlayingPlan* HACK_GetOrCreateSectionPlayingPlan( const CStorySceneSection* requestedSection, const TDynArray< const CStorySceneLinkElement* >& flow );
	void HACK_CollectAllEvents( CStorySceneSectionPlayingPlan* plan, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filter );

	virtual void EndPlayingSectionPlan( Int32 id ) override {};

public:
	virtual CCamera* GetSceneCamera() const;

protected:
	virtual Bool IsFadeInProgress() const override				{ return false; }

	virtual void TogglePlayerMovement( Bool flag ) override		{} // GCommonGame->GetSystem< CStorySceneSystem >()->TogglePlayerMovement( false );
	virtual void SetSceneCameraMovable( Bool flag )	override	{} //GCommonGame->GetSystem< CStorySceneSystem >()->SetSceneCameraMovable( m_currentSection->CanMoveCamera() );

	virtual void OnNonGameplaySceneStarted() override			{} //GGame->NonGameplaySceneStarted();
	virtual void OnNonGameplaySceneEnded(const CStorySceneSection *newSection) override				{} // GGame->NonGameplaySceneEnded();

	virtual void ToggleHud( Bool flag ) override				{} // GCommonGame->GetSystem< CStorySceneSystem >()->ToggleDialogHUD( isBlocking );
	virtual Bool IsHudReady() const override					{ return true; } // GCommonGame->GetSystem< CStorySceneSystem >()->IsDialogHUDAvailable()

	virtual void CreateNoSaveLock( const String& lockReason ) override {} // { SGameSessionManager::GetInstance().CreateNoSaveLock( lockReason, m_saveBlockId ); }
	virtual void ReleaseNoSaveLock() override					{} // { SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveBlockId ); }

	virtual Bool WantsToGoToNextSection() override;
	virtual Bool CanFinishScene() override;
	virtual Bool IsCutsceneAsyncTick() const override { return false; }
	virtual Bool CanUseCutscene() const override;

	virtual Bool CanUseDefaultScenePlacement() const override { return false; }

	virtual void OnInit( const ScenePlayerPlayingContext& context ) override;

	virtual Bool ShouldPlaySpeechSound() const override;
	virtual Bool ShouldPlaySounds() const override;
	virtual Bool ShouldPlayEffects() const override;

	virtual void OnPreResetAllSceneEntitiesState( Bool forceDialogset, const CStorySceneSection* currSection, const CStorySceneDialogsetInstance* currDialogset ) override;

	virtual Bool EvaluateFlowCondition( const CStorySceneFlowCondition* condition ) const override;

	virtual void OnRequestSectionPlayingPlan_StartFlow( const CStorySceneSection* requestedSection, CName& dialogSet ) override;
	virtual Bool HasValidDialogsetFor( const CStorySceneSection* s, const CStorySceneDialogsetInstance* d ) const override;

public:
	virtual void SceneFadeIn( const String& reason, Float duration = 0.2f, const Color& color = Color::BLACK ) override	{}
	virtual void SceneFadeOut( const String& reason, Float duration = 0.2f, const Color& color = Color::BLACK ) override	{}
	virtual void SetSceneBlackscreen( Bool flag, const String& reason ) override {} //GGame->SetBlackscreen( true, TXT( "Section not loaded" ) );

	virtual void PlayVideo( const SVideoParams& params ) override	 {} //{ GCommonGame->GetVideoPlayer()->PlayVideo( params ) ); }
	virtual void StopVideo() override								{} //{ GCommonGame->GetVideoPlayer()->StopVideo(); }
	virtual Bool IsPlayingVideo() const override					{ return false; } // { return GCommonGame->GetVideoPlayer()->IsPlayingVideo(); }
	virtual Bool HasValidVideo() const override						{ return false; } //{ return GCommonGame->GetVideoPlayer()->HasValidVideo(); }
	virtual Bool GetVideoSubtitles( String& outSubtitles ) override { return false; }

	virtual void SoundUpdateAmbientPriorities() override						{} //{ GSoundSystem->UpdateAmbients(); }
	virtual void SoundForceReverb( const String& reverbDefinition ) override	{} //{ GSoundSystem->ForceReverb( reverbDefinition ); }

	virtual void OnActorSpawned( const EntitySpawnInfo& info, const CEntity* entity ) override;
	virtual void OnPropSpawned( const EntitySpawnInfo& info, const CEntity* entity ) override;

	virtual const CStorySceneDialogsetInstance* GetDialogsetForEvent( const CStorySceneEvent* e ) const override;

public:
	Bool CalculateCamera( IViewport* view, CRenderCamera &camera ) const;
	void DeactivateCustomEnv();

	TDynArray< CName > GetActorIds( Int32 actorTypes ) const;

	CStorySceneSection* GetCurrentSection() { return const_cast< CStorySceneSection* >( m_internalState.m_currentSection ); }
	const CStorySceneDialogsetInstance* GetCurrentDialogsetInstance() const { return m_sceneDirector.GetCurrentSettings(); }
	const CStorySceneDialogsetInstance* GetCurrentDialogsetInstanceIfValid() const { return m_sceneDirector.GetCurrentSettingsIfValid(); }

	Bool IsChangingSection() const		{ return m_sectionLoader.m_preloadState != SSPS_None || m_internalState.m_pendingNextSection; }

	EngineTransform	GetCurrentScenePlacement() const;

	CName GetPrevSpeakerName( const CStorySceneElement* currElement ) const;
	const CStorySceneLine* GetPrevLine( const CStorySceneElement* currElement ) const;

	Bool ReloadCamera( const StorySceneCameraDefinition* cameraDefinition );

	void GetActorAnimationNames( const CName& actor, TDynArray< CName >& out ) const;
	Bool GetActorCurrIdleAnimationNameAndTime( CName actorId, CName& animName, Float& animTime ) const;
	Bool GetActorCurrAnimationTime( CName actorId, CName animName, Float& animTime ) const;

	const IStorySceneElementInstanceData* FindElementInstance( const CStorySceneElement* element ) const;
	Bool LocalVoMatchApprovedVoInCurrentSectionVariant() const;

	const CActor* AsSceneActor( const CEntity* e ) const;
	const CEntity* AsSceneProp( const CEntity* e ) const;
	const CEntity* AsSceneLight( const CEntity* e ) const;

	// TODO remove this function
	template< class T >
	Bool FindEventsByTime( Float time, TDynArray< const T* >& out, Float eps = 0.015f ) const
	{
		SCENE_ASSERT( m_internalState.m_planId != -1 );

		if ( m_internalState.m_planId != -1 )
		{
			const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
			if ( plan )
			{
				const TDynArray< const CStorySceneEvent* >& events = plan->m_sectionInstanceData.m_evts;
				for ( Uint32 i=0; i<events.Size(); ++i )
				{
					const CStorySceneEvent* e = events[ i ];
					if ( e->GetClass()->IsA< T >() )
					{
						Float sT = 0.f;
						Float eT = 0.f;
						Float dT = 0.f;

						e->GetEventInstanceData( *plan->m_sectionInstanceData.m_data, sT, eT, dT );

						if ( time + eps >= sT && time - eps <= eT )
						{
							out.PushBack( static_cast< const T* >( e ) );
						}
					}
				}
			}
		}

		return out.Size() > 0;
	}

	Bool FindEventsByTime( const CClass* c, Float time, TDynArray< CStorySceneEvent* >& out, Float eps = 0.015f );
	Bool FindEventsByType( const CClass* c, TDynArray< CStorySceneEvent* >& out );

	void SetCameraAdjustedDebugFrame( Bool adjusted );

	virtual Bool IsSceneInGame() const override { return false;}

private:
	void DrawCompGrid( CRenderFrame* frame );
};

BEGIN_CLASS_RTTI( CStoryScenePreviewPlayer );
	PARENT_CLASS( CStoryScenePlayer );
	PROPERTY_NOSERIALIZE( m_camera );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCutscenePreviewDialogWorldInterface : public ICutsceneWorldInterface
{
	CStoryScenePreviewPlayer*	m_player;

public:
	CCutscenePreviewDialogWorldInterface( CStoryScenePreviewPlayer* player ) 
		: m_player( player )
	{

	}

public:
	virtual CWorld* GetWorld() const
	{
		return m_player->GetSceneWorld();
	}

	virtual void ActivateGameCamera( Float blendTime ) 
	{
		
	}

	virtual void SetBlackscreen( Bool enable, const String& reason, const Color & color = Color::BLACK ) 
	{ 
		m_player->SetSceneBlackscreen( enable, reason );
	}

	virtual void Fade( Bool fadeIn, const String& reason, Float fadeDuration = 1.f, const Color & color = Color::BLACK ) 
	{ 
		if ( fadeIn )
		{
			m_player->SceneFadeIn( reason );
		}
		else
		{
			m_player->SceneFadeOut( reason );
		}
	}

	virtual void OnCutsceneEvent( const String& csName, const CName& csEvent ) 
	{ 
		
	}

	virtual void RegisterCutscene( CCutsceneInstance* cs )
	{
		
	}

	virtual void UnregisterCutscene( CCutsceneInstance* cs )
	{
		
	}
};
