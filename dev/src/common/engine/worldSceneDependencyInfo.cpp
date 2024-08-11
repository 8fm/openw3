/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#ifndef NO_EDITOR

#include "../../common/engine/worldSceneDependencyInfo.h"
//#include "worldSceneDependencyInfo.h"
//#include "meshStats.h"
#include "../../common/engine/mesh.h"
#include "../../common/game/expComponent.h"
#include "../../common/core/feedback.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/engine/clipMap.h"
#include "../../common/core/directory.h"
#include "../../common/core/names.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/particleComponent.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/particleEmitter.h"
#include "../../common/engine/particleModule.h"
#include "../../common/engine/lightComponent.h"
#include "../../common/engine/flareComponent.h"
#include "../../common/engine/material.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionShape.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/materialRootBlock.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/apexResource.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/foliageCellIterator.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/baseTree.h"
#include "../../common/matcompiler/mbOutputVertexModifiers.h"
#include "../../common/engine/foliageBroker.h"
#include "../../common/engine/foliageCell.h"
#include "../../common/engine/texture.h"
#include "../../common/engine/bitmapTexture.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "entity.h"
#include "soundEmitter.h"
#include "dimmerComponent.h"
#include "foliageResourceLoader.h"
#include "areaComponent.h"
#include "triggerAreaComponent.h"
#include "../core/bitField.h"
#include "pathlibRoughtTerrainComponent.h"
#include "pathlibNavmeshComponent.h"
#include "../core/2darray.h"

RED_DEFINE_STATIC_NAME( ELayerBuildTagEnumBuilder );

using namespace WorldSceneDependencyInfo;

TDynArray< String > g_corruptedLines;

