/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CActionPointComponent;

class CActionPointReservationReleaseHandler : public ISpawnEventHandler
{	
	typedef ISpawnEventHandler Super;

	static const Float	AP_RESERVATION_TIME;

protected:
	THandle< CActionPointComponent > m_reservedActionPoint;
	Bool							m_reservationReleased;

	void ReleaseSpawnReservation();

public:
	DECLARE_NAMED_EVENT_HANDLER( ReservationRelease )

	CActionPointReservationReleaseHandler( CActionPointComponent* reservedAP );
	~CActionPointReservationReleaseHandler();

	void OnPostAttach( CEntity* entity ) override;
};