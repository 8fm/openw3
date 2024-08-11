#pragma once

#include "../../common/engine/localizableObject.h"
#include "storySceneControlPart.h"
#include "storySceneCameraSetting.h"
#include "storySceneEvent.h"
#include "storySceneSectionVariant.h"

RED_DECLARE_NAME( GAMEPLAY )
RED_DECLARE_NAME( IMPORTANT )

class CStorySceneElement;
class CStorySceneComment;
class CStorySceneChoice;
class CStorySceneChoiceLine;
class CAbstractStorySceneLine;
class CStorySceneLine;
class CStorySceneScriptLine;
class IStorySceneSectionOverrideCondition;
class CStorySceneQuestChoiceLine;
class CStorySceneCutsceneSection;
class CStorySceneEvent;

struct SStorySceneElementInfo
{
	CStorySceneElement*	m_element;
	CStorySceneSection*	m_section;

	SStorySceneElementInfo( CStorySceneElement* element, CStorySceneSection* section )
		: m_element( element ), m_section( section ) {}
};

/*
Information about story scene event info in context of a section.
*/
class CStorySceneEventInfo
{
public:
	CStorySceneEventInfo()
	: m_eventGuid( CGUID::ZERO )
	, m_eventPtr( nullptr )
	, m_sectionVariantId( -1 )
	{}

	CStorySceneEventInfo( CGUID evGuid, CStorySceneEvent* evPtr, CStorySceneSectionVariantId variantId )
	: m_eventGuid( evGuid )
	, m_eventPtr( evPtr )
	, m_sectionVariantId( variantId )
	{}

	CGUID m_eventGuid;						// GUID of event that this CStorySceneEventInfo describes.
	CStorySceneEvent* m_eventPtr;			// Event pointer (we don't own it).
	CStorySceneSectionVariantId m_sectionVariantId;	// Id of a section variant to which event belongs.

	DECLARE_RTTI_STRUCT( CStorySceneEventInfo )
};

BEGIN_CLASS_RTTI( CStorySceneEventInfo )
	PROPERTY( m_eventGuid )
	PROPERTY( m_sectionVariantId )
END_CLASS_RTTI();

class CStorySceneLocaleVariantMapping
{
public:
	Uint32 m_localeId;
	CStorySceneSectionVariantId m_variantId;

	DECLARE_RTTI_STRUCT( CStorySceneLocaleVariantMapping )
};

BEGIN_CLASS_RTTI( CStorySceneLocaleVariantMapping )
	PROPERTY( m_localeId )
	PROPERTY( m_variantId )
END_CLASS_RTTI();

class CStorySceneSection : public CStorySceneControlPart, public ILocalizableObject
{
	DECLARE_ENGINE_CLASS( CStorySceneSection, CStorySceneControlPart, 0 )

private:
	String									m_sectionName;			//!< Name of the section
	TagList									m_tags;					//!< DEPRECATED Tags used by section
	
	CStorySceneChoice*						m_choice;				//!< Section choice. It doesn't exist in m_sceneElements list but it's
																	//!< also a scene element and it's always treated as a last scene element
																	//!< of a section. May be nullptr - this means section has no choice.

	Uint32									m_sectionId;

	Bool	m_isGameplay;
	Bool	m_isImportant;

	Bool	m_allowCameraMovement;
	Bool	m_hasCinematicOneliners;
	Bool	m_fadeInAtBeginning; // deprecated
	Bool	m_fadeOutAtEnd; // deprecated
	Bool	m_manualFadeIn;

	Float	m_interceptRadius;
	Float	m_interceptTimeout;
	
	Bool	m_pauseInCombat;

	Bool	m_canBeSkipped;
	Bool	m_canHaveLookats;

	Uint32	m_numberOfInputPaths;

	CName	m_dialogsetChangeTo;
	Bool	m_forceDialogset;

	Bool	m_streamingLock;						//!< Lock streaming of the duration of this block (nothing new will stream in, nothing will unstream), there will also be no loading screens
	CName	m_streamingAreaTag;						//!< Before starting this section make sure that EVERYTHING in the given streaming area is loaded, this can be a tag to either waypoint (or any other existing shit) or specialized stremaing area(s).
	Bool	m_streamingUseCameraPosition;			//!< Use camera position for streaming (not player position), ON by default
	Float	m_streamingCameraAllowedJumpDistance;	//!< Prefetch distance for hard check (if camera moves more than that we will have the loading screen)

