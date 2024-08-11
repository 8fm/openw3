/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorPrefetchDataLoader.h"
#include "..\core\ioTags.h"

CSectorPrefetchDataLoader::CSectorPrefetchDataLoader( const String& absolutePath )
	: m_numLoadingBuffers( 0 )
	, m_numLoadingBytes( 0 )
{
	// open file
	m_asyncFileHandle = Red::IO::GAsyncIO.OpenFile( absolutePath.AsChar() );
}

CSectorPrefetchDataLoader::~CSectorPrefetchDataLoader()
{
	if ( m_asyncFileHandle != Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		Red::IO::GAsyncIO.ReleaseFile( m_asyncFileHandle );
	}
}

const Bool CSectorPrefetchDataLoader::RequestLoad( const Uint64 absoluteOffset, const Uint32 size, void* targetMem, CallbackFunction callback )
{
	// stats
	m_numLoadingBuffers.Increment();
	m_numLoadingBytes.ExchangeAdd( size );

	// add to list
	LoadingJob* job = new LoadingJob();
	job->m_token.m_buffer = targetMem;
	job->m_token.m_callback = &OnDataLoaded;
	job->m_token.m_numberOfBytesToRead = size;
	job->m_token.m_offset = absoluteOffset;
	job->m_token.m_userData = job; // self referencing
	job->m_loader = this;
	job->m_callback = callback;

	// add to list of pending jobs
	{
		TScopedLock lock( m_lock );
		m_pendingJobs.PushBack( job );
	}

	// schedule stuff to load
	const auto priority = Red::IO::eAsyncPriority_VeryHigh;
	Red::IO::GAsyncIO.BeginRead( m_asyncFileHandle, job->m_token, priority, eIOTag_StreamingData );

	// job added
	return true;
}

Red::IO::ECallbackRequest CSectorPrefetchDataLoader::OnDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	LoadingJob* job = (LoadingJob*) asyncReadToken.m_userData;
	auto* loader = job->m_loader;

	// stats
	const Int32 newJobCount = loader->m_numLoadingBuffers.Decrement();
	RED_FATAL_ASSERT( newJobCount >= 0, "Invalid job count");
	loader->m_numLoadingBytes.ExchangeAdd( -(Int32)numberOfBytesTransferred );

	// remove from pending job list
	{
		TScopedLock lock( loader->m_lock );
		RED_FATAL_ASSERT( loader->m_pendingJobs.Exist(job), "Finished job that is not registered" );
		loader->m_pendingJobs.Remove( job );
	}

	// execute the post-load callback
	job->m_callback();

	// delete the job holder (not needed any more after callback is executed)
	delete job;

	// loading done for this shit
	return Red::IO::eCallbackRequest_Finish;
}

