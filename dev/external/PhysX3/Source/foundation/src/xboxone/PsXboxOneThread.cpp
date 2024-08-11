// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "XboxOne/PsXboxOneInclude.h"
#include "PsFoundation.h"
#include "PsThread.h"
#include "foundation/PxAssert.h"

// an exception for setting the thread name in Microsoft debuggers
#define PS_MS_VC_EXCEPTION 0x406D1388

namespace physx
{
namespace shdfnd
{

namespace {

	// struct for naming a thread in the debugger
#pragma pack(push, 8)

	typedef struct tagTHREADNAME_INFO
	{
	   DWORD dwType;		// Must be 0x1000.
	   LPCSTR szName;		// Pointer to name (in user addr space).
	   DWORD dwThreadID;	// Thread ID (-1=caller thread).
	   DWORD dwFlags;		// Reserved for future use, must be zero.
	} THREADNAME_INFO;

#pragma pack(pop)

	class _ThreadImpl
	{
	public:
		enum State
		{
			NotStarted,
			Started,
			Stopped
		};

		HANDLE				thread;
		LONG				quitNow;				//Should be 32bit aligned on SMP systems.
		State				state;
		DWORD				threadID;

		ThreadImpl::ExecuteFn	fn;
		void *				arg;
	};

	_ThreadImpl* getThread(ThreadImpl* impl)
	{
		return reinterpret_cast<_ThreadImpl*>(impl);
	}

	DWORD WINAPI PxThreadStart(LPVOID arg)
	{
		_ThreadImpl* impl = getThread((ThreadImpl*)arg);

		// run either the passed in function or execute from the derived class (Runnable).
		if(impl->fn)
			(*impl->fn)(impl->arg);
		else if(impl->arg)
			((Runnable*)impl->arg)->execute();
		return 0;
	}
}

static const PxU32 gSize = sizeof(_ThreadImpl);
const PxU32& ThreadImpl::getSize()  { return gSize; }

ThreadImpl::Id ThreadImpl::getId()
{
	return static_cast<Id>(GetCurrentThreadId());
}

ThreadImpl::ThreadImpl()
{
	getThread(this)->thread = NULL;
	getThread(this)->state = _ThreadImpl::NotStarted;
	getThread(this)->quitNow = 0;
	getThread(this)->fn = NULL;
	getThread(this)->arg = NULL;
}

ThreadImpl::ThreadImpl(ExecuteFn fn, void *arg)
{
	getThread(this)->thread = NULL;
	getThread(this)->state = _ThreadImpl::NotStarted;
	getThread(this)->quitNow = 0;
	getThread(this)->fn = fn;
	getThread(this)->arg = arg;

	start(0, NULL);
}


ThreadImpl::~ThreadImpl()
{
	if(getThread(this)->state == _ThreadImpl::Started)
		kill();
	CloseHandle(getThread(this)->thread);
}

void ThreadImpl::start(PxU32 stackSize, Runnable* runnable)
{
	if(getThread(this)->state != _ThreadImpl::NotStarted)
		return;
	getThread(this)->state = _ThreadImpl::Started;

	if(runnable && !getThread(this)->arg && ! getThread(this)->fn)
		getThread(this)->arg = runnable;

	getThread(this)->thread = CreateThread(NULL, stackSize, PxThreadStart, (LPVOID)this, 0, &getThread(this)->threadID);
}

void ThreadImpl::signalQuit()
{
	InterlockedIncrement(&(getThread(this)->quitNow));
}

bool ThreadImpl::waitForQuit()
{
	if(getThread(this)->state==_ThreadImpl::NotStarted)
		return false;

	WaitForSingleObject(getThread(this)->thread,INFINITE);
	return true;
}


bool ThreadImpl::quitIsSignalled()
{
	return InterlockedCompareExchange(&(getThread(this)->quitNow),0,0)!=0;
}

void ThreadImpl::quit()
{
	getThread(this)->state = _ThreadImpl::Stopped;
	ExitThread(0);
}

void ThreadImpl::kill()
{
	PX_ASSERT(0 && "Thread kill not implemented on XboxOne!!!!");
}

void ThreadImpl::sleep(PxU32 ms)
{
	Sleep(ms);
}

void ThreadImpl::yield()
{
	SwitchToThread();
}

PxU32 ThreadImpl::setAffinityMask(PxU32 mask)
{
	return mask ? (PxU32)SetThreadAffinityMask(getThread(this)->thread, mask) : 0;
}

void ThreadImpl::setName(const char *name)
{
	THREADNAME_INFO info;
	info.dwType		= 0x1000;
	info.szName		= name;
	info.dwThreadID	= getThread(this)->threadID;
	info.dwFlags	= 0;

	// C++ Exceptions are disabled for this project, but SEH is not (and cannot be)
	// http://stackoverflow.com/questions/943087/what-exactly-will-happen-if-i-disable-c-exceptions-in-a-project
	__try
	{
		RaiseException( PS_MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// this runs if not attached to a debugger (thus not really naming the thread)
	}
}

void ThreadImpl::setPriority(ThreadPriority::Enum prio)
{
	switch(prio)
	{
	case ThreadPriority::eHIGH:
		SetThreadPriority(getThread(this)->thread,THREAD_PRIORITY_HIGHEST);
		break;
	case ThreadPriority::eABOVE_NORMAL:
		SetThreadPriority(getThread(this)->thread,THREAD_PRIORITY_ABOVE_NORMAL);
		break;
	case ThreadPriority::eNORMAL:
		SetThreadPriority(getThread(this)->thread,THREAD_PRIORITY_NORMAL);
		break;
	case ThreadPriority::eBELOW_NORMAL:
		SetThreadPriority(getThread(this)->thread,THREAD_PRIORITY_BELOW_NORMAL);
		break;
	case ThreadPriority::eLOW:
		SetThreadPriority(getThread(this)->thread,THREAD_PRIORITY_LOWEST);
		break;
	default:
		break;
	}
}

ThreadPriority::Enum ThreadImpl::getPriority( Id threadId )
{
	ThreadPriority::Enum retval = ThreadPriority::eLOW;
	int priority = GetThreadPriority( (HANDLE) threadId );
	PX_COMPILE_TIME_ASSERT( THREAD_PRIORITY_HIGHEST > THREAD_PRIORITY_ABOVE_NORMAL );
	if ( priority >= THREAD_PRIORITY_HIGHEST )
		retval = ThreadPriority::eHIGH;
	else if ( priority >= THREAD_PRIORITY_ABOVE_NORMAL )
		retval = ThreadPriority::eABOVE_NORMAL;
	else if ( priority >= THREAD_PRIORITY_NORMAL )
		retval = ThreadPriority::eNORMAL;
	else if ( priority >= THREAD_PRIORITY_BELOW_NORMAL )
		retval = ThreadPriority::eBELOW_NORMAL;
	return retval;
}

PxU32 ThreadImpl::getNbPhysicalCores()
{
	//Xb1 has 8 cores but 2 are consumed by the OS so we can only rely on having access to 6.
	return 6;
}

PxU32 TlsAlloc()
{
	DWORD rv=::TlsAlloc();
	PX_ASSERT(rv!=TLS_OUT_OF_INDEXES);
	return (PxU32)rv;
}

void TlsFree(PxU32 index)
{
	::TlsFree(index);
}

void *TlsGet(PxU32 index)
{
	return ::TlsGetValue(index);
}

PxU32 TlsSet(PxU32 index,void *value)
{
	return ::TlsSetValue(index,value);
}

PxU32 ThreadImpl::getDefaultStackSize() { return 1048576; };


} // namespace shdfnd
} // namespace physx
