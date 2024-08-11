#include "build.h"

#include "spawnTreeBaseEntryGenerator.h"
#include "actionPoint.h"
#include "actionPointSelectors.h"
#include "../../common/core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeBaseEntryGenerator )
IMPLEMENT_ENGINE_CLASS( SCreatureDefinitionWrapper )
IMPLEMENT_ENGINE_CLASS( SWorkCategoriesWrapper )
IMPLEMENT_ENGINE_CLASS( SWorkCategoryWrapper )
IMPLEMENT_ENGINE_CLASS( SCreatureEntryEntryGeneratorNodeParam )

Color CSpawnTreeBaseEntryGenerator::GetBlockColor() const
{
	return Color( 163, 73, 164 );
}

void CSpawnTreeBaseEntryGenerator::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = &SActionPointResourcesManager::GetInstance().Get2dArray();
	valueProperties.m_valueColumnName = TXT("Category");
}

Bool CSpawnTreeBaseEntryGenerator::IsSpawnableByDefault() const 
{ 
	return GetClass() != CSpawnTreeBaseEntryGenerator::GetStaticClass() && !GetClass()->IsAbstract();
	/*CSpawnTreeBaseEntryGenerator* notConstThis = const_cast< CSpawnTreeBaseEntryGenerator* >( this );
	Bool retVal = false;
	if( CallFunctionRet< Bool >( notConstThis, CNAME( IsSpawnable ), retVal ) )
	{
		return retVal;
	}
	return false; */
}

String CSpawnTreeBaseEntryGenerator::GetEditorFriendlyName() const
{
	CSpawnTreeBaseEntryGenerator* notConstThis = const_cast< CSpawnTreeBaseEntryGenerator* >( this );
	String retVal = String::EMPTY;
	if( CallFunctionRet< String >( notConstThis, CNAME( GetFriendlyName ), retVal ) )
	{
		return retVal;
	}
	return String::EMPTY; 
}

Bool CSpawnTreeBaseEntryGenerator::UpdateChildrenOrder()
{
	Bool dirty = ListOperations::UpdateChildrenOrder( m_childNodes );
	return TBaseClass::UpdateChildrenOrder() || dirty;
}

void CSpawnTreeBaseEntryGenerator::AddNodeToTree( IEdSpawnTreeNode* newNode, ISpawnTreeBaseNode* parentNode )
{
	static const Int32 GRAPH_POS_CHANGE = 40;

	if ( !parentNode )
	{
		parentNode = this;
	}

#ifndef NO_EDITOR
	CObject* obj = newNode->AsCObject();
	
	obj->OnCreatedInEditor();
#endif 
#ifndef NO_EDITOR_GRAPH_SUPPORT
	
	Int32 x = parentNode->GetGraphPosX() - GRAPH_POS_CHANGE;
	for ( Int32 i = 0; i < parentNode->GetNumChildren(); ++i )
	{
		x = Max( x, parentNode->GetChild( i )->GetGraphPosX() + GRAPH_POS_CHANGE );
	}

	newNode->SetGraphPosition( x + GRAPH_POS_CHANGE, parentNode->GetGraphPosY() + GRAPH_POS_CHANGE );
#endif

	parentNode->AddChild( newNode );
}

void CSpawnTreeBaseEntryGenerator::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}
void CSpawnTreeBaseEntryGenerator::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
	for( Uint16 i = 0, n = Uint16( m_childNodes.Size() ); i < n; ++i )
	{
		ISpawnTreeBaseNode* node = m_childNodes[ i ].Get();
		if ( node->IsActive( instance ) )
		{
			node->Deactivate( instance );
		}
	}
}
void CSpawnTreeBaseEntryGenerator::UpdateLogic( CSpawnTreeInstance& instance )
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

void CSpawnTreeBaseEntryGenerator::ReGenerateEntries()
{
	for( Int32 i = m_childNodes.SizeInt() - 1; i >= 0; --i )
	{
		ListOperations::RemoveChildHandle( m_childNodes, m_childNodes[ i ].Get() );
	}
	//m_childNodes.ClearFast();
	CallFunction( this, CNAME( GenerateEntries ) );
}

/*void CSpawnTreeBaseEntryGenerator::OnPreSave()
{
	ReGenerateEntries();
	TBaseClass::OnPreSave();
}*/

void CSpawnTreeBaseEntryGenerator::funcAddNodeToTree( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< ISpawnTreeBaseNode >, nodeToAdd, nullptr );
	GET_PARAMETER( THandle< ISpawnTreeBaseNode >, parent, nullptr );
	FINISH_PARAMETERS;

	if( !nodeToAdd.Get() )
		return;

	ISpawnTreeBaseNode* newNode = nodeToAdd.Get();

	AddNodeToTree( newNode, parent.Get() );
}

void CSpawnTreeBaseEntryGenerator::funcAddInitializerToNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< ISpawnTreeInitializer >, initializerToAdd, nullptr );
	GET_PARAMETER( THandle< ISpawnTreeBaseNode >, parent, nullptr );
	FINISH_PARAMETERS;

	if( !initializerToAdd.Get() )
		return;

	AddNodeToTree( initializerToAdd.Get(), parent.Get() );
}

void CSpawnTreeBaseEntryGenerator::funcRemoveChildren( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	for( Int32 i = m_childNodes.SizeInt() - 1; i >= 0; --i )
	{
		ListOperations::RemoveChildHandle( m_childNodes, m_childNodes[ i ].Get() );
	}
	//m_childNodes.ClearFast();
}

void CSpawnTreeBaseEntryGenerator::funcSetName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SEncounterActionPointSelectorPair, selectorPair, SEncounterActionPointSelectorPair() );
	GET_PARAMETER( CName, catName, CName::NONE );
	FINISH_PARAMETERS;

	selectorPair.m_name = catName;
}

CEntityTemplate* SCreatureEntryEntryGeneratorNodeParam::GetEntityTemplate()
{
	CName creatureDefinitionName = m_creatureDefinition.m_creatureDefinition;
	if( !creatureDefinitionName )
	{
		return nullptr;
	}

	IEdSpawnTreeNode* spawnTreeNode =  m_parent.Get();		

	while ( spawnTreeNode )
	{
		ICreatureDefinitionContainer* creatureDefContainer = spawnTreeNode->AsCreatureDefinitionContainer();
		if ( creatureDefContainer )
		{
			CEncounterCreatureDefinition* def = creatureDefContainer->GetCreatureDefinition( creatureDefinitionName );
			if ( def )
			{
				return def->GetEntityTemplate().Get();
			}
			break;
		}
		spawnTreeNode = spawnTreeNode->GetParentNode();
	}
	return nullptr;
}
