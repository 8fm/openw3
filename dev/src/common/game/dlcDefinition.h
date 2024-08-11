#pragma once

#ifndef NO_EDITOR
#include "../core/analyzer.h"
#endif

#include "dlcLanguagePack.h"

//--------------------------------------------------

class IGameplayDLCMounter;

//--------------------------------------------------

/// DLC content definition (game side)
/// DLC has some basic params like name, title that can be localized and used in the UI.
/// From the point of the content it consists of "DLC mounters" that actually "attach" the content with the game.
/// It's always up to the gameplay programmer/designer to write such mounter and integrate it with appropiate system.
class CDLCDefinition : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CDLCDefinition, CResource, "reddlc", "DLC Definition" );

private:
	// unique DLC ID
	CName						m_id;

	// localized data (for UI)
	String				m_localizedNameKey;
	String				m_localizedDescriptionKey;

	// mounters
	typedef TDynArray< IGameplayDLCMounter* >		TMounters;
	TMounters					m_mounters;

	// language packs
	typedef TDynArray< SDLCLanguagePack >			TLanguagePacks;
	TLanguagePacks				m_languagePacks;

	// DLC visibility
	Bool						m_initiallyEnabled;
	Bool						m_visibleInDLCMenu;

	// Game saving
	Bool						m_requiredByGameSave;

public:
	CDLCDefinition();

	//! Get DLC ID
	RED_INLINE const CName& GetID() const { return m_id; }

	//! Get localized name
	RED_INLINE const String GetName() const { return m_localizedNameKey; }

	//! Get localized description
	RED_INLINE const String GetDescription() const { return m_localizedDescriptionKey; }

	//! Serialization
	virtual void OnSerialize( IFile& file ) override;

	//! Is this DLC visible in the list in the menu ?
	RED_INLINE const Bool IsVisibleInMenu() const { return m_visibleInDLCMenu; }

	//! Is this DLC enabled by default ?
	RED_INLINE const Bool IsInitiallyEnabled() const { return m_initiallyEnabled; }

	//! Is this DLC enabled by default ?
	RED_INLINE const Bool IsRequiredByGameSave() const { return m_requiredByGameSave; }

	//! Get content mounters
	void GetContentMounters( TDynArray< THandle< IGameplayDLCMounter > >& outMounters ) const;

	//! See if this DLC have properly implemented standalone mode
	Bool CanStartInStandaloneMode() const;

	//! Create reader for standalone mode
	IFile* CreateStarterFileReader() const;

	//! Get language packs
	const TDynArray< SDLCLanguagePack >& GetLanguagePacks() const { return m_languagePacks; }

#ifndef NO_EDITOR
	bool DoAnalyze( CAnalyzerOutputList& outputList );
#endif
};

BEGIN_CLASS_RTTI( CDLCDefinition );
	PARENT_CLASS( CResource );
	PROPERTY_EDIT( m_id, TXT("DLC ID - must be globally unique, please use dlc_xxx_yyy format.") );
	PROPERTY_EDIT( m_localizedNameKey, TXT("Display name of the DLC in the menu") );
	PROPERTY_EDIT( m_localizedDescriptionKey, TXT("Detailed description of the DLC - in the menu") );
	PROPERTY_INLINED( m_mounters, TXT("DLC content mounters") );
	PROPERTY_INLINED( m_languagePacks, TXT("Additional base language packs") );
	PROPERTY_EDIT( m_initiallyEnabled, TXT("Is this DLC enabled by default?") );
	PROPERTY_EDIT( m_visibleInDLCMenu, TXT("Will this DLC show up in the list of DLC so user can toggle it on/off ?") );
	PROPERTY_EDIT( m_requiredByGameSave, TXT("Every game save made with this DLC mounted will require this DLC to load.") );
END_CLASS_RTTI();

//--------------------------------------------------
