#pragma once

#include "storySceneElement.h"

/// The abstract line that can be spoken by an actor
class CAbstractStorySceneLine : public CStorySceneElement
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CAbstractStorySceneLine, CStorySceneElement );

protected:
	CName					m_voicetag;			//!< Name of the speaker ( voice tag )
	LocalizedString			m_comment;			//!< User comment
	CName					m_speakingTo;

public:
	//! Get the name of the speaker ( voice tag ) that should spoke this dialog line
	RED_INLINE CName GetVoiceTag() const { return m_voicetag; }

	//! Set the name of the speaker that should spoke this dialog line
	void SetVoiceTag( CName newValue );

	//! Get the user comment, comment is localized
	RED_INLINE String GetComment() const { return m_comment.GetString(); }

	//! Set the user comment, comment is set for the current language
	RED_INLINE void SetComment( const String& newValue ) { m_comment.SetString( newValue ); }

	RED_INLINE LocalizedString* GetLocalizedComment() /*const */{ return &m_comment; }
	RED_INLINE const LocalizedString* GetLocalizedComment() const { return &m_comment; }

	CName	GetSpeakingTo() const { return  m_speakingTo; }
	void	SetSpeakingTo( CName to ){ m_speakingTo = to; }
public:
	CAbstractStorySceneLine();

	//! Get the content ( text ) of this abstract dialog line, uses localization system
	virtual String GetContent() const = 0;

	//! Set the new content of this abstract dialog line, content is set for the current language
	virtual void SetContent( String newValue ) = 0;

	//! Get localization object of content ( text ) of this dialog line
	virtual LocalizedString* GetLocalizedContent() /*const*/ { return NULL; }

	virtual Bool IsPlayable() const { return true; }

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override /*const*/
	{ 
		localizedStrings.PushBack( LocalizedStringEntry( &m_comment, TXT( "Line comment" ), NULL ) );
	}

	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

protected:
	virtual void OnVoicetagChanged() {}

	virtual Bool MakeCopyUniqueImpl();
};

BEGIN_ABSTRACT_CLASS_RTTI( CAbstractStorySceneLine );
	PARENT_CLASS( CStorySceneElement );
	PROPERTY_RO( m_voicetag, TXT( "Speaker voicetag" ) );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_comment, TXT( "Scene line comment" ), TXT( "LocalizedStringEditor" ) );
	PROPERTY_EDIT( m_speakingTo, TXT("Voicetag of actor this actor is speaking to") )
END_CLASS_RTTI();
