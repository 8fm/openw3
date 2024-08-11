#pragma once



class CUpgradesSpawnerItemEntity : public CItemEntity
{
	DECLARE_ENGINE_CLASS( CUpgradesSpawnerItemEntity, CItemEntity, 0 );
private:
	THandle< CEntity >	m_owner;
	CName				m_slotName;
public:
	CUpgradesSpawnerItemEntity();

	Bool CustomAttach() override;
	void CustomDetach() override;
};

BEGIN_CLASS_RTTI( CUpgradesSpawnerItemEntity );
	PARENT_CLASS( CItemEntity );
END_CLASS_RTTI();