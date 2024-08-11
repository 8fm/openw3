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


#ifndef PVD_OBJECT_MODEL_EVENT_STREAM_INTERNAL_H
#define PVD_OBJECT_MODEL_EVENT_STREAM_INTERNAL_H

#include "PvdObjectModelEventStream.h"
#include "PvdFoundation.h"
#include "PvdRandomAccessIOStream.h"
#include "PvdInternalByteStreams.h"

namespace physx { namespace debugger {

//Mutations required to move forward in the event stream.
class PvdObjectModelEventForwardMutator
{
protected:
	virtual ~PvdObjectModelEventForwardMutator(){}
public:
	virtual InstanceDescription idToDescriptor( PxI32 instId ) const = 0;
	virtual void createInstance( PxI32 clsId, PxI32 instId ) = 0;
	virtual void destroyInstance( PxI32 instId ) = 0;
	virtual void setPropertyValue( void* instId, PxI32 propId, const PxU8* data, PxU32 dataLen ) = 0;
	virtual void setPropertyMessage( void* instId, PxI32 msgId, const PxU8* data, PxU32 dataLen ) = 0;
	virtual void insertArrayElement( void* instId, PxI32 propId, PxI32 propIdx, const PxU8* data, PxU32 dataLen ) = 0;
	virtual void removeArrayElement( void* instId, PxI32 propId, PxI32 propIdx ) = 0;
	virtual void setArrayElementValue( void* instId, PxI32 propId, PxI32 propIdx, const PxU8* data, PxU32 dataLen ) = 0;
	virtual void originShift( void* instId, PxVec3 shift ) = 0;
};

struct PvdObjectModelEventForwardMutatorImpl : public PvdObjectModelEventForwardMutator
{
	PvdObjectModelMutator& mMutator;
	PvdObjectModelEventForwardMutatorImpl( PvdObjectModelMutator& m ) : mMutator( m ) {}
	virtual InstanceDescription idToDescriptor( PxI32 instId ) const { return mMutator.idToDescriptor(instId); }
	virtual void createInstance( PxI32 clsId, PxI32 instId ) { mMutator.createInstance( clsId, instId ); }
	virtual void destroyInstance( PxI32 instId ) { mMutator.destroyInstance( mMutator.idToDescriptor( instId ) ); }
	virtual void setPropertyValue( void* instId, PxI32 propId, const PxU8* data, PxU32 dataLen )
	{
		mMutator.setPropertyValue( instId, propId, data, dataLen, -1 );
	}
	virtual void setPropertyMessage( void* instId, PxI32 msgId, const PxU8* data, PxU32 dataLen ) 
	{
		mMutator.setPropertyMessage( instId, msgId, data, dataLen );
	}
	virtual void insertArrayElement( void* instId, PxI32 propId, PxI32 propIdx, const PxU8* data, PxU32 dataLen )
	{
		mMutator.insertArrayElement( instId, propId, propIdx, data, dataLen );
	}
	virtual void removeArrayElement( void* instId, PxI32 propId, PxI32 propIdx )
	{
		mMutator.removeArrayElement( instId, propId, propIdx );
	}
	virtual void setArrayElementValue( void* instId, PxI32 propId, PxI32 propIdx, const PxU8* data, PxU32 dataLen )
	{
		mMutator.setArrayElementValue( instId, propId, propIdx, data, dataLen, -1 );
	}
	virtual void originShift( void* instId, PxVec3 shift )
	{
		mMutator.originShift( instId, shift );
	}
};
	

class PvdObjectModelEventMutator : public PvdObjectModelEventForwardMutator
{
protected:
	virtual ~PvdObjectModelEventMutator(){}
public:
	virtual void loadDestroyedInstance( PxI32 instId ) = 0;
	virtual void applyPreviousEvent( PxI32 instanceId, PxI64 previousEvent, PxI32 inPropertyId, PxI32 inPropIdx, bool insertChangedToSetElementValue ) = 0;
	virtual void applyPreviousEventFromMessage( PxI32 instanceId, PxI64 previousEvent, PxI32 inMsgId ) = 0;
};


struct CreateInstance
{
	PxI32 mInstanceId;
	PxI32 mClassId;
	CreateInstance( PxI32 instId = -1, PxI32 clsId = -1 )
		: mInstanceId( instId )
		, mClassId( clsId )
	{
	}
	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mClassId );
	}
	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.createInstance( mClassId, mInstanceId );
	}
	void undoEvent( PvdObjectModelEventMutator& mutator )
	{
		mutator.destroyInstance( mutator.idToDescriptor( mInstanceId ).mId );
	}
};

