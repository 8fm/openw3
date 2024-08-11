#pragma once

#include "storySceneElement.h"
#include "storySceneChoiceLineAction.h"

/// Instance data for comment
class StorySceneCommentInstanceData : public IStorySceneElementInstanceData
{
public:
	//! Create line
	StorySceneCommentInstanceData( const CStorySceneComment* comment, CStoryScenePlayer* player );

	virtual String GetName() const { return String( TXT("Comment") ); }

protected:
	virtual Bool OnTick( Float timeDelta ) override;
};

class CStorySceneComment : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStorySceneComment, CStorySceneElement, 0 )

protected:
	LocalizedString m_commentText;

public:
	CStorySceneComment(void);

	inline String GetCommentText() { return m_commentText.GetString(); }
	inline void SetCommentText( String newValue ) { m_commentText.SetString( newValue ); }

	RED_INLINE LocalizedString* GetLocalizedComment() /*const */{ return &m_commentText; }
	RED_INLINE const LocalizedString* GetLocalizedComment() const { return &m_commentText; }

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override /*const*/
	{ 
		localizedStrings.PushBack( LocalizedStringEntry( &m_commentText, m_elementID.StringBefore( TXT( "_" ) ), NULL ) );
	}

	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	virtual Bool MakeCopyUniqueImpl();
};

BEGIN_CLASS_RTTI( CStorySceneComment )
	PARENT_CLASS( CStorySceneElement )
	PROPERTY_CUSTOM_EDIT( m_commentText, TXT( "Scene comment text" ), TXT( "LocalizedStringEditor" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

struct SSceneInjectedChoiceLineInfo
{
	LocalizedString		m_choiceLine;
	Bool				m_emphasisLine;
	Bool				m_returnToChoice;
	String				m_choiceLineActionText;
	EDialogActionIcon	m_dialogActionIcon;
	Bool				m_isDisabled;

	String GetChoiceActionText() const
	{
		return m_choiceLineActionText;
	}
	EDialogActionIcon GetChoiceActionIcon() const
	{
		return m_dialogActionIcon;
	}
	Bool IsDisabled() const
	{
		return m_isDisabled;
	}

};

class CStorySceneQuestChoiceLine : public CStorySceneComment
{
	DECLARE_ENGINE_CLASS( CStorySceneQuestChoiceLine, CStorySceneComment, 0 )

protected:
	Bool		m_emphasisLine;			//!< This choice line should be emphasized
	Bool		m_returnToChoice;		//!< Return to choice HUB once finished with current dialog
	IStorySceneChoiceLineAction*				m_action;


public:
	CStorySceneQuestChoiceLine() : m_emphasisLine( true ), m_returnToChoice( false ), m_action( nullptr ) {}
	void FillInjectedLineInfo( SSceneInjectedChoiceLineInfo& info ) const
	{
		info.m_choiceLine = m_commentText;
		info.m_emphasisLine = m_emphasisLine;
		info.m_returnToChoice = m_returnToChoice;	
		info.m_choiceLineActionText = m_action ? m_action->GetActionText() : String::EMPTY;
		info.m_dialogActionIcon = m_action ? m_action->GetActionIcon() : DialogAction_NONE;
		info.m_isDisabled = m_action && !m_action->CanUseAction();
	}
};

BEGIN_CLASS_RTTI( CStorySceneQuestChoiceLine )
	PARENT_CLASS( CStorySceneComment )	
	PROPERTY_EDIT( m_emphasisLine, TXT( "This choice line should be emphasized" ) );
	PROPERTY_EDIT( m_returnToChoice, TXT( "Return to choice HUB once finished with current dialog" ) );
	PROPERTY_INLINED( m_action, TXT( "Choice action" ) );
END_CLASS_RTTI()
