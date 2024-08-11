#pragma once

#include "storySceneSection.h"
#include "..\\..\\common\\engine\\cutscene.h"

class CStorySceneLinkElement;
class CStorySceneControlPart;
class CCutsceneTemplate;
class CStorySceneElement;
class CStorySceneSection;
struct StorySceneCameraSetting;

struct SCutsceneActorLine
{
	String		m_actorVoicetag;
	String		m_text;
	String		m_sound;
	StringAnsi	m_soundEventName;
	Uint32		m_lineIndex;
};

struct SCutsceneActorOverrideMapping
{
	DECLARE_RTTI_STRUCT( SCutsceneActorOverrideMapping )

	String	m_actorName;
	SCutsceneActorDef	m_cutsceneActorDef;

	Bool HasRelevantData() const;
};

BEGIN_CLASS_RTTI( SCutsceneActorOverrideMapping )
	PROPERTY_EDIT( m_actorName, TXT( "Cutscene actor name to override" ) );
	PROPERTY_INLINED( m_cutsceneActorDef, TXT( "Cutscene actor override definition" ) );
END_CLASS_RTTI()

class CStorySceneCutsceneSection : public CStorySceneSection
{
	DECLARE_ENGINE_CLASS( CStorySceneCutsceneSection, CStorySceneSection, 0 )

private:
	THandle< CCutsceneTemplate >	m_cutscene;
	TagList							m_point;
	Bool							m_looped;
	Bool							m_clearActorsHands;

	TDynArray< SCutsceneActorOverrideMapping >	m_actorOverrides;

public:
	String GetDescriptionText() const;

	Bool GetClearActorsHands() const { return m_clearActorsHands; }

public:
	CStorySceneCutsceneSection();

	void OnPropertyPreChange( IProperty* property );
	void OnPropertyPostChange( IProperty* property );

	virtual CClass* GetBlockClass() const;

	virtual void OnPostLoad();

	virtual void OnSectionCreate();

	const TagList& GetTagPoint() const { return m_point; }

	virtual Bool AutoPlayingNextElement() const { return false; }
	virtual Bool UseTrajectories() const { return false; }

	virtual void GetVoiceTags( TDynArray< CName >& voiceTags, Bool append=false ) const;
	virtual Bool HasActor( CName actorId ) const;

	virtual Bool IsValid() const;

	void GetCutsceneVoicetags( TDynArray< CName >& voiceTags, Bool append=false ) const { TBaseClass::GetVoiceTags( voiceTags, append ); }

	virtual Bool CanAddComment( Uint32 index, Bool after ) const;
	virtual Bool CanAddQuestChoice( Uint32 index, Bool after ) const		{ return false; }
	virtual Bool CanAddDialogLine( Uint32 index, Bool after ) const;
	virtual Bool CanAddRandomLine( Uint32 index, Bool after ) const		{ return false; }
	virtual Bool CanAddScriptLine( Uint32 index, Bool after ) const		{ return false; }

	//virtual Bool CanRemoveElement( Uint32 index ) const;
	virtual Bool CanRemoveElement( CStorySceneElement* element ) const;

	virtual void AddSceneElement( CStorySceneElement* element, Uint32 index );

	CCutsceneTemplate* GetCsTemplate() const { return m_cutscene.Get(); }
	
	void GetDialogLines( TDynArray< SCutsceneActorLine >& lines ) const;

	virtual Bool IsLooped() const { return m_looped; }
	virtual Bool CanChangeSectionImmediately() const { return true; }
	virtual Bool CanPlayDuringBlockingScenes() const { return IsGameplay() == true; }
	virtual Bool UsesSetting() const { return false; }
	virtual Bool HasFadeOut() const;
	virtual Bool HasFadeIn() const;
	virtual Bool CanSwitchSpeakingActor() const { return false; }

	virtual Bool ShouldFadeOutOnEnd() const { return false; }
	virtual Bool ShouldTickDuringFading() const	{ return true; }
	virtual Bool ShouldForceDialogset() const override	{ return false; }

	Bool CanModifyCamera() const;
	Bool GetCameraSettings( StorySceneCameraSetting& cameraSettings ) const;

	virtual void GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const override;

	// Setting and placement methods
public:
	const CStorySceneCutscenePlayer* GetPlayer() const;

protected:
	CStorySceneCutscenePlayer* GetPlayer();
	void CreatePlayer();

	Bool GetCsOffset( Matrix& offset ) const;
	Bool GetSceneOffset( Matrix& offset ) const;

	void CleanJunkCutscenePlayers();
	void UpdatePlayer();
	void CheckContent();

public:
	SCutsceneActorDef* FindCutsceneActorOverrideDefinition( const String& actorName );

	void CreateDialogEvents();
	void DestroyDialogEvents();
};

BEGIN_CLASS_RTTI( CStorySceneCutsceneSection );
	PARENT_CLASS( CStorySceneSection );
	PROPERTY_EDIT( m_cutscene, TXT("Cutscene resource") );
	PROPERTY_EDIT( m_point, TXT("Cutscene root point") );
	PROPERTY_EDIT( m_looped, TXT("Is cutscene looped") );
	PROPERTY_EDIT( m_actorOverrides, TXT( "Cutscene actors overrides" ) );
	PROPERTY_EDIT( m_clearActorsHands, TXT( "Clear hands of all actors at the beginning" ) );
END_CLASS_RTTI();
