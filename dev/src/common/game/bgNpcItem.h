
#pragma once

#include "bgNpcMesh.h"

enum EItemState
{
	IS_MOUNT,
	IS_HOLD,
	IS_HIDDEN,
	IS_INVALID,
};

BEGIN_ENUM_RTTI( EItemState );
	ENUM_OPTION( IS_MOUNT );
	ENUM_OPTION( IS_HOLD );
	ENUM_OPTION( IS_HIDDEN );
END_ENUM_RTTI();

class CBgNpcItemComponent : public CBgMeshComponent
{
	DECLARE_ENGINE_CLASS( CBgNpcItemComponent, CBgMeshComponent, 0 );

protected:
	CName			m_itemName;
	CName			m_itemCategory;
	EItemState		m_defaultState;

	CName			m_equipSlot;
	CName			m_holdSlot;

protected:
	EItemState		m_state;

public:
	CBgNpcItemComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	void SetItemState( EItemState state );

	const CName& GetItemCategory() const;
	const CName& GetItemName() const;

private:
	void EquipItem();
	void HoldItem();
	void HideItem();

	void CheckAttachment( const CName& slotName );

	void AttachComponentToSlot( const CName& slotName, CComponent* component );

#ifndef NO_EDITOR
public:
	struct SItemInfo
	{
		String					m_path;
		CName					m_equipSlot;
		CName					m_holdSlot;
		CName					m_category;
	};

	virtual void OnPropertyPostChange( IProperty* property );

	void SetItemName( const CName& itemName );

	void SetItemDefaultState( EItemState state );

private:
	CMeshComponent* GetMeshFromItem( const String& templatePath ) const;

#endif
};

BEGIN_CLASS_RTTI( CBgNpcItemComponent );
	PARENT_CLASS( CBgMeshComponent );
	PROPERTY_EDIT( m_itemName, TXT("Item name") );
	PROPERTY_EDIT( m_itemCategory, TXT("Item category") );
	PROPERTY_EDIT( m_defaultState, TXT("Item default state") );
	PROPERTY_EDIT( m_equipSlot, TXT("") );
	PROPERTY_EDIT( m_holdSlot, TXT("") );
END_CLASS_RTTI();
