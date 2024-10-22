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


#ifndef PX_PHYSX_PROFILE_EVENT_BUFFER_H
#define PX_PHYSX_PROFILE_EVENT_BUFFER_H
#include "PxProfileEvents.h"
#include "PxProfileEventSerialization.h"
#include "PxProfileEventSystem.h"
#include "PsArray.h"
#include "PsTime.h"
#include "PsCpu.h"
#include "PxProfileDataBuffer.h"

namespace physx { namespace profile {

	/**
	 *	An event buffer maintains an in-memory buffer of events.  When this buffer is full
	 *	it sends to buffer to all handlers registered and resets the buffer.
	 *
	 *	It is parameterized in four ways.  The first is a context provider that provides
	 *	both thread id and context id.
	 *	
	 *	The second is the mutex (which may be null) and a scoped locking mechanism.  Thus the buffer
	 *	may be used in a multithreaded context but clients of the buffer don't pay for this if they
	 *	don't intend to use it this way.
	 *
	 *	Finally the buffer may use an event filtering mechanism.  This mechanism needs one function,
	 *	namely isEventEnabled( PxU8 subsystem, PxU8 eventId ).
	 *
	 *	All of these systems can be parameterized at compile time leading to an event buffer
	 *	that should be as fast as possible given the constraints.
	 *
	 *	Buffers may be chained together as this buffer has a handleBufferFlush method that
	 *	will grab the mutex and add the data to this event buffer.
	 *
	 *	Overall, lets look at the PhysX SDK an how all the pieces fit together.
	 *	The SDK should have a mutex-protected event buffer where actual devs or users of PhysX
	 *	can register handlers.  This buffer has slow but correct implementations of the
	 *	context provider interface.
	 *
	 *	The SDK object should also have a concrete event filter which was used in the
	 *	construction of the event buffer and which it exposes through opaque interfaces.
	 *
	 *	The SDK should protect its event buffer and its event filter from multithreaded
	 *	access and thus this provides the safest and slowest way to log events and to
	 *	enable/disable events.
	 *
	 *	Each scene should also have a concrete event filter.  This filter is updated from
	 *	the SDK event filter (in a mutex protected way) every frame.  Thus scenes can change
	 *	their event filtering on a frame-by-frame basis.  It means that tasks running
	 *	under the scene don't need a mutex when accessing the filter.
	 *
	 *	Furthermore the scene should have an event buffer that always sets the context id
	 *	on each event to the scene.  This allows PVD and other systems to correlate events
	 *	to scenes.  Scenes should provide access only to a relative event sending system
	 *	that looks up thread id upon each event but uses the scene id.
	 *
	 *	The SDK's event buffer should be setup as an EventBufferClient for each scene's
	 *	event buffer. Thus the SDK should expose an EventBufferClient interface that
	 *	any client can use.
	 *
	 *	For extremely *extremely* performance sensitive areas we should create a specialized
	 *	per-scene, per-thread event buffer that is set on the task for these occasions.  This buffer
	 *	uses a trivial event context setup with the scene's context id and the thread id.  It should
	 *	share the scene's concrete event filter and it should have absolutely no locking.  It should
	 *	empty into the scene's event buffer which in some cases should empty into the SDK's event buffer
	 *	which when full will push events all the way out of the system.  The task should *always* flush
	 *	the event buffer (if it has one) when it is finished; nothing else will work reliably.
	 *
	 *	If the per-scene,per-thread event buffer is correctly parameterized and fully defined adding
	 *	a new event should be an inline operation requiring no mutex grabs in the common case.  I don't
	 *	believe you can get faster event production than this; the events are as small as possible (all
	 *	relative events) and they are all produced inline resulting in one 4 byte header and one
	 *	8 byte timestamp per event.  Reducing the memory pressure in this way reduces the communication
	 *	overhead, the mutex grabs, basically everything that makes profiling expensive at the cost
	 *	of a per-scene,per-thread event buffer (which could easily be reduced to a per-thread event
	 *	buffer.
	 */
	template<typename TContextProvider, 
			typename TMutex, 
			typename TScopedLock,
			typename TEventFilter>
	class EventBuffer  : public DataBuffer<TMutex, TScopedLock>
	{
	public:
		typedef DataBuffer<TMutex, TScopedLock> TBaseType;
		typedef TContextProvider	TContextProviderType;
		typedef TEventFilter		TEventFilterType;
		typedef typename TBaseType::TMutexType TMutexType;
		typedef typename TBaseType::TScopedLockType TScopedLockType;
		typedef typename TBaseType::TU8AllocatorType TU8AllocatorType;
		typedef typename TBaseType::TMemoryBufferType TMemoryBufferType;
		typedef typename TBaseType::TBufferClientArray TBufferClientArray;

