/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "enumBuilder.h"
#include "math.h"
#include "object.h"

class CThumbnail;

/// Thumbnail image
class IThumbnailImage
{
public:
	virtual ~IThumbnailImage() {};
	virtual Uint32 GetWidth() const=0;
	virtual Uint32 GetHeight() const=0;
};

enum EThumbnailFlags
{
	TF_UseGround		= FLAG(0),
	TF_UseDebugMaterial	= FLAG(1),
	TF_SetBackgroundColor = FLAG(2),
	TF_CopyEnvironment	= FLAG(3),
	TF_OutputIcon		= FLAG(4),
};

BEGIN_ENUM_RTTI( EThumbnailFlags )
	ENUM_OPTION( TF_UseGround )
	ENUM_OPTION( TF_UseDebugMaterial )
	ENUM_OPTION( TF_SetBackgroundColor )
	ENUM_OPTION( TF_CopyEnvironment )
END_ENUM_RTTI()

enum EThumbnailCustomSettings
{
	TCS_None           = 0,
	TCS_CameraRotation = FLAG(0),
	TCS_CameraPosition = FLAG(1),
	TCS_CameraFov      = FLAG(2),
	TCS_LightPosition  = FLAG(3),
	TCS_All = TCS_CameraRotation|TCS_CameraPosition|TCS_CameraFov|TCS_LightPosition
};

// Thumbnail settings
struct SThumbnailSettings
{
	SThumbnailSettings() 
		: m_flags( 0 ), m_customSettings( TCS_None ), m_lightPosition( 0.0 ), m_cameraFov( 70.0 )
		{}

	Vector		m_cameraPosition;
	EulerAngles	m_cameraRotation;
	Float		m_cameraFov;
	Float		m_lightPosition;
	Int32		m_customSettings;
	Bool		m_customCameraPosition;
	Int32		m_flags;
	String		m_environmentPath;
	Int32		m_width;
	Int32		m_height;
	Vector		m_backgroundColor;
	Bool		m_item;
	String		m_iconOutputPath;
};

/// Abstract thumbnail image loader
class IThumbnailImageLoader : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IThumbnailImageLoader, CObject );

public:
	// Load thumbnail image
	virtual IThumbnailImage* Load( const DataBuffer& data )=0;
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IThumbnailImageLoader, CObject );

/// Abstract thumbnail generator
class IThumbnailGenerator : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IThumbnailGenerator, CObject );

public:
	// Create thumbnail(s) from resource
	virtual TDynArray< CThumbnail* > Create( CResource* resource, const SThumbnailSettings& settings )=0;
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IThumbnailGenerator, CObject );

/// Resource thumbnail data
class CThumbnail : public CObject
{
	DECLARE_ENGINE_CLASS( CThumbnail, CObject, 0 );

protected:
	String					m_name;				//!< Thumbnail name
	TDynArray< String >		m_info;				//!< Extra info
	DataBuffer				m_imageData;		//!< Preview image data
	IThumbnailImage*		m_imageHandle;		//!< Preview image handle
	Vector					m_cameraPosition;	//!< Preview camera position
	EulerAngles				m_cameraRotation;	//!< Preview camera rotation
	Float					m_cameraFov;		//!< Preview camera FOV
	EulerAngles				m_sunRotation;		//!< Preview sun rotation
	Int32					m_flags;			//!< Preview flags

public:
	//! Get thumbnail name
	RED_INLINE const String& GetName() const { return m_name; }

	// Get info string
	RED_INLINE const TDynArray< String >& GetInfo() const { return m_info; }

	// Get internal data buffer
	RED_INLINE const DataBuffer& GetData() const { return m_imageData; }

	// Get thumbnail image handle
	RED_INLINE const IThumbnailImage* GetImage() const { return m_imageHandle; }

	// Get preview camera position
	RED_INLINE const Vector& GetCameraPosition() const { return m_cameraPosition; }

	// Get preview camera rotation
	RED_INLINE const EulerAngles& GetCameraRotation() const { return m_cameraRotation; }

	// Get preview FOV
	RED_INLINE Float GetCameraFov() const { return m_cameraFov; }

	// Get preview sun rotation
	RED_INLINE const EulerAngles& GetSunRotation() const { return m_sunRotation; }

	// Set preview camera position
	RED_INLINE void SetCameraPosition( const Vector& position ) { m_cameraPosition = position; }

	// Set preview camera rotation
	RED_INLINE void SetCameraRotation( const EulerAngles& rotation ) { m_cameraRotation = rotation; }

	// Set preview camera FOV
	RED_INLINE void SetCameraFov( Float fov ) { m_cameraFov = fov; }

	// Set preview sun rotation
	RED_INLINE void SetSunRotation( const EulerAngles& rotation) { m_sunRotation = rotation; }

	// Name this thumbnail
	RED_INLINE void SetName( const String &name ) { m_name = name; }

	// Set the flags
	RED_INLINE void SetFlags( Int32 flags ) { m_flags = flags; }

	// Get the flags
	RED_INLINE Int32 GetFlags() const { return m_flags; }

public:
	CThumbnail();
	~CThumbnail();

	// Create thumbnail from raw image data
	CThumbnail( const void* data, const Uint32 size );

	// Serialization
	virtual void OnSerialize( IFile& file );

	// Loaded from file
	virtual void OnPostLoad();

protected:
	// Discard image
	void DiscardImage();

	// Recreate thumbnail image
	void RecreateImage();

public:
	//! Discard all thumbnails
	static void DiscardThumbnailImages();
};

BEGIN_CLASS_RTTI( CThumbnail );
	PARENT_CLASS( CObject );
	PROPERTY( m_name );
	PROPERTY( m_info );
	PROPERTY( m_cameraPosition );
	PROPERTY( m_cameraRotation );
	PROPERTY( m_cameraFov );
	PROPERTY( m_sunRotation );
	PROPERTY( m_flags );
END_CLASS_RTTI();
