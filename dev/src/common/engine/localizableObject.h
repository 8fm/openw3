/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "localizedContent.h"

struct LocalizedStringEntry
{
	const CResource*	m_parentResource; // if NULL, then use "CUSTOM" string
	String				m_propertyName;
	String				m_voiceoverName;
	LocalizedString*	m_localizedString;
	String				m_stringKey;

	LocalizedStringEntry()
		: m_parentResource( NULL )
		, m_propertyName( String::EMPTY )		
		, m_voiceoverName( String::EMPTY )
		, m_localizedString( NULL )
		, m_stringKey( String::EMPTY )
	{
	}

	LocalizedStringEntry
	(
		LocalizedString *localizedString,
		const String& propertyName, 
		const CResource* parentResource,
		const String& voiceoverName = String::EMPTY,
		const String& stringKey = String::EMPTY
	)
	: m_parentResource( parentResource )
	, m_propertyName( propertyName )
	, m_voiceoverName( voiceoverName )
	, m_localizedString( localizedString )
	, m_stringKey( stringKey )
	{
	}

	Bool operator==( const LocalizedStringEntry& other )
	{
		return m_localizedString == other.m_localizedString;
	}
};

class ILocalizableObject
{
public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) = 0;
};