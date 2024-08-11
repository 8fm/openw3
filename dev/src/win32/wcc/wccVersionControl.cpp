#include "build.h"

#include "wccVersionControl.h"

#include "../../common/core/configFileManager.h"

void CVersionControlWCCInterface::OnNotOperational()
{
	LOG_WCC( TXT("[VersionControl] Not operational") );
}

Int32 CVersionControlWCCInterface::OnNoConnection()
{
	LOG_WCC( TXT("[VersionControl] No connection, disabling version control") );
	return SC_OK;
}

Int32 CVersionControlWCCInterface::OnParallelUsers( const TDynArray< CDiskFile* > &fileList, const TDynArray< String > &users, Bool exclusiveAccess )
{
	for ( Uint32 i=0; i < fileList.Size(); ++i )
	{
		LOG_WCC( TXT("[VersionControl] Cannot check out '%s', locked by '%s'"), fileList[i]->GetDepotPath().AsChar(), users[i].AsChar() );
	}
	return SC_CANCEL;
}

Int32 CVersionControlWCCInterface::OnParallelUsers( CDiskFile &file, const TDynArray< String > &users, Bool exclusiveAccess )
{
	for ( Uint32 i=0; i < users.Size(); ++i )
	{
		LOG_WCC( TXT("[VersionControl] Cannot check out '%s', locked by '%s'"), file.GetDepotPath().AsChar(), users[i].AsChar() );
	}
	return SC_CANCEL;
}

Bool CVersionControlWCCInterface::OnSaveFailure( CDiskFile &file )
{
	LOG_WCC( TXT("[VersionControl] Failed to save '%s'"), file.GetDepotPath().AsChar() );
	return false;
}

Int32 CVersionControlWCCInterface::OnCheckOutRequired( const String &path, const TDynArray< String > &users )
{
	return SC_OK; // always check out
}

void CVersionControlWCCInterface::OnDoubleCheckOut()
{
	// ignore
}

void CVersionControlWCCInterface::OnNotEdited( const CDiskFile &file )
{
	// ignore
}

Int32 CVersionControlWCCInterface::OnSubmit( String &description, CDiskFile &resource )
{
	return SC_CANCEL; // do not submit files from WCC
}

Int32 CVersionControlWCCInterface::OnMultipleSubmit( String &, const TDynArray< CDiskFile * > &, TSet< CDiskFile * > & )
{
	return SC_CANCEL; // do not submit files from WCC
}

void CVersionControlWCCInterface::OnFailedSubmit()
{
	// ignore, we do not submit files from WCC
}

void CVersionControlWCCInterface::OnFailedCheckOut()
{
	LOG_WCC( TXT("[VersionControl] Check out request failed") );
}

void CVersionControlWCCInterface::OnLocalFile()
{
	// ignore
}

void CVersionControlWCCInterface::OnLoadedDelete()
{
	// ignore
}

void CVersionControlWCCInterface::OnFailedDelete()
{
	// ignore
}

void CVersionControlWCCInterface::OnSyncRequired()
{
	LOG_WCC( TXT("VersionControl] Sync required before checking out the file") );
}

Int32 CVersionControlWCCInterface::OnSyncFailed()
{
	LOG_WCC( TXT("VersionControl] Sync failed") );
	return SC_CANCEL;
}

Bool CVersionControlWCCInterface::OnConfirmModifiedDelete()
{
	return false; // Do not delete stuff from WCC
}

Bool CVersionControlWCCInterface::OnConfirmDelete()
{
	return false; // Do not delete stuff from WCC
}

void CVersionControlWCCInterface::OnFileLog( CDiskFile &file, TDynArray< THashMap< String, String > > &history )
{
	// ignore
}

void CVersionControlWCCInterface::OnFilesIdentical()
{
	// ignore
}

Bool CVersionControlWCCInterface::OnConfirmRevert()
{
	return true; // Always revert
}

//////////////////////////////////////////////////////////////////////////

Bool InitializeWCCVersionControl( const ICommandlet::CommandletOptions &options )
{
	SSourceControlSettings settings;
	options.GetSingleOptionValue( TXT( "p4user" ), settings.m_user );
	options.GetSingleOptionValue( TXT( "p4client" ), settings.m_client );
	options.GetSingleOptionValue( TXT( "p4host" ), settings.m_host );
	options.GetSingleOptionValue( TXT( "p4password" ), settings.m_password );
	options.GetSingleOptionValue( TXT( "p4port" ), settings.m_port );

	settings.m_automaticChangelists = false;

	// Check if source control isn't disabled (for default it is enabled)
	Bool isDisabledSourceControl( false );
#if defined(RED_PLATFORM_WIN64) || defined(RED_ARCH_X64)
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("SourceControl"), TXT("IsDisabled64"), isDisabledSourceControl );
#else
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("SourceControl"), TXT("IsDisabled"), isDisabledSourceControl );
#endif
	if ( options.HasOption( TXT("nosourcecontrol") ) )
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
		// Create VC instance
		CVersionControlWCCInterface* iface = new CVersionControlWCCInterface();
		GVersionControl = new CSourceControlP4( iface, settings );
	}

	return true;
}

void LogWCCVersionControlOptions()
{
	LOG_WCC( TXT("   -p4user=username           -- username for Perforce") );
	LOG_WCC( TXT("   -p4client=client           -- client (workspace) for Perforce") );
	LOG_WCC( TXT("   -p4host=host               -- hostname for Perforce") );
	LOG_WCC( TXT("   -p4password=password       -- password for Perforce") );
	LOG_WCC( TXT("   -p4port=port               -- port for Perforce") );
}
