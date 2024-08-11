/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Abstract way to load data from file in an asynchronous way
class IAsyncFile
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

public:
	IAsyncFile();

	enum EResult
	{
		eResult_OK,			//!< Reading access point was created
		eResult_Failed,		//!< Something failed internally (CRC, decompression, IO, etc)
		eResult_NotReady,	//!< Content is not yet ready
	};

	//! Reference counting
	void AddRef();
	void Release();

	//! Get debug file name
	virtual const Char* GetFileNameForDebug() const = 0;

	//! Request an IFile access point
	//! WARNING: memory ownership will not be transfered, you still need to keep the IAsyncFile alive as long as you use the returned IFile
	//! Note: this can return "busy" signal which you need to handle
	virtual const EResult GetReader( IFile*& outReader) const = 0;

protected:
	virtual ~IAsyncFile() {};

private:
	Red::Threads::CAtomic< Int32 >		m_refCount;
};