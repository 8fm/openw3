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
#ifndef PVD_CLIENT_H
#define PVD_CLIENT_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"

namespace physx {
	class PxProfileBulkEventHandler;
	class PxProfileBulkMemoryEventHandler;
}

namespace physx { namespace debugger { namespace comm {
	class PvdCommStreamMetaData;
	class PvdCommStreamMetaDataMutator;
	class PvdCommStreamEventReader;
	class PvdCommStreamParser;
}}}

namespace physx { namespace debugger { 

	class PvdObjectModelMetaData;
	class PvdObjectModelReader;
	class PvdObjectModelEventStream;
	class PvdObjectModelEventStreamFactory;
	class PvdRandomAccessIOStream;
	class PvdObjectModelMutator;

	class PvdClient
	{
	protected:
		virtual ~PvdClient(){}

	public:
		//These four bytes begin all of our files.
		static PxU32 getFileId() { return 907025328; }

		virtual void addRef() = 0;
		virtual void release() = 0;

		//Frame 0 corresponds to event 0.  Seeking to a frame
		//means seeking to the event recorded at its end section.
		//So frame 1 ends at section[0].mEnd.mEventId.
		//The current frame cannot equal frame count, it is at most
		//frame count - 1.
		virtual PxU32 getCurrentFrame() = 0;
		virtual void setCurrentFrame( PxU32 frame ) = 0;
		virtual void parseProfileEvents( PxI64 startEvent, PxI64 endEvent, int profileZone, PxProfileBulkEventHandler& handler ) = 0;
		virtual PxU32 getNbMemoryTotals() = 0;
		virtual PxU32 getMemoryTotals( PxI32* buffer, PxU32 bufSize ) = 0;
		virtual void parseMemoryEvents( PxI64 startEvent, PxI64 endEvent, PxProfileBulkMemoryEventHandler& handler ) = 0;

		virtual comm::PvdCommStreamMetaData&	getStreamMetaData() = 0;
		virtual PvdObjectModelMetaData&			getMetaData() = 0;
		virtual PvdObjectModelReader&			getReader() = 0;
		//Note that this object may or may not exist.  When we are recording,
		//or if this item is loaded from a file then we have an event stream.
		//When we are just capturing without recording, there may not be an
		//event stream.
		virtual PvdObjectModelEventStream*		getEventStream() = 0;

		virtual PvdClient* transferEvents( PxU64 eventCount, PvdRandomAccessIOStream& fileBacking ) = 0;

		//Create a distinct client sharing only the file backend and copying everything else.
		//It should be possible to add information to this client if it has a mutator while
		//accesssing the other client;i.e. no mutable objects can be shared.  The file stream,
		//for all intents and purposes, is the only immutable object and as such it is probably
		//the only thing shared.
		virtual PvdClient* branch() = 0;

		virtual void save( PvdRandomAccessIOStream& ioStream ) const = 0;

		//If the data file isn't a valid pvd file, this function returns NULL.
		static PvdClient* load( PxAllocatorCallback& alloc, PvdRandomAccessIOStream& data );
	};

	//The client mutator is either monitoring events
	//or recording events.  If it is monitoring events
	//then it is not using an event stream factory.
	//If it is recording events then it is, and the
	//mutation 
	class PvdClientMutator
	{
	protected:
		virtual ~PvdClientMutator(){}
	public:
		
		virtual void addRef() = 0;
		virtual void release() = 0;

		virtual PvdClient&							getClient() = 0;
		virtual comm::PvdCommStreamMetaDataMutator&	getCommStreamMetaDataMutator() = 0;
		virtual PvdObjectModelMutator&				getObjectModelMutator() = 0;
		
		virtual void disableRecording() = 0;
		virtual void enableRecording( PvdRandomAccessIOStream& recordingBacking ) = 0;
		virtual bool isRecording() const = 0;

		virtual PvdObjectModelEventStreamFactory*	getEventStreamFactory() = 0;
		virtual comm::PvdCommStreamParser&			createParser( comm::PvdCommStreamEventReader& reader ) = 0;

		static PvdClientMutator& create( PxAllocatorCallback& alloc, PvdRandomAccessIOStream& fileBacking );
	};

}}

#endif
