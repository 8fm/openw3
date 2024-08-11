#pragma once
#include "storySceneElement.h"
#include "../engine/cutscene.h"
#include "../engine/cutsceneInstance.h"

class CExtAnimItemEvent;

// DIALOG_TOMSIN_TODO - tu jest jakis mega krap!!!

/// Instance data for cutscene header

class StorySceneCutscenePlayerInstanceData_Fake : public IStorySceneElementInstanceData
{
public:
	StorySceneCutscenePlayerInstanceData_Fake( const CStorySceneElement* element, CStoryScenePlayer* player )
		: IStorySceneElementInstanceData( element, player )
	{}

	virtual String GetName() const { return String( TXT("Cutscene_Fake") ); }

protected:
	virtual Bool OnTick( Float timeDelta ) override { return true; }
};

class StorySceneCutscenePlayerInstanceData  : public IStorySceneElementInstanceData
											, public ICutsceneProvider
{
private:
	Bool					m_showDesc;
	Float					m_timer;

	Bool					m_asyncTick;

	String					m_desc;
	CCutsceneInstance*		m_cutscene;

	TDynArray< TPair< CName, THandle< CEntity > > >	m_regActors;

	Bool					m_isGameplay;
	Bool					m_clearActorsHands;
	Bool					m_isLooped;

	const CStorySceneCutscenePlayer*	m_csPlayer;

	TDynArray< String >			m_preloadedSoundEvents;
	TDynArray< TPair< CName, SItemUniqueId > > m_spawnedItems;

	// TEMPORARY:
	CCutsceneTemplate*	m_csTemplate;

public:
	StorySceneCutscenePlayerInstanceData( const CStorySceneCutscenePlayer* csPlayer, CStoryScenePlayer* player, CCutsceneTemplate* csTemplate, Bool looped, Bool clearActorsHands );
	~StorySceneCutscenePlayerInstanceData();

	static const Float		DEFAULT_DURATION;

	CCutsceneInstance* GetCutsceneInstance() const { return m_cutscene; }

	virtual Bool IsBlocking() const { return true; }
	virtual void MarkSkipped();

	virtual String GetName() const { return String( TXT("Cutscene") ); }

	//! Checks if element is ready for immediate playing
	virtual Bool IsReady() const;

	//! Does element require confirmation before skipping
	virtual Bool ShouldConfirmSkip() const;

	//! Gets entities used by this element instance - they are known after instance initialization
	virtual void GetUsedEntities( TDynArray< CEntity* >& usedEntities ) const;

public: //ICutsceneProvider
	virtual CEntity* GetCutsceneActor( const CName& voiceTag );
	virtual CEntity* GetCutsceneActorExtContext( const String& name, CName voiceTag );
	virtual SCutsceneActorDef* GetActorDefinition( const String& actorName ) const;
	virtual Bool IsActorOptional( CName id ) const override;;

	// DIALOG_TOMSIN_TODO
public:
	const CCameraComponent* GetCamera() const { return m_cutscene ? m_cutscene->GetCamera() : NULL; }

protected:
#ifndef RED_FINAL_BUILD
	void ShowDesc( const String& text );
	void HideDesc();
#endif

	void RegisterActors();
	void UnregisterActors();

	void RegisterCsActor( CEntity* ent, Bool spawned );
	void UnregisterCsActor( CName id, CEntity* actor );

	Bool IsCsPositionValid( CCutsceneTemplate* csTemplate, const Matrix& offset, String& errorMsg, const CWorld* world ) const;

	void PreprocessItems();
	void RestoreItemsState();

	void PrepareSoundsForCutscene();
	void RestoreSoundsAfterCutscene();

	//! Perform element initialization options. Return true if element is ready to play
	virtual Bool OnInit( const String& locale ) override;

	//! Perform only actions related to playing an element. Preparation should be done earlier
	virtual void OnPlay();

	virtual Bool OnTick( Float timeDelta ) override;

	virtual Float OnSeek( Float newTime ) override;

	virtual Bool AllowSeek() const override { return true; }

	//! Perform actions relating stopping an element. This should not include data cleanup (in case section is looped sections)
	virtual void OnStop();

	virtual void OnPaused( Bool flag );
};

class CStorySceneCutscenePlayer : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStorySceneCutscenePlayer, CStorySceneElement, 0 )

private:
	String						m_descriptionText;
	CCutsceneTemplate*			m_cutscene;
	Bool						m_displayDesc;

public:
	CStorySceneCutscenePlayer();

	RED_INLINE void SetDescriptionText( String newValue ) { m_descriptionText = newValue; }
	RED_INLINE String GetDescriptionText() const			{ return m_descriptionText; }

	RED_INLINE void DisplayDesc( Bool flag ) { m_displayDesc = flag; }
	RED_INLINE void SetCutsceneTemplate( CCutsceneTemplate* cutscene ) { m_cutscene = cutscene; }

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	virtual Bool CanBeSkipped() const;
	virtual Bool CanBeDeleted() const { return false; }
	virtual Bool IsPlayable() const;

	virtual Float CalculateDuration( const String& locale ) const override;

	RED_INLINE CCutsceneTemplate* GetCutscene() const
	{ return m_cutscene; }

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override
	{ 

	}

	Bool GetCsPoint( Matrix& point, const CWorld* world ) const;
	static Bool GetCsPoint( const CStorySceneCutsceneSection* section, Matrix& point, const CWorld* world );
};

BEGIN_CLASS_RTTI( CStorySceneCutscenePlayer )
	PARENT_CLASS( CStorySceneElement )
	PROPERTY_EDIT_NOT_COOKED( m_descriptionText, TXT( "Description" ) );
END_CLASS_RTTI()
