/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_GAME_CONFIGURATION_H_
#define _CORE_GAME_CONFIGURATION_H_

#include "string.h"
#include "singleton.h"

struct SGameConfigurationParameter
{
	String name;
	String dataPathSuffix;
	String userPathSuffix;
	String bundlePathSuffix;
	String scriptsPathSuffix;
	String configDirectoryName;
	String gameClassName;
	String playerClassName;
	String telemetryClassName;
	String cameraDirectorClassName;
};

class CGameConfiguration
{
public:

	const String& GetName() const;
	const String& GetDataPathSuffix() const;
	const String& GetUserPathSuffix() const;
	const String& GetBundlePathSuffix() const;
	const String& GetScriptsPathSuffix() const;
	const String& GetConfigDirName() const;
	const String& GetGameClassName() const;
	const String& GetPlayerClassName() const;
	const String& GetTelemetryClassName() const;
	const String& GetCameraDirectorClassName() const;

	void Initialize( const SGameConfigurationParameter& param );
	const SGameConfigurationParameter& GetConfigParameters() const { return m_param; }

private:

	SGameConfigurationParameter m_param;

};

typedef TSingleton< CGameConfiguration, TDefaultLifetime, TCreateStatic, TSingleThreaded > GGameConfig;

#endif
