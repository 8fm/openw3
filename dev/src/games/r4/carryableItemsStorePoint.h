#pragma once

#include "../../common/game/wayPointComponent.h"

class CCarryableItemStorePointComponent : public CWayPointComponent, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CCarryableItemStorePointComponent, CWayPointComponent, 0 )

	static const Int8 MAX_ITEMS_AMOUNT = 1;  	

protected:
	String	m_storedItemType;
	Bool	m_randomPrespawn;
	Float	m_itemSpawnDistance;

	Int8							m_amountOfStoredItems;
	TDynArray< THandle< CEntity > > m_storedItems;
	THandle< CNewNPC >				m_reservingNPC;

protected:
	static IRenderResource*		s_markerValid;
	static IRenderResource*		s_markerInvalid;
	static IRenderResource*		s_markerNoMesh; 
	static IRenderResource*		s_markerSelection;

	void InitializePointMarkers();

	virtual IRenderResource* GetMarkerValid() override;
	virtual IRenderResource* GetMarkerInvalid() override;
	virtual IRenderResource* GetMarkerNoMesh() override;
	virtual IRenderResource* GetMarkerSelection() override;

public:
	RED_INLINE TDynArray< THandle< CEntity > >& GetStoreItems(){ return m_storedItems; }
	RED_INLINE Bool IsEmpty(){ return m_storedItems.Size() == 0; }
	RED_INLINE Bool IfHasFreeSpace(){ return m_storedItems.Size() < MAX_ITEMS_AMOUNT; }
	RED_INLINE String& GetStoredItemType(){ return m_storedItemType; }
	RED_INLINE void ReserveStorePoint( CNewNPC* npc ){ m_reservingNPC = npc; }
	RED_INLINE Bool IsReservedFor( CNewNPC* npc ){ return npc == m_reservingNPC.Get(); }
	RED_INLINE Bool IsReserved(){ return m_reservingNPC.Get() != nullptr; }	
	RED_INLINE Bool FreeReservation(){ return m_reservingNPC = nullptr; }	

	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
	void OnAttached( CWorld* world ) override;	
	void PutItem( CEntity* item );
	Vector3 CalculateNextItemPosition();
	float CalculateHeadingForPickItem();
	float CalculateHeadingForDropItem();
};

BEGIN_CLASS_RTTI( CCarryableItemStorePointComponent )
	PARENT_CLASS( CWayPointComponent )	
	PROPERTY_CUSTOM_EDIT( m_storedItemType , TXT("Type of item, that can be storen in this point"), TXT("2daValueSelection")  );
	PROPERTY_EDIT( m_randomPrespawn, TXT("If random quantity of items should be spawned") )	
END_CLASS_RTTI()