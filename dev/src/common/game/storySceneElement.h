#pragma once

#include "storySceneIncludes.h"
#include "../../common/engine/localizableObject.h"

class CStoryScenePlayer;
class CStorySceneElement;

// DIALOG_TOMSIN_TODO - to skoro mamy to to po co rtti jest uzywane?
/// Type of scene element, used mostly in editor
enum EStorySceneElementType
{
	SSET_Comment,				//!< A comment
	SSET_Line,					//!< A single spoken line
	SSET_Choice,				//!< A choice of multiple options
	SSET_Linkline,				//!< A link to another scene
	SSET_ScriptLine,			//!< An element containing a script to run
	SSET_Cutscene,				//!< An cutscene
	SSET_Video,					//!> A video
	SSET_QuestChoiceLine,		//!< A quest choice line
	SSET_Pause,					//!< A pause
	SSET_BlockingElement,		//!< A blocking element
};

/// Instance data for started line
class IStorySceneElementInstanceData : public IStorySceneObject
{
protected:
	CStoryScenePlayer*			m_player;					//!< Player owning this element 
	const CStorySceneElement*	m_element;					//!< Get the story scene element represented in this instance

	Float						m_currentTime;				//!< Current time. Range: <0.0f, m_duration> (for looped elements it's wrapped).
	Float						m_startTime;				//!< Start time, in locale specified during initialization.
	Float						m_duration;					//!< Element duration in  locale specified during initialization.
															//!< For looped elements this is duration of one loop.

	Bool						m_lineSkipped;
	Bool						m_running;

public:
	//! Get the player
	CStoryScenePlayer* GetPlayer() const { return m_player; }

	//! Get the story scene element
	const CStorySceneElement* GetElement() const { return m_element; }

	Float GetCurrentTime() const { return m_currentTime; }

	Float GetStartTime() const;

	//! Returns element duration (note it's locale specific).
	Float GetDuration() const { ASSERT( m_duration > 0.f ); return m_duration; }
	
	void SetDuration( Float duration )
	{
		m_duration = duration;
	}

	virtual String GetName() const = 0;

public:
	//! Create line
	IStorySceneElementInstanceData( const CStorySceneElement* element, CStoryScenePlayer* player );

	//! Hide line
	virtual ~IStorySceneElementInstanceData();

	//! Is this element blocking ( section will wait before jumping to another untill this element ends )
	virtual Bool IsBlocking() const { return false; }

	//! Checks if element is ready for immediate playing
	virtual Bool IsReady() const { return true; }

	//! Mark line as skipped
	virtual void MarkSkipped() { m_lineSkipped = true; }

	//! Is line skipped?
	Bool IsSkipped() const { return m_lineSkipped; }

	//! Can element be skipped?
	virtual Bool CanBeSkipped() const;

	//! Does element require confirmation before skipping
	virtual Bool ShouldConfirmSkip() const;

	virtual Bool AllowSeek() const { return false; }

	//! Gets entities used by this element instance - they are known after instance initialization
	virtual void GetUsedEntities( TDynArray< CEntity* >& usedEntities ) const {}

public:
	Bool Init( const String& locale, Float startTime );

	void Deinit()
	{
		OnDeinit();
	}

	//! Perform only actions related to playing an element. Preparation should be done earlier
	void Play() 
	{ 
		ASSERT( !m_running );

		m_running = true; 
		m_currentTime = 0.f; 

		OnPlay(); 
	}

	void Seek( Float newTime )
	{
		const Float time = OnSeek( newTime );
		SCENE_ASSERT( time >= 0.f && time <= GetDuration() );
		m_currentTime = time;
	}

	/*
	Updates element.

	\return True - element is not yet finished. False - element is finished.
	*/
	Bool Tick( Float timeDelta )
	{
		if( IsLooped() )
		{
			// Update current time and wrap it when necessary.
			m_currentTime = fmod( m_currentTime + timeDelta, m_duration );
		}
		else
		{
			// Update current time and clamp it when necessary.
			m_currentTime = Clamp( m_currentTime + timeDelta, 0.0f, m_duration );
		}

		return OnTick( timeDelta );
	}

