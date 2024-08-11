/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/havokDataBufferParser.h"

class CEdHavokPropertiesPage : public wxScrolledWindow
{
	DECLARE_EVENT_TABLE();

	struct TreeItem : public wxTreeItemData
	{
		Int32 m_num;
		TreeItem( Int32 num ) : m_num( num ) {}
	};

private:
#ifdef USE_HAVOK_ANIMATION
	TDynArray< const HavokDataBuffer* >		m_objects;
	CHavokDataBufferParser					m_parser;
	hkArray< hkVariant >					m_items;
#endif
	wxTreeCtrl*								m_tree;
	wxTextCtrl*								m_text;

public:
	CEdHavokPropertiesPage( wxWindow* parent );

	void RefreshValues();

	void SetNoObject();
#ifdef USE_HAVOK_ANIMATION
	void SetObject( const HavokDataBuffer* object );
	void SetObject( TDynArray< const HavokDataBuffer* >& objects );

private:
	void FillTree( const hkVariant& hkRoot, wxTreeItemId& treeRoot );
	void DisplayItemData( const hkVariant& item );
#endif
protected:
	void OnItemSelected( wxTreeEvent& event );
};
