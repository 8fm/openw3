/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/namesRegistry.h"
#include "../core/engineTransform.h"

struct SSceneCameraShotDescription
{
	DECLARE_RTTI_STRUCT( SSceneCameraShotDescription );

public:
	CName	m_shotName;
	CName	m_animationName;
	Bool	m_overrideDof;
	Float	m_dofFocusDistFar;
	Float	m_dofBlurDistFar;
	Float	m_dofIntensity;
	Float	m_dofFocusDistNear;
	Float	m_dofBlurDistNear;

	SSceneCameraShotDescription()
		: m_shotName( CNAME( Default ) )
		, m_animationName( CName::NONE )
		, m_overrideDof( false )
		, m_dofFocusDistFar( 2.0f )
		, m_dofBlurDistFar( 20.0f )
		, m_dofIntensity(1.0f )
		, m_dofFocusDistNear( 1.0f )
		, m_dofBlurDistNear( 5.0f )
	{
	}
};

BEGIN_CLASS_RTTI( SSceneCameraShotDescription );
	PROPERTY_CUSTOM_EDIT( m_shotName, TXT( "Camera Shot Name" ), TXT( "DialogsetCameraShotName" ) );
	PROPERTY_RO( m_animationName, TXT( "Shot Animation" ) );
	PROPERTY_EDIT( m_overrideDof, TXT( "DOF override switch" ) );
	PROPERTY_EDIT( m_dofIntensity, TXT( "DOF intensity" ) );
	PROPERTY_EDIT( m_dofBlurDistNear, TXT( "DOF - distance from camera to start focussing" ) );
	PROPERTY_EDIT( m_dofFocusDistNear, TXT( "DOF - distance from camera to start focussed area" ) );
	PROPERTY_EDIT( m_dofFocusDistFar, TXT( "DOF - distance from camera to end focussed area" ) );
	PROPERTY_EDIT( m_dofBlurDistFar, TXT( "DOF - distance from camera to end focussing" ) );
	
	

END_CLASS_RTTI();

struct SScenePersonalCameraDescription
{
	DECLARE_RTTI_STRUCT( SScenePersonalCameraDescription );

public:
	CName	m_cameraName;
	Uint32	m_cameraNumber;
	Uint32	m_sourceSlot;
	Uint32	m_targetSlot;
	TDynArray< SSceneCameraShotDescription >	m_cameraShots;

	SScenePersonalCameraDescription() 
		: m_cameraName( CName::NONE )
		, m_cameraNumber( 0 )
		, m_sourceSlot( 0 )
		, m_targetSlot( 0 )
	{}
};

BEGIN_CLASS_RTTI( SScenePersonalCameraDescription );
PROPERTY( m_cameraName );
PROPERTY_RO( m_cameraNumber, TXT( "Camera number" ) );
PROPERTY_CUSTOM_EDIT( m_targetSlot, TXT( "Target slot number" ), TXT( "DialogsetCharacterNumber" ) );
PROPERTY_CUSTOM_EDIT( m_sourceSlot, TXT( "Source slot number" ), TXT( "DialogsetCharacterNumber" ) );
PROPERTY_EDIT( m_cameraShots, TXT( "Camera shots" ) );
END_CLASS_RTTI();


struct SSceneMasterCameraDescription
{
	DECLARE_RTTI_STRUCT( SSceneMasterCameraDescription );

public:
	CName	m_cameraName;
	Uint32	m_cameraNumber;
	TDynArray< SSceneCameraShotDescription >	m_cameraShots;

	SSceneMasterCameraDescription()
		: m_cameraName( CName::NONE )
		, m_cameraNumber( 0 )
	{
	}
};

BEGIN_CLASS_RTTI( SSceneMasterCameraDescription );
PROPERTY_EDIT( m_cameraName, TXT( "Camera name" ) );
PROPERTY_RO( m_cameraNumber, TXT( "Camera number" ) );
PROPERTY_EDIT( m_cameraShots, TXT( "Camera shots" ) );
END_CLASS_RTTI();


struct SSceneCustomCameraDescription
{
	DECLARE_RTTI_STRUCT( SSceneCustomCameraDescription );

public:
	CName				m_cameraName;
	EngineTransform		m_cameraTransform;
	Float				m_fov;

	SSceneCustomCameraDescription()
		: m_cameraName( CName::NONE )
		, m_fov( 0.0f )
	{
	}
};

BEGIN_CLASS_RTTI( SSceneCustomCameraDescription );
PROPERTY_EDIT( m_cameraName, TXT( "Camera name" ) );
PROPERTY_EDIT( m_fov, TXT( "Camera field of view" ) );
PROPERTY_EDIT( m_cameraTransform, TXT( "Camera transform" ) );
END_CLASS_RTTI();

