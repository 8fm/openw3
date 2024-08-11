/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redIO/redIO.h"
#include "../redThreads/redThreadsThread.h"

#include "contentManifest.h"
#include "queue.h"

struct SContentManifest;

class CContentManifestAsyncLoader
{
public:
	CContentManifestAsyncLoader();
	~CContentManifestAsyncLoader();

	void BeginLoad( SContentPackInstance* packInstance );
	void FlushFinishedPackages( TDynArray< SContentPackInstance* >& outFinishedPackages );
	void Update();

public:
	Bool IsPending() const;

private:
	void Shutdown();
	void LoadNext();

private:
	typedef Red::IO::CAsyncIO::TFileHandle TFileHandle;
	typedef Red::IO::SAsyncReadToken SAsyncReadToken;
	typedef Red::IO::ECallbackRequest ECallbackRequest;
	typedef Red::IO::EAsyncResult EAsyncResult;

private:
	static ECallbackRequest OnManifestRead( SAsyncReadToken& asyncReadToken, EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );

private:
	TQueue< SContentPackInstance* >		m_packageLoadQueue;
	SAsyncReadToken						m_asyncReadToken;
	SContentPackInstance*				m_loadingEntry;
	SContentManifest					m_loadingManifest;
	StringAnsi							m_loadingBuffer;
	volatile Bool						m_finishedFlag;
	volatile Bool						m_errorFlag;

private:
	TDynArray< SContentPackInstance* >	m_finishedPackages;

private:
	Bool								m_isShuttingDown;
};
