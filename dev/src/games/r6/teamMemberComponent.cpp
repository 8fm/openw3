#include "build.h"

#include "teamMemberComponent.h"
#include "combatUtils.h"
#include "teamManager.h"
#include "team.h"

IMPLEMENT_ENGINE_CLASS( CTeamMemberComponent );

void CTeamMemberComponent::OnEventOccure( CName eventName, CObject* param )
{
	if( eventName == CNAME( OnEnemyNoticed ) )
	{
		FetchTeamIfNeeded();
	}
	else if( eventName == CNAME( OnOwnerKilled ) )
	{
		if( m_team )
		{
			m_team->RemoveMember( this );
			m_team = NULL;
		}
	}
}

void CTeamMemberComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	CEntity* ownerEnt	= GetEntity();
	
	if( ownerEnt->IsA< CPlayer >() )
	{
		m_team = NULL;
		FetchTeamIfNeeded();
	}

	ListenToEvent( CNAME( OnEnemyNoticed ), CName::NONE, false );
	ListenToEvent( CNAME( OnOwnerKilled	 ), CName::NONE, false );	
}

void CTeamMemberComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	if( m_team )
	{
		m_team->RemoveMember( this );
		m_team = NULL;
	}
}

void CTeamMemberComponent::FetchTeamIfNeeded()
{
	if( !m_team )
	{
		CR6Game* r6Game = static_cast< CR6Game* >( GGame );

		CCombatUtils* combatUtils = r6Game->GetCombatUtils();
		CTeamManager* teamManager = combatUtils->GetTeamManager();

		m_team = teamManager->GetTeamByName( m_teamName );
		if( m_team )
		{
			m_team->AddMember( this );
		}
	}
}

Bool CTeamMemberComponent::IfAnyAllyInShootingCorridor( const Vector& shootingDirection, const Vector& enemyPosition, Float corridorWidth )
{
	FetchTeamIfNeeded();

	CEntity* npc			= GetEntity();
	Vector myPosition		= npc->GetWorldPosition();
	Vector targetDirection	=  enemyPosition - myPosition;
	Float corridorWidthSqrt = corridorWidth * corridorWidth;

	const TDynArray< THandle< CTeamMemberComponent > >& teamMembers = m_team->GetMembers();

	for( Uint32 i=0; i<teamMembers.Size(); ++i )
	{
		if( !teamMembers[i].Get() )
			continue;

		CEntity* ally = teamMembers[i].Get()->GetEntity();

		if( npc == ally )
			continue;

		Vector allyPosition		= ally->GetWorldPosition();
		Vector allyDirection	= allyPosition - myPosition;

		Float dot = Vector::Dot3( targetDirection, allyDirection );
		if( dot < 0 )
			continue;
		

		Vector nearest			= allyPosition.NearestPointOnEdge( myPosition, enemyPosition );

		if( ( nearest - allyPosition ).SquareMag3() <= corridorWidthSqrt )
			return true;

	}

	return false;
}

void CTeamMemberComponent::funcGetTeam( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	FetchTeamIfNeeded();
	THandle< CTeam > teamHandle = m_team;
	RETURN_HANDLE( CTeam, teamHandle );
}

void CTeamMemberComponent::funcIfAnyAllyInShootingCorridor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, shootingDirection, Vector::ZEROS );
	GET_PARAMETER( Vector, enemyPosition, Vector::ZEROS );
	GET_PARAMETER( Float, corridorWidth, 0 );
	FINISH_PARAMETERS;

	Bool allyInCorridor = IfAnyAllyInShootingCorridor( shootingDirection, enemyPosition, corridorWidth );

	RETURN_BOOL( allyInCorridor );
}

void CTeamMemberComponent::funcAcquireMovementTicket( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	FetchTeamIfNeeded();

	Bool success = m_team->AcquireMovementTicket( this );
	RETURN_BOOL( success );
}

void CTeamMemberComponent::funcReturnMovementTicket( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	FetchTeamIfNeeded();
	m_team->ReturnMovementTicket( this );
}

void CTeamMemberComponent::funcSetSubteamFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, sFlag, 0 );
	FINISH_PARAMETERS;

	m_subteamFlag = sFlag;
}
void CTeamMemberComponent::funcGetSubteamFlag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_subteamFlag );
}
void CTeamMemberComponent::funcResetSubteamFlag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	m_subteamFlag = 0;
}