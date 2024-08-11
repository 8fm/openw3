/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entityPreviewPanel.h"
#include "entityGraphEditor.h"
#include "../../common/game/inventoryEditor.h"
#include "../../common/game/aiProfile.h"
#include "propertiesPage.h"

#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "multiValueBitmapButtonControl.h"
#include "editorPreviewCameraProvider.h"
#include "dataError.h"

#include "../../common/game/storySceneParam.h"
#include "../../common/core/dataError.h"
#include "../../common/core/diskFile.h"

struct CEntityAppearance;
class CEdEffectEditor;
class CEdEntityEditorProperties;
class CEdEntitySlotProperties;
class CEdGameplayParamEditor;
class CEdEntityEditorMimicsPanel;

struct SEditorActorSpeech
{
	String  m_line;
	Uint32	m_id;

	SEditorActorSpeech() : m_id( 0 ) {}
};

struct SAppearancesAndColoringEntries
{
	TDynArray< CEntityAppearance* > appearances;
	TDynArray< SEntityTemplateColoringEntry > coloringEntries;
};

/// Specialized properties browser for the slot editor
class CEdEntitySlotProperties : public CEdPropertiesPage
{
protected:
	CEdEntityEditor*		m_editor;
	const EntitySlot*		m_slot;

public:
	//! Get owning entity editor
	RED_INLINE CEdEntityEditor* GetEntityEditor() const { return m_editor; }

	//! Get slot
	RED_INLINE const EntitySlot* GetSlot() const { return m_slot; }

public:
	CEdEntitySlotProperties( CEdEntityEditor* editor, wxWindow* parent, CEdUndoManager* undoManager )
		: CEdPropertiesPage( parent, PropertiesPageSettings(), undoManager )
		, m_editor( editor )
		, m_slot( nullptr )
	{};

	void SetSlot( const EntitySlot* slot );

	virtual class CEdEntitySlotProperties* QueryEntitySlotProperties()
	{
		return this;
	}
};

