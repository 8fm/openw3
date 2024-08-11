/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbGradient.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#define  MAX_GRADIENT_KEYS 16

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockGradient );
IMPLEMENT_RTTI_ENUM(EGradientTypes);
//IMPLEMENT_RTTI_ENUM(EGradientInterpolationModes);
IMPLEMENT_RTTI_ENUM(EGradientExtrapolationModes);

void CMaterialBlockGradient::OnPropertyPostChange( IProperty* property )
{
	const CName &propertyName =  property->GetName();

	if( propertyName == CName( TXT("reverse") ) )
	{
		SSimpleCurve reversedGradient = SSimpleCurve( m_gradient );
		reversedGradient.Clear();
		for ( Uint32 i = 0; i < m_gradient.GetNumPoints() ; ++i )
		{
			reversedGradient.AddPoint( 1.0f - m_gradient.GetCurveData().m_curveValues[i].time, m_gradient.GetValueAtIndex(i), true );
		}
		reversedGradient.GetCurveData().Sort();
		m_gradient = reversedGradient;
	}

	if( propertyName == CName( TXT("loop") ) )
	{
		m_gradient.GetCurveData().SetLoop( m_loop );
	}

	TBaseClass::OnPropertyPostChange( property );
}

CMaterialBlockGradient::CMaterialBlockGradient()
	:m_gradientType( GT_Linear )
	,m_reverse( false )
	,m_loop( false )
	,m_offset ( 0.0f )
	,m_gradientExtrapolationMode( GEM_Clamp )
{
	m_gradient = SSimpleCurve( SCT_Color );
	m_gradient.AddPoint( 0.25f, SSimpleCurvePoint::BuildValue( Color( 255, 255, 255, 255), 1.f ) );
	m_gradient.AddPoint( 0.75f, SSimpleCurvePoint::BuildValue( Color( 0, 0, 0, 255), 1.f ) );
	m_gradient.GetCurveData().SetLoop(false);
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockGradient::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );    
	CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( In )) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Out ), Color::WHITE) );
}

#endif

String CMaterialBlockGradient::GetCaption() const
{
	switch (m_gradientType)
	{
		case GT_Linear: return TXT("Linear Gradient");
		case GT_Radial: return TXT("Radial Gradient");
		case GT_Spherical: return TXT("Spherical Gradient");
		case GT_Angle: return TXT("Angle Gradient");
		case GT_Diamond: return TXT("Diamond Gradient");
		default: return TXT("Gradient");
	}
}

