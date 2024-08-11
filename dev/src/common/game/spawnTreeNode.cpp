#include "build.h"
#include "spawnTreeNode.h"

#include "encounter.h"

#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( ISpawnTreeBaseNode );
IMPLEMENT_ENGINE_CLASS( ISpawnTreeBranch );
IMPLEMENT_ENGINE_CLASS( ISpawnTreeDecorator );


//////////////////////////////////////////////////////////////
// ISpawnTreeBaseNode
//////////////////////////////////////////////////////////////
void ISpawnTreeBaseNode::SetEnabled( CSpawnTreeInstance& instance, Bool enabled )
{
	instance[ i_enabled ] = enabled;
	if ( !enabled && instance[ i_active ] )
	{
		Deactivate( instance );
	}
}
void ISpawnTreeBaseNode::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}

void ISpawnTreeBaseNode::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;

	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->Deactivate( instance );
	}
}

Bool ISpawnTreeBaseNode::TestConditions( CSpawnTreeInstance& instance ) const
{
	if ( !instance[ i_enabled ] )
	{
		return false;
	}
	return true;
}

void ISpawnTreeBaseNode::UpdateLogic( CSpawnTreeInstance& instance )
{
	Bool conditionsMet = TestConditions( instance );
	Bool isActive = IsActive( instance );
	if ( isActive != conditionsMet )
	{
		if ( conditionsMet )
		{
			Activate( instance );
		}
		else
		{
			Deactivate( instance );
		}
	}
}

Bool ISpawnTreeBaseNode::SetSpawnPhase( CSpawnTreeInstance& instance, CName phaseName )
{
	Bool b = false;
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		b = GetChildMember( i )->SetSpawnPhase( instance, phaseName ) || b;
	}
	return b;
}
void ISpawnTreeBaseNode::GetSpawnPhases( TDynArray< CName >& outPhaseNames )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->GetSpawnPhases( outPhaseNames );
	}
}
void ISpawnTreeBaseNode::EnableMember( CSpawnTreeInstance& instance, CName& name, Bool enable )
{
	if( m_nodeName == name )
	{
		SetEnabled( instance, enable );
	}
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->EnableMember( instance, name, enable );
	}
}
void ISpawnTreeBaseNode::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->OnFullRespawn( instance );
	}
}

void ISpawnTreeBaseNode::CollectSpawnTags( TagList& tagList )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->CollectSpawnTags( tagList );
	}
}

void ISpawnTreeBaseNode::CollectUsedCreatureDefinitions( TSortedArray< CName >& inOutNames )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->CollectUsedCreatureDefinitions( inOutNames );
	}
}

void ISpawnTreeBaseNode::FillUpDefaultCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& inOutCreatureDefs, CObject* parent )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->FillUpDefaultCreatureDefinitions( inOutCreatureDefs, parent );
	}
}

void ISpawnTreeBaseNode::CompileCreatureDefinitions( CEncounterCreatureDefinition::CompiledCreatureList& creatureList )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->CompileCreatureDefinitions( creatureList );
	}
}

void ISpawnTreeBaseNode::UpdateEntriesSetup( CSpawnTreeInstance& instance ) const
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->UpdateEntriesSetup( instance );
	}
}

Bool ISpawnTreeBaseNode::IsNodeStateSaving( CSpawnTreeInstance& instance ) const
{
	return false;
}
void ISpawnTreeBaseNode::SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{

}
Bool ISpawnTreeBaseNode::LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	return true;
}

void ISpawnTreeBaseNode::OnEvent( CSpawnTreeInstance& instance, CName eventName ) const
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->OnEvent( instance, eventName );
	}
}

void ISpawnTreeBaseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->OnBuildDataLayout( compiler );
	}

	compiler << i_enabled;
	compiler << i_active;
}

void ISpawnTreeBaseNode::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->OnInitData( instance, context );
	}

	instance[ i_enabled ] = true;
	instance[ i_active ] = false;
}

void ISpawnTreeBaseNode::OnDeinitData( CSpawnTreeInstance& instance )
{
	for ( Uint32 i = 0, n = GetChildMembersCount(); i !=n ; ++i )
	{
		GetChildMember( i )->OnDeinitData( instance );
	}
}

Bool ISpawnTreeBaseNode::CanAddChild() const
{
	return false;
}

void ISpawnTreeBaseNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	RemoveNullChildren();
}
Bool ISpawnTreeBaseNode::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName.AsString() == TXT( "name" ) )
	{
		CName nodeName;
		if ( readValue.AsType< CName >( nodeName ) )
		{
			m_nodeName = nodeName;
			return true;
		}
	}
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

