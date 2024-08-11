/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "../core/classBuilder.h"
#include "../core/instanceVar.h"
#include "../engine/graphBlock.h"

struct SBehaviorGraphOutput;
struct SBehaviorUpdateContext;
struct SBehaviorSampleContext;
class IBehaviorGraphNotifier;
class CBehaviorEvent;
class CBehaviorGraphDebugInfo;
class CBehaviorGraphStateNode;
class CBehaviorGraphValueNode;
class CBehaviorGraphInstance;
class CBehaviorGraphVectorValueNode;
class CAnimatedComponent;
class InstanceBuffer;
class CInstancePropertiesBuilder;
struct CSyncInfo;
class CSkeleton;
class CRenderFrame;
class CBehaviorGraph;
struct SBehaviorUsedAnimationData;
class InstanceDataLayoutCompiler;

enum EBehaviorLod : CEnum::TValueType;

//////////////////////////////////////////////////////////////////////////

#define TRACK_BEH_NODES_SAMPLE_ID

#if defined( TRACK_BEH_NODES_SAMPLE_ID ) && !defined( NO_EDITOR_GRAPH_SUPPORT )
#define CHECK_SAMPLE_ID {\
const Uint32 contextID = context.GetSampledID();\
Uint32& sampleID = instance[ i_activeSampleID ];\
Uint32& sampleCounter = instance[ i_activeSampleCounter ];\
if ( contextID != sampleID )\
{\
	sampleID = contextID;\
	sampleCounter = 0;\
}\
else\
{\
	++sampleCounter;\
}\
}
#else
#define CHECK_SAMPLE_ID
#endif

//////////////////////////////////////////////////////////////////////////

#define USE_DEBUG_ANIM_SAMPLE

#ifdef USE_DEBUG_ANIM_SAMPLE
#define ANIM_NODE_PRE_SAMPLE DEBUG_ANIM_POSES( output )
#define ANIM_NODE_POST_SAMPLE DEBUG_ANIM_POSES( output )
#else
#define ANIM_NODE_PRE_SAMPLE
#define ANIM_NODE_POST_SAMPLE
#endif

//////////////////////////////////////////////////////////////////////////

//#define USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER

#ifdef USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER

#define USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER_CODE( ... ) __VA_ARGS__

typedef void (CBehaviorGraphNode::*TReleaseInstanceFunc)(CBehaviorGraphInstance& instance ) const;

class CVitrualMethodTester
{
public:
	CVitrualMethodTester( const void* object, Uint32 index );

	Bool HasSameVirtualMethod( const void* object ) const;

	static const Uint32 GetVTableFunctionIndex( const void* object, TReleaseInstanceFunc func );

private:
	static const void* GetVTableMethod( const void* object, const Uint32 index );
	static const void* GetTrueMethodAddress( const void* method );

private:
	union UnionTrick
	{
		TReleaseInstanceFunc		m_func;
		void*						m_data[4];
	};

	Uint32			m_index;
	const void*		m_method;
};

#else

#define USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER_CODE( ... ) ;

#endif

//////////////////////////////////////////////////////////////////////////

#define DECLARE_BEHAVIOR_ABSTRACT_CLASS( _class, _base ) \
	DECLARE_ENGINE_ABSTRACT_CLASS( _class, _base ) \
	USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER_CODE( virtual Bool IsOnReleaseInstanceAutoOverridden() const { return false; } )

//////////////////////////////////////////////////////////////////////////

struct CBehaviorNodeSyncData
{
	DECLARE_RTTI_STRUCT( CBehaviorNodeSyncData );

public: // PTom: TODO - this should be private
	Float	m_timeMultiplier;

public:
	CBehaviorNodeSyncData() 
		: m_timeMultiplier( 1.0f )
	{
	}

	Float ProcessTimeDelta( Float timeDelta ) const
	{
		return timeDelta * m_timeMultiplier;
	}

