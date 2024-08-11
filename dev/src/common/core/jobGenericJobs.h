/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "math.h"
#include "handleMap.h"
#include "loadingJob.h"
#include "diskFile.h"

class CResource;
class CGenericCountedFence;

///////////////////////////////////////////////////////////////

/// Job task to load resource from depot
class CJobLoadResource : public ILoadJob
{
public:
	struct CCallback
	{
		virtual void			OnJobFinished( CJobLoadResource* job, EJobResult result ) = 0;
	};

	CJobLoadResource( const String& resourceToLoad, EJobPriority prio = JP_Resources, Bool willBeElementOfBatch = false, class CGenericCountedFence* loadingFence = nullptr, EResourceLoadingPriority rprio = EResourceLoadingPriority::eResourceLoadingPriority_Normal, CCallback* callback = nullptr );
	virtual ~CJobLoadResource();

	//! Get loaded resource
	RED_INLINE CResource* GetResource() { return m_loadedResource.Get(); }

	//! Get path of the resource being loaded
    RED_INLINE const String& GetResourceToLoad() const { return m_resourceToLoad; }

	//! Get loading priority
	RED_INLINE const EResourceLoadingPriority GetPriority() const { return m_loadingPriority; }

protected:
	virtual EJobResult Process();

	//! Kick the loading fence associated with this loading job
	void SignalLoadingFence();

	virtual const Char* GetDebugName() const override { return TXT("LoadResource"); }

	String						m_resourceToLoad;
	Int32						m_index;
	THandle< CResource >		m_loadedResource;
	CGenericCountedFence*		m_loadingFence;
	CCallback*					m_callback;
	EResourceLoadingPriority	m_loadingPriority;
};

///////////////////////////////////////////////////////////////

/// Loading job init info
struct JobLoadDataInitInfo
{
	IFile*							m_sourceFile;			//!< File to load from
	const IFileLatentLoadingToken*	m_sourceFileToken;		//!< Token to use to open the file to load from
	size_t							m_offset;				//!< Offset to load from
	size_t							m_size;					//!< Size to load
	void*							m_buffer;				//!< Preallocated buffer
	Bool							m_closeFile;			//!< Close the given file handle
	CObject**						m_object;				//!< Object to load

	RED_INLINE JobLoadDataInitInfo()
		: m_sourceFile( NULL )
		, m_sourceFileToken( NULL )
		, m_offset( 0 )
		, m_size( 0 )
		, m_buffer( NULL )
		, m_closeFile( false )
		, m_object( NULL )
	{};
};

/// Job task to load raw data from file
class CJobLoadData : public ILoadJob
{
public:
	CJobLoadData( const JobLoadDataInitInfo& initData, EJobPriority priority = JP_Default, Red::MemoryFramework::MemoryClass memoryClass = MC_BufferFlash, const Bool isGCBlocker = false );
	~CJobLoadData();

	//! Get size of loaded data
	size_t GetDataSize();

	//! Get buffer to loaded data
	const void* GetDataBuffer();

	//! Get offset to load the data at
	size_t GetLoadOffset() const { return m_offset; }

protected:
	virtual EJobResult Process();

	virtual const Char* GetDebugName() const override { return TXT("LoadData"); }

	IFile*								m_sourceFile;			//!< File to load from. Valid when m_fileToken is NULL.
	IFileLatentLoadingToken*			m_sourceFileToken;		//!< File token to load from. Valid when m_file is NULL.
	size_t								m_offset;				//!< Offset to load from
	size_t								m_size;					//!< Size of data to read
	void*								m_buffer;				//!< Target buffer
	Bool								m_ownsFileHandle;		//!< Do we own the file handle ( will be closed on destroy )
	Bool								m_ownsBufferHandle;		//!< Do we own the buffer memory ( will be freed on destroy )
	Red::MemoryFramework::MemoryClass	m_memoryClass;
};

///////////////////////////////////////////////////////////////

/// Job task to load raw data from file
class CJobLoadDataCreateObject : public CJobLoadData
{
public:
	CJobLoadDataCreateObject( const JobLoadDataInitInfo& initData, EJobPriority priority = JP_Default, Red::MemoryFramework::MemoryClass memoryClass = MC_BufferFlash );
	~CJobLoadDataCreateObject();

protected:
	virtual EJobResult Process();

	virtual const Char* GetDebugName() const override { return TXT("LoadDataCreateObject"); }

	CObject**					m_object;
};

///////////////////////////////////////////////////////////////

