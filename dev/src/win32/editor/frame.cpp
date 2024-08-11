/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "frame.h"

#include "../../common/core/configVarSystem.h"
#include "../../common/core/resourceDefManager.h"
#include "../../common/core/gameConfiguration.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/feedback.h"

#include "../../common/engine/apexClothResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/gameResource.h"
#include "../../common/engine/localizedContent.h"
#include "../../common/engine/TestFramework.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/physics/physicsDebugger.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/reviewSystem.h"
#include "../../common/physics/compiledCollision.h"
#include "../../common/engine/minimapGenerator.h"
#include "../../common/engine/umbraScene.h"
#include "../../common/engine/soundFileLoader.h"
#include "../../common/engine/soundSystem.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/mimicFac.h"
#include "../../common/engine/gameSaveManager.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/environmentComponentArea.h"
#include "../../common/engine/rigidMeshComponent.h"
#include "../../common/engine/worldIterators.h"
#include "../../common/engine/renderProxy.h"

#include "../../common/game/actorsManager.h"
#include "../../common/game/aiParameters.h"
#include "../../common/game/behTreeTask.h"
#include "../../common/game/encounter.h"
#include "../../common/game/explorationFinder.h"
#include "../../common/game/inventoryEditor.h"
#include "../../common/game/journalPath.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questScopeBlock.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/dlcManager.h"

#include "../../games/r4/commonMapManager.h"

#include "../../win32/platform/inputDeviceManagerWin32.h"
#include "../platform/userProfileManagerWindows.h"

#include "googleanalytics.h"
#include "nodeTransformManager.h"
#include "entityEditor.h"
#include "entityLootSetupDialog.h"
#include "aboutDlg.h"
#include "whatsNewDlg.h"
#include "refreshVoicesDlg.h"
#include "xmlErrorsDisplayer.h"
#include "assetBrowser.h"
#include "sceneExplorer.h"
#include "toolsPanel.h"
#include "behaviorEditor.h"
#include "filterPanel.h"
#include "Shellapi.h"
#include "commonDlgs.h"
#include "callbackData.h"
#include "shortcutsEditor.h"
#include "meshEditor.h"
#include "materialEditor.h"
#include "textureViewer.h"
#include "dialogEditor.h"
#include "undoStep.h"
#include "undoManager.h"
#include "animBrowser.h"
#include "spawnSetDebugger.h"
#include "communityDebuggerEditor.h"
#include "editorExternalResources.h"
#include "factsDBEditor.h"
#include "maraudersMap.h"
#include "localizedStringsEditor.h"
#include "localizationToolsFrame.h"
#include "resourceSorter.h"
#include "questsDebugger.h"
#include "questScenesDebugger.h"
#include "questExecutionLog.h"
#include "questThreadsDebugger.h"
#include "questControlledNPCsLog.h"
#include "questEditor.h"
#include "behaviorEditor.h"
#include "behaviorDebugger.h"
#include "tagMiniEditor.h"
#include "brushPanel.h"
#include "worldProperties.h"
#include "attitudeEditor.h"
#include "lootEditor.h"
#include "moveEntities.h"
#include "meshColoringScheme.h"
#include "entitiesBrowser.h"
#include "cutsceneEditor.h"
#include "animationReporter.h"
#include "editorAnimCacheGen.h"
#include "previewCooker.h"
#include "importTextureSourceData.h"
#include "lipsyncPreview.h"
#include "curvePropertyEditor.h"
#include "mapPinValidatorWindow.h"
#include "journalEditor.h"
#include "worldEnvironmentEditor.h"
#include "animFriend.h"
#include "minimapGeneratorTool.h"
#include "screenshotEditor.h"
#include "animationListExporter.h"
#include "exporters\storySceneValidator.h"
#include "interactiveDialogEditor.h"
#include "dataErrorReporter.h"
#include "crowdDebugger.h"
#include "worldSceneDebugger.h"
#include "undoHistoryFrame.h"
#include "autoScriptWindow.h"
#include "characterDBEditor.h"
#include "updateFocusEntityTemplatesDlg.h"
#include "utils.h"
#include "engineConfigProperties.h"
#include "popupNotification.h"
#include "rewardEditor.h"
#include "dlcListDlg.h"
#include "sceneRecorder.h"

#include <wx/grid.h>
#include <wx/display.h>
#include "inGameConfigurationDlg.h"
#include "errorsListDlg.h"
#include "../../common/engine/layersEntityCompatibilityChecker.h"
#include "../../common/core/contentManager.h"
#include "../../common/engine/physicsDestructionResource.h"


#define ID_FRIST_PROFILER_SHOW_ALL		1000
#define ID_FRIST_PROFILER_HIDE_ALL		1001
#define ID_FRIST_PROFILER_SHOW_COUNTER	1002
#define ID_RESAVE_ALL_FILES				3000
#define ID_RECENT_CHOSEN				4000

#define ID_VIEW_TOOLBAR_MAIN			 7001
#define ID_VIEW_TOOLBAR_WIDGETS			 7002
#define ID_VIEW_VIEWPORT_FLOAT			 7003
#define ID_VIEW_TOOLBAR_WIDGETP4		 7004

#define ID_VIEW_FIELDOFVIEW				 7010
#define ID_VIEW_OVERLAYVERTEXSPRITES	 7011
#define ID_VIEW_ENTITIES_AROUND_CAMERA	 7012
#define ID_VIEW_TOGGLE_DISPLAY_MODE		 7013
#define ID_VIEW_VIEWPORT_CACHETASPECTRATIO	7014

#define ID_VIEW_NOTIFICATION_CENTER			7100
#define ID_VIEW_NOTIFICATION_TOP_LEFT		7101
#define ID_VIEW_NOTIFICATION_TOP_RIGHT		7102
#define ID_VIEW_NOTIFICATION_BOTTOM_LEFT	7103
#define ID_VIEW_NOTIFICATION_BOTTOM_RIGHT	7104

#define ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_9	7105
#define ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_10 7106
#define ID_VIEW_VIEWPORT_CACHETASPECTRATIO_4_3 7107
#define ID_VIEW_VIEWPORT_CACHETASPECTRATIO_21_9 7108
#define ID_VIEW_VIEWPORT_CACHETASPECTRATIO_NONE 7109

#define ID_VIEW_SHOW_ALL		8000
#define ID_VIEW_HIDE_ALL		8001
#define ID_VIEW_FLOAT_ALL		8002
#define ID_VIEW_DEFLOAT_ALL		8003

#define ID_VIEW_SCENE			9000
#define ID_VIEW_TOOLS			9020
#define ID_VIEW_PATHS			9050
#define ID_VIEW_PROPERTIES		9060
#define ID_VIEW_ENTITIES		9070
#define ID_VIEW_BRUSHES			9080
#define ID_VIEW_WORLD			9090

#define ID_CONNECT_VDB			9100
#define ID_CONNECT_VDB_MAX		9200

namespace
{
	const wxString GEntityBrowserName( wxT("Entities Browser") );
}


CEdFrame* wxTheFrame = NULL;

BEGIN_EVENT_TABLE(CEdFrame, wxFrame)
	EVT_SIZE( CEdFrame::OnSize )
	EVT_MOVE( CEdFrame::OnMove )
	EVT_CLOSE( CEdFrame::OnCloseWindow )
	EVT_ACTIVATE( CEdFrame::OnActivateWindow )
	EVT_ERASE_BACKGROUND( CEdFrame::OnEraseBackground )

	EVT_MENU( XRCID("fileExitLava"), CEdFrame::OnExit )
	EVT_MENU( XRCID("helpAbout"), CEdFrame::OnAbout )
	EVT_MENU( XRCID("helpWhatsNew"), CEdFrame::OnWhatsNewForced )
	EVT_TOOL( XRCID("assetBrowser"), CEdFrame::OnAssetBrowser )

	EVT_MENU( XRCID("fileNewWorld"), CEdFrame::OnNewWorld )
	EVT_MENU( XRCID("fileOpenWorld"), CEdFrame::OnOpenWorld )
	EVT_MENU( XRCID("fileSaveWorld"), CEdFrame::OnSaveWorld )
	EVT_MENU( XRCID("fileSaveWorldModifiedOnly"), CEdFrame::OnSaveWorldModifiedOnly )
    EVT_MENU( XRCID("fileCloseWorld"), CEdFrame::OnCloseWorld )
	EVT_MENU( XRCID("fileRestoreWorld"), CEdFrame::OnRestoreWorld )

	EVT_MENU( XRCID("reloadSoundbanks"), CEdFrame::OnReloadSoundbanks )

	EVT_MENU( XRCID("GenerateApexDestructionStatistics"), CEdFrame::OnApexDestructionStatistics )
	EVT_MENU( XRCID("GenerateApexClothStatistics"), CEdFrame::OnApexClothStatistics )
	EVT_MENU( XRCID("GeneratePhysxGeometryStatisticsFromCollisionCache"), CEdFrame::OnGeneratePhysxGeometryStatisticsFromCollisionCache )
	EVT_MENU( XRCID("FillCollisionCacheWithEveryMeshInLocalDepotCollision"), CEdFrame::OnFillCollisionCacheWithEveryMeshInLocalDepotCollision )
	
	EVT_MENU( XRCID("clientSettings"), CEdFrame::OnClientSettings )
	EVT_MENU( XRCID("departmentTags"), CEdFrame::OnDepartmentTags )
	EVT_MENU( XRCID("fileRestartWithGame"), CEdFrame::OnRestartWithGame )

	EVT_MENU( XRCID("playGame"), CEdFrame::OnPlayGame )
	EVT_MENU( XRCID("playGameFast"), CEdFrame::OnPlayGameFast )
	EVT_MENU( XRCID("playGameFromSave"), CEdFrame::OnPlayGameFromSave )
	EVT_MENU( XRCID("editorStreaming"), CEdFrame::OnEditorStreaming )

	EVT_MENU( XRCID("regenerateRenderProxies"), CEdFrame::OnRegenerateRenderProxies )

	EVT_CHOICE( XRCID( "displayModifier" ), CEdFrame::OnEnvironmentModifierChanged )

	EVT_MENU( XRCID("editUndo"), CEdFrame::OnUndo )
    EVT_MENU( XRCID("editRedo"), CEdFrame::OnRedo )
	EVT_MENU( XRCID("undoHistory"), CEdFrame::OnUndoHistory )
	EVT_MENU( XRCID("undoTrackSelection"), CEdFrame::OnUndoTrackSelection )

	EVT_MENU( XRCID("editCopy"), CEdFrame::OnCopy )
	EVT_MENU( XRCID("editCut"), CEdFrame::OnCut )
	EVT_MENU( XRCID("editPaste"), CEdFrame::OnPaste )
	EVT_MENU( XRCID("editDelete"), CEdFrame::OnDelete )

	EVT_MENU( XRCID("editCopyCameraView"), CEdFrame::OnCopyCameraView )
	EVT_MENU( XRCID("editPasteCameraView"), CEdFrame::OnPasteCameraView )
	EVT_MENU( XRCID("editPasteQuestView"), CEdFrame::OnPasteQuestView )

	EVT_MENU( XRCID("editSelectAll"), CEdFrame::OnSelectAll )
	EVT_MENU( XRCID("editUnselectAll"), CEdFrame::OnUnselectAll )
	EVT_MENU( XRCID("editInvertSelection"), CEdFrame::OnInvertSelection )
	EVT_MENU( XRCID("editSelectByTheSameEntityTemplate"), CEdFrame::OnSelectByTheSameEntityTemplate )
	EVT_MENU( XRCID("editSelectByTheSameTag"), CEdFrame::OnSelectByTheSameTag )

	EVT_MENU( XRCID("editSelectionOnMultiLayer"), CEdFrame::OnSelectionOnMultiLayer )
	EVT_MENU( XRCID("editSelectionOnActiveLayer"), CEdFrame::OnSelectionOnActiveLayer )

	EVT_MENU( XRCID("editCenterOnSelected"), CEdFrame::OnCenterOnSelected )

	EVT_MENU( XRCID("editHideLayersWithSelectedObjects"), CEdFrame::OnHideLayersWithSelectedObjects )
	EVT_MENU( XRCID("editUnhideAllLoadedLayers"), CEdFrame::OnUnhideAllLoadedLayers )

	EVT_MENU( XRCID("disconnectPerforce"), CEdFrame::OnDisconnectPerforce )

	EVT_MENU( XRCID("widgetModePick"), CEdFrame::OnWidgetPick )
	EVT_MENU( XRCID("widgetModeMove"), CEdFrame::OnWidgetMove )
	EVT_MENU( XRCID("widgetModeRotate"), CEdFrame::OnWidgetRotate )
	EVT_MENU( XRCID("widgetModeScale"), CEdFrame::OnWidgetScale )
	EVT_MENU( XRCID("widgetModeChange"), CEdFrame::OnWidgetChange )
	EVT_MENU( XRCID("widgetSpaceWorld"), CEdFrame::OnSpaceWorld )
	EVT_MENU( XRCID("widgetSpaceLocal"), CEdFrame::OnSpaceLocal )
	EVT_MENU( XRCID("widgetSpaceForeign"), CEdFrame::OnSpaceForeign )
	EVT_MENU( XRCID("widgetSpaceChange"), CEdFrame::OnSpaceChange )

	EVT_MENU( XRCID("widgetSnapNone"), CEdFrame::OnSnapNone )
	EVT_MENU( XRCID("widgetSnapTerrainVisual"), CEdFrame::OnSnapTerrainVisual )
	EVT_MENU( XRCID("widgetSnapTerrainPhysical"), CEdFrame::OnSnapTerrainPhysical )
	EVT_MENU( XRCID("widgetSnapCollision"), CEdFrame::OnSnapCollision )

	EVT_MENU( XRCID("widgetSnapPivot"), CEdFrame::OnSnapPivot )
	EVT_MENU( XRCID("widgetSnapBoundingVolume"), CEdFrame::OnSnapBoundingVolume )

	EVT_MENU( XRCID("gridPosition"), CEdFrame::OnPositionGridToggle )
	EVT_COMBOBOX( XRCID("gridPositionCombo"), CEdFrame::OnPositionGridChange )
	EVT_MENU( XRCID("gridLength"), CEdFrame::OnPositionGridLengthToggle )
	EVT_MENU( XRCID("gridRotation"), CEdFrame::OnRotationGridToggle )
	EVT_COMBOBOX( XRCID("gridRotationCombo"), CEdFrame::OnRotationGridChange )

	EVT_MENU( XRCID("toolWorldSceneDebugger"), CEdFrame::OnWorldSceneDebuggerTool )
	EVT_MENU( XRCID("toolMemWalker"), CEdFrame::OnMemWalkerTool )
	EVT_MENU( XRCID("toolCollisionMemUsage"), CEdFrame::OnCollisionMemUsageTool )

	EVT_MENU( XRCID("moveEntitiesTool"), CEdFrame::OnToolMoveEntities )

	EVT_MENU( XRCID("refreshVoices"), CEdFrame::OnRefreshVoices )
	EVT_MENU( XRCID("reloadResourceDefinitions"), CEdFrame::OnReloadResourceDefinitions )
	EVT_MENU( XRCID("reloadGameDefinitions"), CEdFrame::OnReloadGameDefinitions )	
	EVT_MENU( XRCID("reloadScripts"), CEdFrame::OnReloadScripts )

    EVT_MENU( XRCID("outputPanel"), CEdFrame::OnViewChanged )
	EVT_MENU( XRCID("dataErrorReporter"), CEdFrame::OnViewChanged )
	EVT_MENU( XRCID("filterPanel"), CEdFrame::OnViewChanged )
	EVT_MENU( XRCID("inGameConfig"), CEdFrame::OnViewChanged )

	EVT_TOOL( XRCID("widgetStamper"), CEdFrame::OnStamper )
	EVT_TOOL( XRCID("editorVDB"), CEdFrame::OnVDB )
	EVT_TOOL( XRCID("simulate"), CEdFrame::OnSimulate )
	EVT_COMBOBOX( XRCID("snapCombo"), CEdFrame::OnSnapChange )

	EVT_TOOL( XRCID("openWorldEnvironmentEditorTool"), CEdFrame::OnWorldEnvironmentEditor )
	EVT_TOOL( XRCID("minimapGenerator"), CEdFrame::OnMinimapGenerator )
	EVT_TOOL( XRCID("screenshotEditor"), CEdFrame::OnScreenshotEditor )

    EVT_MENU( XRCID("viewShortcutEditor"), CEdFrame::OnOpenShortcutsEditor )

	EVT_MENU( XRCID("viewMeshesNone"), CEdFrame::OnMeshColoringNone )
	EVT_MENU( XRCID("viewMeshesType"), CEdFrame::OnMeshColoringType )
	EVT_MENU( XRCID("viewMeshesCollisionType"), CEdFrame::OnMeshCollisionType )
	EVT_MENU( XRCID("viewMeshesSoundMaterial"), CEdFrame::OnMeshSoundMaterial )
	EVT_MENU( XRCID("viewMeshesSoundOccl"), CEdFrame::OnMeshSoundOccl )
	EVT_MENU( XRCID("viewMeshesShadows"), CEdFrame::OnMeshShadows )
	EVT_MENU( XRCID("viewMeshesChunks"), CEdFrame::OnMeshChunks )
	EVT_MENU( XRCID("viewTexturesDensity"), CEdFrame::OnMeshTextureDensity )
	EVT_MENU( XRCID("viewMeshRenderingLod"), CEdFrame::OnMeshRenderingLod )
	EVT_MENU( XRCID("viewMeshStreamingLod"), CEdFrame::OnMeshStreamingLod )
	EVT_MENU( XRCID("viewMeshesLayerBuildTag"), CEdFrame::OnMeshLayerBuildTag )
	EVT_MENU( XRCID("viewMoveToTerrainLevel"), CEdFrame::OnViewMoveToTerrainLevel )
	EVT_MENU( XRCID("viewLoadLayersAroundCamera"), CEdFrame::OnViewLoadLayersAroundCamera )
	EVT_MENU( XRCID("viewUnloadLayersAroundCamera"), CEdFrame::OnViewUnloadLayersAroundCamera )
	EVT_MENU( XRCID("viewShowFreeSpaceAroundSelection"), CEdFrame::OnViewShowFreeSpaceAroundSelection )
	EVT_MENU( XRCID("viewLockFreeSpaceVisualization"), CEdFrame::OnViewLockFreeSpaceVisualization )
	EVT_MENU( XRCID("toggleDisplayMode"), CEdFrame::OnViewToggleDisplayMode )

	EVT_MENU( XRCID("spawnSetDebugger"), CEdFrame::OnSpawnSetDebugger )
	EVT_MENU( XRCID("questsDebugger"), CEdFrame::OnQuestsDebugger )
	EVT_MENU( XRCID("behaviorDebugger"), CEdFrame::OnBehaviorDebugger )
	EVT_MENU( XRCID("behaviorDebuggerGroup"), CEdFrame::OnBehaviorGroupDebugger )
	EVT_MENU( XRCID("animationFriend"), CEdFrame::OnAnimationFriend )

	EVT_MENU( XRCID("exportGlobalMappins"), CEdFrame::ExportGlobalMappins )
	EVT_MENU( XRCID("exportEntityMappins"), CEdFrame::ExportEntityMappins )
	EVT_MENU( XRCID("exportEntityMappinsForEP1"), CEdFrame::ExportEntityMappinsForEP1 )
	EVT_MENU( XRCID("exportEntityMappinsForEP2"), CEdFrame::ExportEntityMappinsForEP2 )
	EVT_MENU( XRCID("exportQuestMappins"), CEdFrame::ExportQuestMappins )
	EVT_MENU( XRCID("updateMapPinEntities"), CEdFrame::UpdateMapPinEntities )
	EVT_MENU( XRCID("TEMP_CheckEntitiesThatUseEntityHandles"), CEdFrame::TEMP_CheckEntitiesThatUseEntityHandles )

	EVT_MENU( XRCID("updateFocusEntityTemplates"), CEdFrame::UpdateFocusEntityTemplates )
	EVT_MENU( XRCID("dumpCommunityAgents"), CEdFrame::DumpCommunityAgents )
	EVT_MENU( XRCID("aiTreeDefinitionsRefactor"), CEdFrame::AIGlobalRefactor_IAITree )
	EVT_MENU( XRCID("aiScriptNodesRefactor"), CEdFrame::AIGlobalRefactor_CBehTreeTask )
	EVT_MENU( XRCID("spawnTreesResaving"), CEdFrame::GlobalSpawnTreesResaving )
	EVT_MENU( XRCID("encounterLayersResaving"), CEdFrame::GlobalEncounterLayersResaving )

	EVT_MENU( XRCID("maraudersMap"), CEdFrame::OnMaraudersMap )
	EVT_MENU( XRCID("localizedStringsEditor"), CEdFrame::OnLocalizedStringsEditor )
	EVT_MENU( XRCID("localizationTools"), CEdFrame::OnLocalizationTools ) 
	EVT_MENU( XRCID("resourceSorter"), CEdFrame::OnResourceSorterStart )
	EVT_MENU( XRCID("sceneValidator"), CEdFrame::OnSceneValidator )
	EVT_MENU( XRCID("questBlocksValidator"), CEdFrame::OnQuestBlocksValidator )
	EVT_MENU( XRCID("entitiesBrowser"), CEdFrame::OnEntitiesBrowser )
	EVT_MENU( XRCID("soundsDebugger"), CEdFrame::OnSoundsDebugger )
	EVT_MENU( XRCID("DataErrorReporter"), CEdFrame::OnDataErrorReporter )
	EVT_MENU( XRCID("animationReport"), CEdFrame::OnAnimationsReport )
	EVT_MENU( XRCID("LipsyncItem"), CEdFrame::OnLipsyncPreview )
	EVT_MENU( XRCID("rewardsEditor"), CEdFrame::OnRewardsEditor )
	EVT_MENU( XRCID("journalEditor"), CEdFrame::OnJournalEditor )
	EVT_MENU( XRCID("worldEnvironmentEditor"), CEdFrame::OnWorldEnvironmentEditor )
	EVT_MENU( XRCID("exportAnimList"), CEdFrame::OnExportAnimationList )

	EVT_MENU( XRCID("dlcList"), CEdFrame::OnSelectDLC )

	EVT_MENU( XRCID("debugResaveTextures"), CEdFrame::OnResaveTextures )
	EVT_MENU( XRCID("debugXMLErrors"), CEdFrame::OnShowXMLErrors )

	EVT_MENU( XRCID("attitudeEditor"), CEdFrame::OnAttitudeEditor )	
	EVT_MENU( XRCID("lootEditor"), CEdFrame::OnLootEditor )
	EVT_MENU( XRCID("combineWithLootEntity"), CEdFrame::OnMergeLootWithEntity )

	EVT_MENU( XRCID("animCacheMenuItem"), CEdFrame::OnCreateEditorAnimCache )

	EVT_MENU( XRCID("config"), CEdFrame::OnConfig )
	EVT_MENU( XRCID("crowdDebugger"), CEdFrame::OnCrowdDebugger )
	EVT_MENU( XRCID("idgenerator"), CEdFrame::OnIDGenerator )

	EVT_MENU( XRCID("cookPreview"), CEdFrame::OnShowCookingDialog )	

	EVT_MENU( XRCID("ImportTextureSourceData"), CEdFrame::OnImportTextureSourceData )

	EVT_TEXT( XRCID("debugSoundRangeMin"), CEdFrame::OnDebugSoundRangeMin )
	EVT_TEXT( XRCID("debugSoundRangeMax"), CEdFrame::OnDebugSoundRangeMax )

	EVT_BUTTON( XRCID("performancePlatformSettings"), CEdFrame::OnPerformancePlatformClicked )

	EVT_TOOL( XRCID( "SqlConnection" ), CEdFrame::OnSqlConnectionButton )

	EVT_CHOICE( XRCID( "LanguageChoice" ), CEdFrame::OnChangeLanguage )

	EVT_CHOICE( XRCID( "gameDefinitionNameText" ), CEdFrame::OnGameDefinitionChoice )

    EVT_MENU_RANGE(ID_ACCEL_FILTER_FIRST, ID_ACCEL_FILTER_LAST, CEdFrame::OnAccelFilter)

	EVT_MENU_RANGE( ID_CONNECT_VDB, ID_CONNECT_VDB_MAX, CEdFrame::OnVDBMenu )

	EVT_MENU( XRCID("muteSound"), CEdFrame::OnMuteSound )
	EVT_MENU( XRCID("muteMusic"), CEdFrame::OnMuteMusic )

	EVT_MENU( XRCID("mapPinValidator"), CEdFrame::OnMapPinValidator )
	EVT_MENU( XRCID( "playGameDefinitionButton" ), CEdFrame::OnPlayGameFromGameResource )
	EVT_MENU( XRCID( "selectGameDefinitionButton" ), CEdFrame::OnSelectGameResource )

	EVT_MENU( XRCID( "pathlibEnableGeneration" ), CEdFrame::PathLibEnableGeneration )
	EVT_MENU( XRCID( "pathlibObstacles"), CEdFrame::PathLibEnableObstacles )
	EVT_MENU( XRCID( "pathlibLocalFolder" ), CEdFrame::PathLibLocalFolder )

	EVT_MENU( XRCID( "occlusionCullingUsage" ), CEdFrame::SwitchOcclusionCullingUsage )
	EVT_MENU( XRCID( "autohideToggle" ), CEdFrame::OnAutohideToggle )

	EVT_MENU( XRCID( "AutoScript" ), CEdFrame::OnAutoScriptWindow )

	EVT_MENU( XRCID( "explorationFinder" ), CEdFrame::ExplorationFinder )
	EVT_MENU( XRCID( "explorationFinderOneLayer" ), CEdFrame::ExplorationFinderOneLayer )
	EVT_MENU( XRCID( "explorationFinderPrev" ), CEdFrame::ExplorationFinderPrev )
	EVT_MENU( XRCID( "explorationFinderNext" ), CEdFrame::ExplorationFinderNext )
	EVT_MENU( XRCID( "explorationFinderIgnore" ), CEdFrame::ExplorationFinderToggleSelected )
	EVT_MENU( XRCID( "explorationFinderHide" ), CEdFrame::ExplorationFinderHide )

	EVT_MENU( XRCID( "clearDebugStuff" ), CEdFrame::ClearDebugStuff )
	EVT_MENU( XRCID( "forceAllShadowsFlag" ), CEdFrame::OnForceAllShadowsToggle )
	EVT_TOOL_RCLICKED( XRCID( "clearDebugStuff" ), CEdFrame::DebugFlagsContextMenu )

	EVT_TOGGLEBUTTON( XRCID( "turnOnProfiler" ), CEdFrame::OnProfilerChange )
	
	EVT_UPDATE_UI( wxID_ANY, CEdFrame::OnUpdateUI )

END_EVENT_TABLE()

IMPLEMENT_ENGINE_CLASS( SEditorUserConfig );
//#define LOG_GAME_INPUT_EVENTS

RED_DEFINE_STATIC_NAME( LayerInfoBuildTagChanged );

const Bool  SEditorUserConfig::DEFAULT_DISPLAY_CAMERA_TRANSFORM = false;
const float	SEditorUserConfig::DEFAULT_RENDERING_PANEL_CAMERA_SPEED = 2.f;
const float	SEditorUserConfig::DEFAULT_RENDERING_PANEL_CAMERA_ACCELERATION = 4.f;
const Bool  SEditorUserConfig::DEFAULT_ALLOW_TERRAIN_SYNC_LOADING = true;

SEditorUserConfig::SEditorUserConfig()
	: m_displayCameraTransform(DEFAULT_DISPLAY_CAMERA_TRANSFORM)
	, m_renderingPanelCameraSpeed(DEFAULT_RENDERING_PANEL_CAMERA_SPEED)
	, m_renderingPanelCameraAcceleration(DEFAULT_RENDERING_PANEL_CAMERA_ACCELERATION)
	, m_allowTerrainSyncLoading(DEFAULT_ALLOW_TERRAIN_SYNC_LOADING)
{
}

void SEditorUserConfig::Load()
{
	ResetToDefault();

	if ( !GConfig )
	{
		return;
	}

	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("DisplayCameraTransform"), m_displayCameraTransform );
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("RenderingPanelCameraSpeed"), m_renderingPanelCameraSpeed );
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("RenderingPanelCameraAcceleration"), m_renderingPanelCameraAcceleration );
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("AllowTerrainSyncLoading"), m_allowTerrainSyncLoading );
}

void SEditorUserConfig::Save()
{
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("DisplayCameraTransform"), m_displayCameraTransform );
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("RenderingPanelCameraSpeed"), m_renderingPanelCameraSpeed );
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("RenderingPanelCameraAcceleration"), m_renderingPanelCameraAcceleration );
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("AllowTerrainSyncLoading"), m_allowTerrainSyncLoading );
}


void SEditorUserConfig::ResetToDefault()
{
	m_displayCameraTransform = DEFAULT_DISPLAY_CAMERA_TRANSFORM;
	m_renderingPanelCameraSpeed = DEFAULT_RENDERING_PANEL_CAMERA_SPEED;
	m_renderingPanelCameraAcceleration = DEFAULT_RENDERING_PANEL_CAMERA_ACCELERATION;
	m_allowTerrainSyncLoading = DEFAULT_ALLOW_TERRAIN_SYNC_LOADING;
}

class PerfCounterWrapper : public wxObject
{
public:
	CPerfCounter*		m_counter;

public:
	PerfCounterWrapper( CPerfCounter* counter )
		: m_counter( counter )
	{};
};

#define ID_CONFIG_GAME_CAMERA_CURVE_AUTO		4001
#define ID_CONFIG_GAME_CAMERA_CURVE_AUTO_SPEED	4002

// Event table
BEGIN_EVENT_TABLE( CEdConfig, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "menuItemSave" ), CEdConfig::OnSave )
	EVT_MENU( XRCID( "menuItemExit" ), CEdConfig::OnExit )
	EVT_CLOSE( CEdConfig::OnClose )
END_EVENT_TABLE()


