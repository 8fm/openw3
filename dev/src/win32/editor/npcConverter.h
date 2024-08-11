
#pragma once

class CEntityConverter
{
	DECLARE_RTTI_SIMPLE_CLASS( CEntityConverter )

public:
	virtual Bool CanConvert( const CClass* entityObjClass ) const							{ return false; }
	virtual CEntityTemplate* Convert( const CEntityTemplate* temp, const CEntity* entity )	{ return NULL; }
	virtual CClass* GetDestClass() const													{ return NULL; }
};

BEGIN_CLASS_RTTI( CEntityConverter )
END_CLASS_RTTI()

class CNewNpcToBgNpcConverter : public CEntityConverter
{
	DECLARE_RTTI_SIMPLE_CLASS( CNewNpcToBgNpcConverter )

public:
	virtual Bool CanConvert( const CClass* entityObjClass ) const;
	virtual CEntityTemplate* Convert( const CEntityTemplate* temp, const CEntity* entity );
	virtual CClass* GetDestClass() const;

private:
	CBgRootComponent* CreateRootComponent( CEntity* npc, const CAnimatedComponent* oldRoot );
	Bool ShouldAddItem( const CInventoryComponent* inv, const SInventoryItem& item ) const;
	void AddMeshComponent( CEntity* newNpc, CComponent* newNpcRoot, CMeshComponent* orginal, const CNewNPC* oldNpc ) const;
	void AddMesh( CEntity* newNpc, CComponent* newNpcRoot, CMesh* orginal, const String& name, Int32 i ) const;
	void AddSlotToTempl( const CName& slotName, CEntityTemplate* newTempl, const CEntityTemplate* oldTempl ) const;
	CBgNpcTriggerComponent* CreateTrigger( CEntity* newNpc ) const;
};

BEGIN_CLASS_RTTI( CNewNpcToBgNpcConverter )
	PARENT_CLASS( CEntityConverter )
END_CLASS_RTTI()
