/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once
class CActor;

#include "../../common/game/baseDamage.h"

struct SProcessedDamage
{
	DECLARE_RTTI_STRUCT( SProcessedDamage );
	Float	m_vitalityDamage;
	Float	m_essenceDamage;
	Float	m_moraleDamage;
	Float	m_staminaDamage;	

	SProcessedDamage()
		: m_vitalityDamage( 0.0f )
		, m_essenceDamage( 0.0f )
		, m_moraleDamage( 0.0f )
		, m_staminaDamage( 0.0f )
	{}
};

BEGIN_CLASS_RTTI(SProcessedDamage)
	PROPERTY( m_vitalityDamage );
	PROPERTY( m_essenceDamage );
	PROPERTY( m_moraleDamage );
	PROPERTY( m_staminaDamage );
END_CLASS_RTTI()


class CDamageData : public CBaseDamage
{
public:
	DECLARE_ENGINE_CLASS( CDamageData ,CBaseDamage, 0 );

	SProcessedDamage	m_processedDmg;
	Bool				m_additiveHitReactionAnimRequested;
	Bool				m_customHitReactionRequested;
	Bool				m_isDoTDamage;

	CDamageData()
		: CBaseDamage ()
		, m_additiveHitReactionAnimRequested( false )
		, m_customHitReactionRequested( false )
		, m_isDoTDamage( false )
	{}
};

BEGIN_CLASS_RTTI( CDamageData );	
	PARENT_CLASS( CBaseDamage );
	PROPERTY( m_processedDmg );
	PROPERTY( m_additiveHitReactionAnimRequested );
	PROPERTY( m_customHitReactionRequested );
	PROPERTY( m_isDoTDamage );
END_CLASS_RTTI();