CEdConfig::CEdConfig( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("Config"), true )
{
	// Create engine properties tab
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanelEngine", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_propertiesEngine = new CEdConfigPropertiesPage( rp, nullptr );
		sizer1->Add( m_propertiesEngine, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanelEditor", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_propertiesEditor = new CEdPropertiesPage( rp, settings, nullptr );
		sizer1->Add( m_propertiesEditor, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create properties
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanelEncounter", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_propertiesEncounter = new CEdPropertiesPage( rp, settings, nullptr );
		sizer1->Add( m_propertiesEncounter, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create gameplay properties tab
	{
		wxPanel* rp = XRCCTRL( *this, "PropertiesPanelGameplay", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_propertiesGameplay = new CEdPropertiesPage( rp, settings, nullptr );
		m_propertiesGameplay->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdConfig::OnGameplayPropertiesChanged ), NULL, this );
		sizer1->Add( m_propertiesGameplay, 1, wxEXPAND, 0 );

		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	m_propertiesEditor->SetObject( &( ( ( CEdFrame* ) parent )->GetEditorUserConfig() ) );
	m_propertiesGameplay->SetObject( &GGame->GetGameplayConfig() );
	m_propertiesEncounter->SetObject( &CEncounterGlobalSettings::GetInstance() );
	m_propertiesEngine->Fill();

	Layout();
}

CEdConfig::~CEdConfig()
{
}

void CEdConfig::SaveOptionsToConfig()
{
	PC_SCOPE( SaveOptionsToConfig );
	Save();
}

void CEdConfig::LoadOptionsFromConfig()
{
}

void CEdConfig::OnSave( wxCommandEvent &event )
{
	Save();
}

void CEdConfig::Save()
{
	GGame->GetGameplayConfig().Validate();
	GGame->GetGameplayConfig().Save();

	wxTheFrame->GetEditorUserConfig().Save();
	
	m_propertiesGameplay->Refresh();
	m_propertiesEncounter->Refresh();

	SConfig::GetInstance().Save();
}

void CEdConfig::OnExit( wxCommandEvent &event )
{
	Save();
	Close();
}

void CEdConfig::OnClose( wxCloseEvent &event )
{
	Save();
	Destroy();
}

void CEdConfig::OnGameplayPropertiesChanged( wxCommandEvent &event )
{
	if ( event.GetClientData() )
	{
		CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	
		// Temporary hack. This code need CActor class, but engine don't have it.
		{
			if ( GGame && GGame->IsActive() && GGame->GetActiveWorld() && eventData->m_propertyName == TXT("forceLookAtPlayer") )
			{
				CPlayer* player = GCommonGame->GetPlayer();
				if ( player )
				{
					const Vector& playerPos = player->GetWorldPositionRef();
					const Float range = GGame->GetGameplayConfig().m_forceLookAtPlayerDist;

					Vector minBound( -range, -range, -1.f );
					Vector maxBound( range, range, 1.f );

					TDynArray< TPointerWrapper< CActor > > closestActors;
					GCommonGame->GetActorsManager()->GetClosestToPoint( playerPos, closestActors, Box( minBound, maxBound ), INT_MAX ); 

					for ( TDynArray< TPointerWrapper< CActor > >::iterator it = closestActors.Begin(); it != closestActors.End(); ++it )
					{
						CNewNPC* npc = Cast< CNewNPC >( (*it).Get() );
						if ( npc == NULL ) continue;

						if ( GGame->GetGameplayConfig().m_forceLookAtPlayer )
						{
							int type = GGame->GetGameplayConfig().m_lookAtConfig.m_reactionDebugType;

							if ( type >= 0 && type <= 4 )
							{
								//SLookAtDebugReactionBoneInfo info;
								SLookAtReactionBoneInfo info;

								switch ( type )
								{
								case 0:
									info.m_type = RLT_Glance;
									break;
								case 1:
									info.m_type = RLT_Look;
									break;
								case 2:
									info.m_type = RLT_Gaze;
									break;
								case 3:
									info.m_type = RLT_Stare;
									break;
								}

								info.m_targetOwner = player->GetRootAnimatedComponent();
								info.m_boneIndex = player->GetHeadBone();

								npc->EnableLookAt( info );
							}
							// This is the magic number...
							else if ( type < 0 && type >= -4 )
							{
								SLookAtDebugReactionBoneInfo info;

								switch ( type )
								{
								case -1:
									info.m_type = RLT_Glance;
									break;
								case -2:
									info.m_type = RLT_Look;
									break;
								case -3:
									info.m_type = RLT_Gaze;
									break;
								case -4:
									info.m_type = RLT_Stare;
									break;
								}

								info.m_targetOwner = player->GetRootAnimatedComponent();
								info.m_boneIndex = player->GetHeadBone();

								npc->EnableLookAt( info );
							}
						}
						else
						{
							npc->DisableLookAts();
						}
					}
				}
			}
		}
	}
}

void CEdFrame::AddToProfilerMenu( THashMap< String, TPair< Uint32, CPerfCounter* > > &ps, wxMenu *menu )
{
	for( THashMap< String, TPair< Uint32, CPerfCounter* > >::iterator it=ps.Begin(); it!=ps.End(); ++it )
	{
		Uint32 i = it->m_second.m_first;
		wxMenuItem* item = menu->AppendCheckItem( ID_FRIST_PROFILER_SHOW_COUNTER+i, it->m_first.AsChar() );
		Connect( ID_FRIST_PROFILER_SHOW_COUNTER+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnTogglePerfCounter ), new PerfCounterWrapper( it->m_second.m_second ), this );
	}
}

CEdFrame::CEdFrame( wxWindow *parent, int id )
	: m_properties( nullptr )
	, m_assetBrowser( nullptr )
	, m_sceneExplorer( nullptr )
	, m_viewport( nullptr )
	, m_mainToolbar( nullptr )
    , m_notebook( nullptr )
    , m_notebookSizer( nullptr )
	, m_filterSmartPanel( nullptr )
    , m_clonedMenu( nullptr )
    , m_isSolutionTabDrag( false )
	, m_fileReloadDialog( nullptr )
	, ISmartLayoutWindow( this )
	, m_isViewportFloating( false )
	, m_fullScreenFrame( nullptr )
	, m_worldProperties( nullptr )
	, m_moveEntityTool( nullptr )
	, m_dataErrorReporterWindow( nullptr )
	, m_mapPinValidator( nullptr )
#ifdef REWARD_EDITOR
	, m_rewardsEditor( nullptr )
#endif
	, m_journalEditor( nullptr )
	, m_characterDBeditor( nullptr )
	, m_worldEnvironmentEditor( nullptr )
	, m_worldSceneDebuggerWindow( nullptr )
	, m_undoHistoryFrame( nullptr )
	, m_profilerOnOffToggle( nullptr )
	, m_autoScriptWin( nullptr )
	, m_xmlErrorsDisplayer( nullptr )
	, m_lastDisplayMode( 1 )
	, m_inGameConfigDialog( nullptr )
{
	// Set frame pointer
	wxTheFrame = this;
	m_commandLine = SGetCommandLine();
	if( !m_commandLine.Empty() )
	{
		TDynArray< String > parameter;
		m_commandLine.Slice( parameter, TXT(" ") );
		for( Uint32 i=0; i<parameter.Size(); i++ )
		{
			parameter[ i ].Trim();

			String name, value;
			if( !parameter[ i ].Split( TXT("="), &name, &value ) )
			{
				name = parameter[ i ];
			}

			if( name.GetLength() && name.LeftString( 1 ) == TXT("-") )
			{
				name = name.RightString( name.GetLength()-1 );
			}

			if( name.GetLength() )
			{
				m_commandLineValue.Set( name, value );
			}
		}
	}

	// Load layout from XRC
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("MainFrame") );

	wxFrame tempFrame;
	wxXmlResource::Get()->LoadFrame( &tempFrame, NULL, wxT("UniversalFrame") );
	m_clonedMenu = tempFrame.GetMenuBar();
	tempFrame.SetMenuBar( NULL );

    // Init notebook
    m_notebook = new wxSmartLayoutAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
        wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON ); // XRCCTRL( *this, "EditorNotebook", wxNotebook );
    m_notebookSizer = new wxBoxSizer( wxVERTICAL );
    m_notebookSizer->Add( m_notebook, 1, wxEXPAND | wxALL, 1);
    SetSizer(m_notebookSizer);
    
    m_gamePanel = XRCCTRL( *this, "m_gamePanel", wxPanel );
	ASSERT( m_gamePanel, TXT( "Seems like wxWidgets failed to find 'm_gamePanel' control." ) );
	if ( !m_gamePanel )
	{
		HALT( "wxWidgets failed to find 'm_gamePanel' control. Contact one of the programmers, use previous build until new build is made." );
	}
    m_notebook->AddPage( m_gamePanel, TXT("World"), true, 
        SEdResources::GetInstance().LoadBitmap( _T("IMG_WORLD16") ) );
    m_notebook->Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( CEdFrame::OnGameEditorClosePage ), 0, this);
    m_notebook->Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( CEdFrame::OnGameEditorPageShow ), 0, this);
    // Tell wxAuiManager to manage this frame
    m_auiMgr.SetManagedWindow(m_gamePanel);
	m_gamePanel->SetSizer( NULL );

    m_gameMenu = GetMenuBar();

	// Add version in titilebar	
	SetFrameTitle();

	// Move toolbar to pane
	wxToolBar* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);
	m_mainToolbar = toolbar;
	m_auiMgr.AddPane( toolbar , wxAuiPaneInfo().
		Name(wxT("Toolbar")).Caption(wxT("Main Toolbar")). /*BestSize( 190, 25 ).*/
		ToolbarPane().Top().
		Dockable(false).TopDockable(true).BottomDockable(true).CloseButton(false)  );
	SetToolBar( NULL );

	// Create side bar
	m_solution = new wxNotebook(m_gamePanel, wxID_ANY,	wxDefaultPosition, wxSize(270,500), wxNB_MULTILINE | wxNB_FLAT );
    m_solution->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler(CEdFrame::OnSolutionDragStart), NULL, this );
    m_solution->Connect( wxEVT_MOTION,    wxMouseEventHandler(CEdFrame::OnSolutionDragMove),  NULL, this );
    m_solution->Connect( wxEVT_LEFT_UP,   wxMouseEventHandler(CEdFrame::OnSolutionDragEnd),   NULL, this );

	// Add scene explorer
	m_sceneExplorer = new CEdSceneExplorer( m_solution );

	// Add property grid pane
	PropertiesPageSettings settings;
	m_properties = new CEdSelectionProperties( m_solution, settings, nullptr );

	// Add world properties
	m_worldProperties = new CEdWorldProperties( m_solution, settings, nullptr );

	// Add pane
	m_auiMgr.AddPane( m_solution,  wxAuiPaneInfo().
		Name(wxT("Solution")).Caption(wxT("Panel")).
		Row(2).Right().Floatable( true ).Dockable(true).Layer(1).Movable(true).CloseButton( false ) );

	// Create manipulator toolbar
	m_widgets = new wxToolBar( m_gamePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_DOCKABLE | wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL | wxTB_TEXT );
	m_widgets->SetToolBitmapSize(wxSize(38,24));
	m_widgets->AddTool( XRCID("editSelectionOnActiveLayer"), wxT("Active Layer"), SEdResources::GetInstance().LoadBitmap(_T("IMG_SELECT_SINGLE")), wxT("Active Layer Selection"), wxITEM_CHECK);
	m_widgets->AddTool( XRCID("editSelectionOnMultiLayer"), wxT("Multi Layer"), SEdResources::GetInstance().LoadBitmap(_T("IMG_SELECT_MULTI")), wxT("Multi Layer Selection"), wxITEM_CHECK);
	m_widgets->AddTool( XRCID("editSelectByTheSameTag"), wxT("By Tag"), SEdResources::GetInstance().LoadBitmap(_T("IMG_PICK")), wxT("Select entities by tag"), wxITEM_NORMAL );
	m_widgets->AddSeparator();
	m_widgets->AddTool( XRCID("widgetSpaceWorld"), wxT("World"), SEdResources::GetInstance().LoadBitmap(_T("IMG_WORLD")), wxT("World Transform"), wxITEM_CHECK);
	m_widgets->AddTool( XRCID("widgetSpaceLocal"), wxT("Local"), SEdResources::GetInstance().LoadBitmap(_T("IMG_HOME")), wxT("Local Transform"), wxITEM_CHECK);
	m_widgets->AddTool( XRCID("widgetSpaceForeign"), wxT("Foreign"), SEdResources::GetInstance().LoadBitmap(_T("IMG_FOREIGN")), wxT("Foreign Transform"), wxITEM_CHECK);
	m_widgets->AddSeparator();
	m_widgets->AddTool( XRCID("widgetModeMove"), wxT("Move"), SEdResources::GetInstance().LoadBitmap(_T("IMG_MOVE")), wxT("Move Object"), wxITEM_CHECK);
	m_widgets->AddTool( XRCID("widgetModeRotate"), wxT("Rotate"), SEdResources::GetInstance().LoadBitmap(_T("IMG_ROTATE")), wxT("Rotate Object"), wxITEM_CHECK);
	m_widgets->AddTool( XRCID("widgetModeScale"), wxT("Scale"), SEdResources::GetInstance().LoadBitmap(_T("IMG_SCALE")), wxT("Scale Object"), wxITEM_CHECK);
	m_widgets->Realize();

	m_widgetP4 = new wxToolBar( m_gamePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_DOCKABLE | wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL | wxTB_TEXT );
	m_widgetP4->SetToolBitmapSize(wxSize(38,24));
	m_widgetP4->AddTool( XRCID("disconnectPerforce"), wxT("Disable    P4"), SEdResources::GetInstance().LoadBitmap(_T("IMG_DISCONNECT")), wxT("Disconnect Perforce"), wxITEM_CHECK);
	m_widgetP4->Realize();

	// Setup cyclable widgets
	m_cyclableWidgets.Resize( 3 );
	for ( Uint32 i=0; i<m_cyclableWidgets.Size(); ++i )
	{
		m_cyclableWidgets[i] = true;
	}

	// Add it to layout
	m_auiMgr.AddPane( m_widgets, 
		wxAuiPaneInfo().Name(wxT("WidgetToolbar")).Caption(wxT("Mode")).ToolbarPane().Left()
		.Dockable(false).LeftDockable(true).RightDockable(true).CloseButton(false) );

	m_auiMgr.AddPane( m_widgetP4, 
		wxAuiPaneInfo().Name(wxT("WidgetTBP4")).Caption(wxT("P4")).ToolbarPane().Left()
		.Dockable(false).LeftDockable(true).RightDockable(true).CloseButton(false) );

	m_emptyPanel = new wxPanel( m_gamePanel );
	m_auiMgr.AddPane( m_emptyPanel, wxAuiPaneInfo().Name(wxT("Empty")).Caption(wxT("Empty")).CaptionVisible( false ).CloseButton( false ).Hide() );

	// Add brush panel
	m_brushPanel = new CEdBrushPanel( m_solution );

	// Create rendering viewport
	m_viewport = new CEdWorldEditPanel( m_gamePanel, m_sceneExplorer );

	// Setup UndoManager
    m_undoManager = new CEdUndoManager( m_viewport, true );
    m_undoManager->AddToRootSet();
	m_undoManager->SetMenuItems( m_gameMenu->FindItem( XRCID("editUndo") ), m_gameMenu->FindItem( XRCID("editRedo") ) );
	m_properties->Get().SetUndoManager( m_undoManager );
	m_worldProperties->Get().SetUndoManager( m_undoManager );

	GSplash->UpdateProgress( TXT("Loading main frame...") );

	m_viewport->SetUndoManager( m_undoManager );
	GGame->SetViewport( m_viewport->GetRenderingWindow()->GetViewport() );
	m_auiMgr.AddPane( m_viewport, wxAuiPaneInfo().Name(wxT("Viewport")).Caption(wxT("Viewport")).Centre().Dockable( false ).Movable( true ).Floatable( true ).CloseButton( false ).CaptionVisible( false ) );

	// Setup map opening dialog
	m_mapDialog.SetMultiselection( false );
	m_mapDialog.AddFormat( ResourceExtension< CWorld >(), TXT("The Witcher 3 World") );

	// Get maps directory
	String mapsDirectory;
	GDepot->GetAbsolutePath( mapsDirectory );
	mapsDirectory += TXT("witcher3\\maps\\");
	m_mapDialog.SetDirectory( mapsDirectory );

	// Add tools panel
	m_tools = new CEdToolsPanel( m_solution,  m_viewport );

	// Add entities browser
	m_entitiesBrowser = new CEntitiesBrowser( m_solution );

	m_toolWindowState.GetRef( ID_VIEW_SCENE ).Set( m_sceneExplorer,  wxT("Scene"), true, false );
	m_toolWindowState.GetRef(  ID_VIEW_PROPERTIES ).Set( m_properties,  wxT("Properties"), true, false );
	m_toolWindowState.GetRef(  ID_VIEW_WORLD ).Set( m_worldProperties, wxT("World"), true, false );	
	m_toolWindowState.GetRef(  ID_VIEW_TOOLS ).Set( m_tools,  wxT("Tools"), true, false );
	m_toolWindowState.GetRef(  ID_VIEW_ENTITIES ).Set( m_entitiesBrowser, GEntityBrowserName, true, false );
	m_toolWindowState.GetRef(  ID_VIEW_BRUSHES ).Set( m_brushPanel, wxT("Brushes"), true, false );

	for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
		if( !it->m_second.isFloat )
			m_solution->AddPage( it->m_second.widget, it->m_second.caption );

    int nViewMenuIdx = m_gameMenu->FindMenu( TXT("View") );
    wxMenu* viewMenu = m_gameMenu->GetMenu(nViewMenuIdx);

	wxMenu* viewSolutionMenu = new wxMenu();

	for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
	{
		wxMenu* menu = new wxMenu();
		menu->AppendCheckItem( it->m_first, TXT("Visible") )->Check( it->m_second.isVisible );
		Connect( it->m_first, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
		menu->AppendCheckItem( it->m_first+1, TXT("Tabbed") )->Check( !it->m_second.isFloat );
		Connect( it->m_first+1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
        menu->Append( it->m_first+2, TXT("Show") );
		Connect( it->m_first+2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );

		viewSolutionMenu->Append( it->m_first+3, it->m_second.caption, menu );
	}

	viewSolutionMenu->AppendCheckItem( ID_VIEW_SHOW_ALL, TXT("Show all") )->Check( true );
	Connect( ID_VIEW_SHOW_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
	viewSolutionMenu->AppendCheckItem( ID_VIEW_HIDE_ALL, TXT("Hide all") )->Check( false );
	Connect( ID_VIEW_HIDE_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
	viewSolutionMenu->AppendCheckItem( ID_VIEW_FLOAT_ALL, TXT("All to windows") )->Check( false );
	Connect( ID_VIEW_FLOAT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
	viewSolutionMenu->AppendCheckItem( ID_VIEW_DEFLOAT_ALL, TXT("All to tab") )->Check( true );
	Connect( ID_VIEW_DEFLOAT_ALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );

	viewMenu->Append( 1, TXT("Panel"), viewSolutionMenu );

	wxMenu* viewToolbarsMenu = new wxMenu();
	viewToolbarsMenu->AppendCheckItem( ID_VIEW_TOOLBAR_MAIN, TXT("Main") )->Check( true );
	Connect( ID_VIEW_TOOLBAR_MAIN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
	viewToolbarsMenu->AppendCheckItem( ID_VIEW_TOOLBAR_WIDGETS, TXT("Mode") )->Check( true );
	Connect( ID_VIEW_TOOLBAR_WIDGETS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );
	viewToolbarsMenu->AppendCheckItem( ID_VIEW_TOOLBAR_WIDGETP4, TXT("P4") )->Check( true );
	Connect( ID_VIEW_TOOLBAR_WIDGETP4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );

	viewMenu->Append( 2, TXT("Toolbars"), viewToolbarsMenu );

    wxMenu* viewViewportMenu = new wxMenu();
	viewViewportMenu->AppendCheckItem( ID_VIEW_VIEWPORT_FLOAT, TXT("Float") )->Check( false );
	Connect( ID_VIEW_VIEWPORT_FLOAT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewChanged ), NULL, this );

	viewMenu->Append( 3, TXT("Viewport"), viewViewportMenu );

	wxMenu* popupNotificationMenu = new wxMenu();
	popupNotificationMenu->AppendRadioItem( ID_VIEW_NOTIFICATION_CENTER, TXT("Center") )->Check();
	Connect( ID_VIEW_NOTIFICATION_CENTER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnPopupNotificationLocationChanged ), NULL, this );
	popupNotificationMenu->AppendRadioItem( ID_VIEW_NOTIFICATION_TOP_LEFT, TXT("Top left") );
	Connect( ID_VIEW_NOTIFICATION_TOP_LEFT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnPopupNotificationLocationChanged ), NULL, this );
	popupNotificationMenu->AppendRadioItem( ID_VIEW_NOTIFICATION_TOP_RIGHT, TXT("Top right") );
	Connect( ID_VIEW_NOTIFICATION_TOP_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnPopupNotificationLocationChanged ), NULL, this );
	popupNotificationMenu->AppendRadioItem( ID_VIEW_NOTIFICATION_BOTTOM_LEFT, TXT("Bottom left") );
	Connect( ID_VIEW_NOTIFICATION_BOTTOM_LEFT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnPopupNotificationLocationChanged ), NULL, this );
	popupNotificationMenu->AppendRadioItem( ID_VIEW_NOTIFICATION_BOTTOM_RIGHT, TXT("Bottom right") );
	Connect( ID_VIEW_NOTIFICATION_BOTTOM_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnPopupNotificationLocationChanged ), NULL, this );

	viewMenu->Append( 4, TXT("Popup notification"), popupNotificationMenu );

	viewMenu->AppendSeparator();

	viewMenu->Append( ID_VIEW_FIELDOFVIEW, wxT("Field of view..."), NULL );
	Connect( ID_VIEW_FIELDOFVIEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewFOV ), NULL, this );

	wxMenu* aspectRatioMenu = new wxMenu();
	
	aspectRatioMenu->AppendRadioItem( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_21_9, TXT("21:9") );
	Connect( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_21_9, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewportCachetAspectRatioChanged ), NULL, this );
	aspectRatioMenu->AppendRadioItem( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_9, TXT("16:9") )->Check();
	Connect( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_9, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewportCachetAspectRatioChanged ), NULL, this );
	aspectRatioMenu->AppendRadioItem( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_10, TXT("16:10") );
	Connect( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_10, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewportCachetAspectRatioChanged ), NULL, this );
	aspectRatioMenu->AppendRadioItem( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_4_3, TXT("4:3") );
	Connect( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_4_3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewportCachetAspectRatioChanged ), NULL, this );
	aspectRatioMenu->AppendRadioItem( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_NONE, TXT("NONE") );
	Connect( ID_VIEW_VIEWPORT_CACHETASPECTRATIO_NONE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnViewportCachetAspectRatioChanged ), NULL, this );

	viewMenu->Append( ID_VIEW_VIEWPORT_CACHETASPECTRATIO, wxT("Cachet aspect ratio..."), aspectRatioMenu );

	viewMenu->AppendCheckItem( ID_VIEW_OVERLAYVERTEXSPRITES, wxT("Overlay vertex sprites") )->Check( true );
	Connect( ID_VIEW_OVERLAYVERTEXSPRITES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnOverlayVertexSprites ), NULL, this );

	viewMenu->Append( ID_VIEW_ENTITIES_AROUND_CAMERA, wxT("List entities around camera...") );
	Connect( ID_VIEW_ENTITIES_AROUND_CAMERA, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnListEntitiesAroundCamera ), NULL, this );

	// file menu - recent worlds
	{
		const Int32 LastMenuFilePos = 9; // change it if new items are added to the file menu

		wxMenu *fileMenu = this->m_clonedMenu->GetMenu( 0 );
		
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Recent worlds") );

		TDynArray< TPair<String, String > > settings;
		config.AllSettings( settings );
		Sort( settings.Begin(), settings.End() );

		if ( settings.Size() > 0 )
		{
			Uint32 numEntries = 0;
			for ( Uint32 i = 0; i < settings.Size(); i++ )
			{
				if( !settings[i].m_second.Empty() )
				{
					if ( GDepot->FindFile( settings[i].m_second) == NULL )
					{
						config.DeleteEntry( settings[i].m_first, false );
					}
					else
					{
						Int32 id = fileMenu->Insert( LastMenuFilePos+numEntries, wxID_ANY, settings[i].m_second.AsChar() )->GetId();
						m_recent[id] = settings[i].m_second;
						Connect( id , wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::OnOpenRecent ), NULL, this );
						numEntries++;
					}
				}
			}

			if ( numEntries > 0 )
			{
				wxMenuItem *item = new wxMenuItem( fileMenu, wxID_SEPARATOR );
				fileMenu->Insert( LastMenuFilePos+numEntries, item );
			}
		}
	}

	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( WorldLoadProgress ), this );
	SEvents::GetInstance().RegisterListener( CNAME( WorldUnloaded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameStarted ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnding ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectAsset ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FocusOnAsset ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectAndOpenAsset ), this );
	SEvents::GetInstance().RegisterListener( CNAME( OpenAsset ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CenterOnSelected ), this );
	SEvents::GetInstance().RegisterListener( CNAME( Delete ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SavingConfiguration ), this );
	SEvents::GetInstance().RegisterListener( CNAME( PlayGame ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadConfirm ), this );
	SEvents::GetInstance().RegisterListener( CNAME( FileReloadToConfirm ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ShowReloadFilesDialog ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SimulationComplete ), this );
	SEvents::GetInstance().RegisterListener( CNAME( LayerInfoBuildTagChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SetDLCToStart ), this );
	SEvents::GetInstance().RegisterListener( CNAME( LoadGame ), this );

	
	wxMenuUtils::CloneMenuBar( m_clonedMenu, m_gameMenu );

	// Dockable filters panel
	m_filter = new CEdFilterPanel( m_gamePanel,  m_viewport->GetViewport() );
	m_filterSmartPanel = new wxSmartLayoutPanel(this, TXT("Filter"), true, m_filter );
	m_filterSmartPanel->Hide();

	// In game configuration dialog - this replace performance platform
	m_inGameConfigDialog = new CEdInGameConfigurationDlg( wxTheFrame );
	m_inGameConfigDialog->Hide();

	// Whats new dialog. Will be visible only of the value in the user ini is set to true
	Bool dontShowWhatsNew = false;
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("DontShowAgain"), dontShowWhatsNew );
	if ( !dontShowWhatsNew )
	{
		OnWhatsNew();
	}

	Bool showXmlErrors = false;
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("showXMLErrorsOnStart"), showXmlErrors );
	if ( showXmlErrors )
	{
		OnXMLErrors();
	}

#ifndef NO_DATA_VALIDATION
	// Create the data error reporter
	m_dataErrorReporterWindow = new CEdDataErrorReporterWindow( m_gamePanel );
#endif

	// Reload dialog
	m_fileReloadDialog = new CEdFileReloadDialog();

	if( GetStatusBar() == NULL )
		CreateStatusBar();

	// SetFieldsCount
	m_statusBar = GetStatusBar();
	int satusBarFieldsWidths[] = { -10, -1 };
	m_statusBar->SetFieldsCount( 2, satusBarFieldsWidths );

	// Update grid settings
	UpdateGridWidgets();

	// Update widgets UI
	UpdateWidgetUI();

	UpdateProfilerMenu();
	
    m_notebook->Update();
    
	Layout();
	Refresh();
	Update();

	m_auiMgr.Update();

	m_statusBarTimer = new CEdTimer();
	m_statusBarTimer->Connect( wxEVT_TIMER, wxCommandEventHandler( CEdFrame::OnStatusBarTimeout ), NULL, this );

	SetStatusBarText( TXT( "Loading configuration..." ), 1500 );

	// Asset browser
	m_assetBrowser = new CEdAssetBrowser( NULL );
	m_assetBrowser->Hide();
	
	m_languageChoice = XRCCTRL( *this, "LanguageChoice", wxChoice );
	FillLanguagesChoiceBox();

	GSplash->UpdateProgress( TXT("Loading config...") );
	LoadOptionsFromConfig();

	// Setup depot file watcher
	m_assetDirWatcher = 
		new CDirectoryWatcher( 
			GDepot->GetRootDataPath(), this, FAF_Added | FAF_Renamed | FAF_Modified | FAF_Removed | FAF_OnlyUnderVC
			);

	m_assetDirWatcher->AddExcludedDirectoy( GDepot->GetRootDataPath() + TXT("scripts") );

	// add the "resave animations" tool to the menu
#if 0	// commented out because it crashes the editor when the asset browser is docked
	{
		const Uint32 id = 12345;

		wxMenu* subMenu = this->GetMenuBar()->GetMenu(5);
		subMenu->AppendSeparator();
		subMenu->Append(id, wxT("(DEX) Resave Animations..."), wxEmptyString, false );
		Connect(id, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdFrame::OnResaveAnimationsTool), NULL, this);
	}
#endif

	if ( GEngine && GEngine->GetInputDeviceManager() )
	{
		CInputDeviceManagerWin32* inputDeviceManager = static_cast< CInputDeviceManagerWin32* >( GEngine->GetInputDeviceManager() );
		inputDeviceManager->SetTopLevelHwnd( (HWND)GetHandle() );
	}	

	m_profilerOnOffToggle = XRCCTRL( *this, "turnOnProfiler", wxToggleButton );

	SEvents::GetInstance().RegisterListener( CNAME( ProfilerChangeState ), this );

	RunEditorInExtraMode( m_commandLineValue );

	// bind accelerators to sceneExplorer so that it passes shortcut events to the frame when it's floating
	m_sceneExplorer->SetAcceleratorTable(*GetMenuBar()->GetAcceleratorTable());
}

void CEdFrame::SetFrameTitle()
{
	// Get information about perforce connection
	String perforceUser, perforceWorkspace, finalPerforceSettings;
	SSourceControlSettings settings;
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
#if defined(RED_PLATFORM_WIN64) || defined(RED_ARCH_X64)
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl64") );
#else
	CConfigurationScopedPathSetter pathSetter( config, TXT("/VersionControl") );
#endif
	config.Read( TXT("User"), &perforceUser );
	config.Read( TXT("Workspace"), &perforceWorkspace );
	if( perforceUser.Empty() == true || perforceWorkspace.Empty() == true )
	{
		finalPerforceSettings = TXT("Perforce{ !!!_FILL_IN_CLIENT_SETTINGS_!!! }");
	}
	else
	{
		finalPerforceSettings = String::Printf( TXT("Perforce{ U: %s, W: %s }"), perforceUser.AsChar(), perforceWorkspace.AsChar() ); 
	}

	// Get information about loaded world
	String str = TXT("No world");
	if ( GGame->GetActiveWorld() )
		str = GGame->GetActiveWorld()->GetFile()->GetDepotPath();

	// Merge parts
	SetTitle( wxString( 
		( str 
		+ TXT(" - Red Kit - Build ") 
		+ APP_VERSION_NUMBER 
		+ TXT("  (") 
		+ APP_DATE 
		+ TXT(")") 
#ifdef _DEBUG
		+ TXT(" - DEBUG")
#endif
		+ TXT(" - ")
		+ finalPerforceSettings
		).AsChar() ) );
}

CEdFrame::~CEdFrame()
{
	SEvents::GetInstance().UnregisterListener( this );

    m_undoManager->RemoveFromRootSet();
    m_undoManager->Discard();

    wxTheFrame = NULL;

	m_filterSmartPanel->ClosePanel();
	m_filterSmartPanel->Destroy();
    m_assetBrowser->ClosePanel();
    m_assetBrowser->Destroy();
	m_fileReloadDialog->Close();
	m_fileReloadDialog->Destroy();

	if ( m_fullScreenFrame )
	{
		m_fullScreenFrame->Destroy();
	}

	if ( m_xmlErrorsDisplayer )
	{
		delete m_xmlErrorsDisplayer;
	}

	delete m_timer;
	delete m_statusBarTimer;
	delete m_assetDirWatcher;

	// Deinitialize the frame manager
	m_auiMgr.UnInit();

}

void ToggleFullscreen()
{
	if ( wxTheFrame != NULL )
	{
		wxTheFrame->ToggleFullscreen();
	}
}

void CEdFrame::ToggleFullscreen()
{
	if ( m_fullScreenFrame != NULL )
	{
		m_viewport->Reparent( m_gamePanel );
		m_auiMgr.AddPane( m_viewport, wxAuiPaneInfo().Name(wxT("Viewport")).Caption(wxT("Viewport")).Centre().Dockable( false ).Movable( true ).Floatable( true ).CloseButton( false ).CaptionVisible( false ) );
		
// 		// Recapture input so it is bound to proper window handle
// 		const EInputCaptureMode mode = SRawInputManager::GetInstance().GetInputCaptureMode();
// 		SRawInputManager::GetInstance().CaptureInput( NULL, ICM_None );
// 		GGame->GetInputManager()->ResetInputs();
		//TBD: Still really need to set the toplevel window anyway?
 		m_viewport->GetViewport()->SetTopLevelWindow( (HWND)GetHandle() );
// 		m_viewport->GetViewport()->CaptureInput( mode );

		if ( GEngine && GEngine->GetInputDeviceManager() )
		{
			CInputDeviceManagerWin32* inputDeviceManager = static_cast< CInputDeviceManagerWin32* >( GEngine->GetInputDeviceManager() );
			inputDeviceManager->SetTopLevelHwnd( (HWND)GetHandle() );
		}	

		delete m_fullScreenFrame;
		m_fullScreenFrame = NULL;
	}
	else
	{
		if ( wxDisplay::GetCount() <= 0 )
		{
			ERR_EDITOR( TXT( "FULLSCREEN: Could not identify the displays - thus can't switch to fullscreen" ) );
			return;
		}

		// determine the display on which the fullscreen overlay window should be created
		wxPoint fullscreenWinPos( 0, 0 );
		wxSize fullscreenWinSize( 0, 0 );
		wxPoint parentPos = this->GetPosition();

		LOG_EDITOR( TXT( "FULLSCREEN: checking the creation position - %d displays to check,\nparent window at [%d, %d]" ),
			wxDisplay::GetCount(), parentPos.x, parentPos.y );

		for ( Uint32 displayIdx = 0; displayIdx < wxDisplay::GetCount(); ++displayIdx )
		{
			wxDisplay disp( displayIdx );
			wxRect geometry = disp.GetGeometry();

			LOG_EDITOR( TXT( "FULLSCREEN: display %d: pos [%d, %d], size [%d, %d]" ),
				displayIdx, 
				geometry.x, geometry.y, 
				geometry.GetWidth(), geometry.GetHeight() );

			if ( geometry.Contains( wxRect( parentPos, wxSize( 0, 0 ) ) ) == true )
			{
				LOG_EDITOR( TXT( "FULLSCREEN: match found!!!" ) );
				fullscreenWinPos = geometry.GetPosition();
				fullscreenWinSize = geometry.GetSize();
				break;
			}
		}

		// check if any display has been found - if so, it have non-zero dimensions
		if ( ( fullscreenWinSize.GetWidth() == 0 ) || ( fullscreenWinSize.GetHeight() == 0 ) )
		{
			// select the first available display and use it
			wxDisplay disp( 0 );
			wxRect geometry = disp.GetGeometry();
			fullscreenWinPos = geometry.GetPosition();
			fullscreenWinSize = geometry.GetSize();
		}

		// once a display has been found, create an overlay window
		// and put the rendering viewport there
		LOG_EDITOR( TXT( "Creating fullscreen window at pos[%d, %d], size[%d, %d]" ),
			fullscreenWinPos.x, fullscreenWinPos.y, 
			fullscreenWinSize.GetWidth(), fullscreenWinSize.GetHeight());

		// create the fullscreen overlay window
		m_fullScreenFrame = new wxFrame( this, 
			wxID_ANY, wxString(), fullscreenWinPos, fullscreenWinSize, 
			wxPOPUP_WINDOW | wxSTAY_ON_TOP | wxFRAME_FLOAT_ON_PARENT | wxNO_BORDER );

		// Exactly at the same time that the pane is being detached
		// the render thread fetches the viewport size which atm is 1x1.
		// This causes a hell lot of asserts for this single frame.
		// The following magic sleep prevents this to happen by forcing
		// render thread to fetch actual viewport size before it is detached.
		Sleep( 50 );

		m_auiMgr.DetachPane( m_viewport );
		m_viewport->Reparent( m_fullScreenFrame );
		m_viewport->SetPosition( wxPoint( 0, 0 ) );
		m_viewport->SetSize( fullscreenWinSize );

		m_fullScreenFrame->SetBackgroundColour( wxColour( 0, 0, 0 ) );
		m_fullScreenFrame->Show( true );
		m_fullScreenFrame->SetFocus();

		// Recapture input so it is bound to proper window handle
		CInputDeviceManagerWin32* inputDeviceManager = static_cast< CInputDeviceManagerWin32* >( GEngine->GetInputDeviceManager() );
		RED_ASSERT( inputDeviceManager );
		inputDeviceManager->SetTopLevelHwnd( (HWND)m_fullScreenFrame->GetHandle() );

// 		const EInputCaptureMode mode = SRawInputManager::GetInstance().GetInputCaptureMode();
// 		SRawInputManager::GetInstance().CaptureInput( NULL, ICM_None );
// 		GGame->GetInputManager()->ResetInputs();
		m_viewport->GetViewport()->SetTopLevelWindow( (HWND)m_fullScreenFrame->GetHandle() );
		//m_viewport->GetViewport()->CaptureInput( mode );
	}

	m_auiMgr.Update();
	ToggleFloatingViewport();
}

Bool CEdFrame::IsInFullscreenMode() const
{
	return m_fullScreenFrame != NULL;
}

void CEdFrame::ToggleFloatingViewport()
{
	// If fullscreen is enabled, do nothing
	if( m_fullScreenFrame )
	{
		return;
	}

	if( m_isViewportFloating )
	{
		wxSize size = wxDefaultSize;
		wxPoint pos = wxDefaultPosition;

		// initialize size to something valid.
		size.x = 640;
		size.y = 480;

		m_auiMgr.GetPane( m_viewport )
			.Float()
			.FloatingSize( size )
			.FloatingPosition( pos );

		m_auiMgr.GetPane( m_emptyPanel ).CenterPane();
		m_auiMgr.GetPane( m_emptyPanel ).Show();

		// install the keyboard accelerators from the menubar
		m_viewport->SetAcceleratorTable(*GetMenuBar()->GetAcceleratorTable());
	}
	else
	{
		m_auiMgr.GetPane( m_emptyPanel ).Hide();
		m_auiMgr.GetPane( m_viewport ).CenterPane();
	}
	m_auiMgr.Update();

	// This must be done after AUI update
	if( m_isViewportFloating )
	{
		m_viewport->GetViewport()->SetTopLevelWindow( (HWND)m_auiMgr.GetPane( m_viewport ).frame->GetHandle() );
	}
	else
	{
		m_viewport->GetViewport()->SetTopLevelWindow( (HWND)GetHandle() );
	}
}

Bool CEdFrame::GetCommandLineParameter( const String &name, String &value )
{ 
	if( m_commandLineValue.KeyExist( name ) )
	{
		String* ptr = m_commandLineValue.FindPtr( name );
		if ( ptr )
		{
			value = *ptr;
			return true;
		}
	}
	return false;
}

Bool CEdFrame::HasCommandLineParameter( const String &name )
{
	return m_commandLineValue.KeyExist( name );
}

void CEdFrame::OnViewChanged( wxCommandEvent& event )
{
	int id = event.GetId();

	if( id == ID_VIEW_VIEWPORT_FLOAT )
	{	
		m_isViewportFloating = m_gameMenu->FindItem( id )->IsChecked();
		ToggleFloatingViewport();
	}
    else if( id == XRCID("outputPanel") )
    {
		if ( GLogOutput )
		{
			GLogOutput->Show();
		}
	}
	else if( id == XRCID("dataErrorReporter") )
	{
#ifndef NO_DATA_VALIDATION
		ShowDataErrorReporter();
#else
		::wxMessageBox( TXT("Data Error Reporter is disabled."), TXT("Information"), wxOK | wxCENTRE | wxICON_INFORMATION );
#endif
	}
	else if( id == XRCID("filterPanel") )
	{
		m_filterSmartPanel->Show();
		m_filterSmartPanel->SetFocus();
	}
    else if ( id == XRCID("inGameConfig") )
	{
		m_inGameConfigDialog->Execute();
		m_inGameConfigDialog->Show();
	}
	else if( id == ID_VIEW_TOOLBAR_MAIN )
	{
		if( m_gameMenu->FindItem( id )->IsChecked() )
			m_auiMgr.GetPane( m_mainToolbar ).Show();
		else
			m_auiMgr.GetPane( m_mainToolbar ).Hide();

		m_auiMgr.Update();
	}
	else
	if( id == ID_VIEW_TOOLBAR_WIDGETS )
	{
		if( m_gameMenu->FindItem( id )->IsChecked() )
		{
			m_auiMgr.GetPane( m_widgets ).Show();
		}
		else
		{
			m_auiMgr.GetPane( m_widgets ).Hide();
		}

		m_auiMgr.Update();
	}
	else
	if( id == ID_VIEW_TOOLBAR_WIDGETP4 )
	{
		if( m_gameMenu->FindItem( id )->IsChecked() )
		{
			m_auiMgr.GetPane( m_widgetP4 ).Show();
		}
		else
		{
			m_auiMgr.GetPane( m_widgetP4 ).Hide();
		}

		m_auiMgr.Update();
	}
	else
	if( id == ID_VIEW_SHOW_ALL )
	{
		for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
			UpdateToolWindowState( it->m_first, true, it->m_second.isFloat, false );
		RefreshToolWindows();
	}
	else
	if( id == ID_VIEW_HIDE_ALL )
	{
		for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
			UpdateToolWindowState( it->m_first, false, it->m_second.isFloat, false );
		RefreshToolWindows();
	}
	else
	if( id == ID_VIEW_FLOAT_ALL )
	{
		for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
			UpdateToolWindowState( it->m_first, it->m_second.isVisible, true, false );
		RefreshToolWindows();
	}
	else
	if( id == ID_VIEW_DEFLOAT_ALL )
	{
		for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
			UpdateToolWindowState( it->m_first, it->m_second.isVisible, false, false );
		RefreshToolWindows();
	}
	else
	if( 
		id == ID_VIEW_SCENE || 
		id == ID_VIEW_TOOLS || 
		id == ID_VIEW_PATHS || 
		id == ID_VIEW_PROPERTIES ||
		id == ID_VIEW_BRUSHES ||
		id == ID_VIEW_ENTITIES ||
		id == ID_VIEW_WORLD )
	{
		UpdateToolWindowState( id, m_gameMenu->FindItem( id )->IsChecked(), m_toolWindowState.GetRef( id ).isFloat );
	}
	else
	if( 
		id == ID_VIEW_SCENE+1 || 
		id == ID_VIEW_TOOLS+1 || 
		id == ID_VIEW_PATHS+1 || 
		id == ID_VIEW_PROPERTIES+1 ||
		id == ID_VIEW_BRUSHES+1 ||
		id == ID_VIEW_ENTITIES+1 || 
		id == ID_VIEW_WORLD+1 )

	{
		UpdateToolWindowState( id-1, m_toolWindowState.GetRef( id-1 ).isVisible, !m_gameMenu->FindItem( id )->IsChecked() );
	}
    else
	if( 
		id == ID_VIEW_SCENE+2 || 
		id == ID_VIEW_TOOLS+2 || 
		id == ID_VIEW_PATHS+2 || 
		id == ID_VIEW_PROPERTIES+2 ||
		id == ID_VIEW_BRUSHES+2 ||
		id == ID_VIEW_ENTITIES+2 ||
		id == ID_VIEW_WORLD+2 )

	{
		UpdateToolWindowState( id-2, true, !m_gameMenu->FindItem( id-1 )->IsChecked() );
	}
}

void CEdFrame::OnPopupNotificationLocationChanged( wxCommandEvent& event )
{
	switch ( event.GetId() )
	{
	case ID_VIEW_NOTIFICATION_CENTER:
		SEdPopupNotification::GetInstance().SetLocation( CEdPopupNotification::CENTER );
		break;
	case ID_VIEW_NOTIFICATION_TOP_LEFT:	
		SEdPopupNotification::GetInstance().SetLocation( CEdPopupNotification::TOP_LEFT );
		break;
	case ID_VIEW_NOTIFICATION_TOP_RIGHT:
		SEdPopupNotification::GetInstance().SetLocation( CEdPopupNotification::TOP_RIGHT );
		break;
	case ID_VIEW_NOTIFICATION_BOTTOM_LEFT:
		SEdPopupNotification::GetInstance().SetLocation( CEdPopupNotification::BOTTOM_LEFT );
		break;
	case ID_VIEW_NOTIFICATION_BOTTOM_RIGHT:
		SEdPopupNotification::GetInstance().SetLocation( CEdPopupNotification::BOTTOM_RIGHT );
		break;
	}
}

void CEdFrame::OnViewportCachetAspectRatioChanged( wxCommandEvent& event )
{
	switch ( event.GetId() )
	{
	case ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_9:
		GetWorldEditPanel()->SetViewportCachetAspectRatio(IViewport::EAspectRatio::FR_16_9);
		break;
	case ID_VIEW_VIEWPORT_CACHETASPECTRATIO_16_10:	
		GetWorldEditPanel()->SetViewportCachetAspectRatio(IViewport::EAspectRatio::FR_16_10);
		break;
	case ID_VIEW_VIEWPORT_CACHETASPECTRATIO_4_3:
		GetWorldEditPanel()->SetViewportCachetAspectRatio(IViewport::EAspectRatio::FR_4_3);
		break;
	case ID_VIEW_VIEWPORT_CACHETASPECTRATIO_21_9:
		GetWorldEditPanel()->SetViewportCachetAspectRatio(IViewport::EAspectRatio::FR_21_9);
		break;
	case ID_VIEW_VIEWPORT_CACHETASPECTRATIO_NONE:
		GetWorldEditPanel()->SetViewportCachetAspectRatio(IViewport::EAspectRatio::FR_NONE);
		break;
	}
}

void CEdFrame::OnViewFOV( wxCommandEvent& event )
{
	String value = ToString( GetWorldEditPanel()->GetCameraFov() );
	if ( InputBox( this, TXT("Set field of view"), TXT("Enter new field of view:"), value ) )
	{
		Float fov;
		if ( FromString( value, fov ) )
		{
			if ( fov > 0 && fov < 180 )
			{
				GetWorldEditPanel()->SetCameraFov( fov );
			}
		}
	}
}

void CEdFrame::OnOverlayVertexSprites( wxCommandEvent& event )
{
	extern Bool GVertexEditorEntityOverlay;
	GVertexEditorEntityOverlay = event.IsChecked();
}

void CEdFrame::OnListEntitiesAroundCamera( wxCommandEvent& event )
{
	if ( GGame->GetActiveWorld() == nullptr )
	{
		wxMessageBox( wxT("No world"), wxT("Error"), wxICON_ERROR|wxOK );
		return;
	}

	// Get max distance
	static Float maxDistance = 16;
	static Bool addDynamicLayer = true;
	if ( FormattedDialogBox( wxT("Entities Around Camera"), wxT("H{'Maximum distance (0 for all):'|F}|H{~X'Include entities in the dynamic layer (game only)'}||H{~B@'OK'|B'Cancel'}"), &maxDistance, &addDynamicLayer ) != 0 )
	{
		return;
	}

	// Collect entities
	TDynArray< CEntity* > entities;
	for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CEntity* entity = *it;
		Vector worldPos = entity->GetWorldPositionRef();

		// Skip entities in dynamic layer if necessary
		if ( !addDynamicLayer && entity->GetLayer() == GGame->GetActiveWorld()->GetDynamicLayer() )
		{
			continue;
		}

		if ( maxDistance < 0.00001f || worldPos.DistanceTo( GGame->GetActiveWorld()->GetCameraPosition() ) <= maxDistance )
		{
			entities.PushBack( entity );
		}
	}

	// Check if we found any
	if ( entities.Empty() )
	{
		wxMessageBox( wxT("Entities not found"), wxT("No entities"), wxOK );
		return;
	}

	// Enlist them
	ClearEntityList( TXT("Entities Around Camera") );
	AddEntitiesToEntityList( TXT("Entities Around Camera"), entities );
}

