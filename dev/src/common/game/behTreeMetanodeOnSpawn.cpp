#include "build.h"
#include "behTreeMetanodeOnSpawn.h"

#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeMetanodeOnSpawnDefinition
////////////////////////////////////////////////////////////////////////

Bool IBehTreeMetanodeOnSpawnDefinition::IsTerminal() const
{
	return false;
}
Bool IBehTreeMetanodeOnSpawnDefinition::CanAddChild() const
{
	return m_childNode == NULL;
}
Int32 IBehTreeMetanodeOnSpawnDefinition::GetNumChildren() const
{
	return m_childNode ? 1 : 0;
}
IBehTreeNodeDefinition* IBehTreeMetanodeOnSpawnDefinition::GetChild( Int32 index ) const
{
	return m_childNode;
}
void IBehTreeMetanodeOnSpawnDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	ASSERT( node->GetParent() == this );
	ASSERT( m_childNode == NULL );
	m_childNode = node;
}
void IBehTreeMetanodeOnSpawnDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( m_childNode )
	{
		m_childNode= NULL;
	}	
}
void IBehTreeMetanodeOnSpawnDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< IBehTreeMetanodeOnSpawnDefinition* >( this ) );
	if ( m_childNode )
	{
		m_childNode->CollectNodes( nodes );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void IBehTreeMetanodeOnSpawnDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );
	if ( m_childNode )
	{
		m_childNode->OffsetNodesPosition( offsetX, offsetY );
	}
}
#endif

IBehTreeNodeInstance* IBehTreeMetanodeOnSpawnDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	IBehTreeNodeInstance* ret = NULL;
	if ( m_childNode )
	{
		ret = m_childNode->SpawnInstance( owner, context, parent );
	}

	if ( !context.IsInDynamicBranch() && m_runWhenReattachedFromPool )
	{
		owner->AddMetanodeToReattachCallback( this );
	}

	return ret;
}

Bool IBehTreeMetanodeOnSpawnDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	// Run effector
	RunOnSpawn( context, node->GetOwner() );
	if ( m_childNode )
	{
		return m_childNode->OnSpawn( node, context );
	}
	return false;
}

void IBehTreeMetanodeOnSpawnDefinition::OnReattach( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	RunOnSpawn( context, owner );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeOnSpawnEffector
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBehTreeOnSpawnEffector );


void IBehTreeOnSpawnEffector::Run( CBehTreeSpawnContext* context, CBehTreeInstance* owner ) const
{
	m_context = context;
	m_owner = owner;
	struct Guard
	{
		Guard( const IBehTreeOnSpawnEffector* me )
			: m_me( me ) {}
		~Guard()
		{
			m_me->m_context = NULL;
			m_me->m_owner = NULL;
		}
		const IBehTreeOnSpawnEffector* m_me;
	} guard( this );

	IBehTreeOnSpawnEffector* me = const_cast< IBehTreeOnSpawnEffector* >( this ); 
	if ( !CallFunction( me, CNAME( Run ) ) )
	{
		LOG_GAME( TXT("Problem while running script of '%ls'"), GetClass()->GetName().AsString().AsChar() );
	}
}

void IBehTreeOnSpawnEffector::funcGetActor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	THandle< CActor > actor = m_owner->GetActor();
	RETURN_HANDLE( CActor, actor );
}

void IBehTreeOnSpawnEffector::funcGetObjectFromAIStorage( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, paramName, CName::NONE );
	FINISH_PARAMETERS;

	THandle< IScriptable > obj;
	
	CAIStorageItem* storageItem = m_owner->GetItem( paramName );
	if ( storageItem )
	{
		IScriptable** purePtr = storageItem->GetPtr< IScriptable* >();
		if ( purePtr )
		{
			obj = *purePtr;
		}
		else
		{
			THandle< IScriptable >* handlerPtr = storageItem->GetPtr< THandle< IScriptable > >();
			if ( handlerPtr )
			{
				obj = *handlerPtr;
			}
		}
	}

	RETURN_HANDLE( IScriptable, obj );
}



////////////////////////////////////////////////////////////////////////
// CBehTreeMetanodeScriptOnSpawnDefinition
////////////////////////////////////////////////////////////////////////

void CBehTreeMetanodeScriptOnSpawnDefinition::RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const
{
	IBehTreeOnSpawnEffector* effector = m_scriptOnSpawn.Get();
	if ( effector )
	{
		effector->Run( &context, owner );
	}
}
String CBehTreeMetanodeScriptOnSpawnDefinition::GetNodeCaption() const
{
	IBehTreeOnSpawnEffector* effector = m_scriptOnSpawn.Get();
	if ( effector )
	{
		String name( String::Printf( TXT( "OnSpawn '%ls'" ), effector->GetClass()->GetName().AsString().AsChar() ) );
		return name;
	}
	else
	{
		return TXT( "OnSpawn EMPTY SCRIPT" );
	}
}

Bool CBehTreeMetanodeScriptOnSpawnDefinition::IsValid() const
{
	if ( m_scriptOnSpawn.Get() == nullptr )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
