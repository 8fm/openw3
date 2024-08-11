/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphNotifier.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/animatedComponent.h"
#include "../engine/entity.h"
#include "../engine/skeleton.h"
#include "../engine/graphConnectionRebuilder.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorNodeSyncData );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphNode );

CBehaviorGraphNode::CBehaviorGraphNode()
	: m_id( 0 )
#ifndef NO_EDITOR_GRAPH_SUPPORT
	, m_color(142, 142, 142)
#endif
#ifndef NO_EDITOR
	, m_debugCodeId( 0 )
	, m_debugCodeBreak_Act( false )
	, m_debugCodeBreak_Deact( false )
#endif
{
}

void CBehaviorGraphNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_active;
	compiler << i_activeUpdateID;
#ifdef TRACK_BEH_NODES_SAMPLE_ID
	compiler << i_activeSampleID;
	compiler << i_activeSampleCounter;
#endif
	compiler << i_activationAlpha;
	compiler << i_syncData;
}

void CBehaviorGraphNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	instance[ i_active ] = 0;
	instance[ i_activeUpdateID ] = 0xFFFFFFFF;
#ifdef TRACK_BEH_NODES_SAMPLE_ID
	instance[ i_activeSampleID ] = 0xFFFFFFFF;
	instance[ i_activeSampleCounter ] = 0;
#endif
	instance[ i_activationAlpha ] = 0.f;
	instance[ i_syncData ].Reset();

	ASSERT( i_active.GetOffset() != 0xFFFFFFFF );
}

void CBehaviorGraphNode::OnReleaseInstance( CBehaviorGraphInstance& /*instance*/ ) const
{

}

void CBehaviorGraphNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	INST_PROP_INIT;
	INST_PROP( i_active );
	INST_PROP( i_activationAlpha );
	INST_PROP( i_syncData );
}

void CBehaviorGraphNode::Activate( CBehaviorGraphInstance& instance ) const
{
	const Bool wasActive = IsActive( instance );

	++instance[ i_active ];

	const Bool isActive = IsActive( instance );

	if ( !wasActive && isActive )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( ! m_logOnActivation.Empty() )
		{
			BEH_LOG( TXT("Node %s of %s activated : %s"), GetName().AsChar(), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar(), m_logOnActivation.AsChar() );
		}
#endif

#ifndef NO_EDITOR
		if ( m_debugCodeBreak_Act )
		{
			DebugBreak();
		}
#endif

		instance[ i_syncData ].Reset();
		OnActivated( instance );

		if ( m_activateNotification != CName::NONE )
		{
			instance.NotifyOfNodesActivation( m_activateNotification );
		}

		//PreloadAnimations( instance );
	}
}

void CBehaviorGraphNode::Deactivate( CBehaviorGraphInstance& instance ) const
{
	const Bool wasActive = IsActive( instance );

	// we shouldn't deactivate below 0 (nodes are sometimes deactivated no matter if they're active - eg. when deactivating composite node)
	if ( wasActive  )
	{
		--instance[ i_active ];

		const Bool isActive = IsActive( instance );

		if ( !isActive )
		{
#ifndef NO_EDITOR
			if ( m_debugCodeBreak_Deact )
			{
				DebugBreak();
			}
#endif

			OnDeactivated( instance );

			if ( m_deactivateNotification != CName::NONE )
			{
				instance.NotifyOfNodesDeactivation( m_deactivateNotification );
			}
		}
	}

#ifdef TRACK_BEH_NODES_SAMPLE_ID
	instance[ i_activeSampleCounter ] = 0;
#endif
}