void CEdFrame::UpdateToolWindowState( Int32 wnd, Bool isVisible, Bool isFloat, bool refresh )
{
	bool _refresh = false;
	CToolWindowState *state = m_toolWindowState.FindPtr( wnd );
	if ( state )
	{
		if( state->isFloat != isFloat )
		{
			state->isFloat = isFloat;
			m_gameMenu->FindItem( wnd+1 )->Check( !isFloat );
			if( isFloat )
			{
				for( Uint32 i=0; i<m_solution->GetPageCount(); i++ )
					if( m_solution->GetPage( i ) == state->widget )
					{
						m_solution->RemovePage( i );
						break;
					}
				m_auiMgr.AddPane( state->widget, wxAuiPaneInfo().
					Name(state->caption).Caption(state->caption).
					Floatable( true ).Dockable(true).Layer(1).Movable(true).Float().DestroyOnClose(false).CloseButton(false));
			}
			else
			{
				m_auiMgr.DetachPane( state->widget );
				state->widget->Reparent( m_solution );
				m_solution->AddPage( state->widget, state->caption );
			}
			_refresh = true;
		}

		if( state->isVisible != isVisible )
		{
			state->isVisible = isVisible;
			m_gameMenu->FindItem( wnd )->Check( isVisible );
			if( state->isFloat )
			{
				if( isVisible )
					m_auiMgr.GetPane( state->widget ).Show();
				else
					m_auiMgr.GetPane( state->widget ).Hide();
			}
			else
			{
				if( isVisible )
				{
					state->widget->Show();
					state->widget->Reparent( m_solution );
					m_solution->AddPage( state->widget, state->caption );
				}
				else
				{
					state->widget->Hide();
					for( Uint32 i=0; i<m_solution->GetPageCount(); i++ )
						if( m_solution->GetPage( i ) == state->widget )
						{
							m_solution->RemovePage( i );
							break;
						}
				}
			}
			_refresh = true;
		}
		else
		if( isVisible && !isFloat )
		{
			for( Uint32 i=0; i<m_solution->GetPageCount(); i++ )
				if( m_solution->GetPage( i ) == state->widget )
				{
					m_solution->SetSelection( i );
					break;
				}
		}

		if( _refresh && refresh )
			RefreshToolWindows();
	}
}

void CEdFrame::RefreshToolWindows()
{
	if( m_solution->GetPageCount() == 0 )
		m_auiMgr.GetPane( m_solution ).Hide();
	else
		m_auiMgr.GetPane( m_solution ).Show();
	m_auiMgr.Update();
	m_solution->Refresh( true );

    // Find shortcut for the Dock command
    wxAcceleratorEntry dockAccel(wxACCEL_CTRL, 'D', XRCID( "viewDock" ));
    TEdShortcutArray &shortcuts = *CEdFrame::GetAccelerators();
    for ( Uint32 i = 0; i < shortcuts.Size(); ++i )
    {
        SEdShortcut &shortcut = shortcuts[i];
        if (shortcut.m_acceleratorEntry.GetCommand() == XRCID( "viewDock" ))
        {
            dockAccel = shortcut.m_acceleratorEntry;
            break;
        }
    }
    wxAcceleratorEntry accels[] = { dockAccel };
    wxAcceleratorTable accelTbl(1, accels);
    
	Int32 visibles = 0, floats = 0;
	for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
	{
		if( it->m_second.isVisible )
			visibles ++;
		if( it->m_second.isFloat )
        {
			floats ++;

            wxAuiPaneInfo &pane_info = m_auiMgr.GetPane( it->m_second.widget );
            if (pane_info.frame)
            {
                pane_info.frame->SetAcceleratorTable(accelTbl);
                pane_info.frame->Disconnect(XRCID( "viewDock" ));
                pane_info.frame->Connect(XRCID( "viewDock" ), wxEVT_COMMAND_MENU_SELECTED,
                    wxCommandEventHandler( CEdFrame::OnSolutionDockWidget ), 0, this);
                /*
                ::GetSystemMenu( static_cast<HWND>( pane_info.frame->GetHWND() ), true );
                HMENU menu = ::GetSystemMenu( static_cast<HWND>( pane_info.frame->GetHWND() ), false );
                ::InsertMenu(menu, 0, MF_BYPOSITION, 1000, TXT("&Dock"));
                */
            }
        }
	}

	m_gameMenu->FindItem( ID_VIEW_SHOW_ALL )->Check( visibles == m_toolWindowState.Size() );
	m_gameMenu->FindItem( ID_VIEW_HIDE_ALL )->Check( visibles == 0 );
	m_gameMenu->FindItem( ID_VIEW_FLOAT_ALL )->Check( floats == m_toolWindowState.Size() );
	m_gameMenu->FindItem( ID_VIEW_DEFLOAT_ALL )->Check( floats == 0 );
}

void CEdFrame::OnToolWindowClose( wxCommandEvent& event )
{
	int id = event.GetId();
	if( m_toolWindowState.KeyExist( id ) )
	{
		CToolWindowState* state = m_toolWindowState.FindPtr( id );
		if ( state )
		{
			state->isVisible = false;
		}
	}
}

void CEdFrame::SetStatusBarText( wxString text, int ms )
{
	m_statusBarMsg = text;
	SetStatusText( m_statusBarMsg );
	m_statusBarTimer->Start( ms, true );
}

void CEdFrame::SetSecondStatusBarText( wxString text )
{
	SetStatusText( text, 1 );
}

void CEdFrame::DisplayCameraTransform( wxString text )
{
	if ( GetEditorUserConfig().m_displayCameraTransform )
		SetStatusText( text, 0 );
}

void CEdFrame::UpdateSecondStatusBarText()
{
	String statusText = String::Printf( TXT("Sel Num: %d"), m_secondStatusBarInfo.m_selObjsNum );
	SetSecondStatusBarText( statusText.AsChar() );

}

void CEdFrame::OnStatusBarTimeout( wxCommandEvent& event )
{
	if( GetStatusBar()->GetStatusText() == m_statusBarMsg )
		SetStatusText( wxEmptyString );
}

void CEdFrame::ResaveAllFiles( wxCommandEvent& event )
{
	CDirectory *startDir =  GDepot;

	TQueue< CDirectory* > dirs;
	dirs.Push( startDir );
	
	while( !dirs.Empty() )
	{
		CDirectory *currDir = dirs.Front();
		dirs.Pop();

		const TFiles & files = currDir->GetFiles();
		for( TFiles::const_iterator it = files.Begin(); it != files.End(); ++it )
		{
			CDiskFile* currFile = *it;

			const String &depotFileName = currFile->GetDepotPath();
			CResource *currResource = GDepot->LoadResource( depotFileName );
			if ( !currResource )
			{
				LOG_EDITOR( TXT("Failed to load %s file"), depotFileName.AsChar() );
			}
			else
			{
				LOG_EDITOR( TXT("Resaving %s file"), depotFileName );
				currResource->Save();
			}
		}

		for ( CDirectory * pDir : currDir->GetDirectories() )
		{
			dirs.Push( pDir );
		}
	}
}

void CEdFrame::OnEraseBackground(wxEraseEvent& event)
{
	event.Skip();
}

void CEdFrame::OnSize(wxSizeEvent& event)
{
    event.Skip();
}

void CEdFrame::OnMove(wxMoveEvent& event)
{
	if (IsIconized())
	{
		if ( GLogOutput )
		{
			GLogOutput->Minimize();
		}
	}
	else
	{
		if ( GLogOutput )
		{
			GLogOutput->ShowNormal();
		}
	}
	event.Skip();
}

void CEdFrame::OnExit( wxCommandEvent& event )
{	
	Close();
}

bool CEdFrame::BeforeCloseQuery()
{
	// Get modified files
	TDynArray< CDiskFile * > files;
	{
		// Get modified ones
		for ( ObjectIterator< CResource > it; it; ++it )
		{
			CResource* resource = (*it);
			if ( resource->GetFile() && resource->GetFile()->IsModified() )
			{
				files.PushBack( resource->GetFile() );
			}
		}
	}

	if( files.Size() == 1 )
	{
		wxString msg;
		msg.sprintf( TXT("File %s has been modified. Save changes?"), files[ 0 ]->GetDepotPath().AsChar() );
		wxMessageDialog dialog( this, msg, TXT("Modified file found!"), wxYES_NO | wxCANCEL );
		Int32 ret = dialog.ShowModal();
		if ( ret == wxID_YES )
		{
			Bool ret;
			CLayer *layer = Cast< CLayer >( files[ 0 ]->GetResource() );
			if( layer )
			{
				ret = layer->GetLayerInfo()->Save();
			}
			else
			{
				ret = files[ 0 ]->Save();
			}
			
			if( !ret )
			{
				wxString msg;
				msg.sprintf( TXT("File %s cannot be saved. Quit anyway?"), files[ 0 ]->GetDepotPath().AsChar() );
				wxMessageDialog dialog( this, msg, TXT("Save error"), wxYES_NO | wxICON_ERROR  );
				Int32 ret = dialog.ShowModal();
				if( ret != wxID_YES )
				{
					return false;
				}
			}
		}
		else 
		if ( ret == wxID_CANCEL )
		{
			return false;
		}
	}
	else
	if( files.Size() > 1 )
	{
		CSaveChangedDlg dlg( this, files );
		ESaveChangedDlgReturn ret = dlg.DoModal();
		if( ret == ESC_Cancel )
		{
			return false;
		}
	}
	//SaveLayout();
	CUserConfigurationManager &configUser = SUserConfigurationManager::GetInstance();
	configUser.SaveAll( false );
	CCommonConfigurationManager &configCommon = SCommonConfigurationManager::GetInstance();
	configCommon.SaveAll( false );

	return true;
}

void CEdFrame::OnCloseWindow( wxCloseEvent& event )
{
	if ( BeforeCloseQuery() )
	{
		LOG_EDITOR( TXT("Main window closed" ) );
		PostQuitMessage( 0 );
	}
}

void CEdFrame::OnActivateWindow( wxActivateEvent& event )
{
	if ( GGame && GGame->IsActive() && ( m_fullScreenFrame == NULL ) )
	{
		if ( event.GetActive() == false )
		{
			GGame->Stop();
		}
		else
		{
			GGame->Unstop();
		}
	}

	// Set the undo manager for the undo history window to our manager
	if ( event.GetActive() )
	{
		SetUndoHistoryFrameManager( m_undoManager );
		// If something set to true pop the windows up front then make it false
	}
	event.Skip();
}

void CEdFrame::OnAbout( wxCommandEvent& event )
{
	CEdAboutDlg* about = new CEdAboutDlg( this );
	about->Show();
}

void CEdFrame::OnSelectDLC( wxCommandEvent& event )
{
	if ( !GGame || GGame->IsActive() )
	{
		wxMessageBox( wxT("Unable to configure DLCs while the game is running"), wxT("Error"), wxICON_ERROR | wxOK, this );
		return;
	}

	CEdDLCListDlg dlg( this );
	dlg.ShowModal();
}

void CEdFrame::OnShowXMLErrors( wxCommandEvent& event )
{
	OnXMLErrors();
}

void CEdFrame::OnWhatsNew()
{
	RunLaterOnce( []() {
		CEdWhatsNewDlg* whatsnew = new CEdWhatsNewDlg( wxTheFrame, false );
		if( whatsnew->m_fromChangeList != whatsnew->m_updatedToChangeList && whatsnew->m_showText )
		{				
			SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("DontShowAgain"), false );
			whatsnew->Show();
		}
		else
		{
			whatsnew->Destroy();
		}
	} );
}

void CEdFrame::OnXMLErrors()
{
	if ( !m_xmlErrorsDisplayer )
	{
		m_xmlErrorsDisplayer = new CEdXMLErrorsDisplayer( wxTheFrame );
	}

	RunLaterOnce( [ this ]() {
		if ( m_xmlErrorsDisplayer )
		{
			m_xmlErrorsDisplayer->Execute();
		}
	});
}

void CEdFrame::OnWhatsNewForced( wxCommandEvent& event )
{
	CEdWhatsNewDlg* whatsnew = new CEdWhatsNewDlg( wxTheFrame, true );
	SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("Editor"), TXT("DontShowAgain"), false );
	whatsnew->Show();
}

wxBitmap LoadToolIcon( String dialogPath )
{
	wxIcon icon;
	wxBitmap bmpIcon;

	// Try load *.ico
	wxString iconPath = (dialogPath + TXT(".ico")).AsChar();
	if ( icon.LoadFile( iconPath, wxBITMAP_TYPE_ICO, 16, 16 ) )
	{
		bmpIcon.CopyFromIcon ( icon );
	}
	else
	{
		// Try load *.png
		iconPath = (dialogPath + TXT(".png")).AsChar();
		bmpIcon.LoadFile( iconPath, wxBITMAP_TYPE_PNG );
	}

	return bmpIcon;
}

void CEdFrame::BuildToolsMenu( wxMenu* parent, String path )
{
	// Add *.xrc as dialogs
	TDynArray <String> files;
	GFileManager->FindFiles( path, TXT("*.xrc"), files, false );
	for ( TDynArray <String>::iterator it = files.Begin(); it != files.End(); ++it )
	{
		// Add new menu item
		wxMenuItem* menuItem = new wxMenuItem( parent, wxID_ANY, (CFilePath((*it)).GetFileName()).AsChar() );		
		menuItem->SetBitmap( LoadToolIcon( path + CFilePath((*it)).GetFileName() ) );
		parent->Append( menuItem );

		// Connect event handler
		Connect( menuItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CEdFrame::OnLaunchTool) );
	}

	// Add directories as submenus
	TDynArray <String> dirs;
	GFileManager->FindDirectories( path, dirs );
	for ( TDynArray <String>::iterator it = dirs.Begin(); it != dirs.End(); ++it )
	{
		// Add new sub menu
		wxMenu* subMenu = new wxMenu( );
		parent->AppendSubMenu( subMenu, (*it).AsChar() );
		
		// Recursive fill submenu
		BuildToolsMenu( subMenu, path + (*it) + TXT("\\") );
	}
}

// Menu Helper - Return parent item of menu item
static wxMenuItem* GetMenuParentItem( wxMenuItem* item )
{
	wxMenu* subMenu = item->GetMenu();
	if ( subMenu )
	{	
		wxMenu* parent = subMenu->GetParent();
		if ( parent )
		{
			for ( Uint32 i=0 ; i < parent->GetMenuItemCount(); i++ )
			{
				wxMenuItem* menuItem = parent->GetMenuItems()[i];
				if ( menuItem->IsSubMenu() && menuItem->GetSubMenu() == subMenu )
				{
					return menuItem;
				}
			}
		}
	}

	return NULL;
}

void CEdFrame::OnLaunchTool( wxCommandEvent& event )
{
	wxMenuItem* menuItem = m_gameMenu->FindItem( event.GetId() );

	wxString label =  menuItem->GetItemLabelText();

	// Build path from menu
	wxString filePath = menuItem->GetItemLabelText();
	wxMenuItem* parent = GetMenuParentItem( menuItem );
	while ( parent )
	{
		filePath = parent->GetItemLabelText() + TXT("\\") + filePath ;
		parent = GetMenuParentItem( parent );
	}
}

void CEdFrame::OnReloadScripts( wxCommandEvent& event )
{
	GEngine->RequestScriptsReload();
}

void CEdFrame::OnReloadResourceDefinitions( wxCommandEvent& event )
{
	SResourceDefManager::GetInstance().LoadAllDefinitions();
}

void CEdFrame::OnReloadGameDefinitions( wxCommandEvent& event )
{	
	InventoryEditor::RelodDefinitions();
}

void CEdFrame::OnAssetBrowser( wxCommandEvent& event )
{
    m_assetBrowser->Show();
    m_assetBrowser->SetFocus();
}

void CEdFrame::OnSnapChange( wxCommandEvent& event )
{
	wxComboBox* snapCombo = XRCCTRL( *this, "snapCombo", wxComboBox );
	Int32 snap = snapCombo->GetSelection();
	if ( GGame->GetActiveWorld() )
	{
		if ( snap == 0 )
		{
			m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToNothing );
		}
		else 
		{
			if ( snap == 1 )
			{
				m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToTerrainVisual );
			}
			else if ( snap == 2 )
			{
				m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToTerrainPhysical );
			}
			else if ( snap == 3 )
			{
				m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToStaticCollision );
			}
			m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, -0.5f ) );
			m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, 0.5f ) );
		}
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSnapNone( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToNothing );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSnapTerrainVisual( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToTerrainVisual );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, -0.5f ) );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, 0.5f ) );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSnapTerrainPhysical( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToTerrainPhysical );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, -0.5f ) );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, 0.5f ) );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSnapCollision( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		m_viewport->GetTransformManager()->SetSnapMode( SNAP_ToStaticCollision );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, -0.5f ) );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, 0.5f ) );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSnapPivot( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		m_viewport->GetTransformManager()->SetSnapOrgin( SNAP_ByPivot );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, -0.5f ) );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, 0.5f ) );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSnapBoundingVolume( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		m_viewport->GetTransformManager()->SetSnapOrgin( SNAP_ByBoundingVolume );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, -0.5f ) );
		m_viewport->GetTransformManager()->MoveSelection( Vector( 0.0, 0.0f, 0.5f ) );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnVDB( wxCommandEvent& event )
{
#ifndef PHYSICS_RELEASE
	if ( GPhysicsDebugger->IsAttached() )
	{
		GPhysicsDebugger->DetachFromWorld();
		return;
	}

	wxMenu menu;
	menu.Append( ID_CONNECT_VDB, TXT("Main world") );

	m_mainToolbar->ToggleTool( XRCID("editorVDB"), false );
	
	wxTheFrame->PopupMenu( &menu );
#endif
}

void CEdFrame::OnSimulate( wxCommandEvent& event )
{
	/*if ( GGame == NULL || GGame->GetActiveWorld() == NULL || GGame->GetActiveWorld()->GetSelectionManager() == NULL || GGame->GetActiveWorld()->GetPhysicsWorld() == NULL )
	{
		return;
	}

	if ( wxMessageBox( TXT("Are you sure?\n\nThis operation can not be undone, so it's better you have all layers saved. Proceed?"), TXT("Are you sure?"), wxYES_NO ) == wxYES )
	{
		TDynArray< CEntity* > selection;
		GGame->GetActiveWorld()->GetSelectionManager()->GetSelectedEntities( selection );
		GGame->GetActiveWorld()->GetPhysicsWorld()->SimualteSelection( selection );

		m_mainToolbar->ToggleTool( XRCID("simulate"), true );
	}
	else
	{
		m_mainToolbar->ToggleTool( XRCID("simulate"), false );
	}*/
}

void CEdFrame::OnVDBMenu( wxCommandEvent& event )
{
#ifndef PHYSICS_RELEASE
	CPhysicsWorld *phWorld = NULL;

	if( !GGame->GetActiveWorld() ) return;

	if ( event.GetId() == ID_CONNECT_VDB )
	{
		 GGame->GetActiveWorld()->GetPhysicsWorld( phWorld );
	}

	if ( phWorld == NULL )
	{
		return;
	}

	m_mainToolbar->ToggleTool( XRCID("editorVDB"), true );

	if ( GGame->GetActiveWorld() )
	{
		Int32 errorCode = GPhysicsDebugger->AttachToWorld( phWorld );
		if ( errorCode )
		{
			LOG_EDITOR( TXT("Error code: %ld"), errorCode );
			GPhysicsDebugger->DetachFromWorld();
			::wxMessageBox( TXT("Server creation failed. Check the log for details."), TXT("Error") );
		}
	}
#endif
}

void CEdFrame::OnStamper( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
		return;
	CSelectionManager* selectionMgr = m_viewport->GetSelectionManager();
	TDynArray< CNode* > nodes;
	selectionMgr->GetSelectedNodes( nodes );
	for( TDynArray< CNode* >::iterator it=nodes.Begin(); it!=nodes.End(); it++ )
	{
		CEntity *entity = Cast< CEntity >( *it );
		if( entity )
		{
			Box boundingBox = entity->CalcBoundingBox();
		}
	}
}

void CEdFrame::OnNewWorld( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
    // Remember world switch
    if ( GGame->GetActiveWorld() )
    {
        config.Write( TXT("/Global/LastSession"), GGame->GetActiveWorld()->DepotPath() );
    }
	config.SaveSession();

	// Ask for new world file
	String tag = (ClassID< CWorld >())->GetDefaultObject<CWorld>()->GetFriendlyName();
	m_mapDialog.SetIniTag( tag );
	if ( m_mapDialog.DoSave( (HWND)GetHandle(), NULL, true ) )
	{
		// Translate absolute path to local depot path
		String localDepotPath;
		if ( !GDepot->ConvertToLocalPath( m_mapDialog.GetFile(), localDepotPath ) )
		{
			WARN_EDITOR( TXT("Unable to load world in because path is not under depot") );
		}

		// Close all world related windows
		CloseAllWorldRelatedWindows();

		// Create world
		GGame->CreateWorld( localDepotPath );

        // Restore session
		if ( GGame->GetActiveWorld() )
		{
			config.SetSessionPath( GGame->GetActiveWorld()->DepotPath() );
			ISavableToConfig::RestoreSession();
		}

		UpdateWidgetUI();

		// Add world to the recent list
		if ( GGame->GetActiveWorld() )
		{
			StoreWorldChoice( GGame->GetActiveWorld()->DepotPath() );
		}

		// Rescan directories
		GetAssetBrowser()->UpdateDepotTree();
	}
}

void CEdFrame::OnRestoreWorld( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	String depot = config.Read( TXT("/Global/LastSession"), String::EMPTY );
	if( depot.Empty() )
		return;

	// Load world
	ASSERT( GGame );
	if (!LoadWorld( depot ))
	{
		return;
	}

	UpdateWidgetUI();
}

void CEdFrame::OnReloadSoundbanks( wxCommandEvent& event )
{
	GSoundSystem->ReloadSoundbanks();
}

void CEdFrame::OnApexDestructionStatistics( wxCommandEvent& event )
{
	TDynArray< String > paths;
	GDepot->FindResourcesByExtension( TXT( "redapex" ), paths );
	C2dArray* array = C2dArray::FactoryInfo< C2dArray >().CreateResource();
	array->AddToRootSet();
	String names( TXT("Asset;BufferSize;LoadTime;CollisionChunkCount;CollisionFacesCount;CollisionChunkBytes;CollisionChunkHullDataBytes;RenderTotalBytes;RenderSubmeshCount;RenderPartCount;RenderVertexCount;RenderIndexCount;") );
	array->ParseData( names );
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CApexDestructionResource* res = Cast< CApexDestructionResource >( GDepot->LoadResource( paths[ i ] ) );
		if( !res ) continue;

		CompiledCollisionPtr collision;
		RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
		GCollisionCache->Compile_Sync( collision, res, res->GetDepotPath(), res->GetFileTime() );

		array->AddRow();
		auto & geometry = collision->GetGeometries();
		array->SetValue( collision ? String::Printf( TXT( "%i" ), geometry[ 0 ].GetCompiledDataSize() ) : String( TXT( "-1" ) ), String( TXT("BufferSize") ), array->GetNumberOfRows() - 1 );
		array->SetValue( res->GetFriendlyName(), String( TXT("Asset") ), array->GetNumberOfRows() - 1 );
		res->FillStatistics( array );
		SGarbageCollector::GetInstance().CollectNow();
	}
	m_assetBrowser->EditAsset( array );
	String depotPath;
	GDepot->GetDepotPath( depotPath );
	array->SaveAs( GDepot->FindPath( depotPath.AsChar() ), String::Printf( TXT( "apexDestructionStatistics %f.csv" ), ( Float ) EngineTime::GetNow() ), true );
	array->RemoveFromRootSet();
}

void CEdFrame::OnApexClothStatistics( wxCommandEvent& event )
{
	TDynArray< String > paths;
	GDepot->FindResourcesByExtension( TXT( "redcloth" ), paths );
	C2dArray* array = C2dArray::FactoryInfo< C2dArray >().CreateResource();
	array->AddToRootSet();
	String names( TXT("Asset;BufferSize;LoadTime;RenderTotalBytes;RenderSubmeshCount;RenderPartCount;RenderVertexCount;RenderIndexCount;") );
	array->ParseData( names );
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CApexClothResource* res = Cast< CApexClothResource >( GDepot->LoadResource( paths[ i ] ) );
		if( !res ) continue;
		CompiledCollisionPtr collision;

		RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
		GCollisionCache->Compile_Sync( collision, res, res->GetDepotPath(), res->GetFileTime() );

		array->AddRow();
		auto &geometry = collision->GetGeometries();
		array->SetValue( collision ? String::Printf( TXT( "%i" ), geometry[ 0 ].GetCompiledDataSize() ) : String( TXT( "-1" ) ), String( TXT("BufferSize") ), array->GetNumberOfRows() - 1 );
		array->SetValue( res->GetFriendlyName(), String( TXT("Asset") ), array->GetNumberOfRows() - 1 );
		res->FillStatistics( array );
		SGarbageCollector::GetInstance().CollectNow();
	}
	m_assetBrowser->EditAsset( array );
	String depotPath;
	GDepot->GetDepotPath( depotPath );
	array->SaveAs( GDepot->FindPath( depotPath.AsChar() ), String::Printf( TXT( "apexClothStatistics %f.csv" ), ( Float ) EngineTime::GetNow() ), true );
	array->RemoveFromRootSet();
}

void CEdFrame::OnGeneratePhysxGeometryStatisticsFromCollisionCache( wxCommandEvent& event )
{
	C2dArray* array = GCollisionCache->DumpStatistics();
	if ( array )
	{
		m_assetBrowser->EditAsset( array );
		String depotPath;
		GDepot->GetDepotPath( depotPath );
		array->SaveAs( GDepot->FindPath( depotPath.AsChar() ), String::Printf( TXT( "collisionCacheStatistics %f.csv" ), ( Float ) EngineTime::GetNow() ), true );
		SGarbageCollector::GetInstance().CollectNow();
	}
}

void CEdFrame::OnFillCollisionCacheWithEveryMeshInLocalDepotCollision( wxCommandEvent& event )
{
	TDynArray< String > paths;
	GDepot->FindResourcesByExtension( TXT( "w2ter" ), paths );

	THandle< CWorld > currentWorld;
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CTerrainTile* terrainResource = Cast< CTerrainTile >( GDepot->LoadResource( paths[ i ] ) );

		String path = terrainResource->GetFile()->GetDepotPath();

		size_t pos = 0;

		if( !path.FindSubstring( TXT( "levels" ), pos ) || pos > 0 )
		{
			continue;
		}

		path.FindSubstring( TXT( "terrain_tiles" ), pos );
		String worldPath = path.LeftString( pos );

		String worldDepotPath;
		if( currentWorld.Get() && currentWorld.Get()->GetFile() )
			worldDepotPath = currentWorld.Get()->GetFile()->GetDepotPath();

		if( !worldDepotPath.FindSubstring( worldPath, pos ) )
		{
			currentWorld = nullptr;
			SGarbageCollector::GetInstance().CollectNow();
		}

		terrainResource = Cast< CTerrainTile >( GDepot->LoadResource( path ).Get() );

		if( !currentWorld.Get() )
		{
			static TDynArray< String > worldPaths;
			if( worldPaths.Empty() )
			{
				GDepot->FindResourcesByExtension( TXT( "w2w"), worldPaths );
			}

			for( auto i = worldPaths.Begin(); i != worldPaths.End(); ++i )
			{
				if( i->FindSubstring( worldPath, pos ) )
				{
					currentWorld = Cast< CWorld >( GDepot->LoadResource( *i ) );
					break;
				}
			}
		}

		if( !currentWorld.Get() ) continue;;

		CClipMap* clipMap = Cast< CWorld >( currentWorld.Get() )->GetTerrain();

		if( !clipMap ) continue;

		static Uint16 counter = 0;
		CompiledCollisionPtr compiledMesh;
		++counter;
		GCollisionCache->Compile_Sync( compiledMesh, terrainResource, terrainResource->GetDepotPath(), terrainResource->GetFileTime(), clipMap );

		if( counter > 10 )
		{
			counter = 0;
			GCollisionCache->Flush();
			SGarbageCollector::GetInstance().CollectNow();
		}
	}

	GDepot->FindResourcesByExtension( TXT( "w2mesh" ), paths );
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CMesh* res = Cast< CMesh >( GDepot->LoadResource( paths[ i ] ) );
		if( !res ) continue;

		CCollisionMesh* collisionMesh = const_cast< CCollisionMesh* >( res->GetCollisionMesh() );
		if ( collisionMesh )
		{
			CompiledCollisionPtr compiledMesh;

			RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
			GCollisionCache->Compile_Sync( compiledMesh, collisionMesh, res->GetDepotPath(), res->GetFileTime() );
		}
		SGarbageCollector::GetInstance().CollectNow();
	}

	// apex destruction resoure
	paths.Clear();
	GDepot->FindResourcesByExtension( TXT( "redapex" ), paths );
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CApexResource* res = Cast< CApexResource >( GDepot->LoadResource( paths[ i ] ) );
		if( !res ) continue;
		res->DecideToIncludeToCollisionCache();
		SGarbageCollector::GetInstance().CollectNow();
	}

	// apex clothing resource
	paths.Clear();
	GDepot->FindResourcesByExtension( TXT( "redcloth" ), paths );
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CApexResource* res = Cast< CApexResource >( GDepot->LoadResource( paths[ i ] ) );
		if( !res ) continue;
		res->DecideToIncludeToCollisionCache();
		SGarbageCollector::GetInstance().CollectNow();
	}

	// physx destruction resoure
	paths.Clear();
	GDepot->FindResourcesByExtension( TXT( "reddest" ), paths );
	for( Uint32 i = 0; i != paths.Size(); ++i )
	{
		CPhysicsDestructionResource* res = Cast< CPhysicsDestructionResource >( GDepot->LoadResource( paths[ i ] ) );
		if( !res ) continue;
		res->DecideToIncludeToCollisionCache();
		SGarbageCollector::GetInstance().CollectNow();
	}

};

void CEdFrame::CloseAllWorldRelatedWindows()
{
	if ( m_worldSceneDebuggerWindow )
	{
		m_worldSceneDebuggerWindow->Destroy();
		m_worldSceneDebuggerWindow = nullptr;
	}
}

void CEdFrame::OnCloseWorld( wxCommandEvent& event )
{
	if(GGame && GGame->IsActive())
	{
		ASSERT( false && "Cannot close world while game is running" );
		return;
	}

	// Make sure that the user *really* wants to close the world
	if ( wxMessageBox( wxT("Do you REALLY want to close the current world?"), wxT("I mean... really?"), wxCENTRE|wxYES_NO|wxICON_QUESTION, this ) != wxYES )
	{
		return;
	}

	// Stop the shitty update timer
	PauseConfigTimer();

	// Prevent deleting render scene until render thread deals with proxies processing
	// TODO: it may be bigger issue and should be examined on cook
	IRenderScene* renderScene = NULL;
	if ( GGame && GGame->GetActiveWorld() )
	{
		renderScene = GGame->GetActiveWorld()->GetRenderSceneEx();
		if ( renderScene )
		{
			renderScene->AddRef();
		}
	}

	// Close all world related windows
	CloseAllWorldRelatedWindows();

	// Unload world
	ASSERT( GGame );
	GGame->UnloadWorld();
	m_viewport->Refresh( true );

	if ( m_sceneExplorer )
	{
		m_sceneExplorer->ClearPresets();
	}

	// Wait until all proxies destruction commands are processed and then
	// release scene: it means remove all proxies and delete render scene
	if ( renderScene )
	{
		GRender->Flush();
		renderScene->Release();
	}

	#ifndef NO_MARKER_SYSTEMS
		GEngine->GetMarkerSystems()->SendRequest( MSRT_ReleaseData );
	#endif

	ResumeConfigTimer();
}

void CEdFrame::OnOpenWorld( wxCommandEvent& event )
{
	// Ask for world file
	String tag = (ClassID< CWorld >())->GetDefaultObject<CWorld>()->GetFriendlyName();
	m_mapDialog.SetIniTag( tag );

	if ( m_mapDialog.DoOpen( (HWND)GetHandle(), true ) )
	{
		// Translate absolute path to local depot path
		String localDepotPath;
		if ( !GDepot->ConvertToLocalPath( m_mapDialog.GetFile(), localDepotPath ) )
		{
			WARN_EDITOR( TXT("Unable to load world in because path is not under depot") );
		}

		OpenWorld( localDepotPath );
	}
}

void CEdFrame::OnProgressDialogTimeout( wxCommandEvent& event )
{
}

void CEdFrame::OnCancelLoad( wxCommandEvent& event )
{

}

// Filter for loading only the layers not hidden in user session
class UserSessionHiddenLayersFilter : public IWorldLoadingFilter
{
protected:
	String m_worldPath;
    String m_activePreset;
    TDynArray< String > m_layersToLoad;
    TDynArray< String > m_layersToShow;

public:
	//! Constructor
	UserSessionHiddenLayersFilter( const String& worldPath )
		: m_worldPath( worldPath )
	{
		String absoluteConfigFilePath = GFileManager->GetBaseDirectory() + GGameConfig::GetInstance().GetName() + TXT("LavaEditor2.presets.ini");
		wxString configFileName = absoluteConfigFilePath.AsChar();
        wxFileConfig *config = new wxFileConfig( wxTheApp->GetAppName(), wxT("CDProjektRed"), configFileName, wxEmptyString, wxCONFIG_USE_LOCAL_FILE );

        String path = TXT("Session/") + m_worldPath;
        config->SetPath( path.AsChar() );

        // Read active preset
        m_activePreset = config->Read( wxT("ActivePreset"), wxT("Default") ).wc_str();
        if ( !m_activePreset.Empty() )
        {
            String preset_loaded = TXT("Presets/") + m_activePreset + TXT("_loaded");
            String preset_visible = TXT("Presets/") + m_activePreset + TXT("_visible");
            String layersToLoad = config->Read( preset_loaded.AsChar(), wxEmptyString ).wc_str();
            String layersToShow = config->Read( preset_visible.AsChar(), wxEmptyString ).wc_str();
            m_layersToLoad = layersToLoad.ToLower().Split( TXT(";") );
            m_layersToShow = layersToShow.ToLower().Split( TXT(";") );
        }

        delete config;
        config = NULL;
    };

	//! Filter layer
	Bool ShouldLoadLayer( CLayerInfo* layerInfo ) const
	{
		ASSERT( layerInfo );

        if ( m_activePreset.Empty() )
        {
            return false;
        }

        String layerPath = layerInfo->GetDepotPath().ToLower();
        Bool isLoaded = m_layersToLoad.Exist( layerPath );

        if ( !isLoaded )
        {
            return false;
        }

        Bool isVisible = m_layersToShow.Exist( layerPath );
        
/*
		// Get the config variable
		String configPath = TXT("/Session/") + m_worldPath + TXT("Scene");
		configPath += TXT("/Scene/Layers/");
		configPath += layerInfo->GetShortName();
		configPath += TXT("/Visible");

		// Read the visibility flag
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		Int32 isVisible = config.Read( configPath, 1 );
*/
		// Do not load layer if it's hidden in the user configuration
		if ( !isVisible )
		{
			// Hide the layer itself
			layerInfo->Show( false );

			// Do not load
			LOG_EDITOR( TXT("Layer '%s' loading filtered because of user config"), layerInfo->GetShortName() );
			return false;
		}

		// Load as usual
		return true;
	}
};

Bool CEdFrame::OpenWorld( const String &depotPath )
{
	// Add world to the recent list
	StoreWorldChoice( depotPath );

	// Close all world related windows
	CloseAllWorldRelatedWindows();

	// Load world
	ASSERT( GGame );
	if( !LoadWorld( depotPath ) )
	{
		return false;
	}

	// Update UI
	UpdateWidgetUI();

	return true;
}

Bool CEdFrame::LoadWorld( const String &depotPath )
{
	// Stop the shitty update timer
	PauseConfigTimer();

	// Save current session
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.SaveSession();

	// Setup loading context
	UserSessionHiddenLayersFilter filter( depotPath );
	WorldLoadingContext loadingContext;
	//loadingContext.m_dumpStats = true;
	//loadingContext.m_loadInitialLayers = true;
	loadingContext.m_useDependencies = true;
	loadingContext.m_layerFilter = &filter;

	// Load world
	if( !GGame->LoadWorld( depotPath, loadingContext ) )
	{
		// Loading failed from some reason
		ResumeConfigTimer();
		return false;
	}

	// World file has been edited
	if ( GGame->GetActiveWorld() )
	{
		// Set initial camera position & rotation
		GetWorldEditPanel()->m_cameraPosition.Set3( 20.0f, 20.0f, 20.0f );
		GetWorldEditPanel()->m_cameraRotation = EulerAngles( 0.0f, -20.0f, -45.0f );

		// Remember world switch
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		config.Write( TXT("/Global/LastSession"), GGame->GetActiveWorld()->DepotPath() );

		// Restore session
		config.SetSessionPath( GGame->GetActiveWorld()->DepotPath() );
		ISavableToConfig::RestoreSession();
		// Emit system wide event
	}

	//// Load review flag
	//#ifndef NO_MARKER_SYSTEMS
	//	GReviewSystem::GetInstance().OnLoadWorld(depotPath);
	//#endif

	// World loaded
	ResumeConfigTimer();
	return true;
}

void CEdFrame::OnOpenRecent( wxCommandEvent &event )
{
	// Load world
	ASSERT( GGame );
	if (!LoadWorld( m_recent[event.GetId()] ))
	{
		return;
	}

	if ( GGame->GetActiveWorld() )
	{
		CDiskFile* lastFile = GGame->GetActiveWorld()->GetFile();
		SEvents::GetInstance().DispatchEvent( CNAME( FileEdited ), CreateEventData< String >( lastFile->GetDepotPath() ) );
	}

	UpdateWidgetUI();

	StoreWorldChoice( m_recent[event.GetId()] );
}

void CEdFrame::StoreWorldChoice( String localDepotPath )
{
	const Uint32 maxRecentWorldsListLength = 6;

	// Clear menu from recent worlds list (currently we can't do does, as other frames holds their own copies of menu,
	// so the change would be visible only for the original)
	//wxMenu *fileMenu = this->m_gameMenu->GetMenu( 0 );
	//for ( THashMap< Int32, String >::const_iterator ci = m_recent.Begin();
	//	  ci != m_recent.End();
	//	  ++ci )
	//{
	//	fileMenu->Destroy( ci->m_first );
	//}
	//this->m_gameMenu->Refresh( );

	TDynArray< String > tmpRecentWorldsList;

	// Get config data
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Recent worlds") );
	
	TDynArray< TPair< String, String > > settings;
	config.AllSettings( settings );
	Sort( settings.Begin(), settings.End() );

	// Clear config data
	for ( Uint32 i = 0; i < settings.Size(); i++ )
	{
		tmpRecentWorldsList.PushBack( settings[i].m_second );
		config.DeleteEntry( ToString(i), false );
	}

	// Update data
	tmpRecentWorldsList.Remove( localDepotPath );
	tmpRecentWorldsList.Insert( 0, localDepotPath );
	while( tmpRecentWorldsList.Size() > maxRecentWorldsListLength )
		tmpRecentWorldsList.PopBack();

	// Save config data
	for ( Uint32 i = 0; i < tmpRecentWorldsList.Size(); i++ )
	{
		config.Write( ToString(i), tmpRecentWorldsList[i] );
	}
}

void CEdFrame::OnSaveWorld( wxCommandEvent& event )
{
	if ( GGame->GetActiveWorld() && m_sceneExplorer )
	{
		CLayerGroup* worldGroup = GGame->GetActiveWorld()->GetWorldLayers();
		if ( worldGroup )
		{
			TDynArray< CLayerInfo* > layers;
			worldGroup->GetLayers( layers, false );

			TDynArray< String > compatibilitiesErrors;
			CLayersEntityCompatibilityChecker::CheckLayersEntitiesCompatibility( layers, compatibilitiesErrors, false );
			DisplayErrorsList( TXT( "There are compatibility errors between some entities and layers:" ), 
				TXT( "Please, remove incompatible entities or change layers' types/build tags, otherwise LISTED LAYERS WON'T BE SAVED." ),
				compatibilitiesErrors );
		}
	}
	GGame->SaveWorld();
}

