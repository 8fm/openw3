/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "VersionControl/interface/versionControl.h"

#include "../../common/redNetwork/manager.h"
#include "../../common/redNetwork/channel.h"

#include "../../common/redSystem/windowsDebuggerWriter.h"
#include "../../common/redSystem/logFile.h"

class Solution;

enum ESolutionImage
{
	SOLIMG_Classes,
	SOLIMG_DirOpened,
	SOLIMG_DirClosed,
	SOLIMG_NotInDepot,
	SOLIMG_NotInDepotModified,
	SOLIMG_FileNormal,
	SOLIMG_FileNormalOutOfDate,
	SOLIMG_FileLocked,
	SOLIMG_FileLockedOutOfDate,
	SOLIMG_FileModified,
	SOLIMG_FileModifiedOutOfDate,
	SOLIMG_FileAdd,
	SOLIMG_FileAddModified,
	SOLIMG_Deleted,

	SOLIMG_Max
};

class CSSApp : public wxApp
{
private:
	HMODULE									m_versionControlLibraryHandle;
	VersionControl::Interface*				m_versionControl;
	VersionControl::Interface				m_dummyVersionControl;
	wxPanel*								m_XResourcesPanel;	//!< Resources container
	Red::Network::Manager					m_network;
	uint16_t								m_networkPort;
	Red::System::Log::WindowsDebuggerWriter	m_debugLogDevice;
	Red::System::Log::File					m_fileLogDevice;
	Solution*								m_solution;

private:
	bool InitializeVersionControl( bool noVersionControl = false );
	void ShutdownVersionControl();
	wxString GetVersionControlCredentialsConfigPath() const;

public:
	CSSApp();
	~CSSApp();

	virtual bool OnInit();

	VersionControl::Interface* GetVersionControl() { return m_versionControl; }
	bool SaveVersionControlCredentials( const map< wxString, wxString >& credentials );
	bool LoadVersionControlCredentials( map< wxString, wxString >& credentials ) const;

	uint16_t GetNetworkPort() const { return m_networkPort; }
	void SetNetworkPort( uint16_t port ) { m_networkPort = port; }

public:
	wxBitmap LoadBitmap( const wxString& name );

	wxImageList* CreateSolutionImageList();

	virtual void OnAssertFailure( const wxChar* file, int line, const wxChar* func, const wxChar* cond, const wxChar* msg );
};

/// SS app
#define wxTheSSApp wx_static_cast( CSSApp*, wxApp::GetInstance() )
