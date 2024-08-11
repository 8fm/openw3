/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../redIO/redIOAsyncIO.h"

#include "contentManifest.h"
#include "contentManifestAsyncLoader.h"
#include "contentManifest.h"
#include "contentManifestParser.h"

CContentManifestAsyncLoader::ECallbackRequest CContentManifestAsyncLoader::OnManifestRead( SAsyncReadToken& asyncReadToken, EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CContentManifestAsyncLoader* self = static_cast< CContentManifestAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success && numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead )	
	{
		CContentManifestReader reader( ANSI_TO_UNICODE( self->m_loadingBuffer.AsChar() ) );
		SContentManifest& manifest = self->m_loadingManifest;
		if ( ! reader.ParseManifest( manifest ) )
		{
			ERR_CORE(TXT("Failed to parse manifest!"));
			self->m_errorFlag = true;
		}
	}
	else
	{
		ERR_CORE(TXT("Failed to load manifest"));
		self->m_errorFlag = true;
	}

	self->m_finishedFlag = true;

	return Red::IO::eCallbackRequest_Finish;
}

CContentManifestAsyncLoader::CContentManifestAsyncLoader()
	: m_loadingEntry( nullptr )
	, m_finishedFlag( true )
	, m_errorFlag( false )
	, m_isShuttingDown( false )
{
}

CContentManifestAsyncLoader::~CContentManifestAsyncLoader()
{
	Shutdown();
}

Bool CContentManifestAsyncLoader::IsPending() const
{
	return m_loadingEntry != nullptr || !m_finishedPackages.Empty() || !m_packageLoadQueue.Empty();
}

void CContentManifestAsyncLoader::Shutdown()
{
	m_packageLoadQueue.ClearPtr();

 	while ( !m_finishedFlag )
 	{
 		Red::Threads::YieldCurrentThread();
 	}
}

void CContentManifestAsyncLoader::BeginLoad( SContentPackInstance* packInstance )
{
	m_packageLoadQueue.Push( packInstance );
}

void CContentManifestAsyncLoader::FlushFinishedPackages( TDynArray< SContentPackInstance* >& outFinishedPackages )
{
	outFinishedPackages = Move( m_finishedPackages );
}

void CContentManifestAsyncLoader::Update()
{
	if ( m_finishedFlag )
	{
		if ( m_loadingEntry )
		{
			if ( !m_errorFlag )
			{
				m_loadingEntry->m_contentPack = Move( m_loadingManifest.m_contentPack );
				m_loadingEntry->m_packageStatus = SContentPackInstance::ePackageStatus_Ready;
			}
			else
			{
				m_loadingEntry->m_packageStatus = SContentPackInstance::ePackageStatus_LoadFailed;
			}

			m_finishedPackages.PushBack( m_loadingEntry );
			m_loadingEntry = nullptr;
			m_loadingManifest = SContentManifest();
		}

		m_errorFlag = false;
		LoadNext();
	}
}

void CContentManifestAsyncLoader::LoadNext()
{
	RED_FATAL_ASSERT( m_finishedFlag, "LoadNext() called while update in progress!");

	if ( m_packageLoadQueue.Empty() )
	{
		return;
	}

	m_finishedFlag = false;

	m_loadingEntry = m_packageLoadQueue.Front();
	m_packageLoadQueue.Pop();

	const String absoluteFilePath = String::Printf(TXT("%ls%ls"), m_loadingEntry->m_mountPath.AsChar(), MANIFEST_FILE_NAME);

	const TFileHandle fh = Red::IO::GAsyncIO.OpenFile( absoluteFilePath.AsChar(), Red::IO::eAsyncFlag_TryCloseFileWhenNotUsed );
	if ( fh == Red::IO::CAsyncIO::INVALID_FILE_HANDLE )
	{
		ERR_CORE(TXT("Failed to open manifest '%ls'"), absoluteFilePath.AsChar() );
		m_loadingEntry->m_packageStatus = SContentPackInstance::ePackageStatus_LoadFailed;
		m_finishedFlag = true;
		m_errorFlag = true;
		return;
	}

	const Uint64 fsize = Red::IO::GAsyncIO.GetFileSize( fh );
	if ( fsize < 1 )
	{
		ERR_CORE(TXT("Failed to open manifest '%ls' (empty file)"), absoluteFilePath.AsChar() );

		m_loadingEntry->m_packageStatus = SContentPackInstance::ePackageStatus_LoadFailed;
		m_finishedFlag = true;
		m_errorFlag = true;
		return;
	}

	// FIXME: the encoding is really ANSI
	m_loadingBuffer.Resize( fsize + 1 );
	m_loadingBuffer[ m_loadingBuffer.Size()-1 ] = TXT('\0');
	m_asyncReadToken.m_buffer = m_loadingBuffer.TypedData();
	m_asyncReadToken.m_offset = 0;
	m_asyncReadToken.m_numberOfBytesToRead = static_cast< Uint32 >( fsize );
	m_asyncReadToken.m_callback = &CContentManifestAsyncLoader::OnManifestRead;
	m_asyncReadToken.m_userData = this;

	Red::IO::GAsyncIO.BeginRead( fh, m_asyncReadToken, Red::IO::eAsyncPriority_High );
	Red::IO::GAsyncIO.ReleaseFile( fh );
}
