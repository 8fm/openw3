
#pragma once

#include "../core/object.h"
#include "../core/frustum.h"
#include "renderCamera.h"

class IViewport;
class CNode;
class CRenderFrame;

#define DEBUG_CAMERA_DIRECTOR

//////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_CAMERA_DIRECTOR
#define RED_CAMERA_ASSERT( x ) RED_ASSERT( x )
#else
#pragma optimize("",off)
void CameraDirectorAssertFunc( const Char* msgFile, const Uint32 lineNum, const Char* msg );
#define RED_CAMERA_ASSERT( expression, ... ) if ( !( expression ) ) CameraDirectorAssertFunc( MACRO_TXT( __FILE__ ), __LINE__, TXT( #expression ) );
#endif

//////////////////////////////////////////////////////////////////////////

enum EApertureValue
{
	APERTURE_1_0 = 0,
	APERTURE_1_4,
	APERTURE_2_0,
	APERTURE_2_8,
	APERTURE_4_0,
	APERTURE_5_6,
	APERTURE_8_0,
	APERTURE_11_0,
	APERTURE_16_0,
	APERTURE_22_0,
	APERTURE_32_0
};

BEGIN_ENUM_RTTI( EApertureValue )
	ENUM_OPTION_DESC( TXT( "f/1.0" ), APERTURE_1_0 )
	ENUM_OPTION_DESC( TXT( "f/1.4" ), APERTURE_1_4 )
	ENUM_OPTION_DESC( TXT( "f/2.0" ), APERTURE_2_0 )
	ENUM_OPTION_DESC( TXT( "f/2.8" ), APERTURE_2_8 )
	ENUM_OPTION_DESC( TXT( "f/4.0" ), APERTURE_4_0 )
	ENUM_OPTION_DESC( TXT( "f/5.6" ), APERTURE_5_6 )
	ENUM_OPTION_DESC( TXT( "f/8.0" ), APERTURE_8_0 )
	ENUM_OPTION_DESC( TXT( "f/11.0" ), APERTURE_11_0 )
	ENUM_OPTION_DESC( TXT( "f/16.0" ), APERTURE_16_0 )
	ENUM_OPTION_DESC( TXT( "f/22.0" ), APERTURE_22_0 )
	ENUM_OPTION_DESC( TXT( "f/32.0" ), APERTURE_32_0 )
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SDofParams
{
	SDofParams ()
	{
		Reset();
	}

	void Reset()
	{
		overrideFactor		= 0.f;
		dofIntensity		= 1.f;
		dofBlurDistNear		= 0.f;
		dofFocusDistNear	= 5.f;
		dofFocusDistFar		= 10.f;
		dofBlurDistFar		= 20.f;
	}
	Float overrideFactor;
	Float dofIntensity;
	Float dofBlurDistNear;
	Float dofFocusDistNear;
	Float dofFocusDistFar;
	Float dofBlurDistFar;
};

//////////////////////////////////////////////////////////////////////////

struct SBokehDofParams
{
	DECLARE_RTTI_STRUCT( SBokehDofParams );


	Float					m_planeInFocus;
	EApertureValue			m_fStops;
	Float					m_bokehSizeMuliplier;
	Float					m_hexToCircleScale;
	Bool					m_enabled;
	Bool					m_usePhysicalSetup;


	SBokehDofParams ()
	{
		Reset();
	}

	void Reset()
	{
		m_enabled = false;
		m_usePhysicalSetup = false;
		m_planeInFocus = 3.0f;
		m_fStops = APERTURE_4_0;
		m_bokehSizeMuliplier = 1.0f;
		m_hexToCircleScale = 1.0f;
	}

	Float PupilRelativeSize() const
	{
		switch ( m_fStops )
		{
		case APERTURE_1_0  : return 1.0f;
		case APERTURE_1_4  : return 0.7142f;
		case APERTURE_2_0  : return 0.5f;
		case APERTURE_2_8  : return 0.3571f;
		case APERTURE_4_0  : return 0.25f;
		case APERTURE_5_6  : return 0.1785f;
		case APERTURE_8_0  : return	0.125f;
		case APERTURE_11_0 : return	0.0909f;
		case APERTURE_16_0 : return	0.0625f;
		case APERTURE_22_0 : return	0.0454f;
		case APERTURE_32_0 : return 0.0312f;
		default: return 1.0f;
		}
	}