	TDynArray< CStorySceneLinkElement* >	m_inputPathsElements;

	TDynArray< CStorySceneSection* >		m_interceptSections;

	Int32 m_contexID;																	// TODO: This is to be removed. We keep this for now as it is used when converting old style sections
																						// to variant sections. After section is converted to variant section, m_contexID is set to -1.

	CStorySceneSectionVariantId m_nextVariantId;										// Id that will be assigned to next variant.
	CStorySceneSectionVariantId m_defaultVariantId;										// Id of a section variant that is to be used by default.

	#ifndef NO_EDITOR
		CStorySceneSectionVariantId m_variantIdChosenInEditor;							// Id of a variant displayed in editor or -1 if not set.
		CStorySceneSectionVariantId m_variantIdForcedInEditor;							// Id of a variant forced in editor or -1 if not set.
	#endif // !NO_EDITOR

	TDynArray< CStorySceneSectionVariant* > m_variants;									// Section variants, in no special order.
	THashMap< CStorySceneSectionVariantId, CStorySceneSectionVariant* > m_idToVariant;	// Maps variant id to variant.

	TDynArray< CStorySceneEvent* > m_events;											// List of events contained by section, in no special order. Contains events from all variants.
	TDynArray< CStorySceneEventInfo* > m_eventsInfo;									// Info about each event. Note that m_eventsInfo[ i ] corresponds to m_events[ i ].
	THashMap< CGUID, CStorySceneEventInfo* > m_guidToEventInfo;							// Maps event GUID to event info.

	TDynArray< CStorySceneLocaleVariantMapping* > m_localeVariantMappings;				// Specifies which section variant to use for which locale. If there's no entry for given locale then default variant is used.
	THashMap< Uint32, CStorySceneLocaleVariantMapping* > m_localeToVariant;	

	TDynArray< CName > m_soundEventsOnEnd; //Sound events to trigger when the scene section ends
	TDynArray< CName > m_soundEventsOnSkip; //Sound events to trigger when the scene section is skipped

	Bool	m_blockMusicTriggers;	//Specifies whether this scene section should block music triggers
	String  m_soundListenerOverride; //name of the listener override target (of there is one)

	float	m_maxBoxExtentsToApplyHiResShadows;
	Float	m_distantLightStartOverride;												// Overrides distance when light fallback appears

protected:
	TDynArray< CStorySceneElement* >		m_sceneElements;		//!< List of elements contained by section, in order of their appearance.

public:
	// SCENE_TOMSIN_TODO - do wywalenia
	//! Get the section's tag list
	RED_INLINE const TagList& GetTags() const			{ return m_tags; }

	// SCENE_TOMSIN_TODO - do wywalenia
	//! Set section tags
	RED_INLINE void SetTags( const TagList& tags )	{ m_tags = tags; }

	// SCENE_TOMSIN_TODO - do wywalenia
	//! Check if section has given tag in it's tag list
	RED_INLINE Bool HasTag( CName tag ) const			{ return m_tags.HasTag( tag ); }

public:
	//! Check if it's a gameplay section
	RED_INLINE Bool IsGameplay() const				{ return m_isGameplay || m_tags.HasTag( CNAME( GAMEPLAY ) ); }
	RED_INLINE void SetIsGameplay(Bool val)			{ m_isGameplay = val; }

	RED_INLINE Bool ManualFadeIn() const { return m_manualFadeIn; };

	//! Check if it's an important section
	RED_INLINE Bool IsImportant() const				{ return m_isImportant || m_tags.HasTag( CNAME( IMPORTANT ) ); }

	RED_INLINE Bool CanMoveCamera() const				{ return m_allowCameraMovement; }
	RED_INLINE Bool HasCinematicOneliners() const		{ return m_hasCinematicOneliners; }

	RED_INLINE Bool HasInterception() const			{ return m_interceptRadius > 0.0f; }
	RED_INLINE Bool HasInterceptionSections() const	{ return !m_interceptSections.Empty(); }
	RED_INLINE Float GetInterceptRadius() const		{ return m_interceptRadius; }
	RED_INLINE Float GetInterceptTimeout() const		{ return m_interceptTimeout; }

