/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/renderFragment.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

// Shift color using HSL transform. Uses Red/Blue channels of input color to determine which shift transform to use.
class CMaterialBlockShiftColorRB : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockShiftColorRB, CMaterialBlock, "Deprecated", "Color shift (Red/Blue)" );

protected:

	Float m_colorThresholdLow;
	Float m_colorThresholdHigh;
	Float m_satThresholdLow;
	Float m_satThresholdHigh;

public:

	CMaterialBlockShiftColorRB()
		: m_colorThresholdLow( 0.3f )
		, m_colorThresholdHigh( 1.0f )
		, m_satThresholdLow( 0.1f )
		, m_satThresholdHigh( 0.3f )
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	void OnPropertyPostChange( IProperty* property )
	{
	}

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
		// TODO: These should be using Bool sockets, but setting bool parameters on the render side seems to be broken (shader compiles and
		// generates code correctly, but when creating the material parameter buffers, the bool values are not right). For now, we just
		// say that positive is the same as true (enabled).
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( BlendColors ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( KeepGray ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Enabled ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		IMaterialShaderCompiler& shaderCompiler = compiler.GetShader( shaderTarget );

		CodeChunk color = CompileInput( compiler, CNAME( Color ), shaderTarget );
		CodeChunk mask = CompileInput( compiler, CNAME( Mask ), shaderTarget ).w();
		// TODO: These should be bool values, in which case we don't need the comparisons...
		CodeChunk blendColors = CompileInput( compiler, CNAME( BlendColors ), shaderTarget, CodeChunk(0.0f) ).x() > CodeChunk( 0.0f );
		CodeChunk keepGray = CompileInput( compiler, CNAME( KeepGray ), shaderTarget, CodeChunk(1.0f) ).x() > CodeChunk( 0.0f );
		CodeChunk enabled = CompileInput( compiler, CNAME( Enabled ), shaderTarget, CodeChunk(1.0f) ).x() > CodeChunk( 0.0f );

		CodeChunk output = shaderCompiler.Var( MDT_Float4, CodeChunk::EMPTY );

		//
		// Wrap the whole thing in an if/else, so that when branching is available turning the block off will skip the calculations!
		//
		shaderCompiler.Statement( CodeChunk::Printf( false,
			"%s if ( %s ) {",
			( enabled.IsConst() ? "" : "[branch]" ),
			enabled.AsChar() ) );

		//////////////////////////////////////////////////////////////////////////
		// If enabled
		{
		CodeChunk shiftMatrix1 = PS_CONST_FLOAT44( PSC_ColorOne );
		
		// Since color shift 2 is based on blue, we'll swap rows (columns?) 0 and 2 in color shift 2, so that the same settings applied to either shift
		// result in approximately the same final color. This also helps reduce artifacts where one shift area touches the other.
		CodeChunk shiftMatrix2 = PS_CONST_FLOAT44( PSC_ColorTwo );
		shiftMatrix2 = shaderCompiler.Var( MDT_Float4x4, CodeChunk::Printf( shiftMatrix2.IsConst(),
			"float4x4( %s, %s, %s, %s )",
			shiftMatrix2[2u].AsChar(), shiftMatrix2[1u].AsChar(), shiftMatrix2[0u].AsChar(), shiftMatrix2[3u].AsChar() ) );

		// If we're blending the two shifted colors, we need to do a weighted average.
		// Use Red/Blue channels from input color to determine weighting of the two color shifts.
		// Normalize the RB vector, so that weights are based only on the proportion between channels, and not their actual values.
		// ... and by "normalize" I mean make the weights sum to 1...
		CodeChunk weights = color.xz() / ( color.x() + color.z() );
		// Rescale to apply proper thresholds.
		weights = Saturate( ( weights - m_colorThresholdLow ) / ( m_colorThresholdHigh - m_colorThresholdLow ) );

		// Save these thresholded weights out to a new variable to prevent the shader code from getting too huge.
		weights = shaderCompiler.Var( MDT_Float2, weights );

		// Make sure the weights add to 1, so we don't get over-brightening or anything.
		weights = weights / ( weights.x() + weights.y() );

		// Combine the two color shifts... Since weights add to 1, we can just use one as a lerp factor.
		CodeChunk blendMatrix = ( CodeChunk( 1.0f ) - weights.y() ) * shiftMatrix1 + ( weights.y() * shiftMatrix2 );


		// When not blending the two shifted colors, we can just pick the shift with larger contribution.
		CodeChunk selectMatrix = CodeChunk::Printf( color.IsConst() && shiftMatrix1.IsConst() && shiftMatrix2.IsConst(),
				"( ( %s ) >= ( %s ) ? ( %s ) : ( %s ) )", color.x().AsChar(), color.z().AsChar(), shiftMatrix1.AsChar(), shiftMatrix2.AsChar() );

		CodeChunk finalMatrix = shaderCompiler.Var( MDT_Float4x4, CodeChunk::EMPTY );

		// Pick the final color shift matrix based on whether we're blending the colors together.
		shaderCompiler.Statement( CodeChunk::Printf( blendColors.IsConst() && blendMatrix.IsConst() && selectMatrix.IsConst(),
				"%s if ( %s ) { %s = %s; }"
				"else { %s = %s; }",
				( blendColors.IsConst() ? "" : "[branch]" ),
			blendColors.AsChar(),
			finalMatrix.AsChar(), blendMatrix.AsChar(),
			finalMatrix.AsChar(), selectMatrix.AsChar()
			) );

		CodeChunk combined = Mul( color, finalMatrix );

		// If we aren't shifting gray, we need to figure out how gray the input color is.
		// Get the saturation of the diffuse color. Desaturated areas will not be shifted.
		CodeChunk colorMin = shaderCompiler.Var( MDT_Float, Min( color.x(), Min( color.y(), color.z() ) ) );
		CodeChunk colorMax = shaderCompiler.Var( MDT_Float, Max( color.x(), Max( color.y(), color.z() ) ) );
		CodeChunk saturation = ( colorMax - colorMin ) / colorMax;
		// Rescale saturation so only very desaturated areas are left unshifted.
		saturation = Saturate( ( saturation - m_satThresholdLow ) / ( m_satThresholdHigh - m_satThresholdLow ) );

		CodeChunk finalMask = shaderCompiler.Var( MDT_Float, CodeChunk::EMPTY );
		// Final blend factor comes from both saturation and our mask.
		shaderCompiler.Statement( CodeChunk::Printf( keepGray.IsConst() && mask.IsConst() && saturation.IsConst(),
				"%s if ( %s ) { %s = %s; }"
				"else { %s = %s; }",
				( keepGray.IsConst() ? "" : "[branch]" ),
			keepGray.AsChar(),
			finalMask.AsChar(), ( mask * saturation ).AsChar(),
			finalMask.AsChar(), mask.AsChar()
			) );

			CodeChunk finalColor = Lerp( color, combined, finalMask );

			shaderCompiler.Statement( CodeChunk::Printf( output.IsConst() && color.IsConst() && combined.IsConst() && finalMask.IsConst(),
				"%s = %s;", output.AsChar(), finalColor.AsChar() ) );
		}
		// if enabled
		//////////////////////////////////////////////////////////////////////////

		shaderCompiler.Statement( CodeChunk::Printf( false,
			"} else {",
			enabled.AsChar() ) );

		//////////////////////////////////////////////////////////////////////////
		// If !enabled
		{
			shaderCompiler.Statement( CodeChunk::Printf( output.IsConst() && color.IsConst(),
				"%s = %s;", output.AsChar(), color.AsChar() ) );
		}
		// if !enabled
		//////////////////////////////////////////////////////////////////////////

		shaderCompiler.Statement( CodeChunk::Printf( false, "}" ) );

		return output;
	}

	virtual Uint32 CalcRenderingFragmentParamMask() const
	{
		return RFMP_ColorShiftMatrices;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockShiftColorRB )
	PARENT_CLASS( CMaterialBlock )
	PROPERTY_EDIT_RANGE( m_colorThresholdLow, TXT("Low: areas with small amounts of red/blue are affected by the color shift. High: prevent bleeding to other areas, but some areas might not be shifted."), 0.0f, 0.5f )
	PROPERTY_EDIT_RANGE( m_colorThresholdHigh, TXT("Low: max effect in areas with variation in color. High: max effect limited to areas of almost pure red/blue"), 0.0f, 1.0f )
	PROPERTY_EDIT_RANGE( m_satThresholdLow, TXT("Must be less than satThresholdHigh. Low: sharper edges between shifted and gray areas. High: smoother transition."), 0.0f, 1.0f )
	PROPERTY_EDIT_RANGE( m_satThresholdHigh, TXT("Low: only very desaturated areas are unmodified. High: more tolerance for desaturated areas (may result in original color showing up around gray areas)."), 0.0f, 1.0f )
	END_CLASS_RTTI()


	IMPLEMENT_ENGINE_CLASS( CMaterialBlockShiftColorRB );

#endif
