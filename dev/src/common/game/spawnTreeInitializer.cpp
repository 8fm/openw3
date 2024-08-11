#include "build.h"
#include "spawnTreeInitializer.h"

#include "behTreeVarsUtils.h"
#include "encounter.h"
#include "spawnTreeBaseEntry.h"
#include "spawnTreeDecoratorInitializersList.h"
#include "spawnTreeEntry.h"
#include "spawnTreeMultiEntry.h"


IMPLEMENT_ENGINE_CLASS( SCompiledInitializer );

IMPLEMENT_ENGINE_CLASS( ISpawnTreeInitializer );

IMPLEMENT_ENGINE_CLASS( ISpawnTreeScriptedInitializer );


//			       '
//          .      '      .
//    .      .     :     .      .
//     '.        ______       .'
//       '  _.-"`      `"-._ '
//        .'                '.
// `'--. /                    \ .--'`
//      /                      \
//     ;                        ;
//- -- |                        | -- -
//     |     _.                 |
//     ;    /__`A   ,_          ;
// .-'  \   |= |;._.}{__       /  '-.
//    _.-""-|.' # '. `  `.-"{}<._
//          / 1938  \     \  x   `"
//     ----/         \_.-'|--X----
//     -=_ |         |    |- X.  =_
//    - __ |_________|_.-'|_X-X##
//        `'-._|_|;:;_.-'` '::.  `"-
//     .:;.      .:.   ::.     '::.

////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializer
////////////////////////////////////////////////////////////////////
Bool ISpawnTreeInitializer::Accept( CActor* actor ) const
{
	return true;
}
ISpawnTreeInitializer::EOutput ISpawnTreeInitializer::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	return OUTPUT_SUCCESS;
}
void ISpawnTreeInitializer::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{

}
void ISpawnTreeInitializer::Tick( CEncounterCreaturePool::SCreatureList& creatures, CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) const
{

}
void ISpawnTreeInitializer::OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
}
Bool ISpawnTreeInitializer::CreateDespawners( CSpawnTreeDespawnerHandler& handler, CActor* actor, SpawnTreeDespawnerId id ) const
{
	return false;
}
void ISpawnTreeInitializer::OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const
{

}

void ISpawnTreeInitializer::OnCreatureRemoval( CSpawnTreeInstance& instance, CActor* actor, ESpawnTreeCreatureRemovalReason removalReason, CBaseCreatureEntry* entry ) const
{
	if ( removalReason == SPAWN_TREE_REMOVAL_POOL )
	{
		Deactivate( actor, &instance, entry );
	}
}
void ISpawnTreeInitializer::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	// Do nothing
}

void ISpawnTreeInitializer::OnEvent( const CBaseCreatureEntry* const entry, CSpawnTreeInstance& instance, CName eventName, CSpawnTreeInstance* entryBuffer ) const
{

}

void ISpawnTreeInitializer::UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const
{

}

Bool ISpawnTreeInitializer::CallActivateOnRestore() const
{
	return true;
}
Bool ISpawnTreeInitializer::CallActivateOnSpawn() const
{
	return true;
}
Bool ISpawnTreeInitializer::CallActivateOnPoolSpawn() const
{
	return true;
}
Bool ISpawnTreeInitializer::CallActivateWhenStealing() const
{
	return true;
}

Bool ISpawnTreeInitializer::IsTickable() const
{
	return false;
}

Bool ISpawnTreeInitializer::IsSpawner() const
{
	return false;
}

Bool ISpawnTreeInitializer::HasSubInitializer() const
{
	return false;
}
ISpawnTreeInitializer* ISpawnTreeInitializer::GetSubInitializer() const
{
	ASSERT( false );
	return nullptr;
}

Bool ISpawnTreeInitializer::IsSpawnable() const
{
	return true;
}
 Bool ISpawnTreeInitializer::IsConflicting( const ISpawnTreeInitializer* initializer ) const
 {
	 return initializer->GetClass() == GetClass();
 }
 Bool ISpawnTreeInitializer::IsSpawnableOnPartyMembers() const
 {
	 return true;
 }
 void ISpawnTreeInitializer::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
 {

 }
 void ISpawnTreeInitializer::OnInitData( CSpawnTreeInstance& instance )
 {

 }
 void ISpawnTreeInitializer::OnDeinitData( CSpawnTreeInstance& instance )
 {

 }
 EFindSpawnResult ISpawnTreeInitializer::FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP ) const
 {
	 return FSR_NoneDefined;
 }

 EFindSpawnResult ISpawnTreeInitializer::FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const
 {
	 return FSR_NoneDefined;
 }

 void ISpawnTreeInitializer::CollectSpawnTags( TagList& tagList ) const
 {

 }

