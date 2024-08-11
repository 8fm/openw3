/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialCompilerDefines.h"
#include "renderFragment.h"
#include "materialCompiler.h"
#include "baseEngine.h"

CMaterialCompilerDefines::CMaterialCompilerDefines()
{
}

CMaterialCompilerDefines::CMaterialCompilerDefines( const StringAnsi& defineName, const StringAnsi& value )
{
	Add( defineName, value );
}

void CMaterialCompilerDefines::Clear()
{
	m_defines.Clear();
}

CMaterialCompilerDefines& CMaterialCompilerDefines::AddNumerical( const StringAnsi& defineName, Int32 value )
{
	StringAnsi stringValue = StringAnsi::Printf( "%i", value );
	return Add( defineName, stringValue );
}

CMaterialCompilerDefines& CMaterialCompilerDefines::Add( const StringAnsi& defineName, const StringAnsi& value )
{
	// Change existing define
	for ( TDynArray< TPair< StringAnsi, StringAnsi > >::iterator i=m_defines.Begin(); i!=m_defines.End(); ++i )
	{
		if ( i->m_first == defineName )
		{
			i->m_second = value;
			return *this;
		}
	}

	// Add new define
	if ( !defineName.Empty() )
	{
		TPair< StringAnsi, StringAnsi > define( defineName, value );
		m_defines.PushBack( define );
	}

	//
	return *this;
}

CMaterialCompilerDefines& CMaterialCompilerDefines::Add( const CMaterialCompilerDefines& other )
{
	for ( TDynArray< TPair< StringAnsi, StringAnsi > >::const_iterator i=other.m_defines.Begin(); i!=other.m_defines.End(); ++i )
	{
		TPair< StringAnsi, StringAnsi > define( i->m_first, i->m_second );
		m_defines.PushBackUnique( define );
	}

	return *this;
}

void CMaterialCompilerDefines::RemoveDefine( const StringAnsi& defineName )
{
	for ( TDynArray< TPair< StringAnsi, StringAnsi > >::iterator i=m_defines.Begin(); i!=m_defines.End(); ++i )
	{
		if ( i->m_first == defineName )
		{
			m_defines.Erase( i );
			return;
		}
	}
}

void CMaterialCompilerDefines::InitFromRenderingStates( const RenderingContext& context )
{
	// Pass
	switch ( context.m_pass )
	{
		case RP_NoLighting:					Add( "RS_PASS_NO_LIGHTING",			"1" ); break;
		case RP_HitProxies:					Add( "RS_PASS_HIT_PROXIES",			"1" ); break;
		case RP_ShadowDepthSolid:			Add( "RS_PASS_SHADOW_DEPTH_SOLID",	"1" ); break;
		case RP_ShadowDepthMasked:			Add( "RS_PASS_SHADOW_DEPTH_MASKED",	"1" ); break;
		case RP_GBuffer:					Add( "RS_PASS_G_BUFFER",			"1" ); break;
		case RP_Emissive:					Add( "RS_PASS_EMISSIVE",			"1" ); break;
		case RP_RefractionDelta:			Add( "RS_PASS_REFRACTION_DELTA",	"1" ); break;
		case RP_ReflectionMask:				Add( "RS_PASS_REFLECTION_MASK",		"1" ); break;
		case RP_ForwardLightingSolid:		Add( "RS_FORWARD_LIGHTING",			"1" ); break;
		case RP_ForwardLightingTransparent: Add( "RS_FORWARD_LIGHTING",			"1" ); break;
		case RP_HiResShadowMask:			Add( "RS_HI_RES_SHADOW_MASK",		"1" ); break;
		default: RED_HALT( "invalid pass" );
	}
	static_assert( RP_Max == 11, "ERenderingPass probably changed. Don't forget to update here!" );
}

void CMaterialCompilerDefines::InitFromMaterialStates( const MaterialRenderingContext& context )
{
	// Init render states
	const RenderingContext* renderingContext = context.m_renderingContext;
	InitFromRenderingStates( *renderingContext );

	// Selection highlight
	if ( context.m_selected )
	{
		Add( "MS_SELECTED", "1" );
	}

	// Debug modes
	switch( context.m_materialDebugMode )
	{
	case MDM_UVDensity:		Add( "MS_UVDENSITY", "1" ); break;
	case MDM_Holes:			Add( "MS_DISPLAY_HOLES", "1" ); break;
	case MDM_Mask:			Add( "MS_DISPLAY_MASK", "1" ); break;
	case MDM_Overlay:		Add( "MS_DISPLAY_OVERLAY", "1" ); break;
	case MDM_Heightmap:		Add( "MS_HEIGHTMAP", "1" ); break;
	case MDM_WaterMode:		Add( "MS_BLACK_MASK", "1" ); break;
	case MDM_FullGBuffer:	/*empty*/ break;
	}
	static_assert( MDM_Max == 8, "EMaterialDebugMode probably changed. Don't forget to update here!" );

	// Instancing
	if ( context.m_useInstancing )
	{
		Add( "MS_INSTANCING", "1" );
	}

	if ( context.m_useParticleInstancing )
	{
		Add( "MS_PARTICLE_INSTANCING", "1" );
	}

	// Vertex color stream
	if ( context.m_hasExtraStreams )
	{
		Add( "MS_HAS_EXTRA_STREAMS", "1" );
	}

	// Vertex color stream
	if ( context.m_hasVertexCollapse )
	{
		Add( "MS_HAS_VERTEX_COLLAPSE", "1" );
	}

	// Discarding pass (a pass with discard instruction, disabling hi/early - z
	if ( context.m_discardingPass )
	{
		Add( "MS_DISCARD_PASS", "1" );

		// Dissolve enabled
		if ( context.m_uvDissolveSeparateUV )
		{
			Add( "MS_UVDISSOLVE_SEPARATE", "1" );
		}

	}

	// A pass with pregenerated maps (terrain normals for instance)
	if ( context.m_pregeneratedMaps )
	{
		Add( "MS_PREGENERATED_MAPS", "1" );
	}

	// Terrain tool
	if ( renderingContext->m_terrainToolStampVisible && context.m_vertexFactory != MVF_TerrainSkirt )
	{
		Add( "RS_TERRAIN_TOOL_STAMP_VISIBLE", "1" );
	}

	if ( context.m_lowQuality )
	{
		Add( "MS_LOW_DETAIL", "1" );
	}
}

const CMaterialCompilerDefines& CMaterialCompilerDefines::EMPTY()
{
	static CMaterialCompilerDefines emptyDefinition;
	return emptyDefinition;
}
