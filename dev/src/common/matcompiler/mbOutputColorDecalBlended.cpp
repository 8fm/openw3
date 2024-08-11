/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorDecalBlended.h"
#include "../engine/renderFragment.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorDecalBlended );

using namespace CodeChunkOperators;

CMaterialBlockOutputColorDecalBlended::CMaterialBlockOutputColorDecalBlended()
	: m_maskThreshold( 0.33f )
	, m_isMimicMaterial( false )
{
}

Bool CMaterialBlockOutputColorDecalBlended::IsTwoSided() const
{
	return false;
}

Bool CMaterialBlockOutputColorDecalBlended::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputColorDecalBlended::GetBlendMode() const
{
	// We don't output blending mode here explicitly, because decals are blended implicitly, in a special way.
	return RBM_None; 
}

bool CMaterialBlockOutputColorDecalBlended::IsEmissive() const
{
	return false;
}

ERenderingSortGroup CMaterialBlockOutputColorDecalBlended::GetSortGroup() const
{
	ERenderingSortGroup group = RSG_DecalBlendedColor;

	if ( HasInput( CNAME( Normal ) ) )
	{
		if ( !HasInput(CNAME(Ambient)) && !HasInput(CNAME(Diffuse)) && !HasInput(CNAME(Specularity)) ) // amount and mask may or may not be set
		{
			ASSERT( HasInput(CNAME(Normal)) );
			group = RSG_DecalBlendedNormals;
		}
		else
		{
			group = RSG_DecalBlendedNormalsColor;
		}
	}
	
	return group;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorDecalBlended::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorDecalBlended::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Ambient ) ) );	
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Diffuse ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Specularity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Glossiness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Amount ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorDecalBlended::Compile( CMaterialBlockCompiler& compiler ) const
{
	const ERenderingPass	pass	= compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_pass;

	EMaterialShaderTarget shaderTarget = MSH_PixelShader;

	// Mask 

	if ( HasInput( CNAME( Mask ) ) ) 
	{
		CodeChunk mask = CompileInput( compiler, CNAME( Mask ), shaderTarget, MATERIAL_DEFAULT_MASK );
		CodeChunk clipValue = mask.w() - m_maskThreshold;
		compiler.GetPS().Discard( clipValue );
	}

	// Optional clipping

	// Not including mask here. It's always done (not togglable)
	compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), CodeChunk() );

	// Compile material color for given pass
	
	bool outputDefined		= false;
	CodeChunk outputColor	= MATERIAL_DEFAULT_OUTPUT_COLOR;
	CodeChunk alpha			= 1.f;

	switch ( pass )
	{
	//dex++: decals, not idea about them
	case RP_ForwardLightingSolid:
	case RP_ForwardLightingTransparent:
	//dex--
	case RP_NoLighting:
		{
			outputColor	= CompileInput( compiler, CNAME( Diffuse ), shaderTarget, MATERIAL_DEFAULT_DIFFUSE ).xyz();
			outputColor = compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float3, outputColor, false, true );
			alpha		= CompileInput( compiler, CNAME( Amount ), shaderTarget, 1.f ).x();
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
			HALT( "Codepath not supported anymore. Whole outputColorDecalBlended block should be removed." );

			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", CodeChunkOperators::Float4 ( 1.f, 1.f, 1.f, 1.f ) );
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT1", CodeChunkOperators::Float4 ( 1.f, 1.f, 1.f, 1.f ) );
			outputDefined = true;
		}
		break;		

	case RP_RefractionDelta:
		{
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", MATERIAL_DEFAULT_REFRACTIONDELTA );
			outputDefined = true;
		}
		break;

	case RP_ReflectionMask:
		if ( pass == RP_ReflectionMask )
		{
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", MATERIAL_DEFAULT_REFLECTIONMASK );
			return;
		}

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
			ApplyMaterialSelectionColor3v( outputColor, pass );
		}

		// Finalize

		CodeChunk finalColor = Float4 ( alpha * outputColor, alpha ); // premultiplied alpha
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", finalColor );	
	}
}

void CMaterialBlockOutputColorDecalBlended::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
{
	// Defaults
	passes.PushBackUnique( RP_ShadowDepthSolid );	// we need this because we can have it dissolving
	passes.PushBackUnique( RP_ShadowDepthMasked );	// we need this for the masking
	passes.PushBackUnique( RP_HitProxies );			// hit proxy for everything
	passes.PushBackUnique( RP_HiResShadowMask );	// 


	//For some reason these are not supported in the cooking code, but the Gbuffer pass is not supported in this file so there is no way this material would output color
	//// Supported
	//passes.PushBackUnique( RP_NoLighting );
	//passes.PushBackUnique( RP_ForwardLightingSolid );
	//passes.PushBackUnique( RP_ForwardLightingTransparent );
}

#endif
