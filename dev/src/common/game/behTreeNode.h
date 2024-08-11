/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeEvent.h"
#include "behTreeVars.h"
#include "behTreeRtti.h"
#include "behTreeMachine.h"
#include "behTreeInstance.h"

enum EBTNodeStatus
{
	BTNS_Active,
	BTNS_Failed,
	BTNS_Completed,
};

BEGIN_ENUM_RTTI( EBTNodeStatus )
	ENUM_OPTION( BTNS_Active );
	ENUM_OPTION( BTNS_Failed );
	ENUM_OPTION( BTNS_Completed );
END_ENUM_RTTI()

#define BEHTREE_HIDE_PRIORITY //CProperty::ChangePropertyFlag( m_registeredClass, CNAME( priority ), PF_Editable, 0 );

// #define DEBUG_BEHTREENODE_DESTRUCTION

#define BEHTREE_STANDARD_SPAWNNODE_FUNCTION( _defClass )				IBehTreeNodeInstance* _defClass::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent  ) const { return new Instance( *this, owner, context, parent ); }

////////////////////////////////////////////////////////////////////////
struct SBTNodeResult
{
	DECLARE_RTTI_STRUCT( SBTNodeResult )

	typedef Int32 TValue;

	SBTNodeResult( EBTNodeStatus status = BTNS_Active )
		: m_status( status )
	{}

	EBTNodeStatus	m_status;
};

BEGIN_CLASS_RTTI( SBTNodeResult );
	PROPERTY( m_status );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
typedef Uint16 TBTNodeId;

////////////////////////////////////////////////////////////////////////
#ifdef EDITOR_AI_DEBUG
	#define BT_DEBUG_UPDATE( instance ) DebugUpdate( instance );
#else
	#define BT_DEBUG_UPDATE( instance )
#endif

class CBehTreeInstance;
class IBehTreeNodeInstance;
class IBehTreeTaskDefinition;

class CBehTreeNodeReactionSFlowSynchroDecoratorInstance;

////////////////////////////////////////////////////////////////////////
// DEFINITION
// Data side object. Created via editor and serialized.
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeDefinition : public CObject
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeDefinition, CObject, IBehTreeNodeInstance, Node );
public:
	typedef Uint16 Id;
	typedef Int8 Priority;

	enum eEditorNodeType
	{
		NODETYPE_DEFAULT,												// atomic nodes
		NODETYPE_DECORATOR,												// decorators
		NODETYPE_COMPOSITE,												// composites
		NODETYPE_SCRIPTED,												// scripted nodes
		NODETYPE_METANODE,												// all meta non-real time nodes
		NODETYPE_UNIQUE,												// custom, seldomly used structural goals
		NODETYPE_COMMENT,
	};

protected:
	static Red::Threads::CAtomic< Uint32 > s_globalId;

	CBehTreeValInt			m_priority;
	Id						m_uniqueId;
#ifdef EDITOR_AI_DEBUG
	CName					m_debugName;
#endif
#ifndef NO_EDITOR_GRAPH_SUPPORT
	Int32					m_graphPosX;			//!< Editor position X
	Int32					m_graphPosY;			//!< Editor position Y	
	String					m_comment;				//!< Comment
#endif

public:

	IBehTreeNodeDefinition()
		: m_priority( 50 )
		, m_uniqueId( Id( s_globalId.Increment() ) )
#ifndef NO_EDITOR_GRAPH_SUPPORT
		, m_graphPosX( 0 )
		, m_graphPosY( 0 )
#endif
	{
	}

	virtual ~IBehTreeNodeDefinition() {}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get graph position X
	Int32  GetGraphPosX() const { return m_graphPosX; }

	//! Get graph position Y
	Int32  GetGraphPosY() const { return m_graphPosY; }

	//! Set graph position
	void  SetGraphPosition( Uint32 x, Uint32 y )
	{
		m_graphPosX = x;
		m_graphPosY = y;
	}

	//! Get comment
	const String& GetComment() const { return m_comment; }

