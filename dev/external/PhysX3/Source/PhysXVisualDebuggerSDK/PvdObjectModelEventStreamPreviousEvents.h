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

#ifndef PVD_OBJECT_MODEL_EVENT_STREAM_PREVIOUS_EVENTS_H
#define PVD_OBJECT_MODEL_EVENT_STREAM_PREVIOUS_EVENTS_H
#include "PvdObjectModelBaseTypes.h"
#include "PvdFoundation.h"

namespace physx { namespace debugger {

	struct IntersectResult
	{
		bool mIntersects;
		bool mReplaces;
		bool mStopSearching;
		IntersectResult( bool is, bool rep, bool stopSearch ) : mIntersects( is ), mReplaces( rep ), mStopSearching( stopSearch) {}
		IntersectResult() : mIntersects( false ), mReplaces( false ), mStopSearching( false ) {}
	};
	struct SnapshotPreviousEvent 
	{
		IntersectResult intersectWith( NonNegativeInteger ) { return IntersectResult( true, false, true ); }
		IntersectResult intersectWith( NonNegativeInteger, const PropertyMessageDescription& ) { return IntersectResult( true, false, true ); }
		IntersectResult intersectWithArrayElement( NonNegativeInteger, NonNegativeInteger ) { return IntersectResult( true, false, true ); }
	};

	struct CreateInstancePreviousEvent
	{
		IntersectResult intersectWith( NonNegativeInteger ) { return IntersectResult( true, false, true ); }
		IntersectResult intersectWith( NonNegativeInteger, const PropertyMessageDescription& ) { return IntersectResult( true, false, true ); }
		IntersectResult intersectWithArrayElement( NonNegativeInteger, NonNegativeInteger ) { return IntersectResult( true, false, true ); }
	};

	struct SetPropertyValuePreviousEvent 
	{ 
		NonNegativeInteger mPropId; 
		SetPropertyValuePreviousEvent( NonNegativeInteger prop ) : mPropId( prop ) {}
		IntersectResult intersectWith( NonNegativeInteger propId ) 
		{
			if ( mPropId == propId ) return IntersectResult( true, true, true );
			return IntersectResult();
		}
		IntersectResult intersectWith( NonNegativeInteger, const PropertyMessageDescription& msg) 
		{ 
			PxU32 numIncoming( msg.mProperties.size() );
			for ( PxU32 idx = 0; idx < numIncoming; ++idx )
				if ( msg.mProperties[idx].mProperty.mPropertyId == mPropId )
					return IntersectResult( true, true, false );
			return IntersectResult(); 
		}
		IntersectResult intersectWithArrayElement( NonNegativeInteger propId, NonNegativeInteger ) 
		{ 
			if ( propId == mPropId ) return IntersectResult( true, false, true ); 
			return IntersectResult();
		}
	};

	struct SetPropertyMessagePreviousEvent
	{
		const PvdObjectModelMetaData*	mMetaData;
		NonNegativeInteger				mMsgId;
		SetPropertyMessagePreviousEvent( const PvdObjectModelMetaData* metaData, NonNegativeInteger msgId ) : mMetaData( metaData ), mMsgId( msgId ) {}
		IntersectResult intersectWith( NonNegativeInteger propId ) 
		{
			PropertyMessageDescription message = mMetaData->getPropertyMessage( mMsgId );
			PxU32 numProps = message.mProperties.size();
			for ( PxU32 idx = 0; idx < numProps; ++idx )
				if ( message.mProperties[idx].mProperty.mPropertyId == propId )
					return IntersectResult( true, false, true );
			return IntersectResult();
		}
		IntersectResult intersectWith( NonNegativeInteger msgId, const PropertyMessageDescription& desc )
		{
			if ( mMsgId == msgId ) return IntersectResult( true, true, true );
			PropertyMessageDescription message = mMetaData->getPropertyMessage( mMsgId );
			//If any of the incoming properties overlap with one of our properties, then we return true, false.
			PxU32 numIncoming( desc.mProperties.size() );
			PxU32 numCurrent( message.mProperties.size() );
			for ( PxU32 idx = 0; idx < numIncoming; ++idx )
			{
				for ( PxU32 msgIdx = 0; msgIdx < numCurrent; ++msgIdx )
				{
					if ( message.mProperties[msgIdx].mProperty.mPropertyId == desc.mProperties[idx].mProperty.mPropertyId )
						return IntersectResult( true, false, false );
				}
			}
			return IntersectResult();
		}
		IntersectResult intersectWithArrayElement( NonNegativeInteger propId, NonNegativeInteger ) { return intersectWith( propId ); }
	};

