/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "hashmap.h"
#include "scopedPtr.h"
#include "configVar.h"

/// How configuration works
///  It consists of 4 layers:
///     - Base config (manual entries, read only)
///     - Platform config, per platform: PC, Xbox1, PS4 (manual entries, read only)
///     - Game config (manual entries, read only)
///     - User config (read write, can be used in game, stored in user profile if needed), also per platform
///
///  The values for config vars "visible" to the outside world are calculated as a merged result of those 4 layers.
///
///  Also, when an user config is saved the values that are THE same as in the read-only layers are not saved, only the delta is.

namespace Config
{
	class CConfigVarStorage;
	class CConfigVarRegistry;
	class CRemoteConfig;

	namespace Legacy
	{
		class CConfigLegacyManager;
	} // Legacy

	class IUserConfigLoaderSaver
	{
	public:
		virtual ~IUserConfigLoaderSaver() { /* Intentionally Empty */ }
		virtual Bool LoadToStorage( CConfigVarStorage* storage ) = 0;
		virtual Bool SaveFromStorage( const CConfigVarStorage* storage ) = 0;
	};

	/// Config system master class
	class CConfigSystem
	{
	public:
		CConfigSystem();
		~CConfigSystem();

		// reload configuration
		void Reload();

		// reset user settings
		void ResetUserSettings( const EConfigVarSetMode applyMode );

		// load configuration
		void Load( const String& rootPath, IUserConfigLoaderSaver* userConfigLoaderSaver );

		// flush changes in the settings
		void Save();

		// get value for given group and key
		Bool GetValue( const AnsiChar* groupName, const AnsiChar* keyName, String& outValue ) const;

		// set value for given group and key (propagates to a variable if found)
		Bool SetValue( const AnsiChar* groupName, const AnsiChar* keyName, const String& value );

		// get value for given group and key
		Bool GetDefaultValue( const AnsiChar* groupName, const AnsiChar* keyName, String& outValue ) const;

		// reset values of all registered configs to their default values
		void ResetConfigs( const EConfigVarSetMode mode, const AnsiChar* groupMatch = "", const AnsiChar* nameMatch = "", const Uint32 includeFlags = 0, const Uint32 excludeFlags = 0 );

		// shutdown system, detach from user config loader saver
		void Shutdown();

		// When configs are reset, we have to inform game about that
		RED_FORCE_INLINE Bool AreConfigResetInThisSession() const { return m_configResetInThisSession; }

		// get the instance (created on first use)
		//static CConfigSystem& GetInstance();

		// get config registry
		RED_FORCE_INLINE CConfigVarRegistry& GetRegistry() const { return *m_registry; }

		// get legacy config
		RED_FORCE_INLINE Legacy::CConfigLegacyManager& GetLegacy() { return *m_legacy; }

	private:
		// each group of values lie in different storage
		Red::TScopedPtr< CConfigVarStorage >	m_base; // merged read only config
		Red::TScopedPtr< CConfigVarStorage >	m_user; // dynamic config

		// main config registry
		Red::TScopedPtr< CConfigVarRegistry >	m_registry;

		// legacy config
		Red::TScopedPtr< Legacy::CConfigLegacyManager >		m_legacy;

		// stored user file name
		TDynArray<String>		m_paths;		// All config paths in override order

		// reload settings
		void Load( const EConfigVarSetMode applyMode );

		// Load all paths for given set
		void LoadConfigPaths( const String& rootPath );

		// Always returns valid version - otherwise asserts
		Int32 GetBaseConfigVersion() const;

		// If user version can't be read - returns -1
		Int32 GetUserConfigVersion() const;

		// Applies config version from m_base to given storage
		void ApplyBaseVersionToStorage( CConfigVarStorage& storage );

		// file extension
		static const Char* FILE_EXTENSION;
		static const Char* FILE_SEARCH_EXTENSION;

		IUserConfigLoaderSaver* m_userConfigLoaderSaver;

		Red::Threads::CMutex m_lock;

		Bool m_configResetInThisSession;
	};

	enum EPlatform
	{
		ePlatform_PC,
		ePlatform_XB1,
		ePlatform_PS4,
		ePlatform_Unknown,
	};
	
	// Returns active platform string
	template< typename TChar >
	RED_INLINE const TChar* GetPlatformString()
	{
#if defined(RED_PLATFORM_WINPC)
		return RED_TEMPLATE_TXT( TChar, "pc" );
#elif defined(RED_PLATFORM_DURANGO)
		return RED_TEMPLATE_TXT( TChar, "xbox1" );
#elif defined(RED_PLATFORM_ORBIS)
		return RED_TEMPLATE_TXT( TChar, "ps4" );
#else
		return RED_TEMPLATE_TXT( TChar, "unknown" );
#endif
	}

	// Returns active platform enum
	RED_INLINE const EPlatform GetPlatform()
	{
#if defined(RED_PLATFORM_WINPC)
		return ePlatform_PC;
#elif defined(RED_PLATFORM_DURANGO)
		return ePlatform_XB1;
#elif defined(RED_PLATFORM_ORBIS)
		return ePlatform_PS4;
#else
		return ePlatform_Unknown;
#endif
	}

} // Config

/// New config
typedef TSingleton< Config::CConfigSystem, TNoDestructionLifetime > SConfig;