void WorldSceneDependencyInfo::Start( CWorld* world, String folderToDepFiles, Bool onlyFoliage )
{
	g_corruptedLines.Clear();

	CMesh::ActivatePipelineMesh();
	String worldName = world->GetFile()->GetDirectory()->GetName();
	TDynArray<StringAnsi> myText;
	myText.PushBack("CREATE TABLE IF NOT EXISTS Worlds (id INTEGER PRIMARY KEY, name, size);\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Layers (id INTEGER PRIMARY KEY, path, worldid INTEGER, static, buildTag, layerName, layerType, FOREIGN KEY(worldid) REFERENCES Worlds(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS FoliageTiles (id INTEGER PRIMARY KEY, worldName, flyrFile, treeCounter INTEGER, grassCounter INTEGER, pos);\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Entities (id INTEGER PRIMARY KEY, name, layerid INTEGER, class, componentsCount INTEGER, componentsClassNames, templatePath, posx INTEGER, posy INTEGER, posz INTEGER, bbminx INTEGER, bbminy INTEGER, bbminz INTEGER, bbmaxx INTEGER, bbmaxy INTEGER, bbmaxz INTEGER, volume INTEGER, width INTEGER, depth INTEGER, height INTEGER, drawableComp, FOREIGN KEY(layerid) REFERENCES Layers(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS EntityTemplates (id INTEGER PRIMARY KEY, path, layerPath, name, includedTemplates, allAppearanceNames, allUsedAppearanceNames, staticFlags);\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Appearances (id INTEGER PRIMARY KEY, entitytemplateid, name, bodyParts);\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS MeshComponents (_id INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, bbminx INTEGER, bbminy INTEGER, bbminz, bbmaxx, bbmaxy, bbmaxz, volume, width INTEGER, depth INTEGER, height INTEGER, lodCount INTEGER, materialCount INTEGER, triangleCount INTEGER, vertCount INTEGER, autoHideDistance INTEGER, shadowLodBias INTEGER, collisionShapeCount INTEGER, className, soundType, soundSize INTEGER, soundOcclusion, depotPath, importFile, author, guid, drawableFlags, lightChannels, shadowImportanceBias, forceLodLevel, isStreamed, useExtraStream, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS DestructionComponents (_id INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, bbminx INTEGER, bbminy INTEGER, bbminz INTEGER, bbmaxx INTEGER, bbmaxy INTEGER, bbmaxz INTEGER, materialCount INTEGER, depotPath, importFile, author, guid, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS ClothComponents (_id INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, bbminx INTEGER, bbminy INTEGER, bbminz INTEGER, bbmaxx INTEGER, bbmaxy INTEGER, bbmaxz INTEGER, maxSimulationDistance INTEGER, materialCount INTEGER, depotPath, importFile, author, guid, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Lods (id INTEGER PRIMARY KEY, meshcomponentid INTEGER, lodNR INTEGER, memorySizeCPU INTEGER, memorySizeGPU INTEGER, distance INTEGER, triangleCount INTEGER, vertCount INTEGER, chunkCount INTEGER, materialCount INTEGER, bones INTEGER, UNIQUE(meshcomponentid, lodNR), FOREIGN KEY(meshcomponentid) REFERENCES MeshComponents(_id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS CollisionShapes (id INTEGER PRIMARY KEY, meshcomponentid INTEGER, collisionShape, vertCount INTEGER, triangleCount INTEGER, material, UNIQUE(meshcomponentid, collisionShape, material), FOREIGN KEY(meshcomponentid) REFERENCES MeshComponents(_id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Chunks (id INTEGER PRIMARY KEY, meshcomponentid INTEGER, chunkIndex INTEGER, lodNr INTEGER, material, UNIQUE(meshcomponentid, chunkindex), FOREIGN KEY(meshcomponentid) REFERENCES MeshComponents(_id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Materials (id INTEGER PRIMARY KEY, meshcomponentid INTEGER, entityid INTEGER, destructionid INTEGER, clothid INTEGER, name, base, diffuse, normal, detail, detail1, detail2, spec, tintMask, usingMimics, blockNames, FOREIGN KEY(meshcomponentid) REFERENCES MeshComponents(_id), FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS BaseMaterials (id INTEGER PRIMARY KEY, path UNIQUE, rootBlockName, deprecatedBlocks, usingMimics, blockNames, canUseOnParticles, canUseOnMeshes, canUseOnCollapsableObjects, canUseOnTessellate);\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS MissingMaterials (id INTEGER PRIMARY KEY, meshcomponentid INTEGER, name, FOREIGN KEY(meshcomponentid) REFERENCES MeshComponents(_id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Textures (id INTEGER PRIMARY KEY, materialid INTEGER, name UNIQUE, width INTEGER, height INTEGER, format, textureGroup, compression, isUser, isStreamable, isResizable, isDetailMap, isAtlas, maxSize, hasMipchain, pcDownscaleBias, xboxDownscaleBias, ps4DownscaleBias, importFile, FOREIGN KEY(materialid) REFERENCES Materials(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Lights (_lightid INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, lightRadius INTEGER, lightEnabled, castingShadow, lightShadowMode, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS Dimmers (_dimmerid INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, ambientLevel, marginFactor, dimmerType, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	// AreaComponents
	myText.PushBack("CREATE TABLE IF NOT EXISTS AreaComponents (_id INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, areaType, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS TriggerAreaComponents (id INTEGER PRIMARY KEY, areaid INTEGER, triggerPriority, enabled, useCCD, tags, includedChannels, excludedChannels, FOREIGN KEY(areaid) REFERENCES AreaComponents(_areaid));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS PathLibRoughTerrainComponents (id INTEGER PRIMARY KEY, areaid INTEGER, isRoughTerrain, FOREIGN KEY(areaid) REFERENCES AreaComponents(_areaid));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS NavMeshComponents (id INTEGER PRIMARY KEY, areaid INTEGER, genericFileName, sharedFilePath, useGenerationRootPoints, useTerrainInGeneration, useStaticMeshesInGeneration, collectFoliage, previewOriginalGeometry, useCollisionMeshes, monotonePartitioning, detectTerrainConnection, stepOnNonWalkableMeshes, cutMeshesWithBoundings, smoothWalkableAreas, extensionLength, cellWidth, cellHeight, walkableSlopeAngle, agentHeight, margin, agentClimb, maxEdgeLen, maxEdgeError, regionMinSize, regionMergeSize, vertsPerPoly, detailSampleDist, detailSampleMaxError, extraStreamingRange, FOREIGN KEY(areaid) REFERENCES AreaComponents(_areaid));\n");
	// Particles
	myText.PushBack("CREATE TABLE IF NOT EXISTS ParticleSystems (_particlesystemid INTEGER PRIMARY KEY, entityid INTEGER, autohideDistance INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, depotPath, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS ParticleEmitters (id INTEGER PRIMARY KEY, particlesystemid INTEGER, enabled, maxParticles INTEGER, materialBase, FOREIGN KEY(particlesystemid) REFERENCES ParticleSystems(_particlesystemid));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS ParticleModules (id INTEGER PRIMARY KEY, particleemitterid INTEGER, name, isEnabled, UNIQUE(particleemitterid, name), FOREIGN KEY(particleemitterid) REFERENCES ParticleEmitters(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS ParticleMaterials (id INTEGER PRIMARY KEY, particleemitterid INTEGER UNIQUE, base, blockNames, textureCount, FOREIGN KEY(particleemitterid) REFERENCES ParticleEmitters(id));\n");
	myText.PushBack("CREATE TABLE IF NOT EXISTS ParticleTextures (id INTEGER PRIMARY KEY, particlematerialid INTEGER, name, width INTEGER, height INTEGER, format, textureGroup, compression, isUser, isStreamable, isResizable, isDetailMap, isAtlas, maxSize INTEGER, hasMipchain, pcDownscaleBias INTEGER, xboxDownscaleBias INTEGER, ps4DownscaleBias INTEGER, importFile, FOREIGN KEY(particlematerialid) REFERENCES ParticleMaterials(id));\n");
	// Sounds				
	myText.PushBack("CREATE TABLE IF NOT EXISTS SoundEmitterComponents (id INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, name, objectName, loopStart, loopStop, loopIntensity, maxDistance, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
	// Flare component			  
	myText.PushBack("CREATE TABLE IF NOT EXISTS FlareComponents (id INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");

	// INDEXES
	myText.PushBack("CREATE INDEX IF NOT EXISTS MeshCompDepotIndx ON MeshComponents (depotPath);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS MeshCompIDIndx ON MeshComponents (_id);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS ClothCompDepotIndx ON ClothComponents (depotPath);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS DestructionCompDepotIndx ON DestructionComponents (depotPath);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS LodsMeshCompIDIndx ON Lods (meshcomponentid);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS LodsNRIndx ON Lods (lodNR);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS MaterialsMeshCompIDIndx ON Materials (meshcomponentid);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS MaterialsNameIndx ON Materials (name);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS MaterialsEntityIDIndx ON Materials (entityid);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS LayersPathIndx ON Layers (path);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS EntitiesNameIndx ON Entities (name);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS EntitiesIDIndx ON Entities (id);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS ClothCompDepotIndx ON ClothComponents (depotPath);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS WorldsNameIndx ON Worlds (name);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS ParticleSystemsDepotIndx ON ParticleSystems (depotPath);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS ParticleEmittersParticleSystemIDIndx ON ParticleEmitters (particlesystemid);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS ParticleMaterialsParticleEmitterIDIndx ON ParticleMaterials (particleemitterid);\n");
	myText.PushBack("CREATE INDEX IF NOT EXISTS ParticleModulesParticleEmitterIDIndx ON ParticleModules (particleemitterid);\n");
	myText.PushBack("CREATE UNIQUE INDEX IF NOT EXISTS TextureNameIndx ON Textures (name);\n");

	// Dimensions of the world
	Float dim = world->GetTerrain()->GetTerrainSize();

	myText.PushBack( "insert into worlds (name, size) values ('" );
	myText.PushBack( UNICODE_TO_ANSI( worldName.AsChar() ) );
	myText.PushBack( "','" );
	myText.PushBack( UNICODE_TO_ANSI( ToString(dim).AsChar() ) );
	myText.PushBack("');\n");

	// File to save the data
	String pathWorld = folderToDepFiles;
	if ( !GFileManager->FileExist( pathWorld.AsChar() ) )
	{
		if( !GFileManager->CreatePath( pathWorld.AsChar() ) )
		{
			RED_LOG( Session, TXT("ERROR: Unable to create output folder '%ls'."), pathWorld.AsChar() );
		}
	}
	// Write the file here 
	FILE* nFile; 
	String depFilePath = pathWorld + worldName.AsChar() + TXT(".dep");
	if ( GFileManager->FileExist( depFilePath.AsChar() ) )
	{
		GFileManager->DeleteFileW( depFilePath.AsChar() );
	}

	// fullDependency means it will generate the whole dep file.
	// If this is false it will only generate the vegetation part of it
	if( !onlyFoliage )
	{
		Uint32 garbageCounter = 0;
		
		CLayerGroup* worldLayerGroup = world->GetWorldLayers();

		TDynArray< CLayerInfo* > layerInfos;
		worldLayerGroup->GetLayers( layerInfos, false, true, true );

		String fPath = worldLayerGroup->GetDepotPath();
		CDirectory* directory = world->GetFile()->GetDirectory();
		// Splitting of the folder
		String splitName = directory->GetName();

		LayerLoadingContext layerContext;
		layerContext.m_loadHidden = true;
		const Uint32 layerGroupSize = layerInfos.Size();

		// Progressbar for the generation of dep files
		GFeedback->BeginTask( TXT("Creating Dependency file"), false );
		GFeedback->UpdateTaskProgress( 0, layerGroupSize );

		for ( Uint32 j=0; j<layerInfos.Size(); ++j )
		{
			layerInfos[j]->SyncLoad(layerContext);

			if( layerInfos[j]->IsLoaded() )
			{
				// Get the path to this layer and make a proper name string for it
				CLayer *layer = layerInfos[j]->GetLayer();
				String isStaticString = ToString( layerInfos[j]->IsEnvironment() );
				ELayerType layerTypeEnum = layerInfos[j]->GetLayerType();
				String layerType = CEnum::ToString(layerTypeEnum).AsChar();
				String buildTag = SRTTI::GetInstance().FindEnum( CNAME( ELayerBuildTagEnumBuilder ) )->ToString( layerInfos[j]->GetLayerBuildTag() );

				String layerPath = layer->GetFile()->GetAbsolutePath();	
				String layerName = layer->GetFile()->GetFileName().Split( TXT(".") )[0];
				MakeNiceString( layerName );

				String splitString = TXT("\\")+splitName+TXT("\\");

				// Check to see if the split-string exists first in the layerpath 
				String nPath = layerPath.Split( splitString )[1];
				layerPath.ReplaceAll( TXT("'"), TXT("@") );
				String mPath = nPath.Split( TXT(".") )[0];
				mPath.ReplaceAll( TXT("\\"), TXT("__") );

				//myText.PushBack( UNICODE_TO_ANSI( TXT("\n\tLAYER: ") ) );
				myText.PushBack( "insert or ignore into layers (path, worldid, static, buildTag, layerName, layerType) values ('" );
				myText.PushBack( UNICODE_TO_ANSI( layerPath.AsChar() ) );
				myText.PushBack( "'," );
				myText.PushBack( "(select id from worlds where name='" );
				myText.PushBack( UNICODE_TO_ANSI( worldName.AsChar() ) );
				myText.PushBack( "' LIMIT 1),'" );
				myText.PushBack( UNICODE_TO_ANSI( isStaticString.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( buildTag.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( layerName.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( layerType.AsChar() ) );
				myText.PushBack( "');\n" );

				const LayerEntitiesArray& entities = layerInfos[j]->GetLayer()->GetEntities();
				const Uint32 entityCount = entities.Size();
				garbageCounter += entityCount;

				for( Uint32 entityIndex = 0; entityIndex < entityCount; ++entityIndex )
				{
					CEntity* currentEntity = entities[entityIndex];
					myText.PushBack( GetEntityInfo( currentEntity, layerPath.AsChar() ) );
				}
			}
			layerInfos[j]->SyncUnload();
			// Collect all garbage since it will go out of memory otherwise
			if ( garbageCounter>5000 )
			{
				SGarbageCollector::GetInstance().CollectNow();
				garbageCounter = 0;
			}
			
			// Write the data here into the file
			nFile = _wfopen( depFilePath.AsChar(), TXT("a") );
			if ( nFile != NULL )
			{
				for( Uint32 l=0; l < myText.Size(); ++l )
				{
					fputs( myText[l].AsChar(), nFile );
				}
				fclose(nFile);
			}

			myText.Clear();
			GFeedback->UpdateTaskProgress( j, layerGroupSize );
		}


		CMesh::DeactivatePipelineMesh();

		GFeedback->EndTask();

	}


	// separating this into a separate block so that we can just ask for this without going through the whole level and its assets
	// If not full dependency it will only generate this things under here.

	// Write the data here into the file
	nFile = _wfopen( depFilePath.AsChar(), TXT("a") );
	if ( nFile != NULL )
	{
		for( Uint32 l=0; l < myText.Size(); ++l )
		{
			fputs( myText[l].AsChar(), nFile );
		}
		fclose(nFile);
	}
	
	TDynArray< StringAnsi > folText;
	CFoliageEditionController& foliageController = world->GetFoliageEditionController();
	CFoliageCellIterator foliageIterator = foliageController.GetFoliageBroker()->GetCellIterator( foliageController.GetWorldBox() );
	Uint32 totalCells = foliageController.GetFoliageBroker()->TotalCellCount();
	Uint32 cellsProcessed = 0;
	// Progressbar for the generation of dep files
	GFeedback->BeginTask( TXT("Adding Foliage to the .dep file"), false );
	GFeedback->UpdateTaskProgress( 0, totalCells );

	for ( foliageIterator; foliageIterator; ++foliageIterator )
	{
		folText.PushBack( GetFoliageInfo( foliageIterator, worldName ) );

		++cellsProcessed;
		GFeedback->UpdateTaskProgress( cellsProcessed, totalCells );
		// Write the data here into the file
		nFile = _wfopen( depFilePath.AsChar(), TXT("a") );
		if ( nFile != NULL )
		{
			for( Uint32 l=0; l < folText.Size(); ++l )
			{
				fputs( folText[l].AsChar(), nFile );
			}
			fclose(nFile);
		}
		folText.Clear();
	}
	GFeedback->EndTask();

	if ( !g_corruptedLines.Empty() )
	{
		CDirectory* saveDir = GDepot->FindLocalDirectory( TXT("dep_files") );
		
		CResource::FactoryInfo< C2dArray > info;
		C2dArray* debugDumpInfo = info.CreateResource();
		debugDumpInfo->AddColumn( TXT("OriginalLine") );
		debugDumpInfo->AddRow();
		debugDumpInfo->SetValue( TXT("Original line"), 0, 0 );

		for ( String& corruptedLine : g_corruptedLines )
		{
			debugDumpInfo->AddRow();
			debugDumpInfo->SetValue( corruptedLine, 0, debugDumpInfo->GetNumberOfRows()-1 );
		}

		debugDumpInfo->SaveAs( saveDir, worldName + TXT(".csv"), true );
		debugDumpInfo->Discard();
	}
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetFoliageInfo( CFoliageCellIterator foliageIterator, String worldName )
{
		TDynArray< StringAnsi > myText;
		
		if ( foliageIterator->IsResourceValid() )
		{
			Uint32 treeCounter = 0;
			Uint32 grassCounter = 0;
			foliageIterator->Wait(); // wait for resource to load
			CFoliageResource* res = foliageIterator->GetFoliageResource();
			const CFoliageResource::InstanceGroupContainer& trees = res->GetAllTreeInstances();
			const CFoliageResource::InstanceGroupContainer& grass = res->GetAllGrassInstances();
			String flyrFile = TXT("");
			for ( auto group:trees )
			{
				Box bb = group.baseTree->GetBBox();
				treeCounter += group.instances.Size();
			}
			for ( auto group:grass )
			{
				Box bb = group.baseTree->GetBBox();
				grassCounter += group.instances.Size();
			}

			Vector2 pos = foliageIterator->GetWorldCoordinate();
			String posString = String::Printf( TXT( "[[%f %f 0 1|0 0 0]]" ), pos.X, pos.Y );
			flyrFile = GenerateFoliageFilename( pos );
			myText.PushBack( "insert or ignore into FoliageTiles ( worldName, flyrFile, treeCounter, grassCounter, pos) values ('" );
			myText.PushBack( UNICODE_TO_ANSI( worldName.AsChar() ) );
			myText.PushBack( "','" );
			myText.PushBack( UNICODE_TO_ANSI( flyrFile.AsChar() ) );
			myText.PushBack( "','" );
			myText.PushBack( UNICODE_TO_ANSI( ToString( treeCounter ).AsChar() ) );
			myText.PushBack( "','" );
			myText.PushBack( UNICODE_TO_ANSI( ToString( grassCounter ).AsChar() ) );
			myText.PushBack( "','" );
			myText.PushBack( UNICODE_TO_ANSI( posString.AsChar() ) );
			myText.PushBack( "');\n" );
		}
		
		return myText;
}

TDynArray<StringAnsi> WorldSceneDependencyInfo::GetEntityInfo( CEntity* currentEntity, const String layerPath )
{
	TDynArray<StringAnsi> myText;
	SEntityStreamingState state;
	currentEntity->PrepareStreamingComponentsEnumeration( state, true );
	currentEntity->ForceFinishAsyncResourceLoads();

	const Vector& pos = currentEntity->GetWorldPositionRef();

	const TDynArray< CComponent* > components = currentEntity->GetComponents();

	// If entity is a entityTemplate we can get the path to it
	String templatePath = currentEntity->GetEntityTemplate() ? currentEntity->GetEntityTemplate()->GetDepotPath() : L"";

	String nodeName = currentEntity->GetName();
	nodeName.ReplaceAll( TXT("'"), TXT("@") );
	nodeName.Trim();
	// Check if the entity has drawable components
	Bool hasDrawableComponent = false;
	TDynArray<String> componentsClassNames;
	for ( Uint32 i=0; i<components.Size(); i++ )
	{
		CDrawableComponent* dc = Cast< CDrawableComponent >( components[i] );
		if ( dc && dc->IsVisible() && dc->IsAttached() )
		{
			hasDrawableComponent = true;
		}
		CComponent* cc = Cast<CComponent>(components[i]);
		if ( cc )
		{		
			componentsClassNames.PushBack( cc->GetClass()->GetName().AsChar() );
		}
	}
	String componentsClassNamesString = String::Join( componentsClassNames, TXT(", ") );
	Box bbox = currentEntity->CalcBoundingBox();
	Float volume = bbox.GetMass();
	Float width  = 0.0f;
	Float depth  = 0.0f;
	Float height = 0.0f;
	width = bbox.Max.X - bbox.Min.X;
	depth = bbox.Max.Y - bbox.Min.Y;
	height = bbox.Max.Z - bbox.Min.Z; 

	Vector bboxmin = bbox.Min;
	Vector bboxmax = bbox.Max;

	myText.PushBack( "insert or ignore into entities (name, layerid, class, componentsCount, componentsClassNames, templatePath, posx, posy, posz, bbminx, bbminy, bbminz, bbmaxx, bbmaxy, bbmaxz, volume, width, depth, height, drawableComp) values ('" );
	myText.PushBack( UNICODE_TO_ANSI( nodeName.AsChar() ) );
	myText.PushBack( "'," );
	myText.PushBack( "(select id from layers where path='" );
	myText.PushBack( UNICODE_TO_ANSI( layerPath.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dbStrings;
	dbStrings.PushBack( currentEntity->GetClass()->GetName().AsChar() );
	dbStrings.PushBack( ToString(components.Size()) );
	dbStrings.PushBack( componentsClassNamesString );
	dbStrings.PushBack( templatePath );
	dbStrings.PushBack( ToString(pos.X) );
	dbStrings.PushBack( ToString(pos.Y) );
	dbStrings.PushBack( ToString(pos.Z) );
	dbStrings.PushBack( ToString(bboxmin.X) );
	dbStrings.PushBack( ToString(bboxmin.Y) );
	dbStrings.PushBack( ToString(bboxmin.Z) );
	dbStrings.PushBack( ToString(bboxmax.X) );
	dbStrings.PushBack( ToString(bboxmax.Y) );
	dbStrings.PushBack( ToString(bboxmax.Z) );
	dbStrings.PushBack( ToString(volume) );
	dbStrings.PushBack( ToString(width) );
	dbStrings.PushBack( ToString(depth) );
	dbStrings.PushBack( ToString(height) );
	dbStrings.PushBack( ToString(hasDrawableComponent) );

	// Making a proper db string out of all the data that we gathered
	myText.PushBack( MakeDBString( dbStrings ) );


	CEntityTemplate* entTemplate = currentEntity->GetEntityTemplate();
	if ( entTemplate )
	{
		String path = TXT("");
		String staticFlags = TXT("");
		String includedTemplatesNames = TXT("");
		TDynArray<String> incTemplatesNames;
		TDynArray< THandle<CEntityTemplate> > includedTemplates = entTemplate->GetIncludes();
		//TDynArray< CEntityAppearance > appearances = 
		TDynArray< const CEntityAppearance* > allAppearances;
		entTemplate->GetAllAppearances( allAppearances );
		TDynArray< const CEntityAppearance* > allUsedAppearances;
		entTemplate->GetAllEnabledAppearances( allUsedAppearances );
		TDynArray<String> appNames;
		TDynArray<String> appUsedNames;
		CResource* cRes = Cast<CResource>(entTemplate);
		TDynArray< THandle<CEntityTemplate> > entIncTemplates;
		TDynArray<String> entIncludes;
		if( cRes )
		{
			path = cRes->GetFile()->GetDepotPath();
		}
		for(auto& inc: includedTemplates )
		{
			if(inc)
			{
				incTemplatesNames.PushBack( inc->GetDepotPath() );
			}
		}
		for(auto app: allAppearances )
		{
			appNames.PushBack( app->GetName().AsString() );
		}
		for(auto app: allUsedAppearances )
		{
			appUsedNames.PushBack( app->GetName().AsString() );
		}
		String allAppearanceNames = String::Join( appNames, TXT(", ") );
		String allUsedAppearanceNames = String::Join( appUsedNames, TXT(", ") );
		includedTemplatesNames = String::Join( incTemplatesNames, TXT(", ") );
		
		Uint16 staticFl = currentEntity->GetStaticFlags();
		CBitField* bf = (CBitField*)SRTTI::GetInstance().FindType( RED_NAME( EEntityStaticFlags ) );
		TDynArray <String> bitNames;
		for ( Uint32 i=0; i<32; i++ )
		{
			if ( staticFl & (1<<i))
			{
				CName bitName = bf->GetBitName( i );
				//DUMP IT!
				bitNames.PushBack( bitName.AsString() );
			}
		}
		staticFlags = String::Join( bitNames, TXT(", ") );
		//ETriggerChannel incChannels = trigAreaCmp->GetIncludedChannels();
		//String includedChannels = CBitField::ToString(incChannels);	

		myText.PushBack( "insert or ignore into entitytemplates (path, layerPath, name, includedTemplates, allAppearanceNames, allUsedAppearanceNames, staticFlags) values ('" );
		TDynArray< String > dbTemplateStrings;
		dbTemplateStrings.PushBack( path );
		dbTemplateStrings.PushBack( layerPath );
		dbTemplateStrings.PushBack( nodeName );
		dbTemplateStrings.PushBack( includedTemplatesNames );
		dbTemplateStrings.PushBack( allAppearanceNames );
		dbTemplateStrings.PushBack( allUsedAppearanceNames );
		dbTemplateStrings.PushBack( staticFlags );
		//dbTemplateStrings.PushBack( includedEntities );

		// Making a proper db string out of all the data that we gathered
		myText.PushBack( MakeDBString( dbTemplateStrings ) );

		// Adding the appearances
		for(auto app: allAppearances )
		{
			String appName = app->GetName().AsString();
			entIncTemplates = app->GetIncludedTemplates();
			for(auto ent: entIncTemplates )
			{
				CResource* cIncRes = Cast<CResource>(ent);
				if( cIncRes )
				{
					entIncludes.PushBack( cIncRes->GetFile()->GetDepotPath() );
				}
			}
			String bodyParts = String::Join( entIncludes, TXT(", ") );
			myText.PushBack( "insert or ignore into appearances (entitytemplateid, name, bodyParts ) values (" );
			myText.PushBack( "(select id from entitytemplates where layerPath='" );
			myText.PushBack( UNICODE_TO_ANSI( layerPath.AsChar() ) );
			myText.PushBack( "' AND name='" );
			myText.PushBack( UNICODE_TO_ANSI( nodeName.AsChar() ) );
			myText.PushBack( "' LIMIT 1),'" );

			TDynArray< String > dbIncEntStrings;
			dbIncEntStrings.PushBack( appName );
			dbIncEntStrings.PushBack( bodyParts );
			// Making a proper db string out of all the data that we gathered
			myText.PushBack( MakeDBString( dbIncEntStrings ) );
			entIncludes.ClearFast();
		}
	}

	// Get all the components of the Entity and add them one by one
	for( Uint32 nodeIndex = 0; nodeIndex < components.Size(); ++nodeIndex )
	{
		myText.PushBack( GetComponentInfo( components[nodeIndex], nodeName ) );
	}

	currentEntity->FinishStreamingComponentsEnumeration( state );
	return myText;
}


TDynArray<StringAnsi> WorldSceneDependencyInfo::GetComponentInfo( const CComponent* component, const String entityName )
{
	if ( component->IsA< CMeshComponent >() )					return GetMeshInfo( component, entityName );
	else if ( component->IsA< CParticleComponent >() )			return GetParticleInfo( component, entityName );
	else if ( component->IsA< CDestructionSystemComponent >() )	return GetDestructionInfo( component, entityName );
	else if ( component->IsA< CClothComponent >() )				return GetClothInfo( component, entityName );
	else if ( component->IsA< CLightComponent >() )				return GetLightInfo( component, entityName );
	else if ( component->IsA< CDimmerComponent >() )			return GetDimmerInfo( component, entityName );
	else if ( component->IsA< CSoundEmitterComponent >() )		return GetSoundEmitterInfo( component, entityName );
	else if ( component->IsA< CFlareComponent >() )				return GetFlareInfo( component, entityName );
	else if ( component->IsA< CAreaComponent >() )				return GetAreaInfo( component, entityName );
	else
	{
		TDynArray<StringAnsi> myText;
		return myText;
	}
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetAreaInfo( const CComponent* component, const String entityName )
{
	TDynArray< StringAnsi > myText;

	//////////////////////////////////////////////////////////////////////////
	//AREA COMPONENTS
	//////////////////////////////////////////////////////////////////////////
	Vector pos = component->GetWorldPositionRef();

	myText.PushBack( "insert or ignore into areacomponents (entityid, posx, posy, posz, areaType ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dbStrings;

	dbStrings.PushBack( ToString( pos.X ) );
	dbStrings.PushBack( ToString( pos.Y ) );
	dbStrings.PushBack( ToString( pos.Z ) );
	dbStrings.PushBack( component->GetClass()->GetName().AsChar() );
	// Making a proper db string out of all the data that we gathered
	myText.PushBack( MakeDBString( dbStrings ) );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//TRIGGER AREA COMPONENTS
	//////////////////////////////////////////////////////////////////////////
	const CTriggerAreaComponent* trigAreaCmp = Cast< CTriggerAreaComponent >( component );
	if ( trigAreaCmp )
	{
		Bool enabled = trigAreaCmp->IsEnabled();
		ETriggerChannel incChannels = trigAreaCmp->GetIncludedChannels();
		String includedChannels = CBitField::ToString(incChannels);	

		ETriggerChannel excChannels = trigAreaCmp->GetExludedChannels();
		String excludedChannels = CBitField::ToString(excChannels);

		TDynArray<String> tagNames;

		const TagList& tagList = trigAreaCmp->GetTags();
		for ( Uint32 t = 0; t < tagList.GetTags().Size(); ++t )
		{
			tagNames.PushBack( tagList.GetTag( t ).AsString() );
		}
		String tagNamesString = String::Join( tagNames, TXT(", ") );

		myText.PushBack( "insert or ignore into triggerareacomponents (areaid, triggerPriority, enabled, useCCD, tags, includedChannels, excludedChannels ) values (" );
		myText.PushBack( "(select areacomponents._id from areacomponents where areacomponents.entityid = (select id from entities where name='" );
		myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
		myText.PushBack( "') LIMIT 1),'" );

		TDynArray< String > dbStrings;

		dbStrings.PushBack( ToString( trigAreaCmp->GetTriggerPriority() ) );
		dbStrings.PushBack( ToString( enabled ) );
		dbStrings.PushBack( ToString( trigAreaCmp->GetCCD() ) );
		dbStrings.PushBack( tagNamesString );
		dbStrings.PushBack( includedChannels );
		dbStrings.PushBack( excludedChannels );

		// Making a proper db string out of all the data that we gathered
		myText.PushBack( MakeDBString( dbStrings ) );
	}
	const CPathLibRoughtTerrainComponent* roughTerrainAreaCmp = Cast< CPathLibRoughtTerrainComponent >( component );
	if ( roughTerrainAreaCmp )
	{
		myText.PushBack( "insert or ignore into PathLibRoughTerrainComponents (areaid, isRoughTerrain ) values (" );
		myText.PushBack( "(select areacomponents._id from areacomponents where areacomponents.entityid = (select id from entities where name='" );
		myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
		myText.PushBack( "') LIMIT 1),'" );

		TDynArray< String > dbStrings;

		dbStrings.PushBack( ToString( roughTerrainAreaCmp->IsRoughtTerrain() ) );

		// Making a proper db string out of all the data that we gathered
		myText.PushBack( MakeDBString( dbStrings ) );
	}
	const CNavmeshComponent* navMeshAreaCmp = Cast< CNavmeshComponent >( component );
	if ( navMeshAreaCmp )
	{
		SNavmeshParams navMeshParams = navMeshAreaCmp->GetNavmeshParams();
		String name;
		CNavmeshComponent::GenericFileName( navMeshAreaCmp->GetPathLibAreaId(), name);
		myText.PushBack( "insert or ignore into NavMeshComponents (areaid, genericFileName, sharedFilePath, useGenerationRootPoints, useTerrainInGeneration, useStaticMeshesInGeneration, collectFoliage, previewOriginalGeometry, useCollisionMeshes, monotonePartitioning, detectTerrainConnection, stepOnNonWalkableMeshes, cutMeshesWithBoundings, smoothWalkableAreas, extensionLength, cellWidth, cellHeight, walkableSlopeAngle, agentHeight, margin, agentClimb, maxEdgeLen, maxEdgeError, regionMinSize, regionMergeSize, vertsPerPoly, detailSampleDist, detailSampleMaxError, extraStreamingRange ) values (" );
		myText.PushBack( "(select areacomponents._id from areacomponents where areacomponents.entityid = (select id from entities where name='" );
		myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
		myText.PushBack( "') LIMIT 1),'" );

		TDynArray< String > dbStrings;
		dbStrings.PushBack( name );
		dbStrings.PushBack( navMeshAreaCmp->GetSharedFilePath().AsChar() );

		dbStrings.PushBack( ToString( navMeshParams.m_useGenerationRootPoints ) );
		dbStrings.PushBack( ToString( navMeshParams.m_useTerrainInGeneration ) );
		dbStrings.PushBack( ToString( navMeshParams.m_useStaticMeshesInGeneration ) );
		dbStrings.PushBack( ToString( navMeshParams.m_collectFoliage ) );
		dbStrings.PushBack( ToString( navMeshParams.m_previewOriginalGeometry ) );
		dbStrings.PushBack( ToString( navMeshParams.m_useCollisionMeshes ) );
		dbStrings.PushBack( ToString( navMeshParams.m_monotonePartitioning ) );
		dbStrings.PushBack( ToString( navMeshParams.m_detectTerrainConnection ) );
		dbStrings.PushBack( ToString( navMeshParams.m_stepOnNonWalkableMeshes ) );
		dbStrings.PushBack( ToString( navMeshParams.m_cutMeshesWithBoundings ) );
		dbStrings.PushBack( ToString( navMeshParams.m_smoothWalkableAreas ) );
		dbStrings.PushBack( ToString( navMeshParams.m_extensionLength ) );
		dbStrings.PushBack( ToString( navMeshParams.m_cellWidth ) );
		dbStrings.PushBack( ToString( navMeshParams.m_cellHeight ) );
		dbStrings.PushBack( ToString( navMeshParams.m_walkableSlopeAngle ) );
		dbStrings.PushBack( ToString( navMeshParams.m_agentHeight ) );
		dbStrings.PushBack( ToString( navMeshParams.m_margin ) );
		dbStrings.PushBack( ToString( navMeshParams.m_agentClimb ) );
		dbStrings.PushBack( ToString( navMeshParams.m_maxEdgeLen ) );
		dbStrings.PushBack( ToString( navMeshParams.m_maxEdgeError ) );
		dbStrings.PushBack( ToString( navMeshParams.m_regionMinSize ) );
		dbStrings.PushBack( ToString( navMeshParams.m_regionMergeSize ) );
		dbStrings.PushBack( ToString( navMeshParams.m_vertsPerPoly ) );
		dbStrings.PushBack( ToString( navMeshParams.m_detailSampleDist ) );
		dbStrings.PushBack( ToString( navMeshParams.m_detailSampleMaxError ) );
		dbStrings.PushBack( ToString( navMeshParams.m_extraStreamingRange ) );

		// Making a proper db string out of all the data that we gathered
		myText.PushBack( MakeDBString( dbStrings ) );
	}

	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetFlareInfo( const CComponent* component, const String entityName )
{
	TDynArray< StringAnsi > myText;
	const CFlareComponent* flareCmp = Cast< CFlareComponent >( component );
	Vector pos = component->GetWorldPositionRef();

	myText.PushBack( "insert or ignore into flarecomponents (entityid, posx, posy, posz ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dbStrings;

	dbStrings.PushBack( ToString( pos.X ) );
	dbStrings.PushBack( ToString( pos.Y ) );
	dbStrings.PushBack( ToString( pos.Z ) );
	// Making a proper db string out of all the data that we gathered
	myText.PushBack( MakeDBString( dbStrings ) );
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetClothInfo( const CComponent* component, const String entityName )
{
	TDynArray< StringAnsi > myText;
	const CClothComponent* clothCmp = Cast< CClothComponent >( component );
	SClothParameters params = clothCmp->GetParameters();
	String maxSimulationDistance = ToString( params.m_simulationMaxDistance );

	CMeshTypeResource* meshResource = ((CClothComponent*)clothCmp)->GetMeshTypeResource();
	//String streamingLod = ToString( component->GetStreamingLOD() );
	Vector pos = component->GetWorldPositionRef();
	Box bbox;
	Vector bboxmin(0.0f,0.0f,0.0f,0.0f);
	Vector bboxmax(0.0f,0.0f,0.0f,0.0f);
	String materialCount = TXT("");
	String authorName = TXT("");
	String depotPath = TXT("");
	String importFile = TXT("");
	String guid = GetGuid(component);

	if( meshResource )
	{
		authorName = meshResource->GetAuthorName();
		MakeNiceString( authorName );
		// Cant get this from meshtype resource file
		depotPath = meshResource->GetDepotPath();
		importFile = meshResource->GetImportFile();

		Matrix m;
		component->GetLocalToWorld(m);

		Box meshbbox = meshResource->GetBoundingBox();
		bbox = m.TransformBox(meshbbox);
		if ( !bbox.IsEmpty() )
		{
			bboxmin = bbox.Min;
			bboxmax = bbox.Max;
		}
		const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();
		materialCount = ToString( materials.Size() );
	}

	myText.PushBack( "insert or ignore into clothcomponents (entityid, posx, posy, posz, bbminx, bbminy, bbminz, bbmaxx, bbmaxy, bbmaxz, maxSimulationDistance, materialCount, depotPath, importFile, author, guid ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dbStrings;

	dbStrings.PushBack( ToString( pos.X ) );
	dbStrings.PushBack( ToString( pos.Y ) );
	dbStrings.PushBack( ToString( pos.Z ) );
	dbStrings.PushBack( ToString( bboxmin.X ) );
	dbStrings.PushBack( ToString( bboxmin.Y ) );
	dbStrings.PushBack( ToString( bboxmin.Z ) );
	dbStrings.PushBack( ToString( bboxmax.X ) );
	dbStrings.PushBack( ToString( bboxmax.Y ) );
	dbStrings.PushBack( ToString( bboxmax.Z ) );
	dbStrings.PushBack( maxSimulationDistance );
	dbStrings.PushBack( materialCount );
	//dbStrings.PushBack( streamingLod );
	dbStrings.PushBack( ToString(depotPath) );
	dbStrings.PushBack( ToString(importFile) );
	dbStrings.PushBack( ToString(authorName) );
	dbStrings.PushBack( guid );

	// Making a proper db string out of all the data that we gathered
	myText.PushBack( MakeDBString( dbStrings ) );

	// Materials
	if( meshResource )
	{
		const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();
		const TDynArray< String >& materialNames = meshResource->GetMaterialNames();
		for ( Uint32 j=0; j<materials.Size(); j++ )
		{
			IMaterial* material = materials[j].Get();
			if ( material )
			{
				const String materialName = meshResource->GetMaterialNames()[j];
				String baseName = TXT("");
				String usingMimics = TXT("");
				TDynArray< String > blockNames;
				CMaterialGraph* materialGraph = Cast< CMaterialGraph >( material );
				if( materialGraph )
				{
					usingMimics = ToString( materialGraph->IsMimicMaterial() );
					TDynArray< CGraphBlock* > blocks = materialGraph->GraphGetBlocks();
					if ( !blocks.Empty() )
					{ 
						for(auto block: blocks )
						{
							// Get the block names
							blockNames.PushBack( block->GetCaption() );
						}
					}
				}
				String blockNamesString = String::Join( blockNames, TXT(", ") );
				IMaterial* baseMaterial = material->GetMaterialDefinition();
				if ( baseMaterial )
				{
					// Shader name, material graph
					baseName = baseMaterial->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
					MakeNiceString( baseName );		

					// Adding BaseMaterial info here
					myText.PushBack( AddBaseMaterialInfo( baseMaterial ) );
				}
				String path = TXT("");
				path = material->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
				MakeNiceString( path );

				myText.PushBack( "insert or ignore into materials ( clothid, name, base, usingMimics, blockNames ) values (" );
				myText.PushBack( "(select _id from clothcomponents where depotPath='" );
				myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
				myText.PushBack( "' LIMIT 1),'" );
				myText.PushBack( UNICODE_TO_ANSI( materialName.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( baseName.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( usingMimics.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( blockNamesString.AsChar() ) );
				myText.PushBack( "');\n" );
			}	
		}
	}

	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetDestructionInfo( const CComponent* component, const String entityName )
{
	TDynArray< StringAnsi > myText;


#ifdef USE_APEX
	//String streamingLod = ToString( component->GetStreamingLOD() );
	const CDestructionSystemComponent* destCmp = Cast< CDestructionSystemComponent >( component );
	CApexResource* apxResource = destCmp->GetApexResource();
	const CMeshTypeResource* meshResource = Cast< CMeshTypeResource >( apxResource );

	Vector pos = component->GetWorldPositionRef();
	Box bbox;
	Vector bboxmin(0.0f,0.0f,0.0f,0.0f);
	Vector bboxmax(0.0f,0.0f,0.0f,0.0f);
	String materialCount = TXT("");
	String authorName = TXT("");
	String depotPath = TXT("");
	String importFile = TXT("");
	String guid = GetGuid(component);

	if( meshResource )
	{
		authorName = meshResource->GetAuthorName();
		MakeNiceString( authorName );
		// Cant get this from meshtype resource file
		depotPath = meshResource->GetDepotPath();
		importFile = meshResource->GetImportFile();

		Matrix m;
		component->GetLocalToWorld(m);
		
		Box meshbbox = meshResource->GetBoundingBox();
		bbox = m.TransformBox(meshbbox);
		if ( !bbox.IsEmpty() )
		{
			bboxmin = bbox.Min;
			bboxmax = bbox.Max;
		}
		const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();
		materialCount = ToString( materials.Size() );
	}

	myText.PushBack( "insert or ignore into destructioncomponents (entityid, posx, posy, posz, bbminx, bbminy, bbminz, bbmaxx, bbmaxy, bbmaxz, materialCount, depotPath, importFile, author, guid ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dbStrings;

	dbStrings.PushBack( ToString( pos.X ) );
	dbStrings.PushBack( ToString( pos.Y ) );
	dbStrings.PushBack( ToString( pos.Z ) );
	dbStrings.PushBack( ToString( bboxmin.X ) );
	dbStrings.PushBack( ToString( bboxmin.Y ) );
	dbStrings.PushBack( ToString( bboxmin.Z ) );
	dbStrings.PushBack( ToString( bboxmax.X ) );
	dbStrings.PushBack( ToString( bboxmax.Y ) );
	dbStrings.PushBack( ToString( bboxmax.Z ) );
	dbStrings.PushBack( materialCount );
	//dbStrings.PushBack( streamingLod );
	dbStrings.PushBack( ToString(depotPath) );
	dbStrings.PushBack( ToString(importFile) );
	dbStrings.PushBack( ToString(authorName) );
	dbStrings.PushBack( guid );

	// Making a proper db string out of all the data that we gathered
	myText.PushBack( MakeDBString( dbStrings ) );

	// Materials
	if( meshResource )
	{
		const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();
		for ( Uint32 j=0; j<materials.Size(); j++ )
		{
			IMaterial* material = materials[j].Get();
			if ( material )
			{
				const String materialName = meshResource->GetMaterialNames()[j];
				String baseName = TXT("");
				String usingMimics = TXT("");
				TDynArray< String > blockNames;
				CMaterialGraph* materialGraph = Cast< CMaterialGraph >( material );
				if( materialGraph )
				{
					usingMimics = ToString( materialGraph->IsMimicMaterial() );
					TDynArray< CGraphBlock* > blocks = materialGraph->GraphGetBlocks();
					if ( !blocks.Empty() )
					{ 
						for(auto block: blocks )
						{
							// Get the block names
							blockNames.PushBack( block->GetCaption() );
						}
					}
				}
				String blockNamesString = String::Join( blockNames, TXT(", ") );

				IMaterial* baseMaterial = material->GetMaterialDefinition();
				if ( baseMaterial )
				{
					// Shader name, material graph
					baseName = baseMaterial->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
					MakeNiceString( baseName );		

					// Adding BaseMaterial info here
					myText.PushBack( AddBaseMaterialInfo( baseMaterial ) );
				}
				String path = TXT("");
				path = material->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
				MakeNiceString( path );

				myText.PushBack( "insert or ignore into materials ( destructionid, name, base, usingMimics, blockNames ) values (" );
				myText.PushBack( "(select _id from destructioncomponents where depotPath='" );
				myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
				myText.PushBack( "' LIMIT 1),'" );
				myText.PushBack( UNICODE_TO_ANSI( materialName.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( baseName.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( usingMimics.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( blockNamesString.AsChar() ) );
				myText.PushBack( "');\n" );
			}	
		}
	}
#endif
	return myText;
}

String WorldSceneDependencyInfo::GetGuid( const CComponent* component )
{
	Char buff[RED_GUID_STRING_BUFFER_SIZE];
	const CGUID& guid = component->GetGUID();
	guid.ToString( buff, RED_GUID_STRING_BUFFER_SIZE );
	String returnString( buff );
	return std::move( returnString );
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetMeshInfo( const CComponent* component, const String entityName )
{
	TDynArray<StringAnsi> myText;
	//String streamingLod = ToString( component->GetStreamingLOD() );
	CClass *nclass = component->GetBaseObjectClass();
	String className = nclass->GetName().AsChar();
	
	String meshName = component->GetName();
	MakeNiceString( meshName );

	//CMesh* meshResource = mcmp->GetMeshNow();

	String classType = ToString( component->GetStaticClass()->GetName() );

	CMesh* meshResource = ((CMeshComponent*)component)->GetMeshNow();
	Vector pos = component->GetWorldPositionRef();

	Box bbox;
	Vector bboxmin(0.0f,0.0f,0.0f,0.0f);
	Vector bboxmax(0.0f,0.0f,0.0f,0.0f);
	
	String importFile = TXT("");
	String authorName = TXT("");
	String depotPath = TXT("");
	String vertCount = TXT("");
	String autoHideDistance = TXT("");
	String lodCount = TXT("");
	String materialCount = TXT("");
	String triangleCount = TXT("");
	String collisionShapeCount = TXT("0");
	String soundType = TXT("");
	String soundSize = TXT("");
	String physicalMaterialName = TXT("");
	String collisionVerts = TXT("");
	String collisionTriangles = TXT("");
	String collisionShape = TXT("");
	String soundOcclusion = TXT("0");
	String guid = GetGuid(component);
	String drawableFlags = TXT("");
	String useExtraStream = TXT("");
	String lightChannels = TXT("");
	Float volume = 0.0f;
	Float width  = 0.0f;
	Float depth  = 0.0f;
	Float height = 0.0f;
	Uint32 flags = ((CMeshComponent*)component)->GetDrawableFlags();
	CBitField* bf = (CBitField*)SRTTI::GetInstance().FindType( RED_NAME( EDrawableFlags ) );
	TDynArray <String> bitNames;
	for ( Uint32 i=0; i<32; i++ )
	{
		if ( flags & (1<<i))
		{
			CName bitName = bf->GetBitName( i );
			//DUMP IT!
			bitNames.PushBack( bitName.AsString() );
		}
	}
	drawableFlags = String::Join( bitNames, TXT(", ") );

	String isStreamed =  ToString( ((CMeshComponent*)component)->IsStreamed() );
	String forceLodLevel = ToString( ((CMeshComponent*)component)->GetForcedLODLevel() );
	EMeshShadowImportanceBias shdwImpBias = ((CMeshComponent*)component)->GetShadowImportanceBias();
	String shadowImportanceBias = CEnum::ToString(shdwImpBias).AsChar();

	Uint8 channels = ((CMeshComponent*)component)->GetLightChannels();
	CBitField* lightchannelbf = (CBitField*)SRTTI::GetInstance().FindType( RED_NAME( ELightChannel ) );
	TDynArray <String> lightChannelsNames;
	for ( Uint32 i=0; i<32; i++ )
	{
		if ( channels & (1<<i))
		{
			CName clName = lightchannelbf->GetBitName( i );
			//DUMP IT!
			lightChannelsNames.PushBack( clName.AsString() );
		}
	}
	lightChannels = String::Join( lightChannelsNames, TXT(", ") );


	if ( meshResource )
	{
		useExtraStream = ToString( meshResource->CanUseExtraStreams() );
		Matrix m;
		component->GetLocalToWorld(m);

		Box meshbbox = meshResource->GetBoundingBox();
		bbox = m.TransformBox(meshbbox);
		if ( !bbox.IsEmpty() )
		{
			bboxmin = bbox.Min;
			bboxmax = bbox.Max;
			volume = bbox.GetMass();
			width = bbox.Max.X - bbox.Min.X;
			depth = bbox.Max.Y - bbox.Min.Y;
			height = bbox.Max.Z - bbox.Min.Z; 
		}
		autoHideDistance = ToString( meshResource->GetAutoHideDistance() );
		importFile = meshResource->GetImportFile();
		authorName = meshResource->GetAuthorName();
		MakeNiceString( authorName );
		depotPath = meshResource->GetDepotPath();
		
		const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();
		const CMesh::TLODLevelArray& lodLevelArray = meshResource->GetMeshLODLevels();

		lodCount = ToString( lodLevelArray.Size() );
		vertCount = ToString(meshResource->CountLODVertices( 0 ));
		triangleCount = ToString(meshResource->CountLODTriangles( 0 ));
		materialCount = ToString( materials.Size() );

		

		const CCollisionMesh* collisionMesh = meshResource->GetCollisionMesh();
		if( collisionMesh )
		{
			const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
			collisionShapeCount = ToString( shapes.Size() );

			Float att = collisionMesh->GetOcclusionAttenuation();
			Float diag = collisionMesh->GetOcclusionDiagonalLimit();
			if( att != -1.0f && diag != -1.0f )
			{
				soundOcclusion = TXT("1");
			}
		}
		const SMeshSoundInfo* msi = meshResource->GetMeshSoundInfo();
		if( msi )
		{
			soundType = msi->m_soundTypeIdentification.AsString();  
			soundSize = msi->m_soundSizeIdentification.AsString();  
		}
		else
		{
			soundType = String::EMPTY;
			soundSize = String::EMPTY;
		}
	}

	myText.PushBack( "insert or ignore into meshcomponents (entityid, posx, posy, posz, bbminx, bbminy, bbminz, bbmaxx, bbmaxy, bbmaxz, volume, width, depth, height, lodCount, materialCount, triangleCount, vertCount, autoHideDistance, collisionShapeCount, className, soundType, soundSize, soundOcclusion, depotPath, importFile, author, guid, drawableFlags, lightChannels, forceLodLevel, isStreamed, useExtraStream ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dbStrings;

	dbStrings.PushBack( ToString( pos.X ) );
	dbStrings.PushBack( ToString( pos.Y ) );
	dbStrings.PushBack( ToString( pos.Z ) );
	dbStrings.PushBack( ToString( bboxmin.X ) );
	dbStrings.PushBack( ToString( bboxmin.Y ) );
	dbStrings.PushBack( ToString( bboxmin.Z ) );
	dbStrings.PushBack( ToString( bboxmax.X ) );
	dbStrings.PushBack( ToString( bboxmax.Y ) );
	dbStrings.PushBack( ToString( bboxmax.Z ) );
	dbStrings.PushBack( ToString( volume ) );
	dbStrings.PushBack( ToString(width) );
	dbStrings.PushBack( ToString(depth) );
	dbStrings.PushBack( ToString(height) );
	dbStrings.PushBack( lodCount );
	dbStrings.PushBack( materialCount );
	dbStrings.PushBack( triangleCount );
	dbStrings.PushBack( vertCount );
	dbStrings.PushBack( autoHideDistance );
	dbStrings.PushBack( collisionShapeCount );
	dbStrings.PushBack( className );
	dbStrings.PushBack( soundType );
	dbStrings.PushBack( soundSize );
	dbStrings.PushBack( soundOcclusion );
	dbStrings.PushBack( ToString(depotPath) );
	dbStrings.PushBack( ToString(importFile) );
	dbStrings.PushBack( ToString(authorName) );
	dbStrings.PushBack( guid );
	dbStrings.PushBack( drawableFlags );
	dbStrings.PushBack( lightChannels );
	dbStrings.PushBack( forceLodLevel );
	dbStrings.PushBack( isStreamed );
	dbStrings.PushBack( useExtraStream );

	// Making a proper db string out of all the data that we gathered
	myText.PushBack( MakeDBString( dbStrings ) );
	
	if( meshResource )
	{
		// Adding the Materials, physics shapes, Lods, Chunks later to get the information in properly, need the meshcomponent in the db before doing this
		myText.PushBack( AddMaterialInfo( meshResource, entityName ) );
		myText.PushBack( AddCollisionMeshInfo( meshResource ) );
		myText.PushBack( AddLodInfo( meshResource ) );
		myText.PushBack( AddChunkInfo( meshResource ) );
	}

	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::AddChunkInfo( const CMesh* meshResource )
{
	TDynArray< StringAnsi > myText;
	String depotPath = meshResource->GetDepotPath();

	const CMesh::TLODLevelArray& lodLevelArray = meshResource->GetMeshLODLevels();
	const Uint32 lodCount = lodLevelArray.Size();
	const auto& chunks = meshResource->GetChunks();		// Stop the mesh from constantly unloading/reloading chunks by keeping a handle around
	const TDynArray< String >& matNames = meshResource->GetMaterialNames();

	for( Uint32 j=0; j<lodCount; ++j )
	{
		const CMesh::LODLevel& lodLevel = lodLevelArray[ j ];
		for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
		{
			Int32 chunkIdx = lodLevel.m_chunks[i];
			const auto& chunk = chunks[ chunkIdx ];
			String matName = matNames[ chunk.m_materialID ];

			myText.PushBack( "insert or ignore into chunks (meshcomponentid, chunkIndex, lodNr, material) values (" );
			myText.PushBack( "(select _id from meshcomponents where depotPath='" );
			myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
			myText.PushBack( "' LIMIT 1),'" );
			myText.PushBack( UNICODE_TO_ANSI( ToString( chunkIdx ).AsChar() ) );
			myText.PushBack( "','" );
			myText.PushBack( UNICODE_TO_ANSI( ToString( j ).AsChar() ) );
			myText.PushBack( "','" );
			myText.PushBack( UNICODE_TO_ANSI( matName.AsChar() ) );
			myText.PushBack( "');\n" );
		}
	}
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::AddLodInfo( const CMesh* meshResource )
{
	TDynArray< StringAnsi > myText;
	String depotPath = meshResource->GetDepotPath();
	
	const CMesh::TLODLevelArray& lodLevelArray = meshResource->GetMeshLODLevels();
	const auto& chunks = meshResource->GetChunks();		// Stop the mesh from constantly unloading/reloading chunks by keeping a handle around
	const Uint32 lodCount = lodLevelArray.Size();
	// collect info for all LODs
	

	for( Uint32 j=0; j<lodCount; ++j )
	{
		// Lets add the lods as a table
		myText.PushBack( "insert or ignore into lods (meshcomponentid, lodNR, memorySizeCPU, memorySizeGPU, distance, triangleCount, vertCount, chunkCount, materialCount, bones ) values (" );
		myText.PushBack( "(select _id from meshcomponents where depotPath='" );
		myText.PushBack( UNICODE_TO_ANSI( ToString(depotPath).AsChar() ) );
		myText.PushBack( "' LIMIT 1),'" );
		TDynArray< String > dbLodStrings;
		dbLodStrings.PushBack( ToString( j ) );
		dbLodStrings.PushBack( ToString( meshResource->EstimateMemoryUsageCPU( j ) ) );
		dbLodStrings.PushBack( ToString( meshResource->EstimateMemoryUsageGPU( j ) ) );
		//dbLodStrings.PushBack( ToString( meshResource->GetDefaultLODDistance( j ) ) );
		dbLodStrings.PushBack( ToString( meshResource->GetMeshLODLevels()[j].GetDistance() ) );
		dbLodStrings.PushBack( ToString( meshResource->CountLODTriangles( j ) ) );
		dbLodStrings.PushBack( ToString( meshResource->CountLODVertices( j ) ) );
		dbLodStrings.PushBack( ToString( meshResource->CountLODChunks( j ) ) );
		dbLodStrings.PushBack( ToString( meshResource->CountLODMaterials( j ) ) );
		dbLodStrings.PushBack( ToString( meshResource->CountLODBones( j ) ) );
		myText.PushBack( MakeDBString( dbLodStrings ) );
	}
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::AddMaterialInfo( const CMesh* meshResource, const String entityName )
{
	TDynArray< StringAnsi > myText;
	String depotPath = meshResource->GetDepotPath();

	// Initialize with all materials
	TDynArray< Uint32 > notUsedMaterials;
	const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();
	TDynArray< String > materialNames = meshResource->GetMaterialNames();
	const auto& chunks = meshResource->GetChunks();

	for ( Uint32 i=0; i<materials.Size(); ++i )
	{
		notUsedMaterials.PushBack( i );
	}
	for ( Uint32 i=0; i<chunks.Size(); ++i )
 	{
 		const auto& chunk = chunks[i];
 		notUsedMaterials.Remove( chunk.m_materialID );
 	}
	// Add to the missing materials table if any unused materials !
	for ( Uint32 i=0; i<notUsedMaterials.Size(); ++i )
	{
		const String materialName = meshResource->GetMaterialNames()[ notUsedMaterials[i] ];
		myText.PushBack( "insert or ignore into missingmaterials ( meshcomponentid, name ) values (" );
		myText.PushBack( "(select _id from meshcomponents where depotPath='" );
		myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
		myText.PushBack( "' LIMIT 1),'" );
		myText.PushBack( UNICODE_TO_ANSI( materialName.AsChar() ) );
		myText.PushBack( "');\n" );
	}

	
	for ( Uint32 j=0; j<materials.Size(); j++ )
	{
		IMaterial* material = materials[j].Get();
		if ( material )
		{
			const String materialName = meshResource->GetMaterialNames()[j];
			String baseName = TXT("");
			String usingMimics = TXT("");
			TDynArray< String > blockNames;

			CMaterialGraph* materialGraph = Cast< CMaterialGraph >( material );
			if( materialGraph )
			{
				usingMimics = ToString( materialGraph->IsMimicMaterial() );
				TDynArray< CGraphBlock* > blocks = materialGraph->GraphGetBlocks();
				if ( !blocks.Empty() )
				{ 
					for(auto block: blocks )
					{
						// Get the block names
						blockNames.PushBack( block->GetCaption() );
					}
				}
			}
			String blockNamesString = String::Join( blockNames, TXT(", ") );
			IMaterial* baseMaterial = material->GetMaterialDefinition();
			if ( baseMaterial )
			{
				// Shader name, material graph
				baseName = baseMaterial->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
				MakeNiceString( baseName );		

				// Adding BaseMaterial info here
				myText.PushBack( AddBaseMaterialInfo( baseMaterial ) );
			}		
			String path = TXT("");
			path = material->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
			MakeNiceString( path );

			String diffTexture = TXT("");
			String normTexture = TXT("");
			String detTexture = TXT("");
			String specTexture = TXT("");
			String det1Texture = TXT("");
			String det2Texture = TXT("");
			String tintTexture = TXT("");

			THandle< ITexture > diff = nullptr; 
			THandle< ITexture > norm = nullptr; 
			THandle< ITexture > det = nullptr; 
			THandle< ITexture > det1 = nullptr; 
			THandle< ITexture > det2 = nullptr; 
			THandle< ITexture > spec = nullptr; 
			THandle< ITexture > basedet = nullptr; 
			THandle< ITexture > basedet1 = nullptr; 
			THandle< ITexture > basedet2 = nullptr; 
			THandle< ITexture > tint = nullptr; 

			Bool difffound = material->ReadParameter( CName( TXT("Diffuse") ), diff );
			Bool normfound = material->ReadParameter( CName( TXT("Normal") ), norm );
			Bool detfound = material->ReadParameter( CName( TXT("DetailNormal") ), det );
			Bool det1found = material->ReadParameter( CName( TXT("DetailNormal1") ), det1 );
			Bool det2found = material->ReadParameter( CName( TXT("DetailNormal2") ), det2 );
			Bool specfound = material->ReadParameter( CName( TXT("SpecularTexture") ), spec );
			Bool tintfound = material->ReadParameter( CName( TXT("SpecularTexture") ), tint );
			Bool basedetFound = false;
			Bool basedet1Found = false;
			Bool basedet2Found = false;
			if ( baseMaterial )
			{
				basedetFound = baseMaterial->ReadParameter( CName( TXT("DetailNormal") ), basedet );
				basedet1Found = baseMaterial->ReadParameter( CName( TXT("DetailNormal") ), basedet1 );
				basedet2Found = baseMaterial->ReadParameter( CName( TXT("DetailNormal") ), basedet2 );
			}
			if(difffound && diff){ diffTexture = diff->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") ); }
			if(normfound && norm){ normTexture = norm->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") ); }
			if(specfound && spec){ specTexture = spec->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") ); }
			if(tintfound && tint){ tintTexture = tint->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") ); }
			if(detfound && det && basedetFound && basedet)
			{ 
				detTexture = det->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") );
			}
			if(det1found && det1 && basedet1Found && basedet1)
			{ 
				det1Texture = det1->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") );
			}
			if(det2found && det2 && basedet2Found && basedet2)
			{ 
				det2Texture = det2->GetFriendlyName().StringAfter( TXT("CBitmapTexture ") );
			}


			myText.PushBack( "insert or ignore into materials ( meshcomponentid, entityid, name, base, diffuse, normal, detail, detail1, detail2, spec, tintMask, usingMimics, blockNames) values (" );
			myText.PushBack( "(select _id from meshcomponents where depotPath='" );
			myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
			myText.PushBack( "' LIMIT 1)," );
			myText.PushBack( "(select id from entities where name='" );
			myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
			myText.PushBack( "' LIMIT 1),'" );
			TDynArray< String > dbMatStrings;
			dbMatStrings.PushBack( materialName.AsChar() );
			dbMatStrings.PushBack( baseName.AsChar() );
			dbMatStrings.PushBack( diffTexture );
			dbMatStrings.PushBack( normTexture );
			dbMatStrings.PushBack( detTexture );
			dbMatStrings.PushBack( det1Texture );
			dbMatStrings.PushBack( det2Texture );
			dbMatStrings.PushBack( specTexture );
			dbMatStrings.PushBack( tintTexture );
			dbMatStrings.PushBack( usingMimics );
			dbMatStrings.PushBack( blockNamesString );
			myText.PushBack( MakeDBString( dbMatStrings ) );

			// Adding the textures info here
			TDynArray< CBitmapTexture* > usedTextures;
			material->GatherTexturesUsed( material, j, usedTextures );
			for ( Uint32 tex = 0; tex<usedTextures.Size(); ++tex )
			{
				CBitmapTexture* texture = usedTextures[tex];
				if(texture)
				{
					CBitmapTexture::SStats stats = texture->GetStats();
					String importFile = stats.m_importFile.AsChar();
					String texWidth = ToString(stats.m_width).AsChar();
					String texHeight = ToString(stats.m_height).AsChar();
					String texName = texture->GetFriendlyName().Split(TXT("CBitmapTexture "))[0];
					//ETextureRawFormat enumFormat = stats.m_format;
					String format = CEnum::ToString(stats.m_format).AsChar();

					TextureGroup texGroup = texture->GetTextureGroup();
					String compression = CEnum::ToString(texGroup.m_compression).AsChar();
					myText.PushBack( "insert or ignore into textures ( materialid, name, width, height, format, textureGroup, compression, isUser, isStreamable, isResizable, isDetailMap, isAtlas, maxSize, hasMipchain, pcDownscaleBias, xboxDownscaleBias, ps4DownscaleBias, importFile ) values (" );
					myText.PushBack( "(select materials.id from materials where materials.entityid = (select id from entities where name='" );
					myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
					
					myText.PushBack( "') LIMIT 1),'" );
					TDynArray< String > dbTextureStrings;
					dbTextureStrings.PushBack( texName );
					dbTextureStrings.PushBack( texWidth );
					dbTextureStrings.PushBack( texHeight );
					dbTextureStrings.PushBack( format );
					dbTextureStrings.PushBack( texture->GetTextureGroupName().AsString() );
					dbTextureStrings.PushBack( compression );
					dbTextureStrings.PushBack( ToString( texGroup.m_isUser ) );
					dbTextureStrings.PushBack( ToString( texGroup.m_isStreamable ) );
					dbTextureStrings.PushBack( ToString( texGroup.m_isResizable ) );
					dbTextureStrings.PushBack( ToString( texGroup.m_isDetailMap ) );
					dbTextureStrings.PushBack( ToString( texGroup.m_isAtlas ) );
					dbTextureStrings.PushBack( ToString( texGroup.m_maxSize ) );
					dbTextureStrings.PushBack( ToString( texGroup.m_hasMipchain ) );
					dbTextureStrings.PushBack( ToString( texture->GetPCDownscaleBias() ) );
					dbTextureStrings.PushBack( ToString( texture->GetXBoneDownscaleBias() ) );
					dbTextureStrings.PushBack( ToString( texture->GetPS4DownscaleBias() ) );
					dbTextureStrings.PushBack( importFile );
					myText.PushBack( MakeDBString( dbTextureStrings ) );
				}
			}
		}	
	}
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::AddBaseMaterialInfo( IMaterial* baseMaterial )
{
	TDynArray< StringAnsi > myText;
	// Shader name, material graph
	String baseName = baseMaterial->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
	MakeNiceString( baseName );	

	TDynArray<String> deprecatedBlockNames;

	String rootBlockName( TXT("AbstractRootBlock") );
	TDynArray< String > blockNames;

	String usingMimic;
	String canUseOnParticles;
	String canUseOnMeshes;	
	String canUseOnCollapsableObjects;	

	CMaterialGraph* materialGraph = Cast< CMaterialGraph >( baseMaterial );
	if ( materialGraph )
	{
		TDynArray< CGraphBlock* > graphBlocks = materialGraph->GraphGetBlocks();
		if( !graphBlocks.Empty() )
		{
			for( Uint32 i=0; i<graphBlocks.Size(); ++i )
			{
#ifndef NO_EDITOR_GRAPH_SUPPORT
				if ( graphBlocks[i]->GetBlockCategory() == TXT("Deprecated")) // Temporary visual indicator to help removing deprecated nodes
				{
					deprecatedBlockNames.PushBackUnique( graphBlocks[i]->GetBlockName() );
				}
#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
				CMaterialRootBlock* rootBlock = Cast< CMaterialRootBlock >( graphBlocks[i] );
				if ( rootBlock )
				{
					if ( rootBlock->IsA< CMaterialBlockOutputVertexModifiers >() )
					{
						continue;
					}
					rootBlockName = rootBlock->GetBlockName();
				}
#endif // NO_RUNTIME_MATERIAL_COMPILATION
				// Get the block names
				blockNames.PushBack( graphBlocks[i]->GetFriendlyName() );
			}
			usingMimic = ToString( materialGraph->IsMimicMaterial() );
		}

 		canUseOnParticles			= ToString( materialGraph->CanUseOnParticles() );
 		canUseOnMeshes				= ToString( materialGraph->CanUseOnMeshes() ); 		
 		canUseOnCollapsableObjects	= ToString( materialGraph->CanUseOnCollapsableObjects() );

	}
	String deprecatedBlockNamesString = String::Join( deprecatedBlockNames, TXT(", ") );
	String blockNamesString = String::Join( blockNames, TXT(", ") );
	myText.PushBack( "insert or ignore into basematerials ( path, rootBlockName, deprecatedBlocks, usingMimics, blockNames, canUseOnParticles, canUseOnMeshes, canUseOnCollapsableObjects ) values ('");
	TDynArray< String > dbBaseMaterialStrings;
	dbBaseMaterialStrings.PushBack( baseName );
	dbBaseMaterialStrings.PushBack( rootBlockName );
	dbBaseMaterialStrings.PushBack( deprecatedBlockNamesString );
	dbBaseMaterialStrings.PushBack( usingMimic );
	dbBaseMaterialStrings.PushBack( blockNamesString );
	dbBaseMaterialStrings.PushBack( canUseOnParticles );
	dbBaseMaterialStrings.PushBack( canUseOnMeshes );	
	dbBaseMaterialStrings.PushBack( canUseOnCollapsableObjects );
	myText.PushBack( MakeDBString( dbBaseMaterialStrings ) );

	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::AddCollisionMeshInfo( const CMesh* meshResource )
{
	TDynArray< StringAnsi > myText;
	String collisionShapeCount = TXT("");
	String collisionVerts = TXT("");
	String collisionTriangles = TXT("");
	String collisionShape = TXT("");
	String physicalMaterialName = TXT("");
	String depotPath = meshResource->GetDepotPath();

	const CCollisionMesh* collisionMesh = meshResource->GetCollisionMesh();

	if( !collisionMesh )
	{

	}
	else
	{
		const TDynArray< ICollisionShape* >& shapes = collisionMesh->GetShapes();
		Uint32 vert, indices;
		collisionShapeCount = ToString( shapes.Size() );
		for( Uint32 j = 0; j <shapes.Size(); ++j )
		{
			ICollisionShape* shape = shapes[j];
			//physicalMaterialName = shape->GetPhysicalMaterial().AsChar();

			// Split the string to its parts, removing the CCLASS name which the user is not interested of
			String cName = shape->GetLocalClass()->GetName().AsChar();
			TDynArray< String > parts = cName.Split( TXT("CCollisionShape") );
			if ( !parts.Empty() )
			{
				shape->GetStats( vert,indices );
				collisionVerts = ToString( vert );
				collisionTriangles = ToString( indices/3 ) ;
				collisionShape = parts[0];

				Uint32 numMaterials = shape->GetNumPhysicalMaterials();
				TDynArray<String> mtlNames;
				for ( Uint32 i = 0; i < numMaterials; ++i )
				{
					mtlNames.PushBackUnique( shapes[j]->GetPhysicalMaterial( i ).AsString() );
				}

				String materialName = String::Join( mtlNames, TXT(":: ") );
				physicalMaterialName = materialName.AsChar();

				myText.PushBack( "insert or ignore into collisionshapes (meshcomponentid, collisionShape, vertCount, triangleCount, material ) values (" );
				myText.PushBack( "(select _id from meshcomponents where depotPath='" );
				myText.PushBack( UNICODE_TO_ANSI( ToString(depotPath).AsChar() ) );
				myText.PushBack( "' LIMIT 1),'" );
				TDynArray< String > dbColShapeStrings;
				dbColShapeStrings.PushBack( collisionShape );
				dbColShapeStrings.PushBack( collisionVerts );
				dbColShapeStrings.PushBack( collisionTriangles );
				dbColShapeStrings.PushBack( physicalMaterialName );
				myText.PushBack( MakeDBString( dbColShapeStrings ) );
			}
		}
	}
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::MakeDBString( TDynArray< String > dbStrings )
{
	TDynArray<StringAnsi> myText;
	String originalLine = String::EMPTY;
	Bool corruptedLinesFlag = false;

	for( Uint32 i=0; i<dbStrings.Size(); ++i )
	{
		String currString = dbStrings[i];

		if ( currString.ContainsCharacter( TXT('\n') ) )
		{
			corruptedLinesFlag = true;
			currString.Erase( RemoveIf( currString.Begin(), currString.End(), []( Char& c ){ return c == '\n'; } ) );
		}

		myText.PushBack( UNICODE_TO_ANSI( currString.AsChar() ) );
		originalLine += dbStrings[i];

		if( i == dbStrings.Size()-1 )
		{
			myText.PushBack( "');\n" );
			originalLine += TXT( "');\n" );
		}
		else
		{
			myText.PushBack( "','" );
			originalLine += TXT("','");
		}
	}

	if ( corruptedLinesFlag )
	{
		g_corruptedLines.PushBack( originalLine );
	}

	return myText;
}

void WorldSceneDependencyInfo::MakeNiceString( String& myString )
{
	myString.ReplaceAll( TXT("'"), TXT("@") );
	myString.Trim();
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetParticleInfo( const CComponent* component, const String entityName )
{
	TDynArray<StringAnsi> myText;
	Vector pos = component->GetWorldPositionRef();

	CParticleComponent* resource = (CParticleComponent*)component;
	String particleName = resource->GetName();
	
	const CParticleSystem* particleSystem = resource->GetParticleSystem();
	
//	(_particlesystemid INTEGER PRIMARY KEY, entityid INTEGER, posx INTEGER, posy INTEGER, posz INTEGER, name, depotPath, FOREIGN KEY(entityid) REFERENCES Entities(id));\n");
//		s (id INTEGER PRIMARY KEY, particlesystemid INTEGER, maxParticles, FOREIGN KEY(particlesystemid) REFERENCES ParticleSystems(_particlesystemid));\n");
//		(id INTEGER PRIMARY KEY, particleemitterid INTEGER, name, FOREIGN KEY(particleemitterid) REFERENCES ParticleEmitters(id));\n");
	if( particleSystem )
	{
		String depotPath = particleSystem->GetDepotPath();
		// Send to ParticleSystem Table
		String autohideDistance = ToString( particleSystem->GetAutoHideDistance() );
		myText.PushBack( "insert or ignore into particlesystems (entityid, autohideDistance, posx, posy, posz, depotPath ) values (" );
		myText.PushBack( "(select id from entities where name='" );
		myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
		myText.PushBack( "' LIMIT 1),'" );

		TDynArray< String > dbStrings;

		dbStrings.PushBack( autohideDistance );
		dbStrings.PushBack( ToString( pos.X ) );
		dbStrings.PushBack( ToString( pos.Y ) );
		dbStrings.PushBack( ToString( pos.Z ) );
		dbStrings.PushBack( ToString( depotPath ) );
		// Making a proper db string out of all the data that we gathered
		myText.PushBack( MakeDBString( dbStrings ) );

		// Insert into Particle Emitters Table
		TDynArray< CParticleEmitter* > particleEmitters = particleSystem->GetEmitters();
		if ( !particleEmitters.Empty() )
		{
			for ( Uint32 i=0; i<particleEmitters.Size(); ++i )
			{
				// Adding this info into its own table called emitters and referencing it back to particle system
				String maxParticles = ToString( particleEmitters[i]->GetMaxParticles() );
				IMaterial* material = particleEmitters[i]->GetMaterial();
				//String materialName = material->GetFriendlyName();
				String baseName = TXT("");
				String uniqueID = ToString( particleEmitters[i]->GetUniqueId() );
				if ( material )
				{
					//IMaterial* baseMaterialDefinition = material->GetMaterialDefinition();
					//if ( baseMaterialDefinition )
					//{
					//	// Shader name, material graph
					//	baseDefName = baseMaterialDefinition->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
					//	MakeNiceString( baseDefName );	
					//}
					IMaterial* baseMaterial = material->GetBaseMaterial();
					if ( baseMaterial )
					{
						// Shader name, material graph
						baseName = baseMaterial->GetFriendlyName().Split(TXT("CMaterialGraph ") )[0];
						MakeNiceString( baseName );	

						// Adding BaseMaterial info here
						myText.PushBack( AddBaseMaterialInfo( baseMaterial ) );
					}
					else
					{
						baseName = material->GetFriendlyName();
					}
				}
				String isEnabled = ToString( particleEmitters[i]->IsEnabled() );

				myText.PushBack( "insert or ignore into particleemitters ( particlesystemid, enabled, maxParticles, materialBase ) values (" );
				myText.PushBack( "(select _particlesystemid from particlesystems where depotPath='" );
				myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
				myText.PushBack( "' AND posx=" );
				myText.PushBack( UNICODE_TO_ANSI( ToString( pos.X ).AsChar() ) );
				myText.PushBack( " AND posy=" );
				myText.PushBack( UNICODE_TO_ANSI( ToString( pos.Y ).AsChar() ) );
				myText.PushBack( " AND posz=" );
				myText.PushBack( UNICODE_TO_ANSI( ToString( pos.Z ).AsChar() ) );
				myText.PushBack( " LIMIT 1),'" );
				myText.PushBack( UNICODE_TO_ANSI( isEnabled.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( maxParticles.AsChar() ) );
				myText.PushBack( "','" );
				myText.PushBack( UNICODE_TO_ANSI( baseName.AsChar() ) );
				myText.PushBack( "');\n" );

				if( material )
				{
					IMaterial* baseMaterial = material->GetBaseMaterial();
					if( baseMaterial )
					{
						TDynArray< String > blockNames;
						CMaterialGraph* materialGraph = Cast< CMaterialGraph >( baseMaterial );
						if( materialGraph )
						{
							TDynArray< CGraphBlock* > blocks = materialGraph->GraphGetBlocks();
							if ( !blocks.Empty() )
							{ 
								for(auto block: blocks )
								{
									// Get the block names
									blockNames.PushBack( block->GetCaption() );
								}
							}
						}
						String blockNamesString = String::Join( blockNames, TXT(", ") );
						myText.PushBack( "insert or ignore into particlematerials ( particleemitterid, base, blockNames, textureCount ) values (" );
						myText.PushBack( "(select id from particleemitters where particlesystemid = (select _particlesystemid from particlesystems where entityid=(select id from entities where name='" );
						
						myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );

						myText.PushBack( "')) LIMIT 1),'" );

						myText.PushBack( UNICODE_TO_ANSI( baseName.AsChar() ) );
						myText.PushBack( "','" );
						myText.PushBack( UNICODE_TO_ANSI( blockNamesString.AsChar() ) );
						myText.PushBack( "','" );

						TDynArray< CBitmapTexture* > usedTextures;
						material->GatherTexturesUsed( material, 0, usedTextures );
						String textureCount = ToString( usedTextures.Size() );

						myText.PushBack( UNICODE_TO_ANSI( textureCount.AsChar() ) );
						myText.PushBack( "');\n" );

						// Adding the textures info here
						for ( Uint32 tex = 0; tex<usedTextures.Size(); ++tex )
						{
							//CBitmapTexture* texture = usedTextures[tex]->m_texture;
							CBitmapTexture* texture = usedTextures[tex];
							if(texture)
							{
								CBitmapTexture::SStats stats = texture->GetStats();
								String importFile = stats.m_importFile.AsChar();
								String texWidth = ToString(stats.m_width).AsChar();
								String texHeight = ToString(stats.m_height).AsChar();
								String texName = texture->GetFriendlyName().Split(TXT("CBitmapTexture "))[0];
								String format = CEnum::ToString(stats.m_format).AsChar();

								TextureGroup texGroup = texture->GetTextureGroup();
								String compression = CEnum::ToString(texGroup.m_compression).AsChar();

								myText.PushBack( "insert or ignore into particletextures ( particlematerialid, name, width, height, format, textureGroup, compression, isUser, isStreamable, isResizable, isDetailMap, isAtlas, maxSize, hasMipchain, pcDownscaleBias, xboxDownscaleBias, ps4DownscaleBias, importFile ) values (" );
								myText.PushBack( "(select id from particlematerials where particleemitterid = (select id from particleemitters where particlesystemid = (select _particlesystemid from particlesystems where entityid=(select id from entities where name='" );
								myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
								myText.PushBack( "'))) LIMIT 1),'" );

								TDynArray< String > dbTextureStrings;
								dbTextureStrings.PushBack( texName );
								dbTextureStrings.PushBack( texWidth );
								dbTextureStrings.PushBack( texHeight );
								dbTextureStrings.PushBack( format );
								dbTextureStrings.PushBack( texture->GetTextureGroupName().AsString() );
								dbTextureStrings.PushBack( compression );
								dbTextureStrings.PushBack( ToString( texGroup.m_isUser ) );
								dbTextureStrings.PushBack( ToString( texGroup.m_isStreamable ) );
								dbTextureStrings.PushBack( ToString( texGroup.m_isResizable ) );
								dbTextureStrings.PushBack( ToString( texGroup.m_isDetailMap ) );
								dbTextureStrings.PushBack( ToString( texGroup.m_isAtlas ) );
								dbTextureStrings.PushBack( ToString( texGroup.m_maxSize ) );
								dbTextureStrings.PushBack( ToString( texGroup.m_hasMipchain ) );
								dbTextureStrings.PushBack( ToString( texture->GetPCDownscaleBias() ) );
								dbTextureStrings.PushBack( ToString( texture->GetXBoneDownscaleBias() ) );
								dbTextureStrings.PushBack( ToString( texture->GetPS4DownscaleBias() ) );
								dbTextureStrings.PushBack( importFile );
								myText.PushBack( MakeDBString( dbTextureStrings ) );
							}
						}
					} 
				}

				TDynArray< IParticleModule* > modules = particleEmitters[i]->GetModules();
				for( Uint32 j=0; j<modules.Size(); ++j )
				{
					if ( modules[j] == nullptr )
					{
						continue;
					}
					// Adding this info into its own table called modules and referencing it back to particle emitter
					String name = modules[j]->GetEditorName();
					String isEnabled = ToString( modules[j]->IsEnabled() );

					myText.PushBack( "insert or ignore into particlemodules ( particleemitterid, name, isEnabled ) values (" );
					myText.PushBack( "(select particleemitters.id from particleemitters, particlesystems where particleemitters.particlesystemid = particlesystems._particlesystemid AND particlesystems.depotPath='" );					
					myText.PushBack( UNICODE_TO_ANSI( depotPath.AsChar() ) );
					myText.PushBack( "' LIMIT 1),'" );
					myText.PushBack( UNICODE_TO_ANSI( name.AsChar() ) );
					myText.PushBack( "','" );
					myText.PushBack( UNICODE_TO_ANSI( isEnabled.AsChar() ) );
					myText.PushBack( "');\n" );
				}
			}
		}
		
	}
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetLightInfo( const CComponent* component, const String entityName )
{
	TDynArray<StringAnsi> myText;
	Vector pos = component->GetWorldPositionRef();
	CLightComponent* lightResource = (CLightComponent*)component;
	String lightRadius = ToString( lightResource->GetRadius() );
	String lightEnabled = ToString( lightResource->IsEnabled() );
	String castingShadow = ToString( lightResource->IsCastingShadows() );
	ELightShadowCastingMode lightShadowMode =  lightResource->GetShadowCastingMode();
	String shadowCastingMode = CEnum::ToString(lightShadowMode).AsChar();
	TDynArray<String> name = lightResource->GetFriendlyName().Split(TXT("CLayer "));

	myText.PushBack( "insert or ignore into lights (entityid, posx, posy, posz, lightRadius, lightEnabled, castingShadow, lightShadowMode ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > lightStrings;

	lightStrings.PushBack( ToString( pos.X ) );
	lightStrings.PushBack( ToString( pos.Y ) );
	lightStrings.PushBack( ToString( pos.Z ) );
	lightStrings.PushBack( ToString( lightRadius ) );
	lightStrings.PushBack( ToString( lightEnabled ) );
	lightStrings.PushBack( ToString( castingShadow ) );
	lightStrings.PushBack( ToString( shadowCastingMode ) );

	myText.PushBack( MakeDBString( lightStrings ) );
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetDimmerInfo( const CComponent* component, const String entityName )
{
	TDynArray<StringAnsi> myText;
	Vector pos = component->GetWorldPositionRef();
	CDimmerComponent* dimResource = (CDimmerComponent*)component;
	String ambientLevel = ToString( dimResource->GetAmbientLevel() );
	String marginFactor = ToString( dimResource->GetMarginFactor() );
	EDimmerType dimmerTypeEnum =  dimResource->GetDimmerType();
	String dimmerType = CEnum::ToString(dimmerTypeEnum).AsChar();

	//ambientLevel, marginFactor, dimmerType,

	myText.PushBack( "insert or ignore into dimmers (entityid, posx, posy, posz, ambientLevel, marginFactor, dimmerType ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > dimStrings;

	dimStrings.PushBack( ToString( pos.X ) );
	dimStrings.PushBack( ToString( pos.Y ) );
	dimStrings.PushBack( ToString( pos.Z ) );
	dimStrings.PushBack( ToString( ambientLevel ) );
	dimStrings.PushBack( ToString( marginFactor ) );
	dimStrings.PushBack( ToString( dimmerType ) );

	myText.PushBack( MakeDBString( dimStrings ) );
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetSoundEmitterInfo( const CComponent* component, const String entityName )
{
	TDynArray<StringAnsi> myText;
	CSoundEmitterComponent* resource = (CSoundEmitterComponent*)component;
	Vector pos = component->GetWorldPositionRef();
	String resourceName = resource->GetName();
	StringAnsi objName = resource->GetSoundObjectName().Split( "CLayer " )[0];
	String soundObjectName = ToString(objName);
	MakeNiceString( soundObjectName );

	StringAnsi loopStart = resource->GetLoopStart();
	StringAnsi loopStop = resource->GetLoopStop();
	StringAnsi loopIntensity = resource->GetLoopIntensity();
	Float maxDistance = resource->GetMaxDistance();

	myText.PushBack( "insert or ignore into SoundEmitterComponents (entityid, posx, posy, posz, name, objectName, loopStart, loopStop, loopIntensity, maxDistance ) values (" );
	myText.PushBack( "(select id from entities where name='" );
	myText.PushBack( UNICODE_TO_ANSI( entityName.AsChar() ) );
	myText.PushBack( "' LIMIT 1),'" );

	TDynArray< String > soundStrings;

	soundStrings.PushBack( ToString( pos.X ) );
	soundStrings.PushBack( ToString( pos.Y ) );
	soundStrings.PushBack( ToString( pos.Z ) );
	soundStrings.PushBack( resourceName );
	soundStrings.PushBack( ToString( soundObjectName ) );
	soundStrings.PushBack( ToString( loopStart ) );
	soundStrings.PushBack( ToString( loopStop )  );
	soundStrings.PushBack( ToString( loopIntensity )  );
	soundStrings.PushBack( ToString( maxDistance )  );

	myText.PushBack( MakeDBString( soundStrings ) );
	return myText;
}

TDynArray< StringAnsi > WorldSceneDependencyInfo::GetExplorationInfo( const CComponent* component )
{
	TDynArray<StringAnsi> myText;
	CExplorationComponent* resource = (CExplorationComponent*)component;
	String resourceName = resource->GetName();
	CObject* expObject = resource->GetObjectForEvents();
	String friendlyName = expObject->GetFriendlyName().Split(TXT("CLayer "))[0];

	myText.PushBack( "\n\t\t\tEXPLORATION NAME: " );
	myText.PushBack( UNICODE_TO_ANSI( resourceName.AsChar() ) );
	myText.PushBack( "\n\t\t\tEXPLORATION OBJECT: " );
	myText.PushBack( UNICODE_TO_ANSI( friendlyName.AsChar() ) );
	return myText;
}
#endif