#pragma once

class CTeam;
class CTeamMemberComponent;
struct SR6EnemyInfo;

struct SR6SharedEnemyInfo
{
	SR6SharedEnemyInfo( ): m_enemy( NULL ){}
	SR6SharedEnemyInfo( SR6EnemyInfo* enemyInfo );

	THandle< CActor >			m_enemy;
	Vector				m_knownPosition;

	RED_INLINE const Vector& GetKnownPosition() const { return m_knownPosition; }
};

class CTeamSharedKnowladge: public CObject
{
	DECLARE_ENGINE_CLASS( CTeamSharedKnowladge, CObject, 0 );

static const Float UPDATE_KNOWLEDGE_DELAY;

private:
	TDynArray< SR6SharedEnemyInfo >			m_enemiesInfoBuff1;
	TDynArray< SR6SharedEnemyInfo >			m_enemiesInfoBuff2;
	TDynArray< SR6SharedEnemyInfo >*		m_enemiesInfoCurrent;
	CTeam*									m_team;
	Bool									m_isUpdating;

	Float									m_lastUpdateTime;

	void UpdateKnowladge( );
	void MergeKnowladge( CEntity* teamMember, TDynArray< SR6SharedEnemyInfo >& updatedEnemies );
	void SynchronizeMembersKnowladge();

public:
	CTeamSharedKnowladge() : m_isUpdating( false ), m_lastUpdateTime( EngineTime::ZERO ) {}

	RED_INLINE void SetTeam( CTeam* team ){ m_team = team; }

	const TDynArray< SR6SharedEnemyInfo >& GetEnemiesInfo( );	

	void Tick( Float timeDelta );
};

BEGIN_CLASS_RTTI( CTeamSharedKnowladge );
	PARENT_CLASS( CObject );	
END_CLASS_RTTI();