	const Float PupilSize() const
	{
		return 2.0f * m_bokehSizeMuliplier * PupilRelativeSize(); 
		//0.05 f - default maximum pupil diameter
	}
};

BEGIN_CLASS_RTTI( SBokehDofParams )
	PROPERTY_EDIT( m_enabled, TXT( "Is effect enabled - if not old dof is used" ));
	PROPERTY_EDIT_RANGE( m_hexToCircleScale, TXT( "Distance[m] between camera and focus point" ), 0.0f, 1.0f );
	PROPERTY_EDIT( m_usePhysicalSetup, TXT( "Use this physical based setups instead of near/far distances" ));
	PROPERTY_EDIT_RANGE( m_planeInFocus, TXT( "Distance[m] between camera and focus point" ), 0.0f, 8000.0f );
	PROPERTY_EDIT( m_fStops, TXT( "F-Stops" ));
	PROPERTY_EDIT_RANGE( m_bokehSizeMuliplier, TXT( "Additional value to tweak the intensity of DOF" ), 0.0f, 2.0f );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SCameraVisibilityData
{
private:
	Bool		m_isValid;
	CFrustum	m_frustum;

public:
	SCameraVisibilityData() : m_isValid( false ) {}

	RED_INLINE void Set( const CFrustum& frustum )
	{
		m_isValid = true;
		m_frustum = frustum;
	}

	RED_INLINE void Reset()
	{
		m_isValid = false;
	}

	RED_INLINE Bool IsValid()
	{
		return m_isValid;
	}

	RED_INLINE Uint32 TestBox( const Box &box ) const
	{
		RED_FATAL_ASSERT( m_isValid, "TestBox func is called on invalid object" );
		return m_frustum.TestBox( box );
	}

	RED_INLINE Bool ContainsBox( const Box &box ) const
	{
		RED_FATAL_ASSERT( m_isValid, "TestBox func is called on invalid object" );
		return m_frustum.TestBox( box ) == FR_Inside;
	}

	RED_INLINE Bool IntersectsBox( const Box &box ) const
	{
		RED_FATAL_ASSERT( m_isValid, "TestBox func is called on invalid object" );
		return m_frustum.TestBox( box ) == FR_Intersecting;
	}

	RED_INLINE Bool IsBoxOutside( const Box &box ) const
	{
		RED_FATAL_ASSERT( m_isValid, "TestBox func is called on invalid object" );
		return m_frustum.TestBox( box ) == FR_Outside;
	}
};

//////////////////////////////////////////////////////////////////////////

class ICamera
{
public:
	struct Data
	{
		Vector				m_position;
		EulerAngles			m_rotation;

		Bool				m_hasFocus;
		Vector				m_focus;

		Float				m_fov; 
		Float				m_nearPlane;
		Float				m_farPlane;

		Bool				m_forceNearPlane;
		Bool				m_forceFarPlane;

		SDofParams			m_dofParams;
		SBokehDofParams		m_bokehDofParams;

		void Reset();
	};

	virtual ~ICamera(){}
	virtual Bool Update( Float timeDelta ) = 0;
	virtual Bool GetData( Data& outData ) const = 0;
	virtual Bool SetData( const Data& data ) { return false; }
    
    virtual Bool IsManualControlledHor() const { return false;}
    virtual Bool IsManualControlledVer() const { return false;}

	virtual void ResetCamera() {}
	virtual void OnActivate( const IScriptable* /*prevCameraObject*/ , Bool /*resetCamera*/ ) {}
};

//////////////////////////////////////////////////////////////////////////

class CCameraProxy
{
	friend class CCameraDirector;

private:
	ICamera*				m_camera;
	THandle< IScriptable >	m_parent;

