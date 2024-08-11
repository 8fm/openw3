/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "distanceChecker.h"

//////////////////////////////////////////////////////////////////////////

CDistanceChecker::CDistanceChecker()
	: m_deltaSquared( 0.0f )
{
	Reset();
}

CDistanceChecker::CDistanceChecker( Float delta )
	: m_deltaSquared( delta * delta )
{
	Reset();
}

void CDistanceChecker::Init( Float delta )
{
	m_deltaSquared = delta * delta;
	Reset();
}

Bool CDistanceChecker::ShouldUpdate( Bool update /* = true */ )
{
	if ( m_deltaSquared == 0.0f )
	{
		return false;
	}
	const CEntity* entity = GetEntity();
	if ( entity == nullptr )
	{
		return false;
	}
	if ( m_updatePosition == Vector::ZEROS || entity->GetWorldPosition().DistanceSquaredTo( m_updatePosition ) > m_deltaSquared )
	{
		if ( update )
		{
			Update();
		}
		return true;
	}
	return false;
}

Bool CDistanceChecker::IsFirstUpdate( Bool update /* = true */ )
{
	if ( m_deltaSquared == 0.0f )
	{
		return false;
	}
	if ( m_updatePosition == Vector::ZEROS )
	{
		if ( update )
		{
			Update();
		}
		return true;
	}
	return false;
}

Bool CDistanceChecker::WasFirstUpdate() const
{
	return ( m_lastUpdatePosition == Vector::ZEROS );
}

Vector CDistanceChecker::GetUpdatePosition() const
{
	return m_updatePosition;
}

Vector CDistanceChecker::GetMovementDelta() const
{
	return m_updatePosition - m_lastUpdatePosition;
}

void CDistanceChecker::Update()
{
	m_lastUpdatePosition = m_updatePosition;
	const CEntity* entity = GetEntity();
	if ( entity != nullptr )
	{
		m_updatePosition = entity->GetWorldPosition();
	}
}

void CDistanceChecker::Reset()
{
	m_lastUpdatePosition = Vector::ZEROS;
	m_updatePosition = Vector::ZEROS;
}

//////////////////////////////////////////////////////////////////////////

CPlayerDistanceChecker::CPlayerDistanceChecker()
{
}

CPlayerDistanceChecker::CPlayerDistanceChecker( Float delta )
	: CDistanceChecker( delta )
{
}

const CEntity* CPlayerDistanceChecker::GetEntity() const
{
	return GCommonGame->GetPlayer();
}

//////////////////////////////////////////////////////////////////////////

CEntityDistanceChecker::CEntityDistanceChecker()
	: m_entity( nullptr )
{}

CEntityDistanceChecker::CEntityDistanceChecker( CEntity* entity, Float delta )
	: CDistanceChecker( delta )
	, m_entity( entity )
{}

void CEntityDistanceChecker::Init( CEntity* entity, Float delta )
{
	CDistanceChecker::Init( delta );
	m_entity = entity;
}

const CEntity* CEntityDistanceChecker::GetEntity() const
{
	return m_entity.Get();
}
