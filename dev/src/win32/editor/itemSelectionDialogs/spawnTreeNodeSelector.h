#pragma  once
#include "mappedClassSelectorDialog.h"

class CEdSpawnTreeNodeSelectorDialog : public CEdMappedClassSelectorDialog
{
public:
	CEdSpawnTreeNodeSelectorDialog( wxWindow* parent, CClass* rootClass, IEdSpawnTreeNode *const activeSpawnTreeNode, ESpawnTreeType spawnTreeType, IEdSpawnTreeNode* childSpawnTreeNode = nullptr );

protected:
	Bool IsSelectable( CClass* classId ) const override;

private:
	IEdSpawnTreeNode* const m_activeSpawnTreeNode;
	IEdSpawnTreeNode* const m_childSpawnTreeNode;
	ESpawnTreeType			m_spawnTreeType;
};