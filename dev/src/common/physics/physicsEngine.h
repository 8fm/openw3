#pragma once

#include "../physics/physicalMaterial.h"
#include "../core/events.h"

#define COLLISION_TYPE_MAX 48

class CPhysicsEngine 
#ifndef NO_EDITOR_EVENT_SYSTEM
	: public IEdEventListener
#endif
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_PhysicsEngine );

public:
	typedef Uint64 CollisionMask;

protected:
	TDynArray< CName > m_collisionTypeNames;
	CollisionMask* m_collisionSetMasks;

	TDynArray< SPhysicalMaterial > m_physicalMaterials;

	typedef THashMap< const CName, unsigned char > TextureFileNameToPhysicalMaterialIndexHashMap;
	TextureFileNameToPhysicalMaterialIndexHashMap m_texturesPhysicalMaterials;

	void ReloadCollisionGroupDefinitions();
	void ReloadPhysicalMaterials();
	void ClearMaterials();

	virtual void* CreateMaterialInstance( Float dynamicFriction, Float staticFriction, Float restitution, void* engineDefinition ) { return 0; }
	virtual void FlushMaterialInstances() {}

#ifndef NO_EDITOR_EVENT_SYSTEM
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
#endif

public:
	CPhysicsEngine();
	virtual ~CPhysicsEngine();

	virtual Bool Init();
	virtual void ShutDown();

	virtual void Update( Float timeDelta );

	virtual class CPhysicsWorld* CreateWorld( class IPhysicsWorldParentProvider* parentProvider, Uint32 areaResolution, Float areaSize, Vector2 areaCornerPosition, Uint32 clipMapSize, Uint32 tileRes, Bool useMemorySettings, Bool criticalDispatch, Bool allowGpu );
	virtual void DestroyWorld( class CPhysicsWorld* world );

    void          GetGroupsNamesByCollisionMask( CollisionMask mask, TDynArray< CName > & names );
	CollisionMask GetCollisionTypeBit( const CName& name );
	CollisionMask GetCollisionTypeBit( const TDynArray< CName >& names );
	CollisionMask GetCollisionGroupMask( const CName& name );
	CollisionMask GetCollisionGroupMask( const TDynArray< CName >& names );
	CollisionMask GetCollisionGroupMask( const CollisionMask& mask );

	static CollisionMask GetWithAllCollisionMask() { return 0x00FFFFFFFFFFFFFF; }

	const String GetCollisionMaskFileResourceName() const;
	const String GetPhysicalMaterialFileResourceName() const;
	const String GetTerrainPhysicalMaterialFileResourceName() const;

	const SPhysicalMaterial* GetMaterial( const CName& name = CNAME( default ) );
	const SPhysicalMaterial* GetMaterial( Uint32 index );

	const SPhysicalMaterial* GetTexturePhysicalMaterial( const CName& texture );
	char GetTexturePhysicalMaterialIndex( const CName& texture );

	unsigned char GetCollisionTypesMaxLimit() const { return 48; }

#ifndef NO_EDITOR
	const TDynArray< CName >& GetCollsionTypeNames() const { return m_collisionTypeNames; }
	void GetPhysicalMaterialNames( TDynArray< CName >& names ) const { for( Uint32 i = 0; i != m_physicalMaterials.Size(); ++i ) names.PushBack( m_physicalMaterials[ i ].m_name); }
#endif

	void GenerateEditorFragments( class CRenderFrame* frame );
};

extern CPhysicsEngine* GPhysicEngine;
