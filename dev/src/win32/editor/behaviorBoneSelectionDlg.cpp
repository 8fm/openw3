/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "callbackData.h"
#include "../../common/engine/skeleton.h"

#define ID_BONE_ITEM_SEL			4001
#define ID_BONE_ITEM_DESEL			4002
#define ID_BONE_ITEM_EXP			4003
#define ID_BONE_ITEM_COLL			4004
#define ID_BONE_ITEM_SEL_CHILDS		4005
#define ID_BONE_ITEM_DESEL_CHILDS	4006
#define ID_BONE_ITEM_SEL_ALL		4007
#define ID_BONE_ITEM_DESEL_ALL		4008
#define ID_GRID_WEIGHT_ALL_MAX		4009
#define ID_GRID_WEIGHT_ALL_MIN		4010

BEGIN_EVENT_TABLE( CEdBoneSelectionDialog, wxDialog )
	EVT_BUTTON( XRCID("buttOk"), CEdBoneSelectionDialog::OnOk )
	EVT_BUTTON( XRCID("setAllWeightButt"), CEdBoneSelectionDialog::OnSetAllWeight )
	EVT_BUTTON( XRCID("resetAllWeightButt"), CEdBoneSelectionDialog::OnResetAllWeight )
	EVT_TREE_ITEM_RIGHT_CLICK( XRCID( "boneTree"), CEdBoneSelectionDialog::OnRightClick )
	EVT_TREE_SEL_CHANGED( XRCID( "boneTree"), CEdBoneSelectionDialog::OnSelectionChanged )
	EVT_TREE_ITEM_ACTIVATED( XRCID( "boneTree"), CEdBoneSelectionDialog::OnItemActivated )
	EVT_TREE_ITEM_MIDDLE_CLICK( XRCID( "boneTree"), CEdBoneSelectionDialog::OnItemExpColl )
	EVT_TREE_ITEM_COLLAPSING( XRCID( "boneTree"), CEdBoneSelectionDialog::OnItemCollapsing )
	EVT_TREE_ITEM_EXPANDING( XRCID( "boneTree"), CEdBoneSelectionDialog::OnItemExpanding ) 
END_EVENT_TABLE()

//dex++: converted to use generalized CSkeleton interface
CEdBoneSelectionDialog::CEdBoneSelectionDialog(wxWindow* parent, const CSkeleton* skeleton, Bool multiselect /* = true */, Bool withWeights /* = false */)
//dex--
	: m_skeleton(skeleton)
	, m_multiselect(multiselect)
	, m_isWeight(withWeights)
	, m_grid(NULL)
	, m_tree(NULL)
	, m_selectedBoneText(NULL)
	, m_actionToCheck( CHA_NONE )
{
	ASSERT(m_skeleton);

	// Load dialog
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("BoneSelectionDialog") );

	// Load bone tree
	m_tree = XRCCTRL( *this, "boneTree", wxTreeCtrl );

	if (!m_multiselect && !m_isWeight)
	{
		XRCCTRL( *this, "boneGridPanel", wxPanel )->Hide();
	}
	else if (m_isWeight || m_multiselect)
	{
		// Load bone list
		m_grid = XRCCTRL( *this, "boneGrid", wxScrolledWindow );

		if (m_isWeight)
		{
			XRCCTRL( *this, "setAllWeightButt", wxButton )->Show();
			XRCCTRL( *this, "resetAllWeightButt", wxButton )->Show();
		}
	}

	XRCCTRL( *this, "selectedBonePanel", wxPanel )->Show();
	m_selectedBoneText = XRCCTRL( *this, "editSelectedBone", wxTextCtrl );
	m_selectedBoneText->SetValidator( wxTextValidator(wxFILTER_ASCII) );
	m_selectedBoneText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdBoneSelectionDialog::OnEditBoneName ), NULL, this );

	// Assign image list to tree
	wxImageList* images = new wxImageList( 14, 14, true, 1 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECK_OFF") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECK_ON") ) );
	m_tree->AssignImageList( images );

	// Fill bone array
	//dex++: using generalized CSkeleton functions
	const Uint32 numBones = m_skeleton->GetBonesNum();
	for (Uint32 i=0; i<numBones; i++)
	{
		CEdBoneSelectionData boneData;
		boneData.m_num = i;
		boneData.m_name = m_skeleton->GetBoneName(i);
		m_bones.PushBack(boneData);
	}
	//dex--

	// Fill tree
	if (m_bones.Size()>0)
	{
		wxString itemName = m_bones[0].m_name.AsChar();
		wxTreeItemId root =  m_tree->AddRoot( itemName, 0 );

		FillTree(0, root);
	}

	m_tree->Expand(m_tree->GetRootItem());

	FillList();
}

