/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/uniquePtr.h"
#include "../core/sharedPtr.h"
#include "foliageForward.h"
#include "foliageLodSetting.h"

class CFoliageBroker;
class IFoliageRenderCommandDispatcher;
class CFoliageResourceHandler;
class CFoliageCollisionHandler;
class CFoliageEditionController;
class CFoliageDynamicInstanceService;
class CPhysicsWorld;
class CGenericGrassMask;
class IRenderScene;
class IRenderProxy;


// This class acts as the main entry point for the foliage data
// It handles storage of streamed / raw foliage layer resources and is stored with the world object
class CFoliageScene : public CObject
{
	DECLARE_ENGINE_CLASS( CFoliageScene, CObject, 0 )

public:

	CFoliageScene();
	virtual ~CFoliageScene();

	virtual void OnPropertyPostChange( IProperty* property ) override final;

	void Initialize( IRenderScene * renderScene );
	void Shutdown();
	void Update( const Vector & position );
	void Tick();

	void PrefetchPositionSync( const Vector & position );
	void PrefetchArea( const Box * boxArray, Uint32 boxCount );

	bool IsLoading() const;

	void SetWorldDimensions( const Vector2 & dimension );
	void SetCellDimensions( const Vector2 & cellDimension );
	
	Red::TUniquePtr< CFoliageEditionController > CreateEditionController();
	CFoliageDynamicInstanceService CreateDynamicInstanceController();

	// foliageScene control Cells AND GrassMask. SRP is not respected here. Extract class for grass mask!
	void UploadGenericGrassData( IRenderProxy * terrainProxy ) const;
	CGenericGrassMask * GetInternalGrassMask(); 
	void SetGenericGrassMask( CGenericGrassMask * grassMask );

	void SetTreeFadingReferencePoints( const Vector& left, const Vector& right, const Vector& center );

	void SetInternalFoliageBroker( Red::TSharedPtr< CFoliageBroker > broker );
	void SetInternalFoliageRenderCommandDispatcher( Red::TUniquePtr< IFoliageRenderCommandDispatcher > dispatcher );
	void SetInternalFoliageResourceHandler( Red::TUniquePtr< CFoliageResourceHandler > handler );
	void SetInternalStreamingConfig( bool value );

	const Vector & GetCurrentPosition() const;

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker );
#endif

	CFoliageBroker* GetFoliageBroker( ) const { return m_foliageBroker.Get( ); }

private:
	
	void InitializeGrassMask();
	void UpdateVisibilityDepth();

	Red::TSharedPtr< CFoliageBroker > m_foliageBroker;
	Red::TUniquePtr< IFoliageRenderCommandDispatcher > m_renderCommandDispatcher;
	Red::TSharedPtr< CFoliageResourceHandler > m_resourceHandler;
	Red::TUniquePtr< CFoliageCollisionHandler > m_collisionHandler;
	
	Vector2 m_worldDimensions;
	Vector2 m_cellDimensions;
	Int32 m_visibilityDepth;
	Int32 m_editorVisibilityDepth;
	THandle< CGenericGrassMask > m_grassMask;
	Vector m_currentPosition;
	CGrassOccurrenceMap * m_grassOccurrenceMap;
	SFoliageLODSetting m_lodSetting;
	bool m_forceUpdate;
};

BEGIN_CLASS_RTTI( CFoliageScene )
	PARENT_CLASS( CObject )
	PROPERTY( m_worldDimensions )
	PROPERTY( m_cellDimensions )
	PROPERTY_EDIT( m_visibilityDepth, TXT( "[GAME ONLY] Foliage cell that are visible from camera position, in one dimension" ) )
	PROPERTY_EDIT( m_editorVisibilityDepth, TXT( "[EDITOR ONLY] Foliage cell that are visible from camera position, in one dimension" ) )
	PROPERTY( m_grassMask )
	PROPERTY( m_grassOccurrenceMap )
	PROPERTY_EDIT( m_lodSetting, TXT("") )
END_CLASS_RTTI()
