/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "viewport.h"
#include "viewportHook.h"

extern EShowFlags GShowGameMask[];

IViewport::IViewport( Uint32 width, Uint32 height, EViewportWindowMode windowMode )
	: m_hook( NULL )
	, m_width( width )
	, m_height( height )
	, m_fullWidth( width )
	, m_fullHeight( height )
	, m_x( 0 )
	, m_y( 0 )
	, m_windowMode( windowMode )
	, m_isAutoRedraw( false )
	, m_renderingMode( RM_Wireframe )
	, m_isTerrainToolStampVisible( false )
	, m_isGrassMaskPaintMode( false )
	, m_mouseMode( MM_Normal )
	, m_cachetAspectRatio( EAspectRatio::FR_NONE )
{	
	Red::System::MemoryZero( m_visualDebugCommonOptions, sizeof( Float ) * VDCommon__MAX_INDEX );
	SetRenderingMask( GShowGameMask );

	{
		// set default values, so that we can use all filters on cook as well.
		SetRenderingDebugOptions( VDCommon_MaxRenderingDistance, 10000.0f );
		SetRenderingDebugOptions( VDCommon_DebugLinesThickness, 0.09f );
	}
}

IViewport::~IViewport()
{
}

void IViewport::SetRenderingMode( ERenderingMode mode )
{
	m_renderingMode = mode;
}

void IViewport::SetRenderingMask( const EShowFlags* showFlags )
{
	Red::System::MemoryZero( m_renderingMask, sizeof( Bool ) * SHOW_MAX_INDEX );
	for ( Uint32 i = 0; showFlags[ i ] < SHOW_MAX_INDEX; ++i )
	{
		m_renderingMask[ showFlags[ i ] ] = true;
	}

#if defined( NO_DEBUG_PAGES )

	// calling IsDebuggerPresent() crashes game on ORBIS in Assist mode
	//if ( !Red::System::Error::Handler::GetInstance()->IsDebuggerPresent() )
	{
		m_renderingMask[ SHOW_OnScreenMessages ] = false;
		m_renderingMask[ SHOW_ErrorState ] = false;
		m_renderingMask[ SHOW_VisualDebug ] = false;
	}

#endif
}  

void IViewport::SetRenderingDebugOptions( EVisualDebugCommonOptions option, Float val )
{	
	m_visualDebugCommonOptions[ option ] = val;	
}

Float IViewport::GetRenderingDebugOption( EVisualDebugCommonOptions option  )
{
	return m_visualDebugCommonOptions[ option ];
}

void IViewport::SetRenderingMask( EShowFlags showFlag )
{
	ASSERT( showFlag < SHOW_MAX_INDEX );
	m_renderingMask[ showFlag ] = true;
}  

void IViewport::SetTerrainToolStampVisible( Bool visible )
{
	m_isTerrainToolStampVisible = visible;
}

void IViewport::SetGrassMaskPaintMode( Bool flag )
{
	m_isGrassMaskPaintMode = flag;
}

void IViewport::SetViewportHook( IViewportHook* hook )
{
	m_hook = hook;

	if ( m_hook )
	{
		m_hook->OnHookAttached( this );
	}
}

void IViewport::SetAutoRedraw( Bool autoRedraw )
{
	m_isAutoRedraw = true;
}

void IViewport::ClearRenderingMask( const EShowFlags* showFlags )
{
	for ( Uint32 i = 0; showFlags[ i ] < SHOW_MAX_INDEX; ++i )
	{
		m_renderingMask[ showFlags[ i ] ] = false;
	}
}

void IViewport::ClearRenderingMask( EShowFlags showFlag )
{
	if ( showFlag == SHOW_ALL_FLAGS )
	{
		Red::System::MemoryZero( m_renderingMask, sizeof( Bool ) * SHOW_MAX_INDEX );
		return;
	}

	ASSERT( showFlag < SHOW_MAX_INDEX );
	m_renderingMask[ showFlag ] = false;
}

void IViewport::SetClassRenderingExclusion( CClass * type, Bool exclude )
{
	if ( exclude )
	{
		m_classRenderingExclusion.PushBackUnique( type );
	}
	else
	{
		m_classRenderingExclusion.RemoveFast( type );
	}
}

void IViewport::SetTemplateRenderingExclusion( const String& entTemplate, Bool exclude )
{
	m_templateRenderingExclusion.Set( entTemplate, exclude );
}

Bool IViewport::IsTemplateRenderingDisabled( const CEntityTemplate * entTemplate, const CClass* componentClass ) const
{
	if ( entTemplate )
	{
		const String& templateFilename = entTemplate->GetFile()->GetFileName();
		if ( m_templateRenderingExclusion.KeyExist( templateFilename ) )
		{
			return m_templateRenderingExclusion[ templateFilename ];
		}
		else
		{
			const String otherTemplates = String::Printf( TXT("other templates %ls"), componentClass->GetName().AsString().AsChar() );
			return m_templateRenderingExclusion.KeyExist( otherTemplates ) && m_templateRenderingExclusion[ otherTemplates ];
		}
	}
	else
	{
		const String nonTemplated = String::Printf( TXT("non-templated %ls"), componentClass->GetName().AsString().AsChar() );
		return m_templateRenderingExclusion.KeyExist( nonTemplated ) && m_templateRenderingExclusion[ nonTemplated ];
	}
}

