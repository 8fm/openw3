/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorVariable.h"
#include "behaviorEvent.h"
#include "behaviorIncludes.h"
#include "../core/sortedset.h"
#include "../core/resource.h"
#include "../core/instanceDataLayout.h"

class CBehaviorGraphAnimationBaseSlotNode;
class CBehaviorGraphAnimationSlotNode;
class CBehaviorGraphContainerNode;
class CBehaviorGraphStateMachineNode;
class CBehaviorGraphPoseSlotNode;

//////////////////////////////////////////////////////////////////////////

#ifndef NO_LOG

#define BEH_DUMP_ERROR( __instance, __error )	DumpError( __instance, __error )

#define BEH_WARN_ONCE( ptrA, ptrB, format, ... )													\
{																									\
	static TSortedSet< Uint64 > warnedAlready;														\
	Uint64 smashedPtr = reinterpret_cast< Uint64 >( ptrA ) ^ reinterpret_cast< Uint64 >( ptrB  );	\
	if ( false == warnedAlready.Exist( smashedPtr ) )												\
	{																								\
		warnedAlready.Insert( smashedPtr );															\
		RED_LOG( Warning, format, ## __VA_ARGS__ );													\
	}																								\
}

#else

#define BEH_DUMP_ERROR( __instance, __error )
#define BEH_WARN_ONCE( ptrA, ptrB, format, ... )

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraph : public CResource					 
{
	DECLARE_ENGINE_RESOURCE_CLASS( CBehaviorGraph, CResource, "w2beh", "Behavior graph" );

	friend class CBehaviorGraphInstance;

protected:
	InstanceDataLayout						m_dataLayout;				//!< Layout of data in this graph

	CBehaviorGraphContainerNode*			m_rootNode;					//!< root node
	CBehaviorGraphStateMachineNode*			m_defaultStateMachine;		//!< default state machine

	TDynArray< CBehaviorGraphStateMachineNode* > m_stateMachines;		//!< all state machines
	TDynArray< CBehaviorGraphNode* >		m_nodesToReleaseList;		//!< notes to release

	CBehaviorVariablesList					m_variables;				//!< list of control variables
	CBehaviorVectorVariablesList			m_vectorVariables;			//!< list of control vector variables
	CBehaviorEventsList						m_events;					//!< list of events handled by the graph

	CBehaviorVariablesList					m_internalVariables;		//!< list of internal control variables
	CBehaviorVectorVariablesList			m_internalVectorVariables;	//!< list of internal control vector variables

	TDynArray< CName >						m_customTrackNames;			//!< custom track names
	Bool									m_generateEditorFragments;
	Bool									m_sourceDataRemoved;		//!< Source data for this graph was removed

	TDynArray< CBehaviorGraphPoseSlotNode* >			m_poseSlots;
	TDynArray< CBehaviorGraphAnimationBaseSlotNode* >	m_animSlots;

protected:
	TInstanceVar< TDynArray< Float > >		i_rtFloatVariables;			//!< list of runtime control float variables
	TInstanceVar< TDynArray< Vector > >		i_rtVectorVariables;		//!< list of runtime control vector variables	
	TInstanceVar< TDynArray< Float > >		i_rtInternalFloatVariables;	//!< list of runtime internal control float variables
	TInstanceVar< TDynArray< Vector > >		i_rtInternalVectorVariables;//!< list of runtime internal control vector variables	

private:
	//! Compile data layout
	void CompileDataLayout();

public:
	CBehaviorGraph();
	~CBehaviorGraph();
	
	//! Serialize graph
	virtual void OnSerialize( IFile& file );

	//! Cooking
#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker );
#endif

	//! Called after loading
	virtual void OnPostLoad();

	//! Set as clone of
	void SetAsCloneOf( const CBehaviorGraph* otherGraph );

	//! Create instance of this behavior graph
	CBehaviorGraphInstance*	CreateInstance( CAnimatedComponent* component, const CName& name ) const;

	//! Cache graph connections
	virtual void CacheConnections();

	//! Cache slots
	void CacheSlots();

	//! Cache state machines
	void CacheStateMachines();

	//! Cache nodes to relese
	void CacheNodesToRelease();

	//! Cache connections, slots, state machines with one convenient call
	void CacheData();

	//! Clear source data
	virtual void CleanupSourceData();

	//! Is graph generating editor fragments
	Bool GenerateEditorFragments() const { return m_generateEditorFragments; }

	//! Reload graph
	void Reload();

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	using CResource::Reload;
#endif

	//! Set default state machine
	void SetDefaultStateMachine( CBehaviorGraphStateMachineNode* stateMachine );

	//! Get default state machine
	const CBehaviorGraphStateMachineNode* GetDefaultStateMachine() const { return m_defaultStateMachine; }

	//! Generate blocks id
	Uint32 GenerateBlocksId();

	//! Get all nodes
	Uint32 GetAllNodes( TDynArray< CBehaviorGraphNode* >& nodes ) const;

	//! Is source data removed
	Bool IsSourceDataRemoved() const;

	//! Get slots
	const TDynArray< CBehaviorGraphPoseSlotNode* >&	GetPoseSlots() const { return m_poseSlots; }
	const TDynArray< CBehaviorGraphAnimationBaseSlotNode* >& GetAnimSlots() const { return m_animSlots; }
	const TDynArray< CBehaviorGraphStateMachineNode* >& GetStateMachines() const { return m_stateMachines; }
	const TDynArray< CBehaviorGraphNode* >& GetNodesToRelease() const { return m_nodesToReleaseList; }

	//! Get size
	Uint32 GetSize() const;

public:
	Bool HasFloatValue( CName name ) const { return m_variables.HasVariable( name ); }
	Bool HasInternalFloatValue( CName name ) const { return m_internalVariables.HasVariable( name ); }
	Bool HasVectorValue( CName name ) const { return m_internalVariables.HasVariable( name ); }
	Bool HasInternalVectorValue( CName name ) const { return m_internalVectorVariables.HasVariable( name ); }

	const Float* GetFloatValuePtr( CName name ) const;
	const Float* GetInternalFloatValuePtr( CName name ) const;
	const Vector* GetVectorValuePtr( CName name ) const;
	const Vector* GetInternalVectorValuePtr( CName name ) const;

	//! Get float min value
	Float GetFloatVariableMin( CName name ) const;

	//! Get float max value
	Float GetFloatVariableMax( CName name ) const;

	//! Get float default value
	Float GetFloatVariableDefault( CName name ) const;

	//! Get vector min value
	Vector GetVectorVariableMin( CName name ) const;

	//! Get vector max value
	Vector GetVectorVariableMax( CName name ) const;

	//! Get vector default value
	Vector GetVectorVariableDefault( CName name ) const;

	Bool HasRuntimeFloatVariable( const CBehaviorGraphInstance& instance, CName name ) const;
	Bool HasRuntimeInternalFloatVariable( const CBehaviorGraphInstance& instance, CName name ) const;
	Bool HasRuntimeVectorVariable( const CBehaviorGraphInstance& instance, CName name ) const;
	const Float* GetRuntimeFloatVariablePtr( const CBehaviorGraphInstance& instance, CName name ) const;
	const Float* GetRuntimeInternalFloatVariablePtr( const CBehaviorGraphInstance& instance, CName name ) const;
	const Vector* GetRuntimeVectorVariablePtr( const CBehaviorGraphInstance& instance, CName name ) const;

 public:
	//! Get runtime ( instance ) float variable
 	Float GetRuntimeFloatVariable( const CBehaviorGraphInstance& instance, CName name, Float defVal = 0.0f ) const;

	//! Get runtime ( instance ) vector variable
 	Vector GetRuntimeVectorVariable( const CBehaviorGraphInstance& instance, CName name ) const;
 
	//! Set runtime ( instance ) float variable
 	Bool SetRuntimeFloatVariable( CBehaviorGraphInstance& instance, CName name, Float value ) const;

	//! Set runtime ( instance ) vector variable
 	Bool SetRuntimeVectorVariable( CBehaviorGraphInstance& instance, CName name, const Vector& value) const;

	//! Reset runtime ( instance ) float value
	Float ResetRuntimeFloatVariable( CBehaviorGraphInstance& instance, CName name ) const;

	//! Reset runtime ( instance ) vector value
	Vector ResetRuntimeVectorVariable( CBehaviorGraphInstance& instance, CName name ) const;

	//! Get runtime ( instance ) float variables num
	Uint32 GetRuntimeFloatVariablesNum( CBehaviorGraphInstance& instance ) const;

	//! Get runtime ( instance ) vector variables num
	Uint32 GetRuntimeVectorVariablesNum( CBehaviorGraphInstance& instance ) const;

	//! Store runtime float variables
	void StoreRuntimeFloatVariables( CBehaviorGraphInstance& instance, TDynArray< Float >& variables ) const;

	//! Store runtime float variables
	void StoreRuntimeVectorVariables( CBehaviorGraphInstance& instance, TDynArray< Vector >& variables ) const;

	//! Restore runtime float variables
	void RestoreRuntimeFloatVariables( CBehaviorGraphInstance& instance, const TDynArray< Float >& variables ) const;

	//! Restore runtime float variables
	void RestoreRuntimeVectorVariables( CBehaviorGraphInstance& instance, const TDynArray< Vector >& variables ) const;

	//! Reset all runtime variables
	void ResetRuntimeVariables( CBehaviorGraphInstance& instance ) const;

	//! Fill runtime variables
	void FillRuntimeVariables( CBehaviorGraphInstance& instance ) const;

public:
	//! Get internal float min value
	Float GetInternalFloatVariableMin( CName name ) const;

	//! Get internal float max value
	Float GetInternalFloatVariableMax( CName name ) const;

	//! Get internal float default value
	Float GetInternalFloatVariableDefault( CName name ) const;

	//! Get internal vector min value
	Vector GetInternalVectorVariableMin( CName name ) const;

	//! Get internal vector max value
	Vector GetInternalVectorVariableMax( CName name ) const;

	//! Get internal vector default value
	Vector GetInternalVectorVariableDefault( CName name ) const;

 public:
	 //! Get runtime ( instance ) internal float variable
	 Float GetRuntimeInternalFloatVariable( const CBehaviorGraphInstance& instance, CName name ) const;

	 //! Get runtime ( instance ) internal vector variable
	 Vector GetRuntimeInternalVectorVariable( const CBehaviorGraphInstance& instance, CName name  ) const;

	 //! Set runtime ( instance ) internal float variable
	 Bool SetRuntimeInternalFloatVariable( CBehaviorGraphInstance& instance, CName name, Float value ) const;

	 //! Set runtime ( instance ) internal vector variable
	 Bool SetRuntimeInternalVectorVariable( CBehaviorGraphInstance& instance, CName name, const Vector& value) const;

	 //! Reset runtime ( instance ) internal float value
	 Float ResetRuntimeInternalFloatVariable( CBehaviorGraphInstance& instance, CName name ) const;

	 //! Reset runtime ( instance ) internal vector value
	 Vector ResetRuntimeInternalVectorVariable( CBehaviorGraphInstance& instance, CName name ) const;

	 //! Get runtime ( instance ) internal float variables num
	 Uint32 GetRuntimeInternalFloatVariablesNum( CBehaviorGraphInstance& instance ) const;

	 //! Get runtime ( instance ) internal vector variables num
	 Uint32 GetRuntimeInternalVectorVariablesNum( CBehaviorGraphInstance& instance ) const;

	 //! Store runtime internal float variables
	 void StoreRuntimeInternalFloatVariables( CBehaviorGraphInstance& instance, TDynArray< Float >& variables ) const;

	 //! Store runtime internal vector variables
	 void StoreRuntimeInternalVectorVariables( CBehaviorGraphInstance& instance, TDynArray< Vector >& variables ) const;

	 //! Restore runtime internal float variables
	 void RestoreRuntimeInternalFloatVariables( CBehaviorGraphInstance& instance, const TDynArray< Float >& variables ) const;

	 //! Restore runtime internal vector variables
	 void RestoreRuntimeInternalVectorVariables( CBehaviorGraphInstance& instance, const TDynArray< Vector >& variables ) const;

	 //! Reset all internal runtime variables
	 void ResetRuntimeInternalVariables( CBehaviorGraphInstance& instance ) const;

	 //! Fill runtime internal variables
	 void FillRuntimeInternalVariables( CBehaviorGraphInstance& instance ) const;

public:
	const CBehaviorVariablesList&		GetVariables() const	{ return m_variables; }
	CBehaviorVariablesList&				GetVariables()			{ return m_variables; }

	const CBehaviorVectorVariablesList&	GetVectorVariables() const	{ return m_vectorVariables; }
	CBehaviorVectorVariablesList&		GetVectorVariables()		{ return m_vectorVariables; }

	const CBehaviorVariablesList&		GetInternalVariables() const	{ return m_internalVariables; }
	CBehaviorVariablesList&				GetInternalVariables()			{ return m_internalVariables; }

	const CBehaviorVectorVariablesList&	GetInternalVectorVariables() const	{ return m_internalVectorVariables; }
	CBehaviorVectorVariablesList&		GetInternalVectorVariables()		{ return m_internalVectorVariables; }

	const CBehaviorEventsList&			GetEvents() const		{ return m_events; }
	CBehaviorEventsList&				GetEvents()				{ return m_events; }

	Int32 RemoveUnusedVariablesAndEvents();

public:
	CBehaviorGraphContainerNode* GetRootNode() const { return m_rootNode; }

	CBehaviorGraphNode* FindNodeById( Uint32 id ) const;

	CBehaviorGraphPoseSlotNode*	FindPoseSlot( const CName &slotName ) const;
	CBehaviorGraphAnimationSlotNode* FindAnimSlot( const CName &slotName ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	void CollectUsedVariablesAndEvents( TDynArray<CName>& variables, TDynArray<CName>& vectorVariables, TDynArray<CName>& events, TDynArray<CName>& internalVariables, TDynArray<CName>& internalVectorVariables ) const;

	void EnumVectorVariableNames( TDynArray< CName >& names ) const;
	void EnumVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect = false ) const;
	void EnumEventNames( TDynArray< CName> & names, Bool onlyModifiableByEffect = false ) const;
	void EnumInternalVectorVariableNames( TDynArray< CName >& names ) const;
	void EnumInternalVariableNames( TDynArray< CName >& names, Bool onlyModifiableByEffect = false ) const;

	Uint32 GetNumberOfNodes() const;
	void EnumUsedAnimations( TDynArray< CName >& animations ) const;

	void OnVariableNameChanged();

#endif

	// PTom TODO - w game sa id, nic po name nie powinno byc
	CBehaviorGraphNode*	FindNodeByName( const String &name ) const;
	CBehaviorGraphNode*	FindNodeByName( const String &name, CBehaviorGraphNode* startNode, Bool recursive ) const;

	void FindNodesByName( const String &name, TDynArray< CBehaviorGraphNode* >& nodesOut ) const;
	void FindNodesByName( const String &name, CBehaviorGraphNode* startNode, Bool recursive, TDynArray< CBehaviorGraphNode* >& nodesOut ) const;

	template< class T > void GetNodesOfClass( TDynArray< T* >& nodes ) const;

public:
	String GetCustomTrackNameStr( Int32 index ) const;
	String GetFloatTrackNameStr( Int32 index ) const;

	const CName& GetCustomTrackName( Int32 index ) const;
	const CName& GetFloatTrackName( Int32 index ) const;

	void SetCustomFloatTrackName( String name, Uint32 index );

protected:
	void CreateTrackNames();

	void CheckDefaultStateMachine();
};

BEGIN_CLASS_RTTI( CBehaviorGraph );
	PARENT_CLASS( CResource );
	PROPERTY( m_defaultStateMachine );
	PROPERTY( m_stateMachines );
	//PROPERTY( m_nodesToReleaseList ); // do not do it before 1.1 patch (witcher3)
	PROPERTY( m_sourceDataRemoved );
	PROPERTY_EDIT( m_customTrackNames, TXT("") );
	PROPERTY_EDIT( m_generateEditorFragments, TXT("Generate editor fragments") );
	PROPERTY( m_poseSlots );
	PROPERTY( m_animSlots );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

/// Behavior graph slot
struct SBehaviorGraphInstanceSlot
{
	DECLARE_RTTI_STRUCT( SBehaviorGraphInstanceSlot );

	CName							m_instanceName;
	THandle< CBehaviorGraph >		m_graph;
	Bool							m_alwaysOnTopOfStack;
	SBehaviorGraphInstanceSlot() : m_alwaysOnTopOfStack( false ) {}
};

BEGIN_CLASS_RTTI( SBehaviorGraphInstanceSlot );
	PROPERTY_EDIT( m_instanceName, TXT( "Instance name" ) );
	PROPERTY_EDIT( m_graph, TXT( "Graph resource" ) );
	PROPERTY_EDIT( m_alwaysOnTopOfStack, TXT("Behavior graph always will be on top of the stack (CONSULT THIS BEFORE CHECKING)") );
END_CLASS_RTTI();



//////////////////////////////////////////////////////////////////////////

struct SBehaviorGraphScriptNotification
{
	CName m_notification;
	CName m_sourceState;

	SBehaviorGraphScriptNotification() {}
	SBehaviorGraphScriptNotification(CName const & notification, CName const & sourceState)
		:	m_notification( notification )
		,	m_sourceState( sourceState )
	{}

	bool operator==(SBehaviorGraphScriptNotification const & other) const { return m_notification == other.m_notification && m_sourceState == other.m_sourceState; }
};

//////////////////////////////////////////////////////////////////////////

class CStackAllocator
{
	Uint8	*m_data;
	Uint32	m_offset;
	Uint32	m_size;

public:
	CStackAllocator( Uint32 size )
		: m_data( NULL )
		, m_offset( 0 )
		, m_size( 0 )	
	{
		m_data = new Uint8[ size ];
		m_size = size;
	}

	~CStackAllocator()
	{
		delete[] m_data;
		m_data = NULL;

		m_size = 0;
	}

	void* Allocate( Uint32 size )
	{
		ASSERT( ( m_offset + size + sizeof( Uint32 ) <= m_size ) && "Stack allocator overflow!" );

		// allocate some space for allocation size
		Uint32* allocSize = (Uint32*)(m_data + m_offset);		
		m_offset += sizeof( Uint32 );

		// remember size of allocation
		*allocSize = size;

		// return allocated memory
		void* retVal = m_data + m_offset;
		m_offset += size;

		return retVal;
	}

	void Deallocate( void *ptr )
	{
		// check if we try to free last allocated memory
		Uint8* typedPtr = (Uint8*)ptr;
		Uint32* allocSize = (Uint32*)( typedPtr - sizeof( Uint32 ) );
		ASSERT( ( typedPtr + *allocSize - m_data == (Int32)m_offset ) && "Trying to free memory that is not on the top of the stack!" );

		m_offset -= *allocSize;
		m_offset -= sizeof( Uint32 );
	}
};
