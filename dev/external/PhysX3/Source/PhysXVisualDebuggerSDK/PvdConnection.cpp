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
#include "PvdConnection.h"
#include "PvdDataStream.h"
#include "PvdCommStreamTypes.h"
#include "PvdCommStreamEventSink.h"
#include "PvdFoundation.h"
#include "PsMutex.h"
#include "PvdConnectionManager.h"
#include "PxProfileEventBufferClient.h"
#include "PxProfileZone.h"
#include "PxProfileMemoryEventTypes.h"
#include "PvdImmediateRenderer.h"
#include "PsThread.h"

using namespace physx;
using namespace physx::debugger;
using namespace physx::debugger::comm;
using namespace physx::debugger::renderer;

namespace {

	inline PxU32 getOutStreamBufferSize()
	{
		PxU32 theCapacity = 0x20000; //128K buffer size, works out to two 64KB buffers
#if defined(PX_WINDOWS) || defined(PX_XBOXONE)
		theCapacity = 0x100000; // 1MB of buffer on windows only, works out to two 512KB buffers
#endif
		return theCapacity;
	}


	struct EventBufferClient : public PvdCommStreamEventBufferClient
	{
		PxAllocatorCallback&	mAllocator;
		PvdNetworkStreamOwner&	mOwner;
		PxU32					mRefCount;

		EventBufferClient( PxAllocatorCallback& alloc, PvdNetworkStreamOwner& owner )
			: mAllocator( alloc ), mOwner( owner ), mRefCount( 0 )
		{
			mOwner.addRef();
		}
		virtual ~EventBufferClient()
		{
			mOwner.release();
		}
		virtual void addRef()
		{
			mOwner.lock();
			++mRefCount;
			mOwner.unlock();
		}
		virtual void release()
		{
			mOwner.lock();
			if ( mRefCount ) --mRefCount;
			mOwner.unlock();
			if ( !mRefCount ) 
			{
				PVD_DELETE( mAllocator, this );
			}
		}

		virtual bool sendEventBuffer( DataRef<const PxU8> eventBuffer, PxU32 eventCount, PxU64 streamId )
		{
			PxU32 numBytes = eventBuffer.size();
			PvdNetworkOutStream& stream( lock( numBytes, eventCount, streamId ) );
			PvdError retval = stream.write( eventBuffer.begin(), numBytes );
			unlock();
			return retval == PvdErrorType::Success;
		}

		virtual PvdNetworkOutStream& lock( PxU32 numBytes, PxU32 eventCount, PxU64 streamId )
		{
			PvdNetworkOutStream& stream = mOwner.lock();
			EventGroup evt( numBytes, eventCount, streamId, Time::getCurrentCounterValue() );
			EventStreamifier<PvdNetworkOutStream> streamifier( stream );
			evt.serialize( streamifier );
			return stream;
		}

		virtual bool isConnected()
		{
			bool retval = mOwner.lock().isConnected();
			mOwner.unlock();
			return retval;
		}

		virtual void unlock()
		{
			mOwner.unlock();
		}
	private:
		EventBufferClient& operator=(const EventBufferClient&);
	};

	struct MetaDataProvider : public PvdOMMetaDataProvider
	{
		typedef MutexT<ForwardingAllocator> TMutexType;
		typedef TMutexType::ScopedLock TScopedLockType;
		typedef ForwardingHashMap<const void*,NonNegativeInteger> TInstTypeMap;
		PxAllocatorCallback&		mAllocator;
		PvdObjectModelMetaData&		mMetaData;
		TMutexType					mMutex;
		PxU32						mRefCount;
		TInstTypeMap				mTypeMap;
		PvdConnectionListener&		mListener;

		MetaDataProvider( PxAllocatorCallback& alloc, PvdConnectionListener& listener )
			: mAllocator( alloc )
			, mMetaData( PvdObjectModelMetaData::create( alloc ) )
			, mMutex( ForwardingAllocator( alloc, "MetaDataProvider::mMutex" ) )
			, mRefCount( 0 )
			, mTypeMap( alloc, "MetaDataProvider::mTypeMap" )
			, mListener( listener )
		{
			mMetaData.addRef();
		}
		virtual ~MetaDataProvider()
		{
			mMetaData.release();
		}

