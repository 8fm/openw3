/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "materialCompiler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

class CMaterialOutputSocket;
class CMaterialGraph;

/// Material compiler for material graph
class CMaterialBlockCompiler
{
protected:
	CMaterialGraph*								m_graph;											//!< Compiled graph
	IMaterialCompiler*							m_compiler;											//!< Low level compiler
	Red::Threads::CMutex															m_blocksMutex;
	THashMap< CMaterialBlock*, THashMap< CMaterialOutputSocket*, CodeChunk > >		m_blocks;		//!< Compiled block values
	TDynArray< CMaterialRootBlock* >				m_roots;										//!< Root blocks

public:
	// Get material graph we are compiling
	RED_INLINE CMaterialGraph* GetGraph() const { return  m_graph; }

	// Get internal material compiler
	RED_INLINE IMaterialCompiler* GetMaterialCompiler() const { return m_compiler; }

	// Get pixel shader compiler
	RED_INLINE IMaterialShaderCompiler& GetPS() { return *m_compiler->GetPixelShaderCompiler(); }

	// Get vertex shader compiler
	RED_INLINE IMaterialShaderCompiler& GetVS() { return *m_compiler->GetVertexShaderCompiler(); }

	// Get vertex shader compiler
	RED_INLINE IMaterialShaderCompiler& GetShader( EMaterialShaderTarget shaderTarget ) 
	{ 
		return (shaderTarget == MSH_PixelShader)? *m_compiler->GetPixelShaderCompiler() : *m_compiler->GetVertexShaderCompiler(); 
	}

	// Get hash map of compiled sockets
	RED_INLINE THashMap< CMaterialOutputSocket*, CodeChunk >* GetCompiledBlockOutputs( const CMaterialBlock* block )
	{
		CMaterialBlock* castBlock = const_cast< CMaterialBlock* >( block );
		return m_blocks.FindPtr( castBlock );
	}

	// Is terrain shader?
	Bool IsTerrainShader() const;

public:
	CMaterialBlockCompiler( CMaterialGraph* graph, IMaterialCompiler* compiler );
	~CMaterialBlockCompiler();

	// Compile roots, returns number of roots
	void CompileRoots();

	// Compile block output
	CodeChunk CompileBlock( CMaterialBlock* block, EMaterialShaderTarget shaderTarget, const CMaterialOutputSocket* socket = NULL );
};

#endif