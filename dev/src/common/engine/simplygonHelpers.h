/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef USE_SIMPLYGON

// Undef some stuff that simplygon wants
#ifdef REAL_MIN
#undef REAL_MIN
#endif
#ifdef REAL_MAX
#undef REAL_MAX
#endif

#include "../../../external/Simplygon/SimplygonSDK.h"
#include "../core/feedback.h"
#include "mesh.h"


struct SMeshChunk;


namespace SimplygonHelpers
{
	using namespace SimplygonSDK;

	//!
	class SimplygonProgressObserver : public robserver
	{
	public:
		virtual void Execute( IObject* subject, rid EventId, void* EventParameterBlock, unsigned int EventParameterBlockSize ) override;
	};

	//!
	Int32 InitSDK( ISimplygonSDK*& sdk );

	//!
	void ShutdownSDK();

	//!
	Bool ExportToObj( ISimplygonSDK* sgSDK, spScene scene, const String& fileName );

	//!
	spScene ImportFromObj( ISimplygonSDK* sgSDK, const String& fileName );

	//!
	const Char* GetErrorText( Int32 code );

	//! Extract vertex/index data from a mesh chunk, pack it into a Simplygon IGeometryData.
	//! \param mtlIdOffset an offset added to the material IDs set for each triangle. Normally, this can be 0, but when dealing with chunks from multiple meshes, may be useful.
	//! \param fixWinding if true, the triangle winding order will be reversed.
	spPackedGeometryData CreateGeometryFromMeshChunk( ISimplygonSDK* sgSDK, const SMeshChunk& chunk, Uint32 mtlIdOffset, Bool fixWinding );

	//!
	void ReduceGeometry( ISimplygonSDK* sgSDK, spScene scene, const SLODPresetDefinition& lodDefinition, Bool showProgress );

	//!
	void RemeshGeometry( ISimplygonSDK* sgSDK, spScene scene, Bool showProgress );

	//!
	void CreateMeshChunkFromGeometry( ISimplygonSDK* sgSDK, spPackedGeometryData geom, Bool fixWinding, SMeshChunk& outChunk );

}

#endif
