
#include "build.h"
#include "edSpawnTreeNode.h"

IMPLEMENT_RTTI_ENUM( ESpawnTreeType );

IEdSpawnTreeNode::IEdSpawnTreeNode()
	: m_id( 0 )
#ifndef NO_EDITOR_GRAPH_SUPPORT
	, m_graphPosX( 0 )
	, m_graphPosY( 50 )
	, m_hidden( HIDEN_NOT_INITIALIZED )
#endif
{
}

Bool IEdSpawnTreeNode::RemoveNullChildren()
{
	Uint32 nullChildrenCount = 0;
	for ( Uint32 i = 0, n = GetNumChildren(); i < n; ++i )
	{
		if ( GetChild( i ) == nullptr )
		{
			++nullChildrenCount;
		}
	}

	for ( Uint32 i = 0; i < nullChildrenCount; ++i )
	{
		RemoveChild( nullptr );
	}

	return nullChildrenCount > 0;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void IEdSpawnTreeNode::SetGraphPosition( Int32 x, Int32 y )
{
	m_graphPosX = x;
	m_graphPosY = y;
}
#endif

IScriptable* IEdSpawnTreeNode::GetObjectForPropertiesEdition()
{
	return AsCObject();
}
IEdSpawnTreeNode* IEdSpawnTreeNode::GetParentNode() const
{
	return nullptr;
}

Bool IEdSpawnTreeNode::CanAddChild() const
{
	return false;
}

void IEdSpawnTreeNode::AddChild( IEdSpawnTreeNode* node )
{
	ASSERT( false, TXT("Not implemented") );
}

void IEdSpawnTreeNode::RemoveChild( IEdSpawnTreeNode* node )
{
	ASSERT( false, TXT("Not implemented") );
}

Color IEdSpawnTreeNode::GetBlockColor() const
{
	return Color::WHITE;
}

String IEdSpawnTreeNode::GetBlockCaption() const
{
	return GetEditorFriendlyName();
}
Int32 IEdSpawnTreeNode::GetNumChildren() const
{
	return 0;
}
IEdSpawnTreeNode* IEdSpawnTreeNode::GetChild( Int32 index ) const
{
	ASSERT( false );
	return nullptr;
}
Bool IEdSpawnTreeNode::UpdateChildrenOrder()
{
	Bool dirty = false;
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		IEdSpawnTreeNode* child = GetChild( i );
		dirty = child->UpdateChildrenOrder() || dirty;
	}
	return dirty;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool IEdSpawnTreeNode::SetDefaultVisibilityRec( Bool effectOnlyUnitialized )
{
	Bool dirty = false;

	if ( !effectOnlyUnitialized || m_hidden == HIDEN_NOT_INITIALIZED )
	{
		SetHidden( IsHiddenByDefault() && CanBeHidden() );
	}

	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		IEdSpawnTreeNode* child = GetChild( i );
		dirty = (child && child->SetDefaultVisibilityRec( effectOnlyUnitialized )) || dirty;
	}
	return dirty;
}
void IEdSpawnTreeNode::SetHiddenRec( Bool state )
{
	if ( CanBeHidden() )
	{
		SetHidden( state );
	}
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		IEdSpawnTreeNode* child = GetChild( i );
		child->SetHiddenRec( state );
	}
}

#endif

Bool IEdSpawnTreeNode::IsHiddenByDefault() const
{
	return false;
}

Bool IEdSpawnTreeNode::CanBeHidden() const
{
	return false;
}

Bool IEdSpawnTreeNode::CanBeCopied() const
{
	return true;
}

Bool IEdSpawnTreeNode::IsLocked() const
{
	return false;
}

void IEdSpawnTreeNode::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	
}

void IEdSpawnTreeNode::GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats )
{
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{		
		IEdSpawnTreeNode* child = GetChild( i );
		child->GatherBudgetingStats( container, stats );
	}
}

String IEdSpawnTreeNode::GetEditorFriendlyName() const
{
	CObject* obj = const_cast< IEdSpawnTreeNode* >( this )->AsCObject();
	return obj->GetClass()->GetName().AsString();
}

String IEdSpawnTreeNode::GetBitmapName() const
{
	return String::EMPTY;
}

void IEdSpawnTreeNode::PreStructureModification()
{

}

