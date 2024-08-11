/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/tagList.h"
#include "../engine/cameraDirector.h"

struct SDofParams;

struct ApertureDofParams
{
	DECLARE_RTTI_STRUCT( ApertureDofParams )

	static const Float			CIRCLE_OF_CONFUSION;
	static const Float			DEFAULT_INTENSITY;
	static const EApertureValue	DEFAULT_APERTURE;

	EApertureValue				m_aperture;
	Float						m_focalLength;
	Float						m_distance;

	Bool						m_enabled;

	ApertureDofParams();

	void ToEngineDofParams( SDofParams& dofParams ) const;
	void FromEngineDofParams( const SDofParams& dofParams );
	void FromEngineDofParams( Float dofNear, Float dofFar );

	Bool IsDefault() const;
};

BEGIN_CLASS_RTTI( ApertureDofParams )
	PROPERTY_EDIT( m_aperture, TXT( "Aperture value" ) );
	PROPERTY_EDIT_RANGE( m_focalLength, TXT( "Focal length [mm]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_distance, TXT( "Focus distance [m]" ), 0.0f, 1000.0f );
	PROPERTY_EDIT( m_enabled, TXT( "Temp check" ) );
END_CLASS_RTTI();



enum ECameraPlane
{
	CP_None,
	CP_Wide, 
	CP_Medium, 
	CP_Semicloseup, 
	CP_Closeup, 
	CP_Supercloseup,
	CP_Any,
	CP_Size
};

BEGIN_ENUM_RTTI( ECameraPlane )
	ENUM_OPTION_DESC( TXT( "None" ), CP_None )
	ENUM_OPTION_DESC( TXT( "Wide" ), CP_Wide )
	ENUM_OPTION_DESC( TXT( "Medium" ), CP_Medium )
	ENUM_OPTION_DESC( TXT( "Semicloseup" ), CP_Semicloseup )
	ENUM_OPTION_DESC( TXT( "Closeup" ), CP_Closeup )
	ENUM_OPTION_DESC( TXT( "Supercloseup" ), CP_Supercloseup )
END_ENUM_RTTI();

template <>
RED_INLINE Bool FromString( const String& text, ECameraPlane& value )
{
	String copy(text);
	copy.ToLower();
	if( copy == TXT("wide") || copy == TXT("cp_wide") )
	{
		value = CP_Wide;
		return true;
	}
	else if( copy == TXT("medium") || copy == TXT("cp_medium") )
	{
		value = CP_Medium;
		return true;
	}
	else if( copy == TXT("semicloseup") || copy == TXT("cp_semicloseup") )
	{
		value = CP_Semicloseup;
		return true;
	}
	else if( copy == TXT("closeup") || copy == TXT("cp_closeup") )
	{
		value = CP_Closeup;
		return true;
	}
	else if( copy == TXT("supercloseup") || copy == TXT("cp_supercloseup"))
	{
		value = CP_Supercloseup;
		return true;
	}
	return false;
}


struct CEventGeneratorCameraParams
{
	DECLARE_RTTI_STRUCT( CEventGeneratorCameraParams );

public:
	CEventGeneratorCameraParams()
		: m_usableForGenerator( true )
		, m_targetSlot(0)
		, m_sourceSlot(0)
	{}
	
	Bool				m_usableForGenerator;
	ECameraPlane		m_cameraPlane;
	TagList				m_tags;
	Int32				m_targetSlot;
	Int32				m_sourceSlot;
};
BEGIN_CLASS_RTTI( CEventGeneratorCameraParams );
	PROPERTY_RO( m_cameraPlane, TXT( "Camera plane" ) );
	PROPERTY_RO( m_tags, TXT( "Camera tags" ) );
	PROPERTY_RO( m_targetSlot, TXT( "speaking actor" ) )
	PROPERTY_RO( m_sourceSlot, TXT( "listening actor" ) )
	PROPERTY_EDIT( m_usableForGenerator, TXT( "Can camera be used by automatic event generation" ) );
END_CLASS_RTTI();

RED_WARNING_PUSH();
RED_DISABLE_WARNING_MSC( 4324 );

struct StorySceneCameraDefinition
{
	DECLARE_RTTI_STRUCT( StorySceneCameraDefinition )

	CName				m_cameraName;
	EngineTransform		m_cameraTransform;
	Float				m_cameraZoom;
	Float				m_cameraFov;
	Float				m_dofFocusDistFar;
	Float				m_dofBlurDistFar;
	Float				m_dofIntensity;
	Float				m_dofFocusDistNear;
	Float				m_dofBlurDistNear;
	Int32				m_cameraUniqueID;
	Bool				m_enableCameraNoise;
	CName				m_sourceSlotName;
	CName				m_targetSlotName;
	Vector				m_targetEyesLS;
	Float				m_sourceEyesHeigth;
	SBokehDofParams		m_bokehDofParams;	
	ApertureDofParams	m_dof;
	CEventGeneratorCameraParams m_genParam;
	Uint8				m_cameraAdjustVersion;

	StorySceneCameraDefinition() 
		: m_cameraZoom( 0.f )
		, m_cameraFov( 50.f )
		, m_dofFocusDistFar( 0.f )
		, m_dofBlurDistFar( 0.f )
		, m_dofIntensity( 0.f )
		, m_dofFocusDistNear( 0.f )
		, m_dofBlurDistNear( 0.f )
		, m_enableCameraNoise( true )
		, m_cameraUniqueID( m_keyCounter++ )
		, m_sourceEyesHeigth( 0.f )
		, m_targetEyesLS( Vector::ZEROS )
		, m_cameraAdjustVersion( 0 )
	{
	}

	StorySceneCameraDefinition( const StorySceneCameraDefinition& other );

	Bool ParseCameraParams();

	static Int32 m_keyCounter;
};

RED_WARNING_POP();

BEGIN_CLASS_RTTI( StorySceneCameraDefinition );
	PROPERTY_EDIT( m_cameraName, TXT( "Camera name" ) );
	PROPERTY_EDIT( m_cameraTransform, TXT( "Camera placement" ) );
	PROPERTY_EDIT( m_cameraZoom, TXT( "Camera zoom value" ) );
	PROPERTY_EDIT( m_cameraFov, TXT( "Camera FOV value" ) );
	PROPERTY_EDIT( m_enableCameraNoise, TXT( "Allow camera noise movement" ) );
	PROPERTY_EDIT( m_dofFocusDistFar, TXT( "DOF focus distance far" ) );
	PROPERTY_EDIT( m_dofBlurDistFar, TXT( "DOF blur distance far" ) );
	PROPERTY_EDIT( m_dofIntensity, TXT( "DOF intensity value" ) );
	PROPERTY_EDIT( m_dofFocusDistNear, TXT( "DOF focus distance near" ) );
	PROPERTY_EDIT( m_dofBlurDistNear, TXT( "DOF blur distance near" ) );
	PROPERTY_CUSTOM_EDIT( m_sourceSlotName, TXT( "Slot name for camera offset source" ), TXT( "CameraDefinitionDialogsetSlot" ) );
	PROPERTY_CUSTOM_EDIT( m_targetSlotName, TXT( "Slot name for camera offset target" ), TXT( "CameraDefinitionDialogsetSlot" ) ); 	
	PROPERTY_RO( m_sourceEyesHeigth, TXT("") );
	PROPERTY_RO( m_targetEyesLS, TXT("") );
	PROPERTY_INLINED( m_dof, TXT( "Depth of field parameters" ) );
	PROPERTY_INLINED( m_bokehDofParams, TXT( "Depth of field parameters" ) );
	PROPERTY_RO( m_genParam, TXT( "Parameters for event generator" ) );
	PROPERTY( m_cameraAdjustVersion );
END_CLASS_RTTI();
