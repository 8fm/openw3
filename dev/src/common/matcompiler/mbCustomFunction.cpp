/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/codeParser.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/materialGraph.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#ifndef NO_EDITOR_GRAPH_SUPPORT
static Bool GetParamTypeByString( const String& name, EMaterialParamType& paramType )
{
	if (name == TXT("float"))
	{
		paramType = MPT_Float;
	}
	else if (name == TXT("float2"))
	{
		paramType = MPT_Float2;
	}
	else if (name == TXT("float3"))
	{
		paramType = MPT_Float3;
	}
	else if (name == TXT("float4"))
	{
		paramType = MPT_Float4;
	}
	else if (name == TXT("sampler2D"))
	{
		paramType = MPT_Texture;
	}
	else
	{
		return false;
	}
	
	return true;
}
#endif // NO_EDITOR_GRAPH_SUPPORT

/// Converts value in gamma space into linear space
class CMaterialBlockCustomFunction : public CMaterialBlock
{
	//DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockCustomFunction, CMaterialBlock, "Special", "CustomFunction" ); HACK
	DECLARE_ENGINE_CLASS( CMaterialBlockCustomFunction, CMaterialBlock, 0 );
	virtual String GetBlockName() const
	{
		if (m_functionName.Empty())
		{
			return TXT("Custom Function");
		}
		else
		{
			return String::Printf(TXT("%ls()"), m_functionName.AsChar());
		}
	}
	virtual String GetBlockCategory() const { return TXT("Misc"); }

protected:
	String m_functionReturnType;
	String	m_functionName;
	THashMap<String, EMaterialParamType> m_functionParams;
	String	m_functionContent;
	String	m_customCode;
	mutable CodeChunk m_pixelVar;
	mutable CodeChunk m_vertexVar;
	mutable Red::Threads::CMutex m_mutex;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnPostLoad()
	{
		TBaseClass::OnPostLoad();
		OnPropertyPostChange(NULL);
	}

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );

		for (THashMap< String, EMaterialParamType >::iterator it=m_functionParams.Begin(); it!=m_functionParams.End(); ++it)
		{
			switch(it->m_second)
			{
			case MPT_Float:
			case MPT_Float2:
			case MPT_Float3:
			case MPT_Float4:
				CreateSocket( MaterialInputSocketSpawnInfo( CName(it->m_first.AsChar()) ) );
				break;
			case MPT_Texture:
				CreateSocket( MaterialInputTextureSocketSpawnInfo( CName(it->m_first.AsChar()) ) );
				break;
			default:
				ASSERT(false);
			}
		}
	}

	virtual void OnPropertyPostChange( IProperty *property )
	{
		// Setup a code parser with C-like scanning settings (mostly
		// the default behaviour, minus the standard C/C++ delimiters
		// which we need to manually add)
		CCodeParser parser( m_customCode );
		parser.AddStandardDelimiters();

		// Scan return type (expected to be the first thing)
		m_functionReturnType = parser.ScanToken();

		// Scan the function name
		m_functionName = parser.ScanToken();

		// Skip to opening parentheses for the arguments
		parser.SkipToCharacter( TXT('(') );

		// Parse the parameters
		m_functionParams.Clear();
		if ( parser.GetNextCharacter() == TXT('(') )
		{
			// Skip opening parenthesis
			parser.Skip();

			// Parse each parameter
			while ( parser.HasMore() )
			{
				String paramType = parser.ScanToken();
				String paramName = parser.ScanToken();

				// Make sure we haven't scanned incomplete code
				if ( !paramType.Empty() && !paramName.Empty() )
				{
					EMaterialParamType realParamType;
					if ( !GetParamTypeByString( paramType, realParamType ) )
					{
						// Unknown parameter type, probably scanning
						// incomplete code - so, stop now
						return;
					}
					m_functionParams.Insert( paramName, realParamType );
				}

				// Skip any whitespace
				parser.SkipWhitespace();

				// If there is a closing parenthesis, we are at the end of
				// the function parameters so skip it and stop
				if ( parser.GetNextCharacter() == TXT(')') )
				{
					parser.Skip();
					break;
				}

				// If there is anything else than a comma, we're probably
				// parsing incomplete code so stop everything and return
				if ( parser.GetNextCharacter() != TXT(',') )
				{
					return;
				}

				// Comma, so skip it
				parser.Skip();
			}
		}

		// Skip to the function's code block
		parser.SkipToCharacter( TXT('{') );

		// Scan the whole block
		m_functionContent = parser.ScanCBlock();

		// Rebuild all the things
		OnRebuildSockets();
		GetGraph()->GraphStructureModified();

#if 0
		
		// parse code and get function name and sockets
		// TODO improve
		m_functionReturnType = TXT("float4");

		size_t funcHeaderStart;
		m_customCode.FindSubstring(TXT("float4 "), funcHeaderStart);

		size_t blockStart;
		m_customCode.FindSubstring(TXT("{"), blockStart);

		String funcHeader = m_customCode.MidString( funcHeaderStart, blockStart - funcHeaderStart );
		size_t funcNameStart = funcHeaderStart + 7;
		size_t funcNameEnd;
		funcHeader.FindSubstring(TXT("("), funcNameEnd);

		m_functionName = funcHeader.MidString( funcNameStart, funcNameEnd - funcNameStart );
		m_functionName.Trim();

		size_t funcParamsBegin = funcNameEnd+1;
		size_t funcParamsEnd; 
		funcHeader.FindSubstring(TXT(")"), funcParamsEnd);
		String funcParams = funcHeader.MidString( funcParamsBegin, funcParamsEnd - funcParamsBegin );

		m_functionParams.Clear();
		TDynArray<String> params;
		funcParams.GetTokens( TXT(','), true, params );
		for (Uint32 i = 0; i < params.Size(); ++ i)
		{
			params[i].Trim();
			size_t typeEnd;
			params[i].FindSubstring(TXT(" "), typeEnd);

			EMaterialParamType paramType = MPT_Float;
			ASSERT( GetParamTypeByString(params[i].LeftString(typeEnd), paramType), TXT("Unknown parameter type") );
			String paramName = params[i].RightString( params[i].Size() - typeEnd - 1);
			m_functionParams.Insert(paramName, paramType);
		}

		size_t blockEnd;
		m_customCode.FindSubstring(TXT("}"), blockEnd);

		m_functionContent = m_customCode.MidString(blockStart+1, blockEnd - (blockStart + 1));

		OnRebuildSockets();

		GetGraph()->GraphStructureModified();
#endif 

	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		if (!m_functionName.Empty())
		{
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_mutex );
				Bool alreadyExists = compiler.GetMaterialCompiler()->Function(m_functionName, m_functionContent, m_functionParams, shaderTarget);
				if ( alreadyExists )
				{
					if ( shaderTarget == MSH_PixelShader)
					{
						if ( !m_pixelVar.IsEmpty() )
						{
							return m_pixelVar;
						}
					}
					else
					{
						if ( !m_vertexVar.IsEmpty() )
						{
							return m_vertexVar;
						}
					}
				}
			}

			StringAnsi callParams;
			//TODO ordering problems might occur needs better solution
			for ( auto& socket : m_sockets )
			{
				if ( socket )
				{
					if ( socket->IsA< CMaterialInputSocket >() )
					{
						EMaterialParamType paramType;
						String socketName = socket->GetName().AsString();
						socketName.Trim();
						m_functionParams.Find(socketName, paramType);
						CodeChunk defaultChunk = CodeChunk::Printf(false, "%s(1)", UNICODE_TO_ANSI(compiler.GetMaterialCompiler()->GetMaterialParamTypeName(paramType).AsChar()));

						CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );

						String swizzling = TXT("");
						if (paramType == MPT_Float)
						{
							swizzling = TXT(".x");
						}
						if (paramType == MPT_Float2)
						{
							swizzling = TXT(".xy");
						}
						if (paramType == MPT_Float3)
						{
							swizzling = TXT(".xyz");
						}
						if (paramType == MPT_Float4)
						{
							swizzling = TXT(".xyzw");
						}

						String socketCode = ANSI_TO_UNICODE( is->Compile( compiler, shaderTarget, defaultChunk ).AsChar());
						callParams += UNICODE_TO_ANSI( String::Printf( TXT("( %ls )%ls, "), socketCode.AsChar(), swizzling.AsChar() ).AsChar() );
					}
					else if ( socket->IsA< CMaterialInputTextureSocket >() )
					{
						EMaterialParamType paramType;
						m_functionParams.Find(socket->GetName().AsString(), paramType);

						CMaterialInputTextureSocket* is = static_cast< CMaterialInputTextureSocket* >( socket );

						String socketCode = ANSI_TO_UNICODE( is->Compile( compiler, shaderTarget ).AsChar());
						callParams += UNICODE_TO_ANSI( String::Printf( TXT("%ls, "), socketCode.AsChar() ).AsChar() );
					}
				}
			}
			if (m_functionParams.Size())
			{
				size_t substringIndex = 0;
				callParams.FindSubstring(",", substringIndex, true);
				callParams = callParams.MidString(0, substringIndex);
			}

			CodeChunk call = CodeChunk::Printf( false, "%ls(%hs)", m_functionName.AsChar(), callParams.AsChar() );
			CodeChunk var = compiler.GetShader(shaderTarget).Var( MDT_Float4, call );

			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_mutex );
				if (shaderTarget == MSH_PixelShader)
				{
					m_pixelVar = var;
				}
				else
				{
					m_vertexVar = var;
				}
			}

			return var;

		}
		else
		{
			return CodeChunk::EMPTY;
		}
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockCustomFunction )
	PARENT_CLASS( CMaterialBlock )
	PROPERTY_EDIT( m_customCode, TXT("Custom code") );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CMaterialBlockCustomFunction );

#endif