	struct ArrayElementPreviousEvent
	{
		NonNegativeInteger mPropId;
		NonNegativeInteger mPropIdx;
		ArrayElementPreviousEvent( NonNegativeInteger propId, NonNegativeInteger propIdx ) : mPropId( propId ), mPropIdx( propIdx ) {}
		IntersectResult intersectWith( NonNegativeInteger propId ) 
		{
			if ( propId == mPropId ) return IntersectResult( true, true, false );
			return IntersectResult();
		}
		IntersectResult intersectWith( NonNegativeInteger, const PropertyMessageDescription& desc )
		{
			PxU32 numIncoming( desc.mProperties.size() );
			for ( PxU32 idx = 0; idx < numIncoming; ++idx )
			{
				if ( desc.mProperties[idx].mProperty.mPropertyId == mPropId ) 
					return IntersectResult( true, true, false );
			}
			return IntersectResult();
		}
	};

	struct InsertArrayElementPreviousEvent : public ArrayElementPreviousEvent
	{
		InsertArrayElementPreviousEvent( NonNegativeInteger propId, NonNegativeInteger propIdx ) : ArrayElementPreviousEvent( propId, propIdx ) {}
		IntersectResult intersectWithArrayElement( NonNegativeInteger propId, NonNegativeInteger& propIdx ) 
		{ 
			if ( propId != mPropId ) return IntersectResult();
			if ( propId == mPropId && propIdx == mPropIdx ) return IntersectResult( true, true, true ); //we created the element
			//Going back in time, this element's index was one lower before the insert.
			if ( (PxI32)propIdx > mPropIdx )
				propIdx = propIdx.getValue() - 1;
			//We didn't influence that property value.
			return IntersectResult( false, false, false );
		}
	};
	
	struct SetArrayElementValuePreviousEvent : public ArrayElementPreviousEvent
	{
		SetArrayElementValuePreviousEvent( NonNegativeInteger propId, NonNegativeInteger propIdx ) : ArrayElementPreviousEvent( propId, propIdx ) {}
		IntersectResult intersectWithArrayElement( NonNegativeInteger propId, NonNegativeInteger& propIdx ) 
		{ 
			if ( propId == mPropId && propIdx == mPropIdx ) return IntersectResult( true, true, false ); //we created the element
			//We don't mangle the property indexes because we don't change them.
			return IntersectResult();
		}
	};
	
	struct RemoveArrayElementPreviousEvent : public ArrayElementPreviousEvent
	{
		RemoveArrayElementPreviousEvent( NonNegativeInteger propId, NonNegativeInteger propIdx ) : ArrayElementPreviousEvent( propId, propIdx ) {}

		//It isn't clear how a remove array element could intersect with a later event.
		IntersectResult intersectWithArrayElement( NonNegativeInteger propId, NonNegativeInteger& propIdx ) 
		{
			if ( propId == mPropId && 
				propIdx >= mPropIdx )
			{
				propIdx = propIdx.getValue() + 1;
			}
			return IntersectResult();
		}
	};


	struct PreviousEventTypes
	{
		enum Enum
		{
#define DECLARE_PREVIOUS_EVENT_TYPE( x ) x,
#include "PvdObjectModelEventStreamPreviousEventTypes.h"
#undef DECLARE_PREVIOUS_EVENT_TYPE
		};
	};

