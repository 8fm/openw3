/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dynamicDecal.h"

class CDecalSpawner : public CObject
{
	friend class CRenderDecalSpawner;
	DECLARE_ENGINE_CLASS( CDecalSpawner, CObject, 0 );
protected:
	SDynamicDecalMaterialInfo		m_material;
	Float							m_farZ;
	Float							m_nearZ;
	IEvaluatorFloat*				m_size;
	Float							m_depthFadePower;
	Float							m_normalFadeBias;
	Float							m_normalFadeScale;
	Bool							m_doubleSided;

	ERenderDynamicDecalProjection	m_projectionMode;

	IEvaluatorFloat*				m_decalLifetime;
	Float							m_decalFadeTime;
	Float							m_decalFadeInTime;			// Time needed for decal to reach full opacity
	Bool							m_projectOnlyOnStatic;		// if true, decal will be projected only on static meshes
	Float							m_startScale;				// starting scale of decal (m_startScale * m_size)
	Float							m_scaleTime;				// time needed to reach full size
	Bool							m_useVerticalProjection;	// if set to true, will always project decal from top
	EDynamicDecalSpawnPriority		m_spawnPriority;			//!< Spawn priority, set high for cutscenes
	Float							m_autoHideDistance;			//!< Dynamic decal autohide distance
	Float							m_chance;					//!< Chance to roll the drop [0..1]
	Float							m_spawnFrequency;			//!< [Motion Decal spawner only] Time between spawned decals 

public:
	CDecalSpawner();
	RED_INLINE Float GetAutoHideDistance() const { return m_autoHideDistance; }
};

BEGIN_CLASS_RTTI( CDecalSpawner );
PARENT_CLASS( CObject );
PROPERTY_EDIT( m_material, TXT( "Material for generated decals" ) );
PROPERTY_EDIT( m_farZ, TXT( "Far z" ) );
PROPERTY_EDIT( m_nearZ, TXT( "Near z. Can be negative to project behind the spawn point." ) );
PROPERTY_INLINED( m_size, TXT( "Size of decals" ) );
PROPERTY_EDIT( m_depthFadePower, TXT("Higher values cause the decal to fade out smoothly towards the ends of the wound. Lower creates a sharper cutoff") );
PROPERTY_EDIT( m_normalFadeBias, TXT("Bias applied when fading out based on the surface normal. Higher values make the decal more visible when the surface points away from projection direction") );
PROPERTY_EDIT( m_normalFadeScale, TXT("Scale applied when fading out based on surface normal. Higher values make the fade sharper.") );
PROPERTY_EDIT( m_doubleSided, TXT("Whether the decal is applied to surfaces facing away from the projection") );
PROPERTY_EDIT( m_projectionMode, TXT("How the decal texture coordinates are calculated. Ortho: orthographic projection, decal keeps the same size and shape. Sphere: decal shrinks towards ends, making an ellipsoidal volume.") );
PROPERTY_INLINED( m_decalLifetime, TXT( "Life time of single decal" ) );
PROPERTY_EDIT( m_decalFadeTime, TXT( "Fade time of single decal" ) );
PROPERTY_EDIT( m_decalFadeInTime, TXT( "Fade in time of single decal" ) );
PROPERTY_EDIT( m_projectOnlyOnStatic, TXT( "If set to true, decal will be rendered only on static geometry" ) );
PROPERTY_EDIT( m_startScale, TXT( "starting scale of decal (startScale * size)" ) );
PROPERTY_EDIT( m_scaleTime, TXT( "how long it takes the decal to reach full size" ) );
PROPERTY_EDIT( m_useVerticalProjection, TXT( "if set to true, will always project decal from top" ) );
PROPERTY_EDIT( m_spawnPriority, TXT("Spawn priority, set high for cutscenes") );
PROPERTY_EDIT( m_autoHideDistance, TXT( "Auto hide distance. -1 will set default value from ini file" ) );
PROPERTY_EDIT_RANGE( m_chance , TXT("Chance to drop the decal") , 0.0f , 1.0f );
PROPERTY_EDIT( m_spawnFrequency , TXT("Frequency of decals spawning") );
END_CLASS_RTTI();
