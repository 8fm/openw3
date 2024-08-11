#pragma once

// terrain edit tool V2.0
#include "..\..\common\engine\clipMap.h"
#include "terrainBrush.h"
#include "terrainCursor.h"
#include "textureArrayGrid.h"
#include "terrainEditWorkingBuffers.h"

class CEdTextureArrayGrid;

// NOTE: Make sure to update TERRAIN_TOOL_MEMORY_POOL_MAX_TEXELS in memoryPoolsWindows.h
#define MAX_TEXELS_PER_EDGE	3072

#define NUM_COLOR_PRESETS	20


enum ETerrainEditStampDataFlag
{
	TESDF_None		= 0,
	TESDF_Height	= FLAG( 0 ),
	TESDF_Control	= FLAG( 1 ),
	TESDF_Color		= FLAG( 2 ),
	TESDF_Additive	= FLAG( 3 ),

	TESDF_DataTypesMask = TESDF_Height | TESDF_Control | TESDF_Color,
};


enum ETerrainEditProcessPaintingBufferFlags
{
	PPBF_ReadHM			= FLAG( 0 ),
	PPBF_ReadCM			= FLAG( 1 ),
	PPBF_ReadColor		= FLAG( 2 ),
	PPBF_WriteHM		= FLAG( 3 ),
	PPBF_WriteCM		= FLAG( 4 ),
	PPBF_WriteColor		= FLAG( 5 ),
	PPBF_CollisionType	= FLAG( 6 ),

	PPBF_Read_Mask = PPBF_ReadHM | PPBF_ReadCM | PPBF_ReadColor,
	PPBF_Write_Mask = PPBF_WriteHM | PPBF_WriteCM | PPBF_WriteColor,
};


struct STerrainToolControlsParams
{
	STerrainToolControlsParams ()
		: radius ( 50 )
		, intensity ( 1 )
		, height ( 0 )
		, slope ( 0 )
		, slopeAngle ( 0 )
		, stampAngle( 0 )
		, offset( 0 )
		, stampScaleOrigin( 0 )
		, filterSize( 1 )
	{}

	Float radius;
	Float intensity;
	Float height;
	Float slope;
	Float slopeAngle;
	Float stampAngle;
	Float offset;
	Float stampScaleOrigin;
	Float filterSize;
	String heightmapPath;
};

struct CTerrainEditCursor
{
	EToolType					m_toolType;					// done
	Vector						m_position;					// done
	Vector						m_lastPosition;
	Float						m_brushSize;				// done
	Float						m_desiredElevation;			// done

	Float*						m_falloffData;				// done
	Bool						m_falloffDataNeedsRebuild;

	Uint32						m_texelsPerEdge;			// done
	Float						m_texelsPerUnit;			// done
	Vector2						m_currScreenSpacePos;
	Vector2						m_prevScreenSpacePos;
	SSlopeBrushParams			m_slopeParams;
	SStampBrushParams			m_stampParams;
	CCurve*						m_falloffCurve;
	Float						m_prevBrushSize;
	Float						m_prevDesiredElevation;
	Uint16						m_creationFlags;
	ECursorMode					m_mode;						// done
	Float						m_intensity;				// done
	Float						m_intensityScale;
	Float						m_stampScaleOrigin;
	Float						m_filterSize;
	Bool						m_hasNonZeroValues;

	Float						m_probability;				// Currently set in ProcessingPainting, but should really be done when slider changes.

	String						m_heightmapPath;

	Uint32						m_stampDataFlags;			// ETerrainEditStampDataFlag. What type(s) of data can be stamped.

	Uint16*						m_stampHeightMap;
	TControlMapType*			m_stampControlMap;
	TColorMapType*				m_stampColorMap;
	Uint32						m_stampColorTexelsPerEdge;

	Vector2						m_stampSourcePosition;

	Bool						m_limitsAvailable;
	TOptional< Float >			m_lowLimit;
	TOptional< Float >			m_highLimit;

	//////////////////////////////////////////////////////////////////////////
	// methods	
	CTerrainEditCursor();
	~CTerrainEditCursor();

