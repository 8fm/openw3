/**
* Copyright � 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../redSystem/crt.h"
#include "../redSystem/types.h"


namespace GpuApi
{
	class CRenderStateCache
	{
		CRenderStateCache ( const CRenderStateCache& );				// no definition
		CRenderStateCache& operator= ( const CRenderStateCache& );	// no definition

	public:
		CRenderStateCache ();
		~CRenderStateCache ();

		void Clear();

		void RefreshDeviceState();
		
		void SetupShadowDepthBias( Float depthBiasClamp, Float slopeScaledDepthBias );

		void SetWireframe( Bool newWireframe );
		void SetForcedTwoSided( Bool force );
		void SetReversedProjection( Bool newReversedProjection );

		void SetBlendMode( EBlendMode newMode );
		void SetRasterizerMode( ERasterizerMode newMode );
		void SetDepthStencilMode( EDepthStencilStateMode newMode, Uint32 stencilRef = 0, Uint8 stencilReadMask = 0, Uint8 stencilWriteMask = 0 );
	

		EBlendMode				GetBlendMode()	const { return m_blendMode; }
		ERasterizerMode			GetRasterizerMode() const { return m_rasterizerModeForced; }
		EDepthStencilStateMode	GetDepthStencilMode() const { return m_depthStencilMode; }
		EDepthStencilStateMode	GetDepthStencilModeNoOffset() const { return m_depthStencilModeNoOffset; }

		// GUI Scaleform fun
		void SetBlendModeScaleform( eGUIBlendStateType blendMode );
		void SetStencilModeScaleform( eGUIStencilModeType stencilMode, Uint8 refVal );
		void LeaveScaleformModes();
	
		Bool				GetWireframe()			const { return m_wireframe; }
		Bool				IsForcedTwoSided()		const { return m_forcedTwoSided; }
		Bool				IsReversedProjection()	const { return m_isReversedProjection; }
		Bool				IsDepthWriteEnabled()	const { return m_depthWriteEnabled; }

	private:
		//ID3D11BlendState*			m_blendStates[ BLENDMODE_Max ];
		//ID3D11RasterizerState*		m_rasterizerStates[ RASTERIZERMODE_Max ];
		//ID3D11DepthStencilState*	m_depthStencilStates[ DSSM_Max ];
		void CreateDSMode( EDepthStencilStateMode newMode, Uint8 stencilReadMask, Uint8 stencilWriteMask, Bool isReversedProjection );
		void CreateBMode( EBlendMode newMode );
		void CreateRMode( ERasterizerMode newMode );

		Uint32					m_stencilRef;

		EBlendMode				m_blendMode;
		ERasterizerMode			m_rasterizerModeOriginal;
		ERasterizerMode			m_rasterizerModeForced;
		EDepthStencilStateMode	m_depthStencilMode;
		EDepthStencilStateMode	m_depthStencilModeNoOffset;

		Float					m_shadowDepthBiasClamp;
		Float					m_shadowSlopeScaledDepthBias;

		Bool					m_wireframe;
		Bool					m_forcedTwoSided;
		Bool					m_isReversedProjection;
		Bool					m_depthWriteEnabled;
	};

	RED_INLINE CRenderStateCache::CRenderStateCache ()
	 : m_stencilRef( 0 )
	 , m_blendMode( BLENDMODE_Max )
	 , m_rasterizerModeOriginal( RASTERIZERMODE_Max )
	 , m_rasterizerModeForced( RASTERIZERMODE_Max )
	 , m_depthStencilMode( DSSM_Max )
	 , m_depthStencilModeNoOffset( DSSM_Max )
	 , m_shadowDepthBiasClamp( 0 )
	 , m_shadowSlopeScaledDepthBias( 0 )
	 , m_wireframe( false )
	 , m_forcedTwoSided( false )
	 , m_isReversedProjection( false )
	 , m_depthWriteEnabled( false )
	{
		//Red::System::MemorySet( m_blendStates, NULL, sizeof( ID3D11BlendState* ) * BLENDMODE_Max );
		//Red::System::MemorySet( m_depthStencilStates, NULL, sizeof( m_depthStencilStates ) );
		//GPUAPI_ASSERT( sizeof( ID3D11DepthStencilState* ) * DSSM_Max * 2 == sizeof( m_depthStencilStates ) );
	}

	inline CRenderStateCache::~CRenderStateCache ()
	{
		Clear();
	}

	inline void CRenderStateCache::Clear()
	{
		//for ( Uint32 i=0; i<DSSM_Max; ++i )
		//{
		//	if ( m_depthStencilStates[i] )
		//	{
		//		ULONG refcount = m_depthStencilStates[i]->Release();
		//		GPUAPI_ASSERT(refcount==0, TXT( "DepthStencilS leak" ) );
		//		m_depthStencilStates[i] = NULL;
		//	}
		//}

		//for ( Uint32 i=0; i<BLENDMODE_Max; ++i )
		//{
		//	if ( m_blendStates[i] )
		//	{
		//		ULONG refcount = m_blendStates[i]->Release();
		//		//GPUAPI_ASSERT(refcount==0, TXT( "BlendS leak" ) );
		//		// it looks like there are multiple references on the same D3D object but we will release all of them
		//		m_blendStates[i] = NULL;
		//	}
		//}

		//for ( Uint32 i=0; i<RASTERIZERMODE_Max; ++i )
		//{
		//	if ( m_rasterizerStates[i] )
		//	{
		//		ULONG refcount = m_rasterizerStates[i]->Release();
		//		GPUAPI_ASSERT(refcount==0, TXT( "RasterizerS leak" ) );
		//		m_rasterizerStates[i] = NULL;
		//	}
		//}
	}

	inline void CRenderStateCache::RefreshDeviceState()
	{
		// ace_fix!!! uzupelnic
	}

	inline void CRenderStateCache::SetWireframe( Bool newWireframe )
	{
		m_wireframe = newWireframe;
		SetRasterizerMode( m_rasterizerModeOriginal );
	}

	inline void CRenderStateCache::SetForcedTwoSided( Bool force )
	{
		m_forcedTwoSided = force;
		SetRasterizerMode( m_rasterizerModeOriginal );
	}

	inline void CRenderStateCache::SetBlendModeScaleform( eGUIBlendStateType blendMode )
	{
		switch ( blendMode )
		{
		case GUI_BlendDesc_None:
			SetBlendMode( BLENDMODE_SF_None );
			break;
		case GUI_BlendDesc_Normal:
			SetBlendMode( BLENDMODE_SF_Normal );
			break;
		case GUI_BlendDesc_Layer:
			SetBlendMode( BLENDMODE_SF_Layer );
			break;
		case GUI_BlendDesc_Multiply:
			SetBlendMode( BLENDMODE_SF_Multiply );
			break;
		case GUI_BlendDesc_Screen:
			SetBlendMode( BLENDMODE_SF_Screen );
			break;
		case GUI_BlendDesc_Lighten:
			SetBlendMode( BLENDMODE_SF_Lighten );
			break;
		case GUI_BlendDesc_Darken:
			SetBlendMode( BLENDMODE_SF_Darken );
			break;
		case GUI_BlendDesc_Difference:
			SetBlendMode( BLENDMODE_SF_Difference );
			break;
		case GUI_BlendDesc_Add:
			SetBlendMode( BLENDMODE_SF_Add );
			break;
		case GUI_BlendDesc_Subtract:
			SetBlendMode( BLENDMODE_SF_Subtract );
			break;
		case GUI_BlendDesc_Invert:
			SetBlendMode( BLENDMODE_SF_Invert );
			break;
		case GUI_BlendDesc_Alpha:
			SetBlendMode( BLENDMODE_SF_Alpha );
			break;
		case GUI_BlendDesc_Erase:
			SetBlendMode( BLENDMODE_SF_Erase );
			break;
		case GUI_BlendDesc_Overlay:
			SetBlendMode( BLENDMODE_SF_Overlay );
			break;
		case GUI_BlendDesc_HardLight:
			SetBlendMode( BLENDMODE_SF_HardLight );
			break;
		case GUI_BlendDesc_Overwrite:
			SetBlendMode( BLENDMODE_SF_Overwrite );
			break;
		case GUI_BlendDesc_OverwriteAll:
			SetBlendMode( BLENDMODE_SF_OverwriteAll );
			break;
		case GUI_BlendDesc_FullAdditive:
			SetBlendMode( BLENDMODE_SF_FullAdditive );
			break;
		case GUI_BlendDesc_FilterBlend:
			SetBlendMode( BLENDMODE_SF_FilterBlend );
			break;
		case GUI_BlendDesc_Ignore:
			SetBlendMode( BLENDMODE_SF_Ignore );
			break;

			// Source Ac

		case GUI_BlendDesc_None_SourceAc:
			SetBlendMode( BLENDMODE_SF_None_SourceAc );
			break;
		case GUI_BlendDesc_Normal_SourceAc:
			SetBlendMode( BLENDMODE_SF_Normal_SourceAc );
			break;
		case GUI_BlendDesc_Layer_SourceAc:
			SetBlendMode( BLENDMODE_SF_Layer_SourceAc );
			break;
		case GUI_BlendDesc_Multiply_SourceAc:
			SetBlendMode( BLENDMODE_SF_Multiply_SourceAc );
			break;
		case GUI_BlendDesc_Screen_SourceAc:
			SetBlendMode( BLENDMODE_SF_Screen_SourceAc );
			break;
		case GUI_BlendDesc_Lighten_SourceAc:
			SetBlendMode( BLENDMODE_SF_Lighten_SourceAc );
			break;
		case GUI_BlendDesc_Darken_SourceAc:
			SetBlendMode( BLENDMODE_SF_Darken_SourceAc );
			break;
		case GUI_BlendDesc_Difference_SourceAc:
			SetBlendMode( BLENDMODE_SF_Difference_SourceAc );
			break;
		case GUI_BlendDesc_Add_SourceAc:
			SetBlendMode( BLENDMODE_SF_Add_SourceAc );
			break;
		case GUI_BlendDesc_Subtract_SourceAc:
			SetBlendMode( BLENDMODE_SF_Subtract_SourceAc );
			break;
		case GUI_BlendDesc_Invert_SourceAc:
			SetBlendMode( BLENDMODE_SF_Invert_SourceAc );
			break;
		case GUI_BlendDesc_Alpha_SourceAc:
			SetBlendMode( BLENDMODE_SF_Alpha_SourceAc );
			break;
		case GUI_BlendDesc_Erase_SourceAc:
			SetBlendMode( BLENDMODE_SF_Erase_SourceAc );
			break;
		case GUI_BlendDesc_Overlay_SourceAc:
			SetBlendMode( BLENDMODE_SF_Overlay_SourceAc );
			break;
		case GUI_BlendDesc_HardLight_SourceAc:
			SetBlendMode( BLENDMODE_SF_HardLight_SourceAc );
			break;
		case GUI_BlendDesc_Overwrite_SourceAc:
			SetBlendMode( BLENDMODE_SF_Overwrite_SourceAc );
			break;
		case GUI_BlendDesc_OverwriteAll_SourceAc:
			SetBlendMode( BLENDMODE_SF_OverwriteAll_SourceAc );
			break;
		case GUI_BlendDesc_FullAdditive_SourceAc:
			SetBlendMode( BLENDMODE_SF_FullAdditive_SourceAc );
			break;
		case GUI_BlendDesc_FilterBlend_SourceAc:
			SetBlendMode( BLENDMODE_SF_FilterBlend_SourceAc );
			break;
		case GUI_BlendDesc_Ignore_SourceAc:
			SetBlendMode( BLENDMODE_SF_Ignore_SourceAc );
			break;

			// No color write

		case GUI_BlendDesc_NoColorWrite:
			SetBlendMode( BLENDMODE_SF_NoColorWrite );
			break;

		default:
			GPUAPI_HALT( "Unhandled eGUIBlendStateType" );
			break;
		}
	}

	inline void CRenderStateCache::SetStencilModeScaleform( eGUIStencilModeType stencilMode, Uint8 refVal )
	{
		switch ( stencilMode )
		{
		case GUI_Stencil_Invalid:
			SetDepthStencilMode( DSSM_Scaleform_Invalid, refVal );
			break;
		case GUI_Stencil_Disabled:
			SetDepthStencilMode( DSSM_Scaleform_Disabled, refVal );
			break;
		case GUI_Stencil_StencilClear:
			SetDepthStencilMode( DSSM_Scaleform_StencilClear, refVal );
			break;
		case GUI_Stencil_StencilClearHigher:
			SetDepthStencilMode( DSSM_Scaleform_StencilClearHigher, refVal );
			break;
		case GUI_Stencil_StencilIncrementEqual:
			SetDepthStencilMode( DSSM_Scaleform_StencilIncrementEqual, refVal );
			break;
		case GUI_Stencil_StencilTestLessEqual:
			SetDepthStencilMode( DSSM_Scaleform_StencilTestLessEqual, refVal );
			break;
		case GUI_Stencil_DepthWrite:
			SetDepthStencilMode( DSSM_Scaleform_DepthWrite, refVal );
			break;
		case GUI_Stencil_DepthTestEqual:
			SetDepthStencilMode( DSSM_Scaleform_DepthTestEqual, refVal );
			break;
		default:
			GPUAPI_HALT( "Unhandled eGUIStencilModeType" );
			break;
		}
	}
}


