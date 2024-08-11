/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fileGrid.h"
#include "editorPreviewCameraProvider.h"
#include "../../common/core/diskFile.h"

class CEdIconGrid;
class CEdCharacterEntityEditorConfiguration;
class CEdCharacterEntityEditorPreviewPanel;
class CEdColorShiftPairEditorPopup;
class IEdIconGridHook;

class IEdColorShiftPairEditorPopupHook
{
public:
	virtual ~IEdColorShiftPairEditorPopupHook(){}

	virtual void OnColorShiftPairModify( CEdColorShiftPairEditorPopup* sender, const CColorShift& shift1, const CColorShift& shift2 ){};
};

class CEdColorShiftPairEditorPopup : public wxDialog
{
	CColorShift							m_initialShift1;
	CColorShift							m_initialShift2;
	IEdColorShiftPairEditorPopupHook*	m_hook;

	wxStaticText*						m_primaryColorLabel;
	wxStaticText*						m_primaryColorHueShiftLabel;
	wxSlider*							m_primaryColorHueShiftSlider;
	wxStaticText*						m_primaryColorSaturationShiftLabel;
	wxSlider*							m_primaryColorSaturationShiftSlider;
	wxStaticText*						m_primaryColorLuminanceShiftLabel;
	wxSlider*							m_primaryColorLuminanceShiftSlider;
	wxStaticText*						m_secondaryColorLabel;
	wxStaticText*						m_secondaryColorHueShiftLabel;
	wxSlider*							m_secondaryColorHueShiftSlider;
	wxStaticText*						m_secondaryColorSaturationShiftLabel;
	wxSlider*							m_secondaryColorSaturationShiftSlider;
	wxStaticText*						m_secondaryColorLuminanceShiftLabel;
	wxSlider*							m_secondaryColorLuminanceShiftSlider;
	wxPanel*							m_panel1;
	wxButton*							m_restoreButton;
	wxButton*							m_resetButton;

	static wxString ColorShiftToString( const CColorShift& colorShift );
	static CColorShift StringToColorShift( const wxString& string );
	static void ExtractBothColorShiftsFromString( const wxString& string, CColorShift& colorShift1, CColorShift& colorShift2 );
	void CopyColorShiftsToClipboard( bool copy1, bool copy2 );

	void OnSliderChange( wxCommandEvent& event );
	void OnResetClick( wxCommandEvent& event );
	void OnRestoreClick( wxCommandEvent& event );
	void OnSwapClick( wxCommandEvent& event );
	void OnCopy1Click( wxCommandEvent& event );
	void OnPaste1Click( wxCommandEvent& event );
	void OnCopy2Click( wxCommandEvent& event );
	void OnPaste2Click( wxCommandEvent& event );
	void OnCopyAllClick( wxCommandEvent& event );
	void OnPasteAllClick( wxCommandEvent& event );

public:
	CEdColorShiftPairEditorPopup( wxWindow* parent, const wxString& title, const CColorShift& colorShift1, const CColorShift& colorShift2 );

	void StoreInitial();

	void SetColorShifts( const CColorShift& colorShift1, const CColorShift& colorShift2 );
	void GetColorShifts( CColorShift& colorShift1, CColorShift& colorShift2 ) const;
	static void DoCopyColorShiftsToClipboard( CColorShift* color1, CColorShift* color2, wxWindow* parent = nullptr );
	static bool PasteColorShiftsFromClipboard( CColorShift& colorShift1, CColorShift& colorShift2, bool& pasted1, bool& pasted2, wxWindow* parent = nullptr );

	void SetHook( IEdColorShiftPairEditorPopupHook* hook );
	RED_INLINE IEdColorShiftPairEditorPopupHook* GetHook() const { return m_hook; }
};

class CEdCharacterEntityEditor : public wxSmartLayoutPanel, public IEdColorShiftPairEditorPopupHook, public IEditorPreviewCameraProvider
{
	friend class CEdCharacterEntityEditorGridHook;
	friend class CEdCharacterEntityEditorPreviewPanel;
	friend class CEdCharacterEntityEditorPickPointClient;

	struct Part
	{
		CDiskFile*				file;
		String					bodypart;
		String					type;
		String					partgroup;

		Part(){}
	};

	struct PartRef
	{
		Part*					part;
		CColorShift				shift1;
		CColorShift				shift2;
		Bool					collapse;

		PartRef() : part( NULL ), collapse( false ) {}

		PartRef( Part* a_part )
			: part( a_part )
			, collapse( false )
		{}

		PartRef( const PartRef& src )
			: part( src.part )
			, shift1( src.shift1 )
			, shift2( src.shift2 )
			, collapse( src.collapse )
		{}

		bool operator ==(const PartRef& src) const
		{
			return src.part == part && src.shift1 == shift1 && src.shift2 == shift2 && src.collapse == collapse;
		}
	};