ISpawnTreeBaseNode* ISpawnTreeBaseNode::GetChildMember( Uint32 i ) const
{
	ASSERT( false );
	return NULL;
}
Uint32 ISpawnTreeBaseNode::GetChildMembersCount() const
{
	return 0;
}
ISpawnTreeBaseNode* ISpawnTreeBaseNode::GetTransientChildMember( Uint32 i ) const
{
	return GetChildMember( i );
}
Uint32 ISpawnTreeBaseNode::GetTransientChildMembersCount() const
{
	return GetChildMembersCount();
}

Bool ISpawnTreeBaseNode::IsSpawnableByDefault() const
{
	return true;
}
Bool ISpawnTreeBaseNode::IsUtilityNode() const
{
	return false;
}
CObject* ISpawnTreeBaseNode::AsCObject()
{
	return this;
}
IEdSpawnTreeNode* ISpawnTreeBaseNode::GetParentNode() const
{
	CObject* parent = GetParent();
	if ( !parent )
	{
		return NULL;
	}
	if ( parent->IsA< ISpawnTreeBaseNode >() )
	{
		return static_cast< IEdSpawnTreeNode* >( static_cast < ISpawnTreeBaseNode* >( parent ) );
	}
	else if ( parent->IsA< CEncounter >() )
	{
		return static_cast< IEdSpawnTreeNode* >( static_cast < CEncounter* >( parent ) );
	}
	else if ( parent->IsA< CSpawnTree >() )
	{
		return static_cast< IEdSpawnTreeNode* >( static_cast < CSpawnTree* >( parent ) );
	}
	return NULL;
}
void ISpawnTreeBaseNode::AddChild( IEdSpawnTreeNode* node )
{
	ASSERT( false );
	ASSUME( false );
}
void ISpawnTreeBaseNode::RemoveChild( IEdSpawnTreeNode* node )
{
	ASSERT( false );
	ASSUME( false );
}
Int32 ISpawnTreeBaseNode::GetNumChildren() const
{
	return GetChildMembersCount();
}
IEdSpawnTreeNode* ISpawnTreeBaseNode::GetChild( Int32 index ) const
{
	return GetChildMember( index );
}
void ISpawnTreeBaseNode::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{

}
Bool ISpawnTreeBaseNode::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
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

	return true;
}
Bool ISpawnTreeBaseNode::IsHiddenByDefault() const
{
	return false;
}
Bool ISpawnTreeBaseNode::CanBeHidden() const
{
	return false;
}
Color ISpawnTreeBaseNode::GetBlockColor() const
{
	return Color::GREEN;
}
String ISpawnTreeBaseNode::GetEditorFriendlyName() const
{
	static const String STR( TXT("Base") );
	return STR; 
}
ISpawnTreeBaseNode::EDebugState ISpawnTreeBaseNode::GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const
{
	if ( instanceBuffer )
	{
		return (*instanceBuffer)[ i_active ] ? EDEBUG_ACTIVE : EDEBUG_DEACTIVATED;
	}
	return EDEBUG_INVALID;
}

//////////////////////////////////////////////////////////////
// ISpawnTreeBranch
//////////////////////////////////////////////////////////////
String ISpawnTreeBranch::GetEditorFriendlyName() const
{
	static const String STR( TXT("Branch") );
	return STR;
}

//////////////////////////////////////////////////////////////
// ISpawnTreeDecorator
//////////////////////////////////////////////////////////////
void ISpawnTreeDecorator::UpdateLogic( CSpawnTreeInstance& instance )
{
	TBaseClass::UpdateLogic( instance );
	if ( IsActive( instance ) && m_childNode )
	{
		m_childNode->UpdateLogic( instance );
	}
}
void ISpawnTreeDecorator::Deactivate( CSpawnTreeInstance& instance )
{
	if ( m_childNode && m_childNode->IsActive( instance ) )
	{
		m_childNode->Deactivate( instance );
	}
	TBaseClass::Deactivate( instance );
}

void ISpawnTreeDecorator::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeBaseNode::GetStaticClass() );
}
Bool ISpawnTreeDecorator::CanAddChild() const
{
	return m_childNode == NULL;
}
ISpawnTreeBaseNode* ISpawnTreeDecorator::GetChildMember( Uint32 i ) const
{
	return m_childNode;
}
Uint32 ISpawnTreeDecorator::GetChildMembersCount() const
{
	return m_childNode ? 1 : 0;
}
void ISpawnTreeDecorator::AddChild( IEdSpawnTreeNode* node ) 
{
	m_childNode = static_cast< ISpawnTreeBaseNode* >( node );
	m_childNode->SetParent( this );
}
void ISpawnTreeDecorator::RemoveChild( IEdSpawnTreeNode* node )
{
	m_childNode = NULL;
}
Color ISpawnTreeDecorator::GetBlockColor() const
{
	return Color::LIGHT_GREEN;
}
String ISpawnTreeDecorator::GetEditorFriendlyName() const
{
	static const String STR( TXT("Decorator") );
	return STR;
}