	RED_INLINE Bool CanHaveLookats() const			{ return m_canHaveLookats; }
	RED_INLINE const CName& GetDialogsetChange() const { return m_dialogsetChangeTo; }
	virtual Bool ShouldForceDialogset() const			{ return m_forceDialogset && !IsGameplay(); }
	
	RED_INLINE Int32 GetContexID() const				{ return m_contexID; }

	RED_INLINE const Bool GetStreamingLock() const { return m_streamingLock; }
	RED_INLINE const CName GetStreamingAreaTag() const { return m_streamingAreaTag; }
	RED_INLINE const Bool GetStreamingUseCameraPosition() const { return m_streamingUseCameraPosition; }
	RED_INLINE const Float GetStreamingCameraAllowedJumpDistance() const { return m_streamingCameraAllowedJumpDistance; }

	RED_INLINE const Float GetMaxBoxExtentsToApplyHiResShadows() const { return m_maxBoxExtentsToApplyHiResShadows; }
	RED_INLINE const Float GetDistantLightOverride() const { return m_distantLightStartOverride; }

	virtual Bool IsLooped() const						{ return false; }
	virtual Bool CanChangeSectionImmediately() const	{ return false; }
	virtual Bool CanPlayDuringBlockingScenes() const	{ return false; }

	//++ DIALOG_TOMSIN_TODO - REMOVE THIS
	virtual Bool UsesSetting() const					{ return IsGameplay() == false; }
	//--

	virtual Bool HasFadeOut() const						{ return false; }
	virtual Bool HasFadeIn() const						{ return false; }
	virtual Bool IsBlackscreenAfter() const				{ return false; }
	virtual Bool CanSwitchSpeakingActor() const			{ return true; }
	virtual Bool CanElementBeSkipped() const			{ return m_canBeSkipped; }
	virtual Bool CanSectionBeSkipped() const			{ return m_canBeSkipped && m_choice == NULL; }

	virtual Bool ShouldFadeInOnBeginning() const		{ return m_fadeInAtBeginning ; }
	virtual Bool ShouldFadeOutOnEnd() const				{ return m_fadeOutAtEnd ; }

	virtual Bool ShouldPauseInCombat() const			{ return m_pauseInCombat; }
	virtual Bool ShouldTickDuringFading() const			{ return false; }

	virtual CStorySceneLinkElement* GetSectionNextElement() const;

	const TDynArray< CGUID >& GetEvents( CStorySceneSectionVariantId variantId ) const;
	const TDynArray< CStorySceneEvent* >& GetEventsFromAllVariants() const;

	CStorySceneEvent* GetEvent( CGUID guid );
	const CStorySceneEvent* GetEvent( CGUID guid ) const;

	Float GetEventNormalizedStartTime( CGUID evGuid ) const;

	void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const;

	//Returns true if this scene wants to block music triggers
	RED_INLINE const Bool ShouldBlockMusicTriggers() const { return m_blockMusicTriggers; }
	RED_INLINE const String GetListenerOverride() const { return m_soundListenerOverride; }

	RED_INLINE const TDynArray< CName>& GetSoundEventsOnEnd() const { return m_soundEventsOnEnd; }
	RED_INLINE const TDynArray< CName>& GetSoundEventsOnSkip() const { return m_soundEventsOnSkip; }


public:
	// DIALOG_TOMSIN_TODO - editor only
	void AddEvent( CStorySceneEvent* event, CStorySceneSectionVariantId variantId );
	void GetEventsForElement( TDynArray< const CStorySceneEvent* >& events, const CStorySceneElement* element, CStorySceneSectionVariantId variantId ) const;
	Bool RemoveEvent( CGUID evGuid );
	void RemoveAllEvents( CStorySceneSectionVariantId variantId );

	void ApproveElementDuration( CStorySceneSectionVariantId variantId, const String& elementId, Float duration );
	Float GetElementApprovedDuration( CStorySceneSectionVariantId variantId, const String& elementId ) const;

	Uint32 GetNumVariants() const;
	void EnumerateVariants( TDynArray< CStorySceneSectionVariantId >& outVariants ) const;
	CStorySceneSectionVariantId CreateVariant( Uint32 baseLocaleId );
	CStorySceneSectionVariantId CloneVariant( CStorySceneSectionVariantId orgVariantId );
	void DestroyVariant( CStorySceneSectionVariantId variantId );