#endif

	Id GetUniqueId() const { return m_uniqueId; }

	//! Get parent node
	IBehTreeNodeDefinition* GetParentNode() const
	{
		// Parent of root can be state machine or resource, so NULL will be returned
		return Cast< IBehTreeNodeDefinition >( GetParent() );
	}

	////////////////////////////////////////////////////////////////////
	// Editor support

	//! Get node caption
	virtual String GetNodeCaption() const;

	//! Is terminal node
	virtual Bool IsTerminal() const;

	//! Can add child
	virtual Bool CanAddChild() const;

	//! Remove given child node
	virtual void RemoveChild( IBehTreeNodeDefinition* node );

	//! Get number of children
	virtual Int32 GetNumChildren() const;

	//! Get child
	virtual IBehTreeNodeDefinition* GetChild( Int32 index ) const;

	//! Add child node
	virtual void AddChild( IBehTreeNodeDefinition* node );

	//! Check if tree is useable in-game
	virtual Bool IsValid() const;

	//! Editor only
	virtual eEditorNodeType GetEditorNodeType() const;

	//! For scripted behaviors
	virtual IBehTreeTaskDefinition* GetTask() const;
	virtual Bool SetTask( IBehTreeTaskDefinition* task );
	virtual Bool IsSupportingTasks() const;
	virtual Bool IsSupportingTaskClass( const CClass* classId ) const;
	
	//! Collect nodes
	virtual void CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const;

	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Correct children positions (left to right), returns true if modification occurs
	virtual Bool CorrectChildrenPositions();

	//! Correct children order to be coherent with positions, returns true if modification occurs
	virtual Bool CorrectChildrenOrder();

	//! Recursively offset nodes' position
	virtual void OffsetNodesPosition( Int32 offsetX, Int32 offsetY );
#endif

	//! Dump result to log
	void LogResult( CBehTreeInstance& instance ) const;

	//! Marks node that are internal to engine and are not used directly in definitions
	static Bool IsInternalNode()										{ return false; }

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const = 0;
	virtual void SpawnInstanceList( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent, TStaticArray< IBehTreeNodeInstance*, 128 >& instanceList ) const;

	virtual Bool OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const;
	virtual Uint32 OnSpawnList( IBehTreeNodeInstance* const* nodeList, Uint32 nodeCount, CBehTreeSpawnContext& context ) const;
	Bool IsMyInstance( IBehTreeNodeInstance* node ) const;

#ifndef NO_EDITOR
public:
	// by default all nodes are good for all games
	// this method is to prevent the editor from spawning specific nodes when editing specific game
	// for instance: if you dont want your CBehTreeR6SpecificNode to be spawned accidentally by some R4 guys,
	// just override this methid in your CBehTreeR6SpecificNode and return false when game is R4. 
	virtual Bool IsUsableInGame( const CCommonGame* game ) const { RED_UNUSED( game ); return true; }  
	// for convenience, i've prepared some macros: DECLARE_AS_R4_ONLY and DECLARE_AS_R6_ONLY
#endif

};

#if !defined( NO_EDITOR ) && defined( RED_PLATFORM_WINPC )
#	define DECLARE_AS_R4_ONLY public: virtual Bool IsUsableInGame( const CCommonGame* game ) const override { return 0 == Red::System::StringCompare( game->GetGamePrefix(), TXT("R4") ); } 
#	define DECLARE_AS_R6_ONLY public: virtual Bool IsUsableInGame( const CCommonGame* game ) const override { return 0 == Red::System::StringCompare( game->GetGamePrefix(), TXT("R6") ); } 
#else
#	define DECLARE_AS_R4_ONLY
#	define DECLARE_AS_R6_ONLY
#endif

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeDefinition );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_priority, TXT( "Execution priority [0..100]" ) );
#ifdef EDITOR_AI_DEBUG
	PROPERTY_EDIT_NOT_COOKED( m_debugName, TXT( "Node name used for debugging purposes" ) );
#endif
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_graphPosX );
	PROPERTY_NOT_COOKED( m_graphPosY );
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT( "Comment" ) );

#endif

END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// INSTANCE
// Run-time object created by definition. 
////////////////////////////////////////////////////////////////////////

class IBehTreeNodeInstance
{
public:
	typedef IBehTreeNodeDefinition::Priority Priority;
protected:
	CBehTreeInstance*					m_owner;						// 8
	IBehTreeNodeInstance*				m_parent;						// 16
	Bool								m_isActive;						// 17
#ifdef DEBUG_BEHTREENODE_DESTRUCTION
	Bool								m_isDestroyed;
#endif
	Priority							m_priority;						// 18
	IBehTreeNodeDefinition::Id			m_definitionId;					// 24 - release build
#ifdef EDITOR_AI_DEBUG
	CName								m_debugName;
#endif

public:
	enum eTaskOutcome
	{
		BTTO_FAILED		= 0,
		BTTO_SUCCESS	= 1
	};

	typedef IBehTreeNodeDefinition Definition;

	IBehTreeNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: m_owner( owner )
		, m_parent( parent )
		, m_isActive( false )
#ifdef DEBUG_BEHTREENODE_DESTRUCTION
		, m_isDestroyed( false )
#endif
		, m_priority( Priority( def.m_priority.GetVal( context ) ) )
		, m_definitionId( def.m_uniqueId )		
#ifdef EDITOR_AI_DEBUG
		, m_debugName( !def.m_debugName.Empty() ? def.m_debugName : def.GetNodeName() )
#endif
		{}

