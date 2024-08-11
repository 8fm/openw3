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


#ifndef PVD_COMM_STREAM_TYPES_H
#define PVD_COMM_STREAM_TYPES_H
#include "PvdObjectModelBaseTypes.h"
#include "PvdCommStreamEvents.h"
#include "PvdConnection.h"
#include "PvdNetworkStreams.h"
#include "PxProfileZoneManager.h"
#include "PvdDataStream.h"
#include "PxBroadcastingAllocator.h"
#include "PsMutex.h"
#include "PvdFoundation.h"
#include "foundation/PxErrorCallback.h"

namespace physx
{
	class PxProfileZone;
	class PxProfileMemoryEventBuffer;
}

namespace physx { namespace debugger { namespace comm {
	struct PvdErrorMessage;
}}}
namespace physx { namespace debugger {
	namespace render {
		class PvdImmediateRender;
	}
	class PvdObjectModelMetaData;

	DEFINE_PVD_TYPE_NAME_MAP( PxProfileZone, "_debugger_", "PxProfileZone" );
	DEFINE_PVD_TYPE_NAME_MAP( PxProfileMemoryEventBuffer, "_debugger_", "PxProfileMemoryEventBuffer" );
	DEFINE_PVD_TYPE_NAME_MAP( renderer::PvdImmediateRenderer, "_debugger_", "PvdImmediateRenderer" );
	DEFINE_PVD_TYPE_NAME_MAP( comm::PvdErrorMessage, "_debugger_", "PvdErrorMessage" );
	//All event streams are on the 'events' property of objects of these types
	static inline NamespacedName getMemoryEventTotalsClassName() { return NamespacedName( "_debugger", "MemoryEventTotals" ); }
}}

namespace physx { namespace debugger { namespace comm {
	

	class PvdOMMetaDataProvider
	{
	protected:
		virtual ~PvdOMMetaDataProvider(){}
	public:
		virtual void addRef() = 0;
		virtual void release() = 0;
		virtual PvdObjectModelMetaData& lock() = 0;
		virtual void unlock() = 0;
		virtual bool createInstance( const NamespacedName& clsName, const void* instance ) = 0;
		virtual bool isInstanceValid( const void* instance ) = 0;
		virtual void destroyInstance( const void* instance ) = 0;
		virtual NonNegativeInteger getInstanceClassType( const void* instance ) = 0;
	};

	class PvdCommStreamInternalConnection;

	class PvdConnectionListener
	{
	protected:
		virtual ~PvdConnectionListener(){}
	public:
		virtual void onDisconnect( PvdCommStreamInternalConnection& connection ) = 0;
		virtual void onInstanceDestroyed( const void* instance ) = 0;
	};

	class PvdCommStreamEmbeddedTypes
	{
	public:
		static const char* getProfileEventStreamSemantic() { return "profile event stream"; };
		static const char* getMemoryEventStreamSemantic() { return "memory event stream"; };
		static const char* getRendererEventStreamSemantic() { return "render event stream"; };
	};

	class PvdCommStreamInternalConnection : public PvdConnection, public PxProfileZoneHandler, public PxAllocationListener, public PxErrorCallback
	{
	protected:
		virtual ~PvdCommStreamInternalConnection(){}
	public:
		virtual void setPickable( const void* instance, bool pickable ) = 0;
		virtual void setColor( const void* instance, const PvdColor& color ) = 0;
		virtual void setCamera( String name, const PxVec3& position, const PxVec3& up, const PxVec3& target ) = 0;
		virtual void sendErrorMessage( PxU32 code, String message, String file, PxU32 line) = 0;
		virtual void setIsTopLevelUIElement( const void* instance, bool isTopLevel ) = 0;
		virtual void sendStreamInitialization() = 0;
		virtual void sendStreamEnd() = 0;
		static PvdCommStreamInternalConnection& create( PxAllocatorCallback& callback, PxAllocatorCallback& nonBroadcastingAlloc
											, PvdNetworkOutStream& outStream, PvdNetworkInStream* inStream
											, TConnectionFlagsType inConnectionType, PvdConnectionListener& disconnectListener
											, MutexT<ForwardingAllocator>& allocationMutex, bool doubleBuffered );
	};

