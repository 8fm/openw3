
#pragma once

#include "aaEditorIncludes.h"
#include "wx\generic\treectlg.h"

class CEdControlRigPanel;

class CEdControlRigBoneTransformPanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

	class SItemBoneData : public wxTreeItemData
	{
	public:
		SItemBoneData( Uint32 b ) : bone( b )
		{}

		Uint32 bone;
	};

private:
	CEdControlRigPanel* m_parentWin;
	const CEntity*		m_entity;
	String				m_selectedBone;

	Int32				m_sliderRange;
	Float				m_sliderReference[3];
	TList< wxString >	m_boneNameHistory;	

private:
	wxTextCtrl*			m_boneName;
	wxTreeCtrl*			m_boneTree;
	wxSlider*			m_sliderX;
	wxSlider*			m_sliderY;
	wxSlider*			m_sliderZ;
	wxSpinCtrl*			m_spinX;
	wxSpinCtrl*			m_spinY;
	wxSpinCtrl*			m_spinZ;

public:
	CEdControlRigBoneTransformPanel( wxWindow* parent, CEdControlRigPanel* ed );
	~CEdControlRigBoneTransformPanel();

	void OnActorSelectionChange( const CEntity* e );

	void SelectBone( const wxString& boneName );
	const String& GetSelectedBone() const;

	void RefreshUI( const Matrix& boneLS );
	void MarkElementsModified();
	void DisableSliders( Bool param1 );
private:
	
	void FillBoneTree();
	void FillBoneTree( const CSkeleton* skeleton, Int32 parentIndex, wxTreeItemId& parent, wxTreeCtrl* tree );
	Bool MarkElementsModified( wxTreeItemId& parent, wxTreeCtrl* tree );
	wxTreeItemId GetTreeBoneItemByName( const wxString& itemName ) const;

	void InternalSelectBone( const wxString& boneName );
	Bool SelectBoneForTree( const wxString& boneName, Bool callback );
	void SelectBoneForEdit( const wxString& boneName, Bool callback );
	void DeselectBone();

	void AddBoneSelectionToHistory( const wxString& boneName );
	void CallSelectionCallback( const wxString& boneName );

	void SetSliderRange( Int32 range );
	void CopyPoseToSliders();

	void Rotate( Float valX, Float valY, Float valZ );
protected:
	void OnSelectBoneName( wxCommandEvent& event );
	void OnSelectTreeBone( wxTreeEvent& event );
	void OnSliderRangeChanged( wxCommandEvent& event );
	void OnResetSliders( wxCommandEvent& event );
	void OnHistoryBtnSelected( wxCommandEvent& event );
	void OnRotSlider( wxCommandEvent& event );
	void OnRotSpinCtrl( wxSpinEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdControlRigHandsTransformPanel : public wxPanel
{

public:

	enum Side
	{
		ES_Left = 0,
		ES_Right,
		ES_SideSize
	};

	enum Part
	{
		EP_All = 0,
		EP_FingerA,
		EP_FingerB,
		EP_FingerC,
		EP_FingerD,
		EP_FingerE,
		EP_Twist,
		EP_HandX,
		EP_HandY,
		EP_HandZ,
		EP_PartSize
	};

	enum BoneRotationAxis
	{
		EA_AxisX,
		EA_AxisY,
		EA_AxisZ
	};


	struct PartInfo : public wxObject
	{
		PartInfo( Side side, Part part )
			: m_side( side ), m_part( part)
		{}
		Side	m_side;
		Part	m_part;	 
	};

	struct CorrectionData
	{
		TDynArray< CName >	m_bones;
		TDynArray< Float >	m_ratios;
		Float				m_currentVal;
		BoneRotationAxis	m_rotAxis;
	};

private:
	static Float BASE_ROT_ANGLE;

	wxButton*										m_hardResetBtn;
	TDynArray< TDynArray< wxSlider* > >				m_sliders;
	TDynArray< TDynArray< wxButton* > >				m_buttons;
	TDynArray< TDynArray< CorrectionData > >		m_data;
	TDynArray< CName >								m_dirtyBones;

	CEdControlRigPanel* m_parentWin;

public:
	CEdControlRigHandsTransformPanel( wxWindow* parent, CEdControlRigPanel* ed );

	void OnActorSelectionChange( const CActor* e );

private:
	void CalculatePoses();

public:
	void OnReset( wxCommandEvent& event );
	void OnSlider( wxCommandEvent& event );
	void OnHardReset( wxCommandEvent& event );
};