CEdBoneSelectionDialog::~CEdBoneSelectionDialog()
{
}

void CEdBoneSelectionDialog::DoModal()
{
	UpdateList();
	Layout();

	ShowModal();
}

void CEdBoneSelectionDialog::OnOk(wxCommandEvent& event )
{
	TDynArray<Int32> temp;
	GetSelectedBones(temp);

	if ((m_multiselect && temp.Size()>0) || (!m_multiselect && temp.Size()==1))
	{
		EndDialog(1);
	}
	else if (temp.Size()>1)
	{
		wxString msg = wxT("You must select one bone.");
		wxMessageBox(msg, wxT("Warinng"),wxOK | wxCENTRE | wxICON_WARNING);
	}
	else
	{
		wxString msg = wxT("You must select bone.");
		wxMessageBox(msg, wxT("Warinng"),wxOK | wxCENTRE | wxICON_WARNING);
	}
}

Bool CEdBoneSelectionDialog::GetSelectedBones(TDynArray<Int32>& bonesIndexArray)
{
	Bool ret = false;

	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_select)
		{
			bonesIndexArray.PushBack(m_bones[i].m_num);
			ret = true;
		}
	}

	return ret;
}

Bool CEdBoneSelectionDialog::GetSelectedBones(TDynArray<String>& bonesNameArray)
{
	Bool ret = false;

	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_select)
		{
			bonesNameArray.PushBack(m_bones[i].m_name);
			ret = true;
		}
	}

	return ret;
}

Bool CEdBoneSelectionDialog::GetBonesWeight(TDynArray<Float>& bonesWeight)
{
	Bool ret = false;

	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_select)
		{
			bonesWeight.PushBack(m_bones[i].m_weight);
			ret = true;
		}
	}

	return ret;
}

wxTreeItemId CEdBoneSelectionDialog::GetItemByName(const String& itemName)
{
	TQueue< wxTreeItemId >	items;
	items.Push( m_tree->GetRootItem() );

	while( !items.Empty() )
	{
		wxTreeItemId &currItem = items.Front();
		items.Pop();

		if ( m_tree->GetItemText(currItem) == itemName.AsChar() )
		{
			return currItem;
		}

		// recurse into children
		wxTreeItemIdValue cookie = 0;
		wxTreeItemId child = m_tree->GetFirstChild( currItem, cookie );
		while( child.IsOk() )
		{
			items.Push( child );
			child = m_tree->GetNextChild( currItem, cookie );
		}
	}

	return wxTreeItemId();
}

void CEdBoneSelectionDialog::FillTree(Int32 parentIndex, wxTreeItemId& parent)
{
	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		//dex++: switched to generalized interface
		const Int32 boneParentIndex = m_skeleton->GetParentBoneIndex(i);
		//dex--
		if (boneParentIndex == parentIndex)
		{
			wxString itemName = m_bones[i].m_name.AsChar();
			wxTreeItemId item =  m_tree->AppendItem(parent, itemName, 0);
			if (!m_tree->ItemHasChildren(parent)) m_tree->SetItemHasChildren(parent, true);

			FillTree(i, item);
		}
	}
}

Bool CEdBoneSelectionDialog::SelectBones(const String& bone, Bool propagate /* = true */, Bool update /* = true */)
{
	Bool ret = false;
	if (!m_tree) return ret;

	wxTreeItemId item = GetItemByName(bone);
	if (item.IsOk())
	{
		if (propagate)
		{
			SetSelectChilds(&item, true, update);
			ret = true;
		}
		else
		{
			SetSelect(&item, true, update);
			ret = true;
		}

		m_tree->SelectItem(item, true);
	}

	return ret;
}

