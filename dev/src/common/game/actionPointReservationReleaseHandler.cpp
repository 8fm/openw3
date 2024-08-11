/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actionPointReservationReleaseHandler.h"
#include "actionPointManager.h"
#include "actionPointComponent.h"

const Float	CActionPointReservationReleaseHandler::AP_RESERVATION_TIME = 1; //seconds

CActionPointReservationReleaseHandler::CActionPointReservationReleaseHandler( CActionPointComponent* reservedAP )
	: m_reservedActionPoint( reservedAP )
	, m_reservationReleased( false )
{

}

CActionPointReservationReleaseHandler::~CActionPointReservationReleaseHandler()
{
	ReleaseSpawnReservation();
}

void CActionPointReservationReleaseHandler::ReleaseSpawnReservation()
{
	if( m_reservationReleased )
	{
		return;
	}
	m_reservationReleased = true;
	CActionPointComponent* ap = m_reservedActionPoint.Get();
	if( ap )
	{
		ap->SetFree( CActionPointManager::REASON_SPAWNING );
	}
}

void CActionPointReservationReleaseHandler::OnPostAttach( CEntity* entity )
{
	ReleaseSpawnReservation();

	CActionPointComponent* ap = m_reservedActionPoint.Get();
	if( ap )
	{
		// so no one will take this AP
		ap->SetReserved( CActionPointManager::REASON_RESERVATION, Cast< CNewNPC >( entity ), AP_RESERVATION_TIME );
	}
}