	void Reset()
	{
		m_timeMultiplier = 1.0f;
	}

	void Set( Float factor )
	{
		m_timeMultiplier = factor;
	}
};

BEGIN_CLASS_RTTI( CBehaviorNodeSyncData );
	PROPERTY( m_timeMultiplier );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

/// Behavior node
class CBehaviorGraphNode : public CGraphBlock
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphNode, CGraphBlock );

protected:	
	TInstanceVar< Int32 >					i_active;			//!< Is the behavior block active
	TInstanceVar< Uint32 >					i_activeUpdateID;	//!< ID of last update to this node
#ifdef TRACK_BEH_NODES_SAMPLE_ID
	TInstanceVar< Uint32 >					i_activeSampleID;	//!< ID of last sample to this node
	TInstanceVar< Uint32 >					i_activeSampleCounter;	//!< ID of last sample to this node
#endif
	TInstanceVar< Float	>					i_activationAlpha;	//!< How much this node contributes to output pose ( 0.0f - 1.0f ), debug mostly
	TInstanceVar< CBehaviorNodeSyncData	>	i_syncData;			//!< Node synchronization data

protected:
	// PTom TODO - do wywalenia, w game sa id
	String						m_name;		

	Bool						m_generateEditorFragments;		//!< Generate editor fragments from this node
	CName						m_activateNotification;			//!< Activate notification
	CName						m_deactivateNotification;		//!< Deactivate notification
	Uint32						m_id;							//!< Block's id

#ifndef NO_EDITOR_GRAPH_SUPPORT									//!< Name of the behavior node
	Color						m_color;						//!< Color of the node
	Bool						m_deprecated;					//!< Is block deprecated
	String						m_logOnActivation;				//!< Log text on activation
#endif

#ifndef NO_EDITOR
	Int32						m_debugCodeId;
	Bool						m_debugCodeBreak_Act;
	Bool						m_debugCodeBreak_Deact;
#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	//! Set node deprecated
	RED_INLINE void SetDeprecated( Bool flag, const String& comment = String::EMPTY ) { m_deprecated = flag; if ( m_deprecated && !comment.Empty() ) m_name = comment; else m_name = String::EMPTY; }

	//! Is node deprecated
	RED_INLINE Bool IsDeprecated() const { return m_deprecated; }

	//! Get deprecated comment
	RED_INLINE const String& GetDeprecateComment() const { return m_deprecated ? m_name : String::EMPTY; }

	//! Get block client color
	Color GetClientColor() const;

	//! Validates node in the editor, returns false if any problems encountered, in such case outHumanReadableErrorDesc will contain description of errors.
	virtual Bool ValidateInEditor( TDynArray< String >& /*outHumanReadableErrorDesc*/) const { return true; }
#endif

public:
	//! Get ID
	RED_INLINE Uint32 GetId() const { return m_id; }

	//! Set ID
	RED_INLINE void SetId( Uint32 id ) { m_id = id; }

	//! Set name of a notification transmitted when the node is activated
	RED_INLINE void SetActivateNotification( const CName& notification ) { m_activateNotification = notification; }

	//! Set name of a notification transmitted when the node is deactivated
	RED_INLINE void SetDeactivateNotification( const CName& notification ) { m_deactivateNotification = notification; }

	//! Returns the name of a notification transmitted when the node is activated
	RED_INLINE const CName& GetActivateNotification() const { return m_activateNotification; }

	//! Returns the name of a notification transmitted when the node is deactivated
	RED_INLINE const CName& GetDeactivateNotification() const { return m_deactivateNotification; }

public:
	CBehaviorGraphNode();
	
	//! get node name
	virtual const String & GetName() const { return m_name; }

