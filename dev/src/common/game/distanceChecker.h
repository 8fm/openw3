/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CDistanceChecker
{
public:

	CDistanceChecker();
	CDistanceChecker( Float delta );

	void Init( Float delta );
	Bool ShouldUpdate( Bool update = true );
	Bool IsFirstUpdate( Bool update = true );
	Bool WasFirstUpdate() const;
	Vector GetUpdatePosition() const;
	Vector GetMovementDelta() const;
	void Update();
	void Reset();

	virtual const CEntity* GetEntity() const = 0;

private:

	Vector  m_lastUpdatePosition;
	Vector	m_updatePosition;
	Float	m_deltaSquared;
};

class CPlayerDistanceChecker : public CDistanceChecker
{
public:

	CPlayerDistanceChecker();
	CPlayerDistanceChecker( Float delta );

	const CEntity* GetEntity() const override;
};

class CEntityDistanceChecker : public CDistanceChecker
{
public:

	CEntityDistanceChecker();
	CEntityDistanceChecker( CEntity* entity, Float delta );

	void Init( CEntity* entity, Float delta );
	const CEntity* GetEntity() const override;

private:

	THandle< CEntity >	m_entity;
};
