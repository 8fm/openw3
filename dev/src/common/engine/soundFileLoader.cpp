/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "soundSystem.h"
#include "soundFileLoader.h"
#include "soundSettings.h"

#ifdef USE_WWISE
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include "AK/SoundEngine/Common/AkStreamMgrModule.h"
#endif
#include "../../common/core/gatheredResource.h"
#include "../core/jobGenericJobs.h"
#include "../core/loadingJobManager.h"
#include "../core/depot.h"
#include "../core/2darray.h"
#include "../core/loadingJob.h"
#include "../redIO/redIOCommon.h"
#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"
#include "../core/lazyCacheChain.h"
#include "../core/contentManifest.h"
#include "soundCacheDataFormat.h"
#include "../redSystem/clock.h"
#include "../core/loadingProfiler.h"
#include "../core/ioTags.h"

//////////////////////////////////////////////////////////////////////////

CGatheredResource resAlwaysLoadedSoundBankNames(  TXT( "gameplay\\globals\\soundbanks_always_load.csv" ), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

class CSoundBanksWhileInitTask : public ILoadJob
{
protected:
	Red::Threads::CAtomic< Bool >*	m_whileInit;
	TDynArray< StringAnsi > m_banksNames;

	virtual EJobResult Process() override
	{
		RED_LOG("SoundFileLoader", TXT("CSoundBanksWhileInitTask - Started processing bank names - count [%i]"), m_banksNames.Size());

		for ( Uint32 index = 0; index < m_banksNames.Size(); ++index )
		{
			String name = ANSI_TO_UNICODE( m_banksNames[ index ].AsChar() );

			Uint32 size = 0;

			if( GSoundCacheResolver::GetInstance().IsInitialized() )
			{
				CacheTokenToSoundCachePair cacheFindResult = GSoundCacheResolver::GetInstance().GetSoundResource( name );
				if( cacheFindResult.IsSet() == true )
				{
					size = (Uint32)cacheFindResult.m_token->m_dataSize;
				}
				else
				{
					continue;
				}
			}
			else
			{
				String path = GSoundSystem->GetDepotBankPath();

				path += name;

				CDiskFile* diskFile = GDepot->FindFile( path.AsChar() );

				IFile* file = diskFile->CreateReader();
				if( file)
				{
					size = ( Uint32 )file->GetSize();
				}
				else
				{
					continue;
				}
			}


			CSoundBank::m_soundBanks.PushBack( CSoundBank( CName( name.ToLower() ), size ) );
		}

		RED_LOG("SoundFileLoader", TXT("CSoundBanksWhileInitTask - Starting loading always loaded banks."));

		C2dArray* names = resAlwaysLoadedSoundBankNames.LoadAndGet< C2dArray >();
		if( names )
		{
			Uint32 count = static_cast<Uint32>( names->GetNumberOfRows() );
			RED_LOG("SoundFileLoader", TXT("CSoundBanksWhileInitTask - Always loaded banks count [%i]"), count);

			for ( Uint32 index = 0; index < count; ++index )
			{
				const String& fileName = names->GetValue( 0, index );
				if(fileName.GetLength() == 0)
				{
					continue;
				}
				CSoundBank* soundBank = CSoundBank::FindSoundBank( CName(fileName) );
				RED_ASSERT( soundBank != nullptr, TXT("Always loaded bank is not in the banks array - [%ls]!"), fileName.AsChar() );
				if( soundBank )
				{
					Bool retVal = soundBank->QueueLoading();
					RED_LOG("SoundFileLoader", TXT("CSoundBanksWhileInitTask - QueueLoading [%ls] - result [%i]"), fileName.AsChar(), retVal);
					RED_ASSERT( retVal, TXT("Always loaded bank %ls failed to schedule loading!"), fileName.AsChar() );
					soundBank->m_loadedAlways = true;
				}
			}
		}

		RED_LOG("SoundFileLoader", TXT("CSoundBanksWhileInitTask - Finished processing."));
		return JR_Finished;
	}

public:
	CSoundBanksWhileInitTask( Red::Threads::CAtomic< Bool >* whileInit )
		: ILoadJob(JP_Default, true /* block GC*/ )
		, m_whileInit( whileInit )
	{
		m_whileInit->SetValue( true );
	}

	~CSoundBanksWhileInitTask()
	{
		m_whileInit->SetValue( false );
	}

	TDynArray< StringAnsi >& GetBanksArray() { return m_banksNames; }

public:
	virtual const Char* GetDebugName() const override { return TXT( "CSoundBanksWhileInitTask" ); }
};

class CSoundBanksRefreshTask : public CTask
{
protected:
	TDynArray< StringAnsi > m_banksNames;
	TDynArray< CSoundBank > m_banks;

	void Run() override
	{
		PC_SCOPE_PIX( CSoundBanksRefreshTask )

		for ( Uint32 index = 0; index < m_banksNames.Size(); ++index )
		{
			String name = ANSI_TO_UNICODE( m_banksNames[ index ].AsChar() );

			Uint32 size = 0;

			if( GSoundCacheResolver::GetInstance().IsInitialized() )
			{
				CacheTokenToSoundCachePair cacheFindResult = GSoundCacheResolver::GetInstance().GetSoundResource( name );
				if( cacheFindResult.IsSet() == true )
				{
					size = (Uint32)cacheFindResult.m_token->m_dataSize;
				}
				else
				{
					continue;
				}
			}
			else
			{
				String path = GSoundSystem->GetDepotBankPath();

				path += name;

				CDiskFile* diskFile = GDepot->FindFile( path.AsChar() );

				IFile* file = diskFile->CreateReader();
				if( file)
				{
					size = ( Uint32 )file->GetSize();
				}
				else
				{
					continue;
				}
			}


			m_banks.PushBack( CSoundBank( CName( name.ToLower() ), size ) );
		}
	}

public:
	CSoundBanksRefreshTask( TDynArray< StringAnsi >& soundbanks ) : m_banksNames( soundbanks )
	{}

	const TDynArray< CSoundBank >& GetSoundBanks() { return m_banks; }

public:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT( "CSoundBanksRefreshTask" ); }
	virtual Uint32		GetDebugColor() const override { return Color::BLUE.ToUint32(); }
#endif

};