		virtual void addRef() 
		{ 
			TScopedLockType locker( mMutex ); 
			++mRefCount; 
		}
		virtual void release() 
		{
			{
				TScopedLockType locker( mMutex ); 
				if ( mRefCount ) --mRefCount;
			}
			if ( !mRefCount ) PVD_DELETE( mAllocator, this );
		}
		virtual PvdObjectModelMetaData& lock()
		{
			mMutex.lock();
			return mMetaData;
		}
		virtual void unlock()
		{
			mMutex.unlock();
		}
		virtual bool createInstance( const NamespacedName& clsName, const void* instance )
		{
			TScopedLockType locker( mMutex );
			Option<ClassDescription> cls( mMetaData.findClass( clsName ) );
			if ( cls.hasValue() == false ) return false;
			NonNegativeInteger instType = cls->mClassId;
			mTypeMap.insert( instance, instType );
			return true;
		}
		virtual bool isInstanceValid( const void* instance )
		{
			TScopedLockType locker( mMutex );
			ClassDescription classDesc;
			bool retval = mTypeMap.find( instance ) != NULL;
#ifdef PX_DEBUG
			if ( retval )
				classDesc = mMetaData.getClass( mTypeMap.find( instance )->second );
#endif
			return retval;
		}
		virtual void destroyInstance( const void* instance )
		{
			{
				TScopedLockType locker( mMutex );
				mTypeMap.erase( instance );
			}
			mListener.onInstanceDestroyed( instance );
		}
		virtual NonNegativeInteger getInstanceClassType( const void* instance ) 
		{
			TScopedLockType locker( mMutex );
			const TInstTypeMap::Entry* entry = mTypeMap.find( instance );
			if ( entry ) return entry->second;
			return -1;
		}
	private:
		MetaDataProvider& operator=(const MetaDataProvider&);
	};

	struct ProfileZoneClient : public PxProfileZoneClient
	{
		PxProfileZone&						mZone;
		PvdCommStreamInternalDataStream&	mStream;

		ProfileZoneClient(PxProfileZone& zone, PvdCommStreamInternalDataStream& stream)
			: mZone( zone ), mStream( stream )
		{
			mStream.addRef();
			mStream.addProfileZone( &mZone, zone.getName() );
			mStream.createInstance(&mZone);
			mZone.addClient( *this );
			PxProfileNames names( mZone.getProfileNames() );
			PVD_FOREACH( idx, names.mEventCount )
			{
				handleEventAdded( names.mEvents[idx] );
			}
			mStream.disableCache();
		}
		
		~ProfileZoneClient()
		{
			mZone.removeClient( *this );
			mStream.destroyInstance(&mZone);
			mStream.release();
		}
		static void initializeModelTypes(PvdCommStreamInternalDataStream& stream)
		{
			stream.createClass<PxProfileZone>();
			stream.createProperty<PxProfileZone,PxU8>( "events", PvdCommStreamEmbeddedTypes::getProfileEventStreamSemantic(), PropertyType::Array );

			stream.createClass<PxProfileMemoryEventBuffer>();
			stream.createProperty<PxProfileMemoryEventBuffer,PxU8>( "events", PvdCommStreamEmbeddedTypes::getMemoryEventStreamSemantic(), PropertyType::Array );

			stream.createClass<PvdImmediateRenderer>();
			stream.createProperty<PvdImmediateRenderer,PxU8>( "events", PvdCommStreamEmbeddedTypes::getRendererEventStreamSemantic(), PropertyType::Array );
		}
		
		virtual void handleEventAdded( const PxProfileEventName& inName )
		{
			mStream.addProfileZoneEvent( &mZone, inName.mName, inName.mEventId.mEventId, inName.mEventId.mCompileTimeEnabled );
		}

