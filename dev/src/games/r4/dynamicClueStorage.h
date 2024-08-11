/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/entitiesDetector.h"

struct SDynamicClue
{
	THandle< CEntityTemplate >	m_template;
	CLayer*						m_layer;
	Vector						m_position;
	EulerAngles					m_rotation;
	THandle< CEntity >			m_entity;

	SDynamicClue();
	SDynamicClue( const THandle< CEntityTemplate >& templ, CLayer* layer, const Vector& position, const EulerAngles& rotation );

	Bool ShouldBeSpawned() const;
	Bool ShouldBeDestroyed() const;
	void Spawn();
	void Destroy();

	// for BinaryStorage
	RED_INLINE Vector& GetWorldPositionRef()
	{
		return m_position;
	}
};

class CDynamicClueStorage
{
public:

	CDynamicClueStorage();
	~CDynamicClueStorage();

	void OnGameStart();
	void OnGameEnd();

	SDynamicClue* Add( const THandle< CEntityTemplate >& templ, CLayer* layer, const Vector& position, const EulerAngles& rotation  );
	Bool Remove( SDynamicClue* clue );

	void Update();

	static Float UPDATE_RANGE;
	static Float SPAWN_MIN_RANGE;
	static Float SPAWN_MIN_RANGE_SQR;
	static Float SPAWN_MAX_RANGE;
	static Float SPAWN_MAX_RANGE_SQR;
	static Float DESTROY_RANGE;
	static Float DESTROY_RANGE_SQR;

private:

	TDynArray< SDynamicClue* >	m_clues;
	TDynArray< SDynamicClue* >	m_cluesToDestroy;	
	CPlayerDistanceChecker		m_distanceChecker;

	void Clear();
	void LoadConfig();
	void SetRange( const String& rangeName, Float value );
	void DestroyClues( Bool force = false );
};
