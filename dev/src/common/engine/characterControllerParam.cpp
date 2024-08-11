#include "build.h"
#include "characterControllerParam.h"
#include "physicsCharacterWrapper.h"

IMPLEMENT_ENGINE_CLASS( SControllerRadiusParams )
IMPLEMENT_ENGINE_CLASS( SVirtualControllerParams );
IMPLEMENT_RTTI_ENUM( EInteractionPriority );
IMPLEMENT_ENGINE_CLASS( CCharacterControllerParam );
IMPLEMENT_ENGINE_CLASS( CMovableRepresentationCreator );


CCharacterControllerParam::CCharacterControllerParam()
	: m_height							( SCCTDefaults::DEFAULT_HEIGHT )
	, m_physicalRadius					( SCCTDefaults::DEFAULT_RADIUS )
	, m_baseVirtualCharacterRadius		( -1.f )
	, m_customAvoidanceRadius			( -1.0f )
	, m_stepOffset						( SCCTDefaults::DEFAULT_STEP_OFFSET )
    , m_interactionPriority             ( -1.0f )
    , m_interactionPriorityEnum         ( IP_NotSet )
	, m_collisionType					( SCCTDefaults::DEFAULT_COLLISION_TYPE )
	, m_customMovableRep				( nullptr )
	, m_collisionPrediction				( false )
	, m_collisionPredictionMovementAdd	( 0.0f )
	, m_collisionPredictionMovementMul	( 1.0f )
	, m_collisionPredictionEventName	( CName::NONE )
	, m_significance					( 1.f )
{}