	template<typename TDataType> struct PreviousEventDatatypeToType { bool compile_error; };
	template<PreviousEventTypes::Enum> struct PreviousEventTypeToDatatype { bool compile_error; };

#define DECLARE_PREVIOUS_EVENT_TYPE( x ) \
	template<> struct PreviousEventDatatypeToType<x##PreviousEvent> { enum Enum { EventType = PreviousEventTypes::x }; };	\
	template<> struct PreviousEventTypeToDatatype<PreviousEventTypes::x> { typedef x##PreviousEvent TEventType; };			
#include "PvdObjectModelEventStreamPreviousEventTypes.h"
#undef DECLARE_PREVIOUS_EVENT_TYPE


	template<typename TDataType> PreviousEventTypes::Enum getPreviousEventType() { return static_cast<PreviousEventTypes::Enum>( PreviousEventDatatypeToType<TDataType>::EventType ); }
	template<typename TVisitor> void visitPreviousEventType( PreviousEventTypes::Enum val, TVisitor& visitor )
	{
		switch( val )
		{
#define DECLARE_PREVIOUS_EVENT_TYPE( x ) case PreviousEventTypes::x: visitor.template handle<x##PreviousEvent>(); return;
#include "PvdObjectModelEventStreamPreviousEventTypes.h"
#undef DECLARE_PREVIOUS_EVENT_TYPE
		}
		PX_ASSERT( false );
	}
	class PreviousEvent;

	struct IntersectsWithVisitor
	{
		NonNegativeInteger	mPropId;
		const Union<16>&	mEventData;
		IntersectResult		mReturnValue;
		IntersectsWithVisitor( NonNegativeInteger pid, const Union<16>& evtData )
			: mPropId( pid )
			, mEventData( evtData )
		{
		}

		template<typename TDataType>
		void handle()
		{
			mReturnValue = mEventData.get<TDataType>().intersectWith( mPropId );
		}
		IntersectResult result() { return mReturnValue; }
	};
	
	struct PropertyMessageIntersectsWithVisitor
	{
		NonNegativeInteger					mMsgId;
		const PropertyMessageDescription&	mMsg;
		const Union<16>&					mEventData;
		IntersectResult						mReturnValue;
		PropertyMessageIntersectsWithVisitor( NonNegativeInteger _msg, const PropertyMessageDescription& desc, const Union<16>& evtData )
			: mMsgId( _msg )
			, mMsg( desc )
			, mEventData( evtData )
		{
		}
		template<typename TDataType>
		void handle()
		{
			mReturnValue = mEventData.get<TDataType>().intersectWith( mMsgId, mMsg );
		}
		IntersectResult result() { return mReturnValue; }
	};
	
	struct ArrayElementIntersectsWithVisitor
	{
		NonNegativeInteger					mPropId;
		NonNegativeInteger&					mPropIdx;
		const Union<16>&					mEventData;
		IntersectResult						mReturnValue;
		ArrayElementIntersectsWithVisitor( NonNegativeInteger _pid, NonNegativeInteger& _pidx, const Union<16>& evtData )
			: mPropId( _pid )
			, mPropIdx( _pidx )
			, mEventData( evtData )
		{
		}
		template<typename TDataType>
		void handle()
		{
			mReturnValue = mEventData.get<TDataType>().intersectWithArrayElement( mPropId, mPropIdx );
		}
		IntersectResult result() { return mReturnValue; }
	};


	class PreviousEvent
	{
		PxU64						mEventId;
		Union<16>					mEventData;
		mutable PreviousEvent*		mPreviousEvent;
		PreviousEventTypes::Enum	mEventType;
	public:

		template<typename TDataType>
		PreviousEvent( PxU64 evid, const TDataType& inType ) 
			: mEventId( evid )
			, mPreviousEvent( NULL ) 
			, mEventType( getPreviousEventType<TDataType>() )
		{
			mEventData.set( inType );
		}

		PxU64 getEventId()	const								{ return mEventId; }
		PreviousEventTypes::Enum getType() const				{ return mEventType; }
		PreviousEvent* getPreviousEvent() const					{ return mPreviousEvent; }
		void setPreviousEvent( PreviousEvent* item ) const		{ mPreviousEvent = item; }

		struct IIntersectionOp
		{
			virtual ~IIntersectionOp(){}
			virtual IntersectResult handleEvent( PreviousEvent& evt ) = 0;
		};

		void resolve( ForwardingArray<PxU32>& outIds, ForwardingPool<PreviousEvent>& ioPool, IIntersectionOp& op ) const
		{
			PreviousEvent* currentEvent = mPreviousEvent;
			const PreviousEvent* nextEvent = this;
			PxU64 mId = getEventId();
			while( currentEvent )
			{
				IntersectResult intersection( op.handleEvent( *currentEvent ) );
				if ( intersection.mIntersects )
				{
					outIds.pushBack( static_cast<PxU32>( mId - currentEvent->getEventId() ) );
					if ( intersection.mReplaces ) //then we completely superceded the previous event.
					{
						nextEvent->setPreviousEvent( currentEvent->getPreviousEvent() );
						ioPool.deallocate( currentEvent );
						currentEvent = const_cast<PreviousEvent*>( nextEvent );
					}
					if ( intersection.mStopSearching )
						return;
				}
				nextEvent = currentEvent;
				currentEvent = currentEvent->getPreviousEvent();
			}
		}

		IntersectResult intersectWith( NonNegativeInteger incomingDescription ) const
		{
			IntersectsWithVisitor visitor( incomingDescription, mEventData );
			visitPreviousEventType( mEventType, visitor );
			return visitor.result();
		}
		
		IntersectResult intersectWith( NonNegativeInteger msgId, const PropertyMessageDescription& msg ) const
		{
			PropertyMessageIntersectsWithVisitor visitor( msgId, msg, mEventData );
			visitPreviousEventType( mEventType, visitor );
			return visitor.result();
		}
		IntersectResult intersectWithArrayElement( NonNegativeInteger propId, NonNegativeInteger& propIdx ) 
		{
			ArrayElementIntersectsWithVisitor visitor( propId, propIdx, mEventData );
			visitPreviousEventType( mEventType, visitor );
			return visitor.result();
		}
	};
	
	typedef ForwardingPool<PreviousEvent> TPreviousEventPool;


}}


#endif