struct DestroyInstance
{
	PxI32  mInstanceId;
	DestroyInstance( PxI32 instId = 0 ) : mInstanceId( instId ) {}
	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
	}
	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.destroyInstance( mutator.idToDescriptor( mInstanceId ).mId );
	}
	void undoEvent( PvdObjectModelEventMutator& mutator )
	{
		mutator.loadDestroyedInstance( mInstanceId );
	}
};

struct SetPropertyValue
{
	PxI32	mInstanceId;
	PxI32	mPropertyId;
	DataRef<const PxU8> mData;
	//Previous events to this object, sorted greatest to least.
	DataRef<PxU32>		mRelativePreviousEvents;

	SetPropertyValue( PxI32 instId, PxI32 propId, DataRef<const PxU8> data, DataRef<PxU32> previous )
		: mInstanceId( instId )
		, mPropertyId( propId )
		, mData( data )
		, mRelativePreviousEvents( previous )
	{
	}
	SetPropertyValue() {}
	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mPropertyId );
		s.streamify( mData );
		s.streamify( mRelativePreviousEvents );
	}

	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.setPropertyValue( mutator.idToDescriptor( mInstanceId ), mPropertyId, mData.begin(), mData.size() );
	}

	static void doUndoEvent( PxI64 inCurrentId, PvdObjectModelEventMutator& mutator, PxI32 instId, PxI32 propId, DataRef<PxU32> prevEvents, PxI32 propIdx = -1, bool insertSwitchToSetElementValue = false )
	{
		PxU32 size = prevEvents.size();
		if ( size )
		{
			PVD_FOREACH( idx, prevEvents.size() )
			{
				PxI64 previous = inCurrentId - static_cast<PxI64>( prevEvents[idx] );
				if ( previous >= 0 )
					mutator.applyPreviousEvent( instId, previous, propId, propIdx, insertSwitchToSetElementValue );
			}
		}
		else
			mutator.applyPreviousEvent( instId, 0, propId, propIdx, insertSwitchToSetElementValue );

	}

	void undoEvent( PxI64 inCurrentId, PvdObjectModelEventMutator& mutator )
	{
		doUndoEvent( inCurrentId, mutator, mInstanceId, mPropertyId, mRelativePreviousEvents );
	}
};

struct SetPropertyMessage
{
	PxI32	mInstanceId;
	PxI32	mMsgId;
	DataRef<const PxU8> mData;
	//Previous events to this object, sorted greatest to least.
	DataRef<PxU32>		mRelativePreviousEvents;
	
	SetPropertyMessage( PxI32 instId, PxI32 msgId, DataRef<const PxU8> data, DataRef<PxU32> previous )
		: mInstanceId( instId )
		, mMsgId( msgId )
		, mData( data )
		, mRelativePreviousEvents( previous )
	{
	}
	SetPropertyMessage() {}
	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mMsgId );
		s.streamify( mData );
		s.streamify( mRelativePreviousEvents );
	}

	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.setPropertyMessage( mutator.idToDescriptor( mInstanceId ), mMsgId, mData.begin(), mData.size() );
	}

	void undoEvent( PxI64 inCurrentId, PvdObjectModelEventMutator& mutator )
	{
		PxU32 size = mRelativePreviousEvents.size();
		if ( size )
		{
			PVD_FOREACH( idx, size )
			{
				PxI64 previous = inCurrentId - static_cast<PxI64>( mRelativePreviousEvents[idx] );
				if ( previous >= 0 )
					mutator.applyPreviousEventFromMessage( mInstanceId, previous, mMsgId );
			}
		}
		else
			mutator.applyPreviousEventFromMessage( mInstanceId, 0, mMsgId );
	}
};