void CEdFrame::OnSaveWorldModifiedOnly( wxCommandEvent& event )
{
	if ( m_sceneExplorer )
	{
		m_sceneExplorer->SaveWorldModifiedOnly();
	}
}

void CEdFrame::OnClientSettings( wxCommandEvent &event )
{
	CEdClientDialog dialog( this );
	dialog.ShowModal();
}

void CEdFrame::OnDepartmentTags( wxCommandEvent &event )
{
	String tags;
	SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("SourceControl"), TXT("DepartmentTags"), tags );
	tags.Trim();
	if ( InputBox( this, TXT("Define Department Tags"), TXT("Enter your department's tags. Example: [Art][Models]"), tags ) )
	{
		tags.Trim();
		if ( tags.Empty() )
		{
			wxMessageBox( wxT("Cannot use empty tags"), wxT("No tags"), wxICON_ERROR|wxOK|wxCENTRE, this );
			return;
		}
		SUserConfigurationManager::GetInstance().WriteParam( TXT("User"), TXT("SourceControl"), TXT("DepartmentTags"), tags );
		SUserConfigurationManager::GetInstance().SaveAll();
	}
}

void CEdFrame::RelaunchEditor()
{
	CFilePath* name = GFileManager->GetApplicationFilename();
	::ShellExecute( NULL, TXT("open"), name->ToString().AsChar(), TXT(""), name->GetPathString().AsChar(), SW_SHOW );
		
	// Kill the current process (avoids the possible crash at closing)
	::TerminateProcess( ::GetCurrentProcess(), 0 );
}

void CEdFrame::OnRestartWithGame( wxCommandEvent &event )
{
	if ( BeforeCloseQuery() )
	{
		// Create path to lasteditorgame.txt
		String path = GFileManager->GetBaseDirectory( ) + TXT("lasteditorgame.txt");
		
		// Delete it
		_wunlink( path.AsChar() );
		
		// Relaunch the editor
		RelaunchEditor();
	}
}

void CEdFrame::OnUpdateUI( wxUpdateUIEvent& event )
{	
	PC_SCOPE( OnUpdateUI );

	wxToolBar* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);
	toolbar->EnableTool( XRCID("playGame"), ( GGame->GetActiveWorld() && !GGame->IsActive() ) ? true : false );	
	toolbar->EnableTool( XRCID("playGameFast"), ( GGame->GetActiveWorld() && !GGame->IsActive() ) ? true : false );	
	toolbar->EnableTool( XRCID("playGameFromSave"), true );	

	toolbar->EnableTool( XRCID("fileOpenWorld"), ( !GGame->IsActive() ) ? true : false );	
	toolbar->EnableTool( XRCID("fileRestoreWorld"), ( !GGame->IsActive() ) ? true : false );	
	toolbar->EnableTool( XRCID("fileNewWorld"), ( !GGame->IsActive() ) ? true : false );

	toolbar->ToggleTool( XRCID("pathlibEnableGeneration"), CPathLibWorld::IsTaskManagerEnabled() );
	toolbar->ToggleTool( XRCID("pathlibObstacles"), CPathLibWorld::IsObstacleGenerationEnabled() );
	toolbar->ToggleTool( XRCID("pathlibLocalFolder"), CPathLibWorld::IsLocalNavdataFolderForced() );
	

	for (THashMap< Int32, String >::const_iterator it = m_recent.Begin(); it != m_recent.End(); ++it)
	{
		wxMenuItem* recentworld = m_gameMenu->FindItem(it.Key());
		if (recentworld)
		{
			recentworld->Enable(!GGame->IsActive());
		}
	}

	wxMenuItem* newworld = m_gameMenu->FindItem(XRCID("fileNewWorld"));
	if (newworld)
	{
		newworld->Enable(!GGame->IsActive());
	}

	wxMenuItem* openworld = m_gameMenu->FindItem(XRCID("fileOpenWorld"));
	if (openworld)
	{
		openworld->Enable(!GGame->IsActive());
	}

	wxMenuItem* restoreworld = m_gameMenu->FindItem(XRCID("fileRestoreWorld"));
	if (restoreworld)
	{
		restoreworld->Enable(!GGame->IsActive());
	}

	wxMenuItem* closeworld = m_gameMenu->FindItem(XRCID("fileCloseWorld"));
	if (closeworld)
	{
		closeworld->Enable(!GGame->IsActive());
	}

	if ( wxMenuItem* enableSelectionTracking = m_gameMenu->FindItem( XRCID("undoTrackSelection") ) )
	{
		enableSelectionTracking->Check( m_viewport->GetSelectionTracking() );
	}

	m_gameMenu->FindItem( ID_VIEW_TOOLBAR_MAIN )->Check( m_auiMgr.GetPane( m_mainToolbar ).IsShown() );
	m_gameMenu->FindItem( ID_VIEW_TOOLBAR_WIDGETS )->Check( m_auiMgr.GetPane( m_widgets ).IsShown() );
	m_gameMenu->FindItem( ID_VIEW_TOOLBAR_WIDGETP4 )->Check( m_auiMgr.GetPane( m_widgetP4 ).IsShown() );

	Bool inFullscreenMode = ( m_fullScreenFrame != NULL );
	m_gameMenu->FindItem( ID_VIEW_VIEWPORT_FLOAT )->Check( m_isViewportFloating );
	m_gameMenu->FindItem( ID_VIEW_VIEWPORT_FLOAT )->Enable( !inFullscreenMode );

#ifndef NO_EXPLORATION_FINDER
	UpdateExplorationFinderIcons();
#endif
}

void CEdFrame::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( SavingConfiguration ) )
	{
		SetStatusBarText( TXT("Saving configuration..."), 1500 );
	}
	else if ( name == CNAME( GameEnding ) )
	{
		// Set editor camera position to the last gameplay camera position
		const Bool isLeaveGameCamPosEnabled = m_mainToolbar->GetToolState( XRCID("leaveGameCamPos") );
		if ( isLeaveGameCamPosEnabled )
		{
			if ( m_viewport->GetWorld() )
			{
				const Vector camPos = m_viewport->GetWorld()->GetCameraDirector()->GetCameraPosition();
				const EulerAngles camRot = m_viewport->GetWorld()->GetCameraDirector()->GetCameraRotation();
				GetWorldEditPanel()->SetCameraPosition( camPos );
				GetWorldEditPanel()->SetCameraRotation( camRot );
			}
		}
	}
	else
	// Game ended
	if ( name == CNAME( GameEnded ) )
	{
		// Unpause game when we are ending game during breakpoint
		if ( GGame->IsPaused() )
		{
			GGame->Unpause( TXT( "CEdFrame" ) );
		}

		// Restore view flags
		m_viewport->GetViewport()->ClearRenderingMask( SHOW_ALL_FLAGS );
		m_viewport->GetViewport()->SetRenderingMask( m_filter->GetViewportFlags( VFT_EDITOR ) );
		m_viewport->GetViewport()->RestoreSize();
		m_filter->UpdateSimulationOptions( false );

		// Reopen world environment editor if it is already open in order to reflect the new data
		if ( m_worldEnvironmentEditor && m_worldEnvironmentEditor->IsVisible() )
		{
			OpenWorldEnvironmentEditor();
		}

		RED_FATAL_ASSERT( GCommonGame, "CEdFrame::DispatchEditorEvent(): GCommonGame must not be nullptr." );

		// Reactivate DLCs in editor. Don't do this if game session is already started again as in that case
		// DLCs are reactivated by the game itself (this happens when user loads saved game in editor game).
		const Bool gameStartedAgain = GCommonGame->IsActive();
		if( !gameStartedAgain )
		{
			GCommonGame->GetDLCManager()->OnEditorStarted();
		}
	}
	else
	// Game started
	if ( name == CNAME( GameStarted ) )
	{
		// Unpause game when we are ending game during breakpoint
		if ( GGame->IsPaused() )
		{
			GGame->Unpause( TXT( "CEdFrame" ) );
		}

		// Reveal any hidden entities
		m_viewport->RevealEntities();

		// Restore view flags
		m_viewport->GetViewport()->ClearRenderingMask( SHOW_ALL_FLAGS );
		m_viewport->GetViewport()->SetRenderingMask( m_filter->GetViewportFlags( VFT_GAME) );
		m_viewport->GetViewport()->AdjustSizeWithCachets( GetWorldEditPanel()->GetViewportCachetAspectRatio() );
		m_filter->UpdateSimulationOptions( true );

	}
	// Select asset
	if ( name == CNAME( SelectAsset ) )
	{
		// Open asset browser
		wxCommandEvent fake;
		OnAssetBrowser( fake );

		// Open asset
		String assetFilePath = GetEventData< String >( data );
		m_assetBrowser->SelectFile( assetFilePath );
	}
	// Focus on asset
	if ( name == CNAME( FocusOnAsset ) )
	{	
		String meshName = GetEventData< String >( data );

		if ( !meshName.Empty() )
		{
			for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
			{
				CComponent *comp = *it;
				if ( comp ) 
				{
					if( !comp->GetFriendlyName().Empty() && comp->GetFriendlyName().ContainsSubstring(meshName) ) 
					{
						wxTheFrame->GetWorldEditPanel()->LookAtNode( comp, 2.0, Vector::ZEROS );						
						break;
					}
				}
			}	
		}

	}
	else
	// Select and edit asset
	if ( name == CNAME( SelectAndOpenAsset ) )
	{
		// Open asset browser
		wxCommandEvent fake;
		OnAssetBrowser( fake );

		// Open asset
		String assetFilePath = GetEventData< String >( data );
		m_assetBrowser->SelectAndOpenFile( assetFilePath );
	}
	else
	// Edit asset
	if ( name == CNAME( OpenAsset ) )
	{
		// Open asset
		String assetFilePath = GetEventData< String >( data );
		m_assetBrowser->OpenFile( assetFilePath );
	}
	else if( name == CNAME( CenterOnSelected ) )
	{
		CEdWorldEditPanel* worldEditPanel = GetWorldEditPanel();
		worldEditPanel->LookAtSelectedNodes();
	}
	else
	if ( name == CNAME( Delete ) )
	{
		OnDelete( wxCommandEvent() );
	}
	else
	if( name == CNAME( ActiveWorldChanged ) )
	{
        CWorld* world = GGame->GetActiveWorld();
        m_properties->SetWorld( world );
        m_undoManager->SetWorld( world );

		SetFrameTitle();
	}
	else
	if( name == CNAME( WorldUnloaded ) )
	{
		// Close the world environment editor
		if ( m_worldEnvironmentEditor )
		{
			m_worldEnvironmentEditor->Destroy();
			m_worldEnvironmentEditor = NULL;
		}

		m_mainToolbar->ToggleTool( XRCID("simulate"), false );
		SetFrameTitle();		
	}
	else
	if ( name == CNAME( WorldLoadProgress ) )
	{

	}
	else if ( name == CNAME( PlayGame ) )
	{
		OnPlayGame( wxCommandEvent() );
	}
	else if ( name == CNAME( FileReloadConfirm ) )
	{
		CResource* res = GetEventData< CResource* >( data );
		m_fileReloadDialog->AddResourceToReload( res );
	}
	else if ( name == CNAME( FileReloadToConfirm ) )
	{
		const CReloadFileInfo& reloadInfo = GetEventData< CReloadFileInfo >( data );
		m_fileReloadDialog->AddResourceToReloadFromEditor( reloadInfo );
	}
	else if ( name == CNAME( ShowReloadFilesDialog ) )
	{
		Bool delayFlag = GetEventData< Bool >( data );
		if ( delayFlag ) 
		{ 
			// Send again but with no delay this time
			delayFlag = false;
			SEvents::GetInstance().QueueEvent( CNAME( ShowReloadFilesDialog ), CreateEventData( delayFlag ) ); 
		}
		else
		{
			// Show reload dialog
			PauseConfigTimer();
			m_fileReloadDialog->DoModal();
			ResumeConfigTimer();
		}
	}
	else if ( name == CNAME( SelectionChanged ) )
	{
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			m_secondStatusBarInfo.m_selObjsNum = m_viewport->GetSelectionManager()->GetEntitiesSelectionCount();
			UpdateSecondStatusBarText();

			world->GetEditorFragmentsFilter().UnregisterAllEditorFragmentsOfCategory( SHOW_SelectionContacts );

			CRigidMeshComponent *rigidMesh;
			TDynArray< CNode* > selectedNodes;
			m_viewport->GetSelectionManager()->GetSelectedNodes( selectedNodes );
			for ( Uint32 i = 0; i < selectedNodes.Size(); ++i )
			{
				rigidMesh = Cast< CRigidMeshComponent > ( selectedNodes[ i ] );
				if ( rigidMesh )
				{
					world->GetEditorFragmentsFilter().RegisterEditorFragment( rigidMesh, SHOW_SelectionContacts );
				}
			}
		}
		else
		{
			m_secondStatusBarInfo.m_selObjsNum = 0;
		}
	}
	else if( name == CNAME( ProfilerChangeState ) )
	{
		Bool state = GetEventData< Bool >( data );
		if( state )
		{
			SetSecondStatusBarText( TXT("Profiler start...") );
		}
		else 
		{
			SetSecondStatusBarText( TXT("Profiler stop...") );
		}
	}
	else if ( name == CNAME( LayerInfoBuildTagChanged ) )
	{
		// If the layer build tag mesh coloring is active, refresh it
		if ( GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->IsChecked() )
		{
			CDrawableComponent::RenderingSelectionColorChangedInEditor();
		}
	}
	/*else if ( name == CNAME( SimulationComplete ) )
	{
		CWorld* world = GGame->GetActiveWorld();
		if ( world && world->GetPhysicsWorld() )
		{
			TDynArray< CObject* > affectedTemplates;
			TDynArray< CLayer* > affectedLayers;
			TDynArray< CEntity* > affectedEntities = world->GetPhysicsWorld()->GetEditorSimulatedEntities();  // copy tdynarray, because we have to clear it in EndSimulation()
			String msg = String::Printf( TXT("Affected entities: %ld\n"), affectedEntities.Size() );

			for ( Uint32 i = 0; i < affectedEntities.Size(); ++i )
			{
				if ( affectedEntities[ i ]->GetTemplate() != NULL && affectedEntities[ i ]->GetComponents().Size() > 1 )
				{
					affectedTemplates.PushBackUnique( affectedEntities[ i ]->GetTemplate() ); 
				}
				affectedLayers.PushBackUnique( affectedEntities[ i ]->GetLayer() );
			}

			msg += String::Printf( TXT("\nAffected layers: %ld\n"), affectedLayers.Size() );
			for ( Uint32 i = 0; i < affectedLayers.Size(); ++i )
			{
				msg += String::Printf( TXT("%s\n"), affectedLayers[ i ]->GetFriendlyName().AsChar() );
			}

			msg += String::Printf( TXT("\nEntity templates to detach: %ld\n"), affectedTemplates.Size() );
			for ( Uint32 i = 0; i < affectedTemplates.Size(); ++i )
			{
				msg += String::Printf( TXT("%s\n"), affectedTemplates[ i ]->GetFriendlyName().AsChar() );
			}

			msg += TXT("\nTHIS CAN NOT BE UNDONE! (at the moment...) Layers will be saved");

			if ( affectedTemplates.Size() )
			{
				msg += TXT(", templates will be detached");
			}

			msg += TXT(". Proceed with changes? (NO to reload affected layers, YES to save them)");

			world->GetPhysicsWorld()->EndSimulation();

			if ( wxMessageBox( msg.AsChar(), TXT("Simulation complete"), wxYES_NO ) == wxYES )
			{
				// 1. Checkout layers
				for ( Uint32 i = 0; i < affectedLayers.Size(); ++i )
				{
					CLayer* layer = affectedLayers[ i ];
					layer->MarkModified();
				}

				
				CEntity* thisEntity;
				CComponent* thisComponent;
				for ( Uint32 i = 0; i < affectedEntities.Size(); ++i )
				{
					// 2. Detach templates
					thisEntity = affectedEntities[ i ];
					if ( thisEntity->GetTemplate() != NULL && thisEntity->GetComponents().Size() > 1 )
					{
						thisEntity->DetachTemplate();
					}
					
					// 3. Save new state
					if ( thisEntity->GetComponents().Size() == 1 )
					{
						// single component entity - move whole entity
						thisComponent = thisEntity->GetComponents()[ 0 ];
						thisEntity->SetRawPlacement( &thisComponent->GetWorldPosition(), &thisComponent->GetWorldRotation(), &thisEntity->GetScale() );
						thisComponent->SetPosition( Vector::ZEROS );
						thisComponent->SetRotation( EulerAngles::ZEROS );
						thisEntity->ForceUpdateTransform();
					}
					else
					{
						// multi-component entity
						for ( Uint32 k = 0; k < thisEntity->GetComponents().Size(); ++k )
						{
							thisComponent = thisEntity->GetComponents()[ k ];
							if ( !Vector::Near3( thisComponent->GetPosition(), thisComponent->GetWorldPosition() - thisEntity->GetWorldPosition() ) ||
								 !thisComponent->GetRotation().AlmostEquals( thisComponent->GetWorldRotation() - thisEntity->GetWorldRotation() ) )
							{
								thisComponent->SetPosition( thisComponent->GetWorldPosition() - thisEntity->GetWorldPosition() );
								thisComponent->SetRotation( thisComponent->GetWorldRotation() - thisEntity->GetWorldRotation() );
							}
						}
					}
				}

				// 4. Save layers
				for ( Uint32 i = 0; i < affectedLayers.Size(); ++i )
				{
					CLayer* layer = affectedLayers[ i ];
					layer->Save();
				}
			}
			else
			{
				// This solution is temporary: target solution is that we undo simulated entities here.
				// Hack is to be removed when undoing simulation is done.

				// --- BEGIN MEGA HACK ---
				for ( Uint32 i = 0; i < affectedLayers.Size(); ++i )
				{
					CLayer* layer = affectedLayers[ i ];
					CLayerInfo* info = layer->GetLayerInfo();
					if ( layer->GetFile() )
					{
						layer->GetFile()->Unmodify();
					}

					info->SyncUnload();
					world->DelayedActions();

					if ( layer->GetFile() )
					{
						layer->GetFile()->Unload();
					}
					else
					{
						layer->Discard();
					}

					info->SetLayerToDestroy( NULL );

					LayerLoadingContext context;
					info->SyncLoad( context );

					world->DelayedActions();
				}
				// --- END MEGA HACK ---
			}

		}

		m_mainToolbar->ToggleTool( XRCID("simulate"), false );
	}*/
	else if ( name == CNAME( SetDLCToStart ) )
	{
		SDLCStartData dlc = GetEventData< SDLCStartData > ( data );

		IGameLoader *dlcLoader = GCommonGame->CreateDLCLoader( dlc.m_name );
		if ( nullptr == dlcLoader )
		{
			return;
		}

		String currentWorld;
		{
			CGameSaverBlock block( dlcLoader, CNAME(worldInfo) );
			dlcLoader->ReadValue( CNAME( world ), currentWorld );
		}

		if ( GCommonGame->IsActive() )
		{
			GCommonGame->EndGame();
			GCommonGame->GetDLCManager()->OnEditorStopped();
		}

		if ( false == LoadWorld( currentWorld ) )
		{
			wxMessageBox( String::Printf( TXT("Failed to start DLC from saved game. Unknown world: '%s'"), currentWorld.AsChar() ).AsChar(), wxT("Start Game"), wxOK | wxICON_ERROR );
		}
		else
		{
			GCommonGame->SetDLCToStart( dlc.m_name, dlc.m_difficulty ); 
		}

		delete dlcLoader; // the rest is up to game-side
	}
	else if ( name == CNAME( LoadGame ) )
	{
		SSavegameInfo info = GetEventData< SSavegameInfo > ( data );
		LoadGame( info );
	}
}

Bool CEdFrame::CheckStartGameConditions() const
{
	if ( m_assetBrowser->IsEntityTemplateEditorOpen() )
	{
		wxMessageBox( wxT("Please close all instances of Entity Template Editor before starting the game."), wxT("Red Editor"), wxOK | wxICON_ERROR );
		return false;
	}

	return true;
}

Bool CEdFrame::LoadGame( const SSavegameInfo &info )
{
	SGameSessionManager::GetInstance().OnEditorLoadGame();

	CUserProfileManagerWindows* mgr = ( CUserProfileManagerWindows* ) GUserProfileManager;
	mgr->InitGameLoading( info );

	IGameLoader* gameLoader = nullptr;
	Bool succ = info.IsValid();
	if ( succ )
	{
		ELoaderCreationResult res;
		gameLoader = SGameSaveManager::GetInstance().CreateLoader( info, res );
		succ = gameLoader != nullptr;
	}

	if ( !succ )
	{
		RED_LOG( Session, TXT("Unable to restore session data. Not loading.") );
		return false;
	}

	// World info
	String currentWorld;
	{
		CGameSaverBlock block( gameLoader, CNAME(worldInfo) );
		gameLoader->ReadValue( CNAME( world ), currentWorld );
	}

	// We have to close the game before unloading the world
	if ( GGame->IsActive() )
	{
		GGame->EndGame();
	}

	CQuestsSystem* questsSystem = GCommonGame->GetSystem< CQuestsSystem >();
	if ( questsSystem && questsSystem->IsPaused() )
	{
		questsSystem->Pause( false );
	}

	if ( !LoadWorld( currentWorld ) )
	{
		wxMessageBox( String::Printf( TXT("Failed to restore game from saved game. Unknown world: '%s'"), currentWorld.AsChar() ).AsChar(), wxT("Start Game"), wxOK | wxICON_ERROR );
		return false;
	}

	// Deactivate DLCs activated by editor - they will be reactivated by game during game starting process.
	GCommonGame->GetDLCManager()->OnEditorStopped();

	if ( SGameSessionManager::GetInstance().RestoreSession( gameLoader ) != RESTORE_Success )
	{
		// Reactivate DLCs in editor.
		GCommonGame->GetDLCManager()->OnEditorStarted();

		wxMessageBox( wxT("Failed to restore game from saved game"), wxT("Start Game"), wxOK | wxICON_ERROR );
		return false;
	}

	GUserProfileManager->FinalizeGameLoading();
	return true;
}

Bool CEdFrame::StartGame( Bool fast, const String& gameSaveFilePath /*= String::EMPTY*/, const String& loadingVideoToPlay /*= String::EMPTY*/, const TDynArray< CName >& playGoChunksToActivate /*= TDynArray<CName>()*/ )
{
	// Dump configuration
	CUserConfigurationManager &configUser = SUserConfigurationManager::GetInstance();
	configUser.SaveAll();
	CCommonConfigurationManager &configCommon = SCommonConfigurationManager::GetInstance();
	configCommon.SaveAll();

	// No world
	CWorld* world =	GGame->GetActiveWorld();
	if ( !world && gameSaveFilePath.Empty() )
	{
		wxMessageBox( wxT("No world to start the game on!"), wxT("Start game"), wxOK | wxICON_ERROR );
		return false;
	}

	// Cancel any edit in progress
	m_tools->CancelTool();

	// If there is a floating frame, make sure it gains focus
	if( m_isViewportFloating )
	{
		m_auiMgr.GetPane( m_viewport ).frame->SetFocus();
	}

	// Start game
	if ( !gameSaveFilePath.GetLength() )
	{
		// Start editor game
		Vector cameraPosition = m_viewport->GetCameraPosition();
		EulerAngles cameraRotation = m_viewport->GetCameraRotation();
		const Bool hideCursor = !HasCommandLineParameter( TXT("showcursor") );
        m_sceneExplorer->MarkTreeItemsForUpdate();

		// Adjust cachets now if in the editor, otherwise we'll leave loading screen junk in the cachets because nothing clears them afterwards
		IViewport* vp = GGame ? GGame->GetViewport() : nullptr;
		if ( vp && !vp->IsCachet() )
		{
			vp->AdjustSizeWithCachets( GetWorldEditPanel()->GetViewportCachetAspectRatio() );
		}

		GContentManager->ResetActivatedContent();
		for ( CName chunk : playGoChunksToActivate )
		{
			if ( !GContentManager->ActivateContent( chunk ) )
			{
				ERR_GAME( TXT("Failed to activate content %ls from gameResource"), chunk.AsChar() );
				GContentManager->ResetActivatedContent();
				return false;
			}
		}

		if ( playGoChunksToActivate.Empty() )
		{
			GContentManager->ActivateAllContentForDebugQuests();
		}

		// Deactivate DLCs activated by editor - they will be reactivated by game during game starting process.
		GCommonGame->GetDLCManager()->OnEditorStopped();

		if ( !SGameSessionManager::GetInstance().CreateEditorSession( fast, cameraPosition, cameraRotation, hideCursor, loadingVideoToPlay ) )
		{
			if ( vp && vp->IsCachet() )
			{
				vp->RestoreSize();
			}

			// Reactivate DLCs in editor.
			GCommonGame->GetDLCManager()->OnEditorStarted();

			wxMessageBox( wxT("Failed to create editor game session"), wxT("Start Game"), wxOK | wxICON_ERROR );
			return false;
		}
	}
	else
	{
		// Restore game from save

		// Load session data, for now directly from file
		CFilePath filePath( gameSaveFilePath );
		CUserProfileManagerWindows* mgr = ( CUserProfileManagerWindows* ) GUserProfileManager;
		SSavegameInfo info = mgr->GetSavegameInfo( filePath.GetFileName() );
		if ( false == LoadGame( info ) )
		{
			return false;
		}
	}

	// If the world environment editor is open, recreate it to use the new world data
	if ( m_worldEnvironmentEditor && m_worldEnvironmentEditor->IsVisible() )
	{
		OpenWorldEnvironmentEditor();
	}
	
	// Update the UI
	m_auiMgr.Update();
	return true;
}

void CEdFrame::OnPlayGame( wxCommandEvent& WXUNUSED(event) )
{
	if( const Bool okToStartGame = CheckStartGameConditions() )
	{
		StartGame( false );
	}
}

void CEdFrame::OnPlayGameFast( wxCommandEvent& WXUNUSED(event) )
{
	if( const Bool okToStartGame = CheckStartGameConditions() )
	{
		StartGame( true);
	}
}

void CEdFrame::OnRegenerateRenderProxies( wxCommandEvent& WXUNUSED(event) )
{
	// Assume some rendering resources were changed
	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}

void CEdFrame::OnEnvironmentModifierChanged( wxCommandEvent& event )
{	
	// Get platform
	wxChoice* choice = XRCCTRL( *this, "displayModifier", wxChoice );

	// Get modifier value
	const Int32 modifier = choice->GetSelection();	
	const EEnvManagerModifier newModifier = (modifier < (Uint32)EMM_MAX) ? (EEnvManagerModifier)modifier : EMM_None;
	if ( newModifier != EMM_None )
	{
		m_lastDisplayMode = (Int32)newModifier;
	}

	// Apply new modifier
	CEnvironmentManager::SetModifier( newModifier );
}

void CEdFrame::OnPlayGameFromSave( wxCommandEvent& event )
{
	const Bool okToStartGame = CheckStartGameConditions();
	if( !okToStartGame )
	{
		return;
	}

	String savesDirPath = GFileManager->GetUserDirectory();
	savesDirPath += TXT( "gamesaves\\" );

	// load the xml
	wxFileDialog loadFileDialog( this, wxT("Load a game"), savesDirPath.AsChar(), wxT( "" ), wxT( "*.sav" ), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() != wxID_OK )
	{
		return;
	}

	// Start game from save
	String absLoadPath = loadFileDialog.GetPath().wc_str();
	StartGame( false, absLoadPath );
}

void CEdFrame::OnUndo( wxCommandEvent& event )
{
    m_undoManager->Undo();
}

void CEdFrame::OnRedo( wxCommandEvent& event )
{
    m_undoManager->Redo();
}

void CEdFrame::OnUndoHistory( wxCommandEvent& event )
{
	ShowUndoHistoryFrame();
}

void CEdFrame::OnUndoTrackSelection( wxCommandEvent& event )
{
	Bool track = m_gameMenu->FindItem( XRCID("undoTrackSelection") )->IsChecked();
	m_viewport->SetSelectionTracking( track );
}

void CEdFrame::OnCopy( wxCommandEvent& event )
{
	m_viewport->OnCopy( event );
}

void CEdFrame::OnCut( wxCommandEvent& event )
{
	m_viewport->OnCut( event );
}

void CEdFrame::OnPaste( wxCommandEvent& event )
{
	m_viewport->OnPaste( event );
}

void CEdFrame::OnDelete( wxCommandEvent& event )
{
	m_viewport->OnDelete( event );
}

void CEdFrame::OnCopyCameraView( wxCommandEvent& event )
{
	m_viewport->OnCopyCameraView( event );
}

void CEdFrame::OnPasteCameraView( wxCommandEvent& event )
{
	m_viewport->OnPasteCameraView( event );
}

void CEdFrame::OnPasteQuestView( wxCommandEvent& event )
{
	CEdQuestEditor::GetQuestEditorClipboard().Paste( nullptr );
}

void CEdFrame::OnSpaceForeign( wxCommandEvent& event )
{
	if ( wxGetKeyState( WXK_SHIFT ) )
	{
		CEntity* entity = GetSelectedEntity();
		if ( entity )
		{
			m_viewport->GetTransformManager()->SetForeignRotation( entity->GetWorldRotation() );
		}
		else
		{
			if ( !GGame->GetActiveWorld() )
			{
				wxMessageBox( wxT("You need to load a world before setting the foreign space source entity"), wxT("No world"), wxICON_ERROR|wxOK, this );
			}
			else
			{
				wxMessageBox( wxT("You need to select a single entity to be used as the source for the foreign space"), wxT("Invalid selection"), wxICON_ERROR|wxOK, this );
			}
		}
	}
	else
	{
		m_viewport->m_widgetManager->SetWidgetSpace( RPWS_Foreign );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSpaceLocal( wxCommandEvent& event )
{
	m_viewport->m_widgetManager->SetWidgetSpace( RPWS_Local );
	UpdateWidgetUI();
}

void CEdFrame::OnSpaceWorld( wxCommandEvent& event )
{
	m_viewport->m_widgetManager->SetWidgetSpace( RPWS_Global );
	UpdateWidgetUI();
}

void CEdFrame::OnSpaceChange( wxCommandEvent& event )
{
	if ( m_viewport->m_widgetManager->GetWidgetSpace() == RPWS_Global )
	{
		m_viewport->m_widgetManager->SetWidgetSpace( RPWS_Local );
	}
	else
	{
		m_viewport->m_widgetManager->SetWidgetSpace( RPWS_Global );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnSelectAll( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->SelectAll();
	}
}

void CEdFrame::OnUnselectAll( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->DeselectAll();
	}
}

void CEdFrame::OnInvertSelection( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->InvertSelection();
	}
}

void CEdFrame::OnSelectByTheSameEntityTemplate( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->SelectAllWithTheSameEntityTemplate();
	}
}

void CEdFrame::OnSelectByTheSameTag( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CEdTagMiniEditor* tagEditor = new CEdTagMiniEditor( this, TDynArray<CName>() );
		if ( tagEditor->ShowModal() != wxOK )
		{
			tagEditor->Destroy();
			return;
		}

		TagList tagList;
		tagList.AddTags( tagEditor->GetTags() );
		tagEditor->Destroy();

		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->SelectByTags( tagList );
	}
}

void CEdFrame::OnSelectionOnActiveLayer( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->SetSelectMode( SM_ActiveLayer );
	}

	UpdateWidgetUI();
}

void CEdFrame::OnSelectionOnMultiLayer( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();
		selectionManager->SetSelectMode( SM_MultiLayer );
	}

	UpdateWidgetUI();
}

void CEdFrame::OnCenterOnSelected( wxCommandEvent& event )
{
	CEdWorldEditPanel* worldEditPanel = GetWorldEditPanel();
	worldEditPanel->LookAtSelectedNodes();
}

void CEdFrame::OnHideLayersWithSelectedObjects( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();

		// Get all selected entities
		TDynArray <CEntity* > selectedEntities;
		selectionManager->GetSelectedEntities( selectedEntities );

		// gather all layers with selected entities
		TDynArray< CLayer* > layers;
		for( auto it=selectedEntities.Begin(); it!=selectedEntities.End(); ++it )
		{
			CEntity* entity = ( *it );
			layers.PushBackUnique( entity->GetLayer() );
		}

		// hide layers
		for( auto it=layers.Begin(); it!=layers.End(); ++it )
		{
			CLayer* layer = ( *it );
			layer->GetLayerInfo()->Show( false );
		}
	}
}

void CEdFrame::OnUnhideAllLoadedLayers( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world != nullptr )
	{
		CSelectionManager* selectionManager = m_viewport->GetSelectionManager();

		TDynArray< CLayerInfo* > layerInfos;
		CLayerGroup* layerGroup = world->GetWorldLayers();
		layerGroup->GetLayers( layerInfos, true, true );

		// unhide all founded layers
		for( auto it=layerInfos.Begin(); it!=layerInfos.End(); ++it )
		{
			CLayerInfo* layerInfo = ( *it );
			layerInfo->Show( true );
		}
	}
}

void CEdFrame::OnDisconnectPerforce( wxCommandEvent& event )
{

	if ( m_widgetP4->GetToolState( XRCID("disconnectPerforce") ) )
	{	
		GVersionControl = new ISourceControl;	
		m_widgetP4->SetBackgroundColour( wxColour(240,30,30) );
	}
	else
	{
		if ( GFeedback->AskYesNo(TXT("If you want to connect to Perforce again, you will need to restart the editor.\n\nDo you want to restart the editor now?") ) )
		{
			RelaunchEditor();
		}
		m_widgetP4->ToggleTool( XRCID("disconnectPerforce"), true );
	}
}

void CEdFrame::OnMergeLootWithEntity( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( !world )
	{
		return;
	}

	TDynArray< CNode* > nodes;
	m_viewport->GetSelectionManager()->GetSelectedNodes( nodes );
	if( nodes.Size() != 2 )
	{
		GFeedback->ShowError( TXT( "This tool works only for static meshes" ) );
		return;
	}

	if( !( nodes[ 0 ]->IsA< CMeshComponent >() && nodes[ 1 ]->IsA< CEntity >() ) &&
		!( nodes[ 1 ]->IsA< CMeshComponent >() && nodes[ 0 ]->IsA< CEntity >() ) )
	{
		GFeedback->ShowError( TXT( "This tool works only for static meshes" ) );
		return;
	}

	CMeshComponent* originalMesh = nodes[ 0 ]->IsA< CMeshComponent >() ? Cast< CMeshComponent >( nodes[ 0 ] ) : Cast< CStaticMeshComponent >( nodes[ 1 ] );
	if( !originalMesh )
	{
		GFeedback->ShowError( TXT( "This tool works only for static meshes, error #2" ) );
		return;
	}

	CClass* meshClass = ( originalMesh->IsA< CStaticMeshComponent >() ? ClassID< CStaticMeshComponent >() : ClassID< CMeshComponent >() );

	String newEntityPath;
	String newEntityName;
	String newEntityDir;
	String originalEntityPath;
	String originalMeshPath;

	{ // @todo extract this
		originalMeshPath = originalMesh->GetMeshResourcePath();
		if ( !GetActiveResource( originalEntityPath, ClassID< CEntityTemplate >() ) )
		{
			GFeedback->ShowError( TXT("Please select valid entity template in asset browser first.") );
			return;
		}

		String fpPrefix;
		{
			TDynArray< String > oepSplit = originalMeshPath.Split( TXT( "\\" ) );
			if( oepSplit.Size() < 0 )
			{
				GFeedback->ShowError( TXT("Error while processing original mesh path.") );
				return;
			}

			fpPrefix = oepSplit[ oepSplit.Size() - 1 ];

			if( !fpPrefix.ContainsSubstring( TXT( ".w2mesh" ) ) )
			{
				GFeedback->ShowError( TXT("Error while processing original mesh path.") );
				return;
			}

			fpPrefix = fpPrefix.StringBefore( TXT( ".w2mesh" ) );
		}

		String fpSuffix;
		{
			TDynArray< String > oepSplit = originalEntityPath.Split( TXT( "\\" ) );
			if( oepSplit.Size() < 0 )
			{
				GFeedback->ShowError( TXT("Error while processing original entity path.") );
				return;
			}

			fpSuffix = oepSplit[ oepSplit.Size() - 1 ];

			if( !fpSuffix.ContainsSubstring( TXT( ".w2ent" ) ) )
			{
				GFeedback->ShowError( TXT("Error while processing original mesh path.") );
				return;
			}
		}

		if( ( fpPrefix.Size() == 0 ) || ( fpSuffix.Size() == 0 ) )
		{
			GFeedback->ShowError( TXT("Error while constructing new file name.") );
			return;
		}

		newEntityName = fpPrefix + fpSuffix;
		newEntityDir = TXT( "gameplay\\containers\\_container_definitions\\autogen\\" );
		newEntityPath =  newEntityDir + newEntityName;
	}

	Bool constructEntity = false;
	{ // if the entity doesn't exist, copy the original one
		CResource *resource = GDepot->LoadResource( newEntityPath );
		if( !resource )
		{
			CDirectory* directory = GDepot->FindPath( newEntityDir.AsChar() );
			if( !directory )
			{
				GFeedback->ShowError( TXT( "Depot directory could not be created." ) );
				return;
			}

			TDynArray< String > rNames;
			rNames.PushBack( newEntityName );

			TDynArray< CResource* > resources;
			resources.PushBack( GDepot->LoadResource( originalEntityPath ) );
			SVersionControlWrapper::GetInstance().CopyAsFiles( directory, resources, rNames );

			constructEntity = true;
		}
	}


	CEntity* tmpEnt = world->GetDynamicLayer()->CreateEntitySync( EntitySpawnInfo() );
	if( !tmpEnt )
	{
		GFeedback->ShowError( TXT( "Could not create a temp entity to keep the component." ) );
		return;
	}

	CMeshComponent* meshComponentCopy  = Cast< CMeshComponent >( originalMesh->Clone( tmpEnt ) );

	CEntity* createdEntity = nullptr;
	{ // switch entities on layer
		RED_ASSERT( GetWorldEditPanel() );
		TDynArray< CEntity* > createdEntities;
		TDynArray< CEntity* > selectedEntities;
		m_viewport->GetSelectionManager()->GetSelectedEntities( selectedEntities );
		GetWorldEditPanel()->ReplaceEntitiesWithEntity( newEntityPath, selectedEntities, false, createdEntities );
		RED_ASSERT( createdEntities.Size() == 1 );
		createdEntity = createdEntities[ 0 ];
		RED_ASSERT( createdEntity );
		m_viewport->GetSelectionManager()->Select( createdEntity );
		createdEntity->ForceFinishAsyncResourceLoads();
	}


	if( createdEntity && constructEntity )
	{ // the entity requires filling with mesh
		CEntityTemplate* entityTemplate = createdEntity->GetEntityTemplate();
		RED_ASSERT( entityTemplate );

		if ( !entityTemplate->MarkModified() )
		{
			GFeedback->ShowError( TXT( "Entity template could not be marked as modified." ) );
			world->GetDynamicLayer()->DestroyEntity( tmpEnt );
			return;
		}

		CEntity* entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		entity->ForceFinishAsyncResourceLoads();

		CMeshComponent* newMeshComponent = Cast< CMeshComponent >( meshComponentCopy->Clone( entity ) ); 
		if( !newMeshComponent )
		{
			GFeedback->ShowError( TXT( "Putting mesh into the new created entity failed." ) );
			world->GetDynamicLayer()->DestroyEntity( tmpEnt );
			return;
		}

		entity->AddComponent( newMeshComponent );

		entity->UpdateStreamedComponentDataBuffers();
		entity->PrepareEntityForTemplateSaving();
		entity->DetachTemplate();
		entityTemplate->CaptureData( entity );
		entity->Discard();
		entityTemplate->Save();
	}

	world->GetDynamicLayer()->DestroyEntity( tmpEnt );
	if( createdEntity->GetLayer() )
	{
		createdEntity->GetLayer()->MarkModified();
	}

	createdEntity->CreateStreamedComponents( SWN_NotifyWorld );
	createdEntity->RefreshChildrenVisibility();
}

