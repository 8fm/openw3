#pragma once
#include "selectionEditor.h"

class CTraitData;

// Trait list selection
class CTraitDataListSelection : public ISelectionEditor
{
protected:
	CTraitData* m_traitData;

public:
	CTraitDataListSelection( CPropertyItem* item );
};

// Trait list selection
class CTraitListSelection : public CTraitDataListSelection
{
public:
	CTraitListSelection( CPropertyItem* item );

protected:
	virtual void FillChoices();
};

// Trait list selection
class CSkillListSelection : public CTraitDataListSelection
{
public:
	CSkillListSelection( CPropertyItem* item );

protected:
	virtual void FillChoices();
};
