/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/aiStorage.h"

#include "volumePathManager.h"

class IBehTreeNodeAtomicFlightInstance;

class CBehTreeFlightData
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeFlightData )

public:
	typedef TDynArray< Vector3 > Path;
protected:
	typedef CVolumePathManager::Coordinate Coordinate;
	//Int32				m_lastCel[3];
	//Bool				m_isInEmergencyMode;

	//Float				m_lastPathfindTime;

	CVolumePathManager*	m_pathManager;

	Coordinate			m_lastPositionCoords;
	Coordinate			m_currentPathDestinationCoords;
	Bool				m_isEmergencyMode;
	Bool				m_isPathFollowing;
	Bool				m_forceReachTarget;
	Vector3				m_lastProperPos;
	Vector3				m_desiredPosition;
	Vector3				m_currentDestination;
	Vector3				m_emergencyRepulsion;
	Float				m_pathDistanceFromDesiredTarget;
	
	Path				m_detailedPath;
	Path				m_currentPath;

	Bool						PlotPath( IBehTreeNodeAtomicFlightInstance* node, const Vector3& sourcePos, const Vector3& desiredPosition );

public:
	CBehTreeFlightData();

	void						Activate( CActor* actor );
	Bool						UpdateFlight( IBehTreeNodeAtomicFlightInstance* node );
	void						Deactivate( CActor* actor );
	void						SetDesiredPosition( const Vector& newDestination );

	void						SetupBehaviorVariables( CActor* actor, const Vector& destination );

	void						SetForceReachTarget( Bool b = true )						{ m_forceReachTarget = b; }

	const Vector3&				GetCurrentDestination() const								{ return m_currentDestination; }
	Float						GetPathDistanceFromDesiredTarget() const					{ return m_pathDistanceFromDesiredTarget; }
	const Path&					GetCurrentPath() const										{ return m_currentPath; }
	const Path&					GetCurrentDetailedPath() const								{ return m_detailedPath; }
	Bool						IsInEmergencyMode() const									{ return m_isEmergencyMode; }
	
	static void					EnableFlightState( CActor* actor );
	static void					DisableFlightState( CActor* actor );

	class CInitializer : public CAIStorageItem::CInitializer
	{
	public:
		CName					GetItemName() const override;
		IRTTIType*				GetItemType() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeFlightData )
END_CLASS_RTTI()

class CBehTreeFlightDataPtr : public TAIStoragePtr< CBehTreeFlightData >
{
	typedef TAIStoragePtr< CBehTreeFlightData > Super;
public:
	CBehTreeFlightDataPtr( CAIStorage* storage );
};