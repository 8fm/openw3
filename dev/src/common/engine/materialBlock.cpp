/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialBlock.h"
#include "graphSocket.h"
#include "materialInputTextureSocket.h"
#include "materialOutputTextureSocket.h"
#include "materialInputSocket.h"
#include "materialOutputSocket.h"
#include "materialOutputBoolSocket.h"
#include "materialOutputCubeSocket.h"
#include "materialGraph.h"
#include "materialInputBoolSocket.h"
#include "materialInputCubeSocket.h"


IMPLEMENT_ENGINE_CLASS( CMaterialBlock );

Bool CMaterialBlock::IsInvariant() const
{
	return false;
}

Bool CMaterialBlock::IsRootBlock() const
{
	return false;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

Uint32 CMaterialBlock::CalcRenderingFragmentParamMask() const
{
	return 0;
}

Bool CMaterialBlock::HasInput( const CName& name ) const
{
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket* socket = m_sockets[i];
		if ( socket && socket->GetName() == name )
		{
			if ( socket->IsA< CMaterialInputSocket >() )
			{
				return socket->HasConnections();
			}

			if ( socket->IsA< CMaterialInputBoolSocket >() )
			{
				return socket->HasConnections();
			}

			if ( socket->IsA< CMaterialInputCubeSocket >() )
			{
				return socket->HasConnections();
			}

			if ( socket->IsA< CMaterialInputTextureSocket >() )
			{
				return socket->HasConnections();
			}
		}
	}

	return false;
}

CodeChunk CMaterialBlock::CompileInput( CMaterialBlockCompiler& compiler, const CName& name, EMaterialShaderTarget shaderTarget, const CodeChunk& defaultValue /*=CodeChunk::EMPTY*/ ) const
{
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket* socket = m_sockets[i];
		if ( socket && socket->GetName() == name )
		{
			if ( socket->IsA< CMaterialInputSocket >() )
			{
				CMaterialInputSocket* is = static_cast< CMaterialInputSocket* >( socket );
				return is->Compile( compiler, shaderTarget, defaultValue );
			}

			if ( socket->IsA< CMaterialInputTextureSocket >() )
			{
				CMaterialInputTextureSocket* is = static_cast< CMaterialInputTextureSocket* >( socket );
				return is->Compile( compiler, shaderTarget );
			}
		}
	}

	// Default value
	return defaultValue ? defaultValue : CodeChunk( 0.0f );
}

Int32 CMaterialBlock::GetShaderTarget() const
{
	Int32 target = 0;

	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket* socket = m_sockets[i];
		if ( socket )
		{
			if ( socket->IsA< CMaterialOutputSocket >() )
			{
				target |= SafeCast<CMaterialOutputSocket>(socket)->GetShaderTarget();
			}

			if ( socket->IsA< CMaterialOutputTextureSocket >() )
			{
				target |= SafeCast<CMaterialOutputTextureSocket>(socket)->GetShaderTarget();
			}

			if ( socket->IsA< CMaterialOutputBoolSocket >() )
			{
				target |= SafeCast<CMaterialOutputBoolSocket>(socket)->GetShaderTarget();
			}

			if ( socket->IsA< CMaterialOutputCubeSocket >() )
			{
				target |= SafeCast<CMaterialOutputCubeSocket>(socket)->GetShaderTarget();
			}
		}
	}

	return target;
}


#endif
#ifndef NO_EDITOR_GRAPH_SUPPORT
Color CMaterialBlock::GetTitleColor() const
{
	if ( GetBlockCategory() == TXT("Deprecated")) // Temporary visual indicator to help removing deprecated nodes
	{
		return Color::RED;
	}

 	return TBaseClass::GetTitleColor();
}

Color CMaterialBlock::GetBorderColor() const
{
	if ( GetBlockCategory() == TXT("Deprecated")) // Temporary visual indicator to help removing deprecated nodes
	{
		return Color::RED;
	}

	return TBaseClass::GetBorderColor();
}

#endif

void CMaterialBlock::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );
}
