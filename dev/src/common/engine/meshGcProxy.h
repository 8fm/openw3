#ifndef MESH_GC_PROXY_H_INCLUDED
#define MESH_GC_PROXY_H_INCLUDED
#pragma once

// This class takes ownership of streamed CMesh objects whose lifetime is governed by the streaming system
// This stops them from being garbage collected early
class CMeshGCProxy : public CObject
{
	DECLARE_ENGINE_CLASS( CMeshGCProxy, CObject, 0 );
public:
	CMeshGCProxy();
	~CMeshGCProxy();

	void AddMesh( CMesh* theMesh );
	void RemoveMesh( CMesh* theMesh );

private:
	TDynArray< CMesh* > m_ownedMeshes;
};

BEGIN_CLASS_RTTI( CMeshGCProxy )
	PARENT_CLASS( CObject );
	PROPERTY_RO( m_ownedMeshes, TXT("Owned Meshes") );
END_CLASS_RTTI();

#endif