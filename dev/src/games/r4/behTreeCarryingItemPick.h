#pragma once

#include "../../common/game/behTreeInstance.h"

#include "behTreeCarryingItemBaseNode.h"
#include "behTreeCarryingItemData.h"

class CBehTreePickCarryableItemInstance;
class CBehTreeNodePickItemDefinition : public CBehTreeNodeCarryingItemBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePickItemDefinition, CBehTreeNodeCarryingItemBaseDefinition, CBehTreePickCarryableItemInstance, PickCarryableItem );
protected:	
	CBehTreeValCName m_rBoneToAttachItem;
	CBehTreeValCName m_lBoneToAttachItem;

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
};

BEGIN_CLASS_RTTI( CBehTreeNodePickItemDefinition );
	PARENT_CLASS( CBehTreeNodeCarryingItemBaseDefinition );	
	PROPERTY_EDIT( m_rBoneToAttachItem, TXT("") );
	PROPERTY_EDIT( m_lBoneToAttachItem, TXT("") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreePickCarryableItemInstance : public CBehTreeNodeCarryingItemBaseInstance
{
	typedef CBehTreeNodeCarryingItemBaseInstance Super;
protected:	
	CName	m_rBoneToAttachItem;
	CName	m_lBoneToAttachItem;	
	Float	m_overrideTurnOnTime;
	Bool	m_slideDone;
	Bool	m_itemAttached;
public:
	typedef CBehTreeNodePickItemDefinition Definition;

	CBehTreePickCarryableItemInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeNodeCarryingItemBaseInstance( def, owner, context, parent ) 	
		, m_rBoneToAttachItem( def.m_rBoneToAttachItem.GetVal( context ) )
		, m_lBoneToAttachItem( def.m_lBoneToAttachItem.GetVal( context ) )
		, m_overrideTurnOnTime( -1 )
	{}

	Bool IsAvailable() override;
	void Update() override;	
	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool Activate() override;
	void Deactivate() override;
private:
	Vector3 GetAttachmentPosWS( String& itemType );
};