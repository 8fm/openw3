/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderMesh.h"

class CPhysicsDestructionResource;
struct SBoneIndiceMapping;
struct SBoneIndicesHelper;

class CRenderDestructionMesh : public CRenderMesh
{
	DECLARE_RENDER_RESOURCE_ITERATOR;

private:
	CRenderMesh* m_originalRenderMesh;

protected:
	CRenderDestructionMesh();

public:
	CRenderDestructionMesh( CRenderMesh* toCopy );

	void FinalizeLoading( );

	virtual ~CRenderDestructionMesh();

	Bool UpdateActiveIndices( const TDynArray<Uint16>& indices, const TDynArray< Uint32 >& newOffsets, const TDynArray< Uint32 >& newNumIndices  );

private:
	Bool UpdateBuffer( const TDynArray< Uint16 >& activeIndices );

};
