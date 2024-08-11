/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_FOLIAGE_RENDER_COMMAND_FACTORY_H_
#define _ENGINE_FOLIAGE_RENDER_COMMAND_FACTORY_H_

#include "dynamicCollisionCollector.h"
#include "foliageInstance.h"
#include "grassCellMask.h"

class CSRTBaseTree;
class IRenderObject;
class IRender;
class IRenderScene;
class CFoliageInstancesData;
class CGenericGrassMask;
class IRenderProxy;
struct Vector3;
struct Box;

struct SFoliageRenderParams;
enum EFoliageVisualisationMode : Uint32;

class IFoliageRenderCommandDispatcher
{
public:
	IFoliageRenderCommandDispatcher() { }
	virtual ~IFoliageRenderCommandDispatcher() { }

	virtual void UpdateSpeedTreeInstancesCommand( SFoliageUpdateRequest& updateRequest ) const = 0;
	virtual void UpdateGrassSetupCommand( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData ) const = 0;
	virtual void UpdateDynamicGrassCollisionCommand( const TDynArray< SDynamicCollider >& collisions ) const = 0;
	virtual void UpdateGenericGrassMaskCommand( IRenderProxy * terrainProxy, CGenericGrassMask * grassMask ) const = 0;

	virtual void CreateSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer & instanceData, const Box& rect ) const = 0;
	virtual void CreateSpeedTreeDynamicInstancesCommand( const CSRTBaseTree * tree,  const FoliageInstanceContainer & instanceData, const Box& rect ) const = 0;
	
	virtual void RemoveSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const Box& rect ) const = 0;
	virtual void RemoveSpeedTreeDynamicInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const = 0;
	virtual void RemoveSpeedTreeInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const = 0;
	
	virtual void UploadGrassOccurrenceMasks( const TDynArray< CGrassCellMask >& cellMasks ) const = 0;
	virtual void RefreshGenericGrassCommand() const = 0;
	
	virtual void SetDebugVisualisationModeCommand( EFoliageVisualisationMode mode ) const = 0;

#ifndef NO_EDITOR
	virtual void UpdateFoliageRenderParams( const SFoliageRenderParams &params ) const = 0;
#endif // !NO_EDITOR

	virtual void SetTreeFadingReferencePoints( const Vector& left, const Vector& right, const Vector& center ) = 0;
};

Red::TUniquePtr< IFoliageRenderCommandDispatcher > CreateFoliageRenderCommandDispatcher( IRenderScene * scene );

#endif
