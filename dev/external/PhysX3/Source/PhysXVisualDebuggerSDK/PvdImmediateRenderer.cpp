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
#include "PvdImmediateRenderer.h"
#include "PvdImmediateRenderImpl.h"
#include "PvdFoundation.h"
#include "PvdInternalByteStreams.h"
#include "PvdBits.h"

using namespace physx;
using namespace physx::profile; 
using namespace physx::debugger;
using namespace physx::debugger::renderer;


namespace {

	template<typename TStreamType>
	struct RenderWriter : public RenderSerializer
	{
		TStreamType& mStream;
		RenderWriter( TStreamType& stream )
			: mStream( stream ) {}
		template<typename TDataType>
		void write( const TDataType* val, PxU32 count )
		{
			PxU32 numBytes = count * sizeof( TDataType );
			mStream.write( reinterpret_cast<const PxU8*>( val ), numBytes );
		}
		template<typename TDataType>
		void write( const TDataType& val ) { write( &val, 1 ); }

		template<typename TDataType>
		void writeRef( DataRef<TDataType>& val )
		{
			PxU32 amount = val.size();
			write( amount );
			if ( amount )
				write( val.begin(), amount );
		}
		
		virtual void streamify( PxU64& val ) { write( val ); }
		virtual void streamify( PxF32& val ) { write( val ); }
		virtual void streamify( PxU8& val )  { write( val ); }
		virtual void streamify( DataRef<PvdPoint>& val ) { writeRef( val ); }
		virtual void streamify( DataRef<PvdLine>& val ) { writeRef( val ); }
		virtual void streamify( DataRef<PvdTriangle>& val ) { writeRef( val ); }
		virtual PxU32 hasData() { return false; }
		virtual bool isGood() { return true; }

	private:
		RenderWriter& operator=(const RenderWriter&);
		
	};

	struct ImmediateRenderer : public PvdImmediateRenderer
	{

		PxAllocatorCallback& mAllocator;
		ForwardingMemoryBuffer mBuffer;
		PxU32 mBufferCapacity;
		PxU32 mRefCount;
		ForwardingArray<PxProfileEventBufferClient*> mClients;
		ImmediateRenderer( PxAllocatorCallback& alloc, PxU32 bufferFullAmount )
			: mAllocator( alloc )
			, mBuffer( alloc, "ImmediateRenderBuffer" )
			, mBufferCapacity( bufferFullAmount )
			, mRefCount( 0 )
			, mClients( alloc, "PvdImmediateRenderer::Clients" )
		{
		}
		virtual ~ImmediateRenderer()
		{
			PVD_FOREACH( idx, mClients.size() )
				mClients[idx]->handleClientRemoved();
			mClients.clear();
		}
		virtual void addRef() { ++mRefCount; }
		virtual void release() 
		{ 
			if ( mRefCount ) --mRefCount;
			if ( !mRefCount ) PVD_DELETE( mAllocator, this ); 
		}

		template<typename TEventType>
		void handleEvent( TEventType evt )
		{
			RenderWriter<ForwardingMemoryBuffer> _writer( mBuffer );
			RenderSerializer& writer( _writer );

			PvdImmediateRenderTypes::Enum evtType( getPvdRenderTypeFromType<TEventType>() );
			writer.streamify( evtType );
			evt.serialize( writer );
			if ( mBuffer.size() >= mBufferCapacity )
				flushRenderEvents();
		}
		virtual void setInstanceId( const void* iid )
		{
			handleEvent( SetInstanceIdRenderEvent( PVD_POINTER_TO_U64( iid ) ) );
		}
		//Draw these points associated with this instance
		virtual void drawPoints( const PvdPoint* points, PxU32 count )
		{
			handleEvent( PointsRenderEvent( points, count ) );
		}
		//Draw these lines associated with this instance
		virtual void drawLines( const PvdLine* lines, PxU32 count )
		{
			handleEvent( LinesRenderEvent( lines, count ) );
		}
		//Draw these triangles associated with this instance
		virtual void drawTriangles( const PvdTriangle* triangles, PxU32 count )
		{
			handleEvent( TrianglesRenderEvent( triangles, count ) );
		}
		