struct InsertArrayElement
{
	PxI32	mInstanceId;
	PxI32	mPropId;
	PxI32	mPropIdx;
	DataRef<const PxU8> mData;

	InsertArrayElement( PxI32 inst, PxI32 prop, PxI32 idx, DataRef<const PxU8> data )
		: mInstanceId( inst ), mPropId( prop ), mPropIdx( idx ), mData( data ) {}
	InsertArrayElement(){}

	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mPropId );
		s.streamify( mPropIdx );
		s.streamify( mData );
	}

	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.insertArrayElement( mutator.idToDescriptor( mInstanceId ), mPropId, mPropIdx, mData.begin(), mData.size() );
	}

	void undoEvent( PxI64, PvdObjectModelEventMutator& mutator )
	{
		mutator.removeArrayElement( mutator.idToDescriptor( mInstanceId ), mPropId, mPropIdx );
	}
};

struct RemoveArrayElement
{
	PxI32	mInstanceId;
	PxI32	mPropId;
	PxI32	mPropIdx;
	DataRef<PxU32>		mRelativePreviousEvents;

	RemoveArrayElement( PxI32 inst, PxI32 prop, PxI32 idx, DataRef<PxU32> prevEvents )
		: mInstanceId( inst ), mPropId( prop ), mPropIdx( idx ), mRelativePreviousEvents( prevEvents ) {}
	RemoveArrayElement(){}

	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mPropId );
		s.streamify( mPropIdx );
		s.streamify( mRelativePreviousEvents );
	}

	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.removeArrayElement( mutator.idToDescriptor( mInstanceId ), mPropId, mPropIdx );
	}

	void undoEvent( PxI64 inCurrentId, PvdObjectModelEventMutator& mutator )
	{
		SetPropertyValue::doUndoEvent( inCurrentId, mutator, mInstanceId, mPropId, mRelativePreviousEvents, mPropIdx );
	}
};

struct SetArrayElementValue
{
	PxI32	mInstanceId;
	PxI32	mPropId;
	PxI32	mPropIdx;
	DataRef<const PxU8> mData;
	DataRef<PxU32>		mRelativePreviousEvents;
	SetArrayElementValue( PxI32 inst, PxI32 prop, PxI32 idx, DataRef<const PxU8> data, DataRef<PxU32> prevEvents )
		: mInstanceId( inst ), mPropId( prop ), mPropIdx( idx ), mData( data ), mRelativePreviousEvents( prevEvents ) {}
	SetArrayElementValue(){}

	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mPropId );
		s.streamify( mPropIdx );
		s.streamify( mData );
		s.streamify( mRelativePreviousEvents );
	}

	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		mutator.setArrayElementValue( mutator.idToDescriptor( mInstanceId ), mPropId, mPropIdx, mData.begin(), mData.size() );
	}

	void undoEvent( PxI64 inCurrentId, PvdObjectModelEventMutator& mutator )
	{
		SetPropertyValue::doUndoEvent( inCurrentId, mutator, mInstanceId, mPropId, mRelativePreviousEvents, mPropIdx, true );
	}

};

struct OriginShiftEvent
{
	PxI32	mInstanceId;
	PxU32	mShiftVec_x;
	PxU32	mShiftVec_y;
	PxU32	mShiftVec_z;

	OriginShiftEvent( PxI32 inst, PxVec3 shift ):mInstanceId( inst )
	{
		mShiftVec_x = (PxU32&)shift.x;
		mShiftVec_y = (PxU32&)shift.y;
		mShiftVec_z = (PxU32&)shift.z;
	}

	OriginShiftEvent(){}

	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mShiftVec_x );
		s.streamify( mShiftVec_y );
		s.streamify( mShiftVec_z );
	}

	void doEvent( PvdObjectModelEventForwardMutator& mutator )
	{
		PxVec3 shift( (PxF32&)mShiftVec_x, (PxF32&)mShiftVec_y, (PxF32&)mShiftVec_z);
		mutator.originShift(mutator.idToDescriptor( mInstanceId ), shift);
		//mutator.setArrayElementValue( mutator.idToDescriptor( mInstanceId ), mPropId, mPropIdx, mData.begin(), mData.size() );
	}

	void undoEvent( PxI64 /*inCurrentId*/, PvdObjectModelEventMutator& mutator )
	{
		//SetPropertyValue::doUndoEvent( inCurrentId, mutator, mInstanceId, mPropId, mRelativePreviousEvents, mPropIdx, true );
		PxVec3 shift( (PxF32&)mShiftVec_x, (PxF32&)mShiftVec_y, (PxF32&)mShiftVec_z);
		mutator.originShift(mutator.idToDescriptor( mInstanceId ), -shift);
	}

};