	CStorySceneSectionVariantId GetEventVariant( CGUID evGuid ) const;

	Uint32 GetVariantBaseLocale( CStorySceneSectionVariantId variantId ) const;
	void SetVariantBaseLocale( CStorySceneSectionVariantId variantId, Uint32 baseLocaleId );

	void SetDefaultVariant( CStorySceneSectionVariantId variantId );
	CStorySceneSectionVariantId GetDefaultVariant() const;

	void SetLocaleVariantMapping( Uint32 localeId, CStorySceneSectionVariantId variantId );
	CStorySceneSectionVariantId GetLocaleVariantMapping( Uint32 localeId ) const;
	CStorySceneSectionVariantId GetVariantUsedByLocale( Uint32 localeId ) const;

	#ifndef NO_EDITOR
		void SetVariantChosenInEditor( CStorySceneSectionVariantId variantId );
		CStorySceneSectionVariantId GetVariantChosenInEditor() const;

		void SetVariantForcedInEditor( CStorySceneSectionVariantId variantId );
		CStorySceneSectionVariantId GetVariantForcedInEditor() const;
	#endif // !NO_EDITOR

public:
	CStorySceneSection();
	~CStorySceneSection();

	//! Section was created
	virtual void OnSectionCreate() {}

	//! For events serialization
	virtual void OnSerialize( IFile& file );

	//! Property was changed in editor
	void OnPropertyPostChange( IProperty* property );

	//! Get the name of the section
	virtual String GetName() const;
	
	//! Get more friendly name of the section
	virtual String GetFriendlyName() const;

	//! Set the name of the section
	void SetName( String name );

	//! Add a new comment to this section
	CStorySceneComment* AddComment( Uint32 index = -1 );

	//! Add new dialog line to this section
	CAbstractStorySceneLine* AddDialogLine( Uint32 index = -1 );

	//! Add new script line to this section
	CStorySceneScriptLine* AddScriptLine( Uint32 index = -1 );

	//! Add new dialog line to this section
	CAbstractStorySceneLine* AddDialogLineAfter( CStorySceneElement* element );

	//! Add new script line to this section
	CStorySceneScriptLine* AddScriptLineAfter( CStorySceneElement* element );

	//! Add new choice
	CStorySceneChoice* AddChoice();

	Bool HasQuestChoiceLine() const;

	//! Sets a quest choice marking this section
	CStorySceneQuestChoiceLine* SetQuestChoice();

	//! Get quest choice line assigned to this section
	const CStorySceneQuestChoiceLine* GetQuestChoiceLine() const;

	virtual Bool CanAddComment( Uint32 index, Bool after ) const		{ return true; }
	virtual Bool CanAddQuestChoice( Uint32 index, Bool after ) const;
	virtual Bool CanAddDialogLine( Uint32 index, Bool after ) const		{ return true; }
	virtual Bool CanAddRandomLine( Uint32 index, Bool after ) const		{ return true; }
	virtual Bool CanAddScriptLine( Uint32 index, Bool after ) const		{ return true; }
	virtual Bool CanAddChoice() const;

	virtual Bool CanRemoveElement( CStorySceneElement* element ) const { return true; }

	void RemoveElement( CStorySceneElement* element );
	void RemoveChoice();

	//! Removes a quest choice line from this section
	void RemoveQuestChoice();

	//! Get number of elements in this section
	virtual Uint32 GetNumberOfElements() const;

	//! Get the n-th element from this section. Returns NULL if index is invalid.
	CStorySceneElement* GetElement( Uint32 index );

	const TDynArray< CStorySceneElement* >& GetElements() const { return m_sceneElements ; }

	//! Get n-th element from this section ( read only ). Returns NULL if index is invalid.
	const CStorySceneElement* GetElement( Uint32 index ) const;

	//! Get choice related to this section
	CStorySceneChoice* GetChoice();

	//! Get choice related to this section
	const CStorySceneChoice* GetChoice() const;

	//! Get all lines from section
	void GetLines( TDynArray< CAbstractStorySceneLine* >& lines ) const;

