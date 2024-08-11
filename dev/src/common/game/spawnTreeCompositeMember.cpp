#include "build.h"
#include "spawnTreeCompositeMember.h"

#include "spawnTreeBaseEntry.h"
#include "spawnTreeNodeListOperations.inl"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeEntryList );
IMPLEMENT_ENGINE_CLASS( ISpawnTreeCompositeNode );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeNode );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeParallelNode );


//////////////////////////////////////////////////////////////
// CSpawnTreeEntryList
//////////////////////////////////////////////////////////////

void CSpawnTreeEntryList::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
	for( Uint32 i = 0; i < m_entries.Size(); ++i )
	{
		ISpawnTreeBaseNode* entry = m_entries[i];
		if( entry->IsActive( instance ) )
		{
			entry->Deactivate( instance );
		}
	}
}

void CSpawnTreeEntryList::UpdateLogic( CSpawnTreeInstance& instance )
{
	TBaseClass::UpdateLogic( instance );

	if ( instance[ i_active ] )
	{
		for( Uint32 i = 0, n = m_entries.Size(); i < n; ++i )
		{
			ISpawnTreeBaseNode* entry = m_entries[i];
			entry->UpdateLogic( instance );
		}
	}
}

Bool CSpawnTreeEntryList::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName.AsString() == TXT("entries") )
	{
		m_entries = *static_cast< const TDynArray< ISpawnTreeBaseNode* > * >( readValue.GetData() );
		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Bool CSpawnTreeEntryList::IsSpawnableByDefault() const
{
	return false;
}
void CSpawnTreeEntryList::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	ListOperations::GetRootClassForChildren( rootClasses );
}
Bool CSpawnTreeEntryList::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	if ( classId->IsAbstract() )
	{
		return false;
	}

	ISpawnTreeBaseNode* defaultNodeObj = const_cast< CClass* >( classId )->GetDefaultObject< ISpawnTreeBaseNode >();
	if ( !defaultNodeObj )
	{
		return false;
	}

	if ( !defaultNodeObj->IsSpawnableByDefault() ) 
	{
		return false;
	}

	// if advanced then accept any kind of node as child for EntryList
	if ( spawnTreeType == STT_gameplay )
	{
		return true;
	}

	if ( defaultNodeObj->IsA< ISpawnTreeLeafNode >() )
	{
		return true;
	}

	if ( defaultNodeObj->IsUtilityNode() )
	{
		return true;
	}
	return false;
}
Bool CSpawnTreeEntryList::CanAddChild() const
{
	return true;
}
ISpawnTreeBaseNode* CSpawnTreeEntryList::GetChildMember( Uint32 i ) const
{
	return m_entries[ i ];
}
Uint32 CSpawnTreeEntryList::GetChildMembersCount() const
{
	return m_entries.Size();
}
void CSpawnTreeEntryList::AddChild( IEdSpawnTreeNode* child )
{
	ListOperations::AddChild( m_entries, child );
}
void CSpawnTreeEntryList::RemoveChild( IEdSpawnTreeNode* node )
{
	ListOperations::RemoveChild( m_entries, node );
}
Bool CSpawnTreeEntryList::UpdateChildrenOrder()
{
	Bool dirty = ListOperations::UpdateChildrenOrder( m_entries ) ;
	return TBaseClass::UpdateChildrenOrder() || dirty;
}
Bool CSpawnTreeEntryList::CanBeHidden() const
{
	return true;
}
Color CSpawnTreeEntryList::GetBlockColor() const
{
	return Color( 30, 192, 30 );
}
String CSpawnTreeEntryList::GetEditorFriendlyName() const
{
	static const String STR( TXT("EntryList") );
	return STR;
}


