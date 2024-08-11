#pragma once

class CMusicSystem
{
public:
	enum EMusicResponseEventType
	{
		InvalidResponseType = 0,
		Bar,
		Grid,
		User,
		Beat
	};

	struct SMusicSyncDebugInfo
	{
		SMusicSyncDebugInfo()
			:hasSynched(false)
		{

		}
		String eventName;
		Float timeOffset;
		Float nextEventTime;
		Uint32 type;
		Uint64 uniqueId;
		Bool hasSynched;
		Float lastPollTime;
		CName debugName;
	};

public:



	CMusicSystem( ) 
		: m_enabled( true )
		, m_currentBeatDuration(1.f)
		, m_lastBeatTime(0.f) 
		, m_currentBeatsPerBar(0)
		, m_beatsToNextBar(0)
		, m_currentBeatsPerGrid(0)
		, m_beatsToNextGrid(0)
#ifndef RED_FINAL_BUILD
		, m_enableDebug(false)
#endif // !RED_FINAL_BUILD

	{ }
	~CMusicSystem( ) { }

	void Reset( );

	void Tick( Float delta );
	void NotifyBeat(Float duration);
	void NotifyBar(Uint32 beatsRemaining);
	void NotifyGrid(Uint32 beatsRemaining);

	Float GetTimeToNextValidEventBeforeTime(Uint32 typeFlags, Float timeLimit, Float timeOffset = 0.f);
	void Enable( Bool how ) { m_enabled = how; if( !m_enabled ) Reset( ); }
	Bool IsEnabled( ) const { return m_enabled; }
	void GenerateEditorFragments(CRenderFrame* frame);

	Float GetNextBarTime(Float timeNow, Float timeOffset = 0.f);
	Uint32 GetNumWholeBeatsFromTime(Float time, Float &remainder);
	Float GetNextGridTime(Float timeNow, Float timeOffset = 0.f);
	Float GetNextBeatTime(Float timeNow, Float timeOffset = 0.f);

	void EnableDebug(Bool enable);

#ifndef RED_FINAL_BUILD
	void RegisterPendingSync(SMusicSyncDebugInfo &info);
	void RegisterSync(Uint64 uniqeId);
#endif // !RED_FINAL_BUILD
private:

	Bool					m_enabled;
	Red::Threads::CMutex	m_eventsLock;

	Float					m_currentBeatDuration;
	Float					m_lastBeatTime;
	Float					m_lastBarTime;
	Float					m_lastGridTime;
	
	Uint32					m_currentBeatsPerBar;
	Int32					m_beatsToNextBar;
	Uint32					m_currentBeatsPerGrid;
	Int32					m_beatsToNextGrid;

#ifndef RED_FINAL_BUILD
	TDynArray<SMusicSyncDebugInfo > m_syncInfos;
	Bool					m_enableDebug;
#endif // !RED_FINAL_BUILD


};