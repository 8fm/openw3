

#include "build.h"
#include "gameConfiguration.h"

const String& CGameConfiguration::GetName() const
{
	return m_param.name;
}

const String& CGameConfiguration::GetDataPathSuffix() const
{
	return m_param.dataPathSuffix;
}

const String& CGameConfiguration::GetUserPathSuffix() const
{
	return m_param.userPathSuffix;
}

const String& CGameConfiguration::GetBundlePathSuffix() const
{
	return m_param.bundlePathSuffix;
}

const String& CGameConfiguration::GetScriptsPathSuffix() const
{
	return m_param.scriptsPathSuffix;
}

const String& CGameConfiguration::GetConfigDirName() const
{
	return m_param.configDirectoryName;
}

const String& CGameConfiguration::GetGameClassName() const
{
	return m_param.gameClassName;
}

const String& CGameConfiguration::GetPlayerClassName() const
{
	return m_param.playerClassName;
}

const String& CGameConfiguration::GetTelemetryClassName() const
{
	return m_param.telemetryClassName;
}

const String& CGameConfiguration::GetCameraDirectorClassName() const
{
	return m_param.cameraDirectorClassName;
}

void CGameConfiguration::Initialize( const SGameConfigurationParameter& param )
{
	m_param = param;
}