CObject* ISpawnTreeInitializer::AsCObject()
{
	return this;
}
IEdSpawnTreeNode* ISpawnTreeInitializer::GetParentNode() const
{
	CObject* parent = GetParent();
	if ( parent->IsA< CBaseCreatureEntry >() )
	{
		return static_cast< CBaseCreatureEntry* >( parent );
	}
	else if ( parent->IsA< ISpawnTreeInitializer >() )
	{
		return static_cast< ISpawnTreeInitializer* >( parent );
	}
	else if ( parent->IsA< CSpawnTreeEntrySubDefinition >() )
	{
		return static_cast< CSpawnTreeEntrySubDefinition* >( parent );
	}
	else if ( parent->IsA< CSpawnTreeDecoratorInitializersList >() )
	{
		return static_cast< CSpawnTreeDecoratorInitializersList* >( parent );
	}
	return NULL;
}
Bool ISpawnTreeInitializer::CanAddChild() const
{
	return false;
}
Color ISpawnTreeInitializer::GetBlockColor() const
{
	return Color( 40, 130, 255 );
}
String ISpawnTreeInitializer::GetEditorFriendlyName() const
{
	static String STR( TXT("Initializer") );
	return STR;
}
Bool ISpawnTreeInitializer::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( BehTreeVarsUtils::ConvertPointerTypes( this, existingProperty, readValue.GetRTTIType(), readValue.GetData() ) )
	{
		return true;
	}
	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Bool ISpawnTreeInitializer::AreInitializersStateSaving( ISpawnTreeInitializersIterator& iterator )
{
	const ISpawnTreeInitializer* initializer;
	CSpawnTreeInstance* buffer;

	while ( iterator.Next( initializer, buffer ) )
	{
		if ( initializer->IsStateSaving( *buffer ) )
		{
			return true;
		}
	}

	return false;
}
Bool ISpawnTreeInitializer::SaveInitializersState( ISpawnTreeInitializersIterator& iterator, IGameSaver* writer )
{
	Uint16 numNodesToSerialize = 0;

	const ISpawnTreeInitializer* initializer;
	CSpawnTreeInstance* buffer;

	while ( iterator.Next( initializer, buffer ) )
	{
		if ( initializer->IsStateSaving( *buffer ) )
		{
			++numNodesToSerialize;
		}
	}

	writer->WriteValue< Uint16 >( CNAME( numSaved ), numNodesToSerialize );

	if ( numNodesToSerialize == 0 )
	{
		return false;
	}

	iterator.Reset();

	Uint16 nodeIdx = 0;
	while ( iterator.Next( initializer, buffer ) )
	{
		if ( initializer->IsStateSaving( *buffer ) )
		{
			writer->WriteValue< Uint16 >( CNAME( i ), nodeIdx );
			initializer->SaveState( *buffer, writer );
		}
		++nodeIdx;
	}

	return true;
}
Bool ISpawnTreeInitializer::LoadInitializersState( ISpawnTreeInitializersIterator& iterator, IGameLoader* reader )
{
	Uint16 numNodesToSerialize = 0;
	reader->ReadValue( CNAME( numSaved ), numNodesToSerialize );

	if ( numNodesToSerialize == 0 )
	{
		return false;
	}

	const ISpawnTreeInitializer* initializer;
	CSpawnTreeInstance* buffer;

	Uint16 nodeIdx = 0;
	Uint16 nextIdx = 0xffff;
	reader->ReadValue< Uint16 >( CNAME( i ), nextIdx );
	while ( iterator.Next( initializer, buffer ) )
	{
		if ( nodeIdx == nextIdx )
		{
			initializer->LoadState( *buffer, reader );

			if ( --numNodesToSerialize == 0 )
			{
				return true;
			}
			reader->ReadValue< Uint16 >( CNAME( i ), nextIdx );
		}
		++nodeIdx;
	}
	return true;
}


Bool ISpawnTreeInitializer::IsStateSaving( CSpawnTreeInstance& instance ) const
{
	return false;
}
void ISpawnTreeInitializer::SaveState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{

}
void ISpawnTreeInitializer::LoadState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{

}

CEntityTemplate* ISpawnTreeInitializer::GetCreatureEntityTemplate() const
{
	IEdSpawnTreeNode* spawnTreeNode = NULL;
	CObject* obj = GetParent();
	CName creatureDefinitionName;
	while ( obj )
	{
		if ( obj->IsA< CBaseCreatureEntry >() )
		{
			if ( obj->IsA< CCreatureEntry >() )
			{
				CCreatureEntry* entry = static_cast< CCreatureEntry* >( obj );
				creatureDefinitionName = entry->GetCreatureDefinitionName();
				spawnTreeNode = entry;
			}
			break;
		}
		else if ( obj->IsA< CSpawnTreeEntrySubDefinition >() )
		{
			CSpawnTreeEntrySubDefinition* subDef = static_cast< CSpawnTreeEntrySubDefinition* >( obj );
			creatureDefinitionName = subDef->GetCreatureDefinitionName();
			spawnTreeNode = subDef;
			break;
		}
		obj = obj->GetParent();
	}

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
	return NULL;
}

////////////////////////////////////////////////////////////////////
// ISpawnTreeScriptedInitializer
////////////////////////////////////////////////////////////////////
Bool ISpawnTreeScriptedInitializer::ScriptActivate( CActor* actor ) const
{
	CObject* context = static_cast< CObject* >( const_cast< ISpawnTreeScriptedInitializer* >( this ) );
	THandle< CActor > handle( actor );
	return CallFunction( context, CNAME( Init ), handle );
}
Bool ISpawnTreeScriptedInitializer::ScriptDeactivate( CActor* actor ) const
{
	CObject* context = static_cast< CObject* >( const_cast< ISpawnTreeScriptedInitializer* >( this ) );
	THandle< CActor > handle( actor );
	return CallFunction( context, CNAME( DeInit ), handle );
}

ISpawnTreeInitializer::EOutput ISpawnTreeScriptedInitializer::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	ScriptActivate( actor );
	
	return OUTPUT_SUCCESS;
}

void ISpawnTreeScriptedInitializer::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	ScriptDeactivate( actor );
}


Bool ISpawnTreeScriptedInitializer::IsSpawnable() const
{
	return GetClass() != ISpawnTreeScriptedInitializer::GetStaticClass();
}

String ISpawnTreeScriptedInitializer::GetEditorFriendlyName() const
{
	CObject* context = const_cast< ISpawnTreeScriptedInitializer* >( this );
	String friendlyName;
	CallFunctionRet< String >( context, CNAME( GetEditorFriendlyName ), friendlyName );
	return friendlyName;
}
