/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "..\core\object.h"
#include "behaviorIncludes.h"

struct STeleportBone
{
	CName	m_boneName;
	EAxis	m_teleportDetectionAxisLS;

	STeleportBone() : m_teleportDetectionAxisLS( A_X ) {}

	DECLARE_RTTI_STRUCT( STeleportBone );
};

BEGIN_CLASS_RTTI( STeleportBone )
	PROPERTY_EDIT( m_boneName, TXT("Bone name for teleport check") );
	PROPERTY_EDIT( m_teleportDetectionAxisLS, TXT("Bone axis for teleport check") );
END_CLASS_RTTI();

class CTeleportDetectorData : public CObject
{
	DECLARE_ENGINE_CLASS( CTeleportDetectorData, CObject, 0 );

public:
	CTeleportDetectorData();

private:
	STeleportBone				m_pelvisTeleportData;
	TDynArray< STeleportBone >	m_teleportedBones;
	Float						m_angleDif;
	Float						m_pelvisPositionThreshold;

public:
	const TDynArray< STeleportBone >&	GetTeleportedBones() const { return m_teleportedBones; };
	STeleportBone						GetTeleportPelvisData() const { return m_pelvisTeleportData; };
	RED_INLINE Float					GetTeleportAngleThreshold() const { return m_angleDif; }
	RED_INLINE Float					GetTeleportPositionThreshold() const { return m_pelvisPositionThreshold; }
};

BEGIN_CLASS_RTTI( CTeleportDetectorData )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_angleDif, TXT("Angle threshold for teleport detection ") );
	PROPERTY_EDIT( m_pelvisPositionThreshold, TXT("Pelvis bones position offset to trigget teleport") );
	PROPERTY_EDIT( m_pelvisTeleportData, TXT("Pelvis desription for teleport") );
	PROPERTY_EDIT( m_teleportedBones, TXT("Bones descriptor for teleport") );
END_CLASS_RTTI();