//////////////////////////////////////////////////////////////
// ISpawnTreeCompositeNode
//////////////////////////////////////////////////////////////
void ISpawnTreeCompositeNode::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	if ( spawnTreeType == STT_gameplay )
	{
		IEdSpawnTreeNode* parentNode = GetParentNode();
		if ( parentNode && parentNode->AsCObject()->IsA< CSpawnTreeEntryList >() )
		{
			rootClasses.PushBack( ISpawnTreeCompositeNode::GetStaticClass() );
		}
		else
		{
			rootClasses.PushBack( ISpawnTreeBranch::GetStaticClass() );
		}
		rootClasses.PushBack( ISpawnTreeLeafNode::GetStaticClass() );
	}
	else
	{
		rootClasses.PushBack( ISpawnTreeBranch::GetStaticClass() );
	}
}
Bool ISpawnTreeCompositeNode::CanAddChild() const
{
	return true;
}
ISpawnTreeBaseNode* ISpawnTreeCompositeNode::GetChildMember( Uint32 i ) const
{
	return m_childNodes[ i ];
}
Uint32 ISpawnTreeCompositeNode::GetChildMembersCount() const
{
	return m_childNodes.Size();
}
void ISpawnTreeCompositeNode::AddChild( IEdSpawnTreeNode* child )
{
	ListOperations::AddChild( m_childNodes, child );
}
void ISpawnTreeCompositeNode::RemoveChild( IEdSpawnTreeNode* node )
{
	ListOperations::RemoveChild( m_childNodes, node );
}
Bool ISpawnTreeCompositeNode::UpdateChildrenOrder()
{
	Bool dirty = ListOperations::UpdateChildrenOrder( m_childNodes );
	return TBaseClass::UpdateChildrenOrder() || dirty;
}

Color ISpawnTreeCompositeNode::GetBlockColor() const
{
	return Color( 150, 150, 30 );
}

String ISpawnTreeCompositeNode::GetEditorFriendlyName() const
{
	static const String STR( TXT("Composite") );
	return STR;
}


//////////////////////////////////////////////////////////////
// CSpawnTreeNode
//////////////////////////////////////////////////////////////

void CSpawnTreeNode::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}

void CSpawnTreeNode::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
	Uint16 currentActiveChild = instance[ i_currentActiveChild ];
	if ( currentActiveChild != INVALID_CHILD )
	{
		if ( m_childNodes[ currentActiveChild ]->IsActive( instance ) )
		{
			m_childNodes[ currentActiveChild ]->Deactivate( instance );
		}

		instance[ i_currentActiveChild ] = INVALID_CHILD;
	}
}

void CSpawnTreeNode::UpdateLogic( CSpawnTreeInstance& instance )
{
	TBaseClass::UpdateLogic( instance );

	if ( instance[ i_active ] )
	{
		Uint16 currentActiveChild = instance[ i_currentActiveChild ];

		for( Uint16 i = 0, n = Uint16( m_childNodes.Size() ); i < n; ++i )
		{
			m_childNodes[ i ]->UpdateLogic( instance );
			if ( m_childNodes[ i ]->IsActive( instance ) )
			{
				if ( i != currentActiveChild )
				{
					if ( currentActiveChild != INVALID_CHILD && i < currentActiveChild )
					{
						m_childNodes[ currentActiveChild ]->Deactivate( instance );
					}
					instance[ i_currentActiveChild ] = i;
				}
				return;
			}
		}
		currentActiveChild = INVALID_CHILD;
	}
}


String CSpawnTreeNode::GetEditorFriendlyName() const
{
	static const String STR( TXT("Selector") );
	return STR;
}

void CSpawnTreeNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currentActiveChild;
}
void CSpawnTreeNode::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	instance[ i_currentActiveChild ] = INVALID_CHILD;
}


//////////////////////////////////////////////////////////////
// CSpawnTreeParallelNode
//////////////////////////////////////////////////////////////
void CSpawnTreeParallelNode::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}
void CSpawnTreeParallelNode::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
	for( Uint16 i = 0, n = Uint16( m_childNodes.Size() ); i < n; ++i )
	{
		ISpawnTreeBaseNode* node = m_childNodes[ i ];
		if ( node->IsActive( instance ) )
		{
			node->Deactivate( instance );
		}
	}
}
void CSpawnTreeParallelNode::UpdateLogic( CSpawnTreeInstance& instance )
{
	TBaseClass::UpdateLogic( instance );

	if ( instance[ i_active ] )
	{
		for( Uint16 i = 0, n = Uint16( m_childNodes.Size() ); i < n; ++i )
		{
			m_childNodes[ i ]->UpdateLogic( instance );
		}
	}
}
String CSpawnTreeParallelNode::GetEditorFriendlyName() const
{
	static const String STR( TXT("Parallel") );
	return STR;
}