/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class InstanceBuffer;
class IQuestSystemListener;

struct SQuestThreadSuspensionData
{
	DECLARE_RTTI_STRUCT( SQuestThreadSuspensionData )

	typedef TDynArray< Uint8, MC_Gameplay, MemoryPool_GameSave > TStorageType;

	CGUID		m_scopeBlockGUID;
	TStorageType	m_scopeData;

	SQuestThreadSuspensionData() {}
	SQuestThreadSuspensionData( const CGUID& guid ) : m_scopeBlockGUID( guid ) {}

	SQuestThreadSuspensionData( const SQuestThreadSuspensionData & other )
		: m_scopeBlockGUID( other.m_scopeBlockGUID )
		, m_scopeData( other.m_scopeData )
	{}

	SQuestThreadSuspensionData( SQuestThreadSuspensionData && other ) 
		: m_scopeBlockGUID( std::move( other.m_scopeBlockGUID ) )
		, m_scopeData( std::move( other.m_scopeData ) )
	{}

	Bool operator== ( const SQuestThreadSuspensionData& rhs ) const
	{
		return m_scopeBlockGUID == rhs.m_scopeBlockGUID;
	}

	SQuestThreadSuspensionData & operator=( const SQuestThreadSuspensionData & other )
	{
		if( this != &other )
		{
			m_scopeBlockGUID = other.m_scopeBlockGUID;
			m_scopeData = other.m_scopeData;
		}

		return *this;
	}

	SQuestThreadSuspensionData & operator=( SQuestThreadSuspensionData && other )
	{
		if( this != &other )
		{
			m_scopeBlockGUID = std::move( other.m_scopeBlockGUID );
			m_scopeData = std::move( other.m_scopeData );
		}

		return *this;
	}
};

BEGIN_CLASS_RTTI( SQuestThreadSuspensionData )
	PROPERTY( m_scopeBlockGUID )
	PROPERTY( m_scopeData )
END_CLASS_RTTI();

namespace Helper
{
	struct SStableTickData
	{
		Bool									m_stable:1;
		Bool									m_latchBlockCountStable:1;
		Bool									m_latchChildThreadCountStable:1;
		Bool									m_latchChildThreadsStable:1;
		Bool									m_latchNoForceKeepLoadingScreen:1;

		SStableTickData()
			: m_stable( true )
			, m_latchBlockCountStable( true )
			, m_latchChildThreadCountStable( true )
			, m_latchChildThreadsStable( true )
			, m_latchNoForceKeepLoadingScreen( true )
		{}
	};
}

class CQuestsDebugDumper;

// A quest thread is capable of executing a single instance of a quest graph.
class CQuestThread : public CObject
{
	DECLARE_ENGINE_CLASS( CQuestThread, CObject, 0 );

	friend class CQuestsDebugDumper;

private:
	struct SBlockData
	{
		const CQuestGraphBlock* block;
		TDynArray< CName >		activeInputs;

		SBlockData() : block ( NULL ) {}
		SBlockData( const CQuestGraphBlock* _block, const CName& inputName = CName::NONE )
			: block( _block )
		{
			activeInputs.PushBack( inputName );
		}

		Bool operator== ( const SBlockData& rhs ) const
		{
			return block == rhs.block;
		}
	};

private:
	const CQuestGraphBlock*					m_parentBlock;
	CQuestGraphInstance*					m_graph;
	TDynArray< SBlockData >					m_activeBlocks;
	TDynArray< SBlockDesc >					m_blocksToActivate; 
	Bool									m_canBlockSaves;
	Bool									m_paused;
	
	TDynArray< IQuestSystemListener* >		m_listeners;

	// threads hierarchy related data
	TDynArray< CQuestThread* >				m_threads;
	TDynArray< CQuestThread* >				m_threadsToAdd;
	TDynArray< CQuestThread* >				m_threadsToRemove;
	TDynArray< CQuestThread* >				m_threadsToStabilize; // A subset of m_threads that have yet to reach stabilization once, GC handled by m_threads.
	Helper::SStableTickData					m_stableTickData;

	// scope suspension data
	TDynArray< SQuestThreadSuspensionData >	m_suspendedScopesData; // Hack to check the basic concept

public:
	CQuestThread();
	~CQuestThread();

