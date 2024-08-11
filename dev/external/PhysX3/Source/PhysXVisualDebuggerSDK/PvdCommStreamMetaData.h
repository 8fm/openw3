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
#ifndef PVD_COMM_STREAM_METADATA_H
#define PVD_COMM_STREAM_METADATA_H
#include "physxvisualdebuggersdk/PvdObjectModelBaseTypes.h"
#include "foundation/PxVec3.h"
#include "foundation/PxFlags.h"
#include "PvdObjectModel.h"
#include "PvdObjectModelEventStream.h"

namespace physx { namespace debugger { namespace comm {

struct CameraProps
{
	PxVec3			mPosition;
	PxVec3			mUp;
	PxVec3			mTarget;
	CameraProps( const PxVec3& pos, const PxVec3& up, const PxVec3& target )
		: mPosition( pos ), mUp( up ), mTarget( target ) {}
	CameraProps(){}
};

struct SectionListInfo
{
	PxU64	mStreamId;
	PxU32	mName; //string handle
	SectionListInfo( PxU64 sid, PxU32 name )
		: mStreamId( sid ), mName( name )
	{
	}
	SectionListInfo(){}
};

struct SectionBracket
{
	PxU64			mTimestamp;
	PxU64			mEventIndex;

	SectionBracket( PxU64 ts, PxU64 evIdx )
		: mTimestamp( ts ), mEventIndex( evIdx )
	{
	}
	SectionBracket()
		: mTimestamp( (PxU64)-1)
		, mEventIndex( (PxU64)-1)
	{
	}
	SectionBracket( const SectionBracket& other )
		: mTimestamp( other.mTimestamp )
		, mEventIndex( other.mEventIndex )
	{
	}
	SectionBracket& operator=( const SectionBracket& other )
	{
		mTimestamp = other.mTimestamp;
		mEventIndex = other.mEventIndex;
		return *this;
	}
	bool isEventValid() const { return mEventIndex != (PxU64)-1; }
	bool isTimestampValid() const { return mTimestamp != (PxU64)-1; }
	//checked access to timestamp and event index
	PxU64 getTimestamp() const { PX_ASSERT( isTimestampValid() ); return mTimestamp; }
	PxU64 getEventIndex() const { PX_ASSERT( isEventValid() ); return mEventIndex; }
};

struct Section
{
	SectionBracket mBegin;
	SectionBracket mEnd;
	Section( SectionBracket beg ) 
		: mBegin( beg )
	{
	}
	Section(){}
};

struct SectionEventData
{
	PxU64 mStreamId;
	PxU64 mTimestamp;
	PxU32 mName;
	bool mIsBegin;
	SectionEventData( PxU64 sid, PxU32 nm, PxU64 ts, bool isBegin )
		: mStreamId( sid )
		, mTimestamp( ts )
		, mName( nm )
		, mIsBegin( isBegin )
	{
	}
};

struct PvdProfileZoneEvent
{
	String	mName;
	PxU16	mEventId;
	bool	mIsCompiledTimeEnabled;
	PvdProfileZoneEvent( String nm, PxU16 eid, bool cte )
		: mName( nm ), mEventId( eid ), mIsCompiledTimeEnabled( cte ) {}
	PvdProfileZoneEvent() : mName( "" ) {}
};

struct PvdProfileZone
{
	PxU64							mStreamId;
	String							mName;
	PxI32							mInstanceId;
	DataRef<PvdProfileZoneEvent>	mEvents;

	PvdProfileZone( PxU64 sid, String nm, PxI32 iid, DataRef<PvdProfileZoneEvent> evts )
		: mStreamId( sid ), mName( nm ), mInstanceId( iid ), mEvents( evts )
	{
	}
	PvdProfileZone() : mName( "" ) {}
};

struct PvdMemoryEventEntry
{
	String	mType;
	String	mFile;
	PxU32	mLine;
	PxU32	mTotalsArrayIndex;

