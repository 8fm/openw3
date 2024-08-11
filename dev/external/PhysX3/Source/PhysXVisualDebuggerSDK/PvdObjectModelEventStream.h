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



#ifndef PVD_EVENT_STREAM_H
#define PVD_EVENT_STREAM_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"
#include "PvdObjectModel.h"

namespace physx { namespace debugger {

	class PvdRandomAccessIOStream;
	class PvdObjectModelEventStreamMetaData;
	class PvdObjectModelEventStreamFactory;
	class PvdObjectModelEventStreamTransferDataProvider;

	class PvdObjectModelEventStreamPropertyHandler
	{
	protected:
		virtual ~PvdObjectModelEventStreamPropertyHandler(){}
	public:
		virtual void handleProperty( PxU64 eventIndex, PxI32 instanceId, PxI32 propertyId, const PxU8* data, PxU32 len ) = 0;
	};

	class PvdObjectModelEventStreamUserEventTransfer
	{
	protected:
		virtual ~PvdObjectModelEventStreamUserEventTransfer(){}
	public:
		virtual void handleUserEvent( PvdObjectModelEventStreamFactory& factory, NonNegativeInteger dtype, DataRef<const PxU8> data ) = 0;
	};

	struct InstancePropertyId
	{
		PxI32 mInstanceId;
		PxI32 mPropertyId;
		InstancePropertyId( NonNegativeInteger instId, NonNegativeInteger propId )
			: mInstanceId( instId ), mPropertyId( propId ) {}
		InstancePropertyId(){}
	};

	class PvdObjectModelEventStream
	{
	protected:
		virtual ~PvdObjectModelEventStream(){}
	public:
		virtual void addRef() = 0;
		virtual void release() = 0;

		virtual PvdObjectModelReader& getModel() = 0;
		
		virtual PxU64 getEventCount() const = 0;
		virtual PxU64 getCurrentEvent() const = 0;

		virtual PxU64 getFileSize() const = 0;

		virtual void seekToEvent( PxU64 eventIndex ) = 0;
		
		//seek to event, but forcefully by loading a snapshot
		virtual void loadSnapshot( PxU64 evtIdx ) = 0;

		//Property values are iterated through in the order in which they were set.
		virtual bool getPropertyHistory( DataRef<InstancePropertyId> queries, PxU64 beginEvent, PxU64 endEvent, PvdObjectModelEventStreamPropertyHandler& handler ) = 0;
		bool getPropertyHistory( InstancePropertyId query, PxU64 beginEvent, PxU64 endEvent, PvdObjectModelEventStreamPropertyHandler& handler )
		{
			DataRef<InstancePropertyId> queries( &query, 1 );
			return getPropertyHistory( queries, beginEvent, endEvent, handler );
		}

		virtual PvdObjectModelEventStream& branch( PvdObjectModel& newModel ) = 0;
		//Save this item out to a file.  Open will work on the result.
		virtual void save( PvdRandomAccessIOStream& stream ) = 0;

		//Creates a new meta data object also
		virtual PvdObjectModelEventStreamFactory& createTransferFactory( PvdRandomAccessIOStream& stream, PvdObjectModelMetaData& meta ) = 0;
		//Save off events in this range to a new stream.
		//Save a snapshot of the current DB as event 0.
		//Save next N events, including deleted objects another system
		virtual bool transfer( PxU64 eventCount, PvdObjectModelEventStreamFactory& transferFactory, PvdObjectModelEventStreamUserEventTransfer& userEventHandler ) = 0;

		PvdObjectModelEventStreamFactory& transfer( PxU64 startEvent, PxU64 eventCount, PvdRandomAccessIOStream& stream, PvdObjectModelEventStreamUserEventTransfer& userEventHandler )
		{
			PxU64 current = getCurrentEvent();
			seekToEvent( startEvent );
			PvdObjectModelEventStreamFactory& factory( createTransferFactory( stream, getModel().getMetaData().clone() ) );
			transfer( eventCount, factory, userEventHandler );
			seekToEvent( current );
			return factory;
		}

		//croptoend
		//croptobeginning
		//splitatcurrent
		//Load the stream meta and such from this file and run from here
		static PvdObjectModelEventStream& load( PxAllocatorCallback& callback, PvdObjectModel& modelAndInitialState, PvdRandomAccessIOStream& inStream, PxU64 initialOffset );
		static PvdObjectModelEventStream& create( PxAllocatorCallback& callback, PvdObjectModelEventStreamMetaData& metaData, PvdObjectModel& modelAndInitialState, PvdRandomAccessIOStream& inStream, PxU64 pageSectionStart );
	};

	

	class PvdObjectModelEventStreamUserEventListener
	{
	protected:
		virtual ~PvdObjectModelEventStreamUserEventListener(){}
	public:
		virtual PvdObjectModelMutator& getMutator() = 0;
		//Add an event with arbitrary data.  Returns the event index where
		//this event was added.
		virtual PxU64 addUserEvent( NonNegativeInteger dtype, DataRef<const PxU8> userData ) = 0;
		template<typename TDataType>
		PxU64 addUserEvent( const TDataType* dtype, PxU32 numItems )
		{
			const PxU8* dataStart = reinterpret_cast<const PxU8*>( dtype );
			NonNegativeInteger dtypeClass = getMutator().getMetaData().findClass( getPvdNamespacedNameForType<TDataType>() )->mClassId;
			return addUserEvent( dtypeClass, DataRef<const PxU8>( dataStart, numItems * sizeof( TDataType ) ) );
		}
		template<typename TDataType>
		PxU64 addUserEvent( const TDataType& dtype )
		{
			return addUserEvent( &dtype, 1 );
		}
	};

	class PvdObjectModelEventStreamFactory : public PvdObjectModelEventStreamUserEventListener
	{
	protected:
		virtual ~PvdObjectModelEventStreamFactory(){}
	public:
		virtual void addRef() = 0;
		virtual void release() = 0;

		//Gets the event stream associated with this factory.
		//The event stream and the factory share the underlying object model
		//and event stream meta data.
		virtual PvdObjectModelEventStream& getEventStream() = 0;

		//Add an event that doesn't do anything but marks a spot
		//in the event stream.  Returns the event index in the current stream.
		virtual PvdObjectModelReader& getModel() = 0; //represents the result of the latest events.
		virtual PvdRandomAccessIOStream& getIOStream() = 0;
		virtual void flush() = 0;

		//For the cropping functionality
		virtual void transferEvents( DataRef<const PxU32> offsets, DataRef<const PxU8> data, PvdObjectModelEventStreamTransferDataProvider& dataSupplier, PvdObjectModelEventStreamUserEventTransfer& userEventHandler ) = 0;

		static PvdObjectModelEventStreamFactory& create( PxAllocatorCallback& callback, PvdObjectModelReader& initialState, PvdObjectModel& model, PvdRandomAccessIOStream& inStream, PxU32 pageSizeCutoff = 0x200000 );
		//Start a new page after cutoff has been reached.  Will save out data to inStream while writing information
		//Event 0 corresponds to the model's initial state.
		//This object does not clear the model's update list.
		static PvdObjectModelEventStreamFactory& create( PxAllocatorCallback& callback, PvdObjectModel& modelAndInitialState, PvdRandomAccessIOStream& inStream, PxU32 pageSizeCutoff = 0x200000 );
	};
}}

#endif