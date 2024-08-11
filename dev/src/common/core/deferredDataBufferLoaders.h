/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_DEFERRED_DATA_BUFFER_LOADERS_H_
#define _CORE_DEFERRED_DATA_BUFFER_LOADERS_H_

#pragma once

#include "deferredDataBuffer.h"

//----------

/// Loader that is using latent file token
class DeferredDataAccess_LatentToken : public DeferredDataAccess
{
public:
	DeferredDataAccess_LatentToken( IFileLatentLoadingToken* latentToken );
	virtual ~DeferredDataAccess_LatentToken();

	virtual BufferHandle LoadSync( const SyncAccess& accessInfo ) const;
	virtual BufferAsyncDataHandle LoadAsync( const AsyncAccess& accessInfo ) const;
	virtual DeferredDataAccess* Clone() const;

private:
	IFileLatentLoadingToken*		m_token;
};

//----------

/// Loader that is using latent file token that can be patched during saving with new position
class DeferredDataAccess_LatentTokenPatchable : public DeferredDataAccessPatchable
{
public:
	DeferredDataAccess_LatentTokenPatchable();
	virtual ~DeferredDataAccess_LatentTokenPatchable();

	virtual BufferHandle LoadSync( const SyncAccess& accessInfo ) const;
	virtual BufferAsyncDataHandle LoadAsync( const AsyncAccess& accessInfo ) const;

	virtual void PatchAccess( IFileLatentLoadingToken* token );
	virtual DeferredDataAccess* Clone() const;

private:
	IFileLatentLoadingToken*		m_token;
};

//----------

/// Loader that is using data from bundles
class DeferredDataAccess_BundledFile : public DeferredDataAccess
{
public:
	DeferredDataAccess_BundledFile( Red::Core::Bundle::FileID fileId, const Uint32 expectedSize );

	virtual BufferHandle LoadSync( const SyncAccess& accessInfo ) const;
	virtual BufferAsyncDataHandle LoadAsync( const AsyncAccess& accessInfo ) const;

	virtual DeferredDataAccess* Clone() const;

private:
	Red::Core::Bundle::FileID m_fileId;
	Uint32	m_expectedSize;
};

//----------

/// Loader that is using single physical file on disk (unbundled cooked data)
class DeferredDataAccess_PhysicalFile : public DeferredDataAccess
{
public:
	DeferredDataAccess_PhysicalFile( const String& depotPath, const Uint32 expectedSize );

	virtual BufferHandle LoadSync( const SyncAccess& accessInfo ) const;
	virtual BufferAsyncDataHandle LoadAsync( const AsyncAccess& accessInfo ) const;

	virtual DeferredDataAccess* Clone() const;

private:
	String	m_depotPath;
	Uint32	m_expectedSize;
};

//----------

#endif