	PvdMemoryEventEntry( String type, String file, PxU32 line, PxU32 totalsIdx )
		: mType( type ), mFile( file ), mLine( line ), mTotalsArrayIndex( totalsIdx )
	{}
	PvdMemoryEventEntry() 
		: mType( "" ), mFile( "" ), mLine( 0 ), mTotalsArrayIndex( 0 ) 
	{}
};

struct PvdErrorMessage
{
	PxU32	mCode;
	PxU32	mMessage;	//string handle
	PxU32	mFile;		//string handle
	PxU32	mLine;
	PxU32	mIndex;
	PvdErrorMessage( PxU32 code, PxU32 message, PxU32 file, PxU32 line, PxU32 index )
		: mCode( code ), mMessage( message ), mFile( file ), mLine( line ), mIndex( index )
	{
	}
	PvdErrorMessage(){}
};

class PvdCommStreamMetaDataMutator;

class PvdCommStreamMetaData
{
protected:
	virtual ~PvdCommStreamMetaData(){}
public:
	virtual void addRef() = 0;
	virtual void release() = 0;

	
	virtual void getTimestampNanosecondConversion( PxU64& numerator, PxU64& denominator ) = 0;
	virtual PxU32 getNbInstances() = 0;
	virtual PxU32 getInstances( PxI32* instanceBuf, PxU32 bufSize, PxU32 idx = 0 ) = 0;
	virtual PxU64 getStreamId( NonNegativeInteger instance ) = 0;

	//Only works when the item with the streamid is live
	virtual NonNegativeInteger getInstanceId( PxU64 streamId ) = 0;
	//Returns the first instance that matches this id, works whether the stream
	//id is live or not.
	virtual NonNegativeInteger reverseMap( PxU64 streamId ) = 0;
	virtual bool isPickable( NonNegativeInteger instance ) = 0;
	virtual Option<PvdColor> getColor( NonNegativeInteger instance ) = 0;
	virtual bool isTopLevel( NonNegativeInteger instance ) = 0;

	virtual PxU32 getNbCameras() = 0;
	virtual PxU32 getCameras( StringHandle* buffer, PxU32 bufSize, PxU32 idx = 0 ) = 0;
	virtual Option<CameraProps> getCameraData( StringHandle name ) = 0;

	virtual PxU32 getNbSectionLists() = 0;
	virtual PxU32 getSectionLists(SectionListInfo* sectionBuffer, PxU32 bufSize, PxU32 idx = 0 ) = 0;

	virtual PxU32 getNbSections( const SectionListInfo& list ) = 0;
	virtual PxU32 getSections( const SectionListInfo& list, Section* buffer, PxU32 bufSize, PxU32 idx = 0 ) = 0;

	virtual Option<SectionListInfo> getDefaultFrameMarker() = 0;

	//Section indexes are 1 based, not 0 based!!!!
	//So list.sections[sectionCount] is a valid operation!!!
	virtual Option<Section> getSectionInfo( const SectionListInfo& list, PxU32 currentSectionIndex ) = 0;
	//Section indexes are 1 based, but evId 0 *always* corresponds to idx 0.
	//Event zero corresponds to a 'no index' return.
	//Event EventCount corresponds to SectionCount return.
	virtual PxU32 eventToSectionIndex( const SectionListInfo& list, PxU64 evId ) = 0;
	//Event 0 means none.
	//Events past lastSection.mEnd.mEventId also return none.
	Option<Section> getSectionInfoFromEvent( const SectionListInfo& list, PxU64 evId )
	{
		PxU32 idx = eventToSectionIndex( list, evId );
		return getSectionInfo( list, idx );
	}



