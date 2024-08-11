/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "sortedmap.h"

/// DLC gameplay content activator
/// NOTE: This class depends on the engine classes :(
class IDLCGameplayContentActivator
{
public:
	IDLCGameplayContentActivator();

	/// Validate savegame to be used with this DLC
	/// If failed return false and the error name CName. Error names should be localized.
	virtual Bool OnValidateSaveGame( class IGameLoader* loader ) = 0;

	/// Called just after the DLC is mounted to the game
	/// This call happens outside the active game, useful only for menu changes
	virtual void OnActivated() = 0;

	/// Called just after the DLC is unmounted from the game
	/// NOTE: the physical DLC is still mounted
	virtual void OnDeactivated() = 0;
};

/// DLC localized content - content that may vary per language, does not have to be just text
class CDLCLocalizedContent
{
public:
	CDLCLocalizedContent();

	// Get value for given language
	// TODO: do we have language IDs somewhere ? Right now use generic string.
	void Get( const String& languageKey, String& result ) const;

	// Set value for given language
	// TODO: do we have language IDs somewhere ? Right now use generic string.
	void Set( const String& languageKey, const String& result );

private:
	typedef Uint32 TLangKey;
	typedef TSortedMap< TLangKey, String, MC_Depot > TContentMap;

	TContentMap		m_content;
};

/// DLC manifest - wrapped text file
class CDLCManifest
{
protected:
	// Internal DLC ID
	CName			m_id;

	// DLC localized content
	CDLCLocalizedContent		m_title;
	CDLCLocalizedContent		m_desc;

	// DLC mounter

public:
	CDLCManifest
};