/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/*
NOTE: Perhaps no longer a bug but something to test if unmounting packages at some point in the future:
https://forums.xboxlive.com/AnswerPage.aspx?qid=9a67c3d7-f263-4be0-8a1f-a30ee045a5f8&tgt=1

My title is unable to re-mount a package that it had mounted once but then unmounted later. The error I get back says the package is already mounted. The only way to fix this is to terminate the title and re-launch it. What would be causing this?

Using the LicenseTerminated event can cause this behavior. It can hang onto an internal ref count. We currently do not recommend using this event. Instead call CheckLicense on the DLC packages at these locations for a loss of license:

User sign out 
Title regains focus 
Prior to a mount 
to ensure the license state of your DLC.

Other links:
https://forums.xboxlive.com/AnswerPage.aspx?qid=f2180a68-614e-4f72-8b89-d5d3bdb08a21&tgt=1
https://forums.xboxlive.com/AnswerPage.aspx?qid=0196e984-5c8c-42ae-9985-accfdcc4bbbe&tgt=1
https://forums.xboxlive.com/AnswerPage.aspx?qid=4b65a787-4c04-4de0-acfa-60df73ef79b0&tgt=1
*/

#include "../../common/core/bitset.h"
#include "../../common/core/contentManifest.h"
#include "../../common/engine/baseEngine.h"

#include <collection.h>
#include <ppltasks.h>

namespace xb
{
	using namespace ::Platform;
	using namespace ::Platform::Collections;
	using namespace ::Windows::Foundation;
	using namespace ::Windows::Foundation::Collections;
	using namespace ::Windows::Xbox::Management::Deployment;
	using namespace ::Windows::Xbox::System;
}

namespace Config
{
	// If the game isn't active, refresh more aggressively on error in order to allow the player into the game faster if needed
	TConfigVar< Float, Validation::FloatRange<0,60,1> > cvRefreshErrorRetryMainMenu( "DLC/XboxOne", "RefreshErrorRetryMainMenu", 1.f, eConsoleVarFlag_ReadOnly );
	TConfigVar< Float, Validation::FloatRange<0,60,1> > cvRefreshErrorRetryGame( "DLC/XboxOne", "RefreshErrorRetryGame", 30.f, eConsoleVarFlag_ReadOnly );
}

namespace Helper
{

#ifdef RED_LOGGING_ENABLED
static String ToLogString( xb::IDownloadableContentPackage^ package )
{
	if ( !package )
	{
		return TXT("[IDownloadableContentPackage <null>]");
	}

	return String::Printf(TXT("[IDownloadableContentPackage DisplayName='%ls', ContentId=%ls, ProductId=%ls]"),
		package->DisplayName->Data(),
		package->ContentId->Data(),
		package->ProductId->Data());
}
#endif // RED_LOGGING_ENABLED

const Uint32 MAX_DLC_PACKAGES = 64;

//////////////////////////////////////////////////////////////////////////
// CDlcPackageAutoMounter
//////////////////////////////////////////////////////////////////////////
class CDlcPackageAutoMounter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CDlcPackageAutoMounter();
	~CDlcPackageAutoMounter();
	void Init( const TDynArray< String >& blacklistProductIDs );
	void Update();
	Bool FlushPackageInfos( TDynArray< SPackageInfo >& outPackageInfos );
	Bool IsDirty() const { return m_refreshInProgress || m_needsRefresh.GetValue(); }
	void RequestRefresh() { m_needsRefresh.SetValue( true ); }

public:
	void OnResume();

private:
	enum EMountResult
	{
		eMountResult_Success,
		eMountResult_Fail_InvalidPackage,
		eMountResult_Fail_NoLicense,
		eMountResult_Fail_LicenseCheckError,
		eMountResult_Fail_MountError,
		eMountResult_Fail_Unknown,
	};

#ifdef RED_LOGGING_ENABLED
	static const Char* MountResultTxtForLog( EMountResult mountResult );
#endif

private:
	struct MountPackagesResult
	{
		MountPackagesResult()
			: m_installedPackages( ref new xb::Vector<xb::IDownloadableContentPackage^> )
			, m_hasError( false )
		{}

		MountPackagesResult( xb::Vector<xb::IDownloadableContentPackage^>^ installedPackages )
			: m_installedPackages( installedPackages )
			, m_hasError( false )
		{
			RED_FATAL_ASSERT( installedPackages, "Null installed packages!" );
		}

		typedef TBitSet64<MAX_DLC_PACKAGES>				TLicenseMask;