Bool CEdBoneSelectionDialog::DeselectBones(const String& bone, Bool propagate /* = true */, Bool update /* = true */)
{
	Bool ret = false;
	if (!m_tree) return ret;

	wxTreeItemId item = GetItemByName(bone);
	if (item.IsOk())
	{
		if (propagate)
		{
			SetSelectChilds(&item, false, update);
			ret = true;
		}
		else
		{
			SetSelect(&item, false, update);
			ret = true;
		}

		UpdateList();
	}

	return ret;
}

Bool CEdBoneSelectionDialog::SelectBoneWeight(const String& bone, Float weight)
{
	Bool ret = false;
	if (!m_tree) return ret;

	Int32 index = FindBoneByName(bone);
	m_bones[index].m_weight = weight;
	
	return ret;
}

void CEdBoneSelectionDialog::OnSelectBoneItem( wxCommandEvent& event )
{
	if (m_selectItem.IsOk()) 
	{
		// Deselect all if not multiselect
		if (!m_multiselect) SetSelectChilds(&(m_tree->GetRootItem()), false);

		// Select
		SetSelect(&m_selectItem, true, true);
	}
}

void CEdBoneSelectionDialog::OnDeselectBoneItem( wxCommandEvent& event )
{
	if (m_selectItem.IsOk()) 
	{
		SetSelect(&m_selectItem, false, true);
	}
}

void CEdBoneSelectionDialog::OnRightClick( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();	

	m_tree->SelectItem( item, true);
	m_selectItem = item;

	wxMenu menu;
	if (m_tree->GetItemImage(item) == 0)
	{
		menu.Append( ID_BONE_ITEM_SEL, TEXT("Select"), wxEmptyString );
		menu.Connect( ID_BONE_ITEM_SEL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnSelectBoneItem ), NULL, this );
	}
	else if (m_tree->GetItemImage(item) == 1)
	{
		menu.Append( ID_BONE_ITEM_DESEL, TEXT("Deselect"), wxEmptyString );
		menu.Connect( ID_BONE_ITEM_DESEL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnDeselectBoneItem ), NULL, this );
	}

	if (m_tree->ItemHasChildren(item))
	{
		menu.AppendSeparator();

		if(m_multiselect)
		{
			menu.Append( ID_BONE_ITEM_SEL_CHILDS, TEXT("Select childs"), wxEmptyString );
			menu.Connect( ID_BONE_ITEM_SEL_CHILDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnSelectBoneItemChilds ), NULL, this );
			menu.Append( ID_BONE_ITEM_DESEL_CHILDS, TEXT("Deselect childs"), wxEmptyString );
			menu.Connect( ID_BONE_ITEM_DESEL_CHILDS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnDeselectBoneItemChilds ), NULL, this );

			menu.AppendSeparator();

			menu.Append( ID_BONE_ITEM_SEL_ALL, TEXT("Select all"), wxEmptyString );
			menu.Connect( ID_BONE_ITEM_SEL_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnSelectAllBoneItems ), NULL, this );
		}

		menu.Append( ID_BONE_ITEM_DESEL_ALL, TEXT("Deselect all"), wxEmptyString );
		menu.Connect( ID_BONE_ITEM_DESEL_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnDeselectAllBoneItems ), NULL, this );

		menu.AppendSeparator();

		menu.Append( ID_BONE_ITEM_EXP, TEXT("Expand"), wxEmptyString );
		menu.Connect( ID_BONE_ITEM_EXP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnExpBoneItem ), NULL, this );
		menu.Append( ID_BONE_ITEM_COLL, TEXT("Collapse"), wxEmptyString );
		menu.Connect( ID_BONE_ITEM_COLL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnCollBoneItem ), NULL, this );
	}
	PopupMenu(&menu);
}

void CEdBoneSelectionDialog::OnGridRightDown(wxMouseEvent& event)
{
	//wxMenu menu;
	//menu.Append( ID_GRID_WEIGHT_ALL_MAX, TEXT("Set all weights to 1"), wxEmptyString );
	//menu.Connect( ID_GRID_WEIGHT_ALL_MAX, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnAllWeightToMax ), NULL, this );
	//menu.Append( ID_GRID_WEIGHT_ALL_MIN, TEXT("Set all weights to 0"), wxEmptyString );
	//menu.Connect( ID_GRID_WEIGHT_ALL_MIN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdBoneSelectionDialog::OnAllWeightToMin ), NULL, this );
	//PopupMenu(&menu);

	event.Skip();
}