		virtual void handleBufferFlush( const PxU8* inData, PxU32 inLength )
		{
			mStream.setPropertyValue( &mZone, "events", inData, inLength );
		}

		virtual void handleClientRemoved()
		{
		}
	private:
		ProfileZoneClient& operator=(const ProfileZoneClient&);
	};

	struct MemoryEventClient : public PxProfileEventBufferClient
	{
		PxProfileMemoryEventBuffer&			mBuffer;
		PvdCommStreamInternalDataStream&	mStream;
		MemoryEventClient( PxAllocatorCallback& cback, PvdCommStreamInternalDataStream& stream )
			: mBuffer( PxProfileMemoryEventBuffer::createMemoryEventBuffer( cback ) )
			, mStream( stream )
		{
		}
		void initialize()
		{
			mStream.createInstance( &mBuffer );
			mStream.disableCache();
			mBuffer.addClient( *this );
		}
		virtual ~MemoryEventClient()
		{
			mStream.destroyInstance(&mBuffer);
			mBuffer.release();
			mStream.release();
		}
		void flush() { mBuffer.flushProfileEvents(); }
		void onAllocation( size_t inSize, const char* inType, const char* inFile, int inLine, void* inAddr )
		{
			mBuffer.onAllocation( inSize, inType, inFile, inLine, inAddr );
		}

		void onDeallocation( void* inAddr )
		{
			mBuffer.onDeallocation( inAddr );
		}

		void handleBufferFlush( const PxU8* inData, PxU32 inLength )
		{
			mStream.setPropertyValue( &mBuffer, "events", inData, inLength );
		}
		
		void handleClientRemoved()
		{
		}

	private:
		MemoryEventClient& operator=(const MemoryEventClient&);
	};

	struct RendererEventClient : public PxProfileEventBufferClient
	{
		PxAllocatorCallback&				mAllocator;
		PvdCommStreamInternalDataStream&	mStream;
		PvdImmediateRenderer&				mRenderer;
		RendererEventClient( PxAllocatorCallback& alloc, PvdImmediateRenderer& renderer, PvdCommStreamInternalDataStream& stream )
			: mAllocator( alloc )
			, mStream( stream )
			, mRenderer( renderer )
		{
			mStream.addRef();
			mStream.createInstance( &mRenderer );
			renderer.addClient( *this );
			mStream.disableCache();
		}
		virtual ~RendererEventClient()
		{
			mStream.destroyInstance(&mRenderer);
			mStream.release();
		}
		void handleBufferFlush( const PxU8* inData, PxU32 inLength )
		{
			mStream.setPropertyValue( &mRenderer, "events", inData, inLength );
		}
		void handleClientRemoved()
		{
			PVD_DELETE( mAllocator, this );
		}
	private:
		RendererEventClient& operator=(const RendererEventClient&);
	};

	inline PvdNetworkOutStream& createDoubleBuffered( bool doubleBuffered, PxAllocatorCallback& alloc, PvdNetworkOutStream& outStream )
	{
		if ( doubleBuffered )
			return PvdNetworkOutStream::createDoubleBuffered( alloc, outStream, getOutStreamBufferSize() );
		return outStream;
	}
	typedef MutexT<ForwardingAllocator> TMutexType;
	typedef TMutexType::ScopedLock TScopedLockType;

	struct PvdNetworkInStreamBasicSerializer
	{
		PvdNetworkInStream& mStream;
		bool				mGood;
		PvdNetworkInStreamBasicSerializer( PvdNetworkInStream& s ) : mStream( s ), mGood( true ) {}
		bool isGood() { return mGood; }
		void streamify( PxU32& val ) 
		{
			if ( mGood )
			{
				PvdError err = mStream.readBytes( reinterpret_cast<PxU8*>( &val ), sizeof( val ) );
				mGood = err == PvdErrorType::Success;
			}
		}

	private:
		PvdNetworkInStreamBasicSerializer& operator=(const PvdNetworkInStreamBasicSerializer&);
	};