	private:
		EventContextInformation				mEventContextInformation;
		PxU64								mLastTimestamp;
		TContextProvider					mContextProvider;
		TEventFilterType					mEventFilter;

	public:
		EventBuffer(PxAllocatorCallback* inFoundation
					, PxU32 inBufferFullAmount
					, const TContextProvider& inProvider
					, TMutexType* inBufferMutex
					, const TEventFilterType& inEventFilter )
					: TBaseType( inFoundation, inBufferFullAmount, inBufferMutex, "struct physx::profile::ProfileEvent" )
			, mLastTimestamp( 0 )
			, mContextProvider( inProvider )
			, mEventFilter( inEventFilter )
		{
			PxMemSet(&mEventContextInformation,0,sizeof(EventContextInformation));
		}

		TContextProvider& getContextProvider() { return mContextProvider; }

		inline void startEvent( PxU16 inId, PxU32 threadId, PxU64 contextId, PxU8 cpuId, PxU8 threadPriority, PxU64 inTimestamp )
		{
			TScopedLockType lock( TBaseType::mBufferMutex );
			if ( mEventFilter.isEventEnabled( inId ) )
			{
				StartEvent theEvent;
				theEvent.init( threadId, contextId, cpuId, threadPriority, inTimestamp );
				doAddProfileEvent( inId, theEvent );
			}
		}

		inline void startEvent( PxU16 inId, PxU64 contextId )
		{
			PxProfileEventExecutionContext ctx( mContextProvider.getExecutionContext() );
			startEvent( inId, ctx.mThreadId, contextId, ctx.mCpuId, static_cast<PxU8>(ctx.mThreadPriority), shdfnd::Time::getCurrentCounterValue() );
		}

		inline void startEvent( PxU16 inId, PxU64 contextId, PxU32 threadId )
		{
			startEvent( inId, threadId, contextId, 0, 0, shdfnd::Time::getCurrentCounterValue() );
		}

		inline void stopEvent( PxU16 inId, PxU32 threadId, PxU64 contextId, PxU8 cpuId, PxU8 threadPriority, PxU64 inTimestamp )
		{
			TScopedLockType lock( TBaseType::mBufferMutex );
			if ( mEventFilter.isEventEnabled( inId ) )
			{
				StopEvent theEvent;
				theEvent.init( threadId, contextId, cpuId, threadPriority, inTimestamp );
				doAddProfileEvent( inId, theEvent );
			}
		}

		inline void stopEvent( PxU16 inId, PxU64 contextId )
		{
			PxProfileEventExecutionContext ctx( mContextProvider.getExecutionContext() );
			stopEvent( inId, ctx.mThreadId, contextId, ctx.mCpuId, static_cast<PxU8>(ctx.mThreadPriority), shdfnd::Time::getCurrentCounterValue() );
		}

		inline void stopEvent( PxU16 inId, PxU64 contextId, PxU32 threadId )
		{
			stopEvent( inId, threadId, contextId, 0, 0, shdfnd::Time::getCurrentCounterValue() );
		}