void CEdBoneSelectionDialog::SetSelect(wxTreeItemId* item, Bool select, Bool update /* = false */)
{
	if (!item) return;

	String boneName = m_tree->GetItemText(*item);

	if (select)
	{
		m_tree->SetItemImage(*item, 1);

		Int32 index = FindBoneByName( boneName );
		m_bones[index].m_select = true;
		if (m_selectedBoneText) m_selectedBoneText->SetLabel(boneName.AsChar());
	}
	else
	{
		m_tree->SetItemImage(*item, 0);

		Int32 index = FindBoneByName( boneName );
		m_bones[index].m_select = false;
		if (m_selectedBoneText) m_selectedBoneText->SetLabel(wxEmptyString);
	}

	if (update) UpdateList();
}

void CEdBoneSelectionDialog::SetSelectChilds(wxTreeItemId* parent, Bool select, Bool update /* = true */)
{
	if (!parent) return;

	TQueue< wxTreeItemId >	items;
	items.Push( *parent );

	while( !items.Empty() )
	{
		wxTreeItemId &currItem = items.Front();
		items.Pop();

		(select) ? SetSelect(&currItem, true) : SetSelect(&currItem, false);

		// recurse into children
		wxTreeItemIdValue cookie = 0;
		wxTreeItemId child = m_tree->GetFirstChild( currItem, cookie );
		while( child.IsOk() )
		{
			items.Push( child );
			child = m_tree->GetNextChild( currItem, cookie );
		}
	}

	if (update) UpdateList();
}

void CEdBoneSelectionDialog::OnSelectBoneItemChilds( wxCommandEvent& event )
{
	SetSelectChilds(&m_selectItem, true);
}

void CEdBoneSelectionDialog::OnDeselectBoneItemChilds( wxCommandEvent& event )
{
	SetSelectChilds(&m_selectItem, false);
}

void CEdBoneSelectionDialog::OnSelectAllBoneItems( wxCommandEvent& event )
{
	SetSelectChilds(&(m_tree->GetRootItem()), true);
}

void CEdBoneSelectionDialog::OnDeselectAllBoneItems( wxCommandEvent& event )
{
	SetSelectChilds(&(m_tree->GetRootItem()), false);
}

void CEdBoneSelectionDialog::OnExpBoneItem( wxCommandEvent& event )
{
	if (m_selectItem.IsOk() && m_tree->ItemHasChildren(m_selectItem)) m_tree->Expand(m_selectItem);
}

void CEdBoneSelectionDialog::OnCollBoneItem( wxCommandEvent& event )
{
	if (m_selectItem.IsOk() && m_tree->ItemHasChildren(m_selectItem)) m_tree->Collapse(m_selectItem);
}

void CEdBoneSelectionDialog::OnSelectionChanged(  wxTreeEvent& event  )
{	
	if ( event.GetOldItem() )
	{
		m_tree->SetItemDropHighlight( event.GetOldItem(), false );
	}

	m_tree->SetItemDropHighlight( event.GetItem(), true );
}

void CEdBoneSelectionDialog::OnItemActivated(  wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();
	m_selectItem = item;
	String boneName = m_tree->GetItemText(m_selectItem);
	Int32 index = FindBoneByName( boneName );

	if (!m_multiselect) SetSelectChilds(&(m_tree->GetRootItem()), false, false);

	if (m_tree->IsExpanded(m_selectItem))
	{
		// Expand
		m_actionToCheck = CHA_COLL;
	}
	if (m_tree->ItemHasChildren(m_selectItem) && !m_tree->IsExpanded(m_selectItem))
	{
		// Collapse
		m_actionToCheck = CHA_EXP;
	}

	if (m_bones[index].m_select)
		SetSelect(&m_selectItem, false, true);
	else
		SetSelect(&m_selectItem, true, true);
}

void CEdBoneSelectionDialog::OnItemCollapsing( wxTreeEvent& event )
{
	if (m_actionToCheck == CHA_COLL )
	{
		event.Veto();
	}

	m_actionToCheck = CHA_NONE;
}

void CEdBoneSelectionDialog::OnItemExpanding( wxTreeEvent& event )
{
	if (m_actionToCheck == CHA_EXP )
	{
		event.Veto();
	}

	m_actionToCheck = CHA_NONE;
}

