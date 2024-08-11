/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_EDITOR_TYPE_REGISTRY
#define _H_EDITOR_TYPE_REGISTRY

// this file contains list of all types in 'core' project

// Not defined when included in editor/build.h, but defined when included in editorClasses.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );


REGISTER_RTTI_CLASS( IEditorTool );
REGISTER_RTTI_CLASS( CWXThumbnailImageLoader );
REGISTER_RTTI_CLASS( CEdVertexEdit );
REGISTER_RTTI_CLASS( CEdStripeEdit );
REGISTER_RTTI_CLASS( CEdPivotEdit );
REGISTER_RTTI_CLASS( CEdSpriteEdit );

// terrain tool V2.0
REGISTER_RTTI_CLASS( CEdTerrainEditTool );

REGISTER_RTTI_CLASS( CBaseTerrainCursor );
REGISTER_RTTI_CLASS( CTerrainCursorNull );
REGISTER_RTTI_CLASS( CTerrainCursorRiseLower );
REGISTER_RTTI_CLASS( CTerrainCursorFlatten );
REGISTER_RTTI_CLASS( CTerrainCursorSlope );
REGISTER_RTTI_CLASS( CTerrainCursorSmooth );
REGISTER_RTTI_CLASS( CTerrainCursorStamp );


REGISTER_RTTI_CLASS( CEdBrushFaceEdit );
REGISTER_RTTI_CLASS( CBrushBuilderCube );
REGISTER_RTTI_CLASS( CBehaviorDebugVisualizer );
REGISTER_RTTI_CLASS( CEdRulerTool );
REGISTER_RTTI_CLASS( CEdSceneEdit );
REGISTER_RTTI_CLASS( CEdSceneExporterTool );
REGISTER_RTTI_CLASS( CEdSeedTool );
REGISTER_RTTI_CLASS( CEdGardenerTool );
REGISTER_RTTI_CLASS( CEdGardenerStamp );
REGISTER_RTTI_CLASS( CEdStaticCameraTool );
REGISTER_RTTI_CLASS( CEdPlayAnimTool );
REGISTER_RTTI_CLASS( CEdBgViewerTool );
REGISTER_RTTI_CLASS( CEdBezierEdit );
REGISTER_RTTI_CLASS( CEdCameraPreviewTool );
REGISTER_RTTI_CLASS( CEdPathlib );
REGISTER_RTTI_CLASS( CEdDestructionResetTool );
REGISTER_RTTI_CLASS( CEntityHandleSelector );
REGISTER_RTTI_CLASS( CEdEncounterEditTool );
REGISTER_RTTI_CLASS( CEdMergedGeometryTool );
#ifdef USE_UMBRA
#ifndef NO_UMBRA_DATA_GENERATION
REGISTER_RTTI_CLASS( CEdUmbraTool );
#endif // NO_UMBRA_DATA_GENERATION
#endif // USE_UMBRA

REGISTER_RTTI_CLASS( CEdUndoGroupMarker );

REGISTER_RTTI_CLASS( CBehaviorGraphAnimationTrajDrawNode );
REGISTER_RTTI_CLASS( CBehaviorGraphRootMotionNode );
#ifdef USE_HAVOK_ANIMATION
REGISTER_RTTI_CLASS( CBehaviorGraphIkFootNode );
#endif
REGISTER_RTTI_CLASS( CEdUndoManager );
REGISTER_RTTI_CLASS( IUndoStep );
REGISTER_RTTI_CLASS( CUndoCreateDestroy );
REGISTER_RTTI_CLASS( CUndoSelection );
REGISTER_RTTI_CLASS( CUndoProperty );
REGISTER_RTTI_CLASS( CUndoTransform );
REGISTER_RTTI_CLASS( CUndoToolSwitch );
REGISTER_RTTI_CLASS( CUndoTerrain );
REGISTER_RTTI_CLASS( CUndoVertexCreateDestroy );
REGISTER_RTTI_CLASS( CEdVegetationEditTool );
REGISTER_RTTI_CLASS( CUndoCurveControlPointsAddOrDelete );
REGISTER_RTTI_CLASS( CCurveEditorTool );

REGISTER_RTTI_CLASS( CEdMeshLODProperties );
REGISTER_RTTI_CLASS( CMeshPreviewComponent );
REGISTER_RTTI_CLASS( CPreviewHelperComponent );

REGISTER_RTTI_CLASS( CMeshTypePreviewComponent );
REGISTER_RTTI_CLASS( CMeshTypePreviewMeshComponent );
REGISTER_RTTI_CLASS( CMeshTypePreviewDestructionComponent );
REGISTER_RTTI_CLASS( CMeshTypePreviewClothComponent );

REGISTER_RTTI_CLASS( CCopiedData );

REGISTER_RTTI_CLASS( CPivotEditorEntity )
REGISTER_RTTI_CLASS( CPivotComponent )

REGISTER_RTTI_CLASS( CEntityConverter );
REGISTER_RTTI_CLASS( CNewNpcToBgNpcConverter );

REGISTER_RTTI_CLASS( CRewardResourceManager );
REGISTER_RTTI_STRUCT( SEditorUserConfig );

REGISTER_RTTI_CLASS( CJournalTreeRoot );

REGISTER_RTTI_CLASS( CEdCharacterResourceContainer );

REGISTER_RTTI_CLASS( CStoryScenePreviewPlayer );
REGISTER_RTTI_CLASS( CEdSceneActorsProvider );
REGISTER_RTTI_CLASS( CEdSceneHelperEntity );
REGISTER_RTTI_CLASS( CEdSceneHelperEntityForLightEvent );
REGISTER_RTTI_CLASS( CEdSceneHelperEntityForDurationLookat );
REGISTER_RTTI_CLASS( CEdSceneHelperComponent );

REGISTER_RTTI_CLASS( CUndoGraphBlockExistance );
REGISTER_RTTI_CLASS( CUndoGraphSocketSnaphot );
REGISTER_RTTI_CLASS( CUndoGraphConnectionExistance );
REGISTER_RTTI_CLASS( CUndoGraphBlockLayout );
REGISTER_RTTI_CLASS( CUndoGraphBlockMove );
REGISTER_RTTI_CLASS( CUndoGraphConnectionActivity );
REGISTER_RTTI_CLASS( CUndoGraphSocketVisibility );

REGISTER_RTTI_CLASS( CUndoQuestGraphBlockIO )
REGISTER_RTTI_CLASS( CUndoQuestGraphRandomBlockOutput )
REGISTER_RTTI_CLASS( CUndoQuestGraphVariedInputBlock )
REGISTER_RTTI_CLASS( CUndoQuestGraphPushPop )

REGISTER_RTTI_CLASS( CUndoBehaviorGraphSetRoot )
REGISTER_RTTI_CLASS( CUndoBehaviorGraphContainerNodeInput )
REGISTER_RTTI_CLASS( CUndoBehaviorGraphBlendNodeInput )
REGISTER_RTTI_CLASS( CUndoBehaviorGraphRandomNodeInput )
REGISTER_RTTI_CLASS( CUndoBehaviorGraphSwitchNodeInput )
REGISTER_RTTI_CLASS( CUndoBehaviorGraphVariableExistance )
REGISTER_RTTI_CLASS( CUndoBehaviourGraphVariableChange )

REGISTER_RTTI_CLASS( CUndoTimelineItemExistance )
REGISTER_RTTI_CLASS( CUndoTimelineItemLayout )
REGISTER_RTTI_CLASS( CUndoTimelineTrackExistance )
REGISTER_RTTI_CLASS( CUndoTimelineTrackRename )

REGISTER_RTTI_CLASS( CUndoAnimEventsAnimChange )

REGISTER_RTTI_CLASS( CUndoDialogGraphBlockExistance )
REGISTER_RTTI_CLASS( CUndoDialogSetExistance )
REGISTER_RTTI_CLASS( CUndoDialogSlotExistance )
REGISTER_RTTI_CLASS( CUndoDialogSectionMove )
REGISTER_RTTI_CLASS( CUndoDialogTextChange )
REGISTER_RTTI_CLASS( CUndoDialogElementExistance )
REGISTER_RTTI_CLASS( CUndoDialogChoiceLineExistance )

REGISTER_RTTI_STRUCT( CDialogEventGeneratorConfig )
REGISTER_RTTI_STRUCT( SStorySceneActorAnimationGenData )

REGISTER_RTTI_CLASS( CForceFieldEntity )

REGISTER_RTTI_CLASS( CEdSpawntreeNodeProxy )

// Wizards
REGISTER_RTTI_CLASS( CWizardNode );
REGISTER_RTTI_CLASS( CWizardCNameQuestionNode );
REGISTER_RTTI_CLASS( CWizardOption);
REGISTER_RTTI_CLASS( CWizardOptionData );

REGISTER_RTTI_CLASS( CAiPresetWizardData );
REGISTER_RTTI_CLASS( CCustomParamWizardData );

// template Insertion classes
REGISTER_RTTI_CLASS( CTemplateInsertion );
REGISTER_RTTI_CLASS( CSwarmTemplateInsertion );

REGISTER_RTTI_CLASS( CStorySceneEventPoseKeyPresetData );

REGISTER_RTTI_CLASS( CDataPresets );
REGISTER_RTTI_CLASS( CStoryScenePresets );
REGISTER_RTTI_CLASS( CDataPresetsFactory );

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NOT_REGISTERED
#endif

#endif
