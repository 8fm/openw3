/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "..\..\common\engine\foliageEditionController.h"

namespace WorldSceneDependencyInfo
{

	TDynArray<StringAnsi> GetEntityInfo( CEntity* currentEntity, const String layerPath );
	TDynArray<StringAnsi> GetComponentInfo( const CComponent* component, const String entityName );

	TDynArray< StringAnsi > GetMeshInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetParticleInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetLightInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetSoundEmitterInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetExplorationInfo( const CComponent* component );
	TDynArray< StringAnsi > GetDestructionInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetClothInfo( const CComponent* component, const String entityName );

	TDynArray< StringAnsi > GetMaterialInfo( IMaterial* material, Uint32 nr=0 );

	void MakeNiceString( String& myString );
	TDynArray< StringAnsi > MakeDBString( TDynArray< String > dbStrings );
	void Start( CWorld* world, String folderToDepFiles, Bool onlyFoliage = false );
	TDynArray< StringAnsi > AddMaterialInfo( const CMesh* meshResource, const String entityName );
	TDynArray< StringAnsi > AddCollisionMeshInfo( const CMesh* meshResource );
	TDynArray< StringAnsi > AddChunkInfo( const CMesh* meshResource );
	TDynArray< StringAnsi > AddLodInfo( const CMesh* meshResource );
	TDynArray< StringAnsi > AddBaseMaterialInfo( IMaterial* baseMaterial );
	String GetGuid( const CComponent* component );
	TDynArray< StringAnsi > GetFlareInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetFoliageInfo( CFoliageCellIterator foliageIterator, String worldName );
	TDynArray< StringAnsi > GetDimmerInfo( const CComponent* component, const String entityName );
	TDynArray< StringAnsi > GetAreaInfo( const CComponent* component, const String entityName );
};