#pragma once

#include "storySceneChoiceLine.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"

/// Scene instance data for active choice
class CStorySceneChoiceInstanceData : public IStorySceneElementInstanceData
{
private:
	Float							m_remainingTime;			//!< Time left to make the choice - when it reaches 0 then default choice is made.
																//!< Meaningful for time lmited choices. For choices with no limit it's always 0.
	const CStorySceneChoice*		m_choice;
	IStorySceneDisplayInterface*	m_display;

public:
	TDynArray< SSceneChoice >			m_avaiableChoiceLines;

public:
	//! Constructor
	CStorySceneChoiceInstanceData( const CStorySceneChoice* choice, CStoryScenePlayer* player );

	//! Cleanup
	~CStorySceneChoiceInstanceData();

	//! Is this element blocking ( section will wait before jumping to another until this element ends )
	virtual Bool IsBlocking() const { return true; }

	virtual String GetName() const { return String( TXT("Choice") ); }

	virtual Bool IsLooped() const override;

protected:
	//! Perform only actions related to playing an element. Preparation should be done earlier
	virtual void OnPlay();

	virtual Bool OnTick( Float timeDelta ) override;

	//! Perform actions relating stopping an element. This should not include data cleanup (in case section is looped sections)
	virtual void OnStop();
};

/// A scene element allowing to choose between multiple options
class CStorySceneChoice : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStorySceneChoice, CStorySceneElement, 0 );

private:
	TDynArray< CStorySceneChoiceLine* >		m_choiceLines;		//!< Available choice lines

	Float									m_timeLimit;		//!< Time limit to make the choice. Default choice is made when limit is exceeded. 0 - no time limit.
	Float									m_duration;			//!< Choice duration (duration of one loop).
																//!< Choice duration is the same in all locales.
	Bool									m_isLooped;			//!< Indicates whether choice is looped.
	Bool									m_questChoice;
	Bool									m_showLastLine;
	Bool									m_alternativeUI;

public:
	CStorySceneChoice();

	//! Get the number of choice lines in the choice
	Uint32 GetNumberOfChoiceLines() const;

	//! Get n-th choice line from the choice lists
	CStorySceneChoiceLine* GetChoiceLine( Int32 index );

	//! Get n-th choice line from the choice lists ( read only )
	const CStorySceneChoiceLine* GetChoiceLine( Int32 index ) const;

	//! Get index of given choice line within this choice
	Int32 GetChoiceLineIndex( const CStorySceneChoiceLine* choiceLine ) const;

	//! Add new choice line to the choices
	CStorySceneChoiceLine* AddChoiceLine( Int32 index = -1 );

	//! Add existing choice line to the choices
	void AddChoiceLine( CStorySceneChoiceLine* choiceLine, Int32 index = -1 );

	//! Remove choice line from the list of choices	
	void RemoveChoiceLine( Uint32 index );

	//! Move choice line within choices list
	Uint32 MoveChoiceLine( Uint32 index, Bool moveDown );

public:
	virtual Float CalculateDuration( const String& locale ) const override;

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	virtual Bool IsPlayable() const { return true; }

	virtual Bool CanBeSkipped() const { return false; }

	RED_INLINE Float GetTimeLimit() const { return m_timeLimit; }

	RED_INLINE Bool IsLooped() const { return m_isLooped; }

	RED_INLINE Bool ShowLastLine() const { return m_showLastLine; }

	RED_INLINE Bool IsQuestChoice() const { return m_questChoice; }

	RED_INLINE Bool IsUsingAlternativeUI() const { return m_alternativeUI; }

	// Note: choice duration is the same in all locales.
	RED_INLINE Float GetDuration() const { return m_duration; }

	// Note: choice duration is the same in all locales.
	RED_INLINE void SetDuration( Float duration ) { m_duration = duration; }

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override
	{
		for ( Uint32 i = 0; i < m_choiceLines.Size(); ++i )
		{
			m_choiceLines[ i ]->GetLocalizedStrings( localizedStrings );
		}
	}
	virtual void GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const;


public:
	// Editor
	void NotifyAboutChoiceLinksChanged( CStorySceneChoiceLine* sender );
	void NotifyAboutChoiceLineChanged( CStorySceneChoiceLine* sender );
	void NotifyAboutChoiceLineAdded( Int32 index );
	void NotifyAboutChoiceLineRemoved( Uint32 index );	

	virtual Bool MakeCopyUniqueImpl();
	
};

BEGIN_CLASS_RTTI( CStorySceneChoice )
	PARENT_CLASS( CStorySceneElement )
	PROPERTY( m_choiceLines );
	PROPERTY_EDIT( m_timeLimit, TXT( "Time limit to make the choice. 0 - no limit." ) )
	PROPERTY_EDIT( m_duration, TXT( "Choice duration (duration of one loop)" ) )
	PROPERTY_EDIT( m_isLooped, TXT( "Indicates whether choice is looped" ) )
	PROPERTY_EDIT( m_questChoice, TXT( "Should this choice be extended using quest context dialogs?" ) )
	PROPERTY_EDIT( m_showLastLine, TXT( "Show last dialog line" ) )	
	PROPERTY_EDIT( m_alternativeUI, TXT( "Alternative way to display dialog options on HUD" ) )	
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

// Hmmm this is 'almost' a hack
class CStorySceneCutsceneChoiceInstanceData : public CStorySceneChoiceInstanceData
{
	typedef CStorySceneChoiceInstanceData TBaseClass;

private:
	IStorySceneElementInstanceData*		m_csPlayer;

public:
	CStorySceneCutsceneChoiceInstanceData( const CStorySceneChoice* choice, CStoryScenePlayer* player );
	~CStorySceneCutsceneChoiceInstanceData();

	virtual String GetName() const override { return String( TXT("Cutscene Choice") ); }

	const CCameraComponent* GetCamera() const;

protected:
	virtual Bool OnInit( const String& locale ) override;
	virtual void OnDeinit() override;

	virtual void OnPlay() override;
	virtual Bool OnTick( Float timeDelta ) override;
	virtual void OnStop() override;
	
	virtual Float OnSeek( Float newTime ) override;
	virtual void OnPaused( Bool flag ) override;

	virtual Bool IsReady() const override;

	virtual Bool AllowSeek() const override { return true; }

	virtual void GetUsedEntities( TDynArray< CEntity* >& usedEntities ) const override;
};