public:
	//! Build block data layout
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! Initialize instance buffer
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	//! Destroy instance buffer
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const { return false; }

	//! Build node instance properties
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	// Activate the block.
	void Activate( CBehaviorGraphInstance& instance ) const;

	// Deactivate the bock
	void Deactivate( CBehaviorGraphInstance& instance ) const;

	//! Called on node activation
	virtual void OnActivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! Called on node deactivation
	virtual void OnDeactivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! reset node to default state
	void Reset( CBehaviorGraphInstance& instance ) const;

	//! called during node reset
	virtual void OnReset( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! Check if the node is active
	Bool IsActive( const CBehaviorGraphInstance& instance ) const;

	//! Get node's activation num, use for debug reason 
	Int32 GetActiveNum( const CBehaviorGraphInstance& instance ) const;

#ifdef TRACK_BEH_NODES_SAMPLE_ID
	Uint32 GetSampledNum( const CBehaviorGraphInstance& instance ) const;
#endif

	//! update node
	void Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! update node
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const = 0;

	//! sample data from node
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const = 0; 

	//! Get block activation percentage
	virtual Float GetActivationAlpha( const CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	using CGraphBlock::GetActivationAlpha;
#endif

	//! Set block activation percentage (debug purposes only)
	void SetActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! get sync info
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const = 0;

	//! synchronize to given sync info
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const = 0;

	//! process external event, return if the node processed event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const = 0;

	//! process force external event, return if the node processed event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& /*instance*/, const CBehaviorEvent &/*event*/ ) const { return false; }

	//! called during open in editor
	virtual void OnOpenInEditor( CBehaviorGraphInstance& /*instance*/ ) const {}

	//! Is mimic block
	virtual Bool IsMimic() const { return false; }

public:
	//! Cache block connections
	virtual void CacheConnections();

	//! Remove sockets and connections ( after caching )
	virtual void RemoveConnections();

	//! Cache connection to block
	CBehaviorGraphNode* CacheBlock( const String& inputName ) const;
	CBehaviorGraphNode* CacheBlock( const CName& inputName ) const;

	//! Cache connection to mimic block
	CBehaviorGraphNode* CacheMimicBlock( const String& inputName ) const;
	CBehaviorGraphNode* CacheMimicBlock( const CName& inputName ) const;

	//! Cache connection to block with value
	CBehaviorGraphValueNode* CacheValueBlock( const String& inputName ) const;
	CBehaviorGraphValueNode* CacheValueBlock( const CName& inputName ) const;

	//! Cache connection to block with vector value
	CBehaviorGraphVectorValueNode* CacheVectorValueBlock( const String& inputName ) const;

	//! Cache connection to block
	CBehaviorGraphStateNode* CacheStateBlock( const String& inputName ) const;

	//! Get used variables and events
	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& /*var*/, TDynArray<CName>& /*vecVar*/, TDynArray<CName>& /*events*/, TDynArray<CName>& /*intVar*/, TDynArray<CName>& /*intVecVar*/ ) const {}

	//! Get used animations
	virtual void CollectUsedAnimations( TDynArray< CName >& /*anims*/ ) const {}

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const CName& name, const CBehaviorGraphInstance& instance ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const String& name, const CBehaviorGraphInstance& instance ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const Char* name, const CBehaviorGraphInstance& instance ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const CName& name, const CAnimatedComponent* component ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const String& name, const CAnimatedComponent* component ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const Char* name, const CAnimatedComponent* component ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const CName& name, const CSkeleton* skeleton ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const String& name, const CSkeleton* skeleton ) const;

	//! Find bone index by name
	virtual Int32 FindBoneIndex( const Char* name, const CSkeleton* skeleton ) const;

	//! Find track index by name
	virtual Int32 FindTrackIndex( const String& trackName, const CSkeleton* skeleton ) const;

	//! Find track index by name
	virtual Int32 FindTrackIndex( const Char* trackName, const CSkeleton* skeleton ) const;

	//! Generate fragments
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	//! Reset animation cache
	virtual void OnUpdateAnimationCache( CBehaviorGraphInstance& instance ) const;

	//! Preload animations
	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

	//! Dump error
	void DumpError( CBehaviorGraphInstance& instance, const String& msg ) const;

	//! Dump warning
	void DumpWarn( CBehaviorGraphInstance& instance, const String& msg ) const;

