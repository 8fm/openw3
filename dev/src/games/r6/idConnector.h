/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idSystem.h"
#include "idLine.h"

//------------------------------------------------------------------------------------------------------------------
// A wrapper for voicetag - needed to have a custom editor for it
//------------------------------------------------------------------------------------------------------------------
struct SVoiceTagWrapper
{
	DECLARE_RTTI_STRUCT( SVoiceTagWrapper );

	CName m_voiceTag;

	SVoiceTagWrapper() : m_voiceTag( CName::NONE ) {}
	SVoiceTagWrapper( CName voiceTag ) : m_voiceTag( voiceTag ) {}
	RED_INLINE Bool operator==( const SVoiceTagWrapper& other ) const { return m_voiceTag == other.m_voiceTag; }
};

BEGIN_NODEFAULT_CLASS_RTTI( SVoiceTagWrapper )
	PROPERTY_CUSTOM_EDIT( m_voiceTag, TXT( "Default voice of this interlocutor" ), TXT( "EntityVoiceTagSelect" ) )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
// A wrapper for connector line - needed to have a custom editor for it
//------------------------------------------------------------------------------------------------------------------
struct SIDConnectorLine : public SIDBaseLine
{
	DECLARE_RTTI_STRUCT( SIDConnectorLine );

	SIDConnectorLine()
	{
	}

	RED_INLINE Bool operator==( const SIDConnectorLine& other ) const { return m_text == other.m_text; } 
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDConnectorLine )
	PROPERTY_CUSTOM_EDIT( m_text, TXT("Line content"), TXT("LocalizedStringEditor") );
#ifndef NO_EDITOR
	PROPERTY_RO_NOT_COOKED( m_exampleVoiceoverFile, TXT("") );
#endif
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
// Connector pack definition for use in CIDInterlocutorComponent
//------------------------------------------------------------------------------------------------------------------
struct SIDConnectorPackDefinition
{
	DECLARE_RTTI_STRUCT( SIDConnectorPackDefinition );

	CName							m_category;
	IIDContition*					m_condition;
	TSoftHandle< CIDConnectorPack >	m_pack;
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDConnectorPackDefinition )
	PROPERTY_EDIT( m_category, TXT("Connector category") )
	PROPERTY_INLINED( m_condition, TXT("Connector use condition") )
	PROPERTY_EDIT( m_pack, TXT("Connector pack") )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
// Connector request
//------------------------------------------------------------------------------------------------------------------
struct SIDConnectorRequest : public Red::System::NonCopyable
{
	CName							m_category;
	Uint32							m_dialogId;
	EIDPlayState					m_state;
	TSoftHandle< CIDConnectorPack >	m_resource;
	Int32							m_index;
	const SIDConnectorLine*			m_result;

	SIDConnectorRequest( CName category, CName speaker, Uint32 dialogId )
		: m_category( category )
		, m_dialogId( dialogId )
		, m_state( DIALOG_Loading )
		, m_resource( nullptr )
		, m_index( -1 )
		, m_result( nullptr )
	{
	}
};

//------------------------------------------------------------------------------------------------------------------
// Dialog connector pack resource (a list of connectors)
//------------------------------------------------------------------------------------------------------------------
class CIDConnectorPack : public CResource, public ILocalizableObject
{	
	DECLARE_ENGINE_RESOURCE_CLASS( CIDConnectorPack, CResource, "idcpack", "Connector pack" );

protected:
	TDynArray< SVoiceTagWrapper >	m_matchingVoiceTags;
	TDynArray< SIDConnectorLine >	m_lines;

public:
	CIDConnectorPack();

	RED_INLINE Bool MatchesVoiceTag( CName voiceTag ) const { return m_matchingVoiceTags.Exist( SVoiceTagWrapper( voiceTag ) ); }
	RED_INLINE const TDynArray< SVoiceTagWrapper >& GetMatchingVoiceTags() const { return m_matchingVoiceTags; } 
	RED_INLINE Uint32 GetNumLines() const { return m_lines.Size(); } 
	RED_INLINE const LocalizedString& GetText( Uint32 index ) const { return m_lines[ index ].m_text; }
	RED_INLINE const SIDConnectorLine& GetLine( Uint32 index ) const { return m_lines[ index ]; }

	virtual void OnPostLoad();
	virtual void OnPreSave();
	virtual void OnPropertyPostChange( IProperty* prop );
	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */;

	void GetVoiceoverFileNamesForLine( Uint32 lineIndex, TDynArray< String > outFineNames ) const;
	String GetStringKeyForLine( Uint32 lineIndex ) const;
	String GetStringInCurrentLanguageForLine( Uint32 lineIndex ) const;
	String GetVoiceoverFileNameForLine( Uint32 lineIndex, CName voiceTag ) const;

protected:
	void RefreshVoiceovers();
};


BEGIN_CLASS_RTTI( CIDConnectorPack )
	PARENT_CLASS( CResource )
	PROPERTY_EDIT( m_matchingVoiceTags, TXT("Voice tags that can say those lines") )
	PROPERTY_EDIT( m_lines, TXT("Lines of text") )
END_CLASS_RTTI()
