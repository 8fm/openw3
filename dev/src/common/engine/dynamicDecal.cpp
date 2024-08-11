/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dynamicDecal.h"
#include "bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( SDynamicDecalMaterialInfo );
IMPLEMENT_RTTI_ENUM( ERenderDynamicDecalProjection );
IMPLEMENT_RTTI_ENUM( ERenderDynamicDecalAtlas );
IMPLEMENT_RTTI_ENUM( EDynamicDecalSpawnPriority );

SDynamicDecalInitInfo::SDynamicDecalInitInfo()
	: m_diffuseTexture( nullptr )
	, m_normalTexture( nullptr )
	, m_diffuseColor( Color::WHITE )
	, m_specularColor( Color::BLACK )
	, m_specularScale( 0.0f )
	, m_specularBase( 1.0f )
	, m_origin( 0, 0, 0, 0 )
	, m_dirFront( 0, 1, 0, 0 )
	, m_dirUp( 0, 0, 1, 0 )
	, m_width( 1 )
	, m_height( 1 )
	, m_nearZ( 0 )
	, m_farZ( 1 )
	, m_depthFadePower( 1.0f )
	, m_normalFadeBias( 0.0f )
	, m_normalFadeScale( 1.0f )
	, m_doubleSided( true )
	, m_additiveNormals( true )
	, m_applyInLocalSpace( false )
	, m_worldToDecalParent( Matrix::IDENTITY )
	, m_projectionMode( RDDP_Ortho )
	, m_timeToLive( NumericLimits< Float >::Infinity() )
	, m_fadeTime( 0.0f )
	, m_fadeInTime( 0.0f )
	, m_startScale( 1.0f )
	, m_scaleTime( 1.0f )
	, m_autoHideDistance( -1.f )							// -1 means hide distance will be taken from SRenderSettings
	, m_atlasVector( .5f , .5f , .5f, .5f )
	, m_specularity( -1.0f )
	, m_spawnPriority( EDynamicDecalSpawnPriority::RDDS_Normal )
{
}

Matrix SDynamicDecalInitInfo::GetWorldToDecalMatrix() const
{
	return GetDecalToWorldMatrix().FullInverted();
}

Matrix SDynamicDecalInitInfo::GetDecalToWorldMatrix() const
{
	// ( [-1,1], [0,1], [-1,1] ) local coordinates are inside the decal.
	// local Y axis is "forward" direction.

	// If other projection modes needed, will need to account for that here.

	const Vector dirRight = Vector::Cross( m_dirFront, m_dirUp );

	Vector vecRight	= dirRight   * m_width / 2;
	Vector vecUp	= m_dirUp    * m_height / 2;
	Vector vecFront	= m_dirFront * ( (m_farZ - m_nearZ) * 0.5f );
	Vector middle	= m_origin   + m_dirFront * ( ( m_farZ + m_nearZ ) * 0.5f );
	vecRight.W = 0;
	vecUp.W = 0;
	vecFront.W = 0;
	middle.W = 1;
	return Matrix( vecRight, vecFront, vecUp, middle );
}


Box SDynamicDecalInitInfo::GetWorldBounds() const
{
	// If other projection modes needed, will need to account for that here.

	const Vector dirRight   = Vector::Cross( m_dirFront, m_dirUp );

	const Vector vecRight	= dirRight * m_width;
	const Vector vecFront	= m_dirFront * (m_farZ - m_nearZ);
	const Vector vecUp		= m_dirUp    * m_height;
	const Vector corner		= m_origin  - (vecRight + vecUp) * 0.5f + m_dirFront * m_nearZ;
	const Vector corner1	= corner  + vecRight;
	const Vector corner2	= corner1 + vecUp;
	const Vector corner3	= corner  + vecUp;

	Box box( Box::RESET_STATE );
	box.AddPoint( corner  );
	box.AddPoint( corner1 );
	box.AddPoint( corner2 );
	box.AddPoint( corner3 );
	box.AddPoint( corner  + vecFront );
	box.AddPoint( corner1 + vecFront );
	box.AddPoint( corner2 + vecFront );
	box.AddPoint( corner3 + vecFront );
	return box;
}

void SDynamicDecalInitInfo::SetAtlasVector( ERenderDynamicDecalAtlas atlasType )
{
	static CStandardRand randomGenerator;

	Uint8 scaleS = 1;
	Uint8 scaleT = 1;

	switch( atlasType )
	{
	case RDDA_1x1 : 
		break;
	case RDDA_2x1 :
		scaleS = 2;
		break;
	case RDDA_2x2 :
		scaleS = 2;
		scaleT = 2;
		break;
	case RDDA_4x2 :
		scaleS = 4;
		scaleT = 2;
		break;
	case RDDA_4x4 :
		scaleS = 4;
		scaleT = 4;
		break;
	case RDDA_8x4 :
		scaleS = 8;
		scaleT = 4;
		break;
	default:
		RED_ASSERT( TXT("Undefined SubUV type in decal spawning") );
		break;
	}

	Uint8 randS = randomGenerator.Get<Uint8>();
	Uint8 randT = randomGenerator.Get<Uint8>();

	/*
		Scale-Bias from -1 .. +1 to atlas 0 .. +1
	*/

	m_atlasVector = Vector( 0.5f / scaleS, 0.5f / scaleT , static_cast<Float>( randS % scaleS ) , static_cast<Float>( randT % scaleT ) );
	m_atlasVector.Z = ( 1.0f / scaleS ) * m_atlasVector.Z + m_atlasVector.X;
	m_atlasVector.W = ( 1.0f / scaleT ) * m_atlasVector.W + m_atlasVector.Y;
}


void SDynamicDecalInitInfo::SetAtlasVector( Uint32 scaleS, Uint32 scaleT, Uint32 index )
{
	RED_ASSERT( scaleS >= 1 );
	RED_ASSERT( scaleT >= 1 );

	Uint8 randS = index % scaleS;
	Uint8 randT = index / scaleS;

	/*
		Scale-Bias from -1 .. +1 to atlas 0 .. +1
	*/

	m_atlasVector = Vector( 0.5f / scaleS, 0.5f / scaleT , static_cast<Float>( randS % scaleS ) , static_cast<Float>( randT % scaleT ) );
	m_atlasVector.Z = ( 1.0f / scaleS ) * m_atlasVector.Z + m_atlasVector.X;
	m_atlasVector.W = ( 1.0f / scaleT ) * m_atlasVector.W + m_atlasVector.Y;
}



void SDynamicDecalInitInfo::SetMaterialInfo( const SDynamicDecalMaterialInfo& materialInfo )
{
	static CStandardRand randomGenerator;

	m_diffuseTexture	= nullptr;
	m_normalTexture		= nullptr;

	if ( materialInfo.m_diffuseTexture != nullptr )
	{
		m_diffuseTexture = materialInfo.m_diffuseTexture->GetRenderResource();
	}
	if ( materialInfo.m_normalTexture != nullptr )
	{
		m_normalTexture = materialInfo.m_normalTexture->GetRenderResource();
	}
	{
		const Float randomValue = randomGenerator.Get<Float>();
		m_diffuseColor	= Color::Lerp( randomValue, materialInfo.m_diffuseRandomColor0 , materialInfo.m_diffuseRandomColor1 );
	}

	m_specularColor		= materialInfo.m_specularColor;
	m_specularScale		= materialInfo.m_specularScale;
	m_specularBase		= materialInfo.m_specularBase;
	m_specularity		= materialInfo.m_specularity;

	SetAtlasVector( materialInfo.m_subUVType );
}
