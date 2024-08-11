#include "build.h"
#include "spawnTreeIncludeNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/diskFile.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeIncludeTreeNode );

CSpawnTreeIncludeTreeNode::CSpawnTreeIncludeTreeNode()
	: m_spawnTree( NULL )
{
}

CSpawnTreeIncludeTreeNode::~CSpawnTreeIncludeTreeNode()
{

}
void CSpawnTreeIncludeTreeNode::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}
void CSpawnTreeIncludeTreeNode::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
	CSpawnTreeInstance* privateInstance = GetPrivateInstance( instance );
	if ( privateInstance && m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->Deactivate( *privateInstance );
	}
}
void CSpawnTreeIncludeTreeNode::UpdateLogic( CSpawnTreeInstance& instance )
{
	if ( !IsEnabled( instance ) )
	{
		return;
	}
	if ( !instance[ i_active ] )
	{
		Activate( instance );
	}
	CSpawnTreeInstance* privateInstance = GetPrivateInstance( instance );
	if ( privateInstance && m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->UpdateLogic( *privateInstance );
	}
}

Bool CSpawnTreeIncludeTreeNode::SetSpawnPhase( CSpawnTreeInstance& instance, CName phaseName )
{
	CSpawnTreeInstance* privateInstance = GetPrivateInstance( instance );
	if ( privateInstance && m_spawnTree && m_spawnTree->GetRootNode() )
	{
		return m_spawnTree->GetRootNode()->SetSpawnPhase( *privateInstance, phaseName );
	}
	return false;
}
void CSpawnTreeIncludeTreeNode::GetSpawnPhases( TDynArray< CName >& outPhaseNames )
{
	if ( m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->GetSpawnPhases( outPhaseNames );
	}
}
void CSpawnTreeIncludeTreeNode::EnableMember( CSpawnTreeInstance& instance, CName& name, Bool enable )
{
	if( m_nodeName == name )
	{
		SetEnabled( instance, enable );
	}
	CSpawnTreeInstance* privateInstance = GetPrivateInstance( instance );
	if ( privateInstance && m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->EnableMember( *privateInstance, name, enable );
	}
}
void CSpawnTreeIncludeTreeNode::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	CSpawnTreeInstance* privateInstance = GetPrivateInstance( instance );
	if ( privateInstance && m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->OnFullRespawn( *privateInstance );
	}
}
void CSpawnTreeIncludeTreeNode::CollectSpawnTags( TagList& tagList )
{
	if ( m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->CollectSpawnTags( tagList );
	}
}
void CSpawnTreeIncludeTreeNode::CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames )
{
	if ( m_spawnTree )
	{
		m_spawnTree->CollectUsedCreatureDefinitions( inOutNames );
	}
}
void CSpawnTreeIncludeTreeNode::FillUpDefaultCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& inOutCreatureDefs, CObject* parent ) 
{
	if ( m_spawnTree )
	{
		m_spawnTree->FillUpDefaultCreatureDefinitions( inOutCreatureDefs, parent );
		if ( m_spawnTree->GetRootNode() )
		{
			m_spawnTree->GetRootNode()->FillUpDefaultCreatureDefinitions( inOutCreatureDefs, parent );
		}
	}
}
void CSpawnTreeIncludeTreeNode::CompileCreatureDefinitions( CEncounterCreatureDefinition::CompiledCreatureList& creatureList )
{
	if ( m_spawnTree )
	{
		m_spawnTree->CompileCreatureDefinitions( creatureList );
	}
}
void CSpawnTreeIncludeTreeNode::UpdateEntriesSetup( CSpawnTreeInstance& instance ) const
{
	CSpawnTreeInstance* privateInstance = GetPrivateInstance( instance );
	if ( privateInstance && m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->GetRootNode()->UpdateEntriesSetup( *privateInstance );
	}
}
// Instance buffer interface
void CSpawnTreeIncludeTreeNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler		<<			i_spawnTreeInstanceData;
}
void CSpawnTreeIncludeTreeNode::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	CSpawnTreeInstance* privateInstance = NULL;
	if ( m_spawnTree && m_spawnTree->GetRootNode() )
	{
		m_spawnTree->RequestDataLayout();
		const InstanceDataLayout& layout = m_spawnTree->GetDataLayout();
		privateInstance = new CSpawnTreeInstance();
		privateInstance->Bind( layout, instance.GetEncounter() );
		m_spawnTree->GetRootNode()->OnInitData( *privateInstance, context );
	}

	instance[ i_spawnTreeInstanceData ] = reinterpret_cast< TGenericPtr >( privateInstance );
}
void CSpawnTreeIncludeTreeNode::OnDeinitData( CSpawnTreeInstance& instance )
{
	CSpawnTreeInstance* privateInstance = reinterpret_cast< CSpawnTreeInstance* >( instance[ i_spawnTreeInstanceData ] );
	if ( privateInstance )
	{
		if ( m_spawnTree && m_spawnTree->GetRootNode() )
		{
			m_spawnTree->GetRootNode()->OnDeinitData( *privateInstance );
		}

		delete privateInstance;
		instance[ i_spawnTreeInstanceData ] = NULL;
	}
}