		xb::Vector<xb::IDownloadableContentPackage^>^	m_installedPackages; // Use ->IsMounted() to check mount status
		TLicenseMask									m_licenseMask;
		Bool											m_hasError;
	};

private:

	// Mounts new licensed packages, and keeps previously mounted packages whether currently licensed or not
	concurrency::task<MountPackagesResult>	MountPackagesAsync( xb::Vector<xb::IDownloadableContentPackage^>^ currentPackages );

private:
	void			SetupContentAvailableEventHandlers();
	void			ClearContentAvailableEventHandlers();

private:
	void			RefreshIfNeeded();
	void			CreatePackageInfos( const MountPackagesResult& mountPackagesResult );

private:
	void			AppendNewInstalledPackages( xb::Vector<xb::IDownloadableContentPackage^>^ currentPackages );
	EMountResult	TryMountPackage( xb::IDownloadableContentPackage^ package );
	Bool			IsBlacklisted( xb::IDownloadableContentPackage^ package ) const;

private:
	typedef xb::TypedEventHandler< xb::IDownloadableContentPackage^, xb::ILicenseTerminatedEventArgs^ > LicenseTerminatedEventHandler;

private:
	xb::EventRegistrationToken						m_eventToken_signInCompleted;
	xb::EventRegistrationToken						m_eventToken_downloadableContentPackageInstallCompleted;
	LicenseTerminatedEventHandler^					m_licenseTerminatedEventHandler;
	xb::IDownloadableContentPackageManager^			m_dlcPackageManager;
	xb::Vector<xb::IDownloadableContentPackage^>^	m_installedPackages;
	THashMap< String, RuntimePackageID >			m_contentPackageIDMap;
	TDynArray< String >								m_blacklistProductIDs;
	TDynArray< SPackageInfo >						m_packageInfos;
	EngineTime										m_lastRefreshTime;
	Red::Threads::CAtomic< Bool >					m_needsRefresh;
	Bool											m_refreshInProgress;
	Bool											m_refreshError;
	Bool											m_hasNewPackageInfos;
};

CDlcPackageAutoMounter::CDlcPackageAutoMounter()
	: m_dlcPackageManager( nullptr )
	, m_installedPackages( nullptr )
	, m_needsRefresh( false )
	, m_refreshInProgress( false )
	, m_refreshError( false )
	, m_hasNewPackageInfos( false )
{
}

CDlcPackageAutoMounter::~CDlcPackageAutoMounter()
{
	extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;

	while ( m_refreshInProgress )
	{
		GCoreDispatcher->ProcessEvents( Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent );
	}

	ClearContentAvailableEventHandlers();
}

void CDlcPackageAutoMounter::Init( const TDynArray< String >& blacklistProductIDs )
{
	m_installedPackages = ref new xb::Vector<xb::IDownloadableContentPackage^>;

	m_blacklistProductIDs = blacklistProductIDs;

	m_dlcPackageManager = ref new xb::DownloadableContentPackageManager;
	m_licenseTerminatedEventHandler = ref new LicenseTerminatedEventHandler(
		[this]( xb::IDownloadableContentPackage^ eventPackage, xb::ILicenseTerminatedEventArgs^ args )
		{
#ifdef RED_LOGGING_ENABLED
			LOG_ENGINE(TXT("LicenseTerminated: %ls UserXuidIfCausedBySignout=%ls"),
				ToLogString( eventPackage ).AsChar(),
				args->UserXuidIfCausedBySignout->Data());
#endif

			m_needsRefresh.SetValue( true );
		} );

	SetupContentAvailableEventHandlers();
}

void CDlcPackageAutoMounter::SetupContentAvailableEventHandlers()
{
	m_eventToken_downloadableContentPackageInstallCompleted = m_dlcPackageManager->DownloadableContentPackageInstallCompleted += ref new xb::DownloadableContentPackageInstallCompletedEventHandler(
	[this]()
	{
		LOG_ENGINE(TXT("CDlcPackageAutoMounter[DownloadableContentPackageInstallCompleted]"));
		m_needsRefresh.SetValue( true );
	});

	m_eventToken_signInCompleted = xb::User::SignInCompleted += ref new xb::EventHandler<xb::SignInCompletedEventArgs^>(
	[this](xb::Object^, xb::SignInCompletedEventArgs^ args)
	{
		LOG_ENGINE(TXT("CDlcPackageAutoMounter[SignInCompleted]"));
		m_needsRefresh.SetValue( true );
	});
}

