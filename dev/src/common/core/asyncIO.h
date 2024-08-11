/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CAsyncLoadToken;
class CAsyncFileHandleCache;

//////////////////////////////////////////////////////////////////////////
// CAsyncIO
//////////////////////////////////////////////////////////////////////////
class CDeprecatedIO : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

private:
	typedef Red::Threads::CMutex						CMutex;
	typedef Red::Threads::CScopedLock< CMutex >			CScopedLock;


public:
											CDeprecatedIO();
	virtual									~CDeprecatedIO();
	Bool									Init();
	void									Shutdown();

public:
	//! Request async I/O by token
	RED_MOCKABLE void						LoadAsync( CAsyncLoadToken* asyncLoadToken );
	
	//! Try to cancel an async I/O request by its token
	void									TryCancel( CAsyncLoadToken* asyncLoadToken );
};

// Async file manager instance
extern CDeprecatedIO* GDeprecatedIO;