struct NullEvent
{
	template<typename TSerializer>
	void serialize( TSerializer& )
	{
	}
	void doEvent( PvdObjectModelEventForwardMutator& )
	{
	}
	void undoEvent( PxI64, PvdObjectModelEventMutator& )
	{
	}
};

struct Snapshot : public NullEvent {};

struct UserEvent
{
	NonNegativeInteger mType;
	DataRef<const PxU8> mData;
	UserEvent( NonNegativeInteger type, DataRef<const PxU8> data = DataRef<const PxU8>() )
		: mType( type ), mData( data ) {}
	UserEvent(){}

	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mType );
		s.streamify( mData );
	}
};

struct SavedInstance
{
	NonNegativeInteger	mInstanceId;
	PxU64				mFileOffset;
	SavedInstance( NonNegativeInteger mInstanceId, PxU64 fileOffset )
		: mInstanceId( mInstanceId ), mFileOffset( fileOffset ) {}
	SavedInstance(){}
	template<typename TSerializer>
	void serialize( TSerializer& s )
	{
		s.streamify( mInstanceId );
		s.streamify( mFileOffset );
	}
};

struct PvdObjectModelEventStreamType
{
	enum Enum
	{
		Unknown = 0,
#define DECLARE_PVD_OBJECT_MODEL_EVENT(x) x,
#include "PvdObjectModelEventStreamEventTypes.h"
		LastEvent,
#undef DECLARE_PVD_OBJECT_MODEL_EVENT
	};
};

template<PvdObjectModelEventStreamType::Enum> 
struct PvdObjectModelEventStreamTypeToDataType
{
	bool compile_error;
};
template<typename TDataType>
struct PvdObjectModelDataTypeToEventStreamType
{
	bool compile_error;
};

#define DECLARE_PVD_OBJECT_MODEL_EVENT( x )																						\
template<> struct PvdObjectModelEventStreamTypeToDataType<PvdObjectModelEventStreamType::x> { typedef x TDataType; };			\
template<> struct PvdObjectModelDataTypeToEventStreamType<x> { enum Enum { EventType = PvdObjectModelEventStreamType::x }; };
#include "PvdObjectModelEventStreamEventTypes.h"
#undef DECLARE_PVD_OBJECT_MODEL_EVENT

template<typename TDataType> PvdObjectModelEventStreamType::Enum getObjectModelEventTypeForDataType() 
{ 
	return static_cast<PvdObjectModelEventStreamType::Enum>( PvdObjectModelDataTypeToEventStreamType<TDataType>::EventType ); 
}

template<typename TOperator>
void visitPvdObjectModelEventType( PvdObjectModelEventStreamType::Enum val, TOperator op )
{
	switch( val )
	{
#define DECLARE_PVD_OBJECT_MODEL_EVENT( x )	case PvdObjectModelEventStreamType::x: op.handle##x(); return;
#include "PvdObjectModelEventStreamEventTypes.h"
#undef DECLARE_PVD_OBJECT_MODEL_EVENT
		
		default: PX_ASSERT(false);
	}
	PX_ASSERT( false );
}

struct PageFileData
{
	PxU64 mFirstEventIndex;
	PxU64 mFileOffset; //the file offset to where this page was written.
	PxU32 mEventCount;
	PxU32 mLength;
	PageFileData( PxI64 evtIdx )
		: mFirstEventIndex( static_cast<PxU64>( evtIdx ) )
		, mFileOffset( 0 )
		, mEventCount( 0 )
		, mLength( 0 )
	{
	}
	PageFileData() 
		: mFirstEventIndex( 0 )
		, mFileOffset( 0 )
		, mEventCount( 0 )
		, mLength( 0 )
	{
	}
	void beforePageWrite( PxU32 eventCount, PxU64 fileOffset )
	{
		mEventCount = eventCount;
		mFileOffset = fileOffset;
	}
	void afterPageWrite( PxU32 len )
	{
		mLength = len;
	}
};