void CDlcPackageAutoMounter::ClearContentAvailableEventHandlers()
{
	m_dlcPackageManager->DownloadableContentPackageInstallCompleted -= m_eventToken_downloadableContentPackageInstallCompleted;
	xb::User::SignInCompleted -= m_eventToken_signInCompleted;

	m_eventToken_downloadableContentPackageInstallCompleted = xb::EventRegistrationToken();
	m_eventToken_signInCompleted = xb::EventRegistrationToken();
}

void CDlcPackageAutoMounter::AppendNewInstalledPackages( xb::Vector<xb::IDownloadableContentPackage^>^ currentPackages )
{
	CTimeCounter timeCounter;

	// FindPackages returns new package objects that don't carry mount state, 
	// so need to enumerate and find new packages

	try
	{
		xb::IVectorView<xb::IDownloadableContentPackage^>^ vvPackages = m_dlcPackageManager->FindPackages( xb::InstalledPackagesFilter::AllDownloadableContentOnly );
		LOG_ENGINE(TXT("AppendNewInstalledPackagesAsync: enumerated %u packages"), vvPackages->Size );

		xb::Array<xb::IDownloadableContentPackage^>^ arPackages = ref new xb::Array<xb::IDownloadableContentPackage^>( vvPackages->Size );
		vvPackages->GetMany( 0, arPackages );
		
		Uint32 origLen = currentPackages->Size;
		for ( Uint32 i = 0, len = arPackages->Length; i < len; ++i )
		{
			xb::IDownloadableContentPackage^ tmpPackage = arPackages[ i ];
			LOG_ENGINE(TXT("AppendNewInstalledPackagesAsync: %ls"), ToLogString( tmpPackage ).AsChar());

			if ( IsBlacklisted( tmpPackage ) )
			{
				LOG_ENGINE(TXT("AppendNewInstalledPackagesAsync: ignoring blacklisted DLC: %ls"), ToLogString( tmpPackage ).AsChar());

				continue;
			}

			Bool shouldAppendPackage = true;
			for ( Uint32 j = 0; j < origLen; ++j )
			{
				xb::IDownloadableContentPackage^ curPkg = currentPackages->GetAt( j );
				if ( tmpPackage->ContentId->Equals( curPkg->ContentId ) )
				{
					shouldAppendPackage = false;
					break;
				}
			}

			if ( shouldAppendPackage )
			{
				if ( currentPackages->Size >= MAX_DLC_PACKAGES )
				{
					WARN_ENGINE(TXT("AppendNewInstalledPackagesAsync: exceeded %u supported DLC packages. Skipping %ls"), MAX_DLC_PACKAGES, ToLogString( tmpPackage ).AsChar());
					continue;
				}

				LOG_ENGINE(TXT("AppendNewInstalledPackagesAsync: appending new package %ls"), ToLogString( tmpPackage ).AsChar());
				currentPackages->Append( tmpPackage );
			}
		}
	}
	catch ( xb::Exception^ ex )
	{
		ERR_ENGINE( TXT("AppendNewInstalledPackagesAsync unxpected exception HRESULT: 0x%08X\n"), ex->HResult );
		throw;
	}

	LOG_ENGINE(TXT("AppendNewInstalledPackagesAsync completed in %1.2f sec"), timeCounter.GetTimePeriod());
}

Bool CDlcPackageAutoMounter::IsBlacklisted( xb::IDownloadableContentPackage^ package ) const
{
	// Thread safe in so far as blacklistProductIDs isn't modified past init stage
	for ( const String& productID : m_blacklistProductIDs )
	{
		const Bool match = ( Red::System::StringCompareNoCase( package->ProductId->Data(), productID.AsChar() ) == 0 );
		if ( match )
		{
			return true;
		}
	}

	return false;
}

#ifdef RED_LOGGING_ENABLED
const Char* CDlcPackageAutoMounter::MountResultTxtForLog( EMountResult mountResult )
{
	const Char* txt = TXT("<Unknown>");
	switch (mountResult)
	{
	case eMountResult_Fail_InvalidPackage:
		txt = TXT("eMountResult_Fail_InvalidPackage");
		break;
	case eMountResult_Fail_NoLicense:
		txt = TXT("eMountResult_Fail_NoLicense");
		break;
	case eMountResult_Fail_LicenseCheckError:
		txt = TXT("eMountResult_Fail_LicenseCheckError");
		break;
	case eMountResult_Fail_MountError:
		txt = TXT("eMountResult_Fail_MountError");
		break;
	case eMountResult_Success:
		txt = TXT("eMountResult_Success");
		break;
	default:
		break;
	}

	return txt;
};
#endif // RED_LOGGING_ENABLED

