#pragma once

#include "gameplayEntity.h"

class CBaseDamage : public CObject
{
public:
	DECLARE_ENGINE_CLASS( CBaseDamage ,CObject, 0 );

	Vector					m_hitLocation;
	Vector					m_momentum;
	THandle< IScriptable >  m_causer;

	THandle< CGameplayEntity >	m_attacker;
	THandle< CGameplayEntity >	m_victim;

	Bool				m_hitReactionAnimRequested;


	CBaseDamage()
		: m_hitLocation( Vector::ZEROS )
		, m_momentum (Vector::ZEROS )
		, m_causer( NULL )
		, m_attacker( NULL )
		, m_victim( NULL )
		, m_hitReactionAnimRequested ()
	{}
};

BEGIN_CLASS_RTTI( CBaseDamage );	
	PARENT_CLASS( CObject );
	PROPERTY( m_hitLocation );
	PROPERTY( m_momentum );
	PROPERTY( m_causer );
	PROPERTY( m_attacker );
	PROPERTY( m_victim );
	PROPERTY( m_hitReactionAnimRequested );
END_CLASS_RTTI();