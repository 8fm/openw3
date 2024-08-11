/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "bitmapTexture.h"


class CBitmapTexture;
class IRenderResource;

// NOTE : Keep this synced to dynamicDecalGen.fx and dynamicDecal.fx
enum ERenderDynamicDecalProjection
{
	RDDP_Ortho = 0,		// Project decal into a rectangular area
	RDDP_Sphere = 1,	// Project decal into an ellipsoid volume
};
BEGIN_ENUM_RTTI( ERenderDynamicDecalProjection )
	ENUM_OPTION( RDDP_Ortho )
	ENUM_OPTION( RDDP_Sphere )
END_ENUM_RTTI()

enum EDynamicDecalSpawnPriority
{
	RDDS_Normal		= 0,	// Spawn as regular in-game decal (obey limits)
	RDDS_Highest	= 1,	// Force spawn as cutscene, dismemberment etc (do not obey)
};
BEGIN_ENUM_RTTI( EDynamicDecalSpawnPriority )
	ENUM_OPTION( RDDS_Normal )
	ENUM_OPTION( RDDS_Highest )
	END_ENUM_RTTI()


enum ERenderDynamicDecalAtlas
{
	RDDA_1x1 = 0,	
	RDDA_2x1 = 1,	
	RDDA_2x2 = 2,	
	RDDA_4x2 = 3,	
	RDDA_4x4 = 4,	
	RDDA_8x4 = 5,	
};
BEGIN_ENUM_RTTI( ERenderDynamicDecalAtlas )
	ENUM_OPTION( RDDA_1x1 )
	ENUM_OPTION( RDDA_2x1 )
	ENUM_OPTION( RDDA_2x2 )
	ENUM_OPTION( RDDA_4x2 )
	ENUM_OPTION( RDDA_4x4 )
	ENUM_OPTION( RDDA_8x4 )
END_ENUM_RTTI()

struct SDynamicDecalMaterialInfo
{
	DECLARE_RTTI_STRUCT( SDynamicDecalMaterialInfo );

	THandle< CBitmapTexture >		m_diffuseTexture;
	THandle< CBitmapTexture >		m_normalTexture;

	Color							m_specularColor;
	Float							m_specularScale;
	Float							m_specularBase;
	Float							m_specularity;

	Bool							m_additiveNormals;		// If normal texture is given, says whether decal normals replace or add to scene normals.

	Color							m_diffuseRandomColor0;		//!< Diffuse color used in randomization
	Color							m_diffuseRandomColor1;		//!< Diffuse color used in randomization
	ERenderDynamicDecalAtlas		m_subUVType;				

	SDynamicDecalMaterialInfo()
		: m_diffuseTexture( nullptr )
		, m_normalTexture( nullptr )
		, m_specularColor( 0, 0, 0, 0 )
		, m_specularScale( 0 )
		, m_specularBase( 1 )
		, m_additiveNormals( true )
		, m_diffuseRandomColor0( Color::WHITE )
		, m_diffuseRandomColor1( Color::WHITE )
		, m_subUVType( RDDA_1x1 )
		, m_specularity( -1.0f )
	{}
};
BEGIN_CLASS_RTTI( SDynamicDecalMaterialInfo );
	PROPERTY_EDIT( m_diffuseTexture, TXT("Diffuse texture") );
	PROPERTY_EDIT( m_diffuseRandomColor0 , TXT("Diffuse color used in ranom coloring [begining]") );
	PROPERTY_EDIT( m_diffuseRandomColor1 , TXT("Diffuse color used in ranom coloring [ending]") );
	PROPERTY_EDIT( m_subUVType , TXT("Type of atlas in diffuse texture") );
	PROPERTY_EDIT( m_normalTexture, TXT("Optional normalm ap. Alpha channel holds roughness, like pbr_std. If null, the decal will use the scene's normals. Using the scene's normals is more expensive.") );
	PROPERTY_EDIT( m_additiveNormals, TXT("Only used if a normal map is provided. Additive will combine the normal map with the scene's normals, similar to a detail map. Non-additive uses just the decal's normals. Using scene's normals is more expensive.") );
	PROPERTY_EDIT( m_specularColor, TXT("Specular color") );
	PROPERTY_EDIT( m_specularScale, TXT("Roughness scale") );
	PROPERTY_EDIT( m_specularBase, TXT("Roughness base") );
	PROPERTY_EDIT( m_specularity, TXT("Specularity") );
END_CLASS_RTTI();

struct SDynamicDecalInitInfo
{
	// From SDynamicDecalMaterialInfo, except using the render resources from texture, instead of the textures themselves.
	// This way, it's possible to use this from anywhere.
	IRenderResource*				m_diffuseTexture;		// Does not addref this.
	IRenderResource*				m_normalTexture;		// Does not addref this.

	Color							m_diffuseColor;
	Color							m_specularColor;
	Float							m_specularScale;
	Float							m_specularBase;
	Float							m_specularity;


	Vector							m_origin;				// Position of the decal.
	Vector							m_dirFront;				// Direction the decal should be projected along.
	Vector							m_dirUp;				// Decal's "up" direction, to adjust rotation around dirFront.
	Float							m_width;				// Width of the decal
	Float							m_height;				// Height of the decal
	Float							m_nearZ;				// "near" distance for decal projection. Can be negative to project behind the origin.
	Float							m_farZ;					// "far" distance for decal projection.

	Float							m_depthFadePower;		// Exponent applied to the distance from origin along "front" vector.
	Float							m_normalFadeBias;		// 
	Float							m_normalFadeScale;		// 
	Bool							m_additiveNormals;		// If normal texture is given, says whether decal normals replace or add to scene normals.
	Bool							m_doubleSided;			// Whether the decal should be projected in both directions, or one-way.

	Bool							m_applyInLocalSpace;	// Whether the decal info above describes the decal in the local space of whatever it's being applied
															// to, or in world space. Most things should probably use world space, since multiple objects could
															// be affected by the same decal.

	Matrix							m_worldToDecalParent;	// When applyInLocalSpace is true, this is used to put the decal into world space for objects
															// that don't support local space decals (apex cloth mostly, since we don't have world space coords)
															// When applyInLocalSpace is false, this will still be applied to the decal, but is optional. It might
															// be useful to specify a decal local to some event, and then provide a single transform to put it
															// in the world.

	Bool							m_projectOnlyOnStatic;  // Project only on static geometry
	ERenderDynamicDecalProjection	m_projectionMode;

	Float							m_timeToLive;			// How long the decal lives until it is automatically removed. Defaults to Infinite, but most users
															// should use something finite.

	EDynamicDecalSpawnPriority		m_spawnPriority;		// What is the decal spawning manner / usage (in-game, cutscene etc)

	Float							m_fadeTime;				// How long the decal takes to fade out as TTL expires. Should be lower than TTL.
	Float							m_fadeInTime;			// How long the decal takes to fade in
	Float							m_startScale;			// starting scale of decal (startScale * size)
	Float							m_scaleTime;			// how long it takes the decal to reach full size
	Float							m_autoHideDistance;		// Auto hide distance for dynamic decal
	Vector							m_atlasVector;			// Scale bias vector for decals UV (for random atlasing)

	SDynamicDecalInitInfo();

	Matrix GetWorldToDecalMatrix() const;
	Matrix GetDecalToWorldMatrix() const;
	Box GetWorldBounds() const;

	void SetMaterialInfo( const SDynamicDecalMaterialInfo& materialInfo );
	void SetAtlasVector( ERenderDynamicDecalAtlas atlasType );
	void SetAtlasVector( Uint32 scaleS, Uint32 scaleT, Uint32 index );

};