concurrency::task<CDlcPackageAutoMounter::MountPackagesResult> CDlcPackageAutoMounter::MountPackagesAsync( xb::Vector<xb::IDownloadableContentPackage^>^ currentPackages )
{
	return concurrency::create_task([this, currentPackages]()->MountPackagesResult{

		AppendNewInstalledPackages( currentPackages );

		CTimeCounter mountTimer;
		RED_ASSERT( !::SIsMainThread(), TXT("Performance issue: MountPackagesAsync running on main thread?!") );

		RED_FATAL_ASSERT( currentPackages->Size <= MAX_DLC_PACKAGES, "There are more DLC packages to mount than supported! %u/%u", currentPackages->Size, MAX_DLC_PACKAGES);

		MountPackagesResult asyncResult( currentPackages );
		
		try
		{
			const auto& items = asyncResult.m_installedPackages;
			auto& licenseMask = asyncResult.m_licenseMask;

			for ( Uint32 pkgIndex = 0, len = items->Size; pkgIndex < len; ++pkgIndex )
			{		
				xb::IDownloadableContentPackage^ package = items->GetAt( pkgIndex );

				const EMountResult mountResult = TryMountPackage( package );
				switch( mountResult	)
				{
				case eMountResult_Fail_NoLicense:
					{
						licenseMask.Clear( pkgIndex );
					}
					break;
				case eMountResult_Fail_InvalidPackage:		/* fall through */
				case eMountResult_Fail_LicenseCheckError:	/* fall through */
				case eMountResult_Fail_MountError:			/* fall through */
					{
						LOG_ENGINE(TXT("MountPackagesAsync: failed to mount %ls, eMountResult=%u (%ls)"), 
							 ToLogString( package ).AsChar(), mountResult, MountResultTxtForLog(mountResult) );
						licenseMask.Clear( pkgIndex );
						asyncResult.m_hasError = true;
					}
					break;
				case eMountResult_Success:
					{
						LOG_ENGINE(TXT("MountPackagesAsync: mounted/relicensed %ls"), ToLogString( package ).AsChar());

						// If we get LicenseTerminated a subsequent refresh will correct the licensing state
						licenseMask.Set( pkgIndex );
					}
					break;
				default:
					{
						RED_FATAL( "MountPackagesAsync: unexpected error %ls, eMountResult=%u (%ls)",
							ToLogString( package ).AsChar(), mountResult, MountResultTxtForLog(mountResult) );
						licenseMask.Clear( pkgIndex );
						asyncResult.m_hasError = true;
					}
					break;
				}
			}
		}
		catch( xb::Exception^ ex )
		{
			ERR_ENGINE( TXT("MountPackagesAsync unexpected exception HRESULT: 0x%08X\n"), ex->HResult );
			throw;
		}

		LOG_ENGINE(TXT("MountPackagesAsync mounting/licensing completed in %1.2f sec"), mountTimer.GetTimePeriod());

		return asyncResult;
	});
}

CDlcPackageAutoMounter::EMountResult CDlcPackageAutoMounter::TryMountPackage( xb::IDownloadableContentPackage^ package )
{
	EMountResult result = eMountResult_Fail_MountError;

	if ( !package )
	{
		return eMountResult_Fail_InvalidPackage;
	}

	try
	{
		Bool isTrial = false;
		if ( !package->CheckLicense(&isTrial) )
		{
			return eMountResult_Fail_NoLicense;
		}
	}
	catch ( xb::Exception^ ex )
	{
		ERR_ENGINE( TXT("TryMountPackage CheckLicense exception HRESULT: 0x%08X\n"), ex->HResult );
		return eMountResult_Fail_LicenseCheckError;
	}

	// If already mounted, keep mounted.
	try
	{
		if ( package->IsMounted )
		{
			return eMountResult_Success;
		}
	}
	catch ( xb::Exception^ ex )
	{
		ERR_ENGINE( TXT("TryMountPackage unexpected exception HRESULT: 0x%08X\n"), ex->HResult );
		return eMountResult_Fail_MountError;
	}

	xb::EventRegistrationToken token = xb::EventRegistrationToken();
	try
	{
		token = package->LicenseTerminated += m_licenseTerminatedEventHandler;

		xb::String^ mountPoint = package->Mount();
		if ( mountPoint == nullptr || mountPoint == TXT("") )
		{
			return eMountResult_Fail_MountError;
		}

		result = package->IsMounted ? eMountResult_Success : eMountResult_Fail_MountError;
	}
	catch ( xb::Exception^ ex )
	{
		ERR_ENGINE( TXT("TryMountPackage exception HRESULT: 0x%08X\n"), ex->HResult );

		package->LicenseTerminated -= token;
		return eMountResult_Fail_MountError;
	}

	return result;
}

