/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/redThreads/redThreadsTypes.h"
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

		SPackageInfo( RuntimePackageID packageID, const StringAnsi& mountPoint, Bool isLicensed)
			: m_mountPoint( mountPoint )
			, m_packageID( packageID )
			, m_isLicensed( isLicensed )
		{}

		StringAnsi	m_mountPoint;
		Uint32		m_packageID;
		Bool		m_isLicensed;
	};
}

class CDlcInstallerOrbis : public IContentInstaller, /*private ILanguageProvider,*/ private Red::System::NonCopyable
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

public:
											CDlcInstallerOrbis();
	virtual									~CDlcInstallerOrbis();
	virtual Bool							Init() override;
	virtual void							Update( TDynArray< SContentPackageEvent >& outContentPackageEvents ) override;
	virtual Bool							IsReady() const override;

public:
	void									OnEntitlementUpdated();

private:
	Bool									InitPackageAutoMounter();
	Bool									InitBlacklist( TDynArray< String >& outBlacklistedDlc );

private:
	Helper::CDlcPackageAutoMounter*							m_packageAutoMounter;
	TArrayMap< RuntimePackageID, Helper::SPackageInfo >		m_allPackageInfos;
};