struct SnapshotInfo
{
	PxU64	mEventIndex;
	PxU64	mFileOffset;
	PxU32	mLength;
	SnapshotInfo( PxI64 evtIdx = 0, PxU64 offset = 0, PxU32 len = 0 )
		: mEventIndex( evtIdx )
		, mFileOffset( offset )
		, mLength( len )
	{
	}
};

struct InstanceFilePosAndLength
{
	PxU64	mOffset;
	PxU32	mLength;
	InstanceFilePosAndLength( PxU64 off = 0, PxU32 len = 0 )
		: mOffset( off ), mLength( len )
	{
	}
};

#pragma pack(push)
#pragma pack(4)
 
struct InstanceMetaPair
{
	PxI32   mInstId;
	PxU64	mOffset;	
	PxU32	mLength;	
};

#pragma pack(pop)

template<typename TDataType>
static bool writeArray( PvdOutputStream& stream, const ForwardingArray<TDataType>& data )
{
	stream << (PxU32)data.size();
	if ( data.size() )
		return stream.write( data.begin(), (PxU32)data.size() );
	return true;
}

template<typename TDataType>
static bool readArray( PvdInputStream& stream, ForwardingArray<TDataType>& data )
{
	PxU32 numItems;
	stream >> numItems;
	data.resize( numItems );
	if ( numItems )
		return stream.read( data.begin(), numItems );
	return true;
}

class PvdObjectModelEventStreamMetaData
{

public:
	PxAllocatorCallback&									mAllocator;
	PxU64													mEventCount;
	PxU64													mCurrentEvent; //The last event that *HAS BEEN APPLIED*

	ForwardingArray<PageFileData>							mPageInfo;
	ForwardingArray<SnapshotInfo>							mSnapshots;
	ForwardingHashMap<PxI32, InstanceFilePosAndLength>		mInstanceDestructionMap;


	Option<PageFileData>									mWritePage;	
	MemPvdOutputStream										mPageBuffer;
	ForwardingArray<PxU32>									mCurrentPageEventOffsets;

	PxU32													mRefCount;

	PvdObjectModelEventStreamMetaData( PxAllocatorCallback& allocator )
		: mAllocator( allocator )
		, mEventCount( 0 )
		, mCurrentEvent( 0 )
		, mPageInfo( allocator, "PvdObjectModelEventStreamMetaData::mPageInfo" )
		, mSnapshots( allocator, "PvdObjectModelEventStreamMetaData::mSnapshots" )
		, mInstanceDestructionMap( allocator, "PvdObjectModelEventStreamMetaData::mInstanceDestructionMap" )
		, mPageBuffer( allocator, "PvdObjectModelEventStreamMetaData::mMetaData.mPageBuffer" )
		, mCurrentPageEventOffsets( allocator, "PvdObjectModelEventStreamMetaData::mCurrentPageEventOffsets" )
		, mRefCount( 0 )
	{
	}

	PvdObjectModelEventStreamMetaData( const PvdObjectModelEventStreamMetaData& other )
		: mAllocator( other.mAllocator )
		, mEventCount( other.mEventCount )
		, mCurrentEvent( 0 )
		, mPageInfo( other.mAllocator, "PvdObjectModelEventStreamMetaData::mPageInfo" )
		, mSnapshots( other.mAllocator, "PvdObjectModelEventStreamMetaData::mSnapshots" )
		, mInstanceDestructionMap( other.mAllocator, "PvdObjectModelEventStreamMetaData::mInstanceDestructionMap" )
		, mPageBuffer( other.mAllocator, "PvdObjectModelEventStreamMetaData::mMetaData.mPageBuffer" )
		, mCurrentPageEventOffsets( other.mAllocator, "PvdObjectModelEventStreamMetaData::mCurrentPageEventOffsets" )
		, mRefCount( 0 )
	{
		mPageInfo = other.mPageInfo;
		mSnapshots = other.mSnapshots;
		mInstanceDestructionMap = other.mInstanceDestructionMap;
	}