	PxU64 getInnerSectionTime( const SectionListInfo& list, PxU64 inStartEvent )
	{
		Option<Section> section = getSectionInfoFromEvent( list, inStartEvent );
		if ( section.hasValue() 
			&& section->mBegin.isTimestampValid()
			&& section->mEnd.isTimestampValid() )
			return section->mEnd.mTimestamp - section->mBegin.mTimestamp;
		return 0;
	}
	PxU64 getTotalSectionTime( const SectionListInfo& list, PxU64 inStartEvent )
	{
		PxU32 idx = eventToSectionIndex( list, inStartEvent );
		if ( idx == 0 ) return 0;
		Option<Section> section = getSectionInfo( list, idx );
		--idx;
		Option<SectionBracket> beginSection;
		if ( idx )
		{
			Option<Section> begin = getSectionInfo( list, idx );
			if ( begin->mEnd.isTimestampValid() )
				beginSection = begin->mEnd;
		}
		else if ( section->mBegin.isTimestampValid() )
			beginSection = section->mBegin;
		if ( beginSection.hasValue() 
			&& section->mEnd.isTimestampValid())
			return section->mEnd.mTimestamp - beginSection->mTimestamp;
		return 0;
	}
	PxU64 getSectionEndTimestamp( const SectionListInfo& list, PxU64 inStartEvent )
	{
		Option<Section> section = getSectionInfoFromEvent( list, inStartEvent );
		if ( section.hasValue() && section->mEnd.isTimestampValid() ) return section->mEnd.mTimestamp;
		return 0;
	}
	//What event does the next frame correspond to.
	//If we are at the end, then we use PX_MAX_U64
	PxU64 findNextSectionEvent( const SectionListInfo& list, PxU64 inStartEvent )
	{
		PxU32 idx = eventToSectionIndex( list, inStartEvent ) + 1;
		Option<Section> section = getSectionInfo( list, idx );
		if ( section.hasValue() ) return section->mEnd.mEventIndex;
		return (PxU64)-1;
	}
	//What event does the previous frame correspond to
	PxU64 findPreviousSectionEvent( const SectionListInfo& list, PxU64 inStartEvent )
	{
		if ( inStartEvent == 0 ) return 0;
		PxU32 idx = eventToSectionIndex( list, inStartEvent );
		if ( !idx ) return 0;
		--idx;
		Option<Section> section = getSectionInfo( list, idx );
		if ( section.hasValue() ) return section->mEnd.mEventIndex;
		return 0;
	}
	//Get the total timespan of the DB capture.
	PxU64 getTotalSectionTimespan(const SectionListInfo& list)
	{
		PxU32 numSections = getNbSections( list );
		Option<Section> begin = getSectionInfo( list, 1 );
		Option<Section> end = getSectionInfo( list, numSections );
		if ( begin.hasValue() && end.hasValue() )
		{
			PxU64 beginTs = begin->mBegin.isTimestampValid() ? begin->mBegin.mTimestamp : begin->mEnd.mTimestamp;
			PxU64 endTs = end->mEnd.isTimestampValid() ? end->mEnd.mTimestamp : end->mBegin.mTimestamp;
			if ( endTs > beginTs ) return endTs - beginTs;
		}
		return 0;
	}

	Option<Section> getFrameInfo( PxU32 frameIndex ) 
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() )
			return getSectionInfo( frameMarker, frameIndex );
		return None();
	}

	PxU32 eventToFrame( PxU64 eventIdx )
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() )
			return eventToSectionIndex( frameMarker, eventIdx );
		return 0;
	}

	PxU64 frameToEvent( PxU32 frameIndex )
	{
		Option<Section> frameSection = getFrameInfo( frameIndex );
		if ( frameSection.hasValue() ) 
		{
			const Section& sectionRef( frameSection );
			if ( sectionRef.mEnd.isEventValid() ) 
				return sectionRef.mEnd.mEventIndex;
		}
		if ( frameIndex )
			return (PxU64)-1;
		else
			return 0;
	}

	PxU32 getFrameCount()
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() )
			return getNbSections( frameMarker ) + 1;
		return 0;
	}

	PxU64 getInnerFrameTime( PxU64 inStartEvent )
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() ) return getInnerSectionTime( frameMarker, inStartEvent );
		return 0;
	}
	PxU64 getTotalFrameTime( PxU64 inStartEvent )
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() ) return getTotalSectionTime( frameMarker, inStartEvent );
		return 0;
	}
	PxU64 getFrameEndTimestamp( PxU64 inStartEvent )
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() ) return getSectionEndTimestamp( frameMarker, inStartEvent );
		return 0;
	}
	//What event does the next frame correspond to.
	//If we are at the end, then we use PX_MAX_U64
	PxU64 findNextFrameEvent( PxU64 inStartEvent )
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() ) return findNextSectionEvent( frameMarker, inStartEvent );
		return 0;
	}
	//What event does the previous frame correspond to
	PxU64 findPreviousFrameEvent(PxU64 inStartEvent )
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() ) return findPreviousSectionEvent( frameMarker, inStartEvent );
		return 0;
	}
	//Get the total timespan of the DB capture.
	PxU64 getTotalFrameTimespan()
	{
		Option<SectionListInfo> frameMarker( getDefaultFrameMarker() );
		if ( frameMarker.hasValue() ) return getTotalSectionTimespan( frameMarker );
		return 0;
	}

	virtual PxU32 getNbProfileZones() = 0;
	virtual PxU32 getProfileZones( PvdProfileZone* buffer, PxU32 bufSize, PxU32 idx = 0 ) = 0;

	virtual PxU32 getNbMemoryEventEntries() = 0;
	virtual PxU32 getMemoryEventEntries( PvdMemoryEventEntry* buffer, PxU32 bufSize, PxU32 idx = 0 ) = 0;
	virtual NonNegativeInteger getMemoryEventBufferInstance() = 0;
	virtual NonNegativeInteger getMemoryTotalsInstance() = 0;

	static NamespacedName getImmediateRendererClassName() { return NamespacedName( "_debugger_", "PvdImmediateRenderer" ); }

	virtual PvdCommStreamMetaData& clone( PvdObjectModelMetaData& metaData, PvdObjectModelReader& modelReader ) = 0;
	virtual void save( PvdOutputStream& outputStream ) = 0;

	virtual PvdCommStreamMetaDataMutator& cloneForTransfer( PvdObjectModelMetaData& metaData
															, PvdObjectModelReader& modelReader
															, PvdObjectModelMutator& model
															, PvdObjectModelEventStreamUserEventListener& eventFactory ) = 0;

	static PvdCommStreamMetaData& create( PxAllocatorCallback& callback
											, PvdObjectModelMetaData& metaData
											, PvdObjectModelReader& modelReader
											, PvdInputStream& inStream );
};