	//! Perform actions relating stopping an element. This should not include data cleanup (in case section is looped sections)
	void Stop() 
	{ 
		ASSERT( m_running ); 

		m_running = false; 
		m_lineSkipped = false; 

		OnStop(); 
	}

	void Pause( Bool flag )
	{
		OnPaused( flag );
	}

	void ForceStop()
	{
		m_running = false; 
		m_lineSkipped = false; 

		OnStop();
	}

	Bool IsRunning() const
	{
		return m_running;
	}

	virtual Bool IsLooped() const { return false; }

protected:
	//! Perform element initialization options. Return true if element is ready to play
	virtual Bool OnInit( const String& locale ) { return true; }

	virtual void OnDeinit() {}

	//! Perform only actions related to playing an element. Preparation should be done earlier
	virtual void OnPlay() {}

	virtual Bool OnTick( Float timeDelta ) { return true; }

	virtual Float OnSeek( Float newTime ) { return newTime; }

	//! Perform actions relating stopping an element. This should not include data cleanup (in case section is looped sections)
	virtual void OnStop() {}

	virtual void OnPaused( Bool flag ) {}
};

/// A basic element of the scene
class CStorySceneElement : public CObject, public ILocalizableObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CStorySceneElement, CObject );

protected:
	String		m_elementID;					//!< ID of this dialog element, auto generated
	Float		m_approvedDuration;				// TODO: This is to be removed. We keep this for now as it is used when converting old style sections to variant sections.
												// Approved durations of scene elements are now stored in section variants.
	Bool		m_isCopy;

protected:
	TInstanceVar< Float > i_test;

public:
	//! Get the ID of this dialog element
	const String& GetElementID() const { return m_elementID; }

public:
	CStorySceneElement();

	//! Loaded
	virtual void OnPostLoad();

	//! Generate friendly name
	virtual String GetFriendlyName() const;

	Float GetApprovedDuration() const;		// TODO: This is to be removed. We keep this for now as it is used when converting old style sections to variant sections.

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CStorySceneInstanceBuffer& data ) const;
	virtual void OnReleaseInstance( CStorySceneInstanceBuffer& data ) const;

public:
	//! Get the section we are in
	CStorySceneSection* GetSection() const;

	//! Generate unique ID of this element
	void GenerateElementID();

	void ClearElementID() { m_elementID.Clear(); }

public:
	virtual Float CalculateDuration( const String& locale ) const;
	Float CalculateDuration() const;

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	// DIALOG_TOMSIN_TODO - OnGetSchedulableElements dubluje sie funkcjonalnoscia z IsPlayable albo powinno tak byc. Do wywalenia jedno z nich.
	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	virtual Bool IsPlayable() const { return false; }

	virtual Bool CanBeDeleted() const { return true; }

	virtual Bool CanBeSkipped() const;

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override {}
	virtual void GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const {}

public:
	virtual Bool IsSceneElementCopy() const { return m_isCopy; }
	virtual void SetSceneElementCopy( Bool isCopy = true ) { m_isCopy = isCopy; }
	
	Bool MakeCopyUnique();

protected:
	virtual Bool MakeCopyUniqueImpl() { return false; }
};

BEGIN_CLASS_RTTI( CStorySceneElement )
	PARENT_CLASS( CObject )
	PROPERTY_RO( m_elementID, TXT( "Element identifier" ) )
	PROPERTY( m_approvedDuration )
	PROPERTY( m_isCopy );
END_CLASS_RTTI()

// =================================================================================================
// implementation
// =================================================================================================

RED_INLINE Float IStorySceneElementInstanceData::GetStartTime() const
{
	return m_startTime;
}

RED_INLINE Float CStorySceneElement::GetApprovedDuration() const
{
	return m_approvedDuration;
}