	void addRef() { ++mRefCount; }
	void release() { if ( mRefCount ) --mRefCount; if ( !mRefCount ) PVD_DELETE (mAllocator, this ); }
	PvdObjectModelEventStreamMetaData& clone()
	{
		return *PVD_NEW( mAllocator, PvdObjectModelEventStreamMetaData )( *this );
	}
	void save( PvdOutputStream& stream )
	{
		stream << mEventCount;
		writeArray( stream, mPageInfo );
		writeArray( stream, mSnapshots );

		ForwardingArray<InstanceMetaPair>	tmpInstanceMetaPairs(mAllocator, "temp PvdObjectModelEventStreamMetaData InstanceMetaPair");
		
		for ( ForwardingHashMap<PxI32, InstanceFilePosAndLength>::Iterator iter = mInstanceDestructionMap.getIterator();
			iter.done() == false; ++iter )
		{
			InstanceMetaPair pair;
			pair.mInstId = iter->first;
			pair.mOffset = iter->second.mOffset;
			pair.mLength = iter->second.mLength;
			tmpInstanceMetaPairs.pushBack(pair);
		}
		writeArray( stream, tmpInstanceMetaPairs);
	}
	void load( PvdInputStream& stream )
	{
		stream >> mEventCount;
		readArray( stream, mPageInfo );
		readArray( stream, mSnapshots );

		ForwardingArray<InstanceMetaPair>	tmpInstanceMetaPairs(mAllocator, "temp PvdObjectModelEventStreamMetaData InstanceMetaPair");
		readArray( stream, tmpInstanceMetaPairs);
		
		mInstanceDestructionMap.clear();		
		for ( PxU32 idx =0; idx < tmpInstanceMetaPairs.size(); ++idx )
		{
			InstanceFilePosAndLength offAndLen;
			offAndLen.mOffset = tmpInstanceMetaPairs[idx].mOffset;
			offAndLen.mLength = tmpInstanceMetaPairs[idx].mLength;
			mInstanceDestructionMap.insert( tmpInstanceMetaPairs[idx].mInstId, offAndLen );
		}
	}
};


class PvdObjectModelEventStreamTransferDataProvider
{
protected:
	virtual ~PvdObjectModelEventStreamTransferDataProvider(){}
public:
	virtual DataRef<const PxU8> getSavedInstance( PxU64 fileOffset ) = 0;
	virtual DataRef<const PxU8> getDeletedInstance( PxI32 instId ) = 0;
};


struct EventWriter
{
	PvdOutputStream& mStream;
	EventWriter( PvdOutputStream& stream ) : mStream( stream ) {}
	void streamify( NonNegativeInteger item ) { mStream << item; }
	void streamify( PxI32 item ) { mStream << item; }
	void streamify( PxU32 item ) { mStream << item; }
	void streamify( PxF32 item ) { mStream << item; }
	void streamify( PxU64 item ) { mStream << item; }
	template<typename TDataType>
	void streamify( DataRef<TDataType>& data )
	{
		PxU32 amount = data.size();
		mStream << amount;
		mStream.write( reinterpret_cast<const PxU8*>( data.begin() ), amount * sizeof( TDataType ) );
	}
};



struct EventReader
{
	MemPvdInputStream& mStream;
	EventReader( MemPvdInputStream& stream ) : mStream( stream ) {}
	void streamify( NonNegativeInteger& item ) { mStream >> item; }
	void streamify( PxI32& item ) { mStream >> item; }
	void streamify( PxU32& item ) { mStream >> item; }
	void streamify( PxU64& item ) { mStream >> item; }
	template<typename TDataType>
	void streamify( DataRef<TDataType>& data )
	{
		PxU32 amount;
		mStream >> amount;
		PxU32 numBytes = amount * sizeof( TDataType );
		if ( numBytes == 0 ) { data = DataRef<TDataType>(); return; }
		PxU8* dataPtr = NULL;
		mStream.nocopyRead( dataPtr, numBytes );
		amount = numBytes / sizeof( TDataType );
		data = DataRef<TDataType>( (TDataType*)dataPtr, amount );
	}
};

}}

#endif