void CEdFrame::OnReplaceWithLootable( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( !world )
	{
		return;
	}

	TDynArray< CEntity* > entities;
	m_viewport->GetSelectionManager()->GetSelectedEntities( entities );
	if( entities.Size() != 1 )
	{
		GFeedback->ShowError( TXT( "One entity must be selected to perform this action" ) );
		return;
	}


	TDynArray< CResource* > resources;
	{ // pick resource
		CEntity* ent = entities[ 0 ];
		CEntityGroup* entityGroup = NULL;
		RED_ASSERT( ent );

		resources.PushBack( ent->GetEntityTemplate() );
		if( !resources[ 0 ] )
		{
			GFeedback->ShowError( TXT( "Entity with template must be selected." ) );
			return;
		}
	}

	

	// @todo MS this should not be hardcoded
	wxString defPath;
	defPath.Printf( wxT( "..\\%s\\gameplay\\containers\\autogen" ), GGameConfig::GetInstance().GetDataPathSuffix().AsChar() );
	wxFileDialog fDialog( this, wxT( "Save entity template" ), defPath, wxT( "" ), wxT( "*.w2ent" ), wxFD_SAVE );
	if ( fDialog.ShowModal() != wxID_OK )
	{
		return;
	}

	String fPath = fDialog.GetPath().wc_str();
	String fName;
	if( fPath.Empty() )
	{
		RED_ASSERT( false );
		return;
	}

	{ // extract this
		TDynArray< String > fpSplit = fPath.Split( TXT( "\\" ) );
		fPath = TXT( "" );
		Bool collectPath = false;
		for( Uint32 i = 0; i < fpSplit.Size(); ++i )
		{
			if( collectPath )
			{
				if( !fPath.Empty() )
				{
					fPath += TXT( "\\" );
				}

				fPath += fpSplit[ i ];
			}
			else
			{
				if( fpSplit[ i ] == GGameConfig::GetInstance().GetDataPathSuffix() )
				{
					collectPath = true;
				}
			}
		}

		if( !collectPath || fPath.Empty() )
		{
			wxMessageBox( wxT( "Pick a depot path for the new entity." ), wxT( "Error" ), wxICON_ERROR|wxOK|wxCENTRE, this );
			return;
		}

		RED_ASSERT( fpSplit.Size() > 0 );
		fName = fpSplit[ fpSplit.Size() - 1 ];
	}

	CDirectory* directory = GDepot->FindPath( fPath.AsChar() );
	if( !directory )
	{
		wxMessageBox( wxT( "Pick a depot path for the new entity." ), wxT( "Error" ), wxICON_ERROR|wxOK|wxCENTRE, this );
		return;
	}

	TDynArray< String > rNames;
	RED_ASSERT( fName.GetLength() > 0 );
	rNames.PushBack( fName );

	SVersionControlWrapper::GetInstance().CopyAsFiles( directory, resources, rNames );

	RED_ASSERT( GetWorldEditPanel() );
	TDynArray< CEntity* > createdEntities;
	GetWorldEditPanel()->ReplaceEntitiesWithEntity( fPath, entities, false, createdEntities );

	{ // @todo MS: extract this
		RED_ASSERT( createdEntities.Size() == 1 );
		CEntity* ent = createdEntities[ 0 ];
		m_viewport->GetSelectionManager()->Select( ent );

		ent->CreateStreamedComponents( SWN_DoNotNotifyWorld );
		CEntityTemplate* entTemplate = ent->GetEntityTemplate();

		RED_ASSERT( entTemplate );
		CClass* currentClass = SRTTI::GetInstance().FindClass( CName( TXT( "W3AnimatedContainer" ) ) );
		if( currentClass )
		{
			entTemplate->SetEntityClass( currentClass );
		}

		do
		{ // add include
			String entityTemplatePath = TXT( "gameplay\\containers\\container_template.w2ent" );

			CEntityTemplate* loadedEntityTemplate = LoadResource< CEntityTemplate >( entityTemplatePath );
			if ( !loadedEntityTemplate || ( entTemplate == loadedEntityTemplate ) )
			{
				break;
			}

			// Already included
			if ( EntityEditorUtils::IsTemplateIncludedAtLevel( entTemplate, loadedEntityTemplate ) )
			{
				break;
			}

			// Check if template is already included
			if ( EntityEditorUtils::IsTemplateIncluded( entTemplate, loadedEntityTemplate ) )
			{
				break;
			}

			entTemplate->GetIncludes().PushBackUnique( loadedEntityTemplate );
		} while( false );

		ent->PrepareEntityForTemplateSaving();
		entTemplate->Save();
	}
}

void CEdFrame::OnAddLootOptions( wxCommandEvent& event )
{
	CWorld* world = GGame->GetActiveWorld();
	if( !world )
	{
		return;
	}

	TDynArray< CEntity* > entities;
	m_viewport->GetSelectionManager()->GetSelectedEntities( entities );
	if( entities.Size() != 1 )
	{
		GFeedback->ShowError( TXT( "One entity must be selected to perform this action" ) );
		return;
	}

	CEntityTemplate* entTemplate = entities[ 0 ]->GetEntityTemplate();
	if( !entTemplate )
	{
		wxMessageBox( wxT( "Cannot obtain entity template from selected object." ), wxT( "Error" ), wxICON_ERROR|wxOK|wxCENTRE, this );
		return;
	}

	CEdEntityLootSetupDialog dialog( this, entTemplate );
	dialog.ShowModal();
}

Bool CEdFrame::DisplayErrorsList( const String& header, const String& footer, const TDynArray< String >& errors, const TDynArray< String >& descriptions, Bool cancelBtn ) const
{
	if ( !errors.Empty() )
	{
		// display list of errors
		CEdErrorsListDlg dlg( wxTheFrame, true, cancelBtn );
		dlg.SetHeader( header.AsChar() );
		dlg.SetFooter( footer.AsChar() );
		return dlg.Execute( errors, descriptions );
	}
	return true;
}

//
//////////////////////////////////////////////////////////////////////////

void CEdFrame::AddToIsolated( CEntity* entitity )
{
	// dex_fix!!!
	/*if( m_renderFilter && entitity )
	{
		entitity->GetComponentsOfClass( m_renderFilter->m_allowed );
		entitity->GetComponentsOfClass( m_renderFilter->m_editorAllowed );
	}*/
}

void CEdFrame::AddToIsolated( TDynArray< CEntity* > &entities )
{
	// dex_fix!!!
	/*if( m_renderFilter )
	{
		for( TDynArray< CEntity* >::iterator it=entities.Begin(); it!=entities.End(); it++ )
		{
			( *it )->GetComponentsOfClass( m_renderFilter->m_allowed );
			( *it )->GetComponentsOfClass( m_renderFilter->m_editorAllowed );
		}
	}*/
}

void CEdFrame::OnWidgetPick( wxCommandEvent& event )
{
	m_viewport->m_widgetManager->SetWidgetMode( RPWM_None );
	UpdateWidgetUI();
}

void CEdFrame::OnWidgetMove( wxCommandEvent& event )
{
	if ( wxGetKeyState( WXK_CONTROL ) )
	{
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			wxMessageBox( wxT("No world loaded"), wxT("Error"), wxICON_ERROR|wxOK );
			return;
		}

		String deltaStr( TXT("0 0 0") );
		if ( InputBox( this, TXT("Move by delta"), TXT("Enter X, Y, Z delta"), deltaStr ) )
		{
			Vector delta;
			if ( FromString( deltaStr + TXT(" 1"), delta ) )
			{
				TDynArray<CNode*> nodes;
				m_viewport->GetSelectionManager()->GetSelectedNodes( nodes );

				for ( Uint32 i = 0; i < nodes.Size(); ++i )
				{
					CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( nodes[i] ), RED_NAME( transform ) );
					SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPreChange ), CreateEventData( eventData ) );
				}

				m_viewport->GetTransformManager()->MoveSelection( delta );

				for ( Uint32 i = 0; i < nodes.Size(); ++i )
				{
					CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( nodes[i] ), RED_NAME( transform ) );
					SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPostChange ), CreateEventData( eventData ) );
				}
			}
		}
	}
	else if ( wxGetKeyState( WXK_SHIFT ) )
	{
		ToggleWidgetCyclability( RPWM_Move );
	}
	else
	{
		m_viewport->m_widgetManager->SetWidgetMode( RPWM_Move );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnWidgetRotate( wxCommandEvent& event )
{
	if ( wxGetKeyState( WXK_CONTROL ) )
	{
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			wxMessageBox( wxT("No world loaded"), wxT("Error"), wxICON_ERROR|wxOK );
			return;
		}

		Bool individually = wxGetKeyState( WXK_ALT );
		String deltaStr( TXT("0 0 0") );
		Bool r;
		if ( individually )
		{
			r = InputBox( this, TXT("Rotate individually by specific angle"), TXT("Enter pitch, yaw and roll angles in degrees for individual rotation"), deltaStr );
		}
		else
		{
			r = InputBox( this, TXT("Rotate by specific angle"), TXT("Enter pitch, yaw and roll angles in degrees"), deltaStr );
		}
		if ( r )
		{
			EulerAngles angles;
			if ( FromString( deltaStr, angles ) )
			{
				TDynArray<CNode*> nodes;
				m_viewport->GetSelectionManager()->GetSelectedNodes( nodes );

				for ( Uint32 i = 0; i < nodes.Size(); ++i )
				{
					CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( nodes[i] ), RED_NAME( transform ) );
					SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPreChange ), CreateEventData( eventData ) );
				}

				m_viewport->GetTransformManager()->RotateSelection( angles, individually );

				for ( Uint32 i = 0; i < nodes.Size(); ++i )
				{
					CEdPropertiesPage::SPropertyEventData eventData( NULL, STypedObject( nodes[i] ), RED_NAME( transform ) );
					SEvents::GetInstance().DispatchEvent( CNAME( EditorPropertyPostChange ), CreateEventData( eventData ) );
				}
			}
		}
	}
	else if ( wxGetKeyState( WXK_SHIFT ) )
	{
		ToggleWidgetCyclability( RPWM_Rotate );
	}
	else
	{
		m_viewport->m_widgetManager->SetWidgetMode( RPWM_Rotate );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnWidgetScale( wxCommandEvent& event )
{
	if ( wxGetKeyState( WXK_SHIFT ) )
	{
		ToggleWidgetCyclability( RPWM_Scale );
	}
	else
	{
		m_viewport->m_widgetManager->SetWidgetMode( RPWM_Scale );
	}
	UpdateWidgetUI();
}

void CEdFrame::OnWidgetChange( wxCommandEvent& event )
{
	ERPWidgetMode currentMode = m_viewport->m_widgetManager->GetWidgetMode();
	ERPWidgetMode nextMode = currentMode;
	while ( true )
	{
		nextMode = (ERPWidgetMode)(nextMode + 1);
		if ( (int)nextMode > (int)RPWM_Scale ) nextMode = RPWM_Move;
		if ( nextMode == currentMode ) break;
		if ( m_cyclableWidgets[ nextMode - 1 ] ) break;
	}
	if ( nextMode != currentMode )
	{
		m_viewport->m_widgetManager->SetWidgetMode( nextMode );
		UpdateWidgetUI();
	}
}

void CEdFrame::UpdateWidgetUI()
{
	m_viewport->UpdateViewportWidgets();

	// Modes
	ERPWidgetMode mode = m_viewport->m_widgetManager->GetWidgetMode();
	CheckWidgetUI( XRCID("widgetModeMove"), mode == RPWM_Move );
	CheckWidgetUI( XRCID("widgetModeRotate"), mode == RPWM_Rotate );
	CheckWidgetUI( XRCID("widgetModeScale"), mode == RPWM_Scale );	

	// Spaces
	ERPWidgetSpace space = m_viewport->m_widgetManager->GetWidgetSpace();
	CheckWidgetUI( XRCID("widgetSpaceWorld"), space == RPWS_Global );
	CheckWidgetUI( XRCID("widgetSpaceLocal"), space == RPWS_Local );
	CheckWidgetUI( XRCID("widgetSpaceForeign"), space == RPWS_Foreign );

	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		ESelectMode selectMode = m_viewport->GetSelectionManager()->GetSelectMode();

		// Selection modes
		CheckWidgetUI( XRCID("editSelectionOnActiveLayer"), selectMode == SM_ActiveLayer );
		CheckWidgetUI( XRCID("editSelectionOnMultiLayer"), selectMode == SM_MultiLayer );

		// dex_fix!!!
		//CheckWidgetUI( XRCID("selectIsolate"), false );//m_renderFilter != NULL );
		
		wxComboBox* snapCombo = XRCCTRL( *this, "snapCombo", wxComboBox );
		ERPWidgetSnapMode snapMode = m_viewport->GetTransformManager()->GetSnapMode();
		if( snapMode == SNAP_ToTerrainVisual )
		{
			snapCombo->SetSelection( 1 );
		} 
		else if( snapMode == SNAP_ToStaticCollision )
		{
			snapCombo->SetSelection( 3 );
		}
		else if ( snapMode == SNAP_ToTerrainPhysical )
		{
			snapCombo->SetSelection( 2 );
		}
		else
		{
			snapCombo->SetSelection( 0 );
		}


		ERPWidgetSnapOrgin snapOrgin = m_viewport->GetTransformManager()->GetSnapOrgin();

		ERPWidgetMode mode = m_viewport->m_widgetManager->GetWidgetMode();
		CheckWidgetUI( XRCID("widgetSnapNone"), snapMode == SNAP_ToNothing );
		CheckWidgetUI( XRCID("widgetSnapTerrainVisual"), snapMode == SNAP_ToTerrainVisual );
		CheckWidgetUI( XRCID("widgetSnapTerrainPhysical"), snapMode == SNAP_ToTerrainPhysical );
		CheckWidgetUI( XRCID("widgetSnapCollision"), snapMode == SNAP_ToStaticCollision );
		CheckWidgetUI( XRCID("widgetSnapPivot"), snapOrgin == SNAP_ByPivot );
		CheckWidgetUI( XRCID("widgetSnapBoundingVolume"), snapOrgin == SNAP_ByBoundingVolume );
	}
	else
	{
		CheckWidgetUI( XRCID("editSelectionOnActiveLayer"), false );
		CheckWidgetUI( XRCID("editSelectionOnMultiLayer"), false );
		//CheckWidgetUI( XRCID("selectIsolate"), false );
		wxComboBox* snapCombo = XRCCTRL( *this, "snapCombo", wxComboBox );
		snapCombo->SetSelection( 0 );
		CheckWidgetUI( XRCID("widgetSnapNone"), true );
		CheckWidgetUI( XRCID("widgetSnapTerrainVisual"), false );
		CheckWidgetUI( XRCID("widgetSnapTerrainPhysical"), false );
		CheckWidgetUI( XRCID("widgetSnapCollision"), false );
		CheckWidgetUI( XRCID("widgetSnapPivot"), true );
		CheckWidgetUI( XRCID("widgetSnapBoundingVolume"), false );
	}

	m_mainToolbar->ToggleTool( XRCID("editorStreaming"), GGame->GetActiveWorld() ? GGame->GetActiveWorld()->IsStreamingEnabled() : false );
#ifndef PHYSICS_RELEASE
	m_mainToolbar->ToggleTool( XRCID("editorVDB"), GPhysicsDebugger->IsAttached() );
#else
	m_mainToolbar->ToggleTool( XRCID("editorVDB"), false );
#endif
}

void CEdFrame::CheckWidgetUI( Int32 id, Bool state )
{
	// Update widget
	m_widgets->ToggleTool( id, state );

	// Update menu
	ASSERT( m_gameMenu );
	wxMenuItem* item = m_gameMenu->FindItem( id );
	if ( item )
	{
		item->Check( state );
	}
}

void CEdFrame::ToggleWidgetCyclability( ERPWidgetMode widget )
{
	Bool newState = m_cyclableWidgets[ widget - 1 ] = !m_cyclableWidgets[ widget - 1 ];
	Uint32 id = 0;
	const Char* bmpname = NULL;

	switch ( widget )
	{
	case RPWM_Move:   id = XRCID("widgetModeMove");   bmpname = newState ? L"IMG_MOVE" : L"IMG_MOVE_DISABLED"; break;
	case RPWM_Rotate: id = XRCID("widgetModeRotate"); bmpname = newState ? L"IMG_ROTATE" : L"IMG_ROTATE_DISABLED"; break;
	case RPWM_Scale:  id = XRCID("widgetModeScale");  bmpname = newState ? L"IMG_SCALE" : L"IMG_SCALE_DISABLED"; break;
	default:
		return;
	}

	m_widgets->SetToolNormalBitmap( id, SEdResources::GetInstance().LoadBitmap( bmpname ) );
}

void CEdFrame::OnTogglePerfCounter( wxCommandEvent& event )
{	
	PerfCounterWrapper* wrapper = ( PerfCounterWrapper* ) event.m_callbackUserData;
	CPerfCounter* counter = (CPerfCounter*)wrapper->m_counter;
	ASSERT( counter );
	
	// Update menu
	UpdateProfilerMenu();
}

void CEdFrame::OnShowAllPerfCounters( wxCommandEvent& event )
{
	UpdateProfilerMenu();
}

void CEdFrame::OnHideAllPerfCounters( wxCommandEvent& event )
{
	UpdateProfilerMenu();
}

void CEdFrame::UpdateProfilerMenu()
{
}

wxNotebook* CEdFrame::GetSolutionBar() 
{ 
	return m_solution; 
};

CEdWorldEditPanel* CEdFrame::GetWorldEditPanel( ) 
{ 
	return static_cast< CEdWorldEditPanel* >( m_viewport ); 
};

void CEdFrame::OnOpenShortcutsEditor( wxCommandEvent &event )
{
    class CDummyFrame : public wxSmartLayoutFrame
    {
        wxString            m_sFrameSource;
        CEdShortcutsEditor &m_shortcutEditor;

    public:
        
        CDummyFrame(const wxString &sFrameSource, CEdShortcutsEditor &shortcutEditor, Bool last = true)
            : m_sFrameSource(sFrameSource)
            , m_shortcutEditor(shortcutEditor)
        {
            wxXmlResource::Get()->LoadFrame( this, NULL, m_sFrameSource );

			if ( wxMenuBar* menu = GetMenuBar() ) // Loaded frame can have no menu!
			{
				CEdShortcutsEditor::Load(*GetMenuBar(), GetLabel(), wxString(), true, last);
				m_shortcutEditor.AddItems(*this, *GetMenuBar(), GetLabel());
			}
        }

        bool AddToolBarItems(const wxString &sToolbarName, Bool last = true)
        {
            TEdShortcutArray shortcuts;

            wxToolBar *toolBar = static_cast<wxToolBar*>( FindWindow(sToolbarName) );
            if (toolBar)
                SEdShortcutUtils::AddToolBarItems(shortcuts, *toolBar);
            CEdShortcutsEditor::Load(*this, shortcuts, GetLabel(), false, last); // first load is in the constructor
            m_shortcutEditor.AddItems(*this, shortcuts, GetLabel());
            
            return toolBar != NULL;
        }

        bool AddToolBarItems(const wxString &sToolbarName, const wxString &sPanelName, Bool last)
        {
            TEdShortcutArray shortcuts;

            wxPanel *panel = new wxPanel();
            wxXmlResource::Get()->LoadPanel( panel, this, sPanelName );
            wxToolBar *toolBar = static_cast<wxToolBar*>( panel->FindWindow(sToolbarName) );
            if (toolBar)
                SEdShortcutUtils::AddToolBarItems(shortcuts, *toolBar);
            CEdShortcutsEditor::Load(*panel, shortcuts, GetLabel(), false, last); // first load is in the constructor
            m_shortcutEditor.AddItems(*panel, shortcuts, GetLabel());

            return toolBar != NULL;
        }
    };

    CEdShortcutsEditor* shortcutEditor = new CEdShortcutsEditor(this);

	shortcutEditor->AddItems(*m_assetBrowser, *m_assetBrowser->GetMenuBar(), TXT("Asset Browser"));
	shortcutEditor->AddItems(*m_assetBrowser, *m_assetBrowser->GetAccelerators(), TXT("Asset Browser"));

	CDummyFrame behaviorEditor(TXT("BehaviorEditor"),      *shortcutEditor);

	CEdCutsceneEditor cutsceneEditor( this );
	shortcutEditor->AddItems( cutsceneEditor, *cutsceneEditor.GetMenuBar(), TXT("CutsceneEditor") );
	shortcutEditor->AddItems( cutsceneEditor, *cutsceneEditor.GetAccelerators(), TXT("CutsceneEditor"));

	CDummyFrame entityEditor(TXT("EntityEditor"),          *shortcutEditor);

    // Revert Undo/Redo names to normal ones
    wxMenuItem *undoItem = m_gameMenu->FindItem( XRCID("editUndo") );
    wxMenuItem *redoItem = m_gameMenu->FindItem( XRCID("editRedo") );
	ASSERT ( undoItem && redoItem );
	String prevUndoLabel = wxMenuUtils::ChangeItemLabelPreservingAccel( undoItem, TXT("Undo") );
	String prevRedoLabel = wxMenuUtils::ChangeItemLabelPreservingAccel( redoItem, TXT("Redo") );

    //
    CEdShortcutsEditor::Load(*m_gameMenu, TXT("Main Editor"), wxString(), true, false); // needed only to fill map for the third load
    shortcutEditor->AddItems(*m_gamePanel, *m_gameMenu, TXT("Main Editor"));
    wxToolBar *toolBar = static_cast<wxToolBar*>( m_sceneExplorer->FindWindow(TXT("tlbTools")) );
    if (toolBar)
    {
        TEdShortcutArray shortcuts;
        SEdShortcutUtils::AddToolBarItems(shortcuts, *toolBar, TXT("Scene Explorer"));
        CEdShortcutsEditor::Load(*m_sceneExplorer, shortcuts, TXT("Main Editor"), false, false);  // needed only to fill map for the third load
        shortcutEditor->AddItems(*m_sceneExplorer, shortcuts, TXT("Main Editor"));
    }
    CEdShortcutsEditor::Load(*m_gamePanel, *GetAccelerators(),    TXT("Main Editor"), false, true); // update shortcuts that may have been changed
    shortcutEditor->AddItems(*m_gamePanel, *GetAccelerators(),     TXT("Main Editor"));

    TEdShortcutArray mainToolsShortcuts = m_tools->GetAccelerators();
    CEdShortcutsEditor::Load(*m_viewport, mainToolsShortcuts, TXT("Tools") );
    shortcutEditor->AddItems(*m_viewport, mainToolsShortcuts, TXT("Tools"));
    
	TEdShortcutArray toolsShortcuts = m_tools->GetAllAccelerators();
	CEdShortcutsEditor::Load(*m_viewport, toolsShortcuts, TXT("Tools") );
	shortcutEditor->AddItems(*m_viewport, toolsShortcuts,  TXT("Tools"));

	CEdShortcutsEditor::Load( *GetWorldEditPanel(), *GetWorldEditPanel()->GetAccelerators(), TXT("Main Editor") );
	shortcutEditor->AddItems( *GetWorldEditPanel(), *GetWorldEditPanel()->GetAccelerators(), TXT("Main Editor") );

    CDummyFrame materialEditor(TXT("MaterialEditor"),      *shortcutEditor);

	CDummyFrame textureViewer(TXT("TextureViewer"),      *shortcutEditor);

    CDummyFrame meshEditor(TXT("MeshEditor"),              *shortcutEditor);

    CDummyFrame particleEditor(TXT("ParticleEditor"),      *shortcutEditor, false);
    particleEditor.AddToolBarItems(TXT("previewControlToolBar"), true);
	
	CDummyFrame materialValueMappingEditor(TXT("MaterialValueMapping"),      *shortcutEditor);

	wxFrame someFrame( NULL, wxID_ANY, wxT("") );
	PropertiesPageSettings settings;
	CEdPropertiesPage proPage( &someFrame, settings, nullptr ); // Use external frame as parent, so that this temporary page doesn't mess with the main window*/
	CEdShortcutsEditor::Load(proPage, *proPage.GetAccelerators(), TXT("PropertyPage"));
	shortcutEditor->AddItems(proPage, *proPage.GetAccelerators(), TXT("PropertyPage"));

	PauseConfigTimer();
	shortcutEditor->ShowModal();
	ResumeConfigTimer();

	// update shortcuts that may have been changed - needed only for Dock Widget shortcut,
    // other shortcuts are updated automatically
    CEdShortcutsEditor::Load(*m_gameMenu, TXT("Main Editor"), wxString(), true, false); // needed only to fill map for the third load
    toolBar = static_cast<wxToolBar*>( m_sceneExplorer->FindWindow(TXT("tlbTools")) );
    if (toolBar)
    {
        TEdShortcutArray shortcuts;
        SEdShortcutUtils::AddToolBarItems(shortcuts, *toolBar, TXT("Scene Explorer"));
        CEdShortcutsEditor::Load(*m_sceneExplorer, shortcuts, TXT("Main Editor"), false, false);  // needed only to fill map for the third load
    }
    CEdShortcutsEditor::Load(*m_gamePanel, *GetAccelerators(),    TXT("Main Editor"), false, true); // update shortcuts that may have been changed

	RefreshToolWindows();

	// update Undo/Redo labels back to more descriptive ones.
	wxMenuUtils::ChangeItemLabelPreservingAccel( undoItem, prevUndoLabel );
	wxMenuUtils::ChangeItemLabelPreservingAccel( redoItem, prevRedoLabel );
//    OnUndoManagerHistoryChanged();
}

