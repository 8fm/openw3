#pragma once

#include "../../common/game/behTreeInstance.h"

#include "carryableItemsStorePoint.h"

class CBehTreeCarryingItemData
{
	DECLARE_RTTI_STRUCT( CBehTreeCarryingItemData );
protected:	
	THandle< CCarryableItemStorePointComponent >	m_currentStorePoint;
	THandle< CCarryableItemStorePointComponent >	m_prevStorePoint;
	THandle< CEntity >								m_carriedItem;
	String											m_carriedItemType;
public:
	CBehTreeCarryingItemData(){}
	
	RED_INLINE void SetCurrentStorePoint( CCarryableItemStorePointComponent* storePoint )	{ m_prevStorePoint = m_currentStorePoint;  m_currentStorePoint = storePoint; }	
	RED_INLINE CCarryableItemStorePointComponent* GetCurrentStorePoint()	{ return m_currentStorePoint.Get(); }
	RED_INLINE CCarryableItemStorePointComponent* GetPrevStorePoint()		{ return m_prevStorePoint.Get(); }
	RED_INLINE void ShiftItemStorePoint(){ m_prevStorePoint = m_currentStorePoint; }
	
	RED_INLINE void SetCarriedItem( CEntity* item, String itemType = String::EMPTY ){ m_carriedItem = item; m_carriedItemType = itemType; }
	RED_INLINE CEntity* GetCarriedItem(){ return m_carriedItem.Get(); }

	Bool IfCanBeChoosenAsDestiny( CCarryableItemStorePointComponent* storePoint );
	Bool IfCanBeChoosenAsSource( CCarryableItemStorePointComponent* storePoint );

	static CName			GetStorageName();

	class CInitializer : public CAIStorageItem::CInitializer
	{
	protected:
		CBehTreeInstance*			m_owner;
	public:
		CInitializer( CBehTreeInstance* owner )
			: m_owner( owner )												{}
		CName GetItemName() const override;
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeCarryingItemData );	
	PROPERTY( m_carriedItem );
END_CLASS_RTTI();

class CBehTreeCarryingItemDataPtr : public TAIStoragePtr< CBehTreeCarryingItemData >
{
	typedef TAIStoragePtr< CBehTreeCarryingItemData > Super;
public:
	CBehTreeCarryingItemDataPtr( CBehTreeInstance* owner )
		: Super( CBehTreeCarryingItemData::CInitializer( owner ), owner )			{}

	CBehTreeCarryingItemDataPtr()
		: Super()															{}
	CBehTreeCarryingItemDataPtr( const CBehTreeCarryingItemDataPtr& p )
		: Super( p )														{}

	CBehTreeCarryingItemDataPtr( CBehTreeCarryingItemDataPtr&& p )
		: Super( Move( p ) )												{}

};