		inline void eventValue( PxU16 inId, PxU64 contextId, PxI64 inValue )
		{
			eventValue( inId, mContextProvider.getThreadId(), contextId, inValue );
		}

		inline void eventValue( PxU16 inId, PxU32 threadId, PxU64 contextId, PxI64 inValue )
		{
			TScopedLockType lock( TBaseType::mBufferMutex );
			EventValue theEvent;
			theEvent.init( inValue, contextId, threadId );
			EventHeader theHeader( static_cast<PxU8>( getEventType<EventValue>() ), inId );
			//set the header relative timestamp;
			EventValue& theType( theEvent );
			theType.setupHeader( theHeader );
			sendEvent( theHeader, theType );
		}
		inline void CUDAProfileBuffer( PxU64 inTimestamp, PxF32 batchRuntimeInMilliseconds, const PxU8* cudaData, PxU32 bufLenInBytes, PxU32 bufferVersion )
		{
			TScopedLockType lock( TBaseType::mBufferMutex );
			physx::profile::CUDAProfileBuffer theEvent;
			theEvent.init( inTimestamp, batchRuntimeInMilliseconds, cudaData, bufLenInBytes, bufferVersion );
			EventHeader theHeader( static_cast<PxU8>( getEventType<physx::profile::CUDAProfileBuffer>() ), 0 );
			//Do a check of the event's size first.
			if ( ( bufLenInBytes + TBaseType::mDataArray.size() ) >= TBaseType::mBufferFullAmount )
				TBaseType::flushEvents();
			//with this system, that is the best we can do.  CUDA buffers are massive, like 100 - 200K each.  *massive*.
			sendEvent( theHeader, theEvent );
		}

		inline void CUDAProfileBuffer( PxF32 batchRuntimeInMilliseconds, const PxU8* cudaData, PxU32 bufLenInBytes, PxU32 bufferVersion = PxProfileEventSender::CurrentCUDABufferFormat )
		{
			this->CUDAProfileBuffer( shdfnd::Time::getCurrentCounterValue(), batchRuntimeInMilliseconds, cudaData, bufLenInBytes, bufferVersion );
		}

		void flushProfileEvents()
		{	
			TBaseType::flushEvents();
		}

		void release()
		{
			PX_PROFILE_DELETE( TBaseType::mWrapper.mUserFoundation, this );
		}
	protected:
		//Clears the cache meaning event compression
		//starts over again.
		//only called when the buffer mutex is held
		void clearCachedData()
		{
			mEventContextInformation.setToDefault();
			mLastTimestamp = 0;
		}

		template<typename TProfileEventType>
		inline void doAddProfileEvent( PxU16 eventId, const TProfileEventType& inType )
		{
			if ( mEventContextInformation == inType.mContextInformation )
				doAddEvent( static_cast<PxU8>( inType.getRelativeEventType() ), eventId, inType.getRelativeEvent() );
			else
			{
				mEventContextInformation = inType.mContextInformation;
				doAddEvent( static_cast<PxU8>( getEventType<TProfileEventType>() ), eventId, inType );
			}
		}

		template<typename TDataType>
		inline void doAddEvent( PxU8 inEventType, PxU16 eventId, const TDataType& inType )
		{
			EventHeader theHeader( inEventType, eventId );
			//set the header relative timestamp;
			TDataType& theType( const_cast<TDataType&>( inType ) );
			PxU64 currentTs =  inType.getTimestamp();
			theType.setupHeader( theHeader, mLastTimestamp );
			mLastTimestamp = currentTs;
			sendEvent( theHeader, theType );
		}

		template<typename TDataType>
		inline void sendEvent( EventHeader& inHeader, TDataType& inType )
		{
			inHeader.streamify( TBaseType::mSerializer );
			inType.streamify( TBaseType::mSerializer, inHeader );
			if ( TBaseType::mDataArray.size() >= TBaseType::mBufferFullAmount )
				flushProfileEvents();

		}
	};
}}
#endif