public:
	//! get sync data (const access)
	const CBehaviorNodeSyncData& GetSyncData( const CBehaviorGraphInstance& instance ) const;

	//! get sync data
	CBehaviorNodeSyncData& GetSyncData( CBehaviorGraphInstance& instance ) const;

public:
	//! do all work before loading snapshot (and modify snapshot if needed, before it is restored)
	virtual void OnLoadingSnapshot( CBehaviorGraphInstance& /*instance*/, InstanceBuffer& /*snapshotData*/ ) const {}

	//! do all restoration work after loading snapshot
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& /*instance*/, const InstanceBuffer& /*previousData*/ ) const {}

public:
	const CBehaviorGraph* GetGraph() const;

	//! only for editor
	CBehaviorGraph* GetGraph();

	CBehaviorGraphNode* GetParentNode() const;

protected:
	//! Get socket by name
	CGraphSocket* FindSocket( const String& name ) const
	{
		return CGraphBlock::FindSocket( name );
	}

	//! Get socket by name ( faster )
	CGraphSocket* FindSocket( const CName& name ) const
	{
		return CGraphBlock::FindSocket( name );
	}

	//! Find socket of given type
	template< class T > T* FindSocket( const CName& name ) const
	{
		return CGraphBlock::FindSocket<T>( name );
	}

	USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER_CODE( static const CVitrualMethodTester& GetOnReleaseInstanceTester(); )

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT

	// This is helper function for editor
	virtual Bool WorkWithLod( EBehaviorLod /*lod*/ ) const { return true; }

	// This is helper function that collects animation usage data for animation usage panel, it collects from this single node
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& /*instance*/, TDynArray<SBehaviorUsedAnimationData>& /*collectorArray*/ ) const {}

#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphNode );
	PARENT_CLASS( CGraphBlock );
	PROPERTY_EDIT( m_name, TXT("Node name") );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_EDIT_NOT_COOKED( m_color, TXT("Color"));
#endif
	PROPERTY_EDIT( m_activateNotification, TXT("Activate notification") );
	PROPERTY_EDIT( m_deactivateNotification, TXT("Deactivate notification") );
	PROPERTY_EDIT( m_generateEditorFragments, TXT("Generate editor fragments for debug") );
	PROPERTY( m_id );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_EDIT_NOT_COOKED( m_logOnActivation, TXT("Log text on activation"));
#endif
#ifndef NO_EDITOR
	PROPERTY_EDIT( m_debugCodeId, TXT("") );
	PROPERTY_EDIT( m_debugCodeBreak_Act, TXT("") );
	PROPERTY_EDIT( m_debugCodeBreak_Deact, TXT("") );
#endif
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

//! Base behavior node - one input, one output, no synchronization
class CBehaviorGraphBaseNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphBaseNode, CBehaviorGraphNode );

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphBaseNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

#ifdef USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER

class CBehaviorGraphNodeDummy : public CBehaviorGraphNode
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphNodeDummy, CBehaviorGraphNode, 0 );

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {} 
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const  {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const {}
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const { return false; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphNodeDummy );
	PARENT_CLASS( CBehaviorGraphNode );
END_CLASS_RTTI();

#endif

//////////////////////////////////////////////////////////////////////////

#define DECLARE_BEHAVIOR_NODE_CLASS( _class, _base, _category, _name ) \
	DECLARE_ENGINE_CLASS( _class, _base, 0 ) \
	public:\
	virtual String GetBlockName() const { return TXT(_name); } \
	virtual String GetBlockCategory() const { return TXT(_category); } \
	USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER_CODE( virtual Bool IsOnReleaseInstanceAutoOverridden() const { return !GetOnReleaseInstanceTester().HasSameVirtualMethod( this ); } )

//////////////////////////////////////////////////////////////////////////
