/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include <app_content.h>

#include "../../common/core/fileSys.h"
#include "../../common/core/scopedPtr.h"
#include "../../common/core/bitset.h"
#include "../../common/core/installer.h"
#include "../../common/core/xmlFileReader.h"

#include "dlcInstallerOrbis.h"
#include "dlcInstallerOrbis_p.h"

//////////////////////////////////////////////////////////////////////////
// CDlcInstallerOrbis
//////////////////////////////////////////////////////////////////////////
CDlcInstallerOrbis::CDlcInstallerOrbis()
	: m_packageAutoMounter( nullptr )
{
}	

Bool CDlcInstallerOrbis::Init()
{
	if ( !InitPackageAutoMounter() )
	{
		return false;
	}

	return true;
}

Bool CDlcInstallerOrbis::InitPackageAutoMounter()
{
	TDynArray< String > blacklistedDlc;
	if ( !InitBlacklist( blacklistedDlc ) )
	{
		return false;
	}

	// We'll be tolerant of DLC blacklist XML having lower case letters by converting them to upper case as per this section:
	/*
	PSN Overview  

	3. 1 NP IDs (for PlayStation 4)

	Entitlement Label

	This is an identifier assigned to a unified entitlement that is linked to a downloadable product or expiration date type product.
	Entitlement labels for unified entitlements are 16 digits and are assigned by content providers rather than by SCE.
	The only characters allowed for are upper-case alphabetic characters and numbers (A-Z, 0-9).
	*/
	for ( String& label : blacklistedDlc )
	{
		CLowerToUpper conv( label.TypedData(), label.Size() );
	}

	m_packageAutoMounter = new Helper::CDlcPackageAutoMounter( blacklistedDlc );
	m_packageAutoMounter->RequestRefresh();

	return true;
}

Bool CDlcInstallerOrbis::InitBlacklist( TDynArray< String >& outBlacklistedDlc )
{
	//const String xmlPath = GFileManager->GetBaseDirectory() + TXT("initialdata\\dlc_blacklist.xml");
	// FIXME: CFileManager is initialized with rootPath instead of workingPath on PS4!
	const String xmlPath = GFileManager->GetRootDirectory() + TXT("bin\\initialdata\\dlc_blacklist.xml");

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
				LOG_ENGINE(TXT("CDlcInstallerOrbis::InitBlacklist: blacklisting DLC '%ls'"), id.AsChar() );
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

CDlcInstallerOrbis::~CDlcInstallerOrbis()
{
	delete m_packageAutoMounter;
}

void CDlcInstallerOrbis::OnEntitlementUpdated()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only" );

	m_packageAutoMounter->RequestRefresh();
}

Bool CDlcInstallerOrbis::IsReady() const 
{
	return !m_packageAutoMounter->IsDirty();
}

void CDlcInstallerOrbis::Update( TDynArray< SContentPackageEvent >& outContentPackageEvents )
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

				// Mount paths from the system don't have a trailing slash. E.g., "/addcont0"
				String mountPath = String::Printf(TXT("%hs/content/"), pkgInfo.m_mountPoint.AsChar());
				mountPath.ReplaceAll( TXT('/'), TXT('\\') );
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