	class PvdCommStreamEventBufferClient;

	class PvdCommStreamInternalDataStream : public PvdDataStream
	{
	protected:
		virtual ~PvdCommStreamInternalDataStream(){}
	public:
		virtual void enableCache() = 0;
		virtual void disableCache() = 0;
		virtual void addProfileZone( void* zone, const char* name ) = 0;
		virtual void addProfileZoneEvent( void* zone, const char* name, PxU16 eventId, bool compileTimeEnabled ) = 0;
		
		static PvdCommStreamInternalDataStream& create( PxAllocatorCallback& alloc, PvdOMMetaDataProvider& metaDataProvider, PxU32 bufferSize, PvdCommStreamEventBufferClient& client, PxU64 streamId );
	};
	

	template<typename TStreamType>
	struct EventStreamifier : public EventSerializer
	{
		TStreamType& mBuffer;
		EventStreamifier( TStreamType& buf ) : mBuffer( buf ) {}

		template<typename TDataType> void write( const TDataType& type ) { mBuffer.write( &type, 1 ); }
		template<typename TDataType> void write( const TDataType* type, PxU32 count ) { mBuffer.write( type, count ); }

		void writeRef( DataRef<const PxU8> data ) 
		{ 
			PxU32 amount = static_cast<PxU32>( data.size() );
			write( amount );
			write( data.begin(), amount );
		}
		void writeRef( DataRef<StringHandle> data ) 
		{
			PxU32 amount = static_cast<PxU32>( data.size() );
			write( amount );
			write( data.begin(), amount );
		}
		template<typename TDataType>
		void writeRef( DataRef<TDataType> data ) 
		{
			PxU32 amount = static_cast<PxU32>( data.size() );
			write( amount );
			for ( PxU32 idx = 0; idx < amount; ++idx )
			{
				TDataType& dtype( const_cast<TDataType&>( data[idx] ) ); 
				dtype.serialize( *this );
			}
		}
		
		virtual void streamify( PxU16& val ) { write( val ); }
		virtual void streamify( PxU8& val ) { write( val ); }
		virtual void streamify( PxU32& val ) { write( val ); }
		virtual void streamify( PxF32& val ) { write( val ); }
		virtual void streamify( PxU64& val ) { write( val ); }
		virtual void streamify( String& val ) 
		{ 
			PxU32 len = 0;
			String temp = nonNull( val );
			if ( *temp )
				len = static_cast<PxU32>( strlen( temp ) + 1 );
			write( len );
			write( val, len );
		}
		virtual void streamify( DataRef<const PxU8>& val ) { writeRef( val ); }
		virtual void streamify( DataRef<NameHandleValue>& val ) { writeRef( val ); }
		virtual void streamify( DataRef<StreamPropMessageArg>& val ) { writeRef( val ); }
		virtual void streamify( DataRef<StringHandle>& val ) { writeRef( val ); }
	private:
		EventStreamifier& operator=(const EventStreamifier&);
	};

	struct MeasureStream
	{
		PxU32 mSize;
		MeasureStream() : mSize( 0 ) {}
		template<typename TDataType> void write( const TDataType& val ) { mSize += sizeof( val ); }
		template<typename TDataType> void write( const TDataType*, PxU32 count ) { mSize += sizeof( TDataType ) * count; }
	};
	
	struct DataStreamState
	{
		enum Enum
		{
			Open,
			SetPropertyValue,
			PropertyMessageGroup,
		};
	};
	
	class ExtendedEventSerializer : public EventSerializer
	{
	protected:
		virtual ~ExtendedEventSerializer(){}
	public:
		virtual void setData( DataRef<const PxU8> eventData ) = 0;
		//True if this serializer performs byte swapping
		virtual bool performsSwap() = 0;

		virtual bool isGood() = 0;

		virtual void release() = 0;
		
		static ExtendedEventSerializer& createInputSerializer( PxAllocatorCallback& alloc, bool swapBytes );

	};
}}}

#endif
