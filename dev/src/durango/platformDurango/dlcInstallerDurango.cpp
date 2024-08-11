/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/fileSys.h"
#include "../../common/core/scopedPtr.h"
#include "../../common/core/xmlFileReader.h"
#include "dlcInstallerDurango.h"
#include "dlcInstallerDurango_p.h"

//////////////////////////////////////////////////////////////////////////
// CDlcInstallerDurango
//////////////////////////////////////////////////////////////////////////
CDlcInstallerDurango::CDlcInstallerDurango()
	: m_packageAutoMounter( nullptr )
{
}

CDlcInstallerDurango::~CDlcInstallerDurango()
{
	delete m_packageAutoMounter;
}

Bool CDlcInstallerDurango::Init()
{
	if ( !InitPackageAutoMounter() )
	{
		return false;
	}

	return true;
}

Bool CDlcInstallerDurango::InitPackageAutoMounter()
{
	TDynArray< String > blacklistedDlc;
	if ( !InitBlacklist( blacklistedDlc ) )
	{
		return false;
	}

	m_packageAutoMounter = new Helper::CDlcPackageAutoMounter;
	m_packageAutoMounter->Init( blacklistedDlc );
	m_packageAutoMounter->RequestRefresh();

	return true;
}

Bool CDlcInstallerDurango::InitBlacklist( TDynArray< String >& outBlacklistedDlc )
{
	const String xmlPath = GFileManager->GetBaseDirectory() + TXT("initialdata\\dlc_blacklist.xml");

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( xmlPath, FOF_AbsolutePath | FOF_MapToMemory ) );
	if ( !file )
	{
		ERR_CORE( TXT("Missing '%ls'"), xmlPath.AsChar() );
		return false;
	}
	
	CXMLFileReader xmlReader( *file );
	if ( xmlReader.BeginNode(TXT("blacklist")))
	{
		const String DLC_NODE(TXT("dlc"));
		const String ID_ATTR(TXT("id"));

		while ( xmlReader.BeginNode(DLC_NODE) )
		{
			String id;
			if ( xmlReader.Attribute( ID_ATTR, id ) )
			{
				outBlacklistedDlc.PushBack( id );
				LOG_ENGINE(TXT("CDlcInstallerDurango::InitBlacklist: blacklisting DLC '%ls'"), id.AsChar() );
			}
			else
			{
				ERR_ENGINE(TXT("Blacklisted DLC missing id!"));
				return false;
			}
			xmlReader.EndNode(); // dlc
		}

		xmlReader.EndNode(); // blacklist
	}

	return true;
}

void CDlcInstallerDurango::OnSuspend()
{
	// Wait for resume
}

void CDlcInstallerDurango::OnResume()
{
	LOG_ENGINE(TXT("CDlcInstallerDurango[OnResume]"));

	// Who knows what happened while suspended and what events we miss
	m_packageAutoMounter->RequestRefresh();
}

void CDlcInstallerDurango::OnExitConstrain()
{
	LOG_ENGINE(TXT("CDlcInstallerDurango[OnExitConstrain]"));

	// Could have gotten license changes by settings this as the home console
	// or who knows. We only get events for packages installed, not licensed.
	m_packageAutoMounter->RequestRefresh();
}

Bool CDlcInstallerDurango::IsReady() const 
{
	return !m_packageAutoMounter->IsDirty();
}

void CDlcInstallerDurango::Update( TDynArray< SContentPackageEvent >& outContentPackageEvents )
{
	m_packageAutoMounter->Update();

	TDynArray< Helper::SPackageInfo > packageInfos;
	if ( m_packageAutoMounter->FlushPackageInfos( packageInfos ) )
	{
		for ( auto& pkgInfo : packageInfos )
		{
			auto findIt = m_allPackageInfos.Find( pkgInfo.m_packageID );
			if ( findIt == m_allPackageInfos.End() )
			{
				m_allPackageInfos.Insert( pkgInfo.m_packageID, pkgInfo );
				const Uint32 flags = eContentPackageMountFlags_AutoAttachChunks | eContentPackageMountFlags_IsHidden;

				// Mount paths don't end with a trailing slash
				const String mountPath = pkgInfo.m_mountPoint + TXT("\\content\\");
				outContentPackageEvents.PushBack( SContentPackageEvent( pkgInfo.m_packageID, mountPath.AsChar(), flags ) );

				// Shouldn't ever happen, but just to be defensive about it
				if ( !pkgInfo.m_isLicensed )
				{
					WARN_ENGINE(TXT("Mounted unlicensed packageID=%u??? Sending license event"), pkgInfo.m_packageID );
					outContentPackageEvents.PushBack( SContentPackageEvent( pkgInfo.m_packageID, false ) );
				}
			}
			else
			{
				Helper::SPackageInfo& foundPkgInfo = findIt->m_second;
				if ( foundPkgInfo.m_isLicensed != pkgInfo.m_isLicensed )
				{
					foundPkgInfo.m_isLicensed = pkgInfo.m_isLicensed;
					outContentPackageEvents.PushBack( SContentPackageEvent( pkgInfo.m_packageID, pkgInfo.m_isLicensed ) );
				}
			}
		}
	}
}
