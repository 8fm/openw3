/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////

class CWorldLayerStreamingFence
{
public:
	CWorldLayerStreamingFence( const String& name );

	// Reference counting stuff
	void AddRef();
	void Release();

	// Get internal name
	RED_INLINE const String& GetName() const { return m_name; }

	// Get number of layers in the fence
	RED_INLINE const Uint32 GetLayerCount() const { return m_layersToLoad.Size(); }

	// Have we started loading ?
	RED_INLINE const Bool IsInitialized() const { return m_isInitialized; }

	// Are we done loading on given layers ?
	const Bool CheckIfCompleted();

	// Get the time since this fence was created
	const Double GetTimeFromStart() const;

	// Debug info
	struct DebugInfo
	{
		String				m_name;
		Bool				m_isStarted;
		Bool				m_isCompleted;
		Float				m_timeSinceIssue;
		Float				m_timeSinceStart;
		Float				m_timeCompleted;
		TDynArray< String > m_layersNotLoaded;
	};

	// Gather global debug information from all of the streaming fences
	static void GatherDebugInformation( TDynArray< DebugInfo >& outDebugInfo );

private:
	~CWorldLayerStreamingFence();

	// thread safe reference count
	Red::Threads::AtomicOps::TAtomic32	m_refCount;

	// loading state
	Bool							m_isInitialized:1;
	Bool							m_isCompleted:1;
	String							m_name;
	TDynArray< CLayerInfo* >		m_layersToLoad;

	// timing
	EngineTime						m_createTime;
	EngineTime						m_issueTime;
	EngineTime						m_finishTime;

	// global list of all layer streaming fences (for debugging)
	static Red::Threads::CMutex		st_lock;
	static TDynArray< CWorldLayerStreamingFence* > st_list;

	// internal state management (called from CWorld)
	void MarkIssued( const TDynArray< CLayerInfo* >& layersToLoad );

	friend CWorld;
};

///////////////////////////////////////////////////////////////////////////////
