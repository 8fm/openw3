/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Render feedback data fetch
class CMaterialBlockRenderFeedbackDataFetch : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockRenderFeedbackDataFetch, CMaterialBlock, "System Samplers", "Render Feedback Data" );

	ERenderFeedbackDataType m_feedbackDataType;

public:
	CMaterialBlockRenderFeedbackDataFetch ()
		: m_feedbackDataType( RFDT_Depth )
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Offset ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		using namespace CodeChunkOperators;

		CodeChunk result = Float4 ( 1.f, 1.f, 1.f, 1.f );

		if ( compiler.GetPS().IsFeedbackDataFetchSupported( m_feedbackDataType ) )
		{	
			// Calculate offset
			CodeChunk offset;
			if ( HasInput(CNAME(Offset)) )
			{
				CodeChunk ratio = compiler.GetPS().ConstReg( MDT_Float4, "PSC_ViewportSubSize" ).zw();
				offset = ratio * CompileInput( compiler, CNAME(Offset), shaderTarget ).xy();
			}

			// Compile feedback data
			compiler.GetPS().CompileFeedbackDataFetch( m_feedbackDataType, result, offset.IsEmpty() ? NULL : &offset );
		}

		return result;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockRenderFeedbackDataFetch )
	PARENT_CLASS(CMaterialBlock)	
	PROPERTY_EDIT( m_feedbackDataType, TXT("Feedback data type") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockRenderFeedbackDataFetch );

#endif