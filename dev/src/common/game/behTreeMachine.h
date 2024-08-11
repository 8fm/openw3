/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiParametersSpawnList.h"
#include "gameplayEvent.h"
#include "behTree.h"

class IBehTreeMachineListener;
class IBehTreeNodeInstance;
class CBehTreeSpawnContext;

///////////////////////////////////////////////////////////////////////////////

class CBehTreeMachine : public CObject
{	
	DECLARE_ENGINE_CLASS( CBehTreeMachine, CObject, 0 );

public:
	enum EBreakpointStatus
	{
		BS_Normal,
		BS_Breaked,
		BS_Skip,
	};

	struct BreakpointInfo
	{
		const IBehTreeNodeDefinition*	m_node;					//!< Breakpoint node
		EBreakpointStatus				m_status;				//!< Breakpoint status

		BreakpointInfo()
			: m_node( NULL )
			, m_status( BS_Normal )
		{
		}

		BreakpointInfo( const IBehTreeNodeDefinition*	 node, EBreakpointStatus breaked = BS_Normal )
			: m_node( node )
			, m_status( breaked )
		{
		}

		Bool operator==( const BreakpointInfo& info ) const
		{
			return m_node == info.m_node;
		}
	};

	typedef TDynArray< BreakpointInfo > TBreakpoints;

protected:
	CBehTreeInstance*					m_instance;						//!< Tree instance
	THandle< CBehTree >					m_aiRes;						// Root AI resource
	TDynArray< THandle< IAIParameters > > m_aiParameters;				// persistant template-dependent ai parameters
	SAIParametersSpawnList				m_spawnParameters;
	Bool								m_stopped;						//!< Machine stopped
	Bool								m_initializedOnSpawn;
#ifdef EDITOR_AI_DEBUG
	TBreakpoints								m_breakpoints;			//!< Breakpoints
	TDynArray< IBehTreeMachineListener* >		m_listeners;			//!< Machine listeners	
#endif

	void SetupContext( CBehTreeSpawnContext& context, const SAIParametersSpawnList& spawnParameters );

public:
	CBehTreeMachine()		
		: m_instance( NULL )
		, m_aiRes( NULL )
		, m_aiParameters()
		, m_stopped( true )
		, m_initializedOnSpawn( false )
	{
	}

	//! Called when object is ready to be deleted by GC
	virtual void OnFinalize();

public:
	//! Get debug listener
	RED_INLINE IBehTreeMachineListener* GetListener( Uint32 index ) const
	{ 
#ifdef EDITOR_AI_DEBUG
		if( m_listeners.Size() > index )
		{
			return m_listeners[index];
		}
#endif
		return NULL;
	}

	Uint32 GetListenerCount() const 
	{ 
#ifdef EDITOR_AI_DEBUG
		return m_listeners.Size();
#else
		return 0;
#endif
	}

	Bool AnyListenerExists() const
	{
#ifdef EDITOR_AI_DEBUG
		return !( m_listeners.Empty() );
#else
		return false;
#endif
	}

	//! Is machine stopped
	RED_INLINE Bool IsStopped() const { return m_stopped; }

	//! Returns true if Initialize was called on the behtree
	Bool IsInitialised()const { return m_instance != nullptr; }

	//! Initialize - possibly asynchronous
	Bool Initialize( const SAIParametersSpawnList& parametersList );

	//! Synchronous spawning code
	void OnSpawn();

	//! Free resources
	void Uninitialize();

	//! Called when actor is attached
	void OnAttached();

	//! Callen when actor returns from pool - possibly asynchronous
	void OnReattachedAsync( const SAIParametersSpawnList& parametersList );

	//! Callen when actor returns from pool
	void OnReattached( const SAIParametersSpawnList& parametersList );

	//! Called when actor is detached
	void OnDetached();

	//! Restart machine
	void Restart();

	//! Stop machine
	void Stop( Bool silent = false );

	//! Tick
	void Tick( Float timeDelta );

	//! Updates state forcefully
	void ForceUpdate();

	//! Process animation event
	void ProcessAnimationEvent( const CExtAnimEvent* event, EAnimationEventType eventType, const SAnimationEventAnimInfo& animInfo );

	//! Process gameplay event
	void OnGameplayEvent( CName name, void* additionalData, IRTTIType* additionalDataType );

	//! Base setup required for system to work
	void SetAIDefinition( CAIBaseTree* baseTree );

	//! Set owning system ai parameters
	void SetSpawnSystemParameters( IAIParameters* parameters );

	//! Interface to 'feed ai' with external parameters
	void InjectAIParameters( IAIParameters* parameters );

	//! Add debug listener
	void AddListener( IBehTreeMachineListener* listener );

	//! Remove debug listener
	void RemoveListener( IBehTreeMachineListener* listener );

#ifdef EDITOR_AI_DEBUG
	//! Is breakpoint set on node
	Bool IsBreakpointSet( const IBehTreeNodeDefinition* node, EBreakpointStatus& status );

	//! Set breakpoint
	void SetBreakpoint( const IBehTreeNodeDefinition* node );

	//! Clear breakpoint
	void ClearBreakpoint( const IBehTreeNodeDefinition* node );

	//! Skip breakpoint
	void SkipBreakpoint( const IBehTreeNodeDefinition* node );

	//! Process breakpoint - returns true if should break
	Bool ProcessBreakpoint( const IBehTreeNodeDefinition* node );

	//! Describe AI state for debugging
	void DescribeAIState( String& ai );
#endif

	//! Get machine info
	String GetInfo() const;

	CBehTreeInstance* GetBehTreeInstance() const { return m_instance; }

protected:
	//! Free instance
	void FreeInstance();

#ifdef EDITOR_AI_DEBUG
	//! Inform node debugger about node status
	void DebugUpdateNode( CBehTreeInstance& instance, const IBehTreeNodeInstance* node );

	//! Inform about tree state changed
	void DebugTreeStateChanged();

	//! Request debugging end
	void DebugStopListening();

	//! Clear all breakpoints
	void ClearAllBreakpoints();
#endif
};

BEGIN_CLASS_RTTI( CBehTreeMachine );
	PARENT_CLASS( CObject );
	PROPERTY( m_instance );
	PROPERTY( m_aiRes );
	PROPERTY( m_aiParameters );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