// Editor interface
ISpawnTreeBaseNode* CSpawnTreeIncludeTreeNode::GetChildMember( Uint32 i ) const
{
	ASSERT( false );
	return NULL;
}
Uint32 CSpawnTreeIncludeTreeNode::GetChildMembersCount() const
{
	return 0;
}

ISpawnTreeBaseNode* CSpawnTreeIncludeTreeNode::GetTransientChildMember( Uint32 i ) const
{
	return m_spawnTree->GetRootNode();
}
Uint32 CSpawnTreeIncludeTreeNode::GetTransientChildMembersCount() const
{
	return ( m_spawnTree && m_spawnTree->GetRootNode() ) ? 1 : 0;
}

Bool CSpawnTreeIncludeTreeNode::IsUtilityNode() const
{
	return true;
}

// IEdSpawnTreeNode interface
Bool CSpawnTreeIncludeTreeNode::CanAddChild() const
{
	return false;
}
void CSpawnTreeIncludeTreeNode::RemoveChild( IEdSpawnTreeNode* node )
{
	ASSERT( false, TXT("Child node should be locked!") );
}
Int32 CSpawnTreeIncludeTreeNode::GetNumChildren() const
{
	return ( m_spawnTree && m_spawnTree->GetRootNode() ) ? 1 : 0;
}
IEdSpawnTreeNode* CSpawnTreeIncludeTreeNode::GetChild( Int32 index ) const
{
	return m_spawnTree->GetRootNode();
}
Bool CSpawnTreeIncludeTreeNode::IsHiddenByDefault() const
{
	return true;
}
Bool CSpawnTreeIncludeTreeNode::CanBeHidden() const
{
	return true;
}
Color CSpawnTreeIncludeTreeNode::GetBlockColor() const
{
	return Color::LIGHT_RED;
}
String CSpawnTreeIncludeTreeNode::GetBlockCaption() const
{
	if( !m_spawnTree )
	{
		static const String STR( TXT( "Include EMPTY" ) );
		return STR;
	}
	CDiskFile* file = m_spawnTree->GetFile();
	if ( !file )
	{
		static const String STR( TXT( "Include INVALID" ) );
		return STR;
	}
	return String::Printf( TXT( "Include %s" ), file->GetFileName().AsChar() );
}
String CSpawnTreeIncludeTreeNode::GetEditorFriendlyName() const
{
	static const String STR( TXT( "Include File" ) );
	return STR;
}

Bool CSpawnTreeIncludeTreeNode::HoldsInstanceBuffer() const
{
	return true;
}
CSpawnTreeInstance* CSpawnTreeIncludeTreeNode::GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer )
{
	if ( parentBuffer )
	{
		return GetPrivateInstance( *parentBuffer );
	}
	return NULL;
}

void CSpawnTreeIncludeTreeNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	{
		static const CName SPAWNTREE( TXT( "spawnTree" ) );
		if ( property->GetName() == SPAWNTREE )
		{
			if ( m_spawnTree  )
			{
				Bool acceptRootNode = false;

				if ( m_spawnTree->GetRootNode() )
				{
					ISpawnTreeBaseNode* childNode = m_spawnTree->GetRootNode();
					CClass* childClass = childNode->GetClass();
					IEdSpawnTreeNode* parent = GetParentNode();
					if ( parent && childClass )
					{
						if ( parent->IsPossibleChildClass( childClass, m_spawnTree->GetSpawnTreeType() ) )
						{
							acceptRootNode = true;

							// ok now check if we don't include ourselves
							//IEdSpawnTreeNode* myTopParent = this;
							//for ( IEdSpawnTreeNode* myParent = this; (myParent = myParent->GetParentNode()) != NULL; myTopParent = myParent )
							//{
							//	myTopParent = myParent;
							//}

							//if ( myTopParent != m_spawnTree )
							//{
							//	
							//}
						}
					}
				}

				if ( !acceptRootNode )
				{
					m_spawnTree = NULL;
				}
			}
		}
	}
}

