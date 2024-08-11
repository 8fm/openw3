/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "configVar.h"
#include "configVarRegistry.h"
#include "configVarStorage.h"
#include "configVarSystem.h"
#include "configVarLegacyWrapper.h"

#include <functional>

#include "fileSys.h"

#include "../core/xmlreader.h"

// Ticking quests can hammer saving configs...
// Use sparingly.
Bool GHackDelaySettingsSave;

namespace Config
{
	const Char* CConfigSystem::FILE_EXTENSION = TXT(".ini");
	const Char* CConfigSystem::FILE_SEARCH_EXTENSION = TXT("*.ini");

	CConfigSystem::CConfigSystem()
		: m_base( new CConfigVarStorage() )
		, m_user( new CConfigVarStorage() )
		, m_registry( new CConfigVarRegistry() )
		, m_legacy( new Legacy::CConfigLegacyManager() )
		, m_userConfigLoaderSaver( nullptr )
		, m_configResetInThisSession( false )
	{
	}

	CConfigSystem::~CConfigSystem()
	{
	}

	void CConfigSystem::Reload()
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Config loading has to be done on main thread only." );

		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		// reload values
		Load( eConfigVarSetMode_Reload );
	}

	void CConfigSystem::Load( const String& rootPath, IUserConfigLoaderSaver* userConfigLoaderSaver )
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Config loading has to be done on main thread only." );

		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		m_userConfigLoaderSaver = userConfigLoaderSaver;

		// save paths
		m_paths.Clear();
		LoadConfigPaths( rootPath );

		// load initial values
		Load( eConfigVarSetMode_Reload );

		// load legacy config (per game)
		{
			const String legacyConfigPath = rootPath + TXT("r4game\\legacy\\");
			LOG_CORE( TXT("Config legacy path: '%ls'"), legacyConfigPath.AsChar() );
			m_legacy->Initialize( legacyConfigPath );
		}
	}

	void CConfigSystem::Save()
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Config loading has to be done on main thread only." );

		// Maybe something else will be nice and save us
		if ( GHackDelaySettingsSave )
		{
			LOG_CORE(TXT("CConfigSystem::Save() - returned early due to GHackDelaySettingsSave"));
			return;
		}

		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		// Save only the values that are different than the base values
		// This way we only store what user has actually changed

		// Flush variables from register to m_user
		TDynArray< IConfigVar* > registeredVars;
		m_registry->EnumVars( registeredVars );

		// Flush saveable vars into the storage
		for( auto var : registeredVars )
		{
			if ( var->HasFlag( eConsoleVarFlag_Save ) )
			{
				String value = TXT("");
				var->GetText( value );
				m_user->SetEntry( var->GetGroup(), var->GetName(), value );
			}
		}

		CConfigVarStorage temp;
		m_user->FilterDifferences( *m_base, temp );

		ApplyBaseVersionToStorage( temp );

		// Save the config
		if( m_userConfigLoaderSaver != nullptr )
		{
			if( m_userConfigLoaderSaver->SaveFromStorage( &temp ) == false )
			{
				ERR_CORE( TXT("Unable to save user config") );
			}
		}
	}

	void CConfigSystem::Load( const EConfigVarSetMode setMode )
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Config loading has to be done on main thread only." );

		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		m_configResetInThisSession = false;

		// Iterate through all paths (but not the user path)
		for( Uint32 i=0; i<m_paths.Size(); ++i )
		{
			TDynArray< String > files;
			GFileManager->FindFiles( m_paths[i].AsChar(), FILE_SEARCH_EXTENSION, files, false );
			LOG_CORE( TXT("Found %d engine configs in path %s"), files.Size(), m_paths[i].AsChar() );

			for ( const String& configFilePath : files )
				m_base->Load( configFilePath );
		}

		// load user config
		if( m_userConfigLoaderSaver != nullptr )
		{
			if( m_userConfigLoaderSaver->LoadToStorage( m_user.Get() ) == false )
			{
				WARN_CORE( TXT("There is no custom user config. Using default.") );
			}
		}

		// Apply base values
		m_registry->Refresh( *m_base, setMode );

		// Apply user values if matching base version, otherwise clear user settings
		Int32 baseConfigVersion = GetBaseConfigVersion();
		Int32 userConfigVersion = GetUserConfigVersion();

		Bool applyUserConfigs = true;
		if( userConfigVersion != baseConfigVersion )
		{
			applyUserConfigs = false;
		}

		if( applyUserConfigs == true )
		{
			m_registry->Refresh( *m_user, setMode );
			m_configResetInThisSession = false;
			LOG_CORE( TXT("User settings applied to config registy.") );
		}
		else
		{
			m_user->Clear();
			m_configResetInThisSession = true;
			LOG_CORE( TXT("User settings have different version number than base - resetting user settings (does not include input)") );
		}
	}

	Bool CConfigSystem::GetValue( const AnsiChar* groupName, const AnsiChar* keyName, String& outValue ) const
	{
		// find variable
		IConfigVar* var = m_registry->Find( groupName, keyName );
		if ( var )
			return var->GetText( outValue );

		// find in user storage
		if ( m_user->GetEntry( groupName, keyName, outValue ) )
			return true;

		// as a fallback, use the global storage
		return m_base->GetEntry( groupName, keyName, outValue );
	}

	Bool CConfigSystem::SetValue( const AnsiChar* groupName, const AnsiChar* keyName, const String& value )
	{
		// find variable
		IConfigVar* var = m_registry->Find( groupName, keyName );
		if ( var )
			return var->SetText( value, eConfigVarSetMode_User );

		// set in the user storage
		return m_user->SetEntry( groupName, keyName, value );
	}

	Bool CConfigSystem::GetDefaultValue( const AnsiChar* groupName, const AnsiChar* keyName, String& outValue ) const
	{
		if( m_base->GetEntry( groupName, keyName, outValue ) )
		{
			return true;
		}

		if( const IConfigVar* var = m_registry->Find( groupName, keyName ) )
		{
			return var->GetTextDefault( outValue );
		}

		return false;
	}

	void CConfigSystem::LoadConfigPaths( const String& rootPath )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		String setFilePath = rootPath + TXT("config_description.xml");
		String xmlContent = TXT("");
		GFileManager->LoadFileToString( setFilePath, xmlContent, true );
		CXMLReader reader( xmlContent );

		String setName = GetPlatformString< Char >();

		m_paths.Clear();

		if( reader.BeginNode( TXT("OverrideOrder") ) == true )
		{
			// Iterate all sets and find the one we are looking for
			while( reader.BeginNode( TXT("Set") ) == true )
			{
				String xmlSetName;
				reader.Attribute( TXT("name"), xmlSetName );

				if( xmlSetName == setName )
				{
					// Iterate all path in proper order
					while( reader.BeginNode( TXT("Path") ) == true )
					{
						String xmlPath;
						reader.Attribute( TXT("name"), xmlPath );
						m_paths.PushBack( rootPath + xmlPath );

						reader.EndNode();
					}
				}

				reader.EndNode();
			}

			reader.EndNode( false );
		}
		else
		{
			LOG_CORE( TXT("Can't load or find %s"), setFilePath.AsChar() );
			RED_FATAL_ASSERT( false, "Can't load or find config_description.xml file in: %ls - so no configs for engine and game will be loaded!", setFilePath.AsChar() );
		}
	}

	void CConfigSystem::Shutdown()
	{
		m_userConfigLoaderSaver = nullptr;
	}

	void CConfigSystem::ResetUserSettings( const EConfigVarSetMode applyMode )
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Config resetting has to be done on main thread only." );

		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		// Reset all configs to their default value from code
		ResetConfigs( applyMode );

		// Clear user values
		m_user->Clear();

		// Apply the base values
		m_registry->Refresh( *m_base, applyMode );
	}

	void CConfigSystem::ResetConfigs( const EConfigVarSetMode mode, const AnsiChar* groupMatch /*= ""*/, const AnsiChar* nameMatch /*= ""*/, const Uint32 includeFlags /*= 0*/, const Uint32 excludeFlags /*= 0*/ )
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Config resetting has to be done on main thread only." );

		Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_lock );

		TDynArray<IConfigVar*> vars;
		m_registry->EnumVars( vars, groupMatch, nameMatch, includeFlags, excludeFlags );

		for( IConfigVar* var : vars )
		{
			var->Reset( mode );
		}
	}

	Int32 CConfigSystem::GetBaseConfigVersion() const
	{
		// Get version as string
		Bool baseVersionFound = false;
		String baseConfigVersionString;
		baseVersionFound = m_base->GetEntry( "General", "ConfigVersion", baseConfigVersionString );
		RED_FATAL_ASSERT( baseVersionFound == true, "No config version specified! [General] ConfigVersion=? not found!" );

		// Convert version to int
		Bool baseVersionValid = false;
		Int32 baseConfigVersion = 0;
		baseVersionValid = FromString( baseConfigVersionString, baseConfigVersion );
		RED_FATAL_ASSERT( baseVersionValid == true, "Config base version format is invalid! Version format should be a number" );

		return baseConfigVersion;
	}

	Int32 CConfigSystem::GetUserConfigVersion() const
	{
		// Get version as string
		Bool userVersionFound = false;
		String userConfigVersionString;
		m_user->GetEntry( "General", "ConfigVersion", userConfigVersionString );

		// Convert version to int
		Bool userVersionValid = false;
		Int32 userConfigVersion = -1;
		userVersionValid = FromString( userConfigVersionString, userConfigVersion );

		// If user version has invalid format set it to -1, so we can clear user settings
		if( userVersionValid == false )
		{
			userConfigVersion = -1;
		}

		return userConfigVersion;
	}

	void CConfigSystem::ApplyBaseVersionToStorage( CConfigVarStorage& storage )
	{
		Int32 baseVersion = GetBaseConfigVersion();
		storage.SetEntry( "General", "ConfigVersion", ToString(baseVersion) );
	}

} // Config
