#include "build.h"
#include "meshGcProxy.h"
#include "mesh.h"
#include "meshEnum.h"

IMPLEMENT_ENGINE_CLASS( CMeshGCProxy );

CMeshGCProxy::CMeshGCProxy()
{
	m_ownedMeshes.Reserve( 64 );
}

CMeshGCProxy::~CMeshGCProxy()
{
	RED_ASSERT( m_ownedMeshes.Size() == 0, TXT( "Some streamed meshes were not removed!" ) );
	m_ownedMeshes.Clear();
}

void CMeshGCProxy::AddMesh( CMesh* theMesh )
{
	theMesh->SetParent( this );
	m_ownedMeshes.PushBack( theMesh );
}

void CMeshGCProxy::RemoveMesh( CMesh* theMesh )
{
	theMesh->SetParent( nullptr );
	m_ownedMeshes.Remove( theMesh );
}
