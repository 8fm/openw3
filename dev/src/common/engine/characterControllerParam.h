#pragma once
#include "../physics/physicalCollision.h"
#include "interactionPriority.h"
#include "entityTemplateParams.h"

//////////////////////////////////////////////////////////////////////////

class CMRPhysicalCharacter;
class CMovingPhysicalAgentComponent;

//////////////////////////////////////////////////////////////////////////

class CMovableRepresentationCreator : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMovableRepresentationCreator, CObject );

	virtual CMRPhysicalCharacter*	CreateRepresentation( CMovingPhysicalAgentComponent* ) = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CMovableRepresentationCreator )
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SControllerRadiusParams
{
	DECLARE_RTTI_STRUCT( SControllerRadiusParams );

	SControllerRadiusParams()
	{
		m_name = CName::NONE;
		m_radius = 0.0f;
	}

	CName	m_name;
	Float	m_radius;
};

BEGIN_CLASS_RTTI( SControllerRadiusParams );
	PROPERTY_EDIT(				m_name,		TXT("Radius Name") );
	PROPERTY_EDIT_RANGE(		m_radius,	TXT("Radius Value"), 0.1f, 50.f );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

struct SVirtualControllerParams
{
	DECLARE_RTTI_STRUCT( SVirtualControllerParams );

	SVirtualControllerParams()
	{
		m_name = CName::NONE;;
		m_boneName = CName::NONE;;
		m_localOffset.SetZeros();
		m_height = 0.0f;
		m_radius = 0.0f;
		m_enabled = true;
		m_targetable = false;
		m_collisions = true;
		m_collisionResponse = false;
		m_localOffsetInModelSpace = false;
		m_collisionGrabber = false;

		m_onCollisionEventName = CName::NONE;
		m_additionalRaycastCheck.SetZeros();
		m_additionalRaycastCheckEventName = CName::NONE;
	}

	CName				m_name;
	CName				m_boneName;
	Vector				m_localOffset;
	Float				m_height;
	Float				m_radius;
	Bool				m_enabled;
	Bool				m_targetable;
	Bool				m_collisions;
	Bool				m_collisionResponse;
	Bool				m_localOffsetInModelSpace;
	Bool				m_collisionGrabber;
	TDynArray<CName>	m_collisionGrabberGroupNames;
	CName				m_onCollisionEventName;
	Vector				m_additionalRaycastCheck;
	CName				m_additionalRaycastCheckEventName;
};

BEGIN_CLASS_RTTI( SVirtualControllerParams );
    PROPERTY_EDIT(          m_name,								TXT("Virtual character controller name.") );
	PROPERTY_EDIT(			m_boneName,							TXT("Bone that controller is attached to. If none, it will be attached to animated component directly.") );
	PROPERTY_EDIT(			m_localOffset,						TXT("Offset from parent controller/bone in it's local coordinate system") );
	PROPERTY_EDIT(			m_localOffsetInModelSpace,			TXT("Offset should be calculated in model space") );
	PROPERTY_EDIT_RANGE(	m_height,							TXT("Character height"), 0.1f, 100.f );
	PROPERTY_EDIT_RANGE(	m_radius,							TXT("Radius used for collisions with physical environment (any static or dynamic geometry excpet the characters)."), 0.1f, 50.f );
	PROPERTY_EDIT(			m_enabled,							TXT("Controller is enabled on start") );
	PROPERTY_EDIT(			m_targetable,						TXT("Controller is targetable") );
	PROPERTY_EDIT(			m_collisions,						TXT("Collision with physics scene (statics and platforms).") );
	PROPERTY_EDIT(			m_collisionResponse,				TXT("Do collision response when hit.") );
	PROPERTY_EDIT(			m_collisionGrabber,					TXT("Grab spherical collisions used to do collision response.") );
	PROPERTY_CUSTOM_EDIT(	m_collisionGrabberGroupNames,		TXT("Defines which collision groups will be grabbed" ), TXT("PhysicalCollisionGroupSelector") );
	PROPERTY_EDIT(			m_onCollisionEventName,				TXT("Event called when collision occurs.") );
	PROPERTY_EDIT(			m_additionalRaycastCheck,			TXT("Additional raycast check direction.") );
	PROPERTY_EDIT(			m_additionalRaycastCheckEventName,	TXT("Event called for additional raycast check result.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCharacterControllerParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CCharacterControllerParam, CGameplayEntityParam, 0 );

public:

	Float								m_height;
	Float								m_physicalRadius;
	Float								m_baseVirtualCharacterRadius;
	Float								m_customAvoidanceRadius;
	Float								m_stepOffset;
	Bool								m_collisionPrediction;
	Float								m_collisionPredictionMovementAdd;
	Float								m_collisionPredictionMovementMul;
	CName								m_collisionPredictionEventName;
	CPhysicalCollision					m_collisionType;
    Float				                m_interactionPriority;
    EInteractionPriority                m_interactionPriorityEnum;
	TDynArray<SVirtualControllerParams>	m_virtualControllers;

	CMovableRepresentationCreator* m_customMovableRep;
	TDynArray<SControllerRadiusParams> m_radiuses;

	Float								m_significance;

	CCharacterControllerParam();
};

BEGIN_CLASS_RTTI( CCharacterControllerParam )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_EDIT_RANGE(		m_height, TXT("Character height"), 0.1f, 100.f );
	PROPERTY_EDIT_RANGE(		m_physicalRadius, TXT("Radius used for collisions with physical environment (any static or dynamic geometry excpet the characters)."), 0.1f, 10.f );
	PROPERTY_EDIT_RANGE(		m_baseVirtualCharacterRadius, TXT("Base radius for virtual character. (if < 0 use physical radius)"), -1.f, 10.f )
	PROPERTY_EDIT_RANGE(		m_customAvoidanceRadius, TXT("Custom avoidance radius. (if < 0 use calculated from all v-radiuses)"), -1.f, 10.f )
	PROPERTY_EDIT(				m_interactionPriority, TXT("How important this character is (more important characters push away less important characters out of the way).") );
    PROPERTY_EDIT(				m_interactionPriorityEnum, TXT("How important this character is (more important characters push away less important characters out of the way).") );
	PROPERTY_EDIT_RANGE(		m_stepOffset, TXT("Max height of an obstacle this character can step on."), 0.f, 50.f );
	PROPERTY_CUSTOM_EDIT(		m_collisionType, TXT( "Physical collision type" ), TXT("PhysicalCollisionTypeSelector") );
	PROPERTY_EDIT(				m_collisionPrediction, TXT( "Enable/Disable Collision Prediction" ) );
	PROPERTY_EDIT_RANGE(		m_collisionPredictionMovementAdd, TXT( "Collision Prediction movement vector addition." ), 0.0f, 50.0f );
	PROPERTY_EDIT_RANGE(		m_collisionPredictionMovementMul, TXT( "Collision Prediction movement vector multiplier." ), 0.0f, 100.0f );
	PROPERTY_EDIT(				m_collisionPredictionEventName, TXT( "Collision Prediction Event Name called on hit." ) );
	PROPERTY_INLINED(			m_radiuses, TXT("Radiuses") );
	PROPERTY_INLINED(			m_virtualControllers, TXT("Virtual controllers to add") );
	PROPERTY_INLINED(			m_customMovableRep, TXT("Custom movable representation ,if different than default")  );
	PROPERTY_EDIT(				m_significance, TXT( "How significant is the character in combat (the bigger value the more significant)" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
