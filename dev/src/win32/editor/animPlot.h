/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdAnimPlotTree;

//////////////////////////////////////////////////////////////////////////

typedef TPair< String, CCurve* > TPlotCurveData;

class IAnimPlotTreeHook
{
public:
	virtual void OnCurveSelected( TPlotCurveData curveData ) = 0;
	virtual void OnCurvesSelected( TDynArray< TPlotCurveData >& curveDatas ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimPlot : public wxSmartLayoutPanel, public IAnimPlotTreeHook, public IEdCurveEditorCanvasHook
{
	DECLARE_EVENT_TABLE()

	CEdCurveEditorCanvas*				m_curveEditorCanvas;

	const CSkeletalAnimationSetEntry*	m_animation;

public:
	CEdAnimPlot( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton = NULL );
	~CEdAnimPlot();

public:
	virtual void OnCanvasHookSelectionChanged() {}
	virtual void OnCanvasHookControlPointsChanged() {}
	virtual void OnCanvasHookControlPointsChangedComplete() {}

public:
	virtual void OnCurveSelected( TPlotCurveData curveData );
	virtual void OnCurvesSelected( TDynArray< TPlotCurveData >& curveDatas );

protected:
	const CSkeletalAnimationSetEntry* GetAnimEntry() const;
	const CSkeletalAnimation* GetAnim() const;

protected:
	void OnOk( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimPlotTree : public wxPanel
{
	DECLARE_EVENT_TABLE()

protected:
	enum ECurveType
	{
		CT_PosX = 0,
		CT_PosY = 1,
		CT_PosZ = 2,
		CT_RotX = 3,
		CT_RotY = 4,
		CT_RotZ = 5,
		CT_Size = 6,
		CT_ScaleX = 6,
		CT_ScaleY = 7,
		CT_ScaleZ = 8,
		CT_SizeWithScale = 9,
	};

	enum ETreeDataType
	{
		TDT_None,

		TDT_TransNode,
		TDT_TransX,
		TDT_TransY,
		TDT_TransZ,

		TDT_RotNode,
		TDT_RotX,
		TDT_RotY,
		TDT_RotZ,

		TDT_ScaleNode,
		TDT_ScaleX,
		TDT_ScaleY,
		TDT_ScaleZ,

		TDT_BoneNode,
		TDT_BoneData,

		TDT_TrackNodes,
		TDT_Track,

		TDT_MotionExNode,
	};

	class TreeData : public wxTreeItemData
	{
		ETreeDataType	m_type;
		CCurve*			m_curve;

	public:
		TreeData( ETreeDataType type, CCurve* curve ) : m_type( type ), m_curve( curve ) {}

		ETreeDataType GetType() const { return m_type; }
		CCurve* GetCurve() const { return m_curve; }
	};

	Float					m_fps;
	IAnimPlotTreeHook*		m_hook;

	wxTreeCtrl*				m_wxTree;

	TDynArray< CCurve* >	m_curves;

public:
	CEdAnimPlotTree( wxWindow* parent );
	virtual ~CEdAnimPlotTree();

	virtual void Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton = NULL );

public:
	void SetHook( IAnimPlotTreeHook* hook );

	void ClearAndFill( const CSkeletalAnimation* animation, const CSkeleton* skeleton = NULL );
	void Clear();

protected:
	void AddCurve( CCurve* curve );
	void AddCurves( CCurve** curve, Uint32 num );
	void FillCurves( CCurve** curve, const TDynArray< AnimQsTransform >& data, TDynArray< Float >* times = NULL );
	void FillCurvesWithScale( CCurve** curve, const TDynArray< AnimQsTransform >& data, TDynArray< Float >* times = NULL );
	void ClearCurves();
	void ExtractMotionExtraction( const CSkeletalAnimation* animation, TDynArray< AnimQsTransform >& data, TDynArray< Float >* times = NULL );
	void PrepareBoneData( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data );
	void ReleaseBoneData( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data );
	virtual void SampleBones( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data, const CSkeleton* skeleton );
	void FillSkeletonData( const CSkeleton* skeleton, wxTreeItemId root, Int32 parent, TDynArray< SBehaviorGraphOutput >& data );
	void FillSkeletonTrackData( const CSkeleton* skeleton, wxTreeItemId root, Int32 parent, TDynArray< SBehaviorGraphOutput >& data );
	
	String GetNameFromType( ETreeDataType type ) const;
	Color GetColorFromTrackNum( Int32 num ) const;
	Bool ExistsInList( const TDynArray< TPlotCurveData >& datas, const String& str ) const;
	wxTreeItemId AddNode( wxTreeItemId root, wxString nodeName, ETreeDataType nodeType, TDynArray< AnimQsTransform >& data, TDynArray< Float >* times = NULL );
	wxTreeItemId AddNodeWithScale( wxTreeItemId root, wxString nodeName, ETreeDataType nodeType, TDynArray< AnimQsTransform >& data, TDynArray< Float >* times = NULL );

protected:
	void OnItemSelected( wxTreeEvent& event );
	void OnItemActivated( wxTreeEvent& event );
};

class CEdAnimBonePlotTree : public CEdAnimPlotTree
{
public:
	CEdAnimBonePlotTree( wxWindow* parent );

	virtual void Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton = NULL );
};

class CEdAnimBoneMSPlotTree : public CEdAnimBonePlotTree
{
public:
	CEdAnimBoneMSPlotTree( wxWindow* parent );

	virtual void SampleBones( const CSkeletalAnimation* animation, TDynArray< SBehaviorGraphOutput >& data, const CSkeleton* skeleton );
};

class CEdAnimMotionTree : public CEdAnimPlotTree
{
	CMotionExtractionLineCompression* m_compression;
	CMotionExtractionLineCompression2* m_compression2;

public:
	CEdAnimMotionTree( wxWindow* parent );
	virtual ~CEdAnimMotionTree();

	virtual void Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton = NULL );

	CMotionExtractionLineCompression* GetCompression() const { return m_compression; }
	CMotionExtractionLineCompression2* GetCompression2() const { return m_compression2; }

protected:
	void ExtractCompressedMotionExtraction( const CSkeletalAnimation* animation, TDynArray< AnimQsTransform >& data, TDynArray< Float >& times );
	wxTreeItemId AddDoubleNode( wxTreeItemId root, wxString nodeName, ETreeDataType nodeType, TDynArray< AnimQsTransform >& data1, TDynArray< AnimQsTransform >& data2, TDynArray< Float >* times1 = NULL, TDynArray< Float >* times2 = NULL );
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimBonePlot : public CEdAnimPlot
{
	CEdAnimBonePlotTree*	m_tree;

public:
	CEdAnimBonePlot( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton = NULL );
	~CEdAnimBonePlot();

	void Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton = NULL );
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimBoneMSPlot : public CEdAnimPlot
{
	CEdAnimBoneMSPlotTree*	m_tree;

public:
	CEdAnimBoneMSPlot( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton = NULL );
	~CEdAnimBoneMSPlot();

	void Fill( const CSkeletalAnimation* animation, const CSkeleton* skeleton = NULL );
};

//////////////////////////////////////////////////////////////////////////

class CEdMotionExtractionPreview : public CEdAnimPlot
{
	CEdAnimMotionTree*		m_tree;

	THandle< CSkeleton >				m_skeleton;
	const CSkeletalAnimationSetEntry*		m_animation;

public:
	CEdMotionExtractionPreview( wxWindow* parent, const CSkeletalAnimationSetEntry* anim, const CSkeleton* skeleton = NULL );
	~CEdMotionExtractionPreview();

protected:
	void OnRecompress( wxCommandEvent& event );
};