	struct Appearance
	{
		String					name;
		TDynArray<PartRef>		parts;
	};

	CEdCharacterEntityEditorPreviewPanel*			m_preview;
	CEdCharacterEntityEditorConfiguration*			m_config;
	TDynArray<Part*>								m_parts;
	THashSet<String>								m_partgroups;
	THashSet<Part*>									m_usedparts;
	THashMap<String,TDynArray<Part*>/* die c++ */>	m_bodyparts;
	THashMap<String,TDynArray<Part*>/* die c++ */>	m_types;
	THashMap<CDiskFile*,TDynArray<Part*>/* same */>	m_filePartMap;
	THashMap<Part*,CEdColorShiftPairEditorPopup*>	m_partColorEditorMap;
	CCharacterEntityTemplate*						m_template;
	CEdFileGrid*									m_grid;
	wxPanel*										m_appearancesPanel;
	CEdChoice*										m_appearanceChoice;
	wxPanel*										m_filterControls;
	wxPanel*										m_partButtons;
	wxChoice*										m_typesChoice;
	CEdChoice*										m_partgroupChoice;
	wxCheckBox*										m_usedOnly;
	wxChoice*										m_baseChoice;
	Uint32											m_baseIndex;
	CEdTimer										m_searchTimer;
	CEdTimer										m_applyAppearanceTimer;
	CEntity*										m_entity;
	TDynArray<PartRef>								m_charparts;
	THashMap<String,Appearance>						m_appearances;
	THashMap< CName, String >						m_compsToEntitiesMap;
	THandle<CEntityTemplate>						m_baseEntityOverride;
	IEdIconGridHook*								m_gridhook;
	bool											m_doNotUpdateCharacter;
	class CEdWorldPickPointClient*					m_pickPointClient;
	bool											m_hasWorldPreview;
	Vector											m_worldPreviewPoint;
	THandle< CEntity >								m_worldPreviewEntity;
	Bool											m_showStats;
	Bool											m_alreadyCalculatedTexStats;
	Float											m_textureStats;

	void CollectPartsFromEntity( Uint32& baseIndex, THashMap<String,TDynArray<String>/*c++ sux*/>& partfiles );
	void AfterTemplateLoad( Uint32 baseIndex, const THashMap<String,TDynArray<String>/*c++ sux*/>& partfiles );

	String TranslateBodypart( Char ch );
	String TranslateType( const String& s );

	void ScanDirectory( CDirectory* dir );
	void ConsiderFile( CDiskFile* file );

	void SetBase( Uint32 index );
	Uint32 GetBase() const;

	void RemoveParts();
	void AddPart( CDiskFile* file, const String& bodypart, const String& type );
	void UseParts();
	void SelectPart( Part* part );

	void UpdateAppearance();
	void ApplyAppearance( const Appearance& appearance );

	void CreateCharacterEntityTemplate( CEntityTemplate* target );
	void UpdateCharacter();

	void RemoveFilters();

	void ExportEntityTemplate();

	void RegisterPickPointClient();
	void UnregisterPickPointClient();
	void CreateWorldPreviewAt( const Vector& position );
	void DestroyWorldPreview();
	void UpdateWorldPreview();

	void OnPartButtonClick( wxCommandEvent& event );
	void OnAllPartsButtonClick( wxCommandEvent& event );

	void OnPreviewPopupMenuSelected( wxCommandEvent& event );
	void OnChoiceSelected( wxCommandEvent& event );
	void OnBaseChoiceSelected( wxCommandEvent& event );
	void OnPartgroupChoiceChange( wxCommandEvent& event );
	void OnAppearanceChoiceChange( wxCommandEvent& event );
	void OnDeleteAppearanceClicked( wxCommandEvent& event );
	void OnSearchTimer( wxTimerEvent& event );
	void OnApplyAppearanceTimer( wxTimerEvent& event );
	void OnIconGridSelectionChange();

	void OnFileSaveMenuItem( wxCommandEvent& event );
	void OnFileReloadMenuItem( wxCommandEvent& event );
	void OnFileExportEntityTemplateMenuItem( wxCommandEvent& event );
	void OnFileCloseMenuItem( wxCommandEvent& event );

	void OnCloseColorShiftPairEditor( wxCloseEvent& event );

	void HandleSelectionOfMeshComponent( CMeshTypeComponent* component );

	virtual void OnColorShiftPairModify( CEdColorShiftPairEditorPopup* sender, const CColorShift& shift1, const CColorShift& shift2 );

	void UpdateAppearanceList();

	Float CollectTextureUsage( CEntity* entity );

public:
	CEdCharacterEntityEditor( wxWindow* parent, CCharacterEntityTemplate* ce_template );
	~CEdCharacterEntityEditor();

	virtual wxString GetShortTitle();

	virtual IEditorPreviewCameraProvider::Info GetPreviewCameraInfo() const override;

private:
	DECLARE_EVENT_TABLE();
};
