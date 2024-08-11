#pragma once
#include "triggerAreaComponent.h"
 
#define DEFAULT_ENVIRONMENT_AREA_COMPONENT_PRIORITY 100

enum EAreaEnvironmentPointType
{
	AEPT_FadeOut,
	AEPT_FadeIn,
	AEPT_Additive,
	AEPT_Subtractive,
	AEPT_SubEnvironment
};

BEGIN_ENUM_RTTI( EAreaEnvironmentPointType );
	ENUM_OPTION( AEPT_FadeOut );
	ENUM_OPTION( AEPT_FadeIn );
	ENUM_OPTION( AEPT_Additive );
	ENUM_OPTION( AEPT_Subtractive );
	ENUM_OPTION( AEPT_SubEnvironment );
END_ENUM_RTTI();

enum EAreaEnvironmentPointBlend
{
	AEPB_DistanceOnly,
	AEPB_CameraFocusAndDistance,
	AEPB_CameraAngleAndDistance
};

BEGIN_ENUM_RTTI( EAreaEnvironmentPointBlend );
	ENUM_OPTION( AEPB_DistanceOnly );
	ENUM_OPTION( AEPB_CameraFocusAndDistance );
	ENUM_OPTION( AEPB_CameraAngleAndDistance );
END_ENUM_RTTI();

struct SAreaEnvironmentPoint
{
	DECLARE_RTTI_STRUCT( SAreaEnvironmentPoint );

	Vector								m_position;						//!< Point position
	EulerAngles							m_direction;					//!< Point direction (for AEPB_CameraAngleAndDistance)
	EAreaEnvironmentPointType			m_type;							//!< Point type
	EAreaEnvironmentPointBlend			m_blend;						//!< Point blend type
	Float								m_innerRadius;					//!< Inner radius where 100% of the point applies
	Float								m_outerRadius;					//!< Outer radius where the point's influence fades out
	Float								m_scaleX;						//!< X axis scale
	Float								m_scaleY;						//!< Y axis scale
	Float								m_scaleZ;						//!< Z axis scale
	Bool								m_useCurve;						//!< Use influence curve
	SSimpleCurve						m_curve;						//!< The influence curve
	Float								m_blendScale;					//!< Point blend scale
	THandle< CEnvironmentDefinition >	m_environmentDefinition;		//!< Optional point-specific environment definition

	SAreaEnvironmentPoint();
};

BEGIN_CLASS_RTTI( SAreaEnvironmentPoint );
	PROPERTY_EDIT( m_position, TXT("Point position") );
	PROPERTY_EDIT( m_direction, TXT("Point direction") );
	PROPERTY_EDIT( m_type, TXT("Point type") );
	PROPERTY_EDIT( m_blend, TXT("Point blend type") );
	PROPERTY_EDIT_RANGE( m_innerRadius, TXT("Inner radius where 100% of the point applies"), 0.0f, 16384.0f );
	PROPERTY_EDIT_RANGE( m_outerRadius, TXT("Outer radius where the point's influence fades out"), 0.0f, 16384.0f );
	PROPERTY_EDIT_RANGE( m_scaleX, TXT("Scale the radii on the X axis"), 0.0001f, 16384.0f );
	PROPERTY_EDIT_RANGE( m_scaleY, TXT("Scale the radii on the Y axis"), 0.0001f, 16384.0f );
	PROPERTY_EDIT_RANGE( m_scaleZ, TXT("Scale the radii on the Z axis"), 0.0001f, 16384.0f );
	PROPERTY_EDIT( m_useCurve, TXT("Use the influence curve") );
	PROPERTY_EDIT_RANGE( m_curve, TXT("Influence curve"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_blendScale, TXT("Blending scale"), 0.0f, 1.0f );
	PROPERTY_EDIT( m_environmentDefinition, TXT("Optional environment definition (point type must be AEPT_SubEnvironment)") );
END_CLASS_RTTI();

/// Environment area component
class CAreaEnvironmentComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CAreaEnvironmentComponent, CTriggerAreaComponent, 0 );

protected:	
	THandle< CEnvironmentDefinition >	m_environmentDefinition;		//!< Environment definition
	CWorld*								m_world;						//!< World, we are attached to
	TEnvManagerAreaEnvId				m_id;							//!< Identifier
	Int32								m_priority;						//!< Area priority
	Float								m_blendingDistance;				//!< Distance from the edge where the blending is 100%
	Float								m_blendingScale;				//!< Blending lerp factor scale (use higher values for inner areas)
	Float								m_blendInTime;					//!< Time in seconds to blend in (0=instant)
	Float								m_blendOutTime;					//!< Time in seconds to blend out (0=instant)
	Float								m_terrainBlendingDistance;		//!< For "only below terrain" areas: additional blending distance
	Bool								m_blendAboveAndBelow;			//!< Enable blending from above and below in addition to edge
	Bool								m_blendingCurveEnabled;			//!< Enable blending curve
	SSimpleCurve						m_blendingCurve;				//!< Blending curve
	THandle< CParticleSystem >			m_rainDropsParticleSystem;		//!< Particle system for rain drops
	THandle< CParticleSystem >			m_rainSplashesParticleSystem;	//!< Particle system for rain drops
	THandle< CEntityTemplate >			m_additionalEnvEntity;			//!< Additional env entity
	TDynArray< SAreaEnvironmentPoint >	m_points;						//!< Blending points

