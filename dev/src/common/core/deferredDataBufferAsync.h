/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_DEFERRED_DATA_BUFFER_ASYNC_H_
#define _CORE_DEFERRED_DATA_BUFFER_ASYNC_H_

#pragma once

class IAsyncFile;

#include "deferredDataBuffer.h"
#include "deferredDataBufferKickoff.h"

//---------------------------

/// Generic async data class - holds most of the common parameters
class BufferAsyncDataJobBased : public BufferAsyncData
{
public:
	BufferAsyncDataJobBased( const DeferredDataAccess::AsyncAccess& accessData );
	virtual ~BufferAsyncDataJobBased();

	virtual Uint32 GetSize() const
	{
		return m_accessData.m_size;
	}

	// request the data, can return eResult_NotReady
	virtual EResult GetData( BufferHandle& outData ) const;

	// called from job - a kind request to load data into provided memory
	virtual Bool LoadDataIntoMemory( void* memory, const Uint32 size ) const = 0;

protected:
	BufferHandle						m_data;

	DeferredDataAccess::AsyncAccess		m_accessData;
	mutable Red::Threads::CLightMutex	m_lock;
	mutable ILoadJob*					m_job;
	mutable Bool						m_failed;

	// process loading of the data
	Bool ProcessLoading();

	// issue loading job
	void IssueJob();

	friend class CBufferAsyncData_LoadingJob;
};

//---------------------------

/// Helper used to load uncompressed data from a physical file
class BufferAsyncData_PhysicalFile : public BufferAsyncDataJobBased
{
public:
	BufferAsyncData_PhysicalFile( const DeferredDataAccess::AsyncAccess& accessData, const String& path );

private:	
	virtual Bool LoadDataIntoMemory( void* memory, const Uint32 size ) const;
	virtual void Kick() override {};


	String				m_filePath;	
};

//---------------------------

/// Helper used to load uncompressed data from a data accessible through IFileLatentToken
/// Token should be cloned as it will be deleted automatically
class BufferAsyncData_LatentToken : public BufferAsyncDataJobBased
{
public:
	BufferAsyncData_LatentToken( const DeferredDataAccess::AsyncAccess& accessData, IFileLatentLoadingToken* tokenCloned );
	virtual ~BufferAsyncData_LatentToken();

private:	
	virtual Bool LoadDataIntoMemory( void* memory, const Uint32 size ) const;
	virtual void Kick() override {};


	IFileLatentLoadingToken*		m_token;
};

//---------------------------

/// Helper used to load compressed data from bundles using the DecompressionThread directly
/// This is the most optimal path
class BufferAsyncData_BundledBuffer : public BufferAsyncData
{
public:
	BufferAsyncData_BundledBuffer( const DeferredDataAccess::AsyncAccess& accessData, Red::Core::Bundle::FileID fileID );
	virtual ~BufferAsyncData_BundledBuffer();

	virtual Uint32 GetSize() const;
	virtual EResult GetData( BufferHandle& outData ) const;

private:
	Red::Core::Bundle::FileID			m_fileID;
	DeferredDataAccess::AsyncAccess		m_accessData;

	mutable BufferHandle				m_data;
	mutable Red::Threads::CLightMutex	m_lock;
	mutable IAsyncFile*					m_asyncFile;

	enum EState
	{
		eState_NotStated,
		eState_Started,
		eState_Finished,
		eState_Failed,
	};

	mutable EState				m_state;
	mutable Bool				m_kickRegistered;

	virtual void Kick() override;
	virtual Uint32 Release() override final;

	void UpdateState() const;
	void TryStart() const;

	void UnregisterKicker_NoLock() const;
	void UpdateState_NoLock() const;

	friend class KickoffHandler;
};

//---------------------------

/// Synchronous data wrapper
class BufferAsyncData_SyncData : public BufferAsyncData
{
public:
	BufferAsyncData_SyncData( BufferHandle data, const Uint32 size );

	virtual Uint32 GetSize() const;
	virtual EResult GetData( BufferHandle& outData ) const;
	virtual void Kick() override {};

private:
	BufferHandle		m_data;
	Uint32				m_size;
};



#endif