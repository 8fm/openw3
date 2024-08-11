#include "build.h"
#include "spawnTreeDespawnerHandler.h"

void CSpawnTreeDespawnerHandler::RegisterDespawner( CSpawnTreeDespawnAction&& despawner )
{
	m_despawners.PushBack( Move( despawner ) );
}

SpawnTreeDespawnerId CSpawnTreeDespawnerHandler::GetNextDespawnId( )
{
	return m_nextDespawnId++;
}

Bool CSpawnTreeDespawnerHandler::RemoveDespawner( SpawnTreeDespawnerId id )
{
	Bool removed = false;
	for ( Uint32 i = 0; i < m_despawners.Size(); )
	{
		if ( m_despawners[ i ].GetDespawnerId() == id )
		{
			m_despawners.RemoveAtFast( i );
			removed = true;
		}
		else
		{
			++i;
		}
	}

	return removed;
}

void CSpawnTreeDespawnerHandler::Update()
{
	if ( !m_despawners.Empty() )
	{
		const Vector& referencePos = GGame->GetActiveWorld()->GetCameraPosition();

		for ( Uint32 i = 0; i < m_despawners.Size(); )
		{
			if ( m_despawners[ i ].UpdateDespawn( referencePos ) )
			{
				m_despawners.RemoveAtFast( i );
			}
			else
			{
				++i;
			}
		}
	}
}