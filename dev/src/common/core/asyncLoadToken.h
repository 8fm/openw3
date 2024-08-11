/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once


#include "../redIO/redIO.h"
//////////////////////////////////////////////////////////////////////////
// DEPRECATED. DO NOT USE. COMPATIBILITY LAYER.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CAsyncLoadToken;

//////////////////////////////////////////////////////////////////////////
// IAsyncLoadCallback
//////////////////////////////////////////////////////////////////////////
class IAsyncLoadCallback
{
public:
	typedef Red::IO::EAsyncResult			EAsyncResult;

public:
	virtual void							OnAsyncLoad( CAsyncLoadToken& asyncLoadToken, EAsyncResult asyncResult )=0;

protected:
											IAsyncLoadCallback() {}
	virtual									~IAsyncLoadCallback() {}
};

//////////////////////////////////////////////////////////////////////////
// CAsyncLoadToken
//////////////////////////////////////////////////////////////////////////
class CAsyncLoadToken
{
public:
	typedef Red::IO::EAsyncResult			EAsyncResult;
	typedef Red::IO::EAsyncPriority			EAsyncPriority;
	typedef Red::Threads::CAtomic< Bool >	TEvent;

private:
	IAsyncLoadCallback*						m_asyncLoadCallback;

private:
	String									m_absoluteFilePath;
	void*									m_buffer;
	Uint32									m_numberOfBytesToRead;
	Int64									m_offset;
	EAsyncPriority							m_asyncPriority;
	Red::Threads::CAtomic< Int32 >			m_refCount;

private:
	mutable Red::Threads::CAtomic< EAsyncResult >	m_asyncResult;
	mutable TEvent									m_event;

public:
											CAsyncLoadToken( const String& absoluteFilePath, void* dest, Uint32 fileReadSize, Int64 fileOffset,
																EAsyncPriority asyncPriority = EAsyncPriority::eAsyncPriority_Normal );

											CAsyncLoadToken( const String& absoluteFilePath, void* dest, Uint32 fileReadSize, Int64 fileOffset,
																IAsyncLoadCallback& asyncLoadCallback, EAsyncPriority asyncPriority = EAsyncPriority::eAsyncPriority_Normal );

											~CAsyncLoadToken();

public:
	void									AddRef();
	Int32									Release();

public:
	// Functions called by the CAsyncLoadToken owner
	void									Wait();
	EAsyncResult							GetAsyncResult() const;

public:
	// Functions called from the CAsyncFileManager
	void									Signal( EAsyncResult asyncResult );

public:
	// Functions which can be called by anything
	const String&							GetAbsoluteFilePath() const { return m_absoluteFilePath; }
	void*									GetDestinationBuffer() const { return m_buffer; }
	Uint32									GetFileReadSize() const { return m_numberOfBytesToRead; }
	Int64									GetFileOffset() const { return m_offset; }
	EAsyncPriority							GetAsyncPriority() const { return m_asyncPriority; }
};