	//! Get all voice tags used within this section
	virtual void GetVoiceTags( TDynArray< CName >& voiceTags, Bool append=false ) const;

	//! Callback called when some other object is linked to this element
	virtual void OnConnected( CStorySceneLinkElement* linkedToElement );

	//! Callback called when other object is unlinked from this element
	virtual void OnDisconnected( CStorySceneLinkElement* linkedToElement );

	//! Add generic scene element at given place
	virtual void AddSceneElement( CStorySceneElement* element, Uint32 index );	

	//! Can section be played or should it be skipped;
	virtual Bool IsValid() const;

	virtual void OnPostLoad();

	RED_INLINE Uint32 GetSectionId() const { return m_sectionId; }

	Uint64 GetSectionUniqueId() const;

	void GenerateSectionId();

	Bool CanModifyCamera() const;
	Bool GetCameraSettings( StorySceneCameraSetting& cameraSettings ) const;

	virtual void GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const override;

	// Editor
	// SCENE_TOMSIN_TODO - do wywalenia
	void NotifyAboutNameChanged();
	// SCENE_TOMSIN_TODO - do wywalenia
	void NotifyAboutSocketsChange();
	// SCENE_TOMSIN_TODO - do wywalenia
	void NotifyAboutChoiceRemoved();
	// SCENE_TOMSIN_TODO - do wywalenia
	void NotifyAboutElementAdded( CStorySceneElement* element );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler, CStorySceneSectionVariantId variantId );
	virtual void OnInitInstance( CStorySceneInstanceBuffer& data, CStorySceneSectionVariantId variantId ) const;
	virtual void OnReleaseInstance( CStorySceneInstanceBuffer& data, CStorySceneSectionVariantId variantId ) const;

public:
	virtual Bool AutoPlayingNextElement() const { return true; }
	virtual Bool UseTrajectories() const { return true; }

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override /*const*/;
	virtual void GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const;

	virtual CClass* GetBlockClass() const;

public:
	void AddInterceptSection( CStorySceneSection* section ) { m_interceptSections.PushBackUnique( section ); }
	void RemoveInterceptSection( CStorySceneSection* section ) { m_interceptSections.Remove( section ); }
	CStorySceneSection* DrawInterceptSection();

public:
	Uint32 GetNumberOfInputPaths() const { if( !GIsCooker ) { SCENE_ASSERT__FIXME_LATER( m_inputPathsElements.Size() == m_numberOfInputPaths ); } return m_numberOfInputPaths; }
	CStorySceneLinkElement* GetInputPathLinkElement( Uint32 inputPathIndex );
	const TDynArray< CStorySceneLinkElement* >& GetInputPathLinks() const;
	const CStorySceneLinkElement* GetInputPathLinkElement( Uint32 inputPathIndex ) const;

public:
	virtual void SetScenesElementsAsCopy( Bool isCopy );
	virtual void MakeUniqueElementsCopies();

#ifndef NO_EDITOR
public:
	void SetDialogsetChange( const CName& dialosetChange );
	const CStorySceneInput* GetFirstSceneInput() const;

	virtual Bool SupportsInputSelection() const override;
	virtual void ToggleSelectedInputLinkElement() override;

	void FixInvalidBlendEvents( Uint32& outNumFixedBlendEvents, Uint32& outNumRemovedBlendEvents );
	Uint32 FixStrayInterpolationKeys();
	Uint32 FixStrayBlendKeys();
	Uint32 RemoveBadLinksBetweenEvents();

#endif // !NO_EDITOR

public:

	template< typename T >
	void RemoveAllEventsByClass( CStorySceneSectionVariantId variantId )
	{
		const TDynArray< CGUID > evGuids = GetEvents( variantId );
		for ( auto evGuid : evGuids )
		{
			const CStorySceneEvent* ev = GetEvent( evGuid );
			if ( ev && ev->GetClass()->IsA< T >() )
			{
				RemoveEvent( evGuid );
			}
		}
	}
};

