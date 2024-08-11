/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeDecoratorInitializersList.h"

#include "spawnTreeInitializationContext.h"
#include "spawnTreeInitializer.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeDecoratorInitializersList )


///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeDecoratorInitializersList::InitializersIterator
///////////////////////////////////////////////////////////////////////////////
Bool CSpawnTreeDecoratorInitializersList::InitializersIterator::Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer )
{
	if ( m_currIdx >= m_initializers.Size() )
	{
		return false;
	}
	// progress iterator
	outInitializer = m_initializers[ m_currIdx++ ];
	instanceBuffer = &m_instance;

	return true;
}
void CSpawnTreeDecoratorInitializersList::InitializersIterator::Reset()
{
	m_currIdx = 0;
}
///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeDecoratorInitializersList
///////////////////////////////////////////////////////////////////////////////
void CSpawnTreeDecoratorInitializersList::Deactivate( CSpawnTreeInstance& instance )
{
	for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
	{
		(*it)->OnSpawnTreeDeactivation( instance );
	}
	TBaseClass::Deactivate( instance );
}

void CSpawnTreeDecoratorInitializersList::CollectSpawnTags( TagList& tagList )
{
	TBaseClass::CollectSpawnTags( tagList );
	for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
	{
		(*it)->CollectSpawnTags( tagList );
	}
}
void CSpawnTreeDecoratorInitializersList::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
	{
		(*it)->OnFullRespawn( instance );
	}

	TBaseClass::OnFullRespawn( instance );
}
void CSpawnTreeDecoratorInitializersList::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
	{
		(*it)->OnBuildDataLayout( compiler );
	}

	TBaseClass::OnBuildDataLayout( compiler );
}

void CSpawnTreeDecoratorInitializersList::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
	{
		(*it)->OnInitData( instance );
	}

	CSpawnTreeInitializationContext::SPopData popData;
	context.PushTopInitializers( m_topInitializers, &instance, popData );

	TBaseClass::OnInitData( instance, context );

	context.PopTopInitializers( popData );
}

Bool CSpawnTreeDecoratorInitializersList::GenerateIdsRecursively()
{
	Bool result = false;
	for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
	{
		result |= (*it)->GenerateIdsRecursively();
	}

	result |= TBaseClass::GenerateIdsRecursively();
	return result;
}

Bool CSpawnTreeDecoratorInitializersList::CanAddChild() const
{
	return true;
}
void CSpawnTreeDecoratorInitializersList::AddChild( IEdSpawnTreeNode* node )
{
	CObject* obj = node->AsCObject();
	if ( obj->IsA< ISpawnTreeInitializer >() )
	{
		obj->SetParent( this );
		m_topInitializers.PushBack( static_cast< ISpawnTreeInitializer* >( obj ) );
	}
	else
	{
		TBaseClass::AddChild( node );
	}
}
void CSpawnTreeDecoratorInitializersList::RemoveChild( IEdSpawnTreeNode* node )
{
	if ( !node )
	{
		m_topInitializers.Remove( nullptr );
		return;
	}

	CObject* obj = node->AsCObject();

	if ( obj->IsA< ISpawnTreeInitializer >() )
	{
		m_topInitializers.Remove( static_cast< ISpawnTreeInitializer* >( obj ) );
	}
	else
	{
		TBaseClass::RemoveChild( node );
	}
}
Int32 CSpawnTreeDecoratorInitializersList::GetNumChildren() const
{
	return m_topInitializers.Size() + TBaseClass::GetNumChildren();
}
IEdSpawnTreeNode* CSpawnTreeDecoratorInitializersList::GetChild( Int32 index ) const
{
	if( index < Int32(m_topInitializers.Size()) )
	{
		return m_topInitializers[ index ];
	}
	else
	{
		return TBaseClass::GetChild( index - m_topInitializers.Size() );
	}
}
void CSpawnTreeDecoratorInitializersList::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeInitializer::GetStaticClass() );
	TBaseClass::GetRootClassForChildren( rootClasses, spawnTreeType );
}
Bool CSpawnTreeDecoratorInitializersList::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	if ( classId->IsA< ISpawnTreeInitializer >() )
	{
		const ISpawnTreeInitializer* defaultObj = const_cast< CClass* >( classId )->GetDefaultObject< ISpawnTreeInitializer >();
		if ( !defaultObj )
		{
			return false;
		}
		for ( auto it = m_topInitializers.Begin(), end = m_topInitializers.End(); it != end; ++it )
		{
			ISpawnTreeInitializer* initializer = *it;
			if ( initializer )
			{
				if ( initializer->IsConflicting( defaultObj ) )
				{
					return false;
				}
			}
		}

		return true;
	}
	return TBaseClass::CanSpawnChildClass( classId, spawnTreeType );
}
Bool CSpawnTreeDecoratorInitializersList::IsHiddenByDefault() const
{
	return false;
}
Bool CSpawnTreeDecoratorInitializersList::CanBeHidden() const
{
	return false;
}
Color CSpawnTreeDecoratorInitializersList::GetBlockColor() const
{
	return Color::LIGHT_BLUE;
}
String CSpawnTreeDecoratorInitializersList::GetEditorFriendlyName() const
{
	return TXT("TopInitializersList");
}
Bool CSpawnTreeDecoratorInitializersList::IsNodeStateSaving( CSpawnTreeInstance& instance ) const
{
	InitializersIterator it( *this, instance );

	return ISpawnTreeInitializer::AreInitializersStateSaving( it );
}
void CSpawnTreeDecoratorInitializersList::SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	InitializersIterator it( *this, instance );

	ISpawnTreeInitializer::SaveInitializersState( it, writer );
}
Bool CSpawnTreeDecoratorInitializersList::LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	InitializersIterator it( *this, instance );

	ISpawnTreeInitializer::LoadInitializersState( it, reader );

	return true;
}