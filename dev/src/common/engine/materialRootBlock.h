/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialBlock.h"

enum ERenderingBlendMode : CEnum::TValueType;

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Root block - a block that defines material property
class CMaterialRootBlock : public CMaterialBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialRootBlock, CMaterialBlock );

public:
	// Is block output value invariant ?
	virtual Bool IsInvariant() const;

	// Is this a root block ?
	virtual Bool IsRootBlock() const;

	// Is this a two sided material ?
	virtual Bool IsTwoSided() const=0;

	// Get rendering blend mode.
	virtual ERenderingBlendMode GetBlendMode() const=0;

	// Is this material producing emissive light
	virtual Bool IsEmissive() const=0;

	// Compile block, default compile function overridden, this should never be called
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
	{
		// This should be only used for debug reasons
		Compile(compiler);
		return CodeChunk::EMPTY;
	}

	// Compile block, root blocks decide on shader target and they don't return values
	virtual void Compile( CMaterialBlockCompiler& compiler ) const=0;
	
	// Is this material using mask
	virtual Bool IsMasked() const;

	// Can a material instance enable/disable masking?
	virtual Bool CanOverrideMasked() const { return true; }

	// Is this material accumulatively refracted
	virtual Bool IsAccumulativelyRefracted() const=0;

	// Is this material generating reflection mask
	virtual Bool IsReflectiveMasked() const { return false; }

	// Is this mimic material?
	virtual Bool IsMimicMaterial() const { return false; }

	// Is this skin material?
	virtual Bool IsSkin() const { return false; }

	virtual Int32 GetShaderTarget() const
	{
		return MSH_PixelShader;
	}

	virtual Bool IsDeferred() const = 0;
	virtual Bool IsForward() const = 0;

	virtual void GetValidPasses( TDynArray< ERenderingPass >& passes ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialRootBlock );
	PARENT_CLASS( CMaterialBlock );
END_CLASS_RTTI();

#endif
