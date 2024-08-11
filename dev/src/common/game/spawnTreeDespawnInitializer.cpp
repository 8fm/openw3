#include "build.h"
#include "spawnTreeDespawnInitializer.h"
#include "spawnTreeDespawnerHandler.h"
#include "aiDespawnParameters.h"
#include "encounter.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeDespawnInitializer );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeAIDespawnInitializer );

////////////////////////////////////////////////////////////////////
// CSpawnTreeDespawnInitializer
////////////////////////////////////////////////////////////////////
Bool CSpawnTreeDespawnInitializer::CreateDespawners( CSpawnTreeDespawnerHandler& handler, CActor* actor, SpawnTreeDespawnerId id ) const
{
	handler.RegisterDespawner( Move( CSpawnTreeDespawnAction( m_instantDespawnConfiguration, actor, id ) ) );
	return true;
}

String CSpawnTreeDespawnInitializer::GetEditorFriendlyName() const
{
	static String STR( TXT("Configure despawn") );
	return STR;
}
Bool CSpawnTreeDespawnInitializer::IsConflicting( const ISpawnTreeInitializer* initializer ) const
{
	return initializer->IsA< CSpawnTreeDespawnInitializer >();
}
Bool CSpawnTreeDespawnInitializer::IsSpawnableOnPartyMembers() const
{
	return false;
}

////////////////////////////////////////////////////////////////////
// CSpawnTreeAIDespawnInitializer
////////////////////////////////////////////////////////////////////
Bool CSpawnTreeAIDespawnInitializer::CreateDespawners( CSpawnTreeDespawnerHandler& handler, CActor* actor, SpawnTreeDespawnerId id ) const
{
	Bool ret = TBaseClass::CreateDespawners( handler, actor, id );
	if ( m_ai )
	{
		handler.RegisterDespawner( Move( CSpawnTreeDespawnAction( m_aiDespawnConfiguration, m_ai, Int8( m_aiPriority ), actor, id ) ) );
	}
	return ret;
}

String CSpawnTreeAIDespawnInitializer::GetEditorFriendlyName() const
{
	static String STR( TXT("Configure despawn AI action") );
	return STR;
}

#ifndef NO_EDITOR
void CSpawnTreeAIDespawnInitializer::OnCreatedInEditor()
{
	TBaseClass::OnCreatedInEditor();

	m_ai = CAIDespawnTree::GetStaticClass()->CreateObject< CAIDespawnTree >();
	m_ai->OnCreatedInEditor();
}
#endif