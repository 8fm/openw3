/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/newNpcSensesManager.h"

class CEnemiesCachedData
{
private:
    struct SEnemyInfo
    {
	    THandle< CActor >	m_actor;
		VisibilityQueryId	m_traceQuery;
	    Bool				m_isVisible;
    };

    TDynArray< SEnemyInfo > m_enemies;

	// Settings
	Uint32	m_maxEnemies;
	Box		m_boundingBox;
	CName	m_tag;
	Uint32	m_flags;

public:
	CEnemiesCachedData() { Setup( 10.f, 5.f, 16, CName::NONE, 0 ); }

	void Update( CActor& player );

	void Setup( Float range, Float heightTolerance, Uint32 maxEnemies, const CName& tag, Uint32 flags );

	Bool IsVisible( const THandle<CActor>& actor ) const;

	void GetVisibleEnemies( TDynArray< CActor* >& actors ) const;
	void GetVisibleEnemies( TDynArray< THandle< CActor > >& actors ) const;
	void GetAllEnemiesInRange( TDynArray< THandle< CActor > >& actors ) const;

private:
	void UpdateEnemies( CActor& player );
};