void CEdBoneSelectionDialog::OnItemExpColl(  wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();

	if (m_tree->ItemHasChildren(item) && m_tree->IsExpanded(item))
	{
		m_tree->Collapse(item);
	}
	else if (m_tree->ItemHasChildren(item))
	{
		m_tree->Expand(item);
	}
}

Int32 CEdBoneSelectionDialog::FindBoneByName(const String& boneName)
{
	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_name == boneName)
		{
			return i;
		}
	}

	return -1;
}

Int32 CEdBoneSelectionDialog::FindBoneByRow(Int32 rowNum)
{
	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_row == rowNum)
		{
			return i;
		}
	}

	ASSERT(0);
	return -1;
}

void CEdBoneSelectionDialog::UpdateList()
{
	if (!m_grid) return;

	// Fill
	//m_grid->Freeze();

	Int32 rowNum = 0;

	// Hide all
	for (Uint32 i=0; i<m_rows.Size(); i++)
	{
		m_rows[i].SetVisible(false);
	}

	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_select)
		{
			m_bones[i].m_row = rowNum;
			m_rows[rowNum].m_boneLabels->SetLabel(m_bones[i].m_name.AsChar());
			
			if (m_isWeight)
			{
				String weightValue = String::Printf(TXT("%.2f"), m_bones[i].m_weight);
				m_rows[rowNum].m_weightEdits->SetLabel(weightValue.AsChar());

				m_rows[rowNum].m_weightSliders->SetValue( (Int32)(m_bones[i].m_weight*100.0f) );
			}

			m_rows[rowNum].SetVisible(true);

			rowNum++;
		}
		else
		{
			m_bones[i].m_row = -1;
		}
	}

	//m_grid->Thaw();

	if (m_selectItem.IsOk())
	{
		Int32 i = FindBoneByName( m_tree->GetItemText(m_selectItem).wc_str() );
		Int32 rowNum = m_bones[i].m_row;
		if (rowNum > 0)
		{
			m_grid->SetScrollbar(wxVERTICAL, rowNum, 10, 100, true);		
		}
		else
		{
			m_grid->SetScrollbar(wxVERTICAL, 0, 10, 100, true);
		}
	}
	else
	{
		m_grid->SetScrollbar(wxVERTICAL, 0, 10, 100, true);
	}

	m_grid->FitInside();
	m_grid->Layout();
	Layout();
}

void CEdBoneSelectionDialog::FillList()
{
	if (!m_grid) return;

	//m_grid->Freeze();

	wxPanel *gridPanel = new wxPanel( m_grid );
	wxBoxSizer *scrollSizer = new wxBoxSizer( wxVERTICAL );

	scrollSizer->Add( gridPanel, 1, wxEXPAND | wxALL, 5 );

	wxBoxSizer* gridSizer = new wxBoxSizer(wxVERTICAL);

	// Fill
	const Uint32 MAX_BONES = 350;

	for (Uint32 i=0; i<MAX_BONES; i++)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

		wxTextCtrl *label = new wxTextCtrlEx( gridPanel, -1, wxT(""), wxDefaultPosition, wxSize( 120, -1 ), wxBORDER_NONE );
		label->SetEditable( false );
		sizer->Add(label);

		SBoneEditRow newRow;
		newRow.m_boneLabels = label;

		if (m_isWeight)
		{
			wxTextCtrl *weight = new wxTextCtrl( gridPanel, -1, wxT("0.0"), wxDefaultPosition, wxSize( 40, -1 ), wxTE_PROCESS_ENTER );
			weight->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
			weight->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdBoneSelectionDialog::OnEditUpdate ), new TCallbackData< Int32 >(i), this );
			newRow.m_weightEdits = weight;
			sizer->Add(weight);

			wxSlider* slider = new wxSlider( gridPanel, -1, 0, 0, 100, wxDefaultPosition, wxSize(120, -1) );
			slider->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdBoneSelectionDialog::OnSliderUpdate ), new TCallbackData< Int32 >(i), this );
			slider->Connect( wxEVT_RIGHT_DOWN, -1, wxMouseEventHandler(CEdBoneSelectionDialog::OnGridRightDown) , NULL, this);
			newRow.m_weightSliders = slider;
			sizer->Add(slider);

			wxBitmap bitmap = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
			wxBitmapButton* butt = new wxBitmapButton(gridPanel, -1 , bitmap, wxDefaultPosition, wxSize(20,20));
			butt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdBoneSelectionDialog::OnDeselectFromButton ), new TCallbackData< Int32 >(i), this );
			newRow.m_delButtons = butt;
			sizer->Add(butt);
		}

		//newRow.SetVisible(false);
		m_rows.PushBack(newRow);

		gridSizer->Add(sizer, 0, wxCENTER );
	}

	gridPanel->SetSizer( gridSizer );
	gridSizer->Layout();

	m_grid->SetSizer( scrollSizer );
	m_grid->EnableScrolling( true, true );
	m_grid->SetScrollRate( 1, 1 ); 
	scrollSizer->FitInside( m_grid );
	scrollSizer->Layout();
	Layout();

	//m_grid->Thaw();
	//m_grid->Layout();
	//Layout();
}