public:
	/// Class constructor
	CAreaEnvironmentComponent();
	~CAreaEnvironmentComponent();

	/// Get environment parameters
	const CAreaEnvironmentParams* GetParameters() const;

	RED_INLINE CEnvironmentDefinition* GetEnvironmentDefinition() { return m_environmentDefinition.Get(); }
	void SetEnvironmentDefinition( CEnvironmentDefinition* definition );

	RED_INLINE Float GetBlendingDistance() const { return m_blendingDistance; }
	RED_INLINE Float GetBlendingScale() const { return m_blendingScale; }
	RED_INLINE Float GetBlendInTime() const { return m_blendInTime; }
	RED_INLINE Float GetBlendOutTime() const { return m_blendOutTime; }
	RED_INLINE Float GetTerrainBlendingDistance() const { return m_terrainBlendingDistance; }
	RED_INLINE Int32 GetPriority() const { return m_priority; }
	RED_INLINE Bool GetBlendAboveAndBelow() const { return m_blendAboveAndBelow; }
	RED_INLINE const TDynArray< SAreaEnvironmentPoint >& GetPoints() const { return m_points; }
	RED_INLINE Bool GetBlendingCurveEnabled() const { return m_blendingCurveEnabled; }
	RED_INLINE const SSimpleCurve& GetBlendingCurve() const { return m_blendingCurve; }

#ifndef NO_EDITOR
public:
	void AddPoint( const SAreaEnvironmentPoint& point );
	void RemovePoint( Uint32 pointIndex );
	void RemoveAllPoints();
	RED_INLINE const SAreaEnvironmentPoint& GetPoint( Uint32 pointIndex ) const { return m_points[pointIndex]; }
	void UpdatePoint( Uint32 pointIndex, const SAreaEnvironmentPoint& point );
#endif
	
public:
	/// Properties change notification
	void NotifyPropertiesImplicitChange();

	// Get area env id
	TEnvManagerAreaEnvId GetAreaEnvId() const { return m_id; }

	/// Activates or deactivates this environment
	void SetActivated( bool activate );

	/// Activate this environment
	Bool Activate( bool force = false );

	/// Is this environment active
	Bool IsActive() const;

	/// Deactivated this environment
	void Deactivate( bool force = false );

public:
	/// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	/// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	/// Property post change
	virtual void OnPropertyPostChange( IProperty* property );

	/// Draw area
	virtual void DrawArea( CRenderFrame* frame, const TAreaPoints & worldPoints );

public:
	/// Something have entered zone
	virtual void EnteredArea( CComponent* component );

	/// Something have exited zone
	virtual void ExitedArea( CComponent* component );
};

BEGIN_CLASS_RTTI( CAreaEnvironmentComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_priority,						TXT("Priority") );
	PROPERTY_EDIT_RANGE( m_blendingDistance,		TXT("Distance from the edge where the blending is at 100%"), 0.0f, 10000.0f );
	PROPERTY_EDIT_RANGE( m_blendingScale,			TXT("Blending lerp factor scale (lower values to decrease contribution, higher values for inner areas)"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_blendInTime,				TXT("Blend in time in seconds (0=instant)"), 0.0f, 120.0f );
	PROPERTY_EDIT_RANGE( m_blendOutTime,			TXT("Blend out time in seconds (0=instant)"), 0.0f, 120.0f );
	PROPERTY_EDIT_RANGE( m_terrainBlendingDistance,	TXT("For 'only below terrain' areas: distance from the terrain where the blending is at 100%"), 0.0f, 10000.0f );
	PROPERTY_EDIT( m_blendAboveAndBelow,			TXT("Enable blending from above and below in addition to edge") );
	PROPERTY_EDIT( m_rainDropsParticleSystem,		TXT("Rain particle system for rain drops") );
	PROPERTY_EDIT( m_rainSplashesParticleSystem,	TXT("Rain particle system for rain splashes") );
	PROPERTY_EDIT( m_additionalEnvEntity,			TXT("Additional env entity") );
	PROPERTY_EDIT( m_environmentDefinition,			TXT("Environment definition file") );
	PROPERTY_EDIT( m_blendingCurveEnabled,			TXT("Blending curve enabled (do not forget to set the curve!)") );
	PROPERTY_EDIT( m_blendingCurve,					TXT("Blending curve (do not forget to check blendingCurveEnabled!)") );
	PROPERTY_EDIT( m_points,						TXT("Area blending points") );
END_CLASS_RTTI();