void CDlcPackageAutoMounter::Update()
{
	RefreshIfNeeded();
}

Bool CDlcPackageAutoMounter::FlushPackageInfos( TDynArray< SPackageInfo >& outPackageInfos )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!" );

	if ( m_hasNewPackageInfos )
	{
		m_hasNewPackageInfos = false;
		outPackageInfos = Move( m_packageInfos );
		return true;
	}

	return false;
}

void CDlcPackageAutoMounter::RefreshIfNeeded()
{
	if ( m_refreshError )
	{
		const Float interval = ( GGame && GGame->IsActive() ) ? Config::cvRefreshErrorRetryGame.Get() : Config::cvRefreshErrorRetryMainMenu.Get();
		if ( GEngine && ( GEngine->GetRawEngineTime() - m_lastRefreshTime ) > interval )
		{
			LOG_ENGINE(TXT("CDlcPackageAutoMounter::RefreshIfNeeded refreshError recovery after %1.2f sec"), interval);
			m_refreshError = false;
			m_needsRefresh.SetValue( true );
		}
	}

	if ( !m_refreshInProgress && m_needsRefresh.Exchange( false ) )
	{
		RED_FATAL_ASSERT(::SIsMainThread(), "Main thread only!");

		RED_FATAL_ASSERT( m_installedPackages, "m_installedPackages packages should not be null (vs empty)");

		auto oldInstalledPackages = m_installedPackages;
		m_installedPackages = nullptr; // The task owns it during refresh
		m_refreshInProgress = true;

		MountPackagesAsync( oldInstalledPackages ).
		then([this,oldInstalledPackages](concurrency::task<MountPackagesResult> t)
		{
			extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;
			GCoreDispatcher->RunAsync( Windows::UI::Core::CoreDispatcherPriority::Normal, 
				ref new Windows::UI::Core::DispatchedHandler([=]()
				{
					RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!");

					try
					{
						const auto& mountPackagesResult = t.get();
						m_installedPackages = mountPackagesResult.m_installedPackages;
						m_refreshError = mountPackagesResult.m_hasError;
						CreatePackageInfos( mountPackagesResult );
					}
					catch ( xb::Exception^ ex )
					{
						ERR_ENGINE( TXT("RefreshIfNeeded unexpected exception HRESULT: 0x%08X\n"), ex->HResult );
						m_installedPackages = oldInstalledPackages; // it's OK to keep any newly appended packages
						m_refreshError = true;
					}

					m_refreshInProgress = false;
					RED_FATAL_ASSERT( m_installedPackages, "InstalledPackages packages should not be null (vs empty)");

					// Maybe the network was out and couldn't check all package licenses to mount them -- who knows. Need to try refreshing until successful at least once.
					m_lastRefreshTime = EngineTime::GetNow();
					LOG_ENGINE(TXT("CDlcPackageAutoMounter::RefreshIfNeeded refresh completed m_refreshError=%ls"), (m_refreshError ? TXT("[y]") : TXT("[n]")));
				}));
		});
	}
}

void CDlcPackageAutoMounter::CreatePackageInfos( const MountPackagesResult& mountPackagesResult )
{
	RED_FATAL_ASSERT( ::SIsMainThread() && m_refreshInProgress, "Main thread only and when updating infos!");

	m_packageInfos.ClearFast();

	const MountPackagesResult::TLicenseMask& licenseMask = mountPackagesResult.m_licenseMask;
	auto installedPackages = mountPackagesResult.m_installedPackages;

	for ( Uint32 i = 0, len = installedPackages->Size; i < len; ++i )
	{
		xb::IDownloadableContentPackage^ package = installedPackages->GetAt( i );
		if ( !package || !package->IsMounted )
		{
			continue;
		}

		const String contentID = package->ContentId->Data();
		RuntimePackageID packageID = INVALID_RUNTIME_PACKAGE_ID;
		if ( !m_contentPackageIDMap.Find( contentID, packageID ) )
		{
			packageID = Helper::AllocRuntimePackageID();
			m_contentPackageIDMap.Insert( contentID, packageID );
		}

		const String mountPoint = package->MountPath->Data();
		const Bool isLicensed = licenseMask.Get(i); // if we get a LicenseTerminated event, we'll send a package event to correct this
		const SPackageInfo packageInfo( packageID, mountPoint, isLicensed );
		m_packageInfos.PushBack( Move( packageInfo ) );
	}

	m_hasNewPackageInfos = true;
}

} // namespace Helper