	virtual ~IBehTreeNodeInstance();

	////////////////////////////////////////////////////////////////////
	//! Object life cycle
	virtual void OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context );
	virtual void OnDestruction();
	void RegisterForDeletion();

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	virtual void Update();												// Called simultanously
	virtual Bool Activate();											// goal activation - if it returns false goal cannot be activated
	virtual void Deactivate();											// goal deactivation
	virtual void Complete( eTaskOutcome outcome );						// goal completion that should also deactivate it
	virtual void OnSubgoalCompleted( eTaskOutcome outcome );			// notification of current subgoal being completed
	
	////////////////////////////////////////////////////////////////////
	//! Event handling
	virtual Bool OnEvent( CBehTreeEvent& e );							// Default event processing - all active branch get an event notification
	virtual Bool OnListenedEvent( CBehTreeEvent& e );					// Event listening mechanism - goal can register for event listening
	virtual void MarkDirty();											// Mark node for decision recomputation
	virtual void MarkParentSelectorDirty();								// Marks parent selection node 'dirty'
	void MarkActiveBranchDirty();

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	virtual Bool IsAvailable();
	virtual Int32 Evaluate();

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	virtual Bool Interrupt();

	////////////////////////////////////////////////////////////////////
	//! Visual debug
			void PropagateDebugFragmentsGeneration( CRenderFrame* frame );
	virtual void OnGenerateDebugFragments( CRenderFrame* frame );
	
	
	//template < class DefClass >
	//RED_INLINE const DefClass& GetDefinition() const					{ return static_cast< const DefClass& >( m_definition ); }
	RED_INLINE IBehTreeNodeInstance* GetParent() const					{ return m_parent; }
	RED_INLINE Bool IsActive() const									{ return m_isActive; }
	CBehTreeInstance* GetOwner() const									{ return m_owner; }
	RED_INLINE Priority GetDefaultPriority() const						{ return m_priority; }

	////////////////////////////////////////////////////////////////////
	//! Handling children
	virtual Int32 GetNumChildren() const;
	virtual IBehTreeNodeInstance* GetChild( Int32 index ) const;
	virtual IBehTreeNodeInstance* GetActiveChild() const;
	virtual IBehTreeNodeInstance* GetActiveChild( Uint32 activeChild ) const;
	virtual Uint32 GetActiveChildCount() const;
	virtual Bool IsMoreImportantNodeActive( IBehTreeNodeInstance* askingChild );
	virtual Int32 GetNumPersistantChildren() const;						// persistant children (non - dynamic)
	virtual IBehTreeNodeInstance* GetPersistantChild( Int32 index ) const;

	////////////////////////////////////////////////////////////////////
	// state saving
	virtual Bool IsSavingState() const;
	virtual void SaveState( IGameSaver* writer );
	virtual Bool LoadState( IGameLoader* reader );

	void SetParent( IBehTreeNodeInstance* parent )						{ m_parent = parent; }
	IBehTreeNodeDefinition::Id GetDefinitionId() const					{ return m_definitionId; }

#ifdef EDITOR_AI_DEBUG
	CName GetDebugName() const											{ return m_debugName; }

	////////////////////////////////////////////////////////////////////
	//! Debug interface
	virtual Bool DebugUpdate(Bool cascade = false);
	void DebugReport();
	void ForceSetDebugColor( Uint8 R, Uint8 G, Uint8 B );
	void ClearColorRecursive();
#endif

protected:
	void DebugNotifyActivationFail();
	void DebugNotifyAvailableFail();

public:
	// casts
	virtual CBehTreeNodeReactionSFlowSynchroDecoratorInstance* AsCBehTreeNodeReactionSFlowSynchroDecorator(){ return nullptr; }

	RED_INLINE void* operator new( size_t size )			
	{
		return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_Gameplay, size ); 	
	}	
	
	RED_INLINE void operator delete( void* ptr )			
	{ 
		RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_Gameplay, ptr ); 	
	}
};

RED_FORCE_INLINE void IBehTreeNodeInstance::DebugNotifyActivationFail()
{
#ifdef EDITOR_AI_DEBUG
	if( m_owner->GetIsDebugged() )
	{
		ClearColorRecursive();
		ForceSetDebugColor( 250, 50, 0 );
	}
#endif // EDITOR_AI_DEBUG
}

RED_FORCE_INLINE void IBehTreeNodeInstance::DebugNotifyAvailableFail()
{
#ifdef EDITOR_AI_DEBUG
	if( m_owner->GetIsDebugged() )
	{
		ClearColorRecursive();
		ForceSetDebugColor( 250, 150, 0 );
	}
#endif // EDITOR_AI_DEBUG
}