	struct PvdConnectionReader : public Thread
	{
		TMutexType&					mConnectionStateMutex;
		PvdConnectionState::Enum&	mConnectionState;
		PvdNetworkStreamOwner&		mStreams;
		bool						mRunning;
		PvdConnectionReader( TMutexType& mutex, PvdConnectionState::Enum& connectionState, PvdNetworkStreamOwner& stream )
			: mConnectionStateMutex( mutex ), mConnectionState( connectionState ), mStreams( stream ), mRunning( true ) 
		{
			mStreams.addRef();
			start( 0x4000 ); //small amount of stack space, raised to 16k for linux
		}
		virtual ~PvdConnectionReader()
		{
			mRunning = false;
			waitForQuit();
			mStreams.release();
		}
		//Only called from the recv thread!!
		void setRunningState()
		{
			if ( mConnectionState != PvdConnectionState::eRECORDINGORVIEWING )
			{
				mConnectionState = PvdConnectionState::eRECORDINGORVIEWING;
				mConnectionStateMutex.unlock();
			}
		}
		//Only called from the recv thread!!
		void setPausedState()
		{
			if ( mConnectionState == PvdConnectionState::eRECORDINGORVIEWING )
			{
				mConnectionStateMutex.lock();
				mConnectionState = PvdConnectionState::ePAUSED;
			}
		}
		void handleSetPauseStateSdkEvent(PvdNetworkInStreamBasicSerializer& serializer)
		{
			SetPauseStateSdkEvent evt;
			evt.serialize( serializer );
			if ( serializer.isGood() ) //Else we just had a fail
			{
				switch( evt.getConnectionState() )
				{
				case PvdConnectionState::eRECORDINGORVIEWING: setRunningState(); break;
				case PvdConnectionState::ePAUSED: setPausedState(); break;
				default:
					PX_ASSERT( false );
					setRunningState();
					break;
				}
			}
		}
		void execute()
		{
			setName( "PvdConnectionReader::execute" );
			PX_ASSERT( mConnectionState == PvdConnectionState::eRECORDINGORVIEWING );
			//This is key.  We need to always lock the connection state mutex when the
			//connection state isn't PvdConnectionState::eRECORDINGORVIEWING
			if ( mConnectionState != PvdConnectionState::eRECORDINGORVIEWING )
				mConnectionStateMutex.lock();
			PvdNetworkInStream* inStream = mStreams.getInStream();
			if ( inStream == NULL ) { PX_ASSERT( false ); return; }
			PvdNetworkInStreamBasicSerializer serializer( *inStream );
			while( serializer.isGood() && mRunning )
			{
				PxU32 evtType;
				serializer.streamify( evtType );
				if ( serializer.isGood() )
				{
					switch( evtType )
					{
	#define DECLARE_PVD_COMM_STREAM_SDK_EVENT( name ) case PvdSdkEventTypes::name: handle##name##SdkEvent(serializer); break;
	#include "PvdCommStreamSDKEventTypes.h"
	#undef DECLARE_PVD_COMM_STREAM_SDK_EVENT
						default:
							PX_ASSERT( false );
							mRunning = false;
							break;
					}
				}
			}
			setRunningState();
			quit();
		}

	private:
		PvdConnectionReader& operator=(const PvdConnectionReader&);
	};

	struct Connection : public PvdCommStreamInternalConnection
	{

		PxAllocatorCallback&		mAllocator;
		PxAllocatorCallback&		mNonBroadcastingAllocator;
		PvdNetworkStreamOwner&		mStreams;
		EventBufferClient&			mClient;
		MetaDataProvider&			mMetaDataProvider;

		ForwardingArray<ProfileZoneClient*> mProfileZoneClients;