void CBehaviorGraphNode::Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	//PC_SCOPE_DYNAMIC(this->GetClass()->GetClassIndex(),(TXT("Behavior/")+this->GetClass()->GetName().AsString()),Behavior,PT_MainThread);
	if ( context.m_updateID != instance[ i_activeUpdateID ] )
	{
		// We must be active when we are here
		if ( ! IsActive( instance ) )
		{
#ifdef _DEBUG
			ASSERT( IsActive( instance ) );
#endif
			return;
		}

		// Calculate update time delta
		timeDelta = instance[ i_syncData ].ProcessTimeDelta( timeDelta );

		// Remember that this node was updated
		instance[ i_activeUpdateID ] = context.m_updateID;

		// Update node
		OnUpdate( context, instance, timeDelta );

#ifndef NO_EDITOR
		// Listener
		if ( instance.HasEditorListener() )
		{
			instance.GetEditorListener()->OnNodeUpdate( this );
		}
#endif
	}
}

void CBehaviorGraphNode::Reset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_active ] = 0;
	instance[ i_syncData ].Reset();

	OnReset( instance );
}

Bool CBehaviorGraphNode::IsActive( const CBehaviorGraphInstance& instance ) const 
{ 
	RED_ASSERT( i_active.GetOffset() != 0xFFFFFFFF, TXT("i_active offset is 0xFFFFFFFF, check instanceVar.SetOffset") );
	return instance[ i_active ] > 0; 
}

Int32 CBehaviorGraphNode::GetActiveNum( const CBehaviorGraphInstance& instance ) const 
{
	RED_FATAL_ASSERT( i_active.GetOffset() != 0xFFFFFFFF, "i_active offset is 0xFFFFFFFF, check instanceVar.SetOffset" );
	return instance[ i_active ]; 
}

#ifdef TRACK_BEH_NODES_SAMPLE_ID
Uint32 CBehaviorGraphNode::GetSampledNum( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_activeSampleCounter ]; 
}
#endif

Float CBehaviorGraphNode::GetActivationAlpha( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_activationAlpha ];
}

void CBehaviorGraphNode::SetActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	instance[ i_activationAlpha ] = alpha;
}

void CBehaviorGraphNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	SetActivationAlpha( instance, Max( alpha, GetActivationAlpha( instance ) ) );
}

void CBehaviorGraphNode::OnGenerateFragments( CBehaviorGraphInstance& /*instance*/, CRenderFrame* /*frame*/ ) const
{
}

void CBehaviorGraphNode::OnUpdateAnimationCache( CBehaviorGraphInstance& /*instance*/ ) const
{
}

Bool CBehaviorGraphNode::PreloadAnimations( CBehaviorGraphInstance& /*instance*/ ) const
{
	return true;
}

