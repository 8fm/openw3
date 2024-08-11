#include "build.h"
#include "spawnTree.h"

#include "spawnTreeNode.h"
#include "encounter.h"

#include "../core/factory.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTree );

//////////////////////////////////////////////////////////////////////////
// CSpawnTreeFactory
//////////////////////////////////////////////////////////////////////////
class CSpawnTreeFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CSpawnTreeFactory, IFactory, 0 );

public:
	CSpawnTreeFactory()
	{
		m_resourceClass = ClassID< CSpawnTree >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		return ::CreateObject< CSpawnTree >( options.m_parentObject );		
	}
};

BEGIN_CLASS_RTTI( CSpawnTreeFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CSpawnTreeFactory );


//////////////////////////////////////////////////////////////////////////
// CSpawnTree
//////////////////////////////////////////////////////////////////////////
void CSpawnTree::ComputeLayout()
{
	if ( !m_dataLayoutComputed )
	{
		InstanceDataLayout layout;
		InstanceDataLayoutCompiler compiler( m_dataLayout );

		if ( m_rootNode )
		{
			m_rootNode->OnBuildDataLayout( compiler );
		}

		m_dataLayout.ChangeLayout( compiler );

		m_dataLayoutComputed = true;
	}
}
void CSpawnTree::ClearLayout()
{
	m_dataLayoutComputed = false;
}
void CSpawnTree::FillUpDefaultCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& inOutCreatureDefs, CObject* parent )
{
	for ( Uint32 i = 0, n = m_creatureDefinition.Size(); i != n; ++i )
	{
		CEncounterCreatureDefinition* def = m_creatureDefinition[ i ];
		if ( !def || def->m_definitionName.Empty() )
		{
			continue;
		}

		Bool isRedefined = false;

		for ( Uint32 j = 0, m = inOutCreatureDefs.Size(); j != m; ++j )
		{
			CEncounterCreatureDefinition* checkDef = inOutCreatureDefs[ j ];

			if ( checkDef && checkDef->m_definitionName == def->m_definitionName )
			{
				isRedefined = true;
				break;
			}
		}

		if ( !isRedefined )
		{
			CObject* obj = def->Clone( parent );
			CEncounterCreatureDefinition* newDef = static_cast< CEncounterCreatureDefinition* >( obj );
			newDef->SetOverride( false );
			inOutCreatureDefs.PushBack( newDef );
		}
	}
}

Bool CSpawnTree::GenerateIdsRecursively()
{
	Bool wasRegenerated = false;

	if ( m_rootNode )
	{
		wasRegenerated |= m_rootNode->GenerateIdsRecursively();
	}
	
	return wasRegenerated;
}

void CSpawnTree::CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames )
{
	for ( Uint32 i = 0, n = m_creatureDefinition.Size(); i != n; ++i )
	{
		CEncounterCreatureDefinition* def = m_creatureDefinition[ i ];
		if ( !def || def->m_definitionName.Empty() )
		{
			continue;
		}
		CName defName = def->m_definitionName;
		auto itFind = inOutNames.Find( defName );
		if ( itFind != inOutNames.End() )
		{
			continue;
		}
		inOutNames.Insert( defName );
	}
	if ( m_rootNode )
	{
		m_rootNode->CollectUsedCreatureDefinitions( inOutNames );
	}
}

CObject* CSpawnTree::AsCObject()
{
	return this;
}
IEdSpawnTreeNode* CSpawnTree::GetParentNode() const
{
	return NULL;
}
Bool CSpawnTree::CanBeCopied() const
{
	return false;
}
Bool CSpawnTree::CanAddChild() const
{
	return m_rootNode == NULL;
}
void CSpawnTree::AddChild( IEdSpawnTreeNode* node )
{
	m_rootNode = static_cast< ISpawnTreeBaseNode* >( node );
	m_rootNode->SetParent( this );
}
void CSpawnTree::RemoveChild( IEdSpawnTreeNode* node )
{
	if ( m_rootNode == node )
	{
		m_rootNode = NULL;
	}
}
Int32 CSpawnTree::GetNumChildren() const
{
	return m_rootNode ? 1 : 0;
}
IEdSpawnTreeNode* CSpawnTree::GetChild( Int32 index ) const
{
	return m_rootNode;
}
void CSpawnTree::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeBaseNode::GetStaticClass() );
}
Bool CSpawnTree::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	return true;
}
void CSpawnTree::PreStructureModification()
{
	ClearLayout();
}
void CSpawnTree::GetContextMenuSpecialOptions( SpecialOptionsList& outOptions )
{
	static const String STR_FILL_DEFINITIONS( TXT("Fill up creature definitions") );
	static const String STR_CLEAR_DEFINITIONS( TXT("Clear creature definitions") );
	outOptions.PushBack( STR_FILL_DEFINITIONS );
	outOptions.PushBack( STR_CLEAR_DEFINITIONS );
}
void CSpawnTree::RunSpecialOption( Int32 option )
{
	switch( option )
	{
	case 0:
		FillUpCreatureDefinition();

		break;
	case 1:
		ClearCreatureDefinitions();

		break;
	default:
		break;
	}
}
ICreatureDefinitionContainer* CSpawnTree::AsCreatureDefinitionContainer()
{
	return this;
}

CEncounterCreatureDefinition* CSpawnTree::AddCreatureDefinition()
{
	m_creatureDefinition.PushBack( CreateObject< CEncounterCreatureDefinition >( this ) );
	return m_creatureDefinition.Back();
}
void CSpawnTree::RemoveCreatureDefinition( CEncounterCreatureDefinition* def )
{
	ptrdiff_t idx = m_creatureDefinition.GetIndex( def );
	if ( idx != -1 )
	{
		m_creatureDefinition.Erase( m_creatureDefinition.Begin() + (Int32)idx );
	}
}
CEncounterCreatureDefinition* CSpawnTree::GetCreatureDefinition( CName name )
{
	for( Uint32 i = 0, n = m_creatureDefinition.Size(); i < n; ++i )
	{
		if ( m_creatureDefinition[ i ] && m_creatureDefinition[ i ]->m_definitionName == name )
		{
			return m_creatureDefinition[ i ];
		}
	}
	return NULL;
}
ISpawnTreeBaseNode* CSpawnTree::InternalGetRootTreeNode() const
{
	return m_rootNode;
}
TDynArray< CEncounterCreatureDefinition* >& CSpawnTree::InternalGetCreatureDefinitions()
{
	return m_creatureDefinition;
}
//////////////////////////////////////////////////////////////////////////
// CSpawnTreeInstance
//////////////////////////////////////////////////////////////////////////
CSpawnTreeInstance::CSpawnTreeInstance()
	: m_instanceBuffer( NULL )
	, m_encounter( NULL )
{

}
CSpawnTreeInstance::~CSpawnTreeInstance()
{
	Unbind();
}

void CSpawnTreeInstance::Bind( const InstanceDataLayout& dataLayout, CEncounter* encounter )
{
	Unbind();
	m_instanceBuffer = dataLayout.CreateBuffer( encounter, TXT("SpawnTree") );
	m_encounter = encounter;
}
void CSpawnTreeInstance::Unbind()
{
	if ( m_instanceBuffer )
	{
		m_instanceBuffer->Release();
		m_instanceBuffer = NULL;
	}
}

