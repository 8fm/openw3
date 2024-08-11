/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/contentManifestAsyncLoader.h"
#include "../../common/core/streamingInstaller.h"
#include "../../common/core/installer.h"

struct SContentPackageEvent;

namespace Helper
{
	//////////////////////////////////////////////////////////////////////////
	// Forward Declarations
	//////////////////////////////////////////////////////////////////////////
	class CDlcPackageAutoMounter;

	//////////////////////////////////////////////////////////////////////////
	// SPackageInfo
	//////////////////////////////////////////////////////////////////////////
	struct SPackageInfo
	{
		SPackageInfo()
			: m_packageID( INVALID_RUNTIME_PACKAGE_ID )
			, m_isLicensed( false )
		{}

		SPackageInfo( RuntimePackageID packageID, const String& mountPoint, Bool isLicensed)
			: m_mountPoint( mountPoint )
			, m_packageID( packageID )
			, m_isLicensed( isLicensed )
		{}

		String		m_mountPoint;
		Uint32		m_packageID;
		Bool		m_isLicensed;
	};
}

class CDlcInstallerDurango : public IContentInstaller, /*private ILanguageProvider,*/ private Red::System::NonCopyable
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

public:
											CDlcInstallerDurango();
	virtual									~CDlcInstallerDurango();
	virtual									Bool Init() override;
	virtual									void Update( TDynArray< SContentPackageEvent >& outContentPackageEvents ) override;
	virtual									void OnSuspend() override;
	virtual									void OnResume() override;
	virtual									Bool IsReady() const override;
	virtual									void OnExitConstrain() override;

private:
	Bool									InitPackageAutoMounter();
	Bool									InitBlacklist( TDynArray< String >& outBlacklistedDlc );

private:
	Helper::CDlcPackageAutoMounter*							m_packageAutoMounter;
	TArrayMap< RuntimePackageID, Helper::SPackageInfo >		m_allPackageInfos;
};