		//Constraint visualization routines
		virtual void visualizeJointFrames( const PxTransform& parent, const PxTransform& child )
		{
			handleEvent( JointFramesRenderEvent( parent, child ) );
		}
		virtual void visualizeLinearLimit( const PxTransform& t0, const PxTransform& t1, PxF32 value, bool active )
		{
			handleEvent( LinearLimitRenderEvent( t0, t1, value, active ) );
		}
		virtual void visualizeAngularLimit( const PxTransform& t0, PxF32 lower, PxF32 upper, bool active)
		{
			handleEvent( AngularLimitRenderEvent( t0, lower, upper, active ) );
		}
		virtual void visualizeLimitCone( const PxTransform& t, PxF32 ySwing, PxF32 zSwing, bool active)
		{
			handleEvent( LimitConeRenderEvent( t, ySwing, zSwing, active ) );
		}
		virtual void visualizeDoubleCone( const PxTransform& t, PxF32 angle, bool active)
		{
			handleEvent( DoubleConeRenderEvent( t, angle, active ) );
		}
		//Clear the immedate buffer.
		virtual void flushRenderEvents()
		{
			if ( mClients.size() )
			{
				PVD_FOREACH( idx, mClients.size() )
					mClients[idx]->handleBufferFlush( mBuffer.begin(), mBuffer.size() );
			}
			mBuffer.clear();
		}
		virtual void addClient( PxProfileEventBufferClient& inClient ) 
		{
			mClients.pushBack( &inClient );
		}
		virtual void removeClient( PxProfileEventBufferClient& inClient ) 
		{
			PVD_FOREACH( idx, mClients.size() )
			{
				if ( mClients[idx] == &inClient )
				{
					inClient.handleClientRemoved();
					mClients.replaceWithLast( idx );
					break;
				}
			}
		}
		virtual bool hasClients() const
		{
			return mClients.size() > 0;
		}
	private:
		ImmediateRenderer& operator=(const ImmediateRenderer&);
	};

	
	template<bool swapBytes>
	struct RenderReader : public RenderSerializer
	{
		MemPvdInputStream			mStream;
		ForwardingMemoryBuffer&		mBuffer;

		
		RenderReader( ForwardingMemoryBuffer& buf ) : mBuffer ( buf ) {}
		void setData( DataRef<const PxU8> data )
		{
			mStream.setup( const_cast<PxU8*>( data.begin() ), const_cast<PxU8*>( data.end() ) );
		}
		virtual void streamify( PxU64& val ) { mStream >> val; }
		virtual void streamify( PxF32& val ) { mStream >> val; }
		virtual void streamify( PxU8& val )  { mStream >> val; }
		template<typename TDataType>
		void readRef( DataRef<TDataType>& val )
		{
			PxU32 count;
			mStream >> count;
			PxU32 numBytes = sizeof( TDataType ) * count;
			
			TDataType* dataPtr = reinterpret_cast<TDataType*>( mBuffer.growBuf(numBytes) );
			mStream.read( reinterpret_cast<PxU8*>(dataPtr), numBytes );
			val = DataRef<TDataType>( dataPtr, count );
		}
		virtual void streamify( DataRef<PvdPoint>& val ) 
		{
			readRef( val );
		}
		virtual void streamify( DataRef<PvdLine>& val )
		{
			readRef( val );
		}
		virtual void streamify( DataRef<PvdTriangle>& val )
		{
			readRef( val );
		}
		virtual bool isGood() { return mStream.isGood(); }
		virtual PxU32 hasData() { return mStream.size() > 0; }
	private:
		RenderReader& operator=(const RenderReader&);
	};