/// Entity editor
class CEdEntityEditor	: public wxSmartLayoutPanel
						, public EntityGraphEditorHook
						, public EntityPreviewPanelHook
						, public IEdEventListener
						, public IEquipmentPreview
						, public IEditorPreviewCameraProvider
						, public IDataErrorListener
{
	DECLARE_EVENT_TABLE()
	DECLARE_CLASS( CEdEntityEditor );

protected:
	CEdUndoManager*							m_undoManager;				//!< Undo manager
	CEdToolsPanel*							m_toolsPanel;
	CEdEntityPreviewPanel*					m_preview;					//!< Preview panel
	CEdEntityGraphEditor*					m_graph;					//!< Graph editor
	CEdSelectionProperties*					m_properties;				//!< Properties
	CEdSelectionProperties*					m_destructionProperties;	//!< Destruction properties
	CEntityTemplate*						m_template;					//!< Entity template
	wxAuiNotebook*							m_notebook;
	wxNotebook*								m_propertiesNotebook;
	wxPanel*								m_entityPanel;
	THashMap< int, CEdEffectEditor* >		m_effectEditors;
	CEdGameplayParamEditor*					m_gameplayParamEdit;
	wxToolBar*								m_widgetToolbar;

	// Body Parts tab
	TDynArray<CComponent*>					m_componentsOnBodyStatesList;
	TDynArray<Bool>                         m_bodyPartsIncludedFlag;
	wxListBox*								m_lstBodyParts;
	wxListBox*								m_lstBodyPartStates;
	wxCheckListBox*							m_lstBodyPartStateComponents;
	wxButton*								m_btnGenerateBodyParts;
	wxButton*								m_btnDuplicateBodyPart;
	wxButton*								m_btnAddBodyPart;
	wxButton*								m_btnDelBodyPart;
	wxButton*								m_btnDuplicateBodyPartState;
	wxButton*								m_btnAddBodyPartState;
	wxButton*								m_btnDelBodyPartState;
	wxTextCtrl*								m_txtBodyPartsFilter;
	wxTextCtrl*								m_txtBodyPartComponentsFilter;

	// Instance properties tab
	wxListCtrl*								m_instancePropertiesList;
	TDynArray< SComponentInstancePropertyEntry > m_visibleEntries;

	// Appearances tab
	TDynArray<CEntityAppearance* >			m_appearancesOnList;
	TDynArray<const CEntityTemplate*>		m_bodyPartsOnAppearanceList;
	TDynArray<Bool>                         m_appearancesIncludedFlag;
	wxCheckListBox*							m_lstAppearances;
	wxCheckListBox*							m_lstAppearanceBodyParts;
	wxButton*								m_btnDuplicateAppearance;
	wxButton*								m_btnAddAppearance;
	wxButton*								m_btnDelAppearance;
	wxTextCtrl*								m_txtAppearancesFilter;
	wxTextCtrl*								m_txtAppearanceBodyPartsFilter;
	wxCheckBox*								m_usesRobe;
	wxButton*								m_btnAddAppearanceTemplate;
	wxButton*								m_btnRemoveAppearanceTemplate;
	wxButton*								m_btnToggleAppearanceCollapse;
	wxButton*								m_btnCopyAppearanceColoringEntries;
	wxButton*								m_btnPasteAppearanceColoringEntries;

	// Colors tab
	wxChoice*								m_appearancesChoice;
	wxButton*								m_duplicateMeshColoringButton;
	wxButton*								m_addMeshColoringButton;
	wxButton*								m_removeMeshColoringButton;
	wxListBox*								m_meshColoringEntriesList;
	wxTextCtrl*								m_meshColoringEntryMesh;
	wxSlider*								m_primaryColorHueSlider;
	wxSpinCtrl*								m_primaryColorHueSpinCtrl;
	wxSlider*								m_primaryColorSaturationSlider;
	wxSpinCtrl*								m_primaryColorSaturationSpinCtrl;
	wxSlider*								m_primaryColorLuminanceSlider;
	wxSpinCtrl*								m_primaryColorLuminanceSpinCtrl;
	wxSlider*								m_secondaryColorHueSlider;
	wxSpinCtrl*								m_secondaryColorHueSpinCtrl;
	wxSlider*								m_secondaryColorSaturationSlider;
	wxSpinCtrl*								m_secondaryColorSaturationSpinCtrl;
	wxSlider*								m_secondaryColorLuminanceSlider;
	wxSpinCtrl*								m_secondaryColorLuminanceSpinCtrl;
	THashMap<Int32,Int32>					m_meshColoringListToEntryMap;
	wxButton*								m_copyColorsButton;
	wxButton*								m_pasteColorsButton;
	wxButton*								m_swapColorsButton;
	wxButton*								m_swapAllColorsButton;
	wxButton*								m_pasteAllColorsButton;
	
	// Includes & dependencies
	wxButton*								m_btnAddInclude;
	wxButton*								m_btnRemoveInclude;
	wxButton*								m_btnChangeEntityClass;
	wxTextCtrl*								m_edbClassName;
	wxListBox*								m_lstIncludes;
	wxListBox*								m_lstDependencies;
	wxCheckBox*								m_chkShowTemplatesOnly;

	wxTextCtrl*								m_streamedEntity0;
	wxTextCtrl*								m_streamedEntity1;
	wxTextCtrl*								m_streamedEntity2;
	wxTextCtrl*								m_streamedEntity3;
	wxTextCtrl*								m_streamedEntity4;

	wxButton*								m_btnAddStreamdET0;
	wxButton*								m_btnAddStreamdET1;
	wxButton*								m_btnAddStreamdET2;
	wxButton*								m_btnAddStreamdET3;
	wxButton*								m_btnAddStreamdET4;

	//Effects
	wxListBox*								m_lstAnimations;
	wxListBox*								m_lstEffects;
	wxButton*								m_btnConnect;
	wxButton*								m_btnDisconnect;
	wxButton*								m_btnAddEffect;
	wxButton*								m_btnEditEffect;
	wxButton*								m_btnRenameEffect;
	wxButton*								m_btnRemoveEffect;
	wxButton*								m_btnCopyEffect;
	wxButton*								m_btnPasteEffect;
	wxButton*								m_btnRefreshAnims;
	CEdPropertiesPage*						m_soundParamsProps;
	wxCheckBox*								m_soundOverrideParamsCheck;

	// Slots
	wxListBox*								m_lstSlots;
	wxButton*								m_btnAddSlots;
	wxButton*								m_btnRemoveSlots;
	CEdEntitySlotProperties*				m_slotProperties;
	CEdMultiValueBitmapButtonControl< Bool > m_showSlots;

	// Destruction
	TDynArray< CComponent* >				m_componentsOnDestructionStatesList;
	wxListBox*								m_lstDestructionStates;
	wxCheckListBox*							m_lstStateComponents;
	wxButton*								m_btnDuplicateState;	
	wxButton*								m_btnAddState;
	wxButton*								m_btnDelState;
	wxButton*								m_btnStateUp;
	wxButton*								m_btnStateDown;

	// AI tab
	// AI Tree subpage
	CEdSelectionProperties*					m_aiTreeProperties;
	wxPanel *								m_aiWizardSummaryPanel;
	wxGrid *								m_aiWizardSummaryGrid;
	wxChoice*								m_aiTreeEditChoice;
	wxButton *								m_runAIWizardButton;
	THandle< CAIBaseTree >					m_previewAITree;

	//		Reactions subpage
	wxNotebook*								m_aiProfileNotebook;
	wxListBox*								m_reactionsList;
	wxBitmapButton*							m_addReaction;
	wxBitmapButton*							m_removeReaction;
	wxBitmapButton*							m_moveReactionUp;
	wxBitmapButton*							m_moveReactionDown;
	CEdSelectionProperties*					m_reactionProperties;

	//		Senses suboage
	CEdSelectionProperties*					m_senseProperties;
	wxChoice*								m_senseChoice;
	wxCheckBox*								m_senseInheritCheckBox;
	wxStaticText*							m_senseTemplateStaticText;

	//		Misc subpage
	wxChoice*								m_attitudeGroup;
	wxStaticText*							m_attitudeGroupApplied; // current attitude group (recursive)
	wxStaticText*							m_aiWizardResName;
	wxButton*								m_aiWizardResButton;

	//		Minigame subpage
	wxCheckBox*								m_minigameWWEnable;
	wxTextCtrl*								m_minigameWWHotSpotMinWidth;
	wxTextCtrl*								m_minigameWWHotSpotMaxWidth;
	wxChoice*								m_minigameWWDifficulty;
	wxStaticText*							m_minigameWWStatusText;

	// Inventory
	wxListBox*								m_itemsList;
	wxBitmapButton*							m_addItem;
	wxBitmapButton*							m_removeItem;
	CEdSelectionProperties*					m_itemProperties;
	//THashMap< CName, TDynArray< CName > >		m_cachedEquipmentContents;
	//TDynArray< CName >					m_cachedCategories;

	// Appearance equipment
	wxListBox*								m_categoriesList;
	wxBitmapButton*							m_addCategory;
	wxBitmapButton*							m_removeCategory;
	CEdSelectionProperties*					m_categoryProperties;

	// Heads tab
	SEditorActorSpeech						m_currActorSpeech;
	static const Uint32						DEFAULT_ACTOR_SPEECH_ID = 46600;
	CEdEntityEditorMimicsPanel*				m_mimicsPanel;
	

	// Animation tab
	wxCheckBox*								m_animTabGlobalCheck;
	CEdPropertiesPage*						m_animTabGlobalPanelProp;
	wxCheckBox*								m_animTabLookAtCheck;
	CEdPropertiesPage*						m_animTabLookAtPanelProp;
	wxCheckBox*								m_animTabConstCheck;
	CEdPropertiesPage*						m_animTabConstPanelProp;
	wxCheckBox*								m_animTabMimicCheck;
	CEdPropertiesPage*						m_animTabMimicPanelProp;

	//gameplay tab
	wxCheckBox*								m_charStatsCheck;
	CEdPropertiesPage*						m_charStatsPanelProp;



	// Animation Slots tab
	wxListBox*								m_listAnimSlots;
	CEdPropertiesPage*						m_animSetsPanelProp;

	CGridEditor*							m_voicetagAppearanceGrid;
	wxCheckBox*								m_voiceTabVoicesetCheck;
	CEdPropertiesPage*						m_voiceTabVoicesetPanelProp;


	// Dismemberment tab
	wxCheckListBox*							m_listDismemberWounds;
	CEdPropertiesBrowserWithStatusbar*		m_dismemberWoundProp;
	CEdMultiValueBitmapButtonControl< Bool > m_showWounds;


	Bool									m_hackNeedsSplitBarResize;
	Int32									m_currentStreamingLOD;
	Bool									m_isInitialized; // To check whether initialization is done as autoplay effect is played on the entity and window initialization stops it OnNotebookPageChanged (tabs population)

	TDynArray< CEntityTemplate* >           m_loadedIncludes;   //!< Includes that are currently in memory
	TSortedArray< SDataError, SDataError::BySeverity > m_dataErrors;		//!< Errors collected by data errror reporter for active entity; take care of ordering them by their severity

	THashMap< const CComponent*, String >	m_componentComments;

	CEdMultiValueBitmapButtonControl<Bool>							m_freezeEntityPose;
	CEdMultiValueBitmapButtonControl<IPreviewItem::PreviewSize >	m_previewItemSize;
	TDynArray< String >												m_dependentResourcesPaths;

	static TDynArray< THandle< CFXDefinition > > s_fxInClipboard; //!< Effect that we can copy from one entity to another

public:
	RED_INLINE CEdEntityPreviewPanel* GetPreviewPanel() const { return m_preview; }

public:
	CEdEntityEditor( wxWindow* parent, CEntityTemplate* entityTemplate );
	~CEdEntityEditor();

	static SAppearancesAndColoringEntries m_appearancesAndColorEntriesToCopy;

	virtual wxString GetShortTitle() { return m_template->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Entity Editor")); }

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
	void RealignOverlappingComponents( Bool all = true );

	void UpdateDynamicTemplate();
	void UpdateAppearanceList();
	void UpdateInstancePropertiesList();
	void UpdateAnimEffectsList();
	void UpdateReactionsList();
	void UpdateCategoriesList();
	void UpdateInventoryItemsList();
	void UpdateVoicetagAppearancesTable();
	void UpdateSlotPage();
	void UpdateSlotList( const CName& slotNameToSelect = CName::NONE );
	void UpdateIncludeList();
	void UpdateDependencyList();
	void UpdateEntityClass();
	void UpdateAnimationTab();
	void UpdateAnimSlotsList( const CName& animSlotsNameToSelect = CName::NONE );
	void UpdateTabs();

	CEdEffectEditor* EditEffect( CFXDefinition* def, Bool forceRecreateEditor = false );
	Bool PlayPreviewEffect( CFXDefinition* def );
	Bool CloseEffectEditor( CFXDefinition* def );
	String FormatEffectName( CFXDefinition* def );
	void StopPlayedEffect();
	Bool UpdateAnimationLength( CFXDefinition* def );

	void RefreshComponentComments();
	Bool FindComponentComment( const CComponent* c, String& outStr ) const;

	Int32 GetSelectedStreamingLOD() const;

	// implement IDataErrorListener interface
	virtual void OnDataErrorReported( const SDataError& error );
	virtual void StartProcessing()													{	}
	virtual void ProcessDataErrors( const TDynArray< SDataError >& erros )			{	}
	virtual void StoreNonProcessedErrors( const TDynArray< SDataError >& errors )	{	}
	virtual void StopProcessing()													{	}

	// get data errors
	void GetDataErrors( TDynArray< String >& arrayForErrors );
	void AddDataErrors( const TDynArray< SDataError >& dataErrors );

	virtual IEditorPreviewCameraProvider::Info GetPreviewCameraInfo() const override;

	void SelectComponents( const TDynArray< CComponent* >& components );
	void HideComponents( const TDynArray< CComponent* >& components );
	void IsolateComponents( const TDynArray< CComponent* >& components );

protected:
	virtual void OnGraphSelectionChanged();
	virtual void OnGraphSelectionDeleted();
	void OnPreviewSelectionChanged();
	void OnPreviewWidgetModeChanged();
	void OnPreviewWidgetSpaceChanged();
	void OnGranularityChanged( wxCommandEvent& event );
	void OnPreviewItemTransformChanged();
	void OnAIPropertiesChanged( wxCommandEvent& event );
	void DoSave( Bool withStreamedComponents );
	void OnSave( wxCommandEvent& event );
	void OnSaveWithStreamedComponents( wxCommandEvent& event );

	void UpdateUsedAppearances();
	void OnPrepare( wxCommandEvent& event );
	void OnExportEntity( wxCommandEvent& event );
	void OnImportEntity( wxCommandEvent& event );
	void OnRemapSkinning( wxCommandEvent& event );
	void OnZoomExtentsGraph( wxCommandEvent& event );
	void OnZoomExtentsPreview( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );
	void OnSelectAll( wxCommandEvent& event );
	void OnUnselectAll( wxCommandEvent& event );
	void OnEditFindEntityComponents( wxCommandEvent& event );
	void OnInvertSelection( wxCommandEvent& event );
	void OnRefreshInEditor( wxCommandEvent& event );
	void OnNotebookPageChanged( wxAuiNotebookEvent& event );
	void OnNotebookPageClosed( wxAuiNotebookEvent& event );
	void OnPropertiesNotebookPageChanged( wxNotebookEvent& event );
	void OnResetForcedLOD( wxCommandEvent& event );
	void OnPreviewLOD0( wxCommandEvent& event );
	void OnPreviewLOD1( wxCommandEvent& event );
	void OnPreviewLOD2( wxCommandEvent& event );
	void OnPreviewLOD3( wxCommandEvent& event );

	void OnShowBoundingBox( wxCommandEvent& event );
	void OnShowCollision( wxCommandEvent& event );
	void OnShowWireframe( wxCommandEvent& event );
	void OnShowTBN( wxCommandEvent& event );

public:
	void OnPreviewEntityLOD0( wxCommandEvent& event );
	void OnPreviewEntityLOD1( wxCommandEvent& event );
	void OnPreviewEntityLOD2( wxCommandEvent& event );
	void OnPreviewEntityLOD3( wxCommandEvent& event );

protected:
	void OnIncludeAdd( wxCommandEvent& event );
	void OnIncludeRemove( wxCommandEvent& event );
	void OnIncludeClicked( wxCommandEvent& event );
	void OnDependencyFilterChanged( wxCommandEvent& event );
	void OnDependencyClicked( wxCommandEvent& event );
	void OnChangeEntityClass( wxCommandEvent& event );
	void RebuildEntityAfterIncludesChanged();

	void OnBodyPartSelected                ( wxCommandEvent& event );
	void OnBodyPartDoubleClicked           ( wxCommandEvent& event );
	void OnBodyPartStateDoubleClicked      ( wxCommandEvent& event );
	void OnBodyPartStateComponentToggled   ( wxCommandEvent& event );
	void OnBodyPartsGenerate               ( wxCommandEvent& event );
	void OnBodyPartDuplicate               ( wxCommandEvent& event );
	void OnBodyPartAdd                     ( wxCommandEvent& event );
	void OnBodyPartDel                     ( wxCommandEvent& event );
	void OnBodyPartStateDuplicate          ( wxCommandEvent& event );
	void OnBodyPartStateAdd                ( wxCommandEvent& event );
	void OnBodyPartStateDel                ( wxCommandEvent& event );
	void OnBodyPartsFilterChanged          ( wxCommandEvent& event );
	void OnBodyPartComponentsFilterChanged ( wxCommandEvent& event );
	void OnAppearanceSelected              ( wxCommandEvent& event );
	void OnAppearanceDoubleClicked         ( wxCommandEvent& event );
	void OnAppearanceToggled               ( wxCommandEvent& event );
	void OnActiveAppearancesListContextMenu( wxContextMenuEvent& event );
	void OnActiveAppearancesListMenuExport ( wxCommandEvent& event );
	void OnActiveAppearancesListMenuImport ( wxCommandEvent& event );
	void OnAppearanceBodyPartToggled       ( wxCommandEvent& event );
	void OnAppearanceDuplicate             ( wxCommandEvent& event );
	void OnAppearanceAdd                   ( wxCommandEvent& event );
	void OnAppearanceDel                   ( wxCommandEvent& event );
	void OnAppearancesFilterChanged        ( wxCommandEvent& event );
	void OnAppearanceBodyPartsFilterChanged( wxCommandEvent& event );
	void OnAppearanceVoicetagSelected      ( wxCommandEvent& event );
	void OnUsesRobeToggled					( wxCommandEvent& event );
	void OnAppearanceTemplateAdded		   ( wxCommandEvent& event );
	void OnAppearanceTemplateRemoved	   ( wxCommandEvent& event );
	void OnAppearanceTemplateCollapseToggled( wxCommandEvent& event );
	void OnDisableAppearanceApplication    ( wxCommandEvent& event );
	void OnCopyAppearanceColoringEntries   ( wxCommandEvent& event );
	void OnPasteAppearanceColoringEntries   ( wxCommandEvent& event );

	void OnAddInstancePropertiesEntryClicked( wxCommandEvent& event );	
	void OnRemoveInstancePropertiesEntryClicked( wxCommandEvent& event );	
	void OnRemoveComponentInstancePropertiesEntriesClicked( wxCommandEvent& event );	
	void OnRemoveAllInstancePropertiesEntriesClicked( wxCommandEvent& event );	

	void OnColorAppearanceSelected		   ( wxCommandEvent& event );
	void OnMeshColoringEntriesListSelected ( wxCommandEvent& event );
	void OnDuplicateMeshColorButtonClick   ( wxCommandEvent& event );
	void OnAddMeshColorButtonClick		   ( wxCommandEvent& event );
	void OnRemoveMeshColorButtonClick	   ( wxCommandEvent& event );
	void OnMeshColoringEntryParametersChanged( wxCommandEvent& event );
	void OnCopyColorsButtonClicked		   ( wxCommandEvent& event );
	void OnPasteColorsButtonClicked		   ( wxCommandEvent& event );
	void OnSwapColorsButtonClicked		   ( wxCommandEvent& event );
	void OnSwapAllColors				   ( wxCommandEvent& event );
	void OnPasteAllColors				   ( wxCommandEvent& event );
	void SetColorControlsFromColorShifts   ( const CColorShift& primary, const CColorShift& secondary );
	void EnableMeshColoringEntryControls   ( Bool enable );
	void RefreshMeshColoringEntriesList();
	CEntityAppearance* RetrieveSelectedColorAppearance();

	void OnEffectConnect					( wxCommandEvent& event );
	void OnEffectDisconnect					( wxCommandEvent& event );
	void OnEffectAdd						( wxCommandEvent& event );
	void OnEffectEdit						( wxCommandEvent& event );
	void OnEffectRename						( wxCommandEvent& event );
	void OnEffectRemove						( wxCommandEvent& event );
	void OnEffectCopy						( wxCommandEvent& event );
	void OnEffectPaste						( wxCommandEvent& event );
	void OnEffectRefreshList				( wxCommandEvent& event );
	void OnAnimationsSelected				( wxCommandEvent& event );
	void OnAnimationsDoubleClicked			( wxCommandEvent& event );
	void OnEffectsSelected					( wxCommandEvent& event );
	void OnEffectsDoubleClicked				( wxCommandEvent& event );
	void OnSize								( wxSizeEvent& event );
	void OnPaint							( wxPaintEvent &event );
	void checkAnimEffectConnection();

	void OnSlotAdd							( wxCommandEvent& event );
	void OnSlotRemove						( wxCommandEvent& event );
	void OnSlotSelectionChanged				( wxCommandEvent& event );
	void OnSlotModified						( wxCommandEvent& event );
	void OnSlotShow							( wxCommandEvent& event );
	void OnSlotOverride						( wxCommandEvent& event );
	void OnSlotPageShow();
	void OnSlotPageHide();

	void OnHeadStateNormal					( wxCommandEvent& event );
	void OnHeadStateMimicLow				( wxCommandEvent& event );
	void OnHeadStateMimicHigh				( wxCommandEvent& event );
	void OnHeadSpeechTest					( wxCommandEvent& event );
	void OnHeadSpeechSelect					( wxCommandEvent& event );

	void OnAnimTabGlobalCheck				( wxCommandEvent& event );
	void OnAnimTabLookAtCheck				( wxCommandEvent& event );
	void OnAnimTabConstCheck				( wxCommandEvent& event );
	void OnAnimTabMimicCheck				( wxCommandEvent& event );
	void OnVoiceTabVoicesetCheck			( wxCommandEvent& event );
	void OnMimicPanelActivated				( wxCommandEvent& event );
	void OnMimicPanelFovChanged				( wxCommandEvent& event );

	template< typename T > void OnAnimTabUpdateTabList( const AnsiChar* listCtrlXrcId );
	template< typename T > void OnAnimTabSetObject( T* object, const AnsiChar* propPanelXrcId, Bool setNoneIfNull );
	template< typename T > T* AnimTabGetParamFromEvent( wxCommandEvent& event );
	template< typename T > void AnimTabAddParam( const AnsiChar* propPanelXrcId, const AnsiChar* listCtrlXrcId );
	template< typename T > void AnimTabRemoveParam( const AnsiChar* propPanelXrcId, const AnsiChar* listCtrlXrcId );
	template< typename T > void AnimTabListParam( wxCommandEvent& event, const AnsiChar* propPanelXrcId, const AnsiChar* listCtrlXrcId );

	void OnAnimTabBehAdded					( wxCommandEvent& event );
	void OnAnimTabBehRemoved				( wxCommandEvent& event );
	void OnAnimTabBehListChanged			( wxCommandEvent& event );
	void OnAnimTabBehPropModifiedUnused		( wxCommandEvent& event );

	void OnAnimTabAnimsetAdded				( wxCommandEvent& event );
	void OnAnimTabAnimsetRemoved			( wxCommandEvent& event );
	void OnAnimTabAnimsetListChanged		( wxCommandEvent& event );
	void OnAnimTabAnimsetPropModifiedUnused	( wxCommandEvent& event );

	void OnAnimSlotsAdded					( wxCommandEvent& event );
	void OnAnimSlotsRemoved					( wxCommandEvent& event );
	void OnAnimSlotsSelectionChanged		( wxCommandEvent& event );
	void OnAnimSlotsModified				( wxCommandEvent& event );
	
	void OnAiProfileNotebookPageChanged		( wxNotebookEvent& event );
	void OnReactionSelected					( wxCommandEvent& event );
	void OnAddReaction						( wxCommandEvent& event );
	void OnRemoveReaction					( wxCommandEvent& event );
	void OnMoveReactionUp					( wxCommandEvent& event );
	void OnMoveReactionDown					( wxCommandEvent& event );
	void OnReactionChanged					( wxCommandEvent& event );
	void OnSenseTypeChanged					( wxCommandEvent& event );
	void OnSenseInheritClicked				( wxCommandEvent& event );
	void OnSensePropertiesChanged			( wxCommandEvent& event );
	void OnAITreeSetupChanged				( wxCommandEvent& event );
	void OnRunAIWizard						( wxCommandEvent& event );
	void OnAiTreeChanged					( wxCommandEvent& event );
	void OnAttackRangeInheritClicked		( wxCommandEvent& event );
	void OnAttackRangeChanged				( wxCommandEvent& event );
	void OnAttitudeGroupChoice				( wxCommandEvent& event );
	void OnUseAiWizardRes					( wxCommandEvent& event );

	void OnTogglePreviewItemSize			( wxCommandEvent& event );
	void OnFreezePose						( wxCommandEvent& event );
	void OnForceTPose						( wxCommandEvent& event );

	void OnGenerateProxyMesh				( wxCommandEvent& event );
	void OnSetupProxyMesh					( wxCommandEvent& event );
	void OnDeduplicateComponents			( wxCommandEvent& event );

	// AI
	void ChangeAITree( CClass *const treeClass );
	CResource *const GetAIWizardResource();
	EAISenseType GetSelectedSenseType();
	CEntityTemplate* CacheSenseParams( EAISenseType senseType );
	CAISenseParams* GetCachedSenseParams( EAISenseType senseType ) const;
	THandle< CAIBaseTree > FillAiTreeWithParams( Bool inherit );
	void FillSenseProperties( Bool setCheckBox );
	void FillAiTreePanels( );
	void FillAiWizardResData();
	CEntityTemplate* GetInheritedAIMainTemplate()const;
	THandle< CAIBaseTree > GetCachedAiTree() const;
	CAIBaseTree* GetLocalAIBaseTreeOnTemplate( Bool backwardCompatHack = true ) const;
	void FillAttitudeGroupChoice();
	void RefreshAppliedAttitudeGroup();

	// Minigame tab
	void UpdateMinigameTabGui();
	void UpdateMinigameTabGuiWWStatus();
	void MinigameTabGuiDisableWWControls();
	void OnMinigameWWUpdateDataFromGui();
	void OnMinigameWWEnable( wxCommandEvent& event );
	void OnMinigameWWHotSpotMinWidth( wxFocusEvent& event );
	void OnMinigameWWHotSpotMaxWidth( wxFocusEvent& event );
	void OnMinigameWWDifficulty( wxCommandEvent& event );

	//void FillEquipmentPanel				( CEntityAppearance* appearance );
	void OnInventoryItemSelected			( wxCommandEvent& event );
	void ExpandInventoryItemProperties		( const TDynArray<CBasePropItem*>& items );
	void OnAddInventoryItem					( wxCommandEvent& event );
	void OnRemoveInventoryItem				( wxCommandEvent& event );
	void OnInventoryItemChanged				( wxCommandEvent& event );

	void OnEquipmentCategoryClicked			( wxMouseEvent& event );
	void ExpandEquipmentEntryProperties		( const TDynArray<CBasePropItem*>& items );
	void OnAddEquipmentCategory				( wxCommandEvent& event );
	void OnRemoveEquipmentCategory			( wxCommandEvent& event );
	void OnEquipmentCategoryChanged			( wxCommandEvent& event );
	void ApplyEquipmentDefinitionPreview	();
	CEntityAppearance* GetSelectedAppearance ();

	void SoundUpdateParams					();
	void OnSoundOverrideParams				( wxCommandEvent& event );

	//Bool IsBodyPartIgnoredInColorShift( const CEntityBodyPart* const bodyPart );
	//void CollectBodyPart

	void MoveReaction( Bool up );
	Bool RunAiWizard();


	//
	// Dismemberment tab
	//

	void InitDismemberTab();

	void UpdateDismembermentTab();
	void UpdateDismembermentList( const CName& woundNameToSelect = CName::NONE );

	void OnWoundsToggleShow( wxCommandEvent& event );
	void OnWoundOverride( wxCommandEvent& event );
	void OnWoundAdded( wxCommandEvent& event );
	void OnWoundRemoved( wxCommandEvent& event );
	void OnWoundsExport( wxCommandEvent& event );
	void OnDismemberWoundSelectionChanged( wxCommandEvent& event );
	void OnDismemberWoundModified( wxCommandEvent& event );
	void OnDismemberPageShow();
	void OnDismemberPageHide();
	void RefreshVisualWoundItem( const CDismembermentWound* selectedWound );
	void UpdateVisibleWound();
	void OnDismemberPreviewItemTransformChanged();

	// Only a single dismemberment component is supported. When the entity is changed, we check if there
	// are multiple and give the user a message.
	void CheckForMultipleDismemberment();
	// During save, we need to disable the dismemberment preview, so the fill mesh doesn't get saved.
	void DeselectDismemberWound();
	// User should not be able to select the fill mesh.
	void PreventSelectingDismemberFillMesh();

	// If the entity template has any dismemberment, remove any wound-appearance filters that are not valid (either the wound or
	// the appearance doesn't exist).
	void UpdateUsedDismemberment();
	void UpdateDismemberAppearances();

	void OnDismemberAppearanceSelected( wxCommandEvent& event );
	void OnDismemberWoundChecked( wxCommandEvent& event );

private:
	void SetEntityAppearance( const CEntityAppearance* appearance );


	Bool ValidateEntityOnSave( CEntity* previewEntity );
	Bool Validate( TDynArray< String >& log );
	void ValidateAnimTabs( TDynArray< String >& log );
	void ValidateAttitudeGroup( TDynArray< String >& log );

	void InternalRefreshComponentComments();
	Bool IsModifiedByDLC() const;

private:
	template< typename T > 
	void UpdateAnimTabParam( wxCheckBox* box, CEdPropertiesPage* prop )
	{
		T* params = m_template->FindParameter< T >( false );
		if ( params )
		{
			box->SetValue( true );

			prop->SetReadOnly( false );
			prop->SetObject( params );
		}
		else
		{
			box->SetValue( false );

			T* includeParams = m_template->FindParameter< T >( true );
			if ( includeParams )
			{
				prop->SetObject( includeParams );
				prop->SetReadOnly( true );
			}
			else
			{
				prop->SetReadOnly( false );
				prop->SetNoObject();
			}
		}
	}

	template< typename T > 
	void AnimTabCheck( wxCheckBox* box, CEdPropertiesPage* prop )
	{
		if ( box->IsChecked() )
		{
			{
				T* params = m_template->FindParameter< T >( false );
				ASSERT( !params );
				if ( params )
				{
					m_template->RemoveParameter( params );
				}
			}

			// Create
			{
				T* params = CreateObject< T >( m_template );
				if ( params )
				{
					// Add to template
					m_template->AddParameterUnique( params );

					// Add to property page in editor
					prop->SetReadOnly( false );
					prop->SetObject( params );
				}
				else
				{
					T* includeParams = m_template->FindParameter< T >( true );
					if ( includeParams )
					{
						prop->SetObject( includeParams );
						prop->SetReadOnly( true );
					}
					else
					{
						prop->SetReadOnly( false );
						prop->SetNoObject();
					}
				}
			}
		}
		else
		{
			// Remove from template
			T* params = m_template->FindParameter< T >( false );
			if ( params )
			{
				m_template->RemoveParameter( params );
			}

			{
				T* includeParams = m_template->FindParameter< T >( true );
				if ( includeParams )
				{
					prop->SetObject( includeParams );
					prop->SetReadOnly( true );
				}
				else
				{
					prop->SetReadOnly( false );
					prop->SetNoObject();
				}
			}
		}
	}

	void RefreshVisualSlotItem();
	void RefreshVisualSlotChanges();

	void PrepareForLODChange();
	void LoadEntityLOD( Uint32 lod );

protected:
	void ForceEntityTPose();
	void ToggleFreezeEntityPose();

protected:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void OnWoundPropertyModified( wxCommandEvent& event );
	void OnAnimTabAnimsetPropModified( wxCommandEvent& event );
	void OnAnimTabBehPropModified( wxCommandEvent& event );

};

namespace EntityEditorUtils
{
	Bool IsTemplateIncluded( const CEntityTemplate* testedTemplate, const CEntityTemplate* includedTemplate );
	Bool IsTemplateIncludedAtLevel( const CEntityTemplate* testedTemplate, const CEntityTemplate* includedTemplate );
}