void IViewport::CalcRay( Int32 x, Int32 y, Vector& origin, Vector &dir ) 
{
	const Float halfWidth = GetWidth() * 0.5f;
	const Float halfHeight = GetHeight() * 0.5f;

	// Calculate screen space point
	Vector screenSpacePoint, startPoint, endPoint;
	screenSpacePoint.X = ( x / halfWidth ) - 1.0f;
	screenSpacePoint.Y = 1.0f - ( y / halfHeight );
	screenSpacePoint.Z = 0.1f;
	screenSpacePoint.W = 1.0f;

	// Transform to world space
	CRenderFrameInfo info( this );
	startPoint = info.m_camera.GetScreenToWorld().TransformVectorWithW( screenSpacePoint );
	startPoint.Div4( startPoint.W );

	// End point
	screenSpacePoint.Z = 1.0f;
	endPoint = info.m_camera.GetScreenToWorld().TransformVectorWithW( screenSpacePoint );
	endPoint.Div4( endPoint.W );

	// Use camera origin as ray origin
	origin = info.m_camera.GetPosition();
	dir = ( endPoint - startPoint ).Normalized3();
}

Bool IViewport::AdjustSize( Uint32 width, Uint32 height )
{
	m_fullWidth = m_width = width;
	m_fullHeight = m_height = height;

	if( m_cachetAspectRatio != EAspectRatio::FR_NONE )
	{
		Int32 outVertical = 0, outHorizontal = 0;

		Float viewAspect = (Float)m_fullWidth / (Float)m_fullHeight;

		Float aspect = 1.0f;

		switch ( m_cachetAspectRatio )
		{
		case EAspectRatio::FR_16_9:
			aspect = 1.777777777777778f;
			break;
		case EAspectRatio::FR_16_10:
			aspect = 1.6f;
			break;
		case EAspectRatio::FR_4_3:
			aspect = 1.333333333333333f;
			break;
		case EAspectRatio::FR_21_9:
			aspect = 2.333333333333333f;
			break;
		default:
			aspect = viewAspect;
			break;
		}

		if( aspect != 0.0f )
		{
			if ( viewAspect <= aspect )
			{
				Float cameraHeight = m_fullWidth / aspect;
				outVertical = (Int32) ceilf( ( m_fullHeight - cameraHeight ) / 2.0f );
			}
			else
			{
				Float cameraWidth = m_fullHeight * aspect;
				outHorizontal = (Int32) ceilf( ( m_fullWidth - cameraWidth ) / 2.0f );
			}
		}

		
		AdjustSize( outHorizontal, outVertical, m_fullWidth - 2*outHorizontal, m_fullHeight - 2*outVertical );
	}

	if ( m_hook )
	{
		m_hook->OnViewportSetDimensions( this );
	}

	return true;
}

Bool IViewport::AdjustSize( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
{

	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;

	if ( m_hook )
	{
		m_hook->OnViewportSetDimensions( this );
	}

	return true;
}

void IViewport::AdjustSizeWithCachets( EAspectRatio cachetAspectRatio )
{
	m_cachetAspectRatio = cachetAspectRatio;
	AdjustSize( m_fullWidth, m_fullHeight );
}

void IViewport::RestoreSize()
{
	m_cachetAspectRatio = EAspectRatio::FR_NONE;
	m_x = 0;
	m_y = 0;
	AdjustSize( m_fullWidth, m_fullHeight );
}

void IViewport::GetSizeAdjustedToCachet( const EAspectRatio aspectRatio, const Uint32 inFullWidth, const Uint32 inFullHeight, Int32& outWidth, Int32& outHeight) const
{
	if( aspectRatio != EAspectRatio::FR_NONE )
	{
		Int32 outVertical = 0, outHorizontal = 0;
		Float viewAspect = (Float)inFullWidth / (Float)inFullHeight;
		Float aspect = 1.0f;

		switch ( aspectRatio )
		{
		case EAspectRatio::FR_16_9:
			aspect = 1.777777777777778f;
			break;
		case EAspectRatio::FR_16_10:
			aspect = 1.6f;
			break;
		case EAspectRatio::FR_4_3:
			aspect = 1.333333333333333f;
			break;
		case EAspectRatio::FR_21_9:
			aspect = 2.333333333333333f;
			break;
		default:
			aspect = viewAspect;
			break;
		}

		if( aspect != 0.0f )
		{
			if ( viewAspect <= aspect )
			{
				Float cameraHeight = inFullWidth / aspect;
				outVertical = (Int32) ceilf( ( inFullHeight - cameraHeight ) );
			}
			else
			{
				Float cameraWidth = inFullHeight * aspect;
				outHorizontal = (Int32) ceilf( ( inFullWidth - cameraWidth ) );
			}
		}

		outWidth = inFullWidth - outHorizontal;
		outHeight = inFullHeight - outVertical;
	}
	else
	{
		outWidth = inFullWidth;
		outHeight = inFullHeight;
	}
}