	void ResetStabilizedThreads();

	// Activates a new thread thread
	void Activate( const CQuestGraphBlock* parentBlock, const CQuestGraph& questGraph );

	// Activates the specified input block of a thread
	void ActivateInput( const CName& inputName = CName::NONE );

	// Activates the specified block in this thread, using the specified input (for debugging purposes)
	void ActivateBlock( const CQuestGraphBlock& block, const CName& inputName );

	// Stops the thread
	void Kill();

	// Creates a child thread running in the context of this thread.
	// If the parent is killed, all its children will be killed as well.
	CQuestThread* AddChild( const CQuestGraphBlock* parentBlock, const CQuestGraph& questGraph, Bool canBlockSaves );

	// Stops execution of a child thread
	void KillChild( CQuestThread* thread );

	// Stops the execution of all children threads
	void KillAllChildren();

	// Updates the quest progress
	void Tick();

	// Deactivates the specified block, providing it's currently active
	void DeactivateBlock( const CQuestGraphBlock* block );

	// Forces the block to progress with given output
	void ForceBlockExit( const CQuestGraphBlock* block, const CName& output );

	// Returns the instance data associated with the operated quest graph
	InstanceBuffer& GetInstanceData();

	// Returns the name of the thread
	String GetName() const;

	// Returns a collection of currently active blocks
	void GetActiveBlocks( TDynArray< const CQuestGraphBlock* >& blocks ) const;

	// Returns the children threads of this thread
	RED_INLINE const TDynArray< CQuestThread* >& GetChildrenThreads() const { return m_threads; }

	// Returns an array of inputs the block has active
	const TDynArray< CName >* GetActivatedInputs( const CQuestGraphBlock* block ) const;

	// Returns the parent block that spawned this thread. Can return NULL in case of the top-level quest thread
	RED_INLINE const CQuestGraphBlock* GetParentBlock() const { return m_parentBlock; }

	// Can this thread block saves
	RED_INLINE Bool CanBlockSaves() const { return m_canBlockSaves; }

	// Did the quest graph not make any relevant progress
	RED_INLINE Bool IsStable() const { return m_stableTickData.m_stable && m_threadsToAdd.Empty() && m_threadsToRemove.Empty() && m_blocksToActivate.Empty(); }

	// Checks if the thread or any of its children contains this block instance
	Bool IsBlockActive( const CQuestGraphBlock* block ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	// Checks if the thread or any of its children contains this block instance and was ever visited
	Bool IsBlockVisited( const CQuestGraphBlock* block ) const;
#endif //NO_EDITOR_GRAPH_SUPPORT

	// Tells whether the thread works on the specified graph
	Bool DoesWorkOnGraph( const CQuestGraph& graph ) const;

	void OnNewWorldLoading( const String& worldPath );
	void ReactivateDeadPhasesHack( const String& worldPath );
	
	// CObject implementation
	virtual void OnSerialize( IFile& file );

	void AttachListener( IQuestSystemListener& listener );
	void DetachListener( IQuestSystemListener& listener );

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	Bool CanSaveToSaveGame() const;
	void SaveGame( IGameSaver* saver );
	void LoadGame( IGameLoader* loader );

	void SetPaused( Bool paused );
	RED_INLINE Bool IsPaused() { return m_paused; }

private:
	void ManageThreads();
	void RemoveThread( CQuestThread* thread );
	void TickBlocks();
	void TickChildrenThreads();
	CQuestThread* FindThread( const CQuestGraphBlock* parentBlock );

private:
	Bool KeepLoadingScreenForBlock( const CQuestGraphBlock* block, const InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// notifications
	// ------------------------------------------------------------------------
	void NotifyThreadPaused( bool paused );
	void NotifyThreadAdded( CQuestThread* thread );
	void NotifyThreadRemoved( CQuestThread* thread );
	void NotifyBlockAdded( const CQuestGraphBlock* block );
	void NotifyBlockRemoved( const CQuestGraphBlock* block );
	void NotifyBlockInputActivated( const CQuestGraphBlock* block );
};

BEGIN_CLASS_RTTI( CQuestThread )
	PARENT_CLASS( CObject )
	PROPERTY( m_suspendedScopesData )
END_CLASS_RTTI()
