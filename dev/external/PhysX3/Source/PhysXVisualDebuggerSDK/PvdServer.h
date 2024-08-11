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
#ifndef PVD_SERVER_H
#define PVD_SERVER_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"

namespace physx { namespace debugger {
	class PvdClientMutator;
	class PvdRandomAccessIOStream;
	class PvdNetworkInStream;
	class PvdNetworkOutStream;
}}

namespace physx { namespace debugger { namespace comm {

	class PvdAsyncLoader;

	class PvdDisconnectionListener
	{
	protected:
		virtual ~PvdDisconnectionListener(){}
	public:
		virtual void onLoaderDisconnected( PvdAsyncLoader& inLoader ) = 0;
	};

	struct PvdLoadState
	{
		enum Enum
		{
			Unknown = 0,
			Loading,
			Paused,
		};
	};

	class PvdAsyncLoader
	{
	protected:
		virtual ~PvdAsyncLoader(){}
	public:
		
		virtual const char* getRemoteConnectionName() const = 0;

		virtual void setDisconnectionListener( PvdDisconnectionListener& inListener ) = 0;

		//DB access *must* go through the locks, the loader is continuously pulling from the
		//socket and converting data into the database as fast as it can.
		virtual void lock() = 0;
		//Get the primary database we are writing information to.
		virtual PvdClientMutator& getClient() = 0;
		//DB access *must* go through the locks
		virtual void unlock() = 0;

		//Returns true if this object is still receiving information.
		virtual bool isLoading() const = 0;

		//Allow us to pause the ongoing simulation by setting the loader
		//load state to paused.
		virtual PvdLoadState::Enum getLoadState() const = 0;

		//Allow us to pause the ongoing simulation by setting the loader
		//load state to paused.  This doesn't affect the isLoading flag.
		virtual void setLoadState( PvdLoadState::Enum inState ) = 0;

		//Stop loading data, call the disconnection function.
		virtual void stopLoading() = 0;

		virtual void release() = 0;

		virtual PxU64 getLoadedDataSize() = 0;

		//Creates a loader with a PVD client in a recording state.
		static PvdAsyncLoader* create( PxAllocatorCallback& alloc, const char* remoteName, PxU32 firstWord, PvdNetworkInStream& inStream, PvdNetworkOutStream& outStream, PvdRandomAccessIOStream& backingStream );
	};

	class PvdServerHandler
	{
	protected:
		virtual ~PvdServerHandler(){}

	public:
		//The first word is read from the socket directly so you need to take that into account when creating
		//the handler for these streams.
		virtual void onNewConnection( const char* remoteName, PxU32 firstWord, PvdNetworkInStream& inStream, PvdNetworkOutStream& outStream ) = 0;
		virtual bool getDebugStreamFilename( char* inBuf, PxU32 inBufLen, const char* inConnectionName ) = 0;
		//Called when the server itself is released
		virtual void release() = 0;
	};

	class PvdServer
	{
	protected:
		virtual ~PvdServer(){}
	public:
		virtual PxU16 getPort() const = 0;
		//Port defaults to 5425
		//Return value of zero indicates success.
		//Else this returns a platform specific error code.
		virtual int setPort( PxU16 inPort ) = 0;

		virtual int getMaxConnections() const = 0;
		virtual int setMaxConnections(int maxConnections) = 0;

		virtual void release() = 0;

		static PvdServer& createServer( PxAllocatorCallback& alloc, PvdServerHandler& inHander );
	};

}}}

#endif
