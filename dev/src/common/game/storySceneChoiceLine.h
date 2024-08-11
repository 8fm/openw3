#pragma once

#include "../../common/engine/localizableObject.h"
#include "storySceneElement.h"
#include "storySceneLinkElement.h"

class CStorySceneSection;
class CStorySceneControlPart;
class CStorySceneChoice;
class IStorySceneChoiceStructureListener;
class ISceneChoiceMemo;
class IStorySceneChoiceLineAction;
enum EDialogActionIcon;

/// A single choice
class CStorySceneChoiceLine : public CStorySceneLinkElement, public ILocalizableObject
{
	DECLARE_ENGINE_CLASS( CStorySceneChoiceLine, CStorySceneLinkElement, 0 );

private:
	LocalizedString								m_choiceLine;			//!< A displayed text ( localized )
	LocalizedString								m_choiceComment;		//!< Internal comment
	IQuestCondition*							m_questCondition;		//!< Condition that must be true to make this choice visible
	TDynArray< ISceneChoiceMemo* >				m_memo;					//!< Where should the fact that the choice was made be memorized
	Bool										m_singleUseChoice;		//!< This choice line can be selected only once in single dialog
	Bool										m_emphasisLine;			//!< This choice line should be emphasized
	IStorySceneChoiceLineAction*				m_action;


public:
	Bool IsHidden() const;
	Bool IsDisabled() const;
	CName GetPlayGoChunk() const;

	void OnChoiceSelected( CStoryScenePlayer* player ) const;

	String GetChoiceLineId() const;

	Bool WasChoosen() const;

	//! Get the comment of this choice line
	RED_INLINE String GetChoiceComment() const { return m_choiceComment.GetString(); }

	//! Set the comment of this choice line
	RED_INLINE void SetChoiceComment( String text ) /*const*/ { m_choiceComment.SetString( text ); }

	//! Get the text displayed by the choice line
	RED_INLINE String GetChoiceLine() const { return m_choiceLine.GetString(); }

	String GetChoiceActionText() const;
	EDialogActionIcon GetChoiceActionIcon() const;

	RED_INLINE LocalizedString* GetLocalizedChoiceLine() /*const */{ return &m_choiceLine; }
	RED_INLINE const LocalizedString* GetLocalizedChoiceLine() const { return &m_choiceLine; }

	RED_INLINE LocalizedString* GetLocalizedComment() /*const */{ return &m_choiceComment; }
	RED_INLINE const LocalizedString* GetLocalizedComment() const { return &m_choiceComment; }

	RED_INLINE Bool	HasCondition() const { return m_questCondition != NULL; }

	RED_INLINE Bool	HasMemo() const	{ return m_memo.Empty() == false; }

	RED_INLINE Bool	IsSingleUseChoice() const { return m_singleUseChoice; }

	RED_INLINE Bool	HasEmphasis() const { return m_emphasisLine; }

	RED_INLINE const IQuestCondition*					GetCondition() const	{ return m_questCondition; }
	RED_INLINE const TDynArray< ISceneChoiceMemo* >&	GetMemos() const		{ return m_memo; }

public:
	CStorySceneChoiceLine();

	//! Get the parent choice element
	CStorySceneChoice* GetChoice() const;

	//! Change choice value
	void SetChoiceLine( String newValue );

	//! Callback called when some other scene object is linked to this element
	virtual void OnConnected( CStorySceneLinkElement* linkedToElement );

	//! Callback called when other scene object is unlinked from this element
	virtual void OnDisconnected( CStorySceneLinkElement* linkedToElement );

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override /*const*/
	{
		localizedStrings.PushBack( LocalizedStringEntry( &m_choiceComment, TXT( "Choice line comment" ), NULL ) );
		localizedStrings.PushBack( LocalizedStringEntry( &m_choiceLine, TXT( "Choice line" ), NULL ) );
	}

	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );
};

BEGIN_CLASS_RTTI( CStorySceneChoiceLine )
	PARENT_CLASS( CStorySceneLinkElement )
	PROPERTY_CUSTOM_EDIT( m_choiceLine, TXT( "Choice line text" ), TXT( "LocalizedStringEditor" ) );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_choiceComment, TXT( "Choice line comment" ), TXT( "LocalizedStringEditor" ) );
	PROPERTY_INLINED( m_questCondition, TXT( "Conditions that needs to be fulfilled" ) )
	PROPERTY_INLINED( m_memo, TXT( "Where should the fact that the choice was made be memorized?" ) )
	PROPERTY_EDIT( m_singleUseChoice, TXT( "This choice line can be selected only once in single dialog" ) );
	PROPERTY_EDIT( m_emphasisLine, TXT( "This choice line should be emphasized" ) );
	PROPERTY_INLINED( m_action, TXT( "Choice action" ) );
END_CLASS_RTTI()


///////////////////////////////////////////////////////////////////////////////

// An interface allowing to memorize that a particular quest choice was made
class ISceneChoiceMemo : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISceneChoiceMemo, CObject );

public:
	virtual ~ISceneChoiceMemo() {}

	// Stores the fact that the choice was taken in an underlying data storage
	virtual void Persist() const {}

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const { return String::EMPTY; }
#endif
};
BEGIN_ABSTRACT_CLASS_RTTI( ISceneChoiceMemo )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

// An interface allowing to memorize that a particular quest choice was made
class CFactsDBChoiceMemo : public ISceneChoiceMemo
{
	DECLARE_ENGINE_CLASS( CFactsDBChoiceMemo, ISceneChoiceMemo, 0 );

private:
	String		m_factID;
	Int32		m_value;

public:
	CFactsDBChoiceMemo();

	// Stores the fact that the choice was taken in an underlying data storage
	virtual void Persist() const;

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const { return String::Printf( TXT( "FactID: %s, Value: %d" ), m_factID.AsChar(), m_value ); }
#endif

};
BEGIN_CLASS_RTTI( CFactsDBChoiceMemo )
	PARENT_CLASS( ISceneChoiceMemo )
	PROPERTY_EDIT( m_factID, TXT( "ID of the fact to insert." ) )
	PROPERTY_EDIT( m_value, TXT( "Value we want to insert for that fact." ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