	RED_INLINE Bool			Validate() const					{ return m_texelsPerEdge <= MAX_TEXELS_PER_EDGE; }
	RED_INLINE Float			GetScreenSpaceDeltaX() const		{ return m_currScreenSpacePos.X - m_prevScreenSpacePos.X; }
	RED_INLINE Float			GetScreenSpaceDeltaY() const		{ return m_prevScreenSpacePos.Y - m_currScreenSpacePos.Y; }
	RED_INLINE Float			GetScreenSpaceDistFromStored() const{ return ( m_currScreenSpacePos - m_savedScreenSpacePos ).Mag(); }
	RED_INLINE ECursorMode	GetCursorMode() const				{ return m_mode; }
	RED_INLINE void			SetCursorMode( ECursorMode mode )	{ m_mode = mode; }
	RED_INLINE void			StoreScreenSpacePosition()			{ m_savedScreenSpacePos	= m_currScreenSpacePos; }
	RED_INLINE void			RetrieveScreenSpacePosition()		{ m_prevScreenSpacePos	= m_savedScreenSpacePos; m_currScreenSpacePos = m_savedScreenSpacePos; }
	RED_INLINE Bool			HasNonZeroValues() const			{ return m_hasNonZeroValues && !m_falloffDataNeedsRebuild; }

	void						ReallocFalloff();
	void						BuildFalloff();

	// Track changes to brush size and such. Should be called once per frame.
	void						Tick();

	// Create/rebuild buffers as needed. Should be called before painting.
	void						PrepareBuffers();

	void						SetTexelsPerUnit( Float texelsPerUnit );

	void						SetUseFalloffCurve( Bool useFalloffCurve );
	RED_INLINE Bool			GetUseFalloffCurve() const { return ( m_creationFlags & BT_UseFalloffCurve ) != 0; }

	void						SetControlParams( const STerrainToolControlsParams& params );
	void						UpdateScreenSpaceParams( const CMousePacket& packet );

	void						ClearStamp();
	void						ClearAllData();

	void						LoadStampFromTerrain( CClipMap* terrain, Uint32 flags );
	Bool						LoadStampHeightmap( const String& path );


	// Returns combination of ETerrainEditProcessPaintingBufferFlags
	Uint32						GetPaintingBufferFlags() const;

	String						GetToolName() const;

	//////////////////////////////////////////////////////////////////////////
	// Not used yet
	//void FillSimpleConeBuffer();
	//void FillFromHeightmap( CBitmapTexture* heightmap );
	//void FillFromFalloffCurve( CCurve* curve );
	//////////////////////////////////////////////////////////////////////////

private:
	Vector2						m_savedScreenSpacePos;
};


