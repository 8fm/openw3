/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdBoneSelectionData
{
public:
	Int32		m_num;
	Float	m_weight;
	Bool	m_select;
	String	m_name;
	Int32		m_row;
	CEdBoneSelectionData() 
		: m_num(0), m_weight(0.0f), m_select(false), m_row(-1) {}
};


class CEdBoneSelectionDialog: public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	//dex++: converted to use generalized CSkeleton
	CEdBoneSelectionDialog(wxWindow* parent, const CSkeleton* skeleton, Bool multiselect = true, Bool withWeights = false);
	//dex--
	~CEdBoneSelectionDialog();

	void DoModal();

	Bool GetSelectedBones(TDynArray<Int32>& bonesIndexArray);
	Bool GetSelectedBones(TDynArray<String>& bonesNameArray);
	Bool GetBonesWeight(TDynArray<Float>& bonesWeight);

	Bool SelectBones(const String& bone, Bool propagate = true, Bool update = true);
	Bool SelectBoneWeight(const String& bone, Float weight);
	Bool DeselectBones(const String& bone, Bool propagate = true, Bool update = true);

protected:
	void FillTree(Int32 parentIndex, wxTreeItemId& parent);
	void SetSelect(wxTreeItemId* item, Bool select, Bool update = false);
	void SetSelectChilds(wxTreeItemId* parent, Bool select, Bool update = true);
	
	wxTreeItemId GetItemByName(const String& itemName);
	Int32 FindBoneByName(const String& boneName);
	Int32 FindBoneByRow(Int32 rowNum);

	void FillList();
	void UpdateList();

	void OnOk( wxCommandEvent& event );
	void OnSelectBoneItem( wxCommandEvent& event );
	void OnDeselectBoneItem( wxCommandEvent& event );
	void OnSelectBoneItemChilds( wxCommandEvent& event );
	void OnDeselectBoneItemChilds( wxCommandEvent& event );
	void OnExpBoneItem( wxCommandEvent& event );
	void OnCollBoneItem( wxCommandEvent& event );
	void OnSelectAllBoneItems( wxCommandEvent& event );
	void OnDeselectAllBoneItems( wxCommandEvent& event );
	void OnGridRightDown( wxMouseEvent& event );
	void OnSetAllWeight( wxCommandEvent& event );
	void OnResetAllWeight( wxCommandEvent& event );

	void OnRightClick( wxTreeEvent& event );
	void OnSelectionChanged(  wxTreeEvent& event );
	void OnItemActivated(  wxTreeEvent& event );
	void OnItemExpColl(  wxTreeEvent& event );
	void OnItemCollapsing( wxTreeEvent& event );
	void OnItemExpanding( wxTreeEvent& event );

	void OnSliderUpdate( wxCommandEvent& event );
	void OnEditUpdate( wxCommandEvent& event );
	void OnDeselectFromButton( wxCommandEvent& event );
	void OnEditBoneName( wxCommandEvent& event );

protected:
	struct SBoneEditRow
	{
		wxTextCtrl*		m_boneLabels;
		wxBitmapButton*	m_delButtons;
		wxTextCtrl*		m_weightEdits;
		wxSlider*		m_weightSliders;

		SBoneEditRow() : m_boneLabels(NULL), m_delButtons(NULL), m_weightEdits(NULL), m_weightSliders(NULL) {}

		void SetVisible(Bool visible) 
		{
			if (m_boneLabels) m_boneLabels->Show(visible);
			if (m_delButtons) m_delButtons->Show(visible);
			if (m_weightEdits) m_weightEdits->Show(visible);
			if (m_weightSliders) m_weightSliders->Show(visible);
		}
	};

protected:
	TDynArray<CEdBoneSelectionData> m_bones;

	wxTreeCtrl*			m_tree;
	wxTreeItemId		m_selectItem;

	enum ECheckAction { CHA_NONE, CHA_EXP, CHA_COLL } m_actionToCheck;

	TDynArray<SBoneEditRow>		m_rows;
	wxScrolledWindow*			m_grid;
	Bool						m_isWeight;

	wxTextCtrl*			m_selectedBoneText;
	//dex++
	const CSkeleton*	m_skeleton;
	//dex--
	Bool				m_multiselect;
};