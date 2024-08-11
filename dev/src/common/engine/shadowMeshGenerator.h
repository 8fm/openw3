
#pragma once

class CEntity;
class CLayer;

#ifndef NO_RESOURCE_IMPORT

namespace MeshUtilities
{
	CEntity* ExtractShadowMesh( const TDynArray< CEntity* >& entities, CLayer* layer, const Vector& pivotPos );
}

#endif