class PvdCommStreamMetaDataMutator
{
protected:
	virtual ~PvdCommStreamMetaDataMutator(){}
public:
	virtual void addRef() = 0;
	virtual void release() = 0;
	virtual PvdCommStreamMetaData& getMetaData() = 0;
	virtual PvdObjectModelEventStreamUserEventTransfer& getEventTransfer() = 0;

	virtual void setTimestampNanosecondConversion( PxU64 numerator, PxU64 denominator ) = 0;
	virtual void setPickable( PxU64 streamId, bool pickable ) = 0;
	virtual void setColor( PxU64 streamId, const PvdColor& color ) = 0;
	virtual void setIsTopLevel( PxU64 streamId, bool isTopLevel ) = 0;
	virtual void setInstanceId( PxU64 streamId, NonNegativeInteger instance ) = 0;
	virtual void destroyInstance( PxU64 streamId ) = 0;
	virtual void setCamera( StringHandle name, const PxVec3& pos, const PxVec3& up, const PxVec3& target ) = 0;
	virtual void sendErrorMessage( PxU32 code, StringHandle message, StringHandle file, PxU32 name ) = 0;
	//odd event streams are dealt with as follows.
	//
	//begin,begin - The previous section's end event will equal ts, and its eventId will be one less.
	//
	//end,end - The current event's begin ts will be the previous event's end ts, and its 
	//begin eventId will be one more than the previous events end event id.
	virtual void addSectionEvent( PxU64 streamId, StringHandle name, bool isBegin, PxU64 timestamp ) = 0;
	//Delete all section event information as it no longer applies.
	virtual void onRecordingStopped() = 0;
	virtual void onRecordingStarted() = 0;
	virtual void addProfileZone( PxU64 sid, String nm ) = 0;
	virtual void addProfileZoneEvent( PxU64 sid, String nm, PxU16 evId, bool enabled ) = 0;
	
	virtual void addMemoryEventEntry( String type, String file, PxU32 line, PxU32 totalsArrayIndex ) = 0;
	//events on the event buffer instance, 'totals' on the totals instance
	virtual void setMemoryEventInstances( NonNegativeInteger eventBuffer, NonNegativeInteger totals ) = 0;

	static PvdCommStreamMetaDataMutator& create( PxAllocatorCallback& callback
												, PvdObjectModelMetaData& metaData
												, PvdObjectModelReader& modelReader
												, PvdObjectModelMutator& model
												, PvdObjectModelEventStreamUserEventListener& eventFactory );
};

}}}
#endif
