/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneCameraDefinition.h"
#include "storySceneDialogset.h"

IMPLEMENT_RTTI_ENUM( ECameraPlane )
IMPLEMENT_ENGINE_CLASS( ApertureDofParams )
IMPLEMENT_ENGINE_CLASS( StorySceneCameraDefinition )
IMPLEMENT_ENGINE_CLASS( CEventGeneratorCameraParams )


const Float				ApertureDofParams::CIRCLE_OF_CONFUSION	= 0.00003f;
const Float				ApertureDofParams::DEFAULT_INTENSITY	= 1.0f;
const EApertureValue	ApertureDofParams::DEFAULT_APERTURE		= APERTURE_5_6;
Int32		   StorySceneCameraDefinition::m_keyCounter			= 0;


ApertureDofParams::ApertureDofParams()
	: m_aperture( DEFAULT_APERTURE )
	, m_distance( 2.0f )
	, m_focalLength( 0.5f )
	, m_enabled( false )
{

}

void ApertureDofParams::ToEngineDofParams( SDofParams& dofParams ) const
{
	Float focalInMeters = m_focalLength * 0.001f;
	Float aperture = ::pow( 1.4142f, m_aperture );
	
	Float hyperfocalDistance = ( focalInMeters * focalInMeters )/ ( aperture * CIRCLE_OF_CONFUSION );

	dofParams.dofFocusDistNear = ( hyperfocalDistance * m_distance ) / ( hyperfocalDistance + m_distance );
	if ( m_distance < hyperfocalDistance )
	{
		dofParams.dofFocusDistFar = ( hyperfocalDistance * m_distance ) / ( hyperfocalDistance - m_distance );
	}
	else
	{
		dofParams.dofFocusDistFar = 1000.0f * focalInMeters;
	}

	dofParams.dofIntensity = DEFAULT_INTENSITY;
	dofParams.dofBlurDistNear = 0.0f;
	dofParams.dofBlurDistFar = dofParams.dofFocusDistFar + dofParams.dofFocusDistNear;
}

void ApertureDofParams::FromEngineDofParams( const SDofParams& dofParams )
{
	FromEngineDofParams( dofParams.dofFocusDistNear, dofParams.dofFocusDistFar );
}

void ApertureDofParams::FromEngineDofParams( Float dofNear, Float dofFar )
{
	if ( dofFar < dofNear )
	{
		dofNear = dofFar;
		dofFar = 1000.0f * dofNear;
	}

	m_aperture = DEFAULT_APERTURE;
	m_distance = dofNear * ( ( dofFar - dofNear ) / ( dofFar + dofNear ) + 1 );

	Float hyperfocalDistance = dofNear * ( ( dofFar + dofNear ) / ( dofFar - dofNear ) + 1 );
	Float aperture = ::pow( 1.4142f, m_aperture );

	m_focalLength = ::sqrt( hyperfocalDistance * aperture * CIRCLE_OF_CONFUSION ) * 1000.0f;
}

Bool ApertureDofParams::IsDefault() const
{
	ApertureDofParams defaultDof;
	return m_enabled == defaultDof.m_enabled && m_aperture == defaultDof.m_aperture && m_focalLength == defaultDof.m_focalLength && m_distance == defaultDof.m_distance;
}

Bool StorySceneCameraDefinition::ParseCameraParams()
{
	TDynArray<String> tokens;
	Int32 srcSlotNr = 0;
	Int32 dstSlotNr = 0;
	m_cameraName.AsString().GetTokens( '_', false, tokens );
	Bool result = true;

	if( tokens.Size() > 0 && FromString( tokens[0], srcSlotNr ) ) 
	{
		tokens.RemoveAt(0);
	}
	if( tokens.Size() > 0 && FromString( tokens[0], dstSlotNr ) )
	{
		tokens.RemoveAt(0) ;
	}
	if( tokens.Size() > 0 && FromString( tokens[0], m_genParam.m_cameraPlane ) ) 
	{
		tokens.RemoveAt(0);
	}
	else
	{
		result = false;
	}

	for( Uint32 i = 0; i < tokens.Size(); ++i )
	{
		m_genParam.m_tags.AddTag( CName( tokens[i] ) );
	}

	m_genParam.m_targetSlot = srcSlotNr;
	m_genParam.m_sourceSlot = dstSlotNr;

	return result;
}

/*
Cctor.
*/
StorySceneCameraDefinition::StorySceneCameraDefinition( const StorySceneCameraDefinition& other )
: m_cameraName( other.m_cameraName )
, m_cameraTransform( other.m_cameraTransform )
, m_cameraZoom( other.m_cameraZoom )
, m_cameraFov( other.m_cameraFov )
, m_dofFocusDistFar( other.m_dofFocusDistFar )
, m_dofBlurDistFar( other.m_dofBlurDistFar )
, m_dofIntensity( other.m_dofIntensity )
, m_dofFocusDistNear( other.m_dofFocusDistNear )
, m_dofBlurDistNear( other.m_dofBlurDistNear )
, m_cameraUniqueID( m_keyCounter++ )
, m_enableCameraNoise( other.m_enableCameraNoise )
, m_sourceSlotName( other.m_sourceSlotName )
, m_targetSlotName( other.m_targetSlotName )
, m_targetEyesLS( other.m_targetEyesLS )
, m_sourceEyesHeigth( other.m_sourceEyesHeigth )
, m_bokehDofParams( other.m_bokehDofParams )
, m_dof( other.m_dof )
, m_genParam( other.m_genParam )
, m_cameraAdjustVersion( other.m_cameraAdjustVersion )
{}
