#pragma once

#include "../../common/engine/entity.h"
#include "../../common/engine/triggerAreaComponent.h"
#include "../../common/game/binaryStorage.h"
#include "../../common/engine/weatherManager.h"

//#include "../../common/game/actor.h"

// interior component should be placed inside entity templates of generic building placed around the world
// interior entity has the interior component inside and should be used for interiors that are not inside one entity template (large, unique caves etc)
class CActor;

class CR4InteriorAreaComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CR4InteriorAreaComponent, CTriggerAreaComponent, 0 );

public:
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	virtual void EnteredArea( CComponent* component );
	virtual void ExitedArea( CComponent* component );

	void GetStorageBounds( Box& box ) const override { box = GetBoundingBox(); }

	RED_INLINE const CName& GetEntranceTag() const { return m_entranceTag; }
	RED_INLINE const String& GetTexture() const { return m_texture; }

	TDynArray< THandle< CActor > >& GetActorsInside(){ return m_actorsInside; }

#ifndef NO_EDITOR
	virtual void OnNavigationCook( CWorld* world, CNavigationCookingContext* context ) override;
#endif

protected:
	CName							m_entranceTag;
	String							m_texture;
	TDynArray< THandle< CActor > >	m_actorsInside;

private:
	CWeatherManager*				m_weatherManager;
};

BEGIN_CLASS_RTTI( CR4InteriorAreaComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_entranceTag, TXT("Entrance tag") );
	PROPERTY_EDIT( m_texture, TXT("Interior texture") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////

class CR4InteriorAreaEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CR4InteriorAreaEntity, CEntity, 0 );

public:
	RED_INLINE const CName& GetEntranceTag() const { return m_entranceTag; }
	RED_INLINE const String& GetTexture() const { return m_texture; }
protected:
	CName		m_entranceTag;
	String		m_texture;
};

BEGIN_CLASS_RTTI( CR4InteriorAreaEntity );
	PARENT_CLASS( CEntity );
	PROPERTY_EDIT( m_entranceTag, TXT("Entrance tag") );
	PROPERTY_EDIT( m_texture, TXT("Interior texture") );
END_CLASS_RTTI();

struct CInteriorsRegistryMemberData
{
	CR4InteriorAreaComponent*		m_interior;	

	RED_INLINE CR4InteriorAreaComponent* Get() const								{ return m_interior; }
	RED_INLINE Bool operator==( const CInteriorsRegistryMemberData& other )		{ return m_interior == other.m_interior; }
	RED_INLINE operator CR4InteriorAreaComponent* () const						{ return m_interior; }
};

class CInteriorsRegistry : public CQuadTreeStorage< CR4InteriorAreaComponent, CInteriorsRegistryMemberData >
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public:
	void Add( CR4InteriorAreaComponent* interior );
	void Remove( CR4InteriorAreaComponent* interior );

	Int32 FindInteriorContainingPoint( const Vector& point, CR4InteriorAreaComponent** foundInteriors, Int32 maxElements );
};