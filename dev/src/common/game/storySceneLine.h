#pragma once

#include "storySceneAbstractLine.h"

class StorySceneLineInstanceData;
class IStorySceneDisplayInterface;

/// Instance data for started line
class StorySceneLineInstanceData : public IStorySceneElementInstanceData
{
protected:
	const CStorySceneLine*			m_line;				//!< Spoken line
			
	THandle< CActor >				m_actorHandle;		//!< Actor that is speaking the line
	Bool							m_isOneLiner;		//!< Is this one liner
	Bool							m_isVisible;		//!< Is the line visible
	Uint32							m_lineStringId;

	Float							m_timeOffsetFront;	// Amount of leading silence in seconds.
	Float							m_timeOffsetBack;	// Amount of trailing silence in seconds.

	IStorySceneDisplayInterface*	m_display;

	Bool							m_speechRunning;
	TActorSpeechID					m_speechId;

public:
	//! Get the line
	RED_INLINE const CStorySceneLine* GetLine() const { return m_line; }

	RED_INLINE void GetTimeOffsets( Float& front, Float& back ) const { front = m_timeOffsetFront; back = m_timeOffsetBack; }

	void SetLeadingSilence( Float leadingSilence );
	void SetTrailingSilence( Float trailingSilence );

public:
	//! Create line
	StorySceneLineInstanceData( const CStorySceneLine* line, CStoryScenePlayer* player, THandle< CActor >& actorHandle );

	//! Hide line
	virtual ~StorySceneLineInstanceData();

	//! Is this element blocking ( section will wait before jumping to another until this element ends )
	virtual Bool IsBlocking() const { return true; }

	//! Checks if element is ready for immediate playing
	virtual Bool IsReady() const;

	virtual String GetName() const;

protected:
	//! Perform only actions related to playing an element. Preparation should be done earlier
	virtual void OnPlay();

	virtual Bool OnTick( Float timeDelta ) override;

	//! Perform actions relating stopping an element. This should not include data cleanup (in case section is looped sections)
	virtual void OnStop();

	virtual void OnPaused( Bool flag );
};

/*
Dialog line.

Dialog line may be set as a background line. Such line will be skipped during playing.
So what's it good for? It may be played by "play dialog line" event. By default, line
is not a background line.
*/
class CStorySceneLine : public CAbstractStorySceneLine
{
	DECLARE_ENGINE_CLASS( CStorySceneLine, CAbstractStorySceneLine, 0 )

protected:
	LocalizedString			m_dialogLine;		//!< Displayed subtitle
	String					m_voiceFileName;	//!< Voice file name, without extension
	Bool					m_noBreak;			//!< User cannot skip this line
	StringAnsi				m_soundEventName;	//!< Sound event fired for this line
	Bool					m_isBackgroundLine;	//!< True - line is marked as a background line.
	Bool					m_disableOcclusion;
	Bool					m_alternativeUI;    //!< True - use alternative UI to display this line

public:
	//! If true used cannot skip this line
	RED_INLINE Bool IsNoBreak() const { return m_noBreak; }

	//! Get the file name (without extension) of the voice file associated with this line
	const String GetVoiceFileName() const { return m_voiceFileName; } //return SLocalizationManager::GetInstance().GetVoiceoverFilename( m_dialogLine.GetIndex() );
	const String& GetVoiceFileNameRef() const { return m_voiceFileName; } //return SLocalizationManager::GetInstance().GetVoiceoverFilename( m_dialogLine.GetIndex() );

	//! Get the content ( text ) of this abstract dialog line, uses localization system
	virtual String GetContent() const { return m_dialogLine.GetString(); }

	//! Set the new content of this abstract dialog line, content is set for the current language
	virtual void SetContent( String newValue );

	//! Get localization object of content ( text ) of this dialog line
	virtual LocalizedString* GetLocalizedContent()				{ return &m_dialogLine; }
	virtual const LocalizedString* GetLocalizedContent() const	{ return &m_dialogLine; }

	RED_INLINE const StringAnsi&	GetSoundEventName() const { return m_soundEventName; }
	RED_INLINE Bool GetDisableOclusionFlag() const { return m_disableOcclusion; }

	RED_INLINE void SetAsBackgroundLine( Bool state ) { m_isBackgroundLine = state; }
	RED_INLINE Bool IsBackgroundLine() const { return m_isBackgroundLine; }
	
	RED_INLINE Bool IsAlternativeUI() const { return m_alternativeUI; }

public:
	CStorySceneLine();

	//! Property was changed in editor
	virtual void OnPropertyPostChange( IProperty* property );

	//! Object loaded from file
	virtual void OnPostLoad();

	//! Voicetag of this line has changed
	virtual void OnVoicetagChanged();

public:
	//! Calculate element duration
	virtual Float CalculateDuration( const String& locale ) const override;

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	//! Generate voice over file name
	Bool GenerateVoiceFileName();
	void RefreshVoiceFileName();

	static Bool GetVoicetagId( const CName& voicetag, String& voiceTagId );

	static Float CalcFakeDuration( const String& text );

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override; /*const*/
	virtual void GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const;

	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

protected:
	virtual Bool MakeCopyUniqueImpl();
};

BEGIN_CLASS_RTTI( CStorySceneLine )
	PARENT_CLASS( CAbstractStorySceneLine )
	PROPERTY_CUSTOM_EDIT( m_dialogLine, TXT( "Scene line content" ), TXT( "LocalizedStringEditor" ) );
	PROPERTY_RO_NOT_COOKED( m_voiceFileName, TXT( "Voice file name for this line" ) );
	PROPERTY_EDIT( m_noBreak, TXT("If true player cannot skip this line") );
	PROPERTY_CUSTOM_EDIT( m_soundEventName, TXT( "Sound event name (empty - default)" ), TXT( "AudioEventBrowser" ) );
	PROPERTY_EDIT( m_disableOcclusion, TXT( "Disable occlusion" ) );
	PROPERTY_RO( m_isBackgroundLine, TXT("Indicates whether line is a background line") );
	PROPERTY_EDIT( m_alternativeUI, TXT("True - use alternative UI to display this line") );
END_CLASS_RTTI()