	template<> struct RenderReader<true> : public RenderSerializer
	{
		MemPvdInputStream			mStream;
		ForwardingMemoryBuffer&		mBuffer;
		RenderReader( ForwardingMemoryBuffer& buf ) : mBuffer ( buf ) {}
		void setData( DataRef<const PxU8> data )
		{
			mStream.setup( const_cast<PxU8*>( data.begin() ), const_cast<PxU8*>( data.end() ) );
		}

		template<typename TDataType>
		void read( TDataType& val ) { mStream >> val; swapBytes( val ); }
		virtual void streamify( PxU64& val ) { read( val ); }
		virtual void streamify( PxF32& val ) { read( val ); }
		virtual void streamify( PxU8& val )  { read( val ); }
		template<typename TDataType>
		void readRef( DataRef<TDataType>& val )
		{
			PxU32 count;
			mStream >> count;
			swapBytes( count );
			PxU32 numBytes = sizeof( TDataType ) * count;
			
			TDataType* dataPtr = reinterpret_cast<TDataType*>( mBuffer.growBuf(numBytes) );
			PVD_FOREACH( idx, count )
				RenderSerializerMap<TDataType>().serialize( *this, dataPtr[idx] );
			val = DataRef<TDataType>( dataPtr, count );
		}
		virtual void streamify( DataRef<PvdPoint>& val ) 
		{
			readRef( val );
		}
		virtual void streamify( DataRef<PvdLine>& val )
		{
			readRef( val );
		}
		virtual void streamify( DataRef<PvdTriangle>& val )
		{
			readRef( val );
		}
		virtual bool isGood() { return mStream.isGood(); }
		virtual PxU32 hasData() { return mStream.size() > 0; }
	private:
		RenderReader& operator=(const RenderReader&);
	};

	template<bool swapBytes>
	struct Parser : public PvdImmediateRenderParser
	{
		PxAllocatorCallback&	mAllocator;
		ForwardingMemoryBuffer	mBuffer;
		RenderReader<swapBytes> mReader;
		Parser( PxAllocatorCallback& alloc )
			: mAllocator( alloc )
			, mBuffer( alloc, "PvdImmediateRenderParser::mBuffer" )
			, mReader( mBuffer )
		{
		}
		
		void release() { PVD_DELETE( mAllocator, this ); }
		void parseData( DataRef<const PxU8> data, PvdImmediateRenderHandler& handler ) 
		{
			mReader.setData( data );
			RenderSerializer& serializer( mReader );
			while( serializer.isGood() && serializer.hasData() )
			{
				mReader.mBuffer.clear();
				PvdImmediateRenderTypes::Enum evtType = PvdImmediateRenderTypes::Unknown;
				serializer.streamify( evtType );
				switch( evtType )
				{
	#define DECLARE_PVD_IMMEDIATE_RENDER_TYPE( type )		\
				case PvdImmediateRenderTypes::type:         \
					{										\
						type##RenderEvent evt;				\
						evt.serialize( serializer );		\
						handler.handleRenderEvent( evt );	\
					}										\
					break;
	#include "PvdImmediateRenderTypes.h"
	#undef DECLARE_PVD_IMMEDIATE_RENDER_TYPE
				default: PX_ASSERT( false ); return;

				}
			}
			PX_ASSERT( serializer.isGood() );
			return;
		}
	};
}

PvdImmediateRenderer& PvdImmediateRenderer::create( PxAllocatorCallback& alloc, PxU32 bufferSize )
{
	return *PVD_NEW( alloc, ImmediateRenderer)( alloc, bufferSize ) ;
}
PvdImmediateRenderParser& PvdImmediateRenderParser::create( PxAllocatorCallback& alloc, bool swapBytes )
{
	if ( swapBytes ) return *PVD_NEW( alloc, Parser<true> )( alloc );
	else return *PVD_NEW( alloc, Parser<false> )( alloc );
}