void CBehaviorGraphNode::DumpError( CBehaviorGraphInstance& instance, const String& msg ) const
{
	#if !defined(NO_LOG)
	const String& parentName = GetParentNode() ? GetParentNode()->GetName() : TXT("<empty>");

	BEH_ERROR( TXT("----- ERROR -----") );
	BEH_ERROR( TXT("   Msg: %s"), msg.AsChar() );
	BEH_ERROR( TXT("   Node: %s "), GetName().AsChar() );
	BEH_ERROR( TXT("   Parent: %s "), parentName.AsChar() );
	BEH_ERROR( TXT("   Component: %s "), instance.GetAnimatedComponent()->GetName().AsChar() );
	BEH_ERROR( TXT("   Graph: %s "), instance.GetGraph()->GetDepotPath().AsChar() );
	BEH_ERROR( TXT("   Instance: %s "), instance.GetInstanceName().AsString().AsChar() );
	BEH_ERROR( TXT("   Entity: %s "), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	BEH_ERROR( TXT("-----------------") );

	#endif
}

void CBehaviorGraphNode::DumpWarn( CBehaviorGraphInstance& instance, const String& msg ) const
{
	#if !defined(NO_LOG)
	const String& parentName = GetParentNode() ? GetParentNode()->GetName() : TXT("<empty>");

	BEH_WARN( TXT("----- WARNING -----") );
	BEH_WARN( TXT("   Msg: %s"), msg.AsChar() );
	BEH_WARN( TXT("   Node: %s "), GetName().AsChar() );
	BEH_WARN( TXT("   Parent: %s "), parentName.AsChar() );
	BEH_WARN( TXT("   Component: %s "), instance.GetAnimatedComponent()->GetName().AsChar() );
	BEH_WARN( TXT("   Graph: %s "), instance.GetGraph()->GetDepotPath().AsChar() );
	BEH_WARN( TXT("   Instance: %s "), instance.GetInstanceName().AsString().AsChar() );
	BEH_WARN( TXT("   Entity: %s "), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	BEH_WARN( TXT("-----------------") );
	#endif
}

CBehaviorNodeSyncData& CBehaviorGraphNode::GetSyncData( CBehaviorGraphInstance& instance ) const 
{ 
	return instance[ i_syncData ]; 
}

const CBehaviorNodeSyncData& CBehaviorGraphNode::GetSyncData( const CBehaviorGraphInstance& instance ) const 
{ 
	return instance[ i_syncData ]; 
}

const CBehaviorGraph* CBehaviorGraphNode::GetGraph() const
{
	return FindParent< CBehaviorGraph >();
}

CBehaviorGraph* CBehaviorGraphNode::GetGraph()
{
	return FindParent< CBehaviorGraph >();
}

CBehaviorGraphNode* CBehaviorGraphNode::GetParentNode() const
{
	return Cast< CBehaviorGraphNode >( GetParent() );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CBehaviorGraphNode::GetClientColor() const
{
	static Float hilight = 0.25f; // 25%

	if ( HasFlag( OF_Highlighted ) )
	{
		Color color = m_color;
		color.R = (Uint8)((float)color.R*hilight);
		color.G = (Uint8)((float)color.G*hilight);
		color.B = (Uint8)((float)color.B*hilight);
		color.R = Clamp<Uint8>(color.R, 0, 255);
		color.G = Clamp<Uint8>(color.G, 0, 255);
		color.B = Clamp<Uint8>(color.B, 0, 255);
		return color;
	}
	else
	{
		return m_color;
	}
}

#endif

CBehaviorGraphStateNode* CBehaviorGraphNode::CacheStateBlock( const String& inputName ) const
{
	CBehaviorGraphTransitionSocket* socket = Cast< CBehaviorGraphTransitionSocket >( FindSocket( inputName  ) );
	if ( !socket )
	{
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphNode* CBehaviorGraphNode::CacheBlock( const String& inputName ) const
{
	// Find socket
	CBehaviorGraphAnimationInputSocket* socket = Cast< CBehaviorGraphAnimationInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
//		WARN_ENGINE( TXT("Socket '%ls' not found when caching animation input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphNode* CBehaviorGraphNode::CacheBlock( const CName& inputName ) const
{
	// Find socket
	CBehaviorGraphAnimationInputSocket* socket = Cast< CBehaviorGraphAnimationInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
		//		WARN_ENGINE( TXT("Socket '%ls' not found when caching animation input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphNode* CBehaviorGraphNode::CacheMimicBlock( const String& inputName ) const
{
	// Find socket
	CBehaviorGraphMimicAnimationInputSocket* socket = Cast< CBehaviorGraphMimicAnimationInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
		//		WARN_ENGINE( TXT("Socket '%ls' not found when caching animation input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphNode* CBehaviorGraphNode::CacheMimicBlock( const CName& inputName ) const
{
	// Find socket
	CBehaviorGraphMimicAnimationInputSocket* socket = Cast< CBehaviorGraphMimicAnimationInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
		//		WARN_ENGINE( TXT("Socket '%ls' not found when caching animation input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphValueNode* CBehaviorGraphNode::CacheValueBlock( const String& inputName ) const
{
	// Find socket
RED_MESSAGE(  "Can we change these FindSocket() calls to use CNames instead of Strings?" )
	CBehaviorGraphVariableInputSocket* socket = Cast< CBehaviorGraphVariableInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
//		WARN_ENGINE( TXT("Socket '%ls' not found when caching value input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphValueNode* CBehaviorGraphNode::CacheValueBlock( const CName& inputName ) const
{
	// Find socket
	RED_MESSAGE(  "Can we change these FindSocket() calls to use CNames instead of Strings?" )
		CBehaviorGraphVariableInputSocket* socket = Cast< CBehaviorGraphVariableInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
		//		WARN_ENGINE( TXT("Socket '%ls' not found when caching value input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

CBehaviorGraphVectorValueNode* CBehaviorGraphNode::CacheVectorValueBlock( const String& inputName ) const
{
	// Find socket
	CBehaviorGraphVectorVariableInputSocket* socket = Cast< CBehaviorGraphVectorVariableInputSocket >( FindSocket( inputName ) );
	if ( !socket )
	{
//		WARN_ENGINE( TXT("Socket '%ls' not found when caching vector value input of '%ls'"), inputName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Get connected node
	return socket->GetConnectedNode();
}

void CBehaviorGraphNode::CacheConnections()
{
	
}

void CBehaviorGraphNode::RemoveConnections()
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	BreakAllLinks();
	RemoveAllSockets();
#endif
}

Int32 CBehaviorGraphNode::FindBoneIndex( const CName& name, const CSkeleton* skeleton ) const
{
	ASSERT( skeleton );
	return name && skeleton ? skeleton->FindBoneByName( name ) : -1;
}

Int32 CBehaviorGraphNode::FindBoneIndex( const CName& name, const CAnimatedComponent* component ) const
{
	ASSERT( component );
	ASSERT( component->GetSkeleton() );
	return FindBoneIndex( name, component->GetSkeleton() );
}

Int32 CBehaviorGraphNode::FindBoneIndex( const CName& name, const CBehaviorGraphInstance& instance ) const
{
	return FindBoneIndex( name, instance.GetAnimatedComponent() );
}

Int32 CBehaviorGraphNode::FindBoneIndex( const String& name, const CSkeleton* skeleton ) const
{
	return FindBoneIndex( name.AsChar(), skeleton );
}

Int32 CBehaviorGraphNode::FindBoneIndex( const Char* name, const CSkeleton* skeleton ) const
{
	if ( name && skeleton )
	{
		//dex++: use general CSkeleton interface
		return skeleton->FindBoneByName( name );
		//dex--
	}

	// No bone found
	return -1;
}

Int32 CBehaviorGraphNode::FindBoneIndex( const String& name, const CAnimatedComponent* component ) const
{
	return FindBoneIndex( name.AsChar(), component );
}

Int32 CBehaviorGraphNode::FindBoneIndex( const Char* name, const CAnimatedComponent* component ) const
{
	// Find bone in skeleton and apply transformation
	if ( component )
	{
		return component->FindBoneByName( name );
	}

	// No bone found
	return -1;
}

Int32 CBehaviorGraphNode::FindBoneIndex( const String& name, const CBehaviorGraphInstance& instance ) const
{
	return FindBoneIndex( name.AsChar(), instance );
}

Int32 CBehaviorGraphNode::FindBoneIndex( const Char* boneName, const CBehaviorGraphInstance& instance ) const
{
	// Get animated component we are working on
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	ASSERT( ac );
	
	return FindBoneIndex( boneName, ac );
}

Int32 CBehaviorGraphNode::FindTrackIndex( const String& trackName, const CSkeleton* skeleton ) const
{
	return FindTrackIndex( trackName.AsChar(), skeleton );
}

Int32 CBehaviorGraphNode::FindTrackIndex( const Char* name, const CSkeleton* skeleton ) const
{
	//dex++: use general CSkeleton interface
	return skeleton->FindTrackByName( name );
	//dex--
}

#ifdef USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER

const CVitrualMethodTester& CBehaviorGraphNode::GetOnReleaseInstanceTester()
{
	/*static CBehaviorGraphNodeDummy* dummy = new CBehaviorGraphNodeDummy();*/
	static CVitrualMethodTester tester( CBehaviorGraphNodeDummy::GetStaticClass()->GetDefaultObject< CBehaviorGraphNodeDummy >(), CVitrualMethodTester::GetVTableFunctionIndex( CBehaviorGraphNodeDummy::GetStaticClass()->GetDefaultObject< CBehaviorGraphNodeDummy >(), &CBehaviorGraphNode::OnReleaseInstance ) );
	return tester;
}

#endif

//////////////////////////////////////////////////////////////////////////

#ifdef USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER

CVitrualMethodTester::CVitrualMethodTester( const void* object, Uint32 index )
	: m_index( index )
{
	m_method = GetTrueMethodAddress( GetVTableMethod( object, index ) ); 
}

Bool CVitrualMethodTester::HasSameVirtualMethod( const void* object ) const
{
	const void* method = GetTrueMethodAddress( GetVTableMethod( object, m_index ) );
	return method == m_method;
}

const Uint32 CVitrualMethodTester::GetVTableFunctionIndex( const void* object, TReleaseInstanceFunc func )
{
	UnionTrick data;
	data.m_func = func;

	// shim function
	const void* shimFuncPtr = data.m_data[0];

	// get vcall function address
	const void* vcallFuncPtr = GetTrueMethodAddress( shimFuncPtr ); 

	// validate code in the vcall stub:
	// 48 8B 01             mov         rax,qword ptr [rcx]  
	// FF A0 XX XX 00 00    jmp         qword ptr [rax+XXX] <- magic offset to extract  

	const Uint8* code = (const Uint8* ) vcallFuncPtr;
	RED_FATAL_ASSERT( code[0] == 0x48, "Invalid vcall stub code" );
	RED_FATAL_ASSERT( code[1] == 0x8B, "Invalid vcall stub code" );
	RED_FATAL_ASSERT( code[2] == 0x01, "Invalid vcall stub code" );
	RED_FATAL_ASSERT( code[3] == 0xFF, "Invalid vcall stub code" );
	RED_FATAL_ASSERT( code[4] == 0xA0, "Invalid vcall stub code" );

	// magic instruction offset
	const Uint32 vtableFunctionIndex = *( const Uint32* )( code + 5 );
	//RED_FATAL_ASSERT( vtableFunctionIndex & 7 == 0, "Crap, not valid offset" );
	RED_FATAL_ASSERT( vtableFunctionIndex < 1000, "Crap, not valid offset" );
	return vtableFunctionIndex / 8;
}

const void* CVitrualMethodTester::GetVTableMethod( const void* object, const Uint32 index )
{
	void** vtable = *(void***) object;
	return vtable[ index ];
}

const void* CVitrualMethodTester::GetTrueMethodAddress( const void* method )
{
	if ( method == nullptr )
		return nullptr;

	const Uint8* code = ( const Uint8* ) method;
	if ( *code == 0xE9 )
	{
		// we know it's a function shim, we need to resolve the true address
		// what is stored in the code is the relative address
		const Int32 relAddress = *( const Int32* )( code + 1 );
		const Int64 nextInstrAddress = ( const Int64 )( code + 5 ); // size of instruction + data is 5 bytes: E8 xx xx xx xx
		return (const void*)( nextInstrAddress + relAddress ); // note: the relative address can be signed			
	}

	// assume that the method address is not modified
	return method;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBaseNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBaseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
}

#endif

void CBehaviorGraphBaseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphBaseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	ANIM_NODE_PRE_SAMPLE

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphBaseNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphBaseNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphBaseNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphBaseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphBaseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphBaseNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Input") );
}

void CBehaviorGraphBaseNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphBaseNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->PreloadAnimations( instance );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

#ifdef USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphNodeDummy );

#endif

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
