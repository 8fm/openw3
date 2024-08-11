/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "../engine/baseEngine.h"

struct SLootEntry
{
	CName		m_itemName;
	Uint32		m_quantityMin;
	Uint32		m_quantityMax;
	Uint32		m_respawnTime;
	EngineTime	m_nextRespawnTime;
	Bool		m_hasAreaLimit;

	Bool		m_shouldBeRebalanced;

	SLootEntry()
		: m_shouldBeRebalanced( false )
	{
	}

	Uint32 GetRandomQuantity() const
	{
		if ( m_quantityMax > m_quantityMin )
		{
			return GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_quantityMin , m_quantityMax + 1 );
		}
		return m_quantityMin;
	}
};
