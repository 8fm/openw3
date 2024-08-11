#pragma once

#include "pathlibStreamingItem.h"
#include "pathlib.h"
#include "../core/engineTime.h"
#include "../core/heap.h"

class CPathLibWorld;

namespace PathLib
{
class CNavmeshAreaDescription;
class CStreamingJob;

class CStreamingManager : public Red::System::NonCopyable
{
protected:
	enum
	{
		//STREAMING_ROW = 3,
		//STREAMING_RADIUS = STREAMING_ROW / 2,
		//STREAMING_SLOTS = STREAMING_ROW*STREAMING_ROW,

		MAX_STREAMING_ROW = 7,
		MAX_STREAMING_RADIUS = 3,
		MAX_STREAMING_SLOTS = MAX_STREAMING_ROW*MAX_STREAMING_ROW,
	};

	typedef Uint32 StreamingCelIndex;
	typedef TDynArray< IStreamingItem::CStreamingRequest >	StreamingSlot;

	///////////////////////////////////////////////////////////////////////////
	struct StreamOutEvent
	{
		EngineTime				m_time;
		AreaId					m_areaId;
		Bool operator<( const StreamOutEvent& e ) const
		{
			return
				m_time < e.m_time ? true :
				m_time == e.m_time ? m_areaId < e.m_areaId : false;
		}
	};
	typedef THeap< StreamOutEvent >	StreamOutEvents;
	///////////////////////////////////////////////////////////////////////////
	typedef Red::Threads::CAtomic< Bool >			AtomicBool;
	typedef Red::Threads::CAtomic< Int32 >			AtomicInt;

	CPathLibWorld&									m_pathlib;

	CStreamingJob*									m_currentJob;
	Bool											m_currentJobIsCompleted;

	Uint16											m_versionStamp;

	IStreamingItem::List::Head						m_unloadedItems;
	IStreamingItem::List::Head						m_pendingToLoad;
	IStreamingItem::List::Head						m_loadedItems;
	IStreamingItem::List::Head						m_pendingToUnload;

	Int32											m_currentPlayerX;
	Int32											m_currentPlayerY;

	AtomicInt										m_streamingLock;

	//StreamingSlot									m_streamingSlots[ STREAMING_SLOTS ];
	StreamOutEvents									m_streamOutEvents;

	Box												m_streamedWorldBox;

	Int32												m_streamingRow;
	Int32												m_streamingRadius;
	TStaticArray< StreamingSlot, MAX_STREAMING_SLOTS >	m_streamingSlots;

	Vector											m_referencePosition;
	Bool											m_referencePositionValid;

	//Uint32											m_celsInRow;
	//StreamingCelIndex								GetCelIndex( Uint32 x, Uint32 y ) const			{ return y * m_celsInRow + x; }
	//StreamingCelIndex								GetCelsCount() const							{ return GetCelIndex( m_celsInRow, m_celsInRow ); }
	Uint32											TileCoordsToStreamingSlotIndex( Int32 x, Int32 y );
	void											ComputeStreamingSlot( Int32 tileX, Int32 tileY, Bool forceSync = false )	{ ComputeStreamingSlot( tileX, tileY, m_streamingSlots[ TileCoordsToStreamingSlotIndex( tileX, tileY ) ], forceSync ); }
	void											ComputeStreamingSlot( Int32 tileX, Int32 tileY, StreamingSlot& streamingSlot, Bool forceSync = false );

	void											UpdateStreamedWorldBox();
	void											ForceUnloadAllPendingItems();

public:
	CStreamingManager( CPathLibWorld& pathlib );
	~CStreamingManager();

	void											Initialize();
	void											Shutdown();

	void											Tick();
	void											UpdatePosition( const Vector& v );
	void											UnloadItems();

	void											OnAreaDynamicallyAdded( CNavmeshAreaDescription* area );
	void											OnAreaDynamicallyRemoved( CNavmeshAreaDescription* area );
	void											OnAreaDynamicallyUpdated( CNavmeshAreaDescription* area );

	void											MarkJobCompleted();
	void											AttachStreamedItems( IStreamingItem::List::Head&& list );
	Bool											IsJobRunning() const					{ return m_currentJob != nullptr && !m_currentJobIsCompleted; }
	Bool											HasJob() const							{ return m_currentJob != nullptr; } // m_currentJobIsCompleted can be changed async. Maybe there'll be a new job next tick.
	CPathLibWorld&									GetPathLib() const						{ return m_pathlib; }
	const Box&										GetStreamedWorldBox()					{ return m_streamedWorldBox; }
	Uint16											GetVersionStamp() const					{ return m_versionStamp; }

	void											AddPendingToLoadItem( IStreamingItem& item );
	void											AddPendingToUnloadItem( IStreamingItem& item );

	IStreamingItem::List::Head&						GetUnloadedItems()						{ return m_unloadedItems; }
	IStreamingItem::List::Head&						GetLoadedItems()						{ return m_loadedItems; }

	void											AddStreamingLock()						{ m_streamingLock.Increment(); }
	void											ReleaseStreamingLock()					{ m_streamingLock.Decrement(); }

	void											SetReferencePosition( const Vector& position );

	//static StringAnsi								ComputeBundleNameAnsi( Uint32 x, Uint32 y )		{ return StringAnsi::Printf( "nav_%d_%d.bundle", x, y ); }
	//static String									ComputeBundleName( Uint32 x, Uint32 y )			{ return String::Printf( TXT("nav_%d_%d.bundle"), x, y ); }
};


};			// namespace PathLib

