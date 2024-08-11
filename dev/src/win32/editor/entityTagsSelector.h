#pragma once

#include "selectionEditor.h"

class CEntityTagsSelector : public ISelectionEditor
{
protected:
	CClass*					m_entityClass;


	void FillChoices() override;
	Bool IsTextEditable() const override;

public:
	CEntityTagsSelector( CClass* entityClass, CPropertyItem* item )
		: ISelectionEditor( item )
		, m_entityClass( entityClass )											{ ASSERT( entityClass->IsA( CEntity::GetStaticClass() ) );}
};