/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskSched.h"
#include "taskSchedNode.h"
#include "taskDeque.h"
#include "taskDebug.h"
#include "engineTime.h"

/////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTaskScheduler;
class CTaskGroup;
class CTaskRunner;
class CTaskDispatcher;

//////////////////////////////////////////////////////////////////////////
// CTask
//////////////////////////////////////////////////////////////////////////

const Uint32 RED_TASK_ALIGNMENT = 16;

class CTask : public CTaskSchedNode, private Red::System::NonCopyable
{
private:
	Red::Threads::CAtomic< Int32 >			m_lifeRefCount;				// 4			 bytes
	Uint32									m_taskFlags;				// 4			 bytes
	CTaskDispatcher*						m_taskDispatcher;			// 4/8 (x64)	 bytes

public:
	enum ETaskRoot { Root };

public:
	virtual const Char*						GetDebugName() const { return TXT("CTask"); }
	virtual Uint32							GetDebugColor() const { return 0xFFFFFFFF; }

public:
	Int32									Release();
	Int32									AddRef();

public:
	Bool									HasFlag( Uint32 flag ) const { return ( m_taskFlags & flag ) != 0; }
	CTaskDispatcher*						GetDispatcher() const { return m_taskDispatcher; }

public:
	void									TryCancel();

public:
	virtual void							Run()=0;

public:
	void*									operator new( size_t size, ETaskRoot );

public:
	// Must be available for the compiler. We don't use exceptions, so shouldn't be called here..
	// Can't really stop somebody calling global delete explicitly though, e.g., ::delete pTask
	void									operator delete( void* ptr );
	void									operator delete( void *, ETaskRoot ){RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); } 

protected:
	explicit								CTask( Uint32 taskFlags = 0 );
	virtual									~CTask();

private:
	void*									operator new[]( size_t /*size*/ ) NOEXCEPT { RED_HALT( "Don't call new[]" ); return nullptr; }
	void									operator delete[]( void* /*ptr*/ ) { RED_HALT( "Don't call delete[]" ); }

	friend class CTaskScheduler;
	friend class CTaskRunner;
};

