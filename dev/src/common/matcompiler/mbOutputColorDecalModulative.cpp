/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorDecalModulative.h"
#include "../engine/renderFragment.h"
#include "../engine/renderFrame.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorDecalModulative );

using namespace CodeChunkOperators;

CMaterialBlockOutputColorDecalModulative::CMaterialBlockOutputColorDecalModulative()
	: m_maskThreshold( 0.33f )
{
}

Bool CMaterialBlockOutputColorDecalModulative::IsTwoSided() const
{
	return false;
}

Bool CMaterialBlockOutputColorDecalModulative::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputColorDecalModulative::GetBlendMode() const
{
	// We don't output blending mode here explicitly, because decals are blended implicitly, in a special way.
	return RBM_None; 
}

bool CMaterialBlockOutputColorDecalModulative::IsEmissive() const
{
	return false;
}

ERenderingSortGroup CMaterialBlockOutputColorDecalModulative::GetSortGroup() const
{
	return RSG_DecalModulativeColor;	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorDecalModulative::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorDecalModulative::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Amount ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorDecalModulative::Compile( CMaterialBlockCompiler& compiler ) const
{
	const Vector			decalDefaultColor	= Vector ( 3.f, 0.f, 4.f, 1.f );	
	const ERenderingPass	pass				= compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_pass;

	EMaterialShaderTarget shaderTarget = MSH_PixelShader;

	// Mask 

	if ( HasInput( CNAME( Mask ) ) ) 
	{
		CodeChunk mask = CompileInput( compiler, CNAME( Mask ), shaderTarget, MATERIAL_DEFAULT_MASK );
		CodeChunk clipValue = mask.w() - m_maskThreshold;
		compiler.GetPS().Discard( clipValue );
	}

	// Optional clipping

	// Mask not included here. It's always done, not togglable
	compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), CodeChunk() );

	// Compile material color for given pass
	
	bool outputDefined		= false;
	CodeChunk outputColor	= decalDefaultColor;

	switch ( pass )
	{
	//dex++: decals, not idea about them
	case RP_ForwardLightingSolid:
	case RP_ForwardLightingTransparent:
	//dex--
	case RP_NoLighting:
		{
			CodeChunk decalColor;
			decalColor = CompileInput( compiler, CNAME( Color ), shaderTarget, decalDefaultColor ).xyz();
			decalColor = compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float3, decalColor, false, true );

			if ( HasInput( CNAME( Amount ) ) )
			{
				CodeChunk amount = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Amount ), shaderTarget, 1.f ) );
				decalColor = Lerp( Float3 (1.f, 1.f, 1.f), decalColor, amount );
			}

			outputColor = Float4 ( decalColor, 1.f );
		}		
		break;

	case RP_HitProxies:
	case RP_ShadowDepthSolid:
	case RP_ShadowDepthMasked:
	case RP_HiResShadowMask:
		{
			outputDefined = compiler.GetPS().CompileRenderPassSimple( pass );
			if ( !outputDefined )
			{
				HALT( "Simple pass not recognized, or failed to compile" );
			}			
		}	
		break;

	case RP_GBuffer:
		{
			// not supported
		}
		break;	

	case RP_RefractionDelta:
		{
			// not supported
		}
		break;

	case RP_ReflectionMask:
		{
			// not supported
		}
		break;

	case RP_Emissive:
		{
			// not supported
		}
		break;

	default:
		ASSERT( !"Invalid rendering context pass" );
	};

	// Postprocess calculated color

	if ( !outputDefined )
	{		
		// Handle selection

		if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
		{
			ApplyMaterialSelectionColor4v( outputColor, pass );
		}

		// Finalize

		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", outputColor );	
	}
}

void CMaterialBlockOutputColorDecalModulative::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
{
	// Defaults
	passes.PushBackUnique( RP_ShadowDepthSolid );	// we need this because we can have it dissolving
	passes.PushBackUnique( RP_ShadowDepthMasked );	// we need this for the masking
	passes.PushBackUnique( RP_HitProxies );			// hit proxy for everything
	passes.PushBackUnique( RP_HiResShadowMask );	// 

	// Supported
	passes.PushBackUnique( RP_NoLighting );
	passes.PushBackUnique( RP_ForwardLightingSolid );
	passes.PushBackUnique( RP_ForwardLightingTransparent );
}

#endif