		PxU32						mDataStreamBufferSize;
		PxU64						mNextStreamId;
		TMutexType					mConnectionStateMutex;
		TMutexType					mRefCountMutex;
		PxU32						mRefCount;
		PvdConnectionState::Enum	mConnectionState;
		TConnectionFlagsType		mConnectionType;
		PvdConnectionListener&		mDisconnectListener;
		MemoryEventClient*			mMemoryEventClient;
		TMutexType&					mAllocationMutex;
		PvdConnectionReader*		mConnectionReader;
		bool						mConnected;
		bool						mProfileTypesInitialized;
		PvdImmediateRenderer*		mPvdImmediateRenderer;

		Connection( PxAllocatorCallback& alloc, PxAllocatorCallback& nonBroadcasting, PvdNetworkOutStream& outStream, PvdNetworkInStream* inStream
					, TConnectionFlagsType inConnectionType, PvdConnectionListener& listener
					, MutexT<ForwardingAllocator>& allocMutex, bool doubleBuffered )
			: mAllocator( alloc )
			, mNonBroadcastingAllocator( nonBroadcasting )
			, mStreams( PvdNetworkStreamOwner::create( alloc, createDoubleBuffered( doubleBuffered, alloc, outStream ), inStream ) )
			, mClient( *PVD_NEW( alloc, EventBufferClient )( alloc, mStreams ) )
			, mMetaDataProvider( *PVD_NEW( mNonBroadcastingAllocator, MetaDataProvider )( mNonBroadcastingAllocator, listener ) )
			, mProfileZoneClients( alloc, "Connection::mProfileZoneClients" )
			, mDataStreamBufferSize( 8192 )
			, mNextStreamId( 1 )
			, mConnectionStateMutex( ForwardingAllocator( alloc, "Connection::mConnectionStateMutex" ) )
			, mRefCountMutex( ForwardingAllocator( alloc, "Connection::mRefCountMutex" ) )
			, mRefCount( 0 )
			, mConnectionState( PvdConnectionState::eRECORDINGORVIEWING )
			, mConnectionType( inConnectionType )
			, mDisconnectListener( listener )
			, mMemoryEventClient( NULL )
			, mAllocationMutex( allocMutex )
			, mConnectionReader( NULL )
			, mConnected( true )
			, mProfileTypesInitialized( false )
			, mPvdImmediateRenderer( NULL )
		{
			mStreams.addRef();
			mClient.addRef();
			mMetaDataProvider.addRef(); 
			//Fire off the read thread.
			if ( mStreams.getInStream() )
				mConnectionReader = PVD_NEW( alloc, PvdConnectionReader )( mConnectionStateMutex, mConnectionState, mStreams );
		}

		virtual ~Connection()
		{
			if ( NULL != mPvdImmediateRenderer )
			{
				mPvdImmediateRenderer->release();
				mPvdImmediateRenderer = NULL;
			}

			mStreams.lock().disconnect(); 
			mStreams.unlock();
			if ( mConnectionReader != NULL )
				mConnectionReader->mRunning = false;
			PVD_DELETE( mAllocator, mConnectionReader );
			mClient.release();
			mStreams.release();
			mMetaDataProvider.release();
			PVD_FOREACH( idx, mProfileZoneClients.size() ) {
				PVD_DELETE( mAllocator, mProfileZoneClients[idx] );
			}
			mProfileZoneClients.clear();
			if ( mMemoryEventClient != NULL )
				PVD_DELETE( Foundation::getInstance().getAllocatorCallback(), mMemoryEventClient );
			mMemoryEventClient = NULL;
		}

		virtual void addRef()
		{
			TMutexType::ScopedLock __lock( mRefCountMutex );
			++mRefCount;
		}
		virtual void release()
		{
			PxU32 localCount;
			{
				TMutexType::ScopedLock __lock( mRefCountMutex );
				if ( mRefCount ) --mRefCount;
				localCount = mRefCount;
			}
			if ( !localCount )
			{
				disconnect();
				PVD_DELETE( mAllocator, this );
			}
		}

