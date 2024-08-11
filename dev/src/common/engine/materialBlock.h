/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "graphBlock.h"
#include "materialCompiler.h"

class CMaterialBlockCompiler;

/// Base material block
class CMaterialBlock : public CGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialBlock, CGraphBlock );

public:
	// Get material graph
	RED_INLINE CMaterialGraph* GetGraph() const { return (CMaterialGraph*)GetParent(); }

public:
	// Is block output value invariant ?
	virtual Bool IsInvariant() const;

	// Is this a root block ?
	virtual Bool IsRootBlock() const;

	//! Property changed
	virtual void OnPropertyPostChange( IProperty *property );

#ifndef NO_EDITOR_GRAPH_SUPPORT

	// Title color
	virtual Color GetTitleColor() const;

	// Border color
	virtual Color GetBorderColor() const;
#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

	// Compile block, non root blocks should return value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const=0;

	// Calculate parameter mask for mesh rendering fragment
	virtual Uint32 CalcRenderingFragmentParamMask() const;

	// Get the shader target(s) that this block is compiled for
	virtual Int32 GetShaderTarget() const;

protected:
	// Returns true if we have something connected to input socket
	Bool HasInput( const CName& name ) const;

	// Compile socket input
	CodeChunk CompileInput( CMaterialBlockCompiler& compiler, const CName& name, EMaterialShaderTarget shaderTarget, const CodeChunk& defaultValue = CodeChunk::EMPTY ) const;

#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialBlock );
	PARENT_CLASS( CGraphBlock );
END_CLASS_RTTI();

#ifndef NO_EDITOR_GRAPH_SUPPORT

#define DECLARE_ENGINE_MATERIAL_BLOCK( _class, _superclass, _category, _caption ) \
	DECLARE_ENGINE_CLASS( _class, _superclass, 0 );	\
	virtual String GetBlockName() const { return TXT(_caption); }	\
	virtual String GetBlockCategory() const { return TXT(_category); }

#define DECLARE_ENGINE_MATERIAL_PARAMETER( _class, _superclass, _category, _caption ) \
	DECLARE_ENGINE_CLASS( _class, _superclass, 0 );	\
	virtual String GetBlockName() const { return TXT(_caption); }	\
	virtual String GetBlockCategory() const { return TXT(_category); } \
	virtual String GetCaption() const { return FormatParamCaption( TXT(_caption) ); }

#else

#define DECLARE_ENGINE_MATERIAL_BLOCK( _class, _superclass, _category, _caption ) \
	DECLARE_ENGINE_CLASS( _class, _superclass, 0 );

#define DECLARE_ENGINE_MATERIAL_PARAMETER( _class, _superclass, _category, _caption ) \
	DECLARE_ENGINE_CLASS( _class, _superclass, 0 );

#endif