struct SAsyncReadTokenRefCounted : public Red::IO::SAsyncReadToken
{
	Red::Threads::CAtomic< void* > m_usement;
	SAsyncReadTokenRefCounted()
	{
		m_usement.SetValue( nullptr );
	}
};

class CSoundFileLoaderHook 
#ifdef USE_WWISE
	: public AK::StreamMgr::IAkFileLocationResolver, public AK::StreamMgr::IAkIOHookDeferred
#endif
{
	friend class CSoundFileLoader;
	friend class CSoundBank;
protected:

	TList< SAsyncReadTokenRefCounted* > m_tokensReuse;

	CSoundFileLoaderHook() {}
	~CSoundFileLoaderHook()
	{
		for( auto element = m_tokensReuse.Begin(); element != m_tokensReuse.End(); ++element )
		{
			delete *element;
		}
		m_tokensReuse.Clear();
	}

#ifdef USE_WWISE
	AKRESULT Open( const AkOSChar* in_pszFileName, AkOpenMode in_eOpenMode, AkFileSystemFlags* in_pFlags, bool& io_bSyncOpen, AkFileDesc& out_fileDesc )
	{
		if( in_eOpenMode != AK_OpenModeRead )
		{
			return AK_Fail;
		}

		Red::System::MemorySet( &out_fileDesc, 0, sizeof( out_fileDesc ) );

		// By leaving io_bSyncOpen false, it will attempt to load it from the device I/O thread instead of e.g., AkAudioThread
		if ( !io_bSyncOpen )
		{
			return AK_Success;
		}

		Red::IO::CAsyncFileHandleCache::TFileHandle handle = Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE;
		String fileNameWithPath;
#ifdef RED_PLATFORM_ORBIS
		fileNameWithPath += ANSI_TO_UNICODE( in_pszFileName );
#else
		fileNameWithPath += in_pszFileName;
#endif


		if( Red::System::StringCompare( fileNameWithPath.ToLower().AsChar(), TXT("init.bnk") ) == 0 )
		{
			handle = Red::IO::GAsyncIO.OpenFile( CSoundBank::s_initBankPath.AsChar(), Red::IO::eAsyncFlag_TryCloseFileWhenNotUsed );
			out_fileDesc.iFileSize = GFileManager->GetFileSize( CSoundBank::s_initBankPath );
			RED_FATAL_ASSERT( out_fileDesc.iFileSize > 0 , "Init.bnk file size == 0 !" );
			RED_FATAL_ASSERT( handle != Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE, "Init.bnk file handle is invalid !" );
		}
		else
		{
			CacheTokenToSoundCachePair cacheFindResult = GSoundCacheResolver::GetInstance().GetSoundResource( fileNameWithPath );
			if( cacheFindResult.IsSet() == true )
			{
				handle = cacheFindResult.m_cache->GetFile();
				out_fileDesc.uSector = (Uint32)cacheFindResult.m_token->m_dataOffset;
				out_fileDesc.iFileSize = cacheFindResult.m_token->m_dataSize;
			}
			else
			{
				CDiskFile* diskFile = GDepot->FindFile( GSoundSystem->GetDepotBankPath() + fileNameWithPath );
				if( diskFile )
				{
					String absolutePath;
					GDepot->GetAbsolutePath( absolutePath );
					absolutePath += GSoundSystem->GetDepotBankPath();
					fileNameWithPath = absolutePath + fileNameWithPath;
					handle = Red::IO::GAsyncIO.OpenFile( fileNameWithPath.AsChar(), Red::IO::eAsyncFlag_TryCloseFileWhenNotUsed );

					IFile* temporaryReaded = diskFile->CreateReader();
					out_fileDesc.iFileSize = temporaryReaded->GetSize();
					delete temporaryReaded;

				}
			}
		}
		if( handle == Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
			return AK_Fail;

		out_fileDesc.pCustomParam = reinterpret_cast< void* >( handle );
		out_fileDesc.uCustomParamSize = in_pFlags->bIsAutomaticStream;
		return AK_Success;
	}

	AKRESULT Open( AkFileID in_fileID, AkOpenMode in_eOpenMode, AkFileSystemFlags* in_pFlags, bool& io_bSyncOpen, AkFileDesc& out_fileDesc )
	{
#ifdef RED_PLATFORM_ORBIS
		StringAnsi name = StringAnsi::Printf( "%i.wem", in_fileID );
#else
		String name = String::Printf( TXT( "%i.wem" ), in_fileID );
#endif
		return Open( name.AsChar(), in_eOpenMode, in_pFlags, io_bSyncOpen, out_fileDesc );
	}

	AKRESULT Close( AkFileDesc& in_fileDesc )
	{
		if( in_fileDesc.uSector )
		{
			//file is cached so end here
			return AK_Success;
		}

#ifdef RED_PLATFORM_ORBIS
		Red::IO::CAsyncFileHandleCache::TFileHandle handle = ( Uint64 ) in_fileDesc.pCustomParam;
#else
		Red::IO::CAsyncFileHandleCache::TFileHandle handle = ( Uint32 ) in_fileDesc.pCustomParam;
#endif

		Red::IO::GAsyncIO.ReleaseFile( handle );

		return AK_Success;
	}

	AkUInt32 GetBlockSize( AkFileDesc& in_fileDesc )
	{
		return 1;
	}

	void GetDeviceDesc( AkDeviceDesc& out_deviceDesc )
	{

	}

	AkUInt32 GetDeviceData()
	{
		return 1;
	}

	static Red::IO::ECallbackRequest aioCallback( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
	{
		SAsyncReadTokenRefCounted* token = ( SAsyncReadTokenRefCounted* ) asyncReadToken.m_userData;
		AkAsyncIOTransferInfo* io_transferInfo = ( AkAsyncIOTransferInfo* ) token->m_usement.GetValue();
		if( io_transferInfo )
		{
			io_transferInfo->pCallback( io_transferInfo, AK_Success );
		}
		token->m_usement.SetValue( nullptr );
		return Red::IO::eCallbackRequest_Finish;
	}

	AKRESULT Read( AkFileDesc& in_fileDesc, const AkIoHeuristics& in_heuristics, AkAsyncIOTransferInfo& io_transferInfo )
	{
		if( !io_transferInfo.pBuffer )
			return AK_Success;
			
#ifdef RED_PLATFORM_ORBIS
		Red::IO::CAsyncFileHandleCache::TFileHandle handle = ( Uint64 ) in_fileDesc.pCustomParam;
#else
		Red::IO::CAsyncFileHandleCache::TFileHandle handle = ( Uint32 ) in_fileDesc.pCustomParam;
#endif

		SAsyncReadTokenRefCounted* token = nullptr;
		for( auto element = m_tokensReuse.Begin(); element != m_tokensReuse.End(); ++element )
		{
			if( ( *element )->m_usement.GetValue() != 0 ) continue;

			token = *element;
			m_tokensReuse.Erase( element );
			m_tokensReuse.PushBack( token );
			break;
		}
		if( !token )
		{
			token = new SAsyncReadTokenRefCounted();
			m_tokensReuse.PushBack( token );
		}

		token->m_buffer = io_transferInfo.pBuffer;
		token->m_numberOfBytesToRead = io_transferInfo.uRequestedSize;
		token->m_offset = io_transferInfo.uFilePosition;
		token->m_callback = &aioCallback;

		token->m_userData = token;
		token->m_usement.SetValue( &io_transferInfo );

		Red::IO::EAsyncPriority priority = in_fileDesc.uCustomParamSize ? Red::IO::eAsyncPriority_Critical : Red::IO::eAsyncPriority_Normal;
		Red::IO::GAsyncIO.BeginRead( handle, *token, priority, eIOTag_SoundImmediate );

		return AK_Success;
	}

	AKRESULT Write( AkFileDesc& in_fileDesc, const AkIoHeuristics& in_heuristics, AkAsyncIOTransferInfo& io_transferInfo )
	{
		return AK_Success;
	}
	void Cancel( AkFileDesc& in_fileDesc, AkAsyncIOTransferInfo& io_transferInfo, bool& io_bCancelAllTransfersForThisFile )
	{
		io_bCancelAllTransfersForThisFile = false;
	}
#endif

};

//////////////////////////////////////////////////////////////////////////
TDynArray< CSoundBank > CSoundBank::m_soundBanks;
Red::Threads::CAtomic< Int32 > CSoundBank::m_memoryAllocated;
Int32 CSoundBank::m_memoryAllocationLimit = 400 * 1024 * 1024;
CSoundBank CSoundBank::s_initBank;
String CSoundBank::s_initBankPath;

void CSoundBank::ClearBanks()
{
#ifdef USE_WWISE
	AKRESULT eResult = AK::SoundEngine::ClearBanks();
	CSoundBank::m_soundBanks.ClearFast();
	m_memoryAllocated.SetValue( 0 );
#endif
}

void CSoundBank::ReloadSoundbanks()
{
	for( auto i = m_soundBanks.Begin(); i != m_soundBanks.End(); ++i )
	{
		CSoundBank& bank = ( *i );
		if( bank.m_refCount.GetValue() == 0 ) continue;
		bank.Reload();
	}

	String path = GSoundSystem->GetDepotBankPath();

	CDirectory* directory = GDepot->FindPath( path.AsChar() );
	if( !directory ) return;
	directory->Repopulate();
	TFiles files = directory->GetFiles();

	for( auto i = files.Begin(); i != files.End(); ++i )
	{
		CDiskFile* diskFile = *i;
		String name = diskFile->GetFileName();
		if( !name.EndsWith( TXT( "bnk" ) ) ) continue;

		if( CSoundBank::FindSoundBank( CName( name ) ) ) continue;

		IFile* file = diskFile->CreateReader();
		if( file)
		{
			CSoundBank::m_soundBanks.PushBack( CSoundBank( CName( name.ToLower() ), ( Uint32 )file->GetSize() ) );
		}
	}
}

void CSoundBank::ShutDown()
{
	ClearBanks();
}

CSoundBank* CSoundBank::FindSoundBank( const CName& fileName )
{
	if( fileName == CName::NONE ) 
	{
		return 0;
	}
	Uint32 count = m_soundBanks.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		CSoundBank& soundBank = m_soundBanks[ i ];
		const CName& name = soundBank.GetFileName();

		if( soundBank.GetFileName() == fileName )
		{
			return &soundBank;
		}
	}

	return 0;
}

TDynArray< String > CSoundBank::GetAvaibleBanks()
{
	TDynArray< String > result;

	String path;
	GDepot->GetAbsolutePath( path );
	path += GSoundSystem->GetDepotBankPath();
	path += TXT( "*.bnk" );
	for ( CSystemFindFile findFile( path.AsChar() ); findFile; ++findFile )
		if ( !findFile.IsDirectory() )
		{
			const Char* temp = findFile.GetFileName();
			result.PushBack( findFile.GetFileName() );
		}
	return result;
}

Uint32 CSoundBank::GetSoundBanksCount()
{
	return m_soundBanks.Size();
}

const CSoundBank& CSoundBank::GetSoundBank(Uint32 index)
{
	return m_soundBanks[index];
}

#ifdef USE_WWISE
void BankLoadCallback(	AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId,	void *in_pCookie )
{
	Red::Threads::CAtomic< Int32 >* atomic = ( Red::Threads::CAtomic< Int32 >* ) in_pCookie;
	if( atomic )
	{
		atomic->SetValue( in_eLoadResult );
	}
}
#endif

Bool CSoundBank::QueueLoading()
{
	m_refCount.Increment();
	if( m_refCount.GetValue() > 1 ) return true;

	if( m_memoryAllocated.GetValue() + ( Int32 ) m_size > m_memoryAllocationLimit )
	{
		m_refCount.Decrement();
		return false;
	}
#ifdef USE_WWISE
	if( m_loadedAlways )
	{
		return true;
	}

	AKRESULT eResult = AK::SoundEngine::LoadBank( UNICODE_TO_ANSI( m_fileName.AsChar() ), BankLoadCallback, &m_isLoaded, AK_DEFAULT_POOL_ID, ( AkBankID& ) m_bankId );
	if( eResult == AK_Success )
	{
		m_memoryAllocated.ExchangeAdd( m_size );
		return true;
	}
	else return false;
#else
	return false;
#endif
}

#ifdef USE_WWISE
void BankUnloadCallback( AkUInt32 in_bankID, const void* in_pInMemoryBankPtr, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie )
{
}
#endif

Bool CSoundBank::Unload()
{
	if( m_refCount.GetValue() == 0 ) return false;

	m_refCount.Decrement();

	if( m_refCount.GetValue() > 0 ) return true;

	if( m_loadedAlways )
	{
		return false;
	}

#ifdef USE_WWISE
	AKRESULT eResult = AK::SoundEngine::UnloadBank( UNICODE_TO_ANSI( m_fileName.AsChar() ), nullptr, BankUnloadCallback, nullptr );

	m_isLoaded.SetValue( -1 );
	if( eResult == AK_Success )
	{
		m_memoryAllocated.ExchangeAdd( -(Int32)m_size );
		return true;
	}
#endif

	return true;
}

Bool CSoundBank::Reload()
{
#ifdef USE_WWISE
	AKRESULT eResult = AK::SoundEngine::UnloadBank( UNICODE_TO_ANSI( m_fileName.AsChar() ), NULL );
	if( eResult != AK_Success ) return false;

	eResult = AK::SoundEngine::LoadBank( UNICODE_TO_ANSI( m_fileName.AsChar() ), AK_DEFAULT_POOL_ID, ( AkBankID& ) m_bankId );
	return eResult == AK_Success;
#else
	return false;
#endif
}

Uint32 CSoundBank::GetRefCount() const
{
	return m_refCount.GetValue();
}

Uint32 CSoundBank::GetSize() const
{
	return m_size;
}

Bool CSoundBank::IsLoaded()
{
	return m_isLoaded.GetValue() == AK_Success || m_isLoaded.GetValue() == AKRESULT::AK_BankAlreadyLoaded;
}

String CSoundBank::GetLoadingResultString()
{
	switch( m_isLoaded.GetValue() )
	{
	case -1:
		return TXT( "Not loaded" );
	case AK_Success:
		return TXT( "AK_Success" );
	case AK_Fail:
		return TXT( "AK_Fail" );
	case AK_InvalidParameter:
		return TXT( "AK_InvalidParameter" );
	case AK_InvalidFile:
		return TXT( "AK_InvalidFile" );
	case AK_WrongBankVersion:
		return TXT( "AK_WrongBankVersion" );
	case AK_BankReadError:
		return TXT( "AK_BankReadError" );
	case AK_InsufficientMemory:
		return TXT( "AK_InsufficientMemory" );
	default:
		return String::Printf( TXT( "Unexpected result: [%i]" ), m_isLoaded.GetValue() );
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CSoundFileLoader::m_refreshSounbanks = false;

void CSoundFileLoader::Reset()
{
#ifdef USE_WWISE
	while( m_whileInit.GetValue() )
	{}

	for ( Uint32 index = 0; index < CSoundBank::m_soundBanks.Size(); ++index )
	{
		CSoundBank* soundBank = &CSoundBank::m_soundBanks[ index ];
		if( !soundBank ) continue;

		if( !soundBank->m_loadedAlways ) continue;
		while( !soundBank->IsLoadingFinished() ) {}
		RED_ASSERT( soundBank->IsLoaded(), TXT("CSoundFileLoader::Reset - sound bank didn't load properly - bank: [%ls] - result [%ls]"), 
			soundBank->GetFileName().AsChar(), soundBank->GetLoadingResultString().AsChar() );
	}

#endif
}

void CSoundFileLoader::PreInit( const String& rootPath )
{
	// Initialize loading crap
#ifdef USE_WWISE
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings( deviceSettings );
	deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP;
	deviceSettings.uMaxConcurrentIO = 16;
	deviceSettings.uGranularity = 4096 * 32;
	deviceSettings.uIOMemorySize = deviceSettings.uGranularity * deviceSettings.uMaxConcurrentIO * 2;
	
#ifdef RED_PLATFORM_DURANGO
	deviceSettings.threadProperties.dwAffinityMask = 1 << 4;
	deviceSettings.threadProperties.nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
#elif defined( RED_PLATFORM_ORBIS )
	deviceSettings.threadProperties.dwAffinityMask = 1 << 0 | 1 << 1;
	deviceSettings.threadProperties.nPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST;
#endif

	m_hook = new CSoundFileLoaderHook();

	if ( !AK::StreamMgr::GetFileLocationResolver() )
		AK::StreamMgr::SetFileLocationResolver( m_hook );

	// Create a device in the Stream Manager, specifying this as the hook.
	AkDeviceID m_deviceID = AK::StreamMgr::CreateDevice( deviceSettings, m_hook );
	RED_FATAL_ASSERT( m_deviceID == 0, "We zero out AKFileDesc, so this better be zero. Or just set it to this value!" );

	CSoundBank::ClearBanks();

	// Initialize wWise with init bank
	CSoundBank::s_initBankPath = rootPath + TXT("Init.bnk");
	Uint64 initBankSize = GFileManager->GetFileSize( CSoundBank::s_initBankPath );
	CSoundBank::s_initBank = CSoundBank( CName( TXT("Init.bnk") ), (Uint32)initBankSize );
	CSoundBank::s_initBank.QueueLoading();
	while( CSoundBank::s_initBank.IsLoaded() == false ) { /* Wait for it... waaait for it... */ };
#endif
}

void CSoundFileLoader::Init()
{
#ifdef USE_WWISE
	String path = GSoundSystem->GetDepotBankPath();
	
	CSoundBanksWhileInitTask* task = new CSoundBanksWhileInitTask( &m_whileInit );
	TDynArray<StringAnsi>& banks = task->GetBanksArray();


	if( !GSoundCacheResolver::GetInstance().IsEmpty() )
	{
		while( !GSoundCacheResolver::GetInstance().IsInitialized() )
		{};

		while( !GSoundCacheResolver::GetInstance().ListAllSoundBanks( banks ) )	
		{};
	}
	else
	{
		CDirectory* directory = GDepot->FindPath( path.AsChar() );
		if( !directory ) return;
		TFiles files = directory->GetFiles();

		for( auto i = files.Begin(); i != files.End(); ++i )
		{
			CDiskFile* diskFile = *i;
			String name = diskFile->GetFileName();
			if( !name.EndsWith( TXT( "bnk" ) ) ) continue;

			String lowerName = name.ToLower();
			if( Red::System::StringCompare( lowerName.AsChar(), TXT("init.bnk") ) == 0 )
			{
				continue;
			}

			banks.PushBack( UNICODE_TO_ANSI( lowerName.AsChar() ) );
		}
	}

#ifdef NO_EDITOR
	RED_ASSERT( !(GSoundCacheResolver::GetInstance().IsEmpty()), TXT("SoundCache is empty at this point - that's a big problem if we are launching game (and not, for instance, compiling scripts during cook).") );
#endif
	SJobManager::GetInstance().Issue( task );
	task->Release();

#endif

}

void CSoundFileLoader::Shutdown()
{
	TDynArray< IFile* > fileArray;
#ifdef USE_WWISE
	if( m_hook != nullptr )
	{
		delete m_hook;
		m_hook = nullptr;
	}

	if ( AK::IAkStreamMgr::Get() )
	{
		AK::IAkStreamMgr::Get()->Destroy();
	}
#endif
}

void CSoundFileLoader::FillFileList( CDirectory* dir, TDynArray< String >& fileList, const String& extension )
{
	// Get files from subdirectories
	for ( CDirectory* curDir : dir->GetDirectories() )
	{
		FillFileList( curDir, fileList, extension );
	}

	// Get files from current directory
	for ( CDiskFile* file : dir->GetFiles() )
	{
		CFilePath path( file->GetAbsolutePath() );
		if( !extension.Empty() && path.GetExtension() != extension ) continue;
		fileList.PushBack( path.ToString() );
	}
}

void CSoundFileLoader::SetRefreshSoundBanksFlag()
{
	m_refreshSounbanks = true;
}

void CSoundFileLoader::RefreshSoundBanks()
{
	PC_SCOPE_PIX(CSoundFileLoader RefreshSoundBanks)
	if( !m_refreshSounbanks ) return;
	if( !GSoundCacheResolver::GetInstance().IsInitialized() ) return;

	TDynArray< StringAnsi > soundbanks;
	if( !GSoundCacheResolver::GetInstance().ListAllSoundBanks( soundbanks ) )
	{
		// Hmm?
		return;
		m_refreshSounbanks = false;
	}
	if( m_task )
	{
		if( m_task->IsFinished() )
		{
			m_refreshSounbanks = false;
			for( auto i = m_task->GetSoundBanks().Begin(); i != m_task->GetSoundBanks().End(); ++i )
			{
				if( CSoundBank::FindSoundBank( i->GetFileName() ) ) continue;
				CSoundBank::m_soundBanks.PushBack( *i );
				m_refreshSounbanks = true;
			}
			m_task->Release();
			m_task = nullptr;
		}
	}
	else
	{
		m_task = new ( CTask::Root ) CSoundBanksRefreshTask( soundbanks );
		GTaskManager->Issue( *m_task, TSP_Critical );
	}
}

//////////////////////////////////////////////////////////////////////////

CSoundCache::CSoundCache()
	: m_cacheFile( Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	, m_isReady( false )
	, m_asyncLoader( nullptr )
{
	/* Intentionally Empty */
}

CSoundCache::~CSoundCache()
{
	if( m_asyncLoader != nullptr )
	{
		delete m_asyncLoader;
	}

	if( m_cacheFile != Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		Red::IO::GAsyncIO.ReleaseFile( m_cacheFile );
		m_cacheFile = Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE;
	}
}

const CSoundCacheData::CacheToken* CSoundCache::Find(const String& fileNameWithPath) const
{
	for( Uint32 i=0; i<m_soundCacheData.m_tokens.Size(); ++i )
	{
		auto& token = m_soundCacheData.m_tokens[i];
		String name = ANSI_TO_UNICODE( &m_soundCacheData.m_strings[ token.m_name ] );
		if( name == fileNameWithPath )
		{
			LOG_ENGINE( TXT("SoundResource: %ls"), name.AsChar() );
			return &m_soundCacheData.m_tokens[i];
		}
	}

	return nullptr;
}

void CSoundCache::ListAllSoundBanks(TDynArray<StringAnsi>& soundBanks)
{
	for( CSoundCacheData::CacheToken& token : m_soundCacheData.m_tokens )
	{
		StringAnsi name = &m_soundCacheData.m_strings[ token.m_name ];
		// Skip not banks
		if( !name.EndsWith( "bnk" ) ) continue;

		soundBanks.PushBack( name );
	}
}

Bool CSoundCache::IsReady() const
{
	return m_isReady;
}

CSoundCache* CSoundCache::ProcessCacheAsync(Red::IO::CAsyncFileHandleCache::TFileHandle sourceFile)
{
	if( !sourceFile ) return nullptr;

	CSoundCache* cache = new CSoundCache();
	cache->m_cacheFile = sourceFile;

	cache->m_asyncLoader = new CSoundCacheDataAsyncLoader( cache->m_soundCacheData );
	cache->m_asyncLoader->StartLoading( cache->m_cacheFile, &cache->m_isReady );

	return cache;
}

//////////////////////////////////////////////////////////////////////////

void CSoundCacheResolver::OnContentAvailable(const SContentInfo& contentInfo )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Mounting off the main thread" );

	typedef Bool (CacheChain::* PushFunc)(CSoundCache*);

	for ( const SContentFile* contentFile : contentInfo.m_contentFiles )
	{
		const PushFunc Push = contentFile->m_isPatch ? &CacheChain::PushFront : &CacheChain::PushBack;

		String cacheAbsolutePath = String::Printf( TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), contentFile->m_path.AsChar() );

		LOG_ENGINE(TXT("CSoundCacheResolver: attaching '%ls'"), cacheAbsolutePath.AsChar() );

		Red::IO::CAsyncFileHandleCache::TFileHandle fileReader = Red::IO::GAsyncIO.OpenFile( cacheAbsolutePath.AsChar(), Red::IO::eAsyncFlag_TryCloseFileWhenNotUsed );
		if( fileReader != 0 )
		{
			CSoundCache* cache = CSoundCache::ProcessCacheAsync( fileReader );
			if( cache != nullptr )
			{
				CSoundFileLoader::SetRefreshSoundBanksFlag();
				if ( ! (m_soundCacheChain.*Push)( cache ) )
				{
					delete cache;
					ERR_ENGINE(TXT("Failed to add sound cache '%ls'. Reached chain length limit %u"), cacheAbsolutePath.AsChar(), MAX_CACHE_CHAIN_LENGTH );
					return;
				}
				// We've added something to chain
				m_isEmpty = false;
			}
		}
	}
}

CacheTokenToSoundCachePair CSoundCacheResolver::GetSoundResource( const String& fileNameWithPath )
{
	PC_SCOPE_LV0( SoundResolverGetSound, PBC_CPU );
	CacheTokenToSoundCachePair result;
	m_soundResourceHashMap.Find( fileNameWithPath, result );

	// If not found in hash map - then find in caches and add to hash map
	if( result.IsSet() == false )
	{
		Bool notAllCachesAreReady = false;		// If any cache is not ready, wait for it
		do {

			notAllCachesAreReady = false;

			for( const CSoundCache* soundCache : m_soundCacheChain )
			{
				// Skip not ready caches (the ones which are not yet loaded by async loader)
				if( soundCache->IsReady() == false )
				{
					notAllCachesAreReady = true;
					continue;
				}

				// Find token in cache
				const CSoundCacheData::CacheToken* resourceCachedFile = soundCache->Find( fileNameWithPath );
				if( resourceCachedFile != nullptr )
				{
					// Found one!
					result.m_token = resourceCachedFile;
					result.m_cache = soundCache;
					m_soundResourceHashMap[fileNameWithPath] = result;
					break;
				}
			}

		} while( result.IsSet() == false && notAllCachesAreReady == true );
	}

	return result;
}

Bool CSoundCacheResolver::ListAllSoundBanks(TDynArray<StringAnsi>& soundBanks)
{
	Bool result = true;

	// We wait for the soundCaches to get ready
	for( CSoundCache* soundCache : m_soundCacheChain )
	{
		if( !soundCache->IsReady() )
		{
			return false;
		}
	}

	// We list soundbanks once all caches are resolved
	for( CSoundCache* soundCache : m_soundCacheChain )
	{
		soundCache->ListAllSoundBanks( soundBanks );
	}
	return result;
}

Bool CSoundCacheResolver::IsInitialized()
{
	for( CSoundCache* cache : m_soundCacheChain )
	{
		if( !cache->Empty() )
		{
			return true;
		}
	}

	return false;
}

Bool CSoundCacheResolver::IsEmpty()
{
	return m_isEmpty;
}