		void sendEvent( EventSerializeable& evt, PvdCommStreamEventTypes::Enum type )
		{
			MeasureStream measure;
			PvdCommStreamEventSink::writeEvent( evt, type, measure );
			PvdNetworkOutStream& outStream( mClient.lock( measure.mSize, 1, 0 ) );
			PvdCommStreamEventSink::writeEvent( evt, type, outStream );
			mClient.unlock();
		}

		template<typename TEventType>
		void sendEvent( const TEventType& evt )
		{
			sendEvent( const_cast<TEventType&>(evt), getCommStreamEventType<TEventType>() );
		}
		
		virtual void setPickable( const void* instance, bool pickable )
		{
			sendEvent( SetPickable( PVD_POINTER_TO_U64( instance ), pickable ) );
		}
		virtual void setColor( const void* instance, const PvdColor& color )
		{
			sendEvent( SetColor( PVD_POINTER_TO_U64( instance ), color ) );
		}
		virtual void setIsTopLevelUIElement( const void* instance, bool topLevel )
		{
			sendEvent( SetIsTopLevel( PVD_POINTER_TO_U64( instance ), topLevel ) );
		}
		virtual void setCamera( String name, const PxVec3& position, const PxVec3& up, const PxVec3& target )
		{
			sendEvent( SetCamera( name, position, up, target ) );
		}
		virtual void sendErrorMessage( PxU32 code, String message, String file, PxU32 line)
		{
			sendEvent( ErrorMessage( code, message, file, line ) );
		}

		virtual void sendStreamEnd()
		{
			sendEvent( StreamEndEvent() );
		}
		virtual void sendStreamInitialization()
		{
			PvdNetworkOutStream& outStream = mStreams.lock();
			StreamInitialization init;
			EventStreamifier<PvdNetworkOutStream> stream( outStream );
			init.serialize( stream );
			mStreams.unlock();
			PvdCommStreamInternalDataStream& ds( createInternalDataStream() );
			initializeProfileTypes( ds );
			ds.flush();
			ds.release();
		}

		PxU64 getNextStreamId()
		{
			mStreams.lock();
			PxU64 retval = ++mNextStreamId;
			mStreams.unlock();
			return retval;
		}
		PvdCommStreamInternalDataStream& createInternalDataStream()
		{
			return createInternalDataStream( mAllocator );
		}

		PvdCommStreamInternalDataStream& createInternalDataStream(PxAllocatorCallback& callback)
		{
			return PvdCommStreamInternalDataStream::create( callback, mMetaDataProvider, mDataStreamBufferSize, mClient, getNextStreamId() );
		}
		
		virtual PvdDataStream& createDataStream()
		{
			return createInternalDataStream();
		}

		virtual PvdImmediateRenderer& createRenderer()
		{
			if ( NULL == mPvdImmediateRenderer )
			{
				PvdCommStreamInternalDataStream& stream( createInternalDataStream() );
				PvdImmediateRenderer& retval = PvdImmediateRenderer::create( mAllocator );
				RendererEventClient* client = PVD_NEW( mAllocator, RendererEventClient)( mAllocator, retval, stream );
				(void)client;
				mPvdImmediateRenderer = &retval;
				mPvdImmediateRenderer->addRef();
			}
			return *mPvdImmediateRenderer;
		}

		//May actively change during debugging.
		//Getting this variable may block until the read thread 
		//is disconnected or releases the connection state mutex.
		virtual PvdConnectionState::Enum	getConnectionState()
		{
			TScopedLockType locker( mConnectionStateMutex );
			return mConnectionState;
		}

		//gets the connection state which will block if the system is paused.
		//checks the connection for errors and disconnects if there are any.
		virtual void checkConnection()
		{
			//Empty loop intentional
			while ( isConnected() && getConnectionState() == PvdConnectionState::ePAUSED );
			if( !isConnected() )
			{
				mDisconnectListener.onDisconnect( *this );
			}
		}


		//Will currently never change during debugging
		virtual TConnectionFlagsType getConnectionType() { return mConnectionType; }

		virtual bool isConnected() 
		{ 
			bool retval = mStreams.lock().isConnected();
			mStreams.unlock();
			return retval;
		}