BEGIN_CLASS_RTTI( CStorySceneSection );
	PARENT_CLASS( CStorySceneControlPart );
	PROPERTY( m_contexID );
	PROPERTY( m_nextVariantId );
	PROPERTY( m_defaultVariantId );
	#ifndef NO_EDITOR
		PROPERTY_NOT_COOKED( m_variantIdForcedInEditor );
	#endif // !NO_EDITOR
	PROPERTY( m_variants );
	PROPERTY( m_localeVariantMappings );
	PROPERTY( m_sceneElements );
	PROPERTY_NOSERIALIZE( m_events );
	PROPERTY( m_eventsInfo );
	PROPERTY( m_choice );
	PROPERTY_RO( m_sectionId, TXT( "Section Id" ) );
	PROPERTY_EDIT( m_sectionName, TXT( "Section name" ) );
	PROPERTY( m_tags );
	PROPERTY_EDIT( m_interceptRadius, TXT( "Radius in which this section will be played" ) );
	PROPERTY_EDIT( m_interceptTimeout, TXT( "Timeout beetwen playing intercept sections" ) );
	PROPERTY( m_interceptSections );
	PROPERTY_EDIT( m_isGameplay, TXT( "Is gameplay section" ) );
	PROPERTY_EDIT( m_isImportant, TXT( "Is important section" ) );
	PROPERTY_EDIT( m_allowCameraMovement, TXT( "Can user move camera within this section" ) );
	PROPERTY_EDIT( m_hasCinematicOneliners, TXT( "Should oneliners in this section be displayed in cinematic style" ) );
	PROPERTY_EDIT( m_manualFadeIn, TXT("Dont fade in automatically") )
	PROPERTY( m_fadeInAtBeginning )
	PROPERTY( m_fadeOutAtEnd )
	PROPERTY_EDIT( m_pauseInCombat, TXT( "Should this section be paused when combat starts" ) );
	PROPERTY_EDIT( m_canBeSkipped, TXT( "Allow fast forwarding of this section" ) );
	PROPERTY_EDIT( m_canHaveLookats,TXT( "Can actors look at each other" ) );
	PROPERTY_EDIT( m_numberOfInputPaths, TXT( "Number of possible input paths" ) );
	PROPERTY_CUSTOM_EDIT( m_dialogsetChangeTo, TXT( "Setting to change to on section start" ), TXT( "DialogSetting" ) );
	PROPERTY_EDIT( m_forceDialogset, TXT("") );
	PROPERTY( m_inputPathsElements );
	PROPERTY_EDIT( m_streamingLock, TXT("Lock streaming of the duration of this block (nothing new will stream in, nothing will unstream), there will also be no loading screens") );
	PROPERTY_EDIT( m_streamingAreaTag, TXT("Before starting this section make sure that EVERYTHING in the given streaming area is loaded, this can be a tag to either waypoint (or any other existing shit) or specialized stremaing area(s).") );
	PROPERTY_EDIT( m_streamingUseCameraPosition, TXT("Use camera position for streaming (not player position), ON by default") );
	PROPERTY_EDIT( m_streamingCameraAllowedJumpDistance, TXT("Prefetch distance for hard check (if camera moves more than that we will have the loading screen)") );
	PROPERTY_EDIT( m_blockMusicTriggers, TXT( "Should music triggers be blocked during this scene"));
	PROPERTY_EDIT( m_soundListenerOverride, TXT("Specify the listener override if we want one for the scene"));
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnEnd, TXT("Events to be triggered when this scene ends"), TXT( "AudioEventBrowser"));
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnSkip, TXT("Events to be triggered when this scene is skipped"), TXT( "AudioEventBrowser"));
	PROPERTY_EDIT( m_maxBoxExtentsToApplyHiResShadows, TXT("Specify maximum size of bounding box of an entity to apply hi-res shadows on it (if entity is too small on regular shadow-map we render it to hi-res shadow-map"));
	PROPERTY_EDIT( m_distantLightStartOverride, TXT("Overrides distance when light fallback appears") );

END_CLASS_RTTI();

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE Uint32 CStorySceneSection::GetNumVariants() const
{
	return m_variants.Size();
}

RED_INLINE const TDynArray< CGUID >& CStorySceneSection::GetEvents( CStorySceneSectionVariantId variantId ) const
{
	// TODO: check/assert whether section has such variant

	return m_idToVariant[ variantId ]->m_events;
}

RED_INLINE const TDynArray< CStorySceneEvent* >& CStorySceneSection::GetEventsFromAllVariants() const
{
	return m_events;
}
