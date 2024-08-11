#include "build.h"
#include "wxVersionControlInterface.h"
#include "..\..\win32\versionControl\versionControlP4.h"
#include "../../common/core/configVar.h"
#include "../../common/core/configVarLegacyWrapper.h"

void SInitializeVersionControl()
{
	// Load version control settings
	SSourceControlSettings settings;
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
#if defined(RED_PLATFORM_WIN64) || defined(RED_ARCH_X64)
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl64") );
#else
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl") );
#endif
	config.Read( TXT("User"), &settings.m_user );
	config.Read( TXT("Password"), &settings.m_password );
	config.Read( TXT("Workspace"), &settings.m_client );
	config.Read( TXT("Host"), &settings.m_host );
	config.Read( TXT("Port"), &settings.m_port );
	settings.m_automaticChangelists = config.Read( TXT("AutomaticChangelists"), 1 ) != 0;

#ifndef RED_FINAL_BUILD
	// FIXME: for some reason passing user name to the database results in an error
	// this is a temporary hack to avoid it
	extern String GSQLStringDBUserName;
	GSQLStringDBUserName = TXT(""); //settings.m_user.BeginsWith( TXT("W2_") ) ? settings.m_client : settings.m_user;
#endif

	// Check if source control isn't disabled (for default it is enabled)
	Bool isDisabledSourceControl( false );
	RED_MESSAGE( "Convert to new config" );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("SourceControl"), TXT("IsDisabled64"), isDisabledSourceControl );
	if ( Red::System::StringSearch( SGetCommandLine(), TXT("-nosourcecontrol") ) )
	{
		// command line parameter overrides user.ini setting
		isDisabledSourceControl = true;
	}
	if ( isDisabledSourceControl )
	{
		GVersionControl = new ISourceControl;
	}
	else
	{
		// Spawn new version control - P4
		IVersionControlEditorInterface *editorInterface = new CEdVersionControlInterface();
		GVersionControl = new CSourceControlP4( editorInterface, settings );
	}
}