	Float				m_blendTime;		// Blend time is always the time of a full blend from 0.0 to 1.0 weight regardless of the 'm_desiredWeight'
	Float				m_blendTimeElapsed;
	Float				m_blendStartWeight;
	Float				m_desiredWeight;

	Float				m_weight;
	Bool				m_abandoned;

public:
	RED_INLINE CCameraProxy( ICamera* camera, THandle<IScriptable> parent, Float blend = 0.f, Float weight = 1.f )
		: m_camera( camera ), m_parent( parent ), m_blendTime( blend ), m_blendTimeElapsed( 0.0f ), m_blendStartWeight( 0.0f ), m_desiredWeight( weight )
		, m_weight( 0.f ), m_abandoned( false )
	{
		RED_ASSERT( m_camera );
		RED_ASSERT( m_desiredWeight >= 0.f && m_desiredWeight <= 1.f );
	}

	RED_INLINE Bool		IsValid() const		{ return m_parent.Get() != NULL; }

	RED_INLINE ICamera*	GetCamera()			{ return m_camera; }
	RED_INLINE const ICamera* GetCamera() const { return m_camera; }

	RED_INLINE const THandle< IScriptable >& GetParent()	{ return m_parent; }

	RED_INLINE Float		GetWeight() const	{ return m_weight; }
	RED_INLINE void		SetWeight( Float weight )
	{
		RED_ASSERT( weight >= 0.f && weight <= 1.f );
		m_weight = weight;
	}

	RED_INLINE void		SetBlendTime( Float blend ) {  }
	RED_INLINE void		StartBlending( Float desiredWeight, Float blendTime )
	{
		RED_ASSERT( desiredWeight >= 0.f && desiredWeight <= 1.f );
		m_blendTime = blendTime;
		m_desiredWeight = desiredWeight;
		m_blendTimeElapsed = 0.0f;
		m_blendStartWeight = m_weight;
	}

	RED_INLINE Bool		IsAbandoned() const	{ return m_abandoned; }
	RED_INLINE void		Abandon()			{ m_abandoned = true; }

	void UpdateWeight( Float timeDelta );
};

//////////////////////////////////////////////////////////////////////////
/// @n	Brief description of CameraDirector idea:
///		management of cameras is based on stack:
///		new camera is added always on top of stack, its weight should be between 0-1
///		cameras from bottom are popped (blended) untill total weight of the stack is 1
class CCameraDirector : public CObject
{
	DECLARE_ENGINE_CLASS( CCameraDirector, CObject, 0 )

	struct BlendCameraInfo
	{
		ICamera*		m_camera;
		ICamera::Data	m_data;
		Float			m_weight;
		Float			m_blendProgress;
	};

protected:
	TDynArray<CCameraProxy>		m_cameras;

	ICamera::Data				m_cachedData;
	CRenderCamera				m_cachedRenderCamera;
	Bool						m_isCachedRenderCameraValid;
	SRenderCameraLastFrameData	m_lastFrameCamera;
	Matrix						m_cachedTransform;

#ifdef DEBUG_CAM_ASAP
	mutable CRenderCamera		m_cachedDebugVisRenderCam;
#endif

	Uint32						m_viewportWidth;
	Uint32						m_viewportHeight;
	Float						m_fovDistanceMultiplier;

	// State of the focused blend (i.e. blend that is focused on a specific - movable - focus point)
	struct FocusedBlend
	{
		ICamera*					m_focusCamera;	// If set, indicates focused blend is active
		Bool						m_justStarted;	// Has focused blend just started
		EulerAngles					m_startAngles;	// Start/from blend angles
		EulerAngles					m_endAngles;	// End/to blend angles
		FocusedBlend() : m_focusCamera( nullptr ) {}
	};

	FocusedBlend				m_focusedBlend;
	Bool						m_isCameraResetDisabled;

	Bool						m_invalidateLastFrameCamera;
	Bool						m_cameraInvalidated;