class CEdTerrainEditTool 
	: public IEditorTool, public IEdIconGridHook, public IEdEventListener, public ISavableToConfig, public wxEvtHandler
{
	DECLARE_ENGINE_CLASS( CEdTerrainEditTool, IEditorTool, 0 );

protected:

	static const Float						PaintSelectionThreshold;

	CWorld*									m_world;
	TDynArray< String >						m_tilePaths;
	SClipmapParameters						m_terrainParameters;
	Rect									m_tilesAffectedByCursor;
	TDynArray< bool >						m_tilesSelection;
	Bool									m_paintingRequest;
	Int32									m_paintingRequestFrames;
	CEdRenderingPanel*						m_viewport;

	wxNotebook*								m_editorTabs;

	wxColour								m_currentColor;
	wxColour								m_colorPresets[ NUM_COLOR_PRESETS ];
	wxPanel*								m_colorPresetButtons[ NUM_COLOR_PRESETS ];
	wxPanel*								m_currentColorButton;
	CEdAdvancedColorPicker*					m_colorPickerWindow;
	
	// brushes tool stuff
	CEdCurveEditor*							m_curveEditor;
	CCurve*									m_falloffCurve;
	wxCheckBox*								m_useFalloffCheckBox;
	wxCheckBox*								m_useMaterialTextureMasking;
	wxSlider*								m_brushProbability;

	CEdCurveEditor*							m_colorCurveEditor;
	CCurve*									m_colorFalloffCurve;
	wxCheckBox*								m_colorUseFalloffCheckBox;

	wxCheckBox*								m_substepPainting;
	wxCheckBox*								m_drawBrushAsOverlay;
	wxCheckBox*								m_updateShadowsCheckBox;
	wxCheckBox*								m_saveStrokesCheckBox;

	CEdSpinSliderControl					m_intensityControl;
	CEdSpinSliderControl					m_outsideRadiusControl;
	CEdSpinSliderControl					m_heightControl;
	CEdSpinSliderControl					m_slopeAngleControl;
	CEdSpinSliderControl					m_slopeControl;
	CEdSpinSliderControl					m_slopeOffsetControl;
	CEdSpinSliderControl					m_stampAngleControl;
	CEdSpinSliderControl					m_stampScaleOriginControl;
	CEdSpinSliderControl					m_filterSizeControl;
	wxBitmapButton*							m_heightmapButton;

	CEdSpinSliderControl					m_materialRadiusControl;

	CEdSpinSliderControl					m_colorRadiusControl;
	CEdSpinSliderControl					m_colorIntensityControl;

	wxFrame*								m_dialog;
	wxDialog*								m_InputDialog;
	wxDialog*								m_presetsConfigurationDlg;
	//

	CTerrainEditCursor						m_cursor;

	wxCheckListBox*							m_LayerList;
	Int32									m_LayerCounter;

	wxChoice*								m_FalloffChoice;
	Int32									m_FallofPresetCounter;

	wxListBox*								m_SelectorList;
	Int32									m_SelectorCounter;

	Int32									m_InputDialogParentID;

	wxButton*								m_createButton;
	wxChoice*								m_numTilesPerEdgeChoice;
	wxTextCtrl*								m_sizeText;
	wxTextCtrl*								m_minHeightText;
	wxTextCtrl*								m_maxHeightText;
	wxChoice*								m_lodConfigChoice;
	wxTextCtrl*								m_summary;

	wxTextCtrl*								m_currentMaterialText;
	wxSpinButton*							m_colOffsetSpinButton;
	wxSpinButton*							m_rowOffsetSpinButton;

	wxTextCtrl*								m_currentDiffuseText;
	wxTextCtrl*								m_textureIndexText;
	
	wxSlider*								m_paintProbabilitySlider;

	wxToolBar*								m_brushShapesToolbar;
	wxToggleButton*							m_toolButtons[ TT_Max ];

	STerrainToolControlsParams				m_sessionToolParams[ TT_Max ];

	wxPanel*								m_textureArrayGridPanel;
	CEdTextureArrayGrid*					m_textureArrayGrid;

	struct {
		Int32								selectedHorizontalTexture;
		Int32								selectedVerticalTexture;
		ETerrainPaintSlopeThresholdAction	slopeThresholdAction;
		Uint32								slopeThresholdIndex;
		Uint32								verticalUVMult;
		Int32								probability;
		Bool								verticalUVScaleMask;
		Bool								slopeThresholdMask;
		Bool								horizontalMask;
		Bool								verticalMask;

		Bool								lowLimitMask;
		Bool								highLimitMask;
		Float								lowLimit;
		Float								highLimit;

	}										m_presetsData[6];

	Int32									m_selectedHorizontalTexture;
	Int32									m_selectedVerticalTexture;
	Int32									m_lastSelectedTexture;

	wxPanel*								m_textureParametersPanel;

	wxCheckBox*								m_verticalUVScaleMask;
	wxSpinCtrl*								m_uvMultVerticalSpin;
	Uint32									m_verticalUVMult;

	wxCheckBox*								m_slopeThresholdMask;
	wxChoice*								m_slopeThresholdActionChoice;
	wxSpinCtrl*								m_paintSlopeThresholdSpin;
	ETerrainPaintSlopeThresholdAction		m_slopeThresholdAction;
	Uint32									m_slopeThresholdIndex;

	wxCheckBox*								m_horizontalMask;
	wxCheckBox*								m_verticalMask;

	wxCheckBox*								m_useHorizontalTextureMask;
	wxCheckBox*								m_useVerticalTextureMask;
	wxCheckBox*								m_invertHorizontalTextureMask;
	wxCheckBox*								m_invertVerticalTextureMask;
	wxTextCtrl*								m_horizontalTextureMask;
	wxTextCtrl*								m_verticalTextureMask;
	Uint32									m_horizontalTextureMaskIndex;
	Uint32									m_verticalTextureMaskIndex;


	wxTextCtrl*								m_stampSourceLocation;
	wxTextCtrl*								m_stampOffsetX;
	wxTextCtrl*								m_stampOffsetY;
	wxButton*								m_stampPaste;

	wxHyperlinkCtrl*						m_grassBrushLink;

	TDynArray< wxRadioButton* >				m_presetsRadioBtns;
	TDynArray< wxCheckBox* >				m_presetCheckBoxes;
	Uint32									m_presetIndex;

private:
	Bool									m_anyTilesAffected;
	Bool									m_isStarted;

	EToolType								m_activeToolShapePage;				// Which tool was active on the Shape page.
	Bool									m_paintTextureActive;				// Whether texture paint was active on the materials page.
	Bool									m_paintColorActive;					// Whether color paint was active on the colors page.
	EToolType								m_activeToolCollisionPage;			// Which tool was active on the Collision page.

	Bool									m_paintSelectionActivated;
	Int32									m_textureMaterialParamsSliderRange;
	Int32									m_textureMaterialParamsSliderToTBoxRange;

	Bool									m_needToUpdateCollisionOverlay;

	CTerrainWorkingBufferManager< STerrainWorkingBufferHeight >	m_heightWorkingBuffers;

public:
	CEdTerrainEditTool();
	~CEdTerrainEditTool();

	void SetBrushThumbnail( void );

	void AddNewLayer( wxString layerName );
	void RemoveLayer( void );
	void MergeLayers( void );
	
	void SaveFalloffPreset( wxString presetName );
	void RemoveFalloffPreset( void );

	void AddNewSelector( wxString selectorName );
	void RemoveSelector( void );

	void ToggleUseFalloff(wxCommandEvent & event);

	void OnClose( wxCloseEvent& event );

	void OnFalloffPreset(wxCommandEvent& event);
	void OnModifyLayers(wxCommandEvent& event);
	void OnModifySelectors(wxCommandEvent& event);

	void OnSave( wxCommandEvent& event );
	void OnSelectAll( wxCommandEvent& event );
	
	void OnToolButtonClicked(wxCommandEvent& event);
	void OnToolButtonEnter(wxCommandEvent& event);

	void OnAddMaterial(wxCommandEvent& event);

	void OnChangeHeightmap( wxCommandEvent& event );
	void OnHeightmapButtonSized( wxSizeEvent& event );

	void OnInputEntry(wxCommandEvent& event);
	
	void OnApplySetup( wxCommandEvent& event );
	void OnSetupChanged( wxCommandEvent& event );
	void OnImportButtonHit( wxCommandEvent& event );
	void OnDebugImportPuget( wxCommandEvent& event );
	void OnCreateTerrain( wxCommandEvent& event );
	void OnSetMaterial( wxCommandEvent& event );
	void OnProbabilityChanged( wxCommandEvent& event );
	void OnMaskCheckboxChange( wxCommandEvent& event );
	//void OnApplyOffsetButtonHit( wxCommandEvent& event );
	void OnRowSpinUp( wxSpinEvent& event );
	void OnRowSpinDown( wxSpinEvent& event );
	void OnColSpinLeft( wxSpinEvent& event );
	void OnColSpinRight( wxSpinEvent& event );

	void OnUpdateElevationFromSlider( wxCommandEvent& event );
	void OnUpdateElevationFromSpin( wxCommandEvent& event );
	void OnUpdateRadiusFromSlider( wxCommandEvent& event );
	void OnUpdateRadiusFromSpin( wxCommandEvent& event );

	void OnTextureParameterSliderUpdate( wxCommandEvent& event );
	void OnTextureParameterTextBoxUpdate( wxCommandEvent& event );
	void OnGrassBrushHyperlinkClicked( wxHyperlinkEvent& event ); 
	void OnSetGrassBrushClicked( wxCommandEvent& event );
	void OnRemoveGrassBrushClicked( wxCommandEvent& event );
	
	void OnSlopeThresholdSpin( wxCommandEvent& event );
	void OnSlopeThresholdAction( wxCommandEvent& event );
	void OnUVMultVerticalSpin( wxCommandEvent& event );

	void OnExportTiles( wxCommandEvent& event );
	void OnImportTiles( wxCommandEvent& event );
	void OnResetSelection( wxCommandEvent& event );

	void OnPageChanging( wxCommandEvent& event );
	void OnPageChanged( wxCommandEvent& event );

	void OnFalloffCurveChanged( wxCommandEvent& event );

	void SelectTool( EToolType toolType );

	virtual String GetCaption() const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();
	
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );
	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportTick( IViewport* view, Float timeDelta );
	virtual Bool OnViewportKillFocus( IViewport* view );
	virtual Bool OnViewportSetFocus( IViewport* view );

	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	// ------------------------------------------------------------------------
	// IEdEventListener implementation
	// ------------------------------------------------------------------------
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	Bool ValidateDirectory( const wxDir& directory, const String& pattern, Uint32& tileSize, Uint32& tilesPerSide, String& warningMessage );
	void RefillTilePathsArray( const wxDir& directory, const String& pattern );

	void UpdateCursor( const CMousePacket& packet );

	// Draw a line, optionally using a different appearance where the line goes behind scene geometry. Behind geometry, it is drawn
	// as a faded blue. If asOverlay is true, then just the given color is used, and there is no difference when the line goes behind.
	void DrawBrushLine( CRenderFrame* frame, const Vector& p0, const Vector& p1, const Color& color, Bool asOverlay, Bool clipToZLimit = false );

	void RenderBrush( IViewport* view, CRenderFrame* frame );
	void RenderTilesAffectedByCursor( IViewport* view, CRenderFrame* frame );
	void RenderSelectedTiles( IViewport* view, CRenderFrame* frame );
	void RenderSelectedTile( IViewport* view, CRenderFrame* frame, Uint32 x, Uint32 y );
	void RenderBoxWall( IViewport* view, CRenderFrame* frame, const Vector& topLeft, const Vector& bottomRight );
	Bool DetermineTilesAffectedByCursor( const CTerrainEditCursor& cursor );
	void RenderSlopeRotationGizmo( IViewport* view, CRenderFrame* frame );
	void RenderSlopeSlopeGizmo( IViewport* view, CRenderFrame* frame );
	void RenderSlopeOffsetGizmo( IViewport* view, CRenderFrame* frame );
	void RenderStampRotationGizmo( IViewport* view, CRenderFrame* frame );

	// Calculate slope brush reference point for the current cursor position. Reference point XY is the nearest texel
	// to the cursor. Z is the height, in meters, of that texel.
	Vector GetSlopeReferencePoint();

	enum SelectMode
	{
		SM_Select, SM_Deselect, SM_Toggle
	};

	void SelectTile( IViewport* view, const Vector & position, SelectMode mode );
	void SelectAllTiles();
	Bool ProcessCursorClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	void ProcessMouseMove( const CMousePacket& packet );

	void StartPainting( IViewport* view );
	void StopPainting( IViewport* view );
	Bool ProcessPainting( Float timeDelta );
	void ProcessPaintingInPlace( Uint32 numTimes, Uint32 bufferFlags, const Rect& brushRectInClipmap, const Rect& brushRectInColorMap, Int32 brushExpansion, Int32 brushColorExpansion, TDynArray< SClipmapHMQueryResult >& clipmapParts );
	void ProcessPaintingWithBuffer( Uint32 numTimes, Uint32 bufferFlags, const Rect& brushRectInClipmap, const Rect& brushRectInColorMap, Int32 brushExpansion, Int32 brushColorExpansion, TDynArray< SClipmapHMQueryResult >& clipmapParts );
	void SetupPaintingBrush( CTerrainBrush& brush );
	void ProcessPaintingStamp();

	void SetStampVisible( Bool visible );
	void OnStampOffsetChanged( wxCommandEvent& event );
	void OnStampPasteClicked( wxCommandEvent& event );
	void SetStampOffsetControlsEnabled( Bool enabled );

	void OnRefreshAutoCollisionCurrent( wxCommandEvent& event );
	void OnRefreshAutoCollision( wxCommandEvent& event );
	void RefreshAutoCollision( Bool currentLayers );
	void ProcessPaintingCollision();
	void UpdateTerrainCollisionMask();

	Int32 GetClipmapResolutionIndexForCurrentTerrain();
	Int32 GetClipmapLODConfigIndexForCurrentTerrain();
	Bool IsResolutionSupported( Int32 resolution );
	Bool IsTileResolutionSupported( Int32 resolution );
	Int32 GetClipmapResolutionIndexForResolution( Int32 res );
	Int32 GetFirstClipmapConfigIndexForTileResolution( Int32 tileRes );
	Int32 GetClipmapConfigIndex( Int32 clipSize, Int32 tileRes );
	void UpdateBrushControlRanges();
	void EnableDisableShapeBrushControls( EToolType toolType );
	void UpdateTerrainParamsControls();
	void InitializeSelectionArray();
	void GenerateTerrainParametersSummary( const SClipmapParameters& params, String& summary );
	void UpdateMaterialParameter( Uint32 index, Float normalizedValue );

	void SaveAdditionalInfo( Uint32 minX, Uint32 minY, Uint32 maxX, Uint32 maxY, const TDynArray< String >& exportedHMFiles, const TDynArray< String > exportedCMFiles, const String& dirPath, const String& filename );
	void ReadAdditionalInfo( const String& csvPath, Uint32& offsetX, Uint32& offsetY, Uint32& sizeX, Uint32& sizeY, TDynArray< String >& tilesToImport );

	Float	HeightOffsetToTexels( Float height );
	Uint16	HeightToTexels( Float height );
	Float	TexelsToHeight( Uint16 texels );
	String	GetBareFilename( const String& filenameWithExtension );

	wxString GetToolTooltip( EToolType toolType );

	// ISavableToConfig
	virtual void				SaveOptionsToConfig();
	virtual void				LoadOptionsFromConfig();
	virtual void				SaveSession( CConfigurationManager &config );
	virtual void				RestoreSession( CConfigurationManager &config );
	STerrainToolControlsParams	GetCurrControlParams() const;
	void						SetControlParams( STerrainToolControlsParams &params );

	Bool HandleSlopeBrushInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	Bool HandleStampBrushInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
		 
	void HandleMouseZBrushGeneral( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	void HandleMouseZBrushSlope( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	void HandleMouseZBrushStamp( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	void HandleMouseZBrushPaint( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	void HandleMouseZBrushPaintTex( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	void HandleMouseZBrushSmoothMelt( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	void SetPreset( Int32 slot );
	void UpdatePresetData();
	void UpdateMaterialTexturesGrid();
	void UpdateTextureParameters();

	//////////////////////////////////////////////////////////////////////////
	// Color presets

	// Given a color panel, figure out which preset it belongs to. NUM_COLOR_PRESETS if it isn't for a preset.
	Uint32 GetColorPresetIndex( wxWindowBase* colorButton ) const;
	void OnPaintColorPanel( wxPaintEvent& event );
	void SetColorPreset( Uint32 whichPreset, const wxColour& color );
	void SetCurrentColor( const wxColour& color );

	// Click current color -> edit it
	void OnCurrentColorClicked( wxMouseEvent& event );
	
	// Click color preset -> use it
	void OnColorPresetClicked( wxMouseEvent& event );

	// Right-click color preset -> a few options
	void OnColorPresetRightClicked( wxMouseEvent& event );

	// Context menu options from right-clicking preset
	void OnEditColorPreset( wxCommandEvent& event );
	void OnSetColorPresetFromCurrent( wxCommandEvent& event );
	void OnClearColorPreset( wxCommandEvent& event );
	void OnClearAllColorPresets( wxCommandEvent& event );

	void OpenColorPicker( wxWindowBase* colorButton );
	void OnColorPickerChanged( wxCommandEvent& event );
	void OnColorPickerClosed( wxCloseEvent& event );

	void OnSaveColorPresets( wxCommandEvent& event );
	void OnLoadColorPresets( wxCommandEvent& event );

	void OnLowLimitChoice( wxCommandEvent& event );
	void OnHighLimitChoice( wxCommandEvent& event );
	void OnLowLimitText( wxCommandEvent& event );
	void OnHighLimitText( wxCommandEvent& event );
	void OnLimitPresetSelected( wxCommandEvent& event );
	void OnConfigurePresetsBtnClicked( wxCommandEvent& event );

	void OnTextureUsageTool( wxCommandEvent& event );

	void OnUpdateUI( wxUpdateUIEvent& event );

	//////////////////////////////////////////////////////////////////////////


	void GetImportConfiguration( Bool& importHeightmap, Bool& importColormap );
	void GetExportConfiguration( Bool& exportHeightmap, Bool& exportColormap );
	void UpdatePresetsActivity();

protected:
	// IEdIconGridHook
	virtual void OnIconGridSelectionChange( class CEdIconGrid* grid, Bool primary );
};

BEGIN_CLASS_RTTI( CEdTerrainEditTool );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
