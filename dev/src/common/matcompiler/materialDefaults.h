/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


/// Materials standard parameters defaults

const Float		MATERIAL_DEFAULT_MASK			= 1.0f;
const Vector	MATERIAL_DEFAULT_OUTPUT_COLOR	= Vector ( 0.7f, 0.7f, 0.7f, 1.0f );
const Vector	MATERIAL_DEFAULT_AMBIENT		= Vector ( 0.0f, 0.0f, 0.0f, 0.0f );
const Vector	MATERIAL_DEFAULT_EMISSIVE		= Vector ( 0.0f, 0.0f, 0.0f, 1.0f );
const Vector	MATERIAL_DEFAULT_DIFFUSE		= Vector ( 0.5f, 0.5f, 0.5f, 1.0f );
const Vector	MATERIAL_DEFAULT_DIFFUSE_GBUFFER= Vector ( 0.7f, 0.7f, 0.7f, 1.0f );
const Vector	MATERIAL_DEFAULT_SPECULARITY	= Vector ( 0.0f, 0.0f, 0.0f, 1.0f );
const Vector	MATERIAL_DEFAULT_REFRACTIONDELTA= Vector ( 0.0f, 0.0f, 0.0f, 0.0f );
const Vector	MATERIAL_DEFAULT_REFLECTIONMASK = Vector ( 0.0f, 0.0f, 0.0f, 0.0f );
const Float		MATERIAL_DEFAULT_GLOSSINESS		= 20.f;
const Float		MATERIAL_DEFAULT_ROUGHNESS		= 0.5f;
const Float		MATERIAL_DEFAULT_AO				= 1.0f;
const Float		MATERIAL_DEFAULT_SS				= 0.0f;
const Float		MATERIAL_DEFAULT_TRANSLUCENCY	= 0.0f;
const Float		MATERIAL_DEFAULT_MATERIAL_FLAGS_MASK_ENCODED	= 0.0f; //GBUFF_MATERIAL_MASK_ENCODED_DEFAULT

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

class CodeChunk;
enum ERenderingPass : Int32;

/// Materials standard transformations
void ApplyMaterialSelectionColor3v( CodeChunk &color, ERenderingPass pass );
void ApplyMaterialSelectionColor4v( CodeChunk &color, ERenderingPass pass );

#endif