		virtual void disconnect() 
		{ 
			if ( mConnected )
			{
				mConnected = false;
				mStreams.lock().disconnect(); 
				mStreams.unlock();
				mDisconnectListener.onDisconnect( *this );
			}
		}

		virtual void flush()
		{
			if ( mConnected )
			{
				PVD_FOREACH( idx, mProfileZoneClients.size() )
					mProfileZoneClients[idx]->mZone.flushProfileEvents();

				if ( mConnectionType & PvdConnectionType::eMEMORY )
				{
					mAllocationMutex.lock();
					if ( mMemoryEventClient )
						mMemoryEventClient->flush();
					mAllocationMutex.unlock();
				}
			}
		}
		
		virtual PvdNetworkOutStream& lockOutStream()
		{
			return mStreams.lock();
		}

		virtual void unlockOutStream()
		{
			mStreams.unlock();
		}

		void initializeProfileTypes(PvdCommStreamInternalDataStream& stream)
		{
			if ( mProfileTypesInitialized == false )
			{
				mProfileTypesInitialized = true;
				ProfileZoneClient::initializeModelTypes( stream );
			}
		}

		//Profile zone handler
		virtual void onZoneAdded( PxProfileZone& zone )
		{
			PvdCommStreamInternalDataStream& stream( createInternalDataStream() );
			initializeProfileTypes( stream );
			ProfileZoneClient* client = PVD_NEW( mAllocator, ProfileZoneClient )( zone, stream );
			mProfileZoneClients.pushBack( client );
		}

		virtual void onZoneRemoved( PxProfileZone& zone )
		{
			PVD_FOREACH( idx, mProfileZoneClients.size() )
			{
				if ( &zone == &mProfileZoneClients[idx]->mZone )
				{
					ProfileZoneClient* client = mProfileZoneClients[idx];
					mProfileZoneClients.replaceWithLast( idx );
					PVD_DELETE( mAllocator, client );
					return;
				}
			}
		}

		//PxAllocationListener
		void checkMemoryEventClient()
		{
			if ( mMemoryEventClient == NULL )
			{
				PxAllocatorCallback& alloc( mNonBroadcastingAllocator );
				PvdCommStreamInternalDataStream& stream( createInternalDataStream(alloc) );
				mMemoryEventClient = PVD_NEW( alloc, MemoryEventClient )( alloc, stream );
				mMemoryEventClient->initialize();
			}
		}
		
		virtual void onAllocation( size_t size, const char* typeName, const char* filename, int line, void* allocatedMemory )
		{
			if ( mConnected && ( mConnectionType & PvdConnectionType::eMEMORY ) )
			{
				checkMemoryEventClient();
				mMemoryEventClient->onAllocation( size, typeName, filename, line, allocatedMemory );
			}
		}
		virtual void onDeallocation( void* allocatedMemory )
		{
			if ( mConnected && ( mConnectionType & PvdConnectionType::eMEMORY ) )
			{
				checkMemoryEventClient();
				mMemoryEventClient->onDeallocation( allocatedMemory );
			}
		}

		virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
		{
			if ( mConnected )
			{
				sendErrorMessage( (PxU32)code, message, file, line );
				flush();
			}
		}

	private:
		Connection& operator=(const Connection&);
	};
}

PvdCommStreamInternalConnection& PvdCommStreamInternalConnection::create( PxAllocatorCallback& alloc, PxAllocatorCallback& nonBroadcastingAlloc, PvdNetworkOutStream& outStream
																			, PvdNetworkInStream* inStream, TConnectionFlagsType inConnectionType
																			, PvdConnectionListener& disconnectListener
																			, MutexT<ForwardingAllocator>& allocMutex
																			, bool doubleBuffered )
{
	return *PVD_NEW( alloc, Connection )( alloc, nonBroadcastingAlloc, outStream, inStream, inConnectionType, disconnectListener, allocMutex, doubleBuffered );
}