void CEdBoneSelectionDialog::OnSliderUpdate(wxCommandEvent& event)
{
	TCallbackData< Int32 >* callData = (TCallbackData< Int32 >*)event.m_callbackUserData;

	Int32 selectRow = callData->GetData();
	Int32 boneIndex = FindBoneByRow(selectRow);

	// bone weight
	m_bones[boneIndex].m_weight = ((Float)(m_rows[selectRow].m_weightSliders->GetValue()) / 100.0f);

	// edit weight
	String text = String::Printf(TXT("%.2f"), m_bones[boneIndex].m_weight);
	m_rows[selectRow].m_weightEdits->SetLabel(text.AsChar());
}

void CEdBoneSelectionDialog::OnEditUpdate(wxCommandEvent& event)
{
	TCallbackData< Int32 >* callData = (TCallbackData< Int32 >*)event.m_callbackUserData;

	Int32 selectRow = callData->GetData();
	Int32 boneIndex = FindBoneByRow(selectRow);

	// bone weight
	String strVal = m_rows[selectRow].m_weightEdits->GetValue().wc_str();
	Float weight;
	FromString(strVal, weight);

	// Clamp with edit text change
	if (weight > 1.0f)
	{
		weight = 1.0f;
		String text = String::Printf(TXT("%.2f"), m_bones[boneIndex].m_weight);
		m_rows[selectRow].m_weightEdits->SetLabel(text.AsChar());
	}
	else if (weight < 0.0f)
	{
		weight = 0.0f;
		String text = String::Printf(TXT("%.2f"), m_bones[boneIndex].m_weight);
		m_rows[selectRow].m_weightEdits->SetLabel(text.AsChar());
	}

	m_bones[boneIndex].m_weight = weight;

	// slider weight
	m_rows[selectRow].m_weightSliders->SetValue( (Int32)(m_bones[boneIndex].m_weight * 100.0f) );
}

void CEdBoneSelectionDialog::OnDeselectFromButton(wxCommandEvent& event)
{
	TCallbackData< Int32 >* callData = (TCallbackData< Int32 >*)event.m_callbackUserData;

	Int32 selectRow = callData->GetData();
	Int32 boneIndex = FindBoneByRow(selectRow);

	SetSelect(&(GetItemByName(m_bones[boneIndex].m_name)), false);
	UpdateList();
}

void CEdBoneSelectionDialog::OnSetAllWeight(wxCommandEvent& event)
{
	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_select)
		{
			m_bones[i].m_weight = 1.0f;
		}
	}

	UpdateList();
}

void CEdBoneSelectionDialog::OnResetAllWeight(wxCommandEvent& event)
{
	for (Uint32 i=0; i<m_bones.Size(); i++)
	{
		if (m_bones[i].m_select)
		{
			m_bones[i].m_weight = 0.0f;
		}
	}

	UpdateList();
}

void CEdBoneSelectionDialog::OnEditBoneName(wxCommandEvent& event)
{
	String boneName = m_selectedBoneText->GetValue().wc_str();

	Int32 boneIndex = FindBoneByName(boneName);

	if (boneIndex!=-1)
	{
		if (!m_multiselect) OnDeselectAllBoneItems(event);

		wxTreeItemId item = GetItemByName(boneName);
		SetSelect(&item, true, true);
		m_tree->SelectItem(item);

		if (m_multiselect) m_selectedBoneText->SetLabel(wxT(""));
	}
	else
	{
		m_selectedBoneText->SetLabel(wxT(""));
		OnDeselectAllBoneItems(event);
	}
}