CodeChunk CMaterialBlockGradient::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	if ( shaderTarget == MSH_VertexShader )
	{
		compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );
	}
	if ( shaderTarget == MSH_PixelShader )
	{
		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );
	}
	CodeChunk input = CompileInput( compiler, CNAME( In ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
	CodeChunk numPoints = m_gradient.GetNumPoints();
	CodeChunk pointsMaxIndex = m_gradient.GetNumPoints() - 1;
	CodeChunk offset = m_offset;
	CodeChunk loop;
	if ( m_loop )
	{
		loop = "true";
	}
	else
	{
		loop = "false";
	}

	CodeChunk interpolation;
	switch( m_gradient.GetCurveData().GetBaseType() )
	{
	case( CT_Segmented ): interpolation = "0"; break;
	case( CT_Linear ): interpolation = "1"; break;
	case( CT_Smooth ): interpolation = "2"; break;
	}

	CodeChunk positionsParam = compiler.GetShader(shaderTarget).AutomaticName();
	CodeChunk positionsHeader = CodeChunk::Printf( false, "const float %s[MAX_GRADIENT_KEYS] =\n{", positionsParam.AsChar() );
	compiler.GetShader(shaderTarget).Statement( positionsHeader );
	for ( Uint32 i = 0; i < m_gradient.GetNumPoints(); i++)
	{

		CodeChunk position = m_gradient.GetCurveData().m_curveValues[i].time;
		CodeChunk positionsEntry = CodeChunk::Printf( false, "\t%s,", position.AsChar());
		compiler.GetShader(shaderTarget).Statement( positionsEntry );
	}
	for ( Uint32 i = m_gradient.GetNumPoints(); i < MAX_GRADIENT_KEYS; i++ )
	{
		compiler.GetShader(shaderTarget).Statement( CodeChunk("\t( 0.0f ),") );
	}
	CodeChunk positionEnd = "};";
	compiler.GetShader(shaderTarget).Statement( positionEnd );

	CodeChunk colorsParam = compiler.GetShader(shaderTarget).AutomaticName();
	CodeChunk colorsHeader = CodeChunk::Printf( false, "const float4 %s[MAX_GRADIENT_KEYS] =\n{", colorsParam.AsChar() );
	compiler.GetShader(shaderTarget).Statement( colorsHeader );
	for ( Uint32 i = 0; i < m_gradient.GetNumPoints(); i++)
	{

		CodeChunk color_r = pow( m_gradient.GetValueAtIndex(i).X / 255.0f, 2.2f );
		CodeChunk color_g = pow( m_gradient.GetValueAtIndex(i).Y / 255.0f, 2.2f );
		CodeChunk color_b = pow( m_gradient.GetValueAtIndex(i).Z / 255.0f, 2.2f );
		CodeChunk color_a = m_gradient.GetValueAtIndex(i).W / 255.0f;
		CodeChunk colorsEntry = CodeChunk::Printf( false, "\tfloat4( %s, %s, %s, %s ),", color_r.AsChar(), color_g.AsChar(), color_b.AsChar(), color_a.AsChar());
		compiler.GetShader(shaderTarget).Statement( colorsEntry );
	}

	for ( Uint32 i = m_gradient.GetNumPoints(); i < MAX_GRADIENT_KEYS; i++ )
	{
		compiler.GetShader(shaderTarget).Statement( CodeChunk("\tfloat4( 0.0f, 0.0f, 0.0f, 1.0f ),") );
	}

	CodeChunk colorsEnd = "};";
	compiler.GetShader(shaderTarget).Statement( colorsEnd );


	using namespace CodeChunkOperators;

	
	CodeChunk gradientShapedValue;
	CodeChunk shapeComment;
	CodeChunk gradientShapedValueParam = compiler.GetShader(shaderTarget).AutomaticName();
	switch ( m_gradientType )
	{
	case GT_Linear:
		shapeComment = "// Linear Gradient Type";
		gradientShapedValue = CodeChunk::Printf( false, "float %s = (%s).x;\t%s", gradientShapedValueParam.AsChar(), input.AsChar(), shapeComment.AsChar() );
		break;
	case GT_Radial:
		shapeComment = "// Radial Gradient Type";
		gradientShapedValue = CodeChunk::Printf( false, "float %s = sqrt( ( ( (%s).x - 0.5f ) * 2.0f ) * ( ( (%s).x - 0.5f ) * 2.0f ) + ( ( (%s).y - 0.5f ) * 2.0f ) * ( ( (%s).y - 0.5f ) * 2.0f ) );\t%s", gradientShapedValueParam.AsChar(), input.AsChar(),  input.AsChar(),  input.AsChar(),  input.AsChar(), shapeComment.AsChar() );
		break;
	case GT_Spherical:
		shapeComment = "// Spherical Gradient Type";
		gradientShapedValue = CodeChunk::Printf( false, "float %s = sqrt( (%s).x * (%s).x + (%s).y * (%s).y + (%s).z * (%s).z );\t%s", gradientShapedValueParam.AsChar(), input.AsChar(),  input.AsChar(),  input.AsChar(),  input.AsChar(),  input.AsChar(),  input.AsChar(), shapeComment.AsChar() );
		break;
	case GT_Angle:
		shapeComment = "// Angle Gradient Type";
		gradientShapedValue = CodeChunk::Printf( false, "float %s = atan2( (%s).y - 0.5f, (%s).x - 0.5f ) / 6.28318530718f + 0.5f;\t%s", gradientShapedValueParam.AsChar(), input.AsChar(), input.AsChar(), shapeComment.AsChar() );
		break;
	case GT_Diamond:
		shapeComment = "// Diamond Gradient Type";
		gradientShapedValue = CodeChunk::Printf( false, "float %s = abs( (%s).x - 0.5f ) * 2.0f + abs( (%s).y - 0.5f ) * 2.0f;\t%s", gradientShapedValueParam.AsChar(), input.AsChar(),  input.AsChar(), shapeComment.AsChar() );
		break;
	default:
		gradientShapedValue = "Unknown gradient type";
	}
	compiler.GetShader(shaderTarget).Statement( gradientShapedValue );

	CodeChunk gradientOffsetValueParam = compiler.GetShader(shaderTarget).AutomaticName();
	CodeChunk gradientOffsetValue = CodeChunk::Printf( false, "float %s = %s - %s;\t// Adding Offset", gradientOffsetValueParam.AsChar(), gradientShapedValueParam.AsChar(), offset.AsChar() );
	compiler.GetShader(shaderTarget).Statement( gradientOffsetValue );

	CodeChunk gradientExtendedValue;
	CodeChunk extrapolationComment;
	CodeChunk gradientExtendedValueParam = compiler.GetShader(shaderTarget).AutomaticName();
	switch ( m_gradientExtrapolationMode )
	{
	case GEM_Clamp:
		extrapolationComment = "// Clamp Extrapolation Mode";
		gradientExtendedValue = CodeChunk::Printf( false, "float %s = saturate( %s );\t%s", gradientExtendedValueParam.AsChar(), gradientOffsetValueParam.AsChar(), extrapolationComment.AsChar() );
		break;
	case GEM_Mirror:
		extrapolationComment = "// Mirror Extrapolation Mode";
		gradientExtendedValue = CodeChunk::Printf( false, "float %s = 1.0f - abs( 2.0f * frac( %s * 0.5f ) - 1.0f );\t%s", gradientExtendedValueParam.AsChar(), gradientOffsetValueParam.AsChar(), extrapolationComment.AsChar() );
		break;
	case GEM_Repeat:
		extrapolationComment = "// Repeat Extrapolation Mode";
		gradientExtendedValue = CodeChunk::Printf( false, "float %s = frac( %s );\t%s", gradientExtendedValueParam.AsChar(), gradientOffsetValueParam.AsChar(), extrapolationComment.AsChar() );
		break;
	default: 
		gradientExtendedValue = "Unknown extrapolation mode";
	}
	compiler.GetShader(shaderTarget).Statement( gradientExtendedValue );

	CodeChunk resultParam = compiler.GetShader(shaderTarget).AutomaticName();
	CodeChunk result = CodeChunk::Printf( false, "float4 %s = GetGradientColor( %s, %s, %s, %s, %s, %s );", resultParam.AsChar(), gradientExtendedValueParam.AsChar(), numPoints.AsChar(), positionsParam.AsChar(), colorsParam.AsChar(), interpolation.AsChar(), loop.AsChar() );
	compiler.GetShader(shaderTarget).Statement( result );
	
	return resultParam;
}

#endif