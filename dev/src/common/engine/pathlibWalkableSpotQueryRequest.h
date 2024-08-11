/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/refCountPointer.h"

#include "pathlibConst.h"


namespace PathLib
{

class CWalkableSpotQueryRequest;

class IWalkableSpotQueryCallback
{
public:
	typedef TRefCountPointer< IWalkableSpotQueryCallback > Ptr;
private:
	typedef Red::Threads::CAtomic< Int32 > AtomicInt;

	AtomicInt							m_refCount;
	Ptr									m_nextCallback;

protected:
	virtual void		Callback( CWalkableSpotQueryRequest* request ) = 0;

	virtual ~IWalkableSpotQueryCallback();

public:
	IWalkableSpotQueryCallback();

	void				AddRef()											{ m_refCount.Increment(); }
	void				Release()											{ if ( m_refCount.Decrement() == 0 ) { delete this; } }

	void				AddNextCallback( Ptr callback );

	void				OnQueryCompleted( CWalkableSpotQueryRequest* request );
};

class CWalkableSpotQueryRequest
{
	friend class CNodeFinder;
	friend class CWalkableSpotQueryTask;
private:
	typedef Red::Threads::CAtomic< Int32 > AtomicInt;
	typedef Red::Threads::CAtomic< Bool > AtomicBool;

	AtomicInt							m_refCount;
	AtomicInt							m_queryState;
public:
	typedef Uint8 Flags;
protected:

	// low-level input (that is computed b4 making a virtual call)
	IWalkableSpotQueryCallback::Ptr		m_callback;
	Box									m_testBox;
	Vector3								m_destinationPos;
	Vector3								m_sourcePos;
	Float								m_personalSpace;
	Float								m_maxDist;
	Uint32								m_category;
	Uint32								m_taskPriority;

	CollisionFlags						m_collisionFlags;					// collision flags for spatial queries
	NodeFlags							m_forbiddenNodeFlags;				// navgraph flags for pathfind queries

	// output
	Vector3								m_output;
	AreaId								m_outAreaId;
	Bool								m_outputSuccess;
	// setup flags
	Flags								m_flags;


protected:
	virtual ~CWalkableSpotQueryRequest()									{}

public:
	enum EState
	{
		STATE_DISPOSED,
		STATE_SETUP,
		STATE_ONGOING,
		STATE_COMPLETED_SUCCESS,
		STATE_COMPLETED_FAILURE,
	};

	enum EFlags : Uint8
	{
		FLAG_BAIL_OUT_ON_SUCCESS									= FLAG( 0 ),
		FLAG_REQUIRE_REACHABLE_FROM_SOURCE							= FLAG( 1 ),
		FLAG_SYNCHRONOUS_COMPLETION_CALLBACK						= FLAG( 2 ),
		FLAG_DIST3D													= FLAG( 3 ),
		FLAG_ONLY_BASEAREA											= FLAG( 4 ),
		FLAG_CLOSEST_POSITION_LINETEST								= FLAG( 5 ),

		FLAGS_DEFAULT												= 0
	};


	typedef TRefCountPointer< CWalkableSpotQueryRequest > Ptr;

	CWalkableSpotQueryRequest();

	void				Setup( CPathLibWorld& pathlib, const Box& testBox, const Vector3& destinationPos, const Vector3& sourcePos, Float personalSpace, Float maxDist = 256.f, Flags flags = FLAGS_DEFAULT, CollisionFlags collisionFlags = CT_DEFAULT, NodeFlags forbiddenNodeFlags = NFG_FORBIDDEN_BY_DEFAULT, Uint32 taskPriority = 1023 );
	void				Submit( CPathLibWorld& pathlib );					// start processing request
	void				Dispose()											{ SetQueryState( STATE_DISPOSED ); }
	void				AddCallback( IWalkableSpotQueryCallback::Ptr callback );

	void				AddRef()											{ m_refCount.Increment(); }
	void				Release()											{ if ( m_refCount.Decrement() == 0 ) { delete this; } }

	EState				GetQueryState()	const								{ return EState( m_queryState.GetValue() ); }
	void				SetQueryState( EState state )						{ m_queryState.SetValue( state ); }
	void				QueryCompleted()									{ m_queryState.SetValue( m_outputSuccess ? STATE_COMPLETED_SUCCESS : STATE_COMPLETED_FAILURE ); }

	Bool				IsQueryCompleted() const							{ return GetQueryState() >= STATE_COMPLETED_SUCCESS; }
	Bool				IsQuerySuccess() const								{ return m_outputSuccess; }
	const Vector3&		GetComputedPosition() const							{ return m_output; }

	Bool				ShouldBailOutOnSuccess() const						{ return (m_flags & FLAG_BAIL_OUT_ON_SUCCESS) != 0; }
	Bool				IsRequiringSynchronousCompletionCallback() const	{ return (m_flags & FLAG_SYNCHRONOUS_COMPLETION_CALLBACK) != 0; }
	Bool				IsQueringReachableFromSource() const				{ return (m_flags & FLAG_REQUIRE_REACHABLE_FROM_SOURCE) != 0; }
	Bool				IsUsing3DDistance() const							{ return (m_flags & FLAG_DIST3D) != 0; }
	Bool				IsComputingClosestPositionWithClearLinetest() const	{ return (m_flags & FLAG_CLOSEST_POSITION_LINETEST) != 0; }
	Bool				IsLimitedToBaseArea() const							{ return (m_flags & FLAG_ONLY_BASEAREA) != 0; }

	// redefinable virtual interface
	virtual Bool		AcceptPosition( const Vector3& nodePos );
	virtual void		CompletionCallback();
};


};

