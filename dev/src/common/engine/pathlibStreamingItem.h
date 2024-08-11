/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/intrusiveList.h"
#include "../core/engineTime.h"

namespace PathLib
{

class CStreamingManager;

class IStreamingItem : public IntrusiveList::Node
{
	friend class CStreamingManager;

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_PathLib );

public:
	typedef TIntrusiveList< IStreamingItem > List;

	enum EStreamingState
	{
		STREAMING_STATE_UNLOADED,
		STREAMING_STATE_PENDING_TO_LOAD,
		STREAMING_STATE_LOADING,
		STREAMING_STATE_LOADED,
		STREAMING_STATE_PENDING_TO_UNLOAD,
		STREAMING_STATE_UNLOADING,
	};

	class CStreamingRequest
	{
	private:
		IStreamingItem*						m_item;
		CStreamingManager*					m_manager;
	public:
		CStreamingRequest( IStreamingItem* item, CStreamingManager& manager );

		CStreamingRequest();
		CStreamingRequest( const CStreamingRequest& req );
		CStreamingRequest( CStreamingRequest&& req );

		~CStreamingRequest();

		void operator=( const CStreamingRequest& req );
		void operator=( CStreamingRequest&& req );

		Bool operator==( const CStreamingRequest& req )	const										{ return m_item == req.m_item; }
		Bool operator==( IStreamingItem* item )	const												{ return m_item == item; }

		void Release();
	};

private:
	typedef Red::Threads::CAtomic< Bool > AtomicBool;

	Int32				m_streamingRequestsCount;
	EStreamingState		m_streamingState;
	EngineTime			m_unstreamRequestTime;
	AtomicBool			m_isBeingLoadedNow;

	// CStreamingRequest interface that also reach out for streaming manager
	void				RequestLoad( CStreamingManager* manager );
	void				RequestUnload( CStreamingManager* manager );
	void				AddRef( CStreamingManager* manager );
	void				DelRef( CStreamingManager* manager );
	
	void				CheckStreamingState( CStreamingManager* manager );

	void				SetStreamingState( EStreamingState state )									{ m_streamingState = state; }
protected:
	void				NeverUnstream( CStreamingManager* manager );
public:
						IStreamingItem();
						~IStreamingItem();

	// CStreamingManager interface
	virtual void		PreLoad();																	// synchronous pre-loading callback
	virtual void		Load();																		// asynchronous loading implementation
	virtual void		PostLoad();																	// asynchronous post-loading possibly heavy processing
	virtual void		PostLoadInterconnection();													// asynchronous post-post-loading
	virtual void		Attach( CStreamingManager* manager );										// synchronous post-loading callback
	
	virtual void		PreUnload();																// synchronous pre-unloading callback
	virtual void		Unload();																	// asynchronous unloading code
	virtual void		Detach( CStreamingManager* manager );										// synchronous post-unloading callback
	
	EngineTime			GetUnstreamRequestTime() const												{ return m_unstreamRequestTime; }
	void				ForceImmediateUnstream()													{ m_unstreamRequestTime = EngineTime::ZERO; }
	EStreamingState		GetStreamingState()															{ return m_streamingState; }
	Bool				IsBeingLoaded() const														{ return m_isBeingLoadedNow.GetValue(); }
	Bool				IsLoaded() const															{ return m_streamingState == STREAMING_STATE_LOADED || m_streamingState == STREAMING_STATE_PENDING_TO_UNLOAD; }
	Bool				ShouldBeLoaded() const														{ return m_streamingRequestsCount > 0; }
	
};




};				// namespace PathLib