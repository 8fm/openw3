/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../game/equipmentState.h"
#include "../engine/flashRenderTarget.h"
#include "../engine/flashRenderSceneProvider.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CGameWorld;
class CRenderFrame;
class CJobSpawnEntity;
class CEntityTemplate;
class IGuiScenePlayerListener;
class CBitmapTexture;

// Some internal configuration of scene rendering and behaviors
struct SGuiScenePlayerInfo
{

	Float			m_cameraFarPlane;		//!< How far camera can see in our preview fake world

	Bool			m_updatePhysics;
	Bool			m_uniquePhysicsWorld;	//!< Should there be created separate world for physics simulation
	EShowFlags		m_renderingMask[ SHOW_MAX_INDEX ];

	THandle<CBitmapTexture> m_backgroundTexture;

	SGuiScenePlayerInfo();

	~SGuiScenePlayerInfo();

};

struct SGuiEnhancementInfo
{
	DECLARE_RTTI_STRUCT( SGuiEnhancementInfo );

	CName	m_enhancedItem;
	CName	m_enhancement;

	CName	m_oilItem;
	CName	m_oil;
	
	CName	m_dyeItem;
	CName	m_dye;
	Int32  m_dyeColor;
};

BEGIN_CLASS_RTTI( SGuiEnhancementInfo );
	PROPERTY( m_enhancedItem );
	PROPERTY( m_enhancement );
	PROPERTY( m_oilItem);
	PROPERTY( m_oil);
	PROPERTY( m_dyeItem );
	PROPERTY( m_dye );
	PROPERTY( m_dyeColor );	
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CGuiScenePlayer
//////////////////////////////////////////////////////////////////////////
class CGuiScenePlayer : public CObject, public IFlashRenderSceneProvider
{
	DECLARE_ENGINE_CLASS( CGuiScenePlayer, CObject, CF_AlwaysTransient );

private:
	enum { MaxCancelledSpawnJobs = 2 };
	
private:
	SFlashRenderTargetCamera							m_camera;

private:

	struct SNextSpawnInfo
	{

		THandle< CEntityTemplate >						m_entityTemplate;
		CName											m_animation;

		RED_INLINE Bool	IsValid() const { return m_entityTemplate.Get() != nullptr; }

		RED_INLINE void	Clear() { m_entityTemplate = nullptr; }

		SNextSpawnInfo()
			: m_entityTemplate( nullptr )
			, m_animation( TXT("locomotion_idle" ) )
		{}

	};

	struct SSpawnJobContext
	{
		CJobSpawnEntity*							m_spawnJob;
		SNextSpawnInfo								m_spawnInfo;
		THandle< CEntity >							m_spawnedEntity; // GC handled by world
		
		RED_INLINE Bool IsValid() const { return m_spawnInfo.IsValid(); }

		SSpawnJobContext()
			: m_spawnJob( NULL )
			, m_spawnedEntity( nullptr )
		{}

		SSpawnJobContext( CJobSpawnEntity* spawnJob, const SNextSpawnInfo& spawnInfo )
			: m_spawnJob( spawnJob )
			, m_spawnInfo( spawnInfo )
		{}
	};

private:
	TDynArray< SSpawnJobContext >					m_cancelledSpawnJobContexts;
	SSpawnJobContext								m_currentSpawnJobContext;
	SNextSpawnInfo									m_nextSpawn;
	TDynArray< SItemUniqueId >						m_delayedItemsSpawn;
	TDynArray< SGuiEnhancementInfo >				m_delayedEnhancementsItemsSpawn;
	TDynArray< SItemUniqueId >						m_delayedEnhancementsItems;
	TDynArray< SItemUniqueId >						m_spawningItems;
private:
	TDynArray< IGuiScenePlayerListener* >			m_listeners;

private:
	THandle< CWorld >								m_world;
	THandle< CEntity >								m_entity;
	THashMap< CName, CItemEntity* >					m_itemsArray;
	Vector											m_initialEntityPosition;

	SGuiScenePlayerInfo								m_sceneInfo;
	CRenderFrameInfo								m_cachedFrameInfo;

public:
													CGuiScenePlayer();
	virtual											~CGuiScenePlayer();

	Bool											Init( CClass* worldClass = ClassID< CWorld >() );
	void											Uninit();
	virtual void									OnSerialize( IFile& file ) override;

public:
	void											SetEntityTemplate( const THandle< CEntityTemplate >& entityTemplate , const CName& animationName );
	void											ApplyAppearanceToSceneEntity( const CName& appearanceName );
	Bool											ApplyItemsToSceneEntity( CEntity* entity, const TDynArray< SItemUniqueId >& items, const TDynArray< SGuiEnhancementInfo >& enhancementsItems, TDynArray< SItemUniqueId >* delayedEnhancementsItems );
	void											ApplyFxToSpawnedItems();
	void											ApplyAnimationOnEntity( CEntity* entity, const CName& animationName );
	void											UpdateEntityItems( const TDynArray< SItemUniqueId >& items, const TDynArray< SGuiEnhancementInfo >& enhancements );
	void											SetEnvironment( const THandle< CEnvironmentDefinition >& envDef, const EulerAngles& sunRotation );
	void											EnablePhysics( Bool value );
	void											SetShadowCascadeSettings( const Vector& cascadeRanges, const Vector& cascadeKernels , Uint32 numCascades );
	void											ClearScene( );

public:
//	THandle< CEntity >								GetLoadedEntity() const;

public:
	virtual void									Tick( Float timeDelta ) override;
	virtual const SFlashRenderTargetCamera&			GetDefaultCamera() const override { return m_camera; }

	//!< Render world of the current scene onto provided render target
	virtual void									RenderWorld( IRenderGameplayRenderTarget* renderTarget );

public:
	void											UpdateCamera( const Vector& position, const EulerAngles& rotation, Float fov );
	void											UpdateEntityTransform( const Vector* position, const EulerAngles* rotation,  const Vector* scale );

	void											UpdateBackgroundTexture( CBitmapTexture* resource );

public:
	Bool											RegisterListener( IGuiScenePlayerListener* listener );
	Bool											UnregisterListener( IGuiScenePlayerListener* listener );

private:
	CWorld*											CreateWorld( CClass* worldClass, CObject* parent );
	CJobSpawnEntity*								CreateSpawnJob( const CGuiScenePlayer::SNextSpawnInfo& spawnInfo, CWorld* world );
	void											UpdateWorldEnvironment( CWorld* world, CEnvironmentDefinition* envDef, const EulerAngles& sunRotation );

	Vector											GetSpawnPosition();

private:
	void											TickSpawnJobs();
	void											TickWorld( Float timeDelta );

private:
	Bool											InitEntity( CEntity* entity, const SNextSpawnInfo& spawnInfo );

private:
	Bool											EnhanceItem( CInventoryComponent& inventoryComponent, CName item, const TDynArray< SGuiEnhancementInfo >& enhancementsItems, Bool updateEnhancement = false );
	Int32											GetEnhancementNum( CName item, const TDynArray< SGuiEnhancementInfo >& enhancementsItems );

	CName											GetOilFxName( CName oilName );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI_EX( CGuiScenePlayer, CF_AlwaysTransient );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();