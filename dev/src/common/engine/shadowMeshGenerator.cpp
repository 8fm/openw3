
#include "build.h"
#include "shadowMeshGenerator.h"

#include "mesh.h"
#include "layer.h"
#include "meshComponent.h"
#include "staticMeshComponent.h"

#ifndef NO_RESOURCE_IMPORT

CEntity* MeshUtilities::ExtractShadowMesh( const TDynArray< CEntity* >& entities, CLayer* layer, const Vector& pivotPos )
{
	THandle< CMesh > shadowMesh = CreateObject< CMesh >();

	for ( CEntity* entity : entities )
	{
		entity->DestroyStreamedComponents( SWN_NotifyWorld );
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );

		for ( ComponentIterator< CMeshComponent > it( entity ); it; ++it )
		{
			Matrix m = (*it)->GetLocalToWorld();
			m.SetTranslation( m.GetTranslation() - pivotPos );

			if ( CMesh* partialShadowMesh = (*it)->GetMeshNow()->ExtractCollapsed( MVT_StaticMesh ) )
			{
				shadowMesh->AddMesh( *partialShadowMesh, m, true, false );
				partialShadowMesh->Discard();
			}
		}
	}

	shadowMesh->MergeChunks( false, nullptr, false );
	shadowMesh->SetAutoHideDistance( 100.f );
	//shadowMesh->RecomputeDefaultRenderMask();
	//shadowMesh->RecomputeShadowFadeDistance();

	// = Spawn shadow mesh entity =

	EntitySpawnInfo info;
	info.m_template = nullptr;
	info.m_spawnPosition = pivotPos;
	CEntity* newEntity = layer->CreateEntitySync( info );

	CComponent* meshComp = newEntity->CreateComponent( ClassID< CStaticMeshComponent >(), SComponentSpawnInfo() );
	meshComp->SetResource( shadowMesh );

	//shadowMesh->SaveAs( GDepot, TXT("shadow_mesh_test.w2mesh"), true );
	
	return newEntity;
}

#endif