IEdSpawnTreeNode::EDebugState IEdSpawnTreeNode::GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const
{
	return EDEBUG_NOT_RELEVANT;
}

String IEdSpawnTreeNode::GetBlockDebugCaption( const CSpawnTreeInstance& instanceBuffer ) const
{
	return GetBlockCaption();
}

Bool IEdSpawnTreeNode::HoldsInstanceBuffer() const
{
	return false;
}

CSpawnTreeInstance* IEdSpawnTreeNode::GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer )
{
	return const_cast< CSpawnTreeInstance* >( parentBuffer );
}

void IEdSpawnTreeNode::GetContextMenuSpecialOptions( SpecialOptionsList& outOptions )
{
	CObject* context = AsCObject();
	CallFunctionRef( context, CNAME( GetContextMenuSpecialOptions ), outOptions );
}

void IEdSpawnTreeNode::RunSpecialOption( Int32 option )
{
	CObject* context = AsCObject();
	if( context )
	{
		CallFunction( context, CNAME( RunSpecialOption ), option );
	}
}

void IEdSpawnTreeNode::GetContextMenuDebugOptions( CSpawnTreeInstance& instanceBuffer, TDynArray< String >& outOptions )
{

}

void IEdSpawnTreeNode::RunDebugOption( CSpawnTreeInstance& instanceBuffer, Int32 option )
{

}

void IEdSpawnTreeNode::OnEditorOpened()
{
	
}

ICreatureDefinitionContainer* IEdSpawnTreeNode::AsCreatureDefinitionContainer()
{
	return nullptr;
}

void IEdSpawnTreeNode::GetPossibleChildClasses( TArrayMap< String, CClass * >& result, ESpawnTreeType spawnTreeType, Bool sort ) const
{
	TDynArray< CClass* > rootClasses;
	rootClasses.Reserve( 1 );
	GetRootClassForChildren( rootClasses, spawnTreeType );
	for ( auto it = rootClasses.Begin(), end = rootClasses.End(); it != end; ++it )
	{
		CClass* rootChildClass = *it;
		TDynArray< CClass* > childClasses;
		SRTTI::GetInstance().EnumClasses( rootChildClass, childClasses, nullptr, false );
	
		for ( Uint32 i = 0; i < childClasses.Size(); ++i )
		{
			if ( CanSpawnChildClass( childClasses[i], spawnTreeType ) )
			{
				IEdSpawnTreeNode* defNode = dynamic_cast< IEdSpawnTreeNode* >( childClasses[i]->GetDefaultObject< CObject >() );
				ASSERT ( defNode );
				result.Insert( defNode->GetEditorFriendlyName(), childClasses[i] );
			}
		}
	}
}

Bool IEdSpawnTreeNode::IsPossibleChildClass( CClass* nodeClass, ESpawnTreeType spawnTreeType )
{
	TDynArray< CClass* > rootClasses;
	GetRootClassForChildren( rootClasses, spawnTreeType );

	for ( auto it = rootClasses.Begin(), end = rootClasses.End(); it != end; ++it )
	{
		if ( nodeClass->IsA( *it ) )
		{
			if ( CanSpawnChildClass( nodeClass, spawnTreeType ) )
			{
				return true;
			}
			break;
		}
	}

	return false;
}

void IEdSpawnTreeNode::GenerateId()
{
	CStandardRand& generator = GEngine->GetRandomNumberGenerator();
	m_id = generator.Get< Uint64 >( ULLONG_MAX );
}

Bool IEdSpawnTreeNode::GenerateIdsRecursively()
{
	Bool wasRegenerated = false;

	if ( m_id == 0 )
	{
		GenerateId();
		wasRegenerated = true;
	}

	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		if ( IEdSpawnTreeNode* const child = GetChild( i ) )
		{
			wasRegenerated |= child->GenerateIdsRecursively();
		}
	}
	return wasRegenerated;
}

void IEdSpawnTreeNode::GenerateHashRecursively( Uint64 parentHash, CSpawnTreeInstance* parentBuffer )
{
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		if ( IEdSpawnTreeNode* const child = GetChild( i ) )
		{
			child->GenerateHashRecursively( m_id ^ parentHash, GetInstanceBuffer( parentBuffer ) );
		}
	}
}