	Float						m_inputWeight; // Camera input weight (0 - no input; 1 - full control)

#ifndef NO_EDITOR
	// Overwrite fov for screen shot editor
	Float						m_fovOverwrite;
#endif

public:
	CCameraDirector();
	virtual ~CCameraDirector();

	virtual void Update( Float timeDelta );
	virtual void CacheCameraData();

	/// If the camera is abandoned, then even if parent of the camera is not used anywhere else,
	/// is is not removed (the handle is kept by director). On the other hand, if it is not abandoned,
	/// and parent is removed, then camera is popped out from camera director. 
	void AbandonCamera( ICamera* camera );
	void AbandonTopmostCamera();

	/// adds new camera on top of the stack of cameras
	/// memory management is not performed by the director unless the camera is abandoned
	/// @see AbandonCamera
	/// @param camera the camera to be added on top of the stack
	/// @param parent the object the camera is attached to
	/// @param useFocusTarget if set to true final camera transformation is calculated using provided camera's focus target (only during blend)
	/// blend - time of blend
	void ActivateCamera( ICamera* camera, const THandle< IScriptable >& parent, Float blend = 0.f, Bool useFocusTarget = false, Bool resetCamera = true );
	Bool IsCameraActive( const ICamera* camera ) const;

	/// @return true if any camera was added and is being used.
	Bool IsAnyCameraActive() const;

	// Invalidate last frame camera on next viewport update
	RED_FORCE_INLINE void InvalidateLastFrameCamera() { m_invalidateLastFrameCamera = true; }
	RED_FORCE_INLINE void MarkCachedRenderCameraInvalid() { m_isCachedRenderCameraValid = false; }

	// Gets whether camera reset is disabled
	RED_FORCE_INLINE void SetCameraResetDisabled( Bool disabled ) { m_isCameraResetDisabled = disabled; }
	RED_FORCE_INLINE Bool IsCameraResetDisabled() const { return m_isCameraResetDisabled; }
	
	// Clears dirty flags, and one-time setters
	void ClearLastFrameDataInvalidation();

	// Call this only on viewport request, it will cache previous camera data
	void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

	// Setup camera
	RED_INLINE void OnSetupCamera( CRenderCamera& camera, Uint32 viewportWidth, Uint32 viewportHeight ) const
	{
		CalcCamera( camera, viewportWidth, viewportHeight, m_cachedData.m_position, m_cachedData.m_rotation, GetFov(), 0.0f, m_cachedData.m_nearPlane, m_cachedData.m_farPlane, m_cachedData.m_forceNearPlane, m_cachedData.m_forceFarPlane );
		camera.SetLastFrameData( m_lastFrameCamera );
	}

	void OnSetupCamera( CRenderCamera& camera, IViewport* viewport ) const;

	RED_INLINE void OnSetupCamera( CRenderCamera& camera, const Vector& position, const EulerAngles& rotation ) const
	{
		CalcCamera( camera, m_viewportWidth, m_viewportHeight, position, rotation, GetFov(), 0.0f, m_cachedData.m_nearPlane, m_cachedData.m_farPlane, m_cachedData.m_forceNearPlane, m_cachedData.m_forceFarPlane );
		camera.SetLastFrameData( m_lastFrameCamera );
	}

	static void ViewCoordsToWorldVector( const Vector& camPos, const EulerAngles& camRot, Float fov, Int32 x, Int32 y, Int32 width, Int32 height, Float nearPlane, Float farPlane, Vector& outRayStart, Vector& outRayDirection );
	static void WorldVectorToViewCoords( const Vector& point, const Vector& camPos, const EulerAngles& camRot, Float fov, Int32 width, Int32 height, Float nearPlane, Float farPlane, Int32& outX, Int32& outY );
	static Bool WorldVectorToViewCoords( const Vector& point, const Vector& camPos, const EulerAngles& camRot, Float fov, Int32 width, Int32 height, Float nearPlane, Float farPlane, Float& outXSS, Float& outYSS );

	void ViewCoordsToWorldVector( Int32 x, Int32 y, Vector & outRayStart, Vector & outRayDirection ) const;
	void WorldVectorToViewCoords( const Vector& worldPos, Int32& x, Int32& y ) const;
	Bool WorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y ) const;
	Bool TestWorldVectorToViewRatio( const Vector& worldPos, Float& x, Float& y, const Vector& cameraPos, const EulerAngles& cameraRot ) const;

	Bool IsPointInView( const Vector& point, Float fovMultipier = 1.0f ) const;
	static Bool IsPointInView( const Vector& point, const Matrix& camera, Float fov, Uint32 viewportWidth, Uint32 viewportHeight );

	Vector ProjectPoint( const Vector& worldSpacePoint, const CRenderCamera& renderCamera ) const;
	Vector UnprojectPoint( const Vector& screenSpacePoint, const CRenderCamera& renderCamera ) const;
	RED_INLINE Vector ProjectPoint( const Vector& worldSpacePoint ) const
	{
		RED_CAMERA_ASSERT( m_isCachedRenderCameraValid );
		return ProjectPoint( worldSpacePoint, m_cachedRenderCamera );
	}
	RED_INLINE Vector UnprojectPoint( const Vector& screenSpacePoint ) const
	{
		RED_CAMERA_ASSERT( m_isCachedRenderCameraValid );
		return UnprojectPoint( screenSpacePoint, m_cachedRenderCamera );
	}

	//! Get node angle in camera space
	Float GetNodeAngleInCameraSpace( const CNode* node ) const;

	RED_INLINE void CalcCameraFrustum( CFrustum& frustum ) const
	{
		RED_CAMERA_ASSERT( m_isCachedRenderCameraValid );
		frustum = CFrustum( m_cachedRenderCamera.GetWorldToScreen() );
	}

	RED_INLINE void SetCachedRenderCamera( const CRenderCamera& camera )
	{
		m_cachedRenderCamera = camera;
		m_isCachedRenderCameraValid = true;
	}

	void CalcVisibilityDataAndCacheRenderCamera( SCameraVisibilityData& data );

	RED_INLINE const ICamera::Data&	GetCameraData() const		{ return m_cachedData; }
	RED_INLINE const Vector&		GetCameraPosition() const	{ return m_cachedData.m_position; }
	RED_INLINE const EulerAngles&	GetCameraRotation() const	{ return m_cachedData.m_rotation; }
	RED_INLINE Vector				GetCameraForward() const	{ return m_cachedTransform.GetAxisY(); }
	RED_INLINE Vector				GetCameraRight() const		{ return m_cachedTransform.GetAxisX(); }
	RED_INLINE Vector				GetCameraUp() const			{ return m_cachedTransform.GetAxisZ(); }
	RED_INLINE const Matrix&		GetCameraTransform() const	{ return m_cachedTransform; }
	RED_INLINE Uint32				GetViewportWidth() const	{ return m_viewportWidth; }
	RED_INLINE Uint32				GetViewportHeight() const	{ return m_viewportHeight; }

	RED_INLINE const SDofParams&	GetDofParams() const		{ return m_cachedData.m_dofParams; }
	RED_INLINE const SBokehDofParams& GetBokehDofParams() const	{ return m_cachedData.m_bokehDofParams; }
	RED_INLINE Uint32 GetNumCameras() const { return m_cameras.Size(); }

	void GetCameraProxiesForDebugOnly( TDynArray< const CCameraProxy* >& outProxies ) const;

#ifndef NO_EDITOR
	// Overwrite fov if 'fov != 0'
	RED_INLINE void				OverwriteFOV( Float fov )	{ m_fovOverwrite = fov; }
	RED_INLINE Float				GetFov() const				{ return m_fovOverwrite > 0.f ? m_fovOverwrite : m_cachedData.m_fov; }
#else
	RED_INLINE Float				GetFov() const				{ return m_cachedData.m_fov; }
#endif
	Float							GetFovDistanceMultiplier() const;

	RED_INLINE const THandle< IScriptable >& GetTopmostCameraObject()	{ return m_cameras.Back().GetParent(); }
	RED_INLINE const ICamera*		GetTopmostCamera() const	{ return m_cameras.Back().GetCamera(); }

	void ResetCameraData();

	//////////////////////////////////////////////////////////////////////////

	virtual void OnSerialize( IFile& file );

	void GenerateEditorFragments( CRenderFrame* frame );

	static void CalcCamera( CRenderCamera& camera, Uint32 viewportWidth, Uint32 viewportHeight, const Vector& position, const EulerAngles& rotation, Float fov, Float aspect, Float nearPlane, Float farPlane, Bool forceNearPlane, Bool forceFarPlane );

	// Returns input control weight (0 - no input; 1 - full control)
	Float GetInputWeight() const;

protected:
	void BlendData( const TDynArray<BlendCameraInfo>& dataArray );
	void ApplyFocusCamera( const TDynArray<BlendCameraInfo>& dataArray );

	void OnFocusedBlendBegin();
	void OnFocusedBlendEnd();
	void OnFocusedBlendUpdate( Float progress );

	// SCRIPTS
private:
	void funcViewCoordsToWorldVector( CScriptStackFrame& stack, void* result );
	void funcWorldVectorToViewCoords( CScriptStackFrame& stack, void* result );
	void funcWorldVectorToViewRatio( CScriptStackFrame& stack, void* result );
	void funcGetCameraPosition( CScriptStackFrame& stack, void* result );
	void funcGetCameraRotation( CScriptStackFrame& stack, void* result );
	void funcGetCameraForward( CScriptStackFrame& stack, void* result );
	void funcGetCameraRight( CScriptStackFrame& stack, void* result );
	void funcGetCameraUp( CScriptStackFrame& stack, void* result );
	void funcGetCameraHeading( CScriptStackFrame& stack, void* result );
	void funcGetCameraDirection( CScriptStackFrame& stack, void* result );
	void funcGetFov( CScriptStackFrame& stack, void* result );
	void funcGetTopmostCameraObject( CScriptStackFrame& stack, void* result );
	void funcProjectPoint( CScriptStackFrame& stack, void* result  );
	void funcUnprojectPoint( CScriptStackFrame& stack, void* result );
	void funcIsPointInView( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCameraDirector )
	PARENT_CLASS( CObject )
	NATIVE_FUNCTION( "ViewCoordsToWorldVector", funcViewCoordsToWorldVector )
	NATIVE_FUNCTION( "WorldVectorToViewCoords", funcWorldVectorToViewCoords )
	NATIVE_FUNCTION( "WorldVectorToViewRatio", funcWorldVectorToViewRatio )
	NATIVE_FUNCTION( "GetCameraPosition", funcGetCameraPosition )
	NATIVE_FUNCTION( "GetCameraRotation", funcGetCameraRotation )
	NATIVE_FUNCTION( "GetCameraForward", funcGetCameraForward )
	NATIVE_FUNCTION( "GetCameraRight", funcGetCameraRight )
	NATIVE_FUNCTION( "GetCameraUp", funcGetCameraUp )
	NATIVE_FUNCTION( "GetCameraHeading", funcGetCameraHeading )
	NATIVE_FUNCTION( "GetCameraDirection", funcGetCameraDirection )
	NATIVE_FUNCTION( "GetFov", funcGetFov )
	NATIVE_FUNCTION( "GetTopmostCameraObject", funcGetTopmostCameraObject )
	NATIVE_FUNCTION( "ProjectPoint", funcProjectPoint )
	NATIVE_FUNCTION( "UnprojectPoint", funcUnprojectPoint )
	NATIVE_FUNCTION( "IsPointInView", funcIsPointInView )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CSimpleFreeCamera : public ICamera
{
private:
	Data m_data;

public:
	virtual Bool Update( Float timeDelta ) override;
	virtual Bool GetData( Data& outData ) const override;

	virtual void OnActivate( const IScriptable* prevCameraObject, Bool resetCamera ) override;
};

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_CAMERA_DIRECTOR
#pragma optimize("",on)
#endif