void CEdFrame::OnMeshColoringNone( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( NULL );
	( new CRenderCommand_ToggleMeshSelectionOverride( false ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshCollisionType( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeCollisionType() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshShadows( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeShadows() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( true );	
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}


void CEdFrame::OnMeshColoringType( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeEntityType() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}


void CEdFrame::OnMeshTextureDensity( wxCommandEvent &event )
{
	CEnvironmentManager::SetModifier( GEngine->GetRenderDebugMode() == MDM_UVDensity ? EMM_None : EMM_GBuffAlbedo );
	GEngine->SetRenderDebugMode( GEngine->GetRenderDebugMode() == MDM_UVDensity ? MDM_None : MDM_UVDensity );

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshSoundMaterial( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeSoundMaterial() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshSoundOccl( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

#ifdef SOUND_OCCLUSSION_MESH_COLORING

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeSoundOccl() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

#endif

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshRenderingLod( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeRendering() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshStreamingLod( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeStreaming() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}

void CEdFrame::OnMeshLayerBuildTag( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeLayerBuildTag() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( true );
}

void CEdFrame::OnMeshChunks( wxCommandEvent &event )
{
	int id = event.GetId();
	Bool isEnabled = m_gameMenu->FindItem( id )->IsChecked();

	GEngine->SetMeshColoringScheme( new CMeshColoringSchemeChunks() );
	( new CRenderCommand_ToggleMeshSelectionOverride( true ) )->Commit();
	CDrawableComponent::RenderingSelectionColorChangedInEditor();

	GetMenuBar()->FindItem( XRCID( "viewMeshesNone" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesCollisionType" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundMaterial" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesSoundOccl" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesShadows" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesChunks" ) )->Check( true );
	GetMenuBar()->FindItem( XRCID( "viewTexturesDensity" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshRenderingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshStreamingLod" ) )->Check( false );
	GetMenuBar()->FindItem( XRCID( "viewMeshesLayerBuildTag" ) )->Check( false );
}


void CEdFrame::OnViewMoveToTerrainLevel( wxCommandEvent &event )
{
	CEdWorldEditPanel* wep = GetWorldEditPanel();
	if ( wep )
	{
		wep->MoveToTerrainLevel();
	}
}

void CEdFrame::OnViewLoadLayersAroundCamera(  wxCommandEvent& event )
{
	static Float radius = 16.0f; // default radius - static so it will be remembered

	// Check if we have an active world
	if ( !GGame->GetActiveWorld() )
	{
		return;
	}

	// Ask the user for the radius
	if ( FormattedDialogBox( wxT("Load Layers Around"), wxT("H{'Enter Range:'|F}|H{~B@'OK'|B'Cancel'}"), &radius ) == 0 )
	{
		LoadLayersWithEntitiesAroundPosition( GGame->GetActiveWorld(), GGame->GetActiveWorld()->GetCameraPosition(), radius );
	}
}

void CEdFrame::OnViewUnloadLayersAroundCamera(  wxCommandEvent& event )
{
	static Float radius = 16.0f; // default radius - static so it will be remembered

	// Check if we have an active world
	if ( !GGame->GetActiveWorld() )
	{
		return;
	}

	// Ask the user for the radius
	if ( FormattedDialogBox( wxT("Unload Layers Around"), wxT("H{'Enter Range:'|F}|H{~B@'OK'|B'Cancel'}"), &radius ) == 0 )
	{
		TDynArray< CLayerGroup* > groups;

		for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
		{
			if ( GGame->GetActiveWorld()->GetCameraPosition().DistanceTo( (*it)->GetWorldPosition() ) <= radius )
			{
				if ( (*it)->GetLayer() != nullptr && (*it)->GetLayer()->GetLayerInfo() && (*it)->GetLayer()->GetLayerInfo()->GetLayerGroup() )
				{
					groups.PushBackUnique( (*it)->GetLayer()->GetLayerInfo()->GetLayerGroup() );
				}
			}
		}

		for ( CLayerGroup* group : groups )
		{
			group->SyncUnload();
		}
	}
}

void CEdFrame::OnViewShowFreeSpaceAroundSelection( wxCommandEvent& event )
{
	GetWorldEditPanel()->ShowFreeSpaceVisualization( event.IsChecked() );
	GetWorldEditPanel()->RefreshSelectionFreeSpaceMesh();
}

void CEdFrame::OnViewLockFreeSpaceVisualization( wxCommandEvent& event )
{
	GetWorldEditPanel()->LockFreeSpaceVisualization( event.IsChecked() );
	GetWorldEditPanel()->RefreshSelectionFreeSpaceMesh();
}

void CEdFrame::OnViewToggleDisplayMode(  wxCommandEvent& event )
{
	// Get choice box
	wxChoice* choice = XRCCTRL( *this, "displayModifier", wxChoice );

	// Toggle
	if ( choice->GetSelection() == m_lastDisplayMode )
	{
		choice->SetSelection( 0 );
	}
	else
	{
		choice->SetSelection( m_lastDisplayMode );
	}
		
	// Apply new modifier
	const Int32 modifier = choice->GetSelection();	
	const EEnvManagerModifier newModifier = (modifier < (Uint32)EMM_MAX) ? (EEnvManagerModifier)modifier : EMM_None;
	CEnvironmentManager::SetModifier( newModifier );
}

TEdShortcutArray *CEdFrame::GetAccelerators()
{
    if (m_shortcuts.Empty())
    {
        struct ToolbarShortcutAdder
        {
            static bool Add(TEdShortcutArray &shortcuts, wxToolBar &toolbar, Int32 xrcid, const wxString &id = wxString())
            {
                wxToolBarToolBase *tool = toolbar.FindById(xrcid);
                if (!tool) return false;
                
                if (!id.empty())
                    shortcuts.PushBack(SEdShortcut(id, *tool));
                else
                    shortcuts.PushBack(SEdShortcut(TXT("Toolbar\\") + SEdShortcut::StripToolTipShortcut(tool->GetShortHelp()), *tool));
                return true;
            }
        };

        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("fileNew"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("fileOpen"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("fileRestoreWorld"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("fileSave"));

        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("editCopy"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("editPaste"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("editDelete"));

        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("assetBrowser"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("cookMap"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("playGame"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("toolReslinker"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("editorStreaming"),        TXT("Toolbar\\Toggle Editor Streaming"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("editorVDB"),				TXT("Toolbar\\Toggle havok VDB"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("gridPosition"),           TXT("Toolbar\\Toggle position snapping"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("gridRotation"),           TXT("Toolbar\\Toggle rotation snapping"));
        ToolbarShortcutAdder::Add(m_shortcuts, *m_mainToolbar, XRCID("widgetStamper"));

        m_shortcuts.PushBack( SEdShortcut( TXT("Accelerators\\Dock Widget"), wxAcceleratorEntry(wxACCEL_CTRL, 'D', XRCID( "viewDock" )) ) );

        TEdShortcutArray *filterShortcuts = m_filter->GetAccelerators();
        if (filterShortcuts)
            for (size_t i = 0; i < filterShortcuts->Size(); ++i)
                m_shortcuts.PushBack((*filterShortcuts)[i]);
    }

    return &m_shortcuts;
}

void CEdFrame::OnAccelFilter( wxCommandEvent& event )
{
    m_filter->OnAccelFilter(event);
}

void CEdFrame::OnEditorStreaming( wxCommandEvent& event )
{
	if ( GGame->GetActiveWorld() )
	{
		if ( event.GetEventObject() == this )
		{
			m_mainToolbar->ToggleTool( XRCID("editorStreaming"), !GGame->GetActiveWorld()->IsStreamingEnabled() );
		}
		GGame->GetActiveWorld()->EnableStreaming( m_mainToolbar->GetToolState( XRCID("editorStreaming") ) );
	}
	else
	{
		m_mainToolbar->ToggleTool( XRCID("editorStreaming"), false );
	}
}

Float positionGridValues[] = { 0.01f, 0.0125f, 0.025f, 0.05f, 0.1f, 0.125f, 0.25f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 5.0f, 10.0f, 12.5f, 25.0f, 50.0f, 100.0f };
const char* positionGridNames[] = { "1 cm", "1.25 cm", "2.5 cm", "5 cm", "10 cm", "12.5 cm", "25 cm", "0.5 m", "1 m", "1.25 m", "1.5 m", "1.75 m", "2 m", "2.5 m", "5 m", "10 m", "12.5 m", "25 m", "50 m", "100 m" };
Float rotaionGridValues[] = { 1.0f, 3.0f, 5.0f, 10.0f, 15.0f, 30.0f, 45.0f, 90.0f };
const char* rotationGridNames[] = { "1°", "3°", "5°", "10°", "15°", "30°", "45°", "90°" };

void CEdFrame::OnPositionGridToggle( wxCommandEvent& event )
{
    if (event.GetEventObject() == this) // Raised by Accelerator, and not a Toolbar Button click, so we must toggle it ourselves
        m_mainToolbar->ToggleTool( XRCID("gridPosition"), !m_mainToolbar->GetToolState( XRCID("gridPosition") ));
	if ( m_mainToolbar->GetToolState( XRCID("gridPosition") ) )
	{
		m_mainToolbar->ToggleTool( XRCID("gridLength"), false );
	}
	CaptureGridSettings();
	UpdateGridWidgets();
}

void CEdFrame::OnPositionGridChange( wxCommandEvent& event )
{
	CaptureGridSettings();
}

void CEdFrame::OnPositionGridLengthToggle( wxCommandEvent& event )
{
	if (event.GetEventObject() == this) // Raised by Accelerator, and not a Toolbar Button click, so we must toggle it ourselves
		m_mainToolbar->ToggleTool( XRCID("gridLength"), !m_mainToolbar->GetToolState( XRCID("gridLength") ));
	if ( m_mainToolbar->GetToolState( XRCID("gridLength") ) )
	{
		m_mainToolbar->ToggleTool( XRCID("gridPosition"), false );
	}
	CaptureGridSettings();
	UpdateGridWidgets();
}

void CEdFrame::OnRotationGridToggle( wxCommandEvent& event )
{
    if (event.GetEventObject() == this) // Raised by Accelerator, and not a Toolbar Button click, so we must toggle it ourselves
        m_mainToolbar->ToggleTool( XRCID("gridRotation"), !m_mainToolbar->GetToolState( XRCID("gridRotation") ));
	CaptureGridSettings();
	UpdateGridWidgets();
}

void CEdFrame::OnRotationGridChange( wxCommandEvent& event )
{
	CaptureGridSettings();
}

void CEdFrame::CaptureGridSettings()
{
	// Buttons
	m_gridSettings.m_usePositionGrid = m_mainToolbar->GetToolState( XRCID("gridPosition") );
	m_gridSettings.m_usePositionGridLength = m_mainToolbar->GetToolState( XRCID("gridLength") );
	m_gridSettings.m_useRotationGrid = m_mainToolbar->GetToolState( XRCID("gridRotation") );

	// Position grid size combo
	if ( m_gridSettings.m_usePositionGrid )
	{
		wxComboBox* combo = XRCCTRL( *this, "gridPositionCombo", wxComboBox );
		if ( combo->IsEnabled() )
		{
			m_gridSettings.m_positionGrid = positionGridValues[ combo->GetSelection() ];
		}
	}

	// Rotation grid size combo
	if ( m_gridSettings.m_useRotationGrid )
	{
		wxComboBox* combo = XRCCTRL( *this, "gridRotationCombo", wxComboBox );
		if ( combo->IsEnabled() )
		{
			m_gridSettings.m_rotationGrid = rotaionGridValues[ combo->GetSelection() ];
		}
	}
}

void CEdFrame::UpdateGridWidgets()
{
	// Buttons
	m_mainToolbar->ToggleTool( XRCID("gridPosition"), m_gridSettings.m_usePositionGrid );
	m_mainToolbar->ToggleTool( XRCID("gridLength"), m_gridSettings.m_usePositionGridLength );
	m_mainToolbar->ToggleTool( XRCID("gridRotation"), m_gridSettings.m_useRotationGrid );

	// Position grid combo
	wxComboBox* combo = XRCCTRL( *this, "gridPositionCombo", wxComboBox );
	if ( m_gridSettings.m_usePositionGrid || m_gridSettings.m_usePositionGridLength )
	{
		combo->Freeze();
		combo->Clear();

		// Append grid values
		ASSERT( ARRAY_COUNT(positionGridValues) == ARRAY_COUNT(positionGridNames) );
		for ( Uint32 i=0; i<ARRAY_COUNT(positionGridValues); i++ )
		{
			combo->Append( ANSI_TO_UNICODE( positionGridNames[ i ] ) ); 
			if ( m_gridSettings.m_positionGrid == positionGridValues[i] )
			{
				combo->SetSelection( i );
			}
		}

		// Select neutral grid suze
		if ( combo->GetSelection() == -1 )
		{
			combo->SetSelection( 8 ); // 1 m 
		}

		// Enable combo
		combo->Enable( true );
		combo->Thaw();
		combo->Refresh();
	}
	else
	{
		// Disable combo
		combo->Freeze();
		combo->Clear();
		combo->Append( TXT("(off)" ) );
		combo->SetSelection( 0 );
		combo->Enable( false );
		combo->Thaw();
		combo->Refresh();
	}

	// Rotation grid combo
	combo = XRCCTRL( *this, "gridRotationCombo", wxComboBox );
	if ( m_gridSettings.m_useRotationGrid )
	{
		combo->Freeze();
		combo->Clear();

		// Append grid values
		ASSERT( ARRAY_COUNT(rotaionGridValues) == ARRAY_COUNT(rotationGridNames) );
		for ( Uint32 i=0; i<ARRAY_COUNT(rotationGridNames); i++ )
		{
			combo->Append( ANSI_TO_UNICODE( rotationGridNames[ i ] ) ); 
			if ( m_gridSettings.m_rotationGrid == rotaionGridValues[i] )
			{
				combo->SetSelection( i );
			}
		}

		// Select neutral grid suze
		if ( combo->GetSelection() == -1 )
		{
			combo->SetSelection( 4 ); // 15°
		}

		// Enable combo
		combo->Enable( true );
		combo->Thaw();
		combo->Refresh();
	}
	else
	{
		// Disable combo
		combo->Freeze();
		combo->Clear();
		combo->Append( TXT("(off)" ) );
		combo->SetSelection( 0 );
		combo->Enable( false );
		combo->Thaw();
		combo->Refresh();
	}
}

void CEdFrame::OnDebugSoundRangeMin( wxCommandEvent& event )
{

}

void CEdFrame::OnDebugSoundRangeMax( wxCommandEvent& event )
{

}

#include "meshStatsViewerFrame.h"

void CEdFrame::OnWorldSceneDebuggerTool( wxCommandEvent& event )
{
	if ( !GGame->GetActiveWorld() )
	{
		wxMessageBox( wxT("There is no world loaded"), wxT("No world"), wxOK|wxCENTRE|wxICON_ERROR, this );
		return;
	}
	if( m_worldSceneDebuggerWindow == nullptr )
	{
		m_worldSceneDebuggerWindow = new CEdWorldSceneDebugger( this );
		m_worldSceneDebuggerWindow->Bind( wxEVT_CLOSE_WINDOW, &CEdFrame::OnCloseWorldSceneDebuggerWindow, this );
	}
	m_worldSceneDebuggerWindow->Show();
}

void CEdFrame::OnCloseWorldSceneDebuggerWindow( wxCloseEvent& event )
{
	m_worldSceneDebuggerWindow->Destroy();
	m_worldSceneDebuggerWindow = nullptr;
}

#include "virtualMemWalkerDlg.h"

void CEdFrame::OnMemWalkerTool( wxCommandEvent& event )
{
	static CEdVirtualMemWalkerDlg dlg( this );
	dlg.Show();
}

void CEdFrame::OnCollisionMemUsageTool( wxCommandEvent& event )
{
	CCollisionMemUsageTool * pCollisionMemUsageTool = new CCollisionMemUsageTool( this );
	pCollisionMemUsageTool->Show();
}

void CEdFrame::OnToolMoveEntities( wxCommandEvent& event )
{
	if ( !m_moveEntityTool )
	{
		m_moveEntityTool = new CEdMoveEntity( this );
	}

	m_moveEntityTool->Show();
	m_moveEntityTool->SetFocus();
}

void CEdFrame::LoadOptionsFromConfig()
{
	m_userConfig.Load();

    int nViewMenuIdx = m_gameMenu->FindMenu( TXT("View") );
    wxMenu* viewMenu = m_gameMenu->GetMenu(nViewMenuIdx);

	SEdPopupNotification::GetInstance().LoadOptionsFromConfig();

	switch ( SEdPopupNotification::GetInstance().GetLocation() )
	{
	case CEdPopupNotification::CENTER:
		viewMenu->FindItem( ID_VIEW_NOTIFICATION_CENTER )->Check();
		break;
	case CEdPopupNotification::TOP_LEFT:
		viewMenu->FindItem( ID_VIEW_NOTIFICATION_TOP_RIGHT )->Check();
		break;
	case CEdPopupNotification::TOP_RIGHT:
		viewMenu->FindItem( ID_VIEW_NOTIFICATION_TOP_RIGHT )->Check();
		break;
	case CEdPopupNotification::BOTTOM_LEFT:
		viewMenu->FindItem( ID_VIEW_NOTIFICATION_BOTTOM_LEFT )->Check();
		break;
	case CEdPopupNotification::BOTTOM_RIGHT:
		viewMenu->FindItem( ID_VIEW_NOTIFICATION_BOTTOM_RIGHT )->Check();
		break;
	}

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

    Bool selectionTracking = config.Read( TXT("/MainFrame/SelectionTracking"), 1 ) != 0;
	m_viewport->SetSelectionTracking( selectionTracking );

	//SetAssetBrowserDockable( config.Read( TXT("/Frames/AssetBrowser/Frame"), 1 ) == 0 );

	{
		// restore frame position and size
		CConfigurationScopedPathSetter pathSetter( config, TXT("/MainFrame/Window") );

		Int32	x = config.Read( TXT("X"), 50 ),
				y = config.Read( TXT("Y"), 50 ),
				w = config.Read( TXT("Width"), 1024 ),
				h = config.Read( TXT("Height"), 768 );

		SmartSetSize( x, y, w, h );

		bool maximized = config.Read( TXT("Maximized"), 1 ) == 1;
		Maximize( maximized );

		// restore tool windows
		for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
		{
			CToolWindowState &state = it->m_second;
			String path = TXT("/ToolWindow/") + ToString( it->m_first ) + TXT("/");
			Bool isVisible = config.Read( path + TXT("Visible"), state.isVisible ? 1 : 0 ) == 1 ? true : false;
			Bool isFloat = config.Read( path + TXT("Float"), state.isFloat ? 1 : 0 ) == 1 ? true : false;
			UpdateToolWindowState( it->m_first, isVisible, isFloat, false );
		}
		RefreshToolWindows();
	}

	{
		// restore AUI layout
		CConfigurationScopedPathSetter pathSetter( config, TXT("/MainFrame/AUI") );

		String layout = config.Read( TXT("Layout"), String::EMPTY );
		if( !layout.Empty() )
			m_auiMgr.LoadPerspective( layout.AsChar() );
	}

	m_isViewportFloating = m_auiMgr.GetPane( m_viewport ).IsFloating();
	// make sure everything is set
	ToggleFloatingViewport();

	// correct size for new buttons
	m_auiMgr.GetPane( TXT("Toolbar") ).window->Fit();
	m_auiMgr.GetPane( TXT("Toolbar") ).BestSize( m_auiMgr.GetPane( TXT("Toolbar") ).window->GetSize() ); 
	wxSize sz = m_widgets->GetBestSize();
	m_widgets->SetMinSize( sz );
	//	m_widgets->Fit();
	//	m_widgets->Refresh( true );
	m_auiMgr.GetPane( m_widgets ).BestSize( sz );
	//	m_widgets->GetParent()->SetMinSize( sz );

    CEdShortcutsEditor::Load(*m_clonedMenu,     TXT("Main Editor"), wxEmptyString, true, true);
    CEdShortcutsEditor::Load(*m_gameMenu,       TXT("Main Editor"), wxEmptyString, true, false);
    wxToolBar *toolBar = static_cast<wxToolBar*>( m_sceneExplorer->FindWindow(TXT("tlbTools")) );
    if (toolBar)
    {
        TEdShortcutArray shortcuts;
        SEdShortcutUtils::AddToolBarItems(shortcuts, *toolBar, TXT("Scene Explorer"));
        CEdShortcutsEditor::Load(*m_sceneExplorer, shortcuts, TXT("Main Editor"), false, false);
        
    }
    CEdShortcutsEditor::Load(*m_gamePanel, *GetAccelerators(), TXT("Main Editor"), false, true);

	m_assetBrowser->LoadOptionsFromConfig();

	m_filterSmartPanel->LoadLayout( TXT("/Frames/Filter") );
	m_notebook->LoadOptionsFromConfig( );

	wxString selectedEditor = config.Read( TXT("/Editors/Selected"), TXT("World") ).AsChar();
	if (selectedEditor.Length() > 0)
		for ( Uint32 i = 0; i < m_notebook->GetPageCount(); ++i )
			if (m_notebook->GetPageText(i) == selectedEditor)
			{
				m_notebook->SetSelection(i);
				break;
			}

	m_timer = new CEdFrameTimer( this );

	RestoreWindowsStack( config );

	if( HasCommandLineParameter( TXT("restore_session") ) )
	{
		OnRestoreWorld( wxCommandEvent() );
	}
	else
	{
		CConfigurationScopedPathSetter pathSetter( config, config.GetSessionPath() );
		RestoreSession( config );
	}

	// Language
	String language = config.Read( TXT( "/MainFrame/Language" ), TXT( "PL" ) );
	m_languageChoice->Select( m_languageChoice->FindString( language.AsChar() ) );
	if ( m_languageChoice->GetSelection() == wxNOT_FOUND ) m_languageChoice->Select( 0 );

	// Change locale
	SLocalizationManager::GetInstance().SetCurrentLocale( m_languageChoice->GetStringSelection().wc_str() );
	SEvents::GetInstance().QueueEvent( CNAME( CurrentLocaleChanged ), NULL ); 

	// Connection to string db
	Int32 stringDbConnection = config.Read( TXT( "/MainFrame/IsConnectedToStringDB" ), 0 );
	if( stringDbConnection != 0 && !SLocalizationManager::GetInstance().IsConnected() )
	{
		ConnectToStringDb();
	}

	// Load sound banks
	// CSoundBankLoader::ReloadSoundbanks( false );

	// Sounds mute
	Int32 isMuted = config.Read( TXT( "/MainFrame/AreSoundsMuted" ), 0 );
	if( isMuted == 1 )
	{
		GSoundSystem->SoundEvent( "mute_sounds" );

		// Update icon
		m_mainToolbar->ToggleTool( XRCID( "muteSound" ), true );
	}

	isMuted = config.Read( TXT( "/MainFrame/IsMusicMuted" ), 0 );
	if( isMuted == 1 )
	{
		GSoundSystem->SoundEvent( "mute_music" );

		// Update icon
		m_mainToolbar->ToggleTool( XRCID( "muteMusic" ), true );
	}

	// Game definition resource
	String gameResourcePath = config.Read( TXT( "/MainFrame/GameResource" ), String::EMPTY );
	GGame->SetupGameResourceFromFile( gameResourcePath );
	UpdateGameResourceNameField();

	// Umbra usage
#ifdef USE_UMBRA
	Int32 isUmbraActive = config.Read( TXT( "/Umbra/IsActive" ), 1 );
	Bool useOcclusionCulling = isUmbraActive == 1;
	CUmbraScene::UseOcclusionCulling( useOcclusionCulling );
	m_mainToolbar->ToggleTool( XRCID( "occlusionCullingUsage" ), useOcclusionCulling );
#endif
}

void CEdFrame::SaveOptionsToConfig()
{
	m_userConfig.Save();

	SEdPopupNotification::GetInstance().SaveOptionsToConfig();

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

    config.Write( TXT("/MainFrame/SelectionTracking"), m_viewport->GetSelectionTracking() ? 1 : 0 );

	// save AUI layout
	String frameConfig = m_auiMgr.SavePerspective().wc_str();
	config.Write( TXT("/MainFrame/AUI/Layout"), frameConfig );

	config.Write( TXT( "/MainFrame/Language" ), m_languageChoice->GetStringSelection().wc_str() );
    //config.Write( TXT("/Frames/AssetBrowser/Frame"), m_assetBrowserFrame != NULL ? 1 : 0 );

	// Connection to string db
	config.Write( TXT( "/MainFrame/IsConnectedToStringDB" ),
		SLocalizationManager::GetInstance().IsConnected() ? 1 : 0 );

	// save frame position & size
	// get info from native window placement to keep last (not maximized) size
	WINDOWPLACEMENT wp;
	GetWindowPlacement( GetHwnd(), &wp );

	Int32 x = wp.rcNormalPosition.left,
		y = wp.rcNormalPosition.top,
		w = wp.rcNormalPosition.right - x,
		h = wp.rcNormalPosition.bottom - y;

	WorkspaceToScreen( x, y );
	
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/MainFrame/Window") );
		config.Write( TXT("X"), x );
		config.Write( TXT("Y"), y );
		config.Write( TXT("Width"), w );
		config.Write( TXT("Height"), h );
		config.Write( TXT("Maximized"), (long) IsMaximized() );
	}

	for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
	{
		CToolWindowState &state = it->m_second;
		String path = TXT("/ToolWindow/") + ToString( it->m_first ) + TXT("/");
		config.Write( path + TXT("Visible"), state.isVisible ? 1 : 0 );
		config.Write( path + TXT("Float"), state.isFloat ? 1 : 0 );
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( world )
		config.Write( TXT("/Global/LastSession"), world->DepotPath() );


	m_filterSmartPanel->SaveLayout( TXT("/Frames/Filter") );

	m_notebook->SaveOptionsToConfig();

	SaveWindowsStack( config );

	config.Write( TXT("/Editors/Selected"), m_notebook->GetPageText(m_notebook->GetSelection()).GetData().AsWChar() );

	// Sounds mute
	config.Write( TXT( "/MainFrame/AreSoundsMuted" ), m_mainToolbar->GetToolState( XRCID( "muteSound" ) ) ? 1 : 0 );
	config.Write( TXT( "/MainFrame/IsMusicMuted" ), m_mainToolbar->GetToolState( XRCID( "muteMusic" ) ) ? 1 : 0 );

	// Game definition resource
	config.Write( TXT( "/MainFrame/GameResource" ), GGame->GetGameResourcePath() );

	// Umbra usage
#ifdef USE_UMBRA
	config.Write( TXT( "/Umbra/IsActive" ), CUmbraScene::IsUsingOcclusionCulling() ? 1 : 0 );
#endif
}

void CEdFrame::SaveSession( CConfigurationManager &config )
{
	m_assetBrowser->SaveSession( config );

	if( m_solution->GetPageCount() )
		config.Write( TXT("ToolPage"), m_solution->GetSelection() );

	// Mode
	ERPWidgetMode mode = m_viewport->m_widgetManager->GetWidgetMode();
	config.Write( TXT("Viewport/Mode"), mode );
	// Space
	ERPWidgetSpace space = m_viewport->m_widgetManager->GetWidgetSpace();
	config.Write( TXT("Viewport/Space"), space );
	// Grid
	config.Write( TXT("Grid/Position/Enabled"), m_gridSettings.m_usePositionGrid ? 1 : 0 );
	config.Write( TXT("Grid/Length/Enabled"), m_gridSettings.m_usePositionGridLength ? 1 : 0 );
	config.Write( TXT("Grid/Rotation/Enabled"), m_gridSettings.m_useRotationGrid ? 1 : 0 );
	config.Write( TXT("Grid/Position/Value"), ToString( m_gridSettings.m_positionGrid ) );
	config.Write( TXT("Grid/Rotation/Value"), ToString( m_gridSettings.m_rotationGrid ) );

	wxSlider* smoothSlider = XRCCTRL( *this, "stamperSlider", wxSlider );
	config.Write( TXT("Stamper/Smooth"), ToString( smoothSlider->GetValue() ) );

//	config.Write( TXT("Streaming"), GGame->IsStreaming() ? 1 : 0 );

	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		ESelectMode selectMode = m_viewport->GetSelectionManager()->GetSelectMode();
		config.Write( TXT("Mode/Selection"), selectMode );
		ERPWidgetSnapMode snapMode = m_viewport->GetTransformManager()->GetSnapMode();
		config.Write( TXT("Mode/Snap"), snapMode );
		ERPWidgetSnapOrgin snapOrgin = m_viewport->GetTransformManager()->GetSnapOrgin();
		config.Write( TXT("Mode/SnapOrgin"), snapOrgin );

		config.Write( TXT("Camera/Position/X"),	ToString( GetWorldEditPanel()->m_cameraPosition.X ) );
		config.Write( TXT("Camera/Position/Y"),	ToString( GetWorldEditPanel()->m_cameraPosition.Y ) );
		config.Write( TXT("Camera/Position/Z"),	ToString( GetWorldEditPanel()->m_cameraPosition.Z ) );

		config.Write( TXT("Camera/Rotation/X"),	ToString( GetWorldEditPanel()->m_cameraRotation.Roll ) );
		config.Write( TXT("Camera/Rotation/Y"),	ToString( GetWorldEditPanel()->m_cameraRotation.Pitch ) );
		config.Write( TXT("Camera/Rotation/Z"),	ToString( GetWorldEditPanel()->m_cameraRotation.Yaw ) );

		// Save world viewport bookmarks
		GWorldCameraBookmarks.SaveSession( config );

		TDynArray< CEntity* > nodes;
		m_viewport->GetSelectionManager()->GetSelectedEntities( nodes );
		String names;
		for( TDynArray< CEntity* >::iterator it=nodes.Begin(); it!=nodes.End(); it++ )
			names += ( *it )->GetUniqueName() + TXT( ";" );
		config.Write( TXT("Scene/Selection"), names );

		// dex_fix!!!
/*		if( m_renderFilter )
		{
			config.Write( TXT("Scene/Isolated/Enabled"), 1 );
			String isolated;
			for( TDynArray< CEntity* >::iterator it=m_renderFilter->m_entities.Begin(); it!=m_renderFilter->m_entities.End(); it++ )
				isolated += ( *it )->GetUniqueName() + TXT( ";" );
			config.Write( TXT("Scene/Isolated/Entities"), isolated );
		}
		else
		{
			config.Write( TXT("Scene/Isolated/Enabled"), 0 );
		}*/
	}
}

void CEdFrame::RestoreSession( CConfigurationManager &config )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		GEngine->FlushJobs();
		//SJobManager::GetInstance().FlushPendingJobs();
		//GEngine->Tick( 0 );
	}

	if( m_solution->GetPageCount() )
		m_solution->SetSelection( config.Read( TXT("ToolPage"), 0 ) );

	m_assetBrowser->RestoreSession( config );

	Int32 mode = config.Read( TXT("Viewport/Mode"), -1 );
	m_viewport->m_widgetManager->SetWidgetMode( mode == RPWM_Rotate ? RPWM_Rotate : ( mode == RPWM_Scale ? RPWM_Scale : RPWM_Move ) );
	Int32 space = config.Read( TXT("Viewport/Space"), -1 );
	m_viewport->m_widgetManager->SetWidgetSpace( space == RPWS_Local ? RPWS_Local : RPWS_Global );

	m_gridSettings.m_usePositionGrid = config.Read( TXT("Grid/Position/Enabled"), 1 ) == 1 ? true : false;
	m_gridSettings.m_usePositionGridLength = config.Read( TXT("Grid/Length/Enabled"), 1 ) == 1 ? true : false;
	m_gridSettings.m_useRotationGrid = config.Read( TXT("Grid/Rotation/Enabled"), 1 ) == 1 ? true : false;
	Float f = 0.0f;
	if( FromString( config.Read( TXT("Grid/Position/Value"), String::EMPTY ), f ) )
		m_gridSettings.m_positionGrid = f;
	if( FromString( config.Read( TXT("Grid/Rotation/Value"), String::EMPTY ), f ) )
		m_gridSettings.m_rotationGrid = f;

	wxSlider* smoothSlider = XRCCTRL( *this, "stamperSlider", wxSlider );
	smoothSlider->SetValue( config.Read( TXT("Stamper/Smooth"), 50 ) );

	// Set streaming always true, regardless of config file (ini)
	//GGame->SetStreaming( config.Read( TXT("Streaming"), 0 ) == 1 ? true : false );
//	GGame->SetStreaming( true );

	m_tools->RestoreSession( config );
	m_sceneExplorer->RestoreSession( config );

	if ( world )
	{
		Int32 selectMode = config.Read( TXT("Mode/Selection"), -1 );
		m_viewport->GetSelectionManager()->SetSelectMode( selectMode == SM_ActiveLayer ? SM_ActiveLayer : SM_MultiLayer );
		Int32 snapMode = config.Read( TXT("Mode/Snap"), -1 );
		ERPWidgetSnapMode snapModeEnum = SNAP_ToNothing;
		if ( snapMode >= 0 && snapMode <= SNAP_ToStaticCollision )
		{
			snapModeEnum = (ERPWidgetSnapMode) snapMode;
		}
		m_viewport->GetTransformManager()->SetSnapMode( snapModeEnum );

		Int32 snapOrgin = config.Read( TXT("Mode/SnapOrgin"), -1 );
		ERPWidgetSnapOrgin snapOrginEnum = SNAP_ByPivot;
		if ( snapOrgin >= 0 && snapOrgin <= SNAP_ByBoundingVolume )
		{
			snapOrginEnum = (ERPWidgetSnapOrgin) snapOrgin;
		}
		m_viewport->GetTransformManager()->SetSnapOrgin( snapOrginEnum );

		Float none = -99999.9f;
		Float cx = config.Read(TXT("Camera/Position/X"), none );
		Float cy = config.Read(TXT("Camera/Position/Y"), none );
		Float cz = config.Read(TXT("Camera/Position/Z"), none );
		if( cx != none && cy != none && cz != none )
			GetWorldEditPanel()->m_cameraPosition.Set3( cx, cy, cz );
		Float rx = config.Read(TXT("Camera/Rotation/X"), none );
		Float ry = config.Read(TXT("Camera/Rotation/Y"), none );
		Float rz = config.Read(TXT("Camera/Rotation/Z"), none );
		if( rx != none && ry != none && rz != none )
			GetWorldEditPanel()->m_cameraRotation = EulerAngles( rx, ry, rz );
		
		GetWorldEditPanel()->GetWorld()->EnableStreaming( false, false );
		GetWorldEditPanel()->OnCameraMoved();
		GetWorldEditPanel()->GetWorld()->EnableStreaming( true, false );


		// Restore world viewport bookmarks
		GWorldCameraBookmarks.RestoreSession( config );

		//restore selected items
        CSelectionManager::CSelectionTransaction transaction(*m_viewport->GetSelectionManager());
		m_viewport->GetSelectionManager()->DeselectAll();
		String nodes = config.Read( TXT("Scene/Selection"), String::EMPTY );
		if ( world && !nodes.Empty() )
		{
			TSet< String > parts;
			nodes.Slice( parts, TXT( ";" ) );

			{
				// Format selection path
				for ( WorldAttachedEntitiesIterator it( world ); it; ++it )
				{
					CEntity* entity = *it;
					if ( !entity->IsSelected() )
					{
						String name = entity->GetUniqueName();
						if ( parts.Find( name ) != parts.End() )
						{
							m_viewport->GetSelectionManager()->Select( entity );
						}
					}
				}
			}
		}

		Int32 isolated = config.Read( TXT("Scene/Isolated/Enabled"), 0 );
		// dex_fix!!!
		//if( m_renderFilter )
		//{
			//delete m_renderFilter;
			//m_renderFilter = NULL;
		//}

		// Isolated selection
		if ( isolated == 1 )
		{
			String isolatedEntitiesStr = config.Read( TXT("Scene/Isolated/Entities"), String::EMPTY );

			// Filter isolated entities
			TDynArray< CEntity* > isolatedEntities;
			{
				// Slice config path
				TSet< String > parts;
				isolatedEntitiesStr.Slice( parts, TXT( ";" ) );

				// Format selection path
				for ( WorldAttachedEntitiesIterator it( world ); it; ++it )
				{
					CEntity* entity = *it;
					String name = entity->GetUniqueName();
					if ( parts.Find( name ) != parts.End() )
					{
						isolatedEntities.PushBack( entity );
					}
				}
			}

			// Add to isolated
			// dex_fix!!!
			//m_renderFilter = new CEdFrameRenderFilter();
			//m_renderFilter->m_entities = isolatedEntities;
			AddToIsolated( isolatedEntities );
		}
		// dex_fix!!!
//		world->SetRenderFilter( m_renderFilter );
	
		// Apply forced daycycle if the world environment editor isn't active
		if ( !m_worldEnvironmentEditor && !GGame->IsActive() && world->GetEnvironmentManager() )
		{
			// Read world environment tool's config
			CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/WorldEnvironmentTool") );
			Bool fakeDaycycleEnabled = config.Read( TXT("fakeDayCycleEnabled"), false ) != 0;
			Float fakeDaycycleProgress = config.Read( TXT("fakeDayCycleProgress"), 0 );

			if ( fakeDaycycleEnabled )
			{
				// Modify game environment parameters
				RunLaterOnce( [ fakeDaycycleProgress ]() {
					if ( CWorld* world = GGame->GetActiveWorld() )
					{
						CGameEnvironmentParams gameParams = world->GetEnvironmentManager()->GetGameEnvironmentParams();
						gameParams.m_dayCycleOverride.m_fakeDayCycleEnable = true;
						gameParams.m_dayCycleOverride.m_fakeDayCycleHour = Lerp( fakeDaycycleProgress / 400.0f, 0.f, 24.f );
						world->GetEnvironmentManager()->SetGameEnvironmentParams( gameParams );
						SEvents::GetInstance().DispatchEvent( CNAME( SimpleCurveDisplayedTimeChanged ), NULL );
						GGame->GetActiveWorld()->GetEnvironmentManager()->UpdateCurrentAreaEnvironment();
					}
				} );
			}
		}
	}

	UpdateWidgetUI();
	UpdateGridWidgets();
}

void CEdFrame::PushWindow( wxTopLevelWindow *w )
{
	if ( m_windowsStack.Remove( w ) )
	{
		m_windowsStack.PushBack( w );
	}
}

void CEdFrame::WindowCreated( wxTopLevelWindow *w )
{
	m_windowsStack.Insert( 0, w );
}

void CEdFrame::WindowDestroyed( wxTopLevelWindow *w )
{
	m_windowsStack.Remove( w );
}

void CEdFrame::SaveWindowsStack( CConfigurationManager &config )
{
	String order;
	if( m_windowsStack.Size() )
	{
		for( TDynArray< wxWindow* >::iterator it=m_windowsStack.Begin(); it!=m_windowsStack.End(); it++ )
			if( ( *it )->IsShown() )
				order += ( *it )->GetLabel() + TXT(";");
	}
	config.Write( TXT("/WindowsOrder"), order );
}

void CEdFrame::RestoreWindowsStack( CConfigurationManager &config )
{
	String order = config.Read( TXT("/WindowsOrder"), String::EMPTY );
	if( !order.Empty() )
	{
		TDynArray< String > labels;
		order.Slice( labels, TXT(";") );
		THashMap< String, wxWindow* > window;
		for( TDynArray< wxWindow* >::iterator it=m_windowsStack.Begin(); it!=m_windowsStack.End(); it++ )
		{
			window.Insert( ( *it )->GetLabel().wc_str(), *it );
		}

		for( TDynArray< String >::iterator it=labels.Begin(); it!=labels.End(); it++ )
		{
			wxWindow** windowPtr = window.FindPtr( *it );
			if ( windowPtr )
			{
				(*windowPtr)->Raise();
			}
		}
	}
}

void CEdFrame::OnGameEditorPageShow( wxAuiNotebookEvent& event )
{
	if ( event.GetEventObject() == m_notebook )
	{
        if ( GetEditorsBar()->GetPage( event.GetSelection() ) == m_gamePanel )
        {
            if ( GetMenuBar() != m_gameMenu )
            {
                while ( Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED ) ) {}
                SetMenuBar( m_gameMenu );
                m_viewport->SetFocus();
            }
        }
        else
		{
            if ( GGame->IsActive() && !GGame->IsPaused() )
			{
                GGame->Pause( TXT( "CEdFrame" ) );
			}
		}
	}

    event.Skip();
}

void CEdFrame::OnGameEditorClosePage( wxAuiNotebookEvent& event )
{
    if ( event.GetEventObject() == m_notebook && GetEditorsBar()->GetPage( event.GetSelection() ) == m_gamePanel )
	{
		OnCloseWorld( wxCommandEvent() );
        event.Veto();
	}
    else
        event.Skip();
}

void CEdFrame::OnSolutionDragStart( wxMouseEvent &event )
{
    m_isSolutionTabDrag = true;
    m_solution->CaptureMouse();
    m_solutionTabDragStartPoint = event.GetPosition();
    m_solution->SetCursor( wxCURSOR_SIZING );
    event.Skip();
}

void CEdFrame::OnSolutionDragMove ( wxMouseEvent &event )
{
    if (m_isSolutionTabDrag)
    {
        wxPoint curPos = event.GetPosition();
        wxPoint delta  = m_solutionTabDragStartPoint - curPos;

        // Check if the tab has been dragged more than the 5px threshold distance
        if ( abs( delta.x ) > 5 || abs( delta.y ) > 5 )
        {// Undock the tab
            m_isSolutionTabDrag = false;
            m_solution->SetCursor( wxCURSOR_ARROW );
            m_solution->ReleaseMouse();

            wxWindow *page = m_solution->GetPage( m_solution->GetSelection() );

            auto toolCurr = m_toolWindowState.Begin(),
                                                    toolLast = m_toolWindowState.End();
            for ( ; toolCurr != toolLast ; ++toolCurr )
                if (toolCurr->m_second.widget == page)
                {
                    UpdateToolWindowState( toolCurr->m_first, true, true, true );
                    m_auiMgr.GetPane(page).FloatingPosition( curPos.x - 5, curPos.y - 5 );
                    m_auiMgr.Update();
                    page->SetFocus();
#if 0
                    class wxAuiHack : public wxAuiManager
                    {
                    public:
                        void StartDrag( wxWindow *pane )
                        {
                            m_action = actionDragFloatingPane;
                            m_action_window = GetPane(pane).frame;
                            m_action_offset = wxPoint(30, 10);
                            m_frame->CaptureMouse();
                        }
                    };

                    static_cast<wxAuiHack*>( &m_auiMgr )->StartDrag( page );
#endif
					m_auiMgr.StartPaneDrag( page, wxPoint( 30, 10 ) );
                    break;
                }
        }
    }
    else
        event.Skip();
}

void CEdFrame::OnSolutionDragEnd  ( wxMouseEvent &event )
{
    if (m_isSolutionTabDrag)
    {// There were no undocking, just tab selection
        m_isSolutionTabDrag = false;
        m_solution->SetCursor( wxCURSOR_ARROW );
        m_solution->ReleaseMouse();
    }
    else
        event.Skip();
}

void CEdFrame::OnSolutionDockWidget( wxCommandEvent &event )
{
    for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
    {
        if( it->m_second.isFloat )
        {
            wxAuiPaneInfo &pane_info = m_auiMgr.GetPane( it->m_second.widget );
            if (pane_info.frame && pane_info.frame == event.GetEventObject())
            {
                UpdateToolWindowState( it->m_first, true, false, true );
                return;
            }
        }
    }
}

CEdFrameTimer::CEdFrameTimer( CEdFrame* owner )
: m_owner( owner ),
  m_first( true )
{
	Start( 5000 );
}

void CEdFrameTimer::Pause()
{
	Stop();
}

void CEdFrameTimer::Unpause()
{
	if( m_first )
		Start( 5000 );
	else
		Start( 10000 );
}

void CEdFrameTimer::NotifyOnce()
{
	CUserConfigurationManager &configUser = SUserConfigurationManager::GetInstance();
	CCommonConfigurationManager &configCommon = SCommonConfigurationManager::GetInstance();
	if( m_first )
	{
		//m_owner->LoadOptionsFromConfig();

		CConfigurationScopedPathSetter pathSetter( configUser, configUser.GetSessionPath() );
		m_owner->RestoreWindowsStack( configUser );

		Start( 10000 );
		m_first = false;
	}
	else
	{
		// Save config
		if ( !GGame->IsActive() )
		{
			configUser.SaveAll();
			configCommon.SaveAll();
		}

#ifndef NO_DATA_VALIDATION
		if( wxTheFrame != nullptr )
		{
			if( wxTheFrame->GetDataErrorReporter() != nullptr )
			{
				wxTheFrame->GetDataErrorReporter()->UpdateContent();
			}
		}
#endif
	}
}


// Save changed dialog
BEGIN_EVENT_TABLE( CSaveChangedDlg, wxDialog )
	EVT_BUTTON( XRCID("Save"), CSaveChangedDlg::OnSave )
	EVT_BUTTON( XRCID("Skip"), CSaveChangedDlg::OnSkip )
	EVT_BUTTON( XRCID("Choose"), CSaveChangedDlg::OnChoose )
	EVT_BUTTON( XRCID("Cancel"), CSaveChangedDlg::OnCancel )
	EVT_BUTTON( XRCID("Cancel2"), CSaveChangedDlg::OnCancel )
	EVT_BUTTON( XRCID("Ok"), CSaveChangedDlg::OnOk )
END_EVENT_TABLE()

CSaveChangedDlg::CSaveChangedDlg( wxWindow* parent, TDynArray< CDiskFile* > &files )
: m_custom( false )
, m_files( files )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("SaveChangesDialog") );

	wxScrolledWindow* scroll = XRCCTRL( *this, "Grid", wxScrolledWindow );

	wxPanel *grid = new wxPanel( scroll );
	wxBoxSizer *scrollSizer = new wxBoxSizer( wxVERTICAL );

	scrollSizer->Add( grid, 1, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer *gridSizer = new wxFlexGridSizer( 3, 5, 5 );
	//gridSizer->SetFlexibleDirection( wxHORIZONTAL );
	gridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gridSizer->AddGrowableCol( 0, 4 );
	gridSizer->AddGrowableCol( 1, 1 );
	gridSizer->AddGrowableCol( 2, 1 );

	for( Uint32 i=0; i<m_files.Size(); i++ )
	{
		//wxStaticText *label = new wxStaticText( grid, -1, m_files[ i ]->GetDepotPath().AsChar() );
		wxTextCtrl *label = new wxTextCtrlEx( grid, -1, m_files[ i ]->GetDepotPath().AsChar(), wxDefaultPosition, wxSize( 220, -1 ), wxBORDER_NONE );
		label->SetEditable( false );
		wxCheckBox *checkSave = new wxCheckBox( grid, -1, TXT("Save") );
		wxCheckBox *checkSkip = new wxCheckBox( grid, -1, TXT("Skip") );
		m_checkboxSave.PushBack( checkSave );
		m_checkboxSkip.PushBack( checkSkip );
		checkSkip->SetValue( true );
		checkSave->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CSaveChangedDlg::OnCheckboxSave ), new TCallbackData< Int32 >( i ), this );
		checkSkip->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CSaveChangedDlg::OnCheckboxSkip ), new TCallbackData< Int32 >( i ), this );

		gridSizer->Add( label );
		gridSizer->Add( checkSave );
		gridSizer->Add( checkSkip );
	}

	grid->SetSizer( gridSizer );
	gridSizer->Layout();

	scroll->SetSizer( scrollSizer );
	scroll->EnableScrolling( true, true );
	scroll->SetScrollRate( 1, 1 ); 
	scrollSizer->FitInside( scroll );
	scrollSizer->Layout();
	Layout();

	//scroll->Hide();
	//Layout();
}

CSaveChangedDlg::~CSaveChangedDlg()
{

}

void CSaveChangedDlg::OnSave( wxCommandEvent& event )
{
	if( !m_custom )
	{
		wxString failedItems;
		for( Uint32 i=0; i<m_files.Size(); i++ )
		{
			Bool ret;
			CLayer *layer = Cast< CLayer >( m_files[ i ]->GetResource() );
			if( layer )
			{
				ret = layer->GetLayerInfo()->Save();
			}
			else
			{
				ret = m_files[ i ]->Save();
			}
			if( !ret )
			{
				failedItems += m_files[ i ]->GetDepotPath().AsChar();
				failedItems += TXT("\n");
			}
		}

		if( !failedItems.IsEmpty() )
		{
			Hide();
			CDetailsDlg dlg( this, TXT("Save error"), TXT("Some files cannot be saved. Quit anyway?"), failedItems );
			if( dlg.DoModal() == wxID_OK )
				EndDialog( ESC_SaveAll );
			else
				EndDialog( ESC_Cancel );
			return;
		}

		EndDialog( ESC_SaveAll );
	}
	else
	{
		for( Uint32 i=0; i<m_checkboxSave.Size(); i++ )
		{
			m_checkboxSave[ i ]->SetValue( true );
			m_checkboxSkip[ i ]->SetValue( false );
		}
	}
}

void CSaveChangedDlg::OnSkip( wxCommandEvent& event )
{
	if( !m_custom )
	{
		EndDialog( ESC_SkipAll );
	}
	else
	{
		for( Uint32 i=0; i<m_checkboxSave.Size(); i++ )
		{
			m_checkboxSave[ i ]->SetValue( false );
			m_checkboxSkip[ i ]->SetValue( true );
		}
	}
}

void CSaveChangedDlg::OnChoose( wxCommandEvent& event )
{
	wxPanel* panel = XRCCTRL( *this, "PanelChoose", wxPanel );
	panel->Show();
	wxButton* btn = XRCCTRL( *this, "Choose", wxButton );
	btn->Hide();

	wxButton* btnCancel = XRCCTRL( *this, "Cancel", wxButton );
	btnCancel->Hide();

	wxButton* btnSave = XRCCTRL( *this, "Save", wxButton );
	btnSave->SetLabel( TXT("Mark all Save") );
	wxButton* btnSkip = XRCCTRL( *this, "Skip", wxButton );
	btnSkip->SetLabel( TXT("Mark all Skip") );

	m_custom = true;

	//Layout();
	SetSize( -1, 300 );
	//Refresh( true );
	//EndDialog( ESC_User );
}

void CSaveChangedDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( ESC_Cancel );
}

ESaveChangedDlgReturn CSaveChangedDlg::DoModal()
{
	return ( ESaveChangedDlgReturn ) wxDialog::ShowModal();
}

void CSaveChangedDlg::OnCheckboxSave( wxCommandEvent& event )
{
	Int32 id = ( ( TCallbackData< Int32 >* )( event.m_callbackUserData ) )->GetData();
	if( m_checkboxSave[ id ]->IsChecked() )
	{
		m_checkboxSkip[ id ]->SetValue( false );
	}
	else
	{
		m_checkboxSkip[ id ]->SetValue( true );
	}
}

void CSaveChangedDlg::OnCheckboxSkip( wxCommandEvent& event )
{
	Int32 id = ( ( TCallbackData< Int32 >* )( event.m_callbackUserData ) )->GetData();
	if( m_checkboxSkip[ id ]->IsChecked() )
	{
		m_checkboxSave[ id ]->SetValue( false );
	}
	else
	{
		m_checkboxSave[ id ]->SetValue( true );
	}
}

void CSaveChangedDlg::OnOk( wxCommandEvent& event )
{
	wxString failedItems;
	for( Uint32 i=0; i<m_files.Size(); i++ )
		if( m_checkboxSave[ i ]->IsChecked() )
		{
			Bool ret;
			CLayer *layer = Cast< CLayer >( m_files[ i ]->GetResource() );
			if( layer )
			{
				ret = layer->GetLayerInfo()->Save();
			}
			else
			{
				ret = m_files[ i ]->Save();
			}
			if( !ret )
			{
				failedItems += m_files[ i ]->GetDepotPath().AsChar();
				failedItems += TXT("\n");
			}
		}
	if( !failedItems.IsEmpty() )
	{
		Hide();
		CDetailsDlg dlg( this, TXT("Save error"), TXT("Some files cannot be saved. Quit anyway?"), failedItems );
		if( dlg.DoModal() == wxID_OK )
			EndDialog( ESC_SaveAll );
		else
			EndDialog( ESC_Cancel );
		return;
	}
	EndDialog( ESC_Custom );
}

void CEdFrame::OnPerformancePlatformClicked( wxCommandEvent& event )
{
	m_inGameConfigDialog->SelectSettingsTab( "video.general" );
	m_inGameConfigDialog->Execute();
}

void CEdFrame::OnSpawnSetDebugger( wxCommandEvent& event )
{
	CCommunityDebuggerEditor *commDebugEd = new CCommunityDebuggerEditor( this );
	commDebugEd->Show();
}

void CEdFrame::OnQuestsDebugger( wxCommandEvent& event )
{
	TDynArray< IQuestEdTool* > tools;
	tools.PushBack( new CQuestsDebugger() );
	tools.PushBack( new CQuestScenesDebugger() );
	tools.PushBack( new CEdFactsDB() );
	tools.PushBack( new CQuestExecutionLog() );
	tools.PushBack( new CQuestThreadsDebugger() );
	tools.PushBack( new CQuestControlledNPCsLog() );

	CEdQuestEditor* questsEditor = new CEdQuestEditor( this, nullptr, tools );
	questsEditor->Show();
}

void CEdFrame::OnBehaviorDebugger( wxCommandEvent& event )
{
	/*if ( !GGame || !GGame->GetActiveWorld() )
	{
		return;
	}

	CEdTagMiniEditor tagEditor( this, TDynArray<CName>() );
	if ( tagEditor.ShowModal() != wxOK )
	{
		return;
	}

	TagList tagList;
	tagList.AddTags( tagEditor.GetTags() );

	if ( tagList.GetTags().Size() == 0 )
	{
		return;
	}

	TDynArray< CEntity* > entities;
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( tagList, entities, BCTO_MatchAll );

	CEntity* selectedEntity = NULL;

	if ( entities.Size() == 1 )
	{
		selectedEntity = entities[0];
	}
	else if ( entities.Size() > 0 )
	{
		TDynArray< String > list;

		for ( Uint32 i=0; i<entities.Size(); ++i )
		{
			list.PushBack( entities[i]->GetName() );
		}

		const String selectedName = InputComboBox( this, TXT("Select entity"), TXT(""), list[0], list );

		for ( Uint32 i=0; i<list.Size(); ++i )
		{
			if ( list[i] == selectedName )
			{
				selectedEntity = entities[i];
				break;
			}
		}
	}
	else
	{
		return;
	}

	TDynArray< CAnimatedComponent* > animComponents;
	selectedEntity->GetComponentsOfClass( animComponents );

	CAnimatedComponent* selectedComponent = NULL;

	//if ( animComponents.Size() == 1 )
	//{
	//	selectedComponent = animComponents[0];
	//}
	//else if ( animComponents.Size() > 0 )
	//{
	//	TDynArray< String > list;

	//	for ( Uint32 i=0; i<animComponents.Size(); ++i )
	//	{
	//		list.PushBack( animComponents[i]->GetName() );
	//	}

	//	const String selectedName = InputComboBox( this, TXT("Select component"), TXT(""), list[0], list );

	//	for ( Uint32 i=0; i<list.Size(); ++i )
	//	{
	//		if ( list[i] == selectedName )
	//		{
	//			selectedComponent = animComponents[i];
	//			break;
	//		}
	//	}
	//}
	//else
	//{
	//	return;
	//}

	selectedComponent = selectedEntity->GetRootAnimatedComponent();
	if ( !selectedComponent )
	{
		return;
	}*/

	extern void StartBehaviorGraphDebug( CEntity* entity );
	StartBehaviorGraphDebug( NULL );
}

void CEdFrame::OnBehaviorGroupDebugger( wxCommandEvent& event )
{
	CEdBehaviorsListener* behDebug = new CEdBehaviorsListener( this );
	behDebug->Show();
}

#include "animFriendPageIkAttack.h"
#include "animFriendPageHitReaction.h"
#include "animFriendPageStepPlayer.h"
#include "animFriendPagePointCloudLookAt.h"

void CEdFrame::OnAnimationFriend( wxCommandEvent& event )
{
	CEdAnimationFriend* animFriend = new CEdAnimationFriend( this );
	
	static Int32 OPTION = 3;

	if ( OPTION == 0 )
	{
		animFriend->SetAnimationFilter( new CEdAnimationParamFilter< CSkeletalAnimationAttackTrajectoryParam >() );
		animFriend->AddPage( new CEdAnimationFriendIkAttackPage( animFriend ) );
	}
	else if ( OPTION == 1 )
	{
		animFriend->SetAnimationFilter( new CEdAnimationParamFilter< CSkeletalAnimationHitParam >() );
		animFriend->AddPage( new CEdAnimationFriendHitReactionPage( animFriend ) );
	}
	else if ( OPTION == 2 )
	{
		animFriend->SetAnimationFilter( new CEdAnimationParamFilter< CSkeletalAnimationStepClipParam >() );
		animFriend->AddPage( new CEdAnimationFriendStepPlayerPage( animFriend ) );
	}
	else if ( OPTION == 3 )
	{
		animFriend->SetAnimationFilter( new CEdAnimationParamFilter< CAnimPointCloudLookAtParam >() );
		animFriend->AddPage( new CEdAnimationPointCloudLookAtPlayerPage( animFriend ) );
	}

	animFriend->Show();
}

void CEdFrame::ExportGlobalMappins( wxCommandEvent& event )
{
	CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( commonMapManager )
	{
		commonMapManager->ExportGlobalMapPins();
	}
}

void CEdFrame::ExportEntityMappins( wxCommandEvent& event )
{
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( manager )
	{
		manager->ExportEntityMapPins();
	}
}

void CEdFrame::ExportEntityMappinsForEP1( wxCommandEvent& event )
{
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( manager )
	{
		manager->ExportEntityMapPins( 1 );
	}
}

void CEdFrame::ExportEntityMappinsForEP2( wxCommandEvent& event )
{
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( manager )
	{
		manager->ExportEntityMapPins( 2 );
	}
}

void CEdFrame::ExportQuestMappins( wxCommandEvent& event )
{
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( manager )
	{
		manager->ExportQuestMapPins();
	}
}

void CEdFrame::UpdateMapPinEntities( wxCommandEvent& event )
{
	CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( manager )
	{
		manager->UpdateMapPinEntities( ( 0x8000 & GetAsyncKeyState( VK_LCONTROL ) ) != 0 );
	}
}

RED_DEFINE_STATIC_NAME( CTeleportEntity );
RED_DEFINE_STATIC_NAME( CRiftEntity );
RED_DEFINE_STATIC_NAME( CMonsterNestEntity );

RED_DEFINE_STATIC_NAME( linkingMode );
RED_DEFINE_STATIC_NAME( pairedNodeTag );
RED_DEFINE_STATIC_NAME( controlledEncounterTag );
RED_DEFINE_STATIC_NAME( linkedEncounterTag );

void CEdFrame::TEMP_CheckEntitiesThatUseEntityHandles( wxCommandEvent& event )
{
#ifndef NO_EDITOR
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}
	CLayerGroup* worldLayers = world->GetWorldLayers();
	if ( !worldLayers )
	{
		return;
	}

	TDynArray< CLayerInfo* > layers;
	world->GetWorldLayers()->GetLayers( layers, true );

	CClass* teleportEntityClass  = SRTTI::GetInstance().FindClass( CNAME( CTeleportEntity ) );
	ASSERT( teleportEntityClass );
	CClass* riftEntityClass = SRTTI::GetInstance().FindClass( CNAME( CRiftEntity ) );
	ASSERT( riftEntityClass );
	CClass* monsterNestClass = SRTTI::GetInstance().FindClass( CNAME( CMonsterNestEntity ) );
	ASSERT( monsterNestClass );

	RED_LOG( RED_LOG_CHANNEL( Bug35356 ), TXT("----------------------------------------") );

	Uint32 teleportFail = 0, teleportSuccess = 0;
	Uint32 riftFail = 0, riftSuccess = 0;
	Uint32 monsterNestFail = 0, monsterNestSuccess = 0;

	for ( Uint32 k = 0; k < layers.Size(); k++ )
	{
		CLayerInfo* info = layers[k];
		if ( info->IsLoaded() )
		{
			CLayer* layer = info->GetLayer();

			TDynArray<CEntity*> entities;
			layer->GetEntities( entities );
			for ( Uint32 i = 0; i < entities.Size(); ++i )
			{
				CGameplayEntity* entity = Cast< CGameplayEntity >( entities[ i ] );

				if ( !entity )
				{
					continue;
				}
				// continue if it's not a proper class
				if ( !entity->IsA( teleportEntityClass ) &&
					 !entity->IsA( riftEntityClass ) &&
					 !entity->IsA( monsterNestClass ) )
				{
					continue;
				}

				// teleport
				if ( teleportEntityClass && entity->IsA( teleportEntityClass ) )
				{
					CProperty* modeProperty = teleportEntityClass->FindProperty( CNAME( linkingMode ) );
					CProperty* tagProperty = teleportEntityClass->FindProperty( CNAME( pairedNodeTag ) );
					ASSERT( modeProperty );
					ASSERT( tagProperty );
					if ( !modeProperty || !tagProperty )
					{
						continue;
					}

					Bool linkingMode = false;
					modeProperty->Get( entity, &linkingMode );
					CName tag;
					tagProperty->Get( entity, &tag );

					if ( linkingMode || tag.Empty() )
					{
						String entityPath = entity->GetLayer()->GetDepotPath();

						size_t index;
						if ( entityPath.FindSubstring( TXT(".w2l"), index, true ) )
						{
							entityPath = entityPath.LeftString( index );
						}
						entityPath += TXT( "\\" );
						entityPath += entity->GetDisplayName();

						RED_LOG( RED_LOG_CHANNEL( Bug35356 ), TXT("TELEPORT    %s"), entityPath.AsChar() );
						teleportFail++;
					}
					else
					{
						teleportSuccess++;
					}
				}
				else if ( riftEntityClass && entity->IsA( riftEntityClass ) )
				{
					CProperty* modeProperty = riftEntityClass->FindProperty( CNAME( linkingMode ) );
					CProperty* tagProperty = riftEntityClass->FindProperty( CNAME( controlledEncounterTag ) );
					ASSERT( modeProperty );
					ASSERT( tagProperty );
					if ( !modeProperty || !tagProperty )
					{
						continue;
					}

					Bool linkingMode = false;
					modeProperty->Get( entity, &linkingMode );
					CName tag;
					tagProperty->Get( entity, &tag );

					if ( linkingMode || tag.Empty() )
					{
						String entityPath = entity->GetLayer()->GetDepotPath();

						size_t index;
						if ( entityPath.FindSubstring( TXT(".w2l"), index, true ) )
						{
							entityPath = entityPath.LeftString( index );
						}
						entityPath += TXT( "\\" );
						entityPath += entity->GetDisplayName();

						RED_LOG( RED_LOG_CHANNEL( Bug35356 ), TXT("RIFT        %s"), entityPath.AsChar() );
						riftFail++;
					}
					else
					{
						riftSuccess++;
					}
				}
				else if ( monsterNestClass && entity->IsA( monsterNestClass ) )
				{
					CProperty* modeProperty = monsterNestClass->FindProperty( CNAME( linkingMode ) );
					CProperty* tagProperty = monsterNestClass->FindProperty( CNAME( linkedEncounterTag ) );
					ASSERT( modeProperty );
					ASSERT( tagProperty );
					if ( !modeProperty || !tagProperty )
					{
						continue;
					}

					Bool linkingMode = false;
					modeProperty->Get( entity, &linkingMode );
					CName tag;
					tagProperty->Get( entity, &tag );

					if ( linkingMode || tag.Empty() )
					{
						String entityPath = entity->GetLayer()->GetDepotPath();

						size_t index;
						if ( entityPath.FindSubstring( TXT(".w2l"), index, true ) )
						{
							entityPath = entityPath.LeftString( index );
						}
						entityPath += TXT( "\\" );
						entityPath += entity->GetDisplayName();

						RED_LOG( RED_LOG_CHANNEL( Bug35356 ), TXT("MONSTERNEST %s"), entityPath.AsChar() );
						monsterNestFail++;
					}
					else
					{
						monsterNestSuccess++;						
					}
				}
			}
		}
	}

	if ( teleportFail + riftFail + monsterNestFail == 0 )
	{
		GFeedback->ShowMsg( TXT("Info"), TXT("The entities seem to be OK") );
	}
	else
	{
		String text = TXT("Some entities still use entity handles.\nPlease look at details in log channel 'Bug35356' and fix the entites.\n\n");
		if ( teleportFail > 0 )
			text += String::Printf( TXT("Teleports: %d/%d\n"), teleportFail, teleportFail + teleportSuccess );
		if ( riftFail > 0 )
			text += String::Printf( TXT("Rifts: %d/%d\n"), riftFail, riftFail + riftSuccess );
		if ( monsterNestFail > 0 )
			text += String::Printf( TXT("Monster nests: %d/%d\n"), monsterNestFail, monsterNestFail + monsterNestSuccess );

		GFeedback->ShowMsg( TXT("Info"), text.AsChar() );
	}

#endif //NO_EDITOR
}

void CEdFrame::UpdateFocusEntityTemplates( wxCommandEvent& event )
{
	if ( !GCommonGame->IsActive() )
	{
		CUpdateFocusEntityTemplatesDlg* dlg = new CUpdateFocusEntityTemplatesDlg( this );
		dlg->Show();
	}
	else
	{
		GFeedback->ShowError( TXT("Stop the game before resaving templates") );
	}
}

void CEdFrame::DumpCommunityAgents( wxCommandEvent& event )
{
	if ( !GCommonGame->IsActive() )
	{
		GFeedback->ShowError( TXT("The game must be running in order to dump active agents") );
		return;
	}

	if ( CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >() )
	{
		if ( !communitySystem->GetAgentsWorld().Dump() )
		{
			GFeedback->ShowError( TXT("Community agents dump failed - see logs") );
			return;
		}

		GFeedback->ShowMsg( TXT("Info"), TXT("Community agents dumped to CSV and logs") );
	}
}

void CEdFrame::GlobalSpawnTreesResaving( wxCommandEvent& event )
{
	if ( GCommonGame->GetActiveWorld() )
	{
		GFeedback->ShowError( TXT("Close world in order for spawn tree nodes ids regeneration to progress.") );
		return;
	}

	static Uint32 garbage_counter = 0;
	static Uint32 totalCountTrees = 0;

	struct Local
	{
		static void DoDirectoryLevel( CDirectory* dir )
		{
			for ( auto file : dir->GetFiles() )
			{
				const String& fileName = file->GetFileName();

				if ( !fileName.EndsWith( CSpawnTree::GetFileExtension() ) )
				{
					continue;
				}

				if ( file->Load() )
				{
					THandle< CResource > res = file->GetResource();
					if ( res )
					{
						res->AddToRootSet();
						const String& filePath = file->GetDepotPath();

						CSpawnTree* const tree = SafeCast< CSpawnTree >( res );
						const Bool changed = tree->GenerateIdsRecursively();
						if ( changed )
						{
							file->SilentCheckOut();
							if ( file->Save() )
							{
								RED_LOG( SPAWNTREE_RESAVING, TXT("File '%s' successfully resaved.\n"), filePath.AsChar() );
								++totalCountTrees;
							}
							else
							{
								RED_LOG( SPAWNTREE_RESAVING, TXT("File '%s' failed to save.\n"), filePath.AsChar() );
							}
						}

						res->RemoveFromRootSet();
						file->Unload();

						if ( garbage_counter > 50 )
						{
							SGarbageCollector::GetInstance().CollectNow();
							garbage_counter = 0;
						}
					}
				}
			}

			for ( CDirectory* childDir : dir->GetDirectories() )
			{
				DoDirectoryLevel( childDir );
			}
		}
	};
	
	Local::DoDirectoryLevel( GDepot );
	RED_LOG( SPAWNTREE_RESAVING, TXT("Resaved %d spawn trees.\n"), totalCountTrees );
}

void CEdFrame::GlobalEncounterLayersResaving( wxCommandEvent& event )
{
	if ( GCommonGame->GetActiveWorld() )
	{
		GFeedback->ShowError( TXT("Close world in order for spawn tree nodes ids regeneration to progress.") );
		return;
	}

	static Uint32 garbage_counter = 0;
	static Uint32 totalCountEncounters = 0;
	static Uint32 totalCountEncountersResaved = 0;

	struct Local
	{
		static void DoDirectoryLevel( CDirectory* dir )
		{
			if ( dir->GetName() == String( L"qa" ) )
				return;

			for ( auto file : dir->GetFiles() )
			{
				const String& worldFileName = file->GetFileName();

				if ( !worldFileName.EndsWith( CWorld::GetFileExtension() ) )
				{
					continue;
				}

				const String& worldFilePath = file->GetDepotPath();

				WorldLoadingContext loadingContext;

				CWorld* const world = CWorld::LoadWorld( worldFilePath, loadingContext );
				if ( !world )
				{
					RED_LOG( ENCOUNTER_RESAVING, TXT("Can't load the '%s' world resource\n"), worldFilePath.AsChar() );
					continue;
				}
				TDynArray< CLayerInfo* > layersToLoad;

				CLayerGroup* const rootLayers = world->GetWorldLayers();
				rootLayers->GetLayers( layersToLoad, false, true, false );

				// iterate over all layers
				for ( auto itLayers = layersToLoad.Begin(), endLayers = layersToLoad.End(); itLayers != endLayers; ++itLayers )
				{
					// process layer
					{
						CLayerInfo* const layerInfo = *itLayers;
						LayerLoadingContext context;
						context.m_loadHidden = true;

						if ( !layerInfo->SyncLoad( context ) )
						{
							continue;
						}
						// iterate over all entities at the layer
						CLayer* const layer = layerInfo->GetLayer();
						if ( !layer )
						{
							continue;
						}

						// stream all
						const LayerEntitiesArray& entities = layer->GetEntities();
						Bool hadEncounter = false;

						for ( auto entity : entities )
						{
							if ( CEncounter* const encounter = Cast< CEncounter >( entity ) )
							{
								const Bool changed = encounter->GenerateIdsRecursively();
								if ( changed )
								{
									hadEncounter = true;
									++totalCountEncounters;
								}
							}
						}
								
						if ( hadEncounter )
						{
							CDiskFile* const layerFile = layer->GetFile();
							layerFile->SilentCheckOut();
							if ( layerFile->Save() )
							{
								RED_LOG( ENCOUNTER_RESAVING, TXT("Layer '%s' successfully resaved.\n"), layerFile->GetDepotPath().AsChar() );
								++totalCountEncountersResaved;
							}
							else
							{
								RED_LOG( ENCOUNTER_RESAVING, TXT("Layer '%s' failed to save.\n"), layerFile->GetDepotPath().AsChar() );
							}
						}
						layerInfo->SyncUnload();
					}
				}
				CWorld::UnloadWorld( world );

				if ( garbage_counter > 50 )
				{
					SGarbageCollector::GetInstance().CollectNow();
					garbage_counter = 0;
				}
			}

			for ( CDirectory* childDir : dir->GetDirectories() )
			{
				DoDirectoryLevel( childDir );
			}
		}
	};

	Local::DoDirectoryLevel( GDepot );
	RED_LOG( ENCOUNTER_RESAVING, TXT("Resaved %d encounters layers and total %d encounters.\n"), totalCountEncountersResaved, totalCountEncounters );
}

void CEdFrame::AIGlobalRefactor_IAITree( wxCommandEvent& event )
{
	if ( GCommonGame->GetActiveWorld() )
	{
		GFeedback->ShowError( TXT("Close world in order for global AI refactor to progress.") );
		return;
	}

	static Uint32 s_Counter = 0;

	struct Local
	{
		static void DoDirectoryLevel( CDirectory* dir )
		{
			for ( auto file : dir->GetFiles() )
			{
				const String& fileName = file->GetFileName();

				if ( !fileName.EndsWith( CEntityTemplate::GetFileExtension() ) 
					&& !fileName.EndsWith( CQuestPhase::GetFileExtension() )
					&& !fileName.EndsWith( CQuest::GetFileExtension() )
					&& !fileName.EndsWith( CBehTree::GetFileExtension() ) )
				{
					continue;
				}

				if ( file->Load() )
				{
					THandle< CResource > res = file->GetResource();
					if ( res )
					{
						res->AddToRootSet();
						const String& filePath = file->GetDepotPath();
						// Its old type resource. Almost for sure its one of ours!
						

						Bool modified = IAITree::RefactorAll();

						if ( !modified )
						{
							RED_LOG( Gui, TXT("File '%s' not modified (no refactor needed).\n"), filePath.AsChar() );
						}
						else
						{
							file->SilentCheckOut();
							if ( file->Save() )
							{
								RED_LOG( Gui, TXT("File '%s' successfully resaved.\n"), filePath.AsChar() );
							}
							else
							{
								RED_LOG( Gui, TXT("File '%s' failed to save.\n"), filePath.AsChar() );
							}
						}

						res->RemoveFromRootSet();
						file->Unload();

						if ( modified || s_Counter > 50 )
						{
							SGarbageCollector::GetInstance().CollectNow();
							s_Counter = 0;
						}
					}
				}
			}

			for ( CDirectory* childDir : dir->GetDirectories() )
			{
				DoDirectoryLevel( childDir );
			}
		}
	};
	
	Local::DoDirectoryLevel( GDepot );
}

void CEdFrame::AIGlobalRefactor_CBehTreeTask( wxCommandEvent& event )
{
	if ( GCommonGame->GetActiveWorld() )
	{
		GFeedback->ShowError( TXT("Close world in order for global AI refactor to progress.") );
		return;
	}

	struct Local
	{
		static void DoDirectoryLevel( CDirectory* dir )
		{
			for ( auto file : dir->GetFiles() )
			{
				const String& fileName = file->GetFileName();

				if ( !fileName.EndsWith( CBehTree::GetFileExtension() ) )
				{
					continue;
				}

				if ( file->Load() )
				{
					THandle< CBehTree > behTree = Cast< CBehTree >( file->GetResource() );
					if ( behTree )
					{
						behTree->AddToRootSet();
						const String& filePath = file->GetDepotPath();
						// Its old type resource. Almost for sure its one of ours!
						file->SilentCheckOut();

						IBehTreeObjectDefinition::RefactorAll();

						if ( behTree->Save() )
						{
							RED_LOG( Gui, TXT("Behtree '%s' resaved.\n"), filePath.AsChar() );
						}
						else
						{
							RED_LOG( Gui, TXT("Behtree '%s' couldn't be saved.\n"), filePath.AsChar() );
						}

						behTree->RemoveFromRootSet();
						file->Unload();
					}
				}
			}

			for ( CDirectory* childDir : dir->GetDirectories() )
			{
				DoDirectoryLevel( childDir );
			}
		}
	};


	Local::DoDirectoryLevel( GDepot );
}

void CEdFrame::OnQuestBlocksValidator( wxCommandEvent& event )
{
	THashMap< CGUID, TDynArray< CQuestGraphBlock* > > duplicatedGUIDs;
	THashMap< CQuestPhase*, TDynArray< CQuest* > >    questRefs;
	THashSet< CResource* > resources;
	TDynArray< CResource* > resourcesToSave;
	TDynArray< String > resourceFilenames;

	GFeedback->BeginTask( TXT("Getting quest list..."), false );

	//CDirectory* dir = GDepot->FindDirectory( TXT( "qa\\pawelm_test\\" ) );
	//dir->FindResourcesByExtension( extensions[ e ], resourceFilenames );
	GDepot->FindResourcesByExtension( TXT("w2quest"), resourceFilenames );

	for( Uint32 i = 0; i != resourceFilenames.Size(); ++i )
	{
		GFeedback->UpdateTaskInfo( TXT("Checking '%s'... (%i/%i)"), resourceFilenames[ i ].AsChar(), i + 1, resourceFilenames.Size() );
		GFeedback->UpdateTaskProgress( i, resourceFilenames.Size() );

		CQuest* currQuest = Cast< CQuest >( GDepot->LoadResource( resourceFilenames[ i ] ) );
		if ( currQuest )
		{
			if ( !resources.Exist( currQuest ) )
			{
				currQuest->AddToRootSet();
				resources.Insert( currQuest );
			}

			CQuestGraph* graph = currQuest->GetGraph();
			if ( graph )
			{
				CheckGraph( graph, duplicatedGUIDs, questRefs, currQuest );
			}
		}
	}

	Uint32 duplicatedGuidsCount = 0;
	Uint32 duplicatedBlocksCount = 0;
	for ( THashMap< CGUID, TDynArray< CQuestGraphBlock* > >::iterator it = duplicatedGUIDs.Begin(); it != duplicatedGUIDs.End(); ++it )
	{
		if ( it->m_second.Size() > 1 )
		{
			duplicatedGuidsCount++;
			duplicatedBlocksCount += ( it->m_second.Size() - 1 );
		}
	}

	GFeedback->EndTask();

	RED_LOG( RED_LOG_CHANNEL( QuestBlockValidator ), TXT("------------------------------------------") );

	Uint32 updatedBlocksCount = 0;
	String text = String::Printf( TXT("Number of resources: %d\nNumber of GUIDs: %d\nNumber of duplicated GUIDs: %d\nNumber of blocks with duplicated GUID: %d\n\n"),
		                          resources.Size(), duplicatedGUIDs.Size(), duplicatedGuidsCount, duplicatedBlocksCount );
	if ( duplicatedGuidsCount == 0 )
	{
		text += TXT("No need to resave anything :)");
		GFeedback->ShowMsg( TXT("Info"), text.AsChar() );
	}
	else
	{
		text += TXT("Do you want to recreate GUIDs and resave resources?");
		if ( GFeedback->AskYesNo( text.AsChar() ) )
		{
			GFeedback->BeginTask( TXT("Updating block GUIDs..."), false );

			for ( THashMap< CGUID, TDynArray< CQuestGraphBlock* > >::iterator it = duplicatedGUIDs.Begin(); it != duplicatedGUIDs.End(); ++it )
			{
				if ( it->m_second.Size() > 1 )
				{
					TDynArray< CQuestGraphBlock* >& blocks = it->m_second;
					for ( Uint32 i = 1; i < blocks.Size(); ++i )
					{
						CObject* parent = blocks[ i ]->GetParent();
						while ( parent && !parent->IsA< CResource >() )
						{
							parent = parent->GetParent();
						}
						if ( parent )
						{
							CResource* res = Cast< CResource >( parent );
							if ( res )
							{
								CDiskFile* file = res->GetFile();
								if ( file )
								{
									file->SilentCheckOut( true );
									if ( file->IsCheckedOut() || file->IsLocal() )
									{
										{
											CQuestPhase* realQuestPhase = nullptr;
											TDynArray< String > lines;
											String str;
											CObject* parent = blocks[ i ];
											while ( parent )
											{
												CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( parent );
												CQuestGraph* questGraph = Cast< CQuestGraph >( parent );
												CQuestPhase* questPhase = Cast< CQuestPhase >( parent );
												if ( questBlock )
												{
													String str = String( TXT("          ") ) +
														         TXT("[") + questBlock->GetClass()->GetName().AsString() + TXT("] ") +
													             TXT("[") + questBlock->GetCaption() + TXT("] ") +
													             TXT("[") + questBlock->GetComment() + TXT("] ");
													lines.Insert( 0 , str );
												}
												else if ( questGraph )
												{
													// nothing
												}
												else if ( questPhase )
												{
													CDiskFile* file = questPhase->GetFile();

													String str;
													if ( questPhase->IsA< CQuest >() )
													{
														str = TXT("QUEST  [") + file->GetFileName() + TXT("] ");
													}
													else
													{
														str = TXT("     PHASE  [") + file->GetFileName() + TXT("] ");
														realQuestPhase = questPhase;
													}

													lines.Insert( 0, str );
													break;
												}
												parent = parent->GetParent();
											}

											if ( realQuestPhase )
											{
												TDynArray< CQuest* >* referencedQuests = questRefs.FindPtr( realQuestPhase );
												ASSERT( referencedQuests );
												if ( referencedQuests )
												{
													for ( Uint32 k = 0; k < referencedQuests->Size(); ++k )
													{
														CDiskFile* file = (*referencedQuests)[ k ]->GetFile();
														RED_LOG( RED_LOG_CHANNEL( QuestBlockValidator ), TXT("QUEST* [%s]"), file->GetFileName().AsChar() );
													}
												}
											}
											for ( Uint32 i = 0; i < lines.Size(); ++i )
											{
												RED_LOG( RED_LOG_CHANNEL( QuestBlockValidator ), TXT("%s"), lines[ i ].AsChar() );
											}
										}
										updatedBlocksCount++;
										blocks[ i ]->UpdateGUID();
										res->MarkModified();
										resourcesToSave.PushBackUnique( res );
										continue;
									}
								}
							}
						}
					}
				}
			}

			GFeedback->UpdateTaskInfo( TXT("Saving resources...") );

			Uint32 savedResourcesCount = 0;
			for ( Uint32 i = 0; i < resourcesToSave.Size(); ++i )
			{
				if ( resourcesToSave[ i ]->Save() )
				{
					savedResourcesCount++;
				}
			}

			GFeedback->EndTask();


			GFeedback->ShowMsg( TXT("Info"), TXT("Updated blocks: %d/%d\nSaved resources: %d"), updatedBlocksCount, duplicatedBlocksCount, savedResourcesCount );
		}
	}

	for ( THashSet< CResource* >::iterator it = resources.Begin(); it != resources.End(); ++it )
	{
		(*it)->RemoveFromRootSet();
	}
}

void CEdFrame::CheckGraph( CQuestGraph* graph, THashMap< CGUID, TDynArray< CQuestGraphBlock* > >& duplicatedGUIDs, THashMap< CQuestPhase*, TDynArray< CQuest* > >& questRefs, CQuest* currQuest )
{
	TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();
	for ( Uint32 i = 0; i < blocks.Size(); ++i )
	{
		CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( blocks[ i ] );
		if ( questBlock )
		{
			// add block to map for given guid
			const CGUID& guid = questBlock->GetGUID();
			TDynArray< CQuestGraphBlock* >* foundEntry = duplicatedGUIDs.FindPtr( guid );
			if ( foundEntry )
			{
				foundEntry->PushBackUnique( questBlock );
			}
			else
			{
				TDynArray< CQuestGraphBlock* > sameGuidBlocks;
				sameGuidBlocks.PushBack( questBlock );
				duplicatedGUIDs.Insert( guid, sameGuidBlocks );
			}

			CQuestScopeBlock* questScopeBlock = Cast< CQuestScopeBlock >( questBlock );
			if ( questScopeBlock )
			{
				// check if there is any embedded phase
				CQuestPhase* phase = questScopeBlock->GetPhase();
				if ( phase )
				{
					// add reference to quest for given embedded phase
					TDynArray< CQuest* >* foundEntry = questRefs.FindPtr( phase );
					if ( foundEntry )
					{
						foundEntry->PushBackUnique( currQuest );
					}
					else
					{
						TDynArray< CQuest* > questsUsingThisPhase;
						questsUsingThisPhase.PushBack( currQuest );
						questRefs.Insert( phase, questsUsingThisPhase );
					}
				}

				// recursively search the graph
				CQuestGraph* graph = questScopeBlock->GetGraph();
				if ( graph )
				{
					CheckGraph( graph, duplicatedGUIDs, questRefs, currQuest );
				}
			}
		}
	}
}

void CEdFrame::OnMaraudersMap( wxCommandEvent& event )
{
	CMaraudersMap *maraudersMap = m_viewport->GetMaraudersMap();
	if ( maraudersMap == NULL )
	{
		maraudersMap = new CMaraudersMap( this );
		m_viewport->SetMaraudersMap( maraudersMap );
	}

	maraudersMap->Show();
	maraudersMap->Raise(); // bring to front
}

void CEdFrame::OnSoundsDebugger( wxCommandEvent& event )
{
}

void CEdFrame::OnDataErrorReporter( wxCommandEvent& event )
{
#ifndef NO_DATA_VALIDATION
	ShowDataErrorReporter();
#else
	::wxMessageBox( TXT("Data Error Reporter is disabled."), TXT("Information"), wxOK | wxCENTRE | wxICON_INFORMATION );
#endif
}

void CEdFrame::OnAnimationsReport( wxCommandEvent& event )
{
	CEdAnimationReporterWindow* win = new CEdAnimationReporterWindow( NULL );
	win->Show();
	win->SetFocus();
}

void CEdFrame::OnLipsyncPreview( wxCommandEvent& event )
{
	CEdLipsyncPreview* preview = new CEdLipsyncPreview( NULL );
	preview->Show();
	preview->SetFocus();
}

void CEdFrame::OnLocalizedStringsEditor( wxCommandEvent& event )
{
	//CLocalizedStringsEditor *localizedStringsEditor = new CLocalizedStringsEditor( this );
	//localizedStringsEditor->Show();

	CLocalizedStringsEditor *editor = m_viewport->GetLocStrindEditor();
	if ( editor == NULL )
	{
		editor = new CLocalizedStringsEditor( this );
		m_viewport->SetLocStringsEditor( editor );
	}

	editor->Show();
	editor->Raise(); // bring to front
	editor->SetFocus();
}

void CEdFrame::OnLocalizationTools( wxCommandEvent& event )
{
	CEdLocalizationTool* localizationTool = new CEdLocalizationTool( this );
	localizationTool->Show();
}

void CEdFrame::OnResourceSorterStart( wxCommandEvent& event )
{
	CEdResourceSorterFrame* resourceSequencer = new CEdResourceSorterFrame( this );
	resourceSequencer->Show();
}

void CEdFrame::OnSceneValidator( wxCommandEvent& event )
{
	CStorySceneValidator validator;
	validator.Initialize();
}

void CEdFrame::OnEntitiesBrowser( wxCommandEvent& event )
{
	if( m_solution != nullptr )
	{
		Uint32 count = m_solution->GetPageCount();
		for( Uint32 i=0; i<count; ++i )
		{
			if( m_solution->GetPageText( i ) == GEntityBrowserName )
			{
				m_solution->ChangeSelection( i );
				return;
			}
		}

		// restore entities window
		for( auto it=m_toolWindowState.Begin(); it!=m_toolWindowState.End(); ++it )
		{
			CToolWindowState &state = it->m_second;
			if( state.caption == GEntityBrowserName )
			{
				UpdateToolWindowState( it->m_first, true, false, false );
				m_solution->ChangeSelection( count );
			}
		}
	}
}

static void GetDirectoryFiles( CDirectory* dir, TDynArray< CDiskFile* >& files )
{
	const TFiles & dirFiles = dir->GetFiles();
	for ( TFiles::const_iterator i=dirFiles.Begin(); i!=dirFiles.End(); ++i )
	{
		files.PushBack( *i );
	}

	for( CDirectory * pDir : dir->GetDirectories() )
	{
		GetDirectoryFiles( pDir, files );
	}
}

extern Bool GSilentFeedback;
extern Bool GDoNotCreateRenderResources;

void CEdFrame::OnResaveTextures( wxCommandEvent& event )
{
	// Get all files
	TDynArray< CDiskFile* > allFiles;
	GetDirectoryFiles( GDepot, allFiles );

	// Collect all textures
	TDynArray< CDiskFile* > textureFiles;
	for ( Uint32 i=0; i<allFiles.Size(); i++ )
	{
		CDiskFile* file = allFiles[i];
		CFilePath filePath( file->GetFileName() );
		if ( filePath.GetExtension().EqualsNC( ResourceExtension< CBitmapTexture >() ) )
		{
			if ( !file->IsLocal() && file->IsCheckedOut() && !file->LoadThumbnail() )
			{
				textureFiles.PushBack( file );
			}
		}
	}

	// Start task
	GFeedback->BeginTask( TXT("Resaving textures..."), true );

	// Do not disturb
	GSilentFeedback = true;
	GDoNotCreateRenderResources = true;

	// Begin resave
	TDynArray< CDiskFile* > invalidFiles;
	Int32 savedFiles = 0;	
	CTimeCounter resaveTimeCounter;
	for ( Uint32 i=0; i<textureFiles.Size(); i++ )
	{
		CDiskFile* file = textureFiles[i];

		// Cancel
		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		// Update
		GFeedback->UpdateTaskInfo( TXT("Resaving '%s'... (%i/%i)"), file->GetFileName().AsChar(), i+1, textureFiles.Size() );
		GFeedback->UpdateTaskProgress( i, textureFiles.Size() );

		// Checkout
		/*if ( !GVersionControl->SilentCheckOut( *file ) )
		{
			WARN_EDITOR( TXT("Unable to checkout '%s' !!!"), file->GetDepotPath().AsChar() );
			invalidFiles.PushBack( file );
			continue;
		}*/

		// Load and save :)
		if ( file->Load() )
		{
			// Update thumbnail
			file->UpdateThumbnail();

			// Save the file
			if ( !file->Save() )
			{
				invalidFiles.PushBack( file );
				continue;
			}

			// Unload and continue
			file->Unload();
			savedFiles++;
		}

		// Collect garbage
		if ( savedFiles % 10 == 0 )
		{
			SGarbageCollector::GetInstance().CollectNow();
		}
	}

	// Do not disturb
	GDoNotCreateRenderResources = false;
	GSilentFeedback = false;

	// End task
	GFeedback->EndTask();

	// Print fail list
	LOG_EDITOR( TXT("======================================") );
	LOG_EDITOR( TXT("Fail List:") );
	
	// Show stats
	String msg = String::Printf( TXT("%i of %i files were saved in %1.0fs.\n"), savedFiles, textureFiles.Size(), resaveTimeCounter.GetTimePeriod() );
	if ( invalidFiles.Size() )
	{
		msg += String::Printf( TXT("There were error resaving following %i files:\n"), invalidFiles.Size() );
		for ( Uint32 i=0; i<invalidFiles.Size(); i++ )
		{
			CDiskFile* file = invalidFiles[i];
			msg += file->GetDepotPath();
			msg += TXT("\n");

			LOG_EDITOR( TXT("%i: %s"), i, file->GetDepotPath().AsChar() );
		}
	}

	LOG_EDITOR( TXT("======================================") );

	// Show message
	wxMessageBox( msg.AsChar(), wxT("Texture resave"), wxOK | wxICON_INFORMATION );
}

void CEdFrame::OnAttitudeEditor( wxCommandEvent& event )
{
	CEdAttitudeEditor *editor = new CEdAttitudeEditor( this );
	editor->Show();
	editor->Raise(); // bring to front
}

void CEdFrame::OnLootEditor( wxCommandEvent& event )
{
	CEdLootEditor *editor = new CEdLootEditor( this );
	editor->Show();
	editor->Raise(); // bring to front
}

void CEdFrame::OnConfig( wxCommandEvent& event )
{
	CEdConfig *editor = new CEdConfig( this );
	editor->Show();
	editor->Raise(); 
}

void CEdFrame::OnCrowdDebugger( wxCommandEvent& event )
{
	//CEdCrowdDebugger *editor = new CEdCrowdDebugger( this );
	//editor->Show();
	//editor->Raise(); 
}

void CEdFrame::OnShowCookingDialog( wxCommandEvent& event )
{
	CEdPreviewCooker::ShowFrame();
}

void CEdFrame::OnImportTextureSourceData( wxCommandEvent& event )
{
	CEdImportTextureSourceData *dialog = new CEdImportTextureSourceData( this );
	dialog->Show();
	dialog->Raise();
}

void CEdFrame::OnCreateEditorAnimCache( wxCommandEvent& event )
{
	CEdAnimCacheGenerator* gen = new CEdAnimCacheGenerator( NULL );
	gen->Show();
	gen->Raise();
}

void CEdFrame::OnSqlConnectionButton( wxCommandEvent& event )
{
	if( SLocalizationManager::GetInstance().IsConnected() )
	{
		SLocalizationManager::GetInstance().Reconnect();
	}
	else
	{
		// We override neverconnect settings - user explicitly says he wants to connect
		/*if( SLocalizationManager::GetInstance().GetNeverConnect() )
		{
			SLocalizationManager::GetInstance().SetNeverConnect( false );
		}*/

		if( SLocalizationManager::GetInstance().OpenSomeDataAccess( true, true ) )
		{
			String selectedLang = m_languageChoice->GetStringSelection().wc_str();

			FillLanguagesChoiceBox();

			m_languageChoice->Select( m_languageChoice->FindString( selectedLang.AsChar() ) );

			// Nothing should be modified at this point
			SLocalizationManager::GetInstance().SetCurrentLocaleForce( selectedLang );

		}
	}

	m_mainToolbar->ToggleTool( XRCID("SqlConnection"), SLocalizationManager::GetInstance().IsConnected() );


}

void CEdFrame::OnChangeLanguage( wxCommandEvent& event )
{
	if( SLocalizationManager::GetInstance().IsAnyContentModified() )
	{
		// locale will not be changed but language choice is already showing new locale - fix this
		const String& currLocale = SLocalizationManager::GetInstance().GetCurrentLocale();
		m_languageChoice->SetSelection( m_languageChoice->FindString( currLocale.AsChar() ) );

		GFeedback->ShowMsg( TXT( "Locale not changed" ), TXT( "Locale was not changed as some localized content is modified. Please save it before changing locale." ) );
	}
	else
	{
		SLocalizationManager::GetInstance().SetCurrentLocale( event.GetString().wc_str() );
		SEvents::GetInstance().QueueEvent( CNAME( CurrentLocaleChanged ), NULL ); 
	}
}

void CEdFrame::ForceNoAutohide( Bool forceNoAutohide, Bool updateToolbarBtn /* = false */ )
{
	CDrawableComponent::SetForceNoAutohideDebug( forceNoAutohide );

	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		TDynArray< CDrawableComponent* > components;
		world->GetAttachedComponentsOfClass( components );

		for( Uint32 i=0; i<components.Size(); ++i )
		{
			components[i]->RefreshRenderProxies();
		}
	}

	m_mainToolbar->ToggleTool( XRCID( "autohideToggle" ), forceNoAutohide );
}

void CEdFrame::ConnectToStringDb()
{
	OnSqlConnectionButton( wxCommandEvent() );
}

void CEdFrame::OnMuteSound( wxCommandEvent& event )
{
	if( m_mainToolbar->GetToolState( XRCID( "muteSound" ) ) )
	{
		GSoundSystem->SoundEvent( "mute_sounds" );
		m_mainToolbar->ToggleTool( XRCID( "muteSound" ), true );
	}
	else
	{
		GSoundSystem->SoundEvent( "unmute_sounds" );
		m_mainToolbar->ToggleTool( XRCID( "muteSound" ), false );
	}
}

void CEdFrame::OnMuteMusic( wxCommandEvent& event )
{
	if( m_mainToolbar->GetToolState( XRCID( "muteMusic" ) ) )
	{
		GSoundSystem->SoundEvent( "mute_music" );
		m_mainToolbar->ToggleTool( XRCID( "muteMusic" ), true );
	}
	else
	{
		GSoundSystem->SoundEvent( "unmute_music" );
		m_mainToolbar->ToggleTool( XRCID( "muteMusic" ), false );
	}
}

void CEdFrame::OnCameraMoved( const Vector& cameraPosition, const EulerAngles& cameraRotation )
{
}

void CEdFrame::OnRefreshVoices( wxCommandEvent& event )
{
	CEdRefreshVoicesDlg* rvoDlg = new CEdRefreshVoicesDlg( wxTheFrame );
	if ( rvoDlg )
	{
		rvoDlg->Execute();
	}
}

void CEdFrame::OnMapPinValidator( wxCommandEvent& event )
{
	if ( !m_mapPinValidator )
	{
		// Create map pin validator
		m_mapPinValidator = new CEdMapPinValidatorWindow( NULL );
	}

	if ( m_mapPinValidator )
	{
		m_mapPinValidator->Show();
		m_mapPinValidator->SetFocus();
	}
}

void CEdFrame::OnSelectGameResource( wxCommandEvent& event )
{
	if ( GGame->IsActive() == true )
	{
		return;
	}

	String gameResourcePath = String::EMPTY;
	if ( GetActiveResource( gameResourcePath, ClassID< CGameResource >() ) == true )
	{
		GGame->SetupGameResourceFromFile( gameResourcePath );
		UpdateGameResourceNameField();
	}

}

void CEdFrame::OnPlayGameFromGameResource( wxCommandEvent& event )
{
	const Bool okToStartGame = CheckStartGameConditions();
	if( !okToStartGame )
	{
		return;
	}

	CGameResource* gameResource = GGame->GetGameResource();
	Bool runTheGame = true;

	// Find the world file
	CDiskFile* worldFile = GDepot->FindFile( gameResource->GetStartWorldPath() );
	if ( !worldFile )
	{
		String msg = String::Printf( TXT("Cannot find world file '%s'"), gameResource->GetStartWorldPath().AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Error"), wxOK | wxICON_ERROR | wxCENTRE );
		return;
	}

	if( GGame->IsActive() )
	{
		SGameSessionManager::GetInstance().EndSession();
	}

	// Load the world if it isn't loaded or another world is loaded
	if ( !worldFile->IsLoaded() || worldFile->GetResource() != GGame->GetActiveWorld() )
	{
		runTheGame = LoadWorld( gameResource->GetStartWorldPath() );
	}

	// Run the game
	if ( runTheGame )
	{
		const String& loadingVideoToPlay = gameResource->GetNewGameLoadingVideo();
		const TDynArray< CName >& playGoChunksToActivate = gameResource->GetPlayGoChunksToActivate();
		StartGame( false, String::EMPTY, loadingVideoToPlay, playGoChunksToActivate );
	}
}

void CEdFrame::UpdateGameResourceNameField()
{
	// current game resource
	CGameResource* gameResource = GGame->GetGameResource();
	String currnetResPath = String::EMPTY;
	if( gameResource != nullptr ) currnetResPath = gameResource->GetDepotPath();
	Bool gameDefFoundInRecentList = false;

	// config stored recent game resources
	String recentGameResourcesPaths = String::EMPTY;
	SUserConfigurationManager::GetInstance().ReadParam( TXT("Gameplay"), TXT("GameResources"), TXT("RecentGameResourcesPaths"), recentGameResourcesPaths );
	
	const Uint32 limitRecentEntries = 10;
	TDynArray< String >	recentGameResourcesFromConfig;
	if( !recentGameResourcesPaths.Empty() ) recentGameResourcesFromConfig = recentGameResourcesPaths.Split( TXT(";") );

	// limit the most recent list to some magic number
	if( recentGameResourcesFromConfig.Size() > limitRecentEntries )
	{		
		recentGameResourcesFromConfig.Erase( recentGameResourcesFromConfig.Begin(), recentGameResourcesFromConfig.Begin()+ (recentGameResourcesFromConfig.Size() - limitRecentEntries) );
	}

	wxChoice* gameResourceNameField = XRCCTRL( *this, "gameDefinitionNameText", wxChoice );		
	gameResourceNameField->Clear();	

	if( !currnetResPath.Empty() ) 
	{		
		// initialize recent list in config
		Bool addNewEntry = true;
		for( Uint32 i=0; i<recentGameResourcesFromConfig.Size(); ++i )
		{
			if( !recentGameResourcesFromConfig[i].Empty() && currnetResPath == recentGameResourcesFromConfig[i] )
			{
				addNewEntry = false;
				break;			
			}
		}

		// save recent list in config
		if( addNewEntry )
		{			
			recentGameResourcesPaths += currnetResPath;
			recentGameResourcesPaths += TXT(";");
			SUserConfigurationManager::GetInstance().WriteParam( TXT("Gameplay"), TXT("GameResources"), TXT("RecentGameResourcesPaths"), recentGameResourcesPaths );
			SUserConfigurationManager::GetInstance().SaveAll();
		}


		// initialize wxChoice
		 recentGameResourcesFromConfig = recentGameResourcesPaths.Split( TXT(";") );
		for( Uint32 i=0; i<recentGameResourcesFromConfig.Size(); ++i )
		{			
			TDynArray< String >	temp = recentGameResourcesFromConfig[i].Split( TXT("\\") );
			if( !temp.Empty() ) 
			{
				gameResourceNameField->Append( temp[ temp.Size()-1 ].AsChar() );

				// set the current game def selection
				if( currnetResPath == recentGameResourcesFromConfig[i] )
				{
					gameResourceNameField->SetSelection( i );
					gameDefFoundInRecentList = true;
				}
			}
		}

		if( !gameDefFoundInRecentList ) gameResourceNameField->SetSelection( gameResourceNameField->GetCount()-1 );

	}
	else
	{
		Bool recentEntryFound = false;

		// initialize wxChoice
		for( Uint32 i=0; i<recentGameResourcesFromConfig.Size(); ++i )
		{			
			TDynArray< String >	temp = recentGameResourcesFromConfig[i].Split( TXT("\\") );
			if( !temp.Empty() ) 
			{
				gameResourceNameField->Append( temp[ temp.Size()-1 ].AsChar() );
				recentEntryFound = true;
			}
		}	

		if( recentEntryFound ) gameResourceNameField->SetSelection( gameResourceNameField->GetCount()-1 );
	}
}

void CEdFrame::OnGameDefinitionChoice( wxCommandEvent& event )
{
	wxChoice* gameResourceNameField = XRCCTRL( *this, "gameDefinitionNameText", wxChoice );
	
	Int32 currentSelection = gameResourceNameField->GetCurrentSelection();

	// config stored recent game resources
	String recentGameResourcesPaths = String::EMPTY;
	SUserConfigurationManager::GetInstance().ReadParam( TXT("Gameplay"), TXT("GameResources"), TXT("RecentGameResourcesPaths"), recentGameResourcesPaths );
	
	TDynArray< String >	recentGameResourcesFromConfig;
	if( !recentGameResourcesPaths.Empty() ) recentGameResourcesFromConfig = recentGameResourcesPaths.Split( TXT(";") );

	if( currentSelection < recentGameResourcesFromConfig.SizeInt() && recentGameResourcesFromConfig[ currentSelection ] != String::EMPTY )
	{
		GGame->SetupGameResourceFromFile( recentGameResourcesFromConfig[ currentSelection ] );
	}	
}

void CEdFrame::PathLibEnableGeneration( wxCommandEvent& event )
{
	Bool enable = event.IsChecked();
	CPathLibWorld::EnableTaskManager( enable );
	if ( enable )
	{
		wxToolBar* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);
		CPathLibWorld::ForceLocalNavdataFolder( true );
		toolbar->ToggleTool( XRCID("pathlibLocalFolder"), true );
	}
}

void CEdFrame::PathLibEnableObstacles( wxCommandEvent& event )
{
	CPathLibWorld::EnableObstacleGeneration( event.IsChecked() );
}

void CEdFrame::PathLibLocalFolder( wxCommandEvent& event )
{
	CPathLibWorld::ForceLocalNavdataFolder( event.IsChecked() );
}

void CEdFrame::OnAutoScriptWindow( wxCommandEvent& event )
{
	const Bool state = event.IsChecked();
	if ( state )
	{
		m_autoScriptWin = new CEdAutoScriptWindow( this );
		m_autoScriptWin->Show();
		m_autoScriptWin->GetOriginalFrame()->Bind( wxEVT_CLOSE_WINDOW, [this]( wxCloseEvent& evt ){ 
			m_mainToolbar->ToggleTool( XRCID( "AutoScript" ), false ); 
			evt.Skip();
		} );
		m_autoScriptWin->SetFocus();
	}
	else if ( m_autoScriptWin )
	{
		m_autoScriptWin->Destroy();
		m_autoScriptWin = nullptr;
	}
}

void CEdFrame::UpdateExplorationFinderIcons()
{
#ifndef NO_EXPLORATION_FINDER
	wxToolBar* toolbar = XRCCTRL(*this, "ToolBar", wxToolBar);

	Bool active = SExplorationFinder::GetInstance().IsActive();
	toolbar->ToggleTool( XRCID("explorationFinder"), active );
	toolbar->ToggleTool( XRCID("explorationFinderOneLayer"), active );
#endif
}

void CEdFrame::ExplorationFinder( wxCommandEvent& event )
{
#ifndef NO_EXPLORATION_FINDER
	SExplorationFinder::GetInstance().Hide( false );

	GGame->GetViewport()->SetRenderingMask( SHOW_Exploration );

	SExplorationFinder::GetInstance().FindExplorations( ! SExplorationFinder::GetInstance().IsActive() );

	UpdateExplorationFinderIcons();
#endif
}

void CEdFrame::ExplorationFinderOneLayer( wxCommandEvent& event )
{
#ifndef NO_EXPLORATION_FINDER
	SExplorationFinder::GetInstance().Hide( false );

	GGame->GetViewport()->SetRenderingMask( SHOW_Exploration );

	if ( m_sceneExplorer && m_sceneExplorer->GetActiveLayer() )
	{
		SExplorationFinder::GetInstance().FindExplorations( ! SExplorationFinder::GetInstance().IsActive(), Cast<CLayer>( m_sceneExplorer->GetActiveLayer() ) );
	}

	UpdateExplorationFinderIcons();
#endif
}

void CEdFrame::ExplorationFinderJumpTo( Bool next )
{
#ifndef NO_EXPLORATION_FINDER
	SExplorationFinder::GetInstance().Hide( false );
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		GGame->GetViewport()->SetRenderingMask( SHOW_Exploration );
		CEntity* selectedEntity = nullptr;
		TDynArray< CEntity* > selectedEntities;
		m_viewport->GetSelectionManager()->GetSelectedEntities( selectedEntities );
		if ( selectedEntities.Size() )
		{
			selectedEntity = selectedEntities[0];
		}
		m_viewport->GetSelectionManager()->DeselectAll();
		Bool includeIgnored = wxGetKeyState( WXK_ALT );
		if( CEntity* toSelect = next ? CExplorationFinder::FindNext( world, selectedEntity, includeIgnored ) : CExplorationFinder::FindPrev( world, selectedEntity, includeIgnored ) )
		{
			m_viewport->GetSelectionManager()->Select( toSelect );
			wxTheFrame->GetWorldEditPanel()->LookAtNode( toSelect, 2.0f, Vector::ZEROS );						
		}
	}
#endif
}

void CEdFrame::ExplorationFinderPrev( wxCommandEvent& event )
{
#ifndef NO_EXPLORATION_FINDER
	ExplorationFinderJumpTo( false );
#endif
}

void CEdFrame::ExplorationFinderNext( wxCommandEvent& event )
{
#ifndef NO_EXPLORATION_FINDER
	ExplorationFinderJumpTo( true );
#endif
}

void CEdFrame::ExplorationFinderToggleSelected( wxCommandEvent& event )
{
#ifndef NO_EXPLORATION_FINDER
	SExplorationFinder::GetInstance().Hide( false );
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		CEntity* selectedEntity = nullptr;
		TDynArray< CEntity* > selectedEntities;
		m_viewport->GetSelectionManager()->GetSelectedEntities( selectedEntities );
		Bool toggleTo = false;
		for ( auto iEntity = selectedEntities.Begin(); iEntity != selectedEntities.End(); ++ iEntity )
		{
			if ( CExplorationFinder::IsFoundExploration( *iEntity, &toggleTo ) )
			{
				toggleTo = ! toggleTo;
				break;
			}
		}
		for ( auto iEntity = selectedEntities.Begin(); iEntity != selectedEntities.End(); ++ iEntity )
		{
			CExplorationFinder::ToggleIgnoredTo( *iEntity, toggleTo );
		}
	}
#endif
}

void CEdFrame::ExplorationFinderHide( wxCommandEvent& event )
{
	if ( wxGetKeyState( WXK_ALT ) )
	{
		SExplorationFinder::GetInstance().RemoveAll();
	}
	else
	{
		SExplorationFinder::GetInstance().Hide( ! SExplorationFinder::GetInstance().IsHidden() );
	}
}

void CEdFrame::SwitchOcclusionCullingUsage( wxCommandEvent& event )
{
#ifdef USE_UMBRA
	CUmbraScene::UseOcclusionCulling( event.IsChecked() );
#endif // USE_UMBRA
}

void CEdFrame::OnAutohideToggle( wxCommandEvent& event )
{
	Bool forceNoAutohide = event.IsChecked();
	ForceNoAutohide( forceNoAutohide );
	if ( m_worldSceneDebuggerWindow )
	{
		m_worldSceneDebuggerWindow->UpdateForceNoAutohideButtonLabel( forceNoAutohide );
	}
}

void CEdFrame::OnRewardsEditor( wxCommandEvent& event )
{
#ifdef REWARD_EDITOR
//	if( !m_rewardsEditor )
//	{
		m_rewardsEditor = new CEdRewardEditor( NULL );
// 	}
// 
// 	if( m_rewardsEditor )
// 	{
		m_rewardsEditor->Show();
		m_rewardsEditor->SetFocus();
//	}
#else
	wxMessageBox( wxT("Rewards are stored in xml file"), wxT("Rewards in xml"), wxCENTRE|wxOK|wxICON_ERROR, this );
#endif

}

void CEdFrame::OnJournalEditor( wxCommandEvent& event )
{
	OpenJournalEditor();
}

void CEdFrame::OnWorldEnvironmentEditor( wxCommandEvent& event )
{
	OpenWorldEnvironmentEditor();
}

void CEdFrame::OnMinimapGenerator( wxCommandEvent& event )
{
	// Get the m_world
	CWorld* world = GGame->GetActiveWorld();
	if( world == nullptr )
	{
		wxMessageBox( wxT("Please load any world."), wxT("Lack of world"), wxCENTRE|wxOK|wxICON_ERROR, this );
		return;
	}

	// Get clip map
	CClipMap* terrain = world->GetTerrain();
	if( terrain == nullptr )
	{
		wxMessageBox( wxT("World doesn't have terrain."), wxT("Lack of terrain"), wxCENTRE|wxOK|wxICON_ERROR, this );
		return;
	}

	// show the generator dialog (contains the rest of the logic in CEdMinimapGenerator::Generate)
	CMinimapGeneratorTool* minimapGeneratorTool = new CMinimapGeneratorTool();
	minimapGeneratorTool->ShowModal();
	minimapGeneratorTool->Destroy();
}

void CEdFrame::OnScreenshotEditor( wxCommandEvent& event )
{
	CEdScreenshotEditor* screenshotEditor = m_viewport->GetScreenshotEditor();
	if ( !screenshotEditor )
	{
		screenshotEditor = new CEdScreenshotEditor( this );
		m_viewport->SetScreenshotEditor( screenshotEditor );
	}
	screenshotEditor->Show();
	screenshotEditor->Raise();
}

void CEdFrame::OnExportAnimationList( wxCommandEvent& event )
{
	CEdAnimationListExporter exporter( this );
	exporter.ExportToCSV();
}

void CEdFrame::OpenJournalEditor( THandle< CJournalPath > path )
{
	if( !m_journalEditor )
	{
		m_journalEditor = new CEdJournalEditor( this, path );
		m_journalEditor->Bind( wxEVT_CLOSE_PANEL, &CEdFrame::OnJournalEditorClosed, this );
	}

	m_journalEditor->Show();
	m_journalEditor->SetFocus();
}

void CEdFrame::OnCharacterDBEditorClosed( wxEvent& event )
{
	m_characterDBeditor = NULL;
	event.Skip();
}

void CEdFrame::OpenCharacterDBEditor( CCharacterResource* characterResource )
{
	if( !m_characterDBeditor )
	{
		m_characterDBeditor = new CEdCharacterDBEditor( this, characterResource );
		m_characterDBeditor->Bind( wxEVT_CLOSE_PANEL, &CEdFrame::OnCharacterDBEditorClosed, this );
	}

	m_characterDBeditor->Show();
	m_characterDBeditor->SetFocus();
}

void CEdFrame::OpenWorldEnvironmentEditor()
{
	if ( !m_worldEnvironmentEditor )
	{
		m_worldEnvironmentEditor = new CEdWorldEnvironmentTool();
	}

	m_worldEnvironmentEditor->Show( GetWorldEditPanel() );
	//dex++
	m_worldEnvironmentEditor->SetFocus();
	//dex--
}

void CEdFrame::EditEnvironmentParams( CObject* e )
{
	// Open the world environment editor if needed
	if ( !m_worldEnvironmentEditor )
	{
		OpenWorldEnvironmentEditor();
	}

	// Asked to edit an entity's environment params, assume there is a
	// CAreaEnvironmentComponent in there - find it and edit it
	if ( e && e->IsA< CEntity >() )
	{
		CEntity* ent = static_cast<CEntity*>( e );
		const TDynArray<CComponent*>& components = ent->GetComponents();
		for( auto it=components.Begin(); it != components.End(); ++it )
		{
			CAreaEnvironmentComponent* eArea = Cast< CAreaEnvironmentComponent > ( *it );
			// Found the area environment component, edit it and go away
			if ( eArea ) 
			{
				m_worldEnvironmentEditor->ShowAndEdit( eArea );
				break;
			}
		}
	}
	else // Pass the object to the world environment editor and let it handle the object
	{
		m_worldEnvironmentEditor->ShowAndEdit( e );
	}
}

void CEdFrame::PauseConfigTimer()
{
	m_timer->Pause();
}

void CEdFrame::ResumeConfigTimer()
{
	m_timer->Unpause();
}

void CEdFrame::ShowDataErrorReporter()
{
#ifndef NO_DATA_VALIDATION
	m_dataErrorReporterWindow->ShowWindow();
#endif
}

CEdDataErrorReporterWindow* CEdFrame::GetDataErrorReporter() 
{
#ifndef NO_DATA_VALIDATION
	return m_dataErrorReporterWindow;
#else
	return nullptr;
#endif
}

CEdUndoHistoryFrame* CEdFrame::GetUndoHistoryFrame()
{
	// Create the undo history frame, if necessary
	if ( m_undoHistoryFrame == nullptr )
	{
		m_undoHistoryFrame = new CEdUndoHistoryFrame( this );
		m_undoHistoryFrame->SetUndoManager( m_undoManager );
	}

	return m_undoHistoryFrame;
}

void CEdFrame::SetUndoHistoryFrameManager( CEdUndoManager* undoManager, const String& title )
{
	// Set the undo manager for the undo history frame only if the frame is
	// already created
	if ( m_undoHistoryFrame )
	{
		m_undoHistoryFrame->SetTitle( title.Empty() ? TXT("Undo History") : ( TXT("Undo History [") + title + L"]" ).AsChar() );
		m_undoHistoryFrame->SetUndoManager( undoManager );
	}
}

void CEdFrame::ShowUndoHistoryFrame()
{
	// Show or bring to front the undo history
	GetUndoHistoryFrame()->RaiseAndFocus();
}

void CEdFrame::OnJournalEditorClosed( wxEvent& event )
{
	m_journalEditor = NULL;
	event.Skip();
}

void CEdFrame::FillLanguagesChoiceBox()
{
	m_languageChoice->Clear();

	TDynArray< String > textLanguages, speechLanguages;
	SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );

	for ( TDynArray< String >::const_iterator langIter = textLanguages.Begin(); 
		langIter != textLanguages.End(); ++langIter )
	{
		m_languageChoice->Append( langIter->AsChar() );
	}
}

void CEdFrame::OnDirectoryChange( const TDynArray< ChangedFileData >& changes )
{
	if ( YesNo( TXT("External depot change detected. You should restart to avoid data loss. Restart the editor?") ) )
	{
		RelaunchEditor();
	}
}

void CEdFrame::OnResaveAnimationsTool( wxCommandEvent& event )
{
	wxMessageBox( wxT("Animations resave tool was removed, it was part of the Havok legacy and was not working - ask Dex if you want it back for some reason"), wxT("It's gone now"), wxOK );
}

void CEdFrame::OnIDGenerator( wxCommandEvent& event )
{
	//CEdInteractiveDialogEditor::GenerateAllAudioAndLipsyncFiles();
	event.Skip();
}

void CEdFrame::GetDebugFlags( TDynArray< EShowFlags > &debugFlags )
{
	extern EShowFlags GShowGameFilter[];
	extern EShowFlags GShowRenderFilter[];
	extern EShowFlags GShowPostProcessFilter[];

	if ( !debugFlags.Empty() )
	{
		debugFlags.Clear();
	}

	// collect all flags
	for ( Uint32 i=0; GShowGameFilter[i] != SHOW_MAX_INDEX; ++i )
	{
		EShowFlags flag = GShowGameFilter[i];
		debugFlags.PushBack( flag );
	}

	// remove flags which are not debug
	for ( Uint32 i=0; GShowRenderFilter[i] != SHOW_MAX_INDEX; ++i )
	{
		EShowFlags flag = GShowRenderFilter[i];
		debugFlags.Remove( flag );
	}
	for ( Uint32 i=0; GShowPostProcessFilter[i] != SHOW_MAX_INDEX; ++i )
	{
		EShowFlags flag = GShowPostProcessFilter[i];
		debugFlags.Remove( flag );
	}
	debugFlags.Remove( SHOW_GUI );
}

void CEdFrame::ClearDebugStuff( wxCommandEvent& event )
{
	TDynArray< EShowFlags > debugFlags;
	GetDebugFlags( debugFlags );

	// change state debug flags
	IViewport* gameViewport = GGame->GetViewport();
	static TDynArray< EShowFlags >	activeFlags;

	if( event.GetInt() == 0 )
	{
		const Uint32 activedFlagCount = activeFlags.Size();
		for( Uint32 i=0; i<activedFlagCount; ++i )
		{
			EShowFlags flag = activeFlags[i];
			gameViewport->SetRenderingMask( flag );
		}
	}
	else
	{
		activeFlags.ClearFast();

		const Uint32 debugFlagCount = debugFlags.Size();
		for( Uint32 i=0; i<debugFlagCount; ++i )
		{
			EShowFlags flag = debugFlags[i];
			
			if( gameViewport->GetRenderingMask()[ flag ] == true )
			{
				activeFlags.PushBack( flag );
			}

			gameViewport->ClearRenderingMask( flag );
		}
	}	
}

void CEdFrame::OnForceAllShadowsToggle( wxCommandEvent& event )
{
	/*IViewport* gameViewport = GGame->GetViewport();
	if( event.GetInt() != 0 )
	{
	gameViewport->SetRenderingMask( SHOW_ForceAllShadows );
	}
	else
	{
	gameViewport->ClearRenderingMask( SHOW_ForceAllShadows );
	}	*/
	EViewportFlagsType flagsType = GGame && GGame->IsActive() ? VFT_GAME : VFT_EDITOR;
	static Bool prevMergedMeshes = false;

	if ( event.GetInt() != 0 )
	{
		prevMergedMeshes = false;

		const EShowFlags* flags = GetFilterPanel()->GetViewportFlags( flagsType );
		while ( (*flags) != SHOW_MAX_INDEX )
		{
			if ( (*flags) == SHOW_MergedMeshes )
			{
				prevMergedMeshes = true;
				break;
			}
			++flags;
		}

		GetFilterPanel()->SetViewportFlag( flagsType, SHOW_ForceAllShadows, true );
		GetFilterPanel()->SetViewportFlag( flagsType, SHOW_MergedMeshes, false );
	}
	else
	{
		GetFilterPanel()->SetViewportFlag( flagsType, SHOW_ForceAllShadows, false );
		GetFilterPanel()->SetViewportFlag( flagsType, SHOW_MergedMeshes, prevMergedMeshes );
	}

}

void CEdFrame::DebugFlagsContextMenu( wxCommandEvent& event )
{
	if( event.GetId() == XRCID( "clearDebugStuff" ) ) 
	{
		extern EShowFlags GShowGameFilter[];
		extern EShowFlags GShowRenderFilter[];
		extern EShowFlags GShowPostProcessFilter[];

		TDynArray< EShowFlags >	debugFlags;

		// collect all flags
		for ( Uint32 i=0; GShowGameFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = GShowGameFilter[i];
			debugFlags.PushBack( flag );
		}

		// remove flags which are not debug
		for ( Uint32 i=0; GShowRenderFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = GShowRenderFilter[i];
			debugFlags.Remove( flag );
		}
		for ( Uint32 i=0; GShowPostProcessFilter[i] != SHOW_MAX_INDEX; ++i )
		{
			EShowFlags flag = GShowPostProcessFilter[i];
			debugFlags.Remove( flag );
		}
		debugFlags.Remove( SHOW_GUI );

		wxMenu* debugFlagsMenu = new wxMenu();

		// sort
		struct LocalSorter
		{
			static Bool Sort( const EShowFlags& p1, const EShowFlags& p2 )
			{
				String text1 = CEnum::ToString< EShowFlags >( p1 );
				String text2 = CEnum::ToString< EShowFlags >( p2 );
				return Red::System::StringCompareNoCase( text1.AsChar(), text2.AsChar() ) < 0;
			}
		};
		Sort( debugFlags.Begin(), debugFlags.End(), LocalSorter::Sort );

		const Uint32 debugFlagCount = debugFlags.Size();
		for( Uint32 i=0; i<debugFlagCount; ++i )
		{
			EShowFlags flag = debugFlags[i];
			String GPrefix( TXT("SHOW_") );
			String text = CEnum::ToString< EShowFlags >( flag );
			text = text.RightString( text.GetLength() - GPrefix.GetLength() );
			IViewport* gameViewport = GGame->GetViewport();
			debugFlagsMenu->AppendCheckItem( flag, text.AsChar() )->Check( gameViewport->GetRenderingMask()[ flag ] );
			debugFlagsMenu->Connect( flag, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdFrame::ChangeDebugFlag ) );
		}	

		// show popu menu
		wxWindowBase::PopupMenu( debugFlagsMenu );
	}
}

void CEdFrame::ChangeDebugFlag( wxCommandEvent& event )
{
	Int32 id = event.GetId();
	EShowFlags flag = static_cast< EShowFlags >( id );
	Bool state = ( event.GetInt() == 0 ) ? false : true;
	IViewport* gameViewport = GGame->GetViewport();
	if( gameViewport != nullptr )
	{
		if( state == true )
		{
			gameViewport->SetRenderingMask( flag );
		}
		else
		{
			gameViewport->ClearRenderingMask( flag );
		}
	}
}

void CEdFrame::OnProfilerChange( wxCommandEvent& event )
{
	wxToggleButton* toggle = static_cast<wxToggleButton*>( event.GetEventObject() );

	if( toggle->GetValue () )
	{
		PROFILER_Start();
		toggle->SetForegroundColour( wxColour( 255, 0, 0 ) );
		toggle->SetToolTip( TXT("Stop profiling session") );
	}
	else
	{
		String sessionId;
		unsigned long long sessionQpc = 0;
		unsigned long long sessionQpf = 0;

#if !defined( NO_TELEMETRY )
		IRedTelemetryServiceInterface* interfaceService = SRedTelemetryServicesManager::GetInstance().GetService( TXT("telemetry") );
		if( interfaceService != NULL)
		{
			sessionId = interfaceService->GetSessionId(Telemetry::BT_RED_TEL_API);
			double time;
			unsigned long long qpf, qpc;
			interfaceService->GetTime( time, qpf, qpc );
			sessionQpf = qpf;
			sessionQpc = qpc;
		}
#endif

		ANALYTICS_EVENT_VL( "Tool", "Profiler", "START_REC", 0 );

		PROFILER_Store( TXT(""), sessionId, sessionQpc, sessionQpf );
		toggle->SetForegroundColour( wxColour( 0, 102, 0 ) );
		toggle->SetToolTip( TXT("Start profiling session") );
#ifdef NEW_PROFILER_ENABLED

		String profilerStatusText = String::Printf( TXT( "Profile file save as: %s. Do you want to open?" ),  PROFILER_GetLastStorePath().AsChar() );
		if( wxMessageBox( profilerStatusText.AsChar(), wxT( "Profiler" ), wxYES_NO | wxICON_INFORMATION , this ) == wxYES )
		{
			String command = PROFILER_GetLastStorePath().AsChar();
			command.ReplaceAll( '/', '\\' );
			ShellExecute(0, TXT("open"), TXT("explorer"), command.AsChar(), 0, SW_HIDE);
		}
#endif
	}
}

void CEdFrame::RunEditorInExtraMode( const THashMap< String, String >& commandLine )
{
#if !defined( RED_FINAL_BUILD )
	String game = String::EMPTY;
	if ( commandLine.Find( TXT("testmode"), game ) )
	{
		RunEditorInTestMode( game );
	}
	else
#endif
	if( commandLine.Find( TXT("minimapmode") ) != commandLine.End() )
	{
		RunEditorInMinimapMode( commandLine );
		return;
	}
	else if ( commandLine.Find( TXT("recordermode") ) != commandLine.End() )
	{
		RunEditorInRecorderMode( commandLine );
		return;
	}
	// here you can add more extra modes for editor
}

void CEdFrame::RunEditorInMinimapMode( const THashMap< String, String >& commandLine )
{
	RED_LOG( RED_LOG_CHANNEL( MinimapMode), TXT("Editor starts in minimap generation mode") );

	// disconnect from perforce
	if( GVersionControl != nullptr )
	{
		delete GVersionControl;
		GVersionControl = nullptr;
	}
	GVersionControl = new ISourceControl;

	// turn off feedback system
	GFeedback = &GNullFeedback;

	// run minimap generation mode
	RunLater( [ this, commandLine ]() 
	{
		String levelName = String::EMPTY;
		commandLine.Find( TXT("levelName"), levelName );

		String levelPath;

		if( commandLine.Find( TXT("dlcmode") ) != commandLine.End() )
			levelPath = String::Printf( TXT("levels\\%s\\%s.w2w"), levelName.AsChar(), levelName.AsChar() );
		else
			levelPath = String::Printf( TXT("dlc\\bob\\data\\levels\\bob\\bob.w2w") );

		if( levelName.Empty () == false )
		{
			GSplash->UpdateProgress( TXT("Loading world %s for minimap generator ..."), levelName.AsChar() );
			// load world
			if( LoadWorld( levelPath ) == true )
			{
				m_sceneExplorer->LoadAllLayers();
				RunLater( [ this, levelName, commandLine ]() 
				{
					String outputDir = String::EMPTY;
					if( commandLine.Find( TXT("outputDir"), outputDir ) == false )
					{
						RED_HALT( "Minimap generation mode needs outputDir argument" );
					}

					String imageZoom = String::EMPTY;
					if( commandLine.Find( TXT("imageZoom"), imageZoom ) ==  false)
					{
						RED_HALT( "Minimap generation mode needs imageZoom argument" );
					}
					Uint32 imageZoomValue = 1;
					FromString< Uint32 >( imageZoom, imageZoomValue );

					String envDefPath = String::EMPTY;
					commandLine.Find( TXT("envDef"), envDefPath );

					// image size/resolution
					String resolution = String::EMPTY;
					if( commandLine.Find( TXT("resolution"), resolution ) ==  false)
					{
						RED_HALT( "Minimap generation mode needs resolution argument" );
					}
					Uint32 resolutionValue = 1;
					FromString< Uint32 >( resolution, resolutionValue );

					// directory layout
					String dirLayout = String::EMPTY;
					if( commandLine.Find( TXT("dirLayout"), dirLayout ) ==  false)
					{
						RED_HALT( "Minimap generation mode needs dirLayout argument" );
					}
					EDirLayout dirLayoutValue = DL_Photoshop;
					if( dirLayout == TXT("Default") )
					{
						dirLayoutValue = DL_Default;
					}
					else if( dirLayout == TXT("Photoshop") )
					{
						dirLayoutValue = DL_Photoshop;
					}

					//
					Red::TUniquePtr< CMinimapGenerator > minimapGenerator( new CMinimapGenerator );
					SMinimapSettings settings;

					//
					String maskNames = String::EMPTY;
					commandLine.Find( TXT("masks"), maskNames );
					TDynArray< String > masks = maskNames.Split( TXT(";") );
					for( Uint32 i=0; i<masks.Size(); ++i )
					{
						EMinimapMask mask = MM_Count;
						FromString( masks[i], mask );

						if( mask != MM_Count )
						{
							settings.m_exteriors.m_enabledMasks[mask] = true;
						}
					}

					settings.m_outputDir = outputDir;
					settings.m_exteriors.m_dirLayout = dirLayoutValue;
					settings.m_exteriors.m_imageZoom = imageZoomValue;
					settings.m_exteriors.m_envSettingsPath = envDefPath;
					settings.m_exteriors.m_continueMode = true;
					settings.m_exteriors.m_imageSize = resolutionValue;

					String range = String::EMPTY;
					if( commandLine.Find( TXT("range"), range ) )
					{
						TDynArray<String> rangeValues = range.Split( TXT(";") );

						if( rangeValues.Size() == 4 )
						{
							settings.m_generationMode = GM_TileRange;
							FromString( rangeValues[0], settings.m_tileRange.Min.X );
							FromString( rangeValues[1], settings.m_tileRange.Min.Y );
							FromString( rangeValues[2], settings.m_tileRange.Max.X );
							FromString( rangeValues[3], settings.m_tileRange.Max.Y );
						}
						else
						{
							RED_HALT( "Minimap generation mode has range argument but with invalid format, should be -range=d;d;d;d" );
						}
					}

					minimapGenerator->SetSettings( settings );

					if( commandLine.Find( TXT("generateExterior") ) != commandLine.End() )
					{
						GSplash->UpdateProgress( TXT("Generating exteriors for %s ..."), levelName.AsChar() );
					minimapGenerator->GenerateExteriors();
					}
					else if( commandLine.Find( TXT("generateInterior") ) != commandLine.End() )
					{
						GSplash->UpdateProgress( TXT("Generating interiors for %s ..."), levelName.AsChar() );
						minimapGenerator->GenerateInteriors();
					}

					GEngine->RequestExit();
				} );
			}
		}
		else
		{
			RED_HALT( "Minimap generation mode needs levelName argument" );
		}
	} );
}

void CEdFrame::RunEditorInTestMode( const String& game )
{
#ifndef NO_TEST_FRAMEWORK
	
	// disconnect from perforce (so that no 'what's new' popup will be shown)
	if( GVersionControl != nullptr )
	{
		delete GVersionControl;
		GVersionControl = nullptr;
	}
	GVersionControl = new ISourceControl;

	// turn off feedback system
	GFeedback = &GNullFeedback;

	// feed TestFramework with commandline parameters
	if(! STestFramework::GetInstance().ParseCommandline( this->m_commandLine ) )
	{
		ERR_R4( TXT( "Failed to initialize test framework." ) );
		GEngine->RequestExit();
		return;
	}

	// run test
	RunLater( [ this, game ]() 
	{
		GGame->SetupGameResourceFromFile( game );
		this->OnPlayGameFromGameResource( wxCommandEvent() );
		STestFramework::GetInstance().OnStart( GGame->GetViewport() );
	} );
#endif
}

void CEdFrame::RunEditorInRecorderMode( const THashMap< String, String >& commandLine )
{
	// disconnect from perforce
	if( GVersionControl != nullptr )
	{
		delete GVersionControl;
		GVersionControl = nullptr;
	}
	GVersionControl = new ISourceControl;

	// turn off feedback system
	GFeedback = &GNullFeedback;

	if( !SSceneRecorder::GetInstance().ParseCommandline( commandLine ) )
	{
		ERR_EDITOR( TXT( "Failed to initialize scene recorder." ) );
		GEngine->RequestExit();
		return;
	}

	RunLater( [ this ]()
	{
		SSceneRecorder::GetInstance().OnStart();
	} );
}
//-----------------------------------------------------------------------------