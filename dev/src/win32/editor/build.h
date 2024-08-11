/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

// Core
#include "..\..\common\core\core.h"
#include "..\..\common\core\version.h"
#include "../../common/core/diskFile.h"

// Engine
#include "..\..\common\engine\engine.h"

// Win32 platform
#include "..\..\win32\platform\win32.h"

// Game
#include "../../common/game/game.h"

#include "..\..\games\r4\r4.h"
#include "..\..\games\r6\r6.h"
#include "..\..\games\r6\r6TypeRegistry.h"
#include "..\..\games\r6\r6game.h"

#define LOG_EDITOR( format, ... ) RED_LOG( Editor, format, ##__VA_ARGS__ )
#define ERR_EDITOR( format, ... ) RED_LOG_ERROR( Editor, format, ##__VA_ARGS__ )
#define WARN_EDITOR( format, ... ) RED_LOG_WARNING( Editor, format, ##__VA_ARGS__ );

RED_DISABLE_WARNING_MSC( 4996 )	// 'function': was declared deprecated

// Editor
#include "editorTypeRegistry.h"

#include "namesRegistry.h"

// Widgets
//TMP: GFx was suppressing this warning and push 'n popping it in the pch doesn't seem to work right (pops warnings that wxWidgets suppresses).
#pragma warning( disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc

#define wxUSE_RICHTEXT 1
#define wxNO_PNG_LIB
#include "wx/wxprec.h"
#include "wx/xrc/xmlres.h"
#include "wx/splitter.h"
#include "wx/aui/aui.h"
#include "wx/dcbuffer.h"
#include "wx/dateevt.h"
#include "wx/evtloop.h"
#include "wx/statusbr.h"
#include "wx/menu.h"
#include "wx/gdicmn.h"
#include "wx/treectrl.h"
#include "wx/toolbar.h"
#include "wx/spinctrl.h"
#include "wx/srchctrl.h"
#include "wx/combobox.h"
#include "wx/combo.h"
#include "wx/treectrl.h"
#include "wx/panel.h"
#include "wx/statline.h"
#include "wx/sizer.h"
#include "wx/gbsizer.h"
#include "wx/splitter.h"
#include "wx/frame.h"
#include "wx/aui/aui.h"
#include "wx/artprov.h"
#include "wx/laywin.h"
#include "wx/bmpcbox.h"
#include "wx/xrc/xmlres.h"
#include "wx/xml/xml.h"
#include "wx/image.h"
#include "wx/collpane.h"
#include "wx/dcbuffer.h"
#include "wx/tglbtn.h"
#include "wx/clipbrd.h"
#include "wx/fileconf.h"
#include "wx/datectrl.h"
#include "wx/timectrl.h"
#include "wx/listbase.h"
#include "wx/grid.h"
#include "wx/popupwin.h"
#include "wx/colordlg.h"
#include "wx/filepicker.h"
#include "wx/numdlg.h"
#include "wx/html/htmlwin.h"
#include "edtimer.h"
#include "choice.h"
#include "textCtrlEx.h"
#include "treeListCtrl.h"
#include "treeListCtrl_XRC.h"
#include "gradientCtrl.h"
#include "gradientCtrl_XRC.h"
#include "dropTarget.h"
#include "wx/clrpicker.h"
#include "wx/wizard.h"
#include "wx/dir.h"
#include "wx/popupwin.h"
#include "wx/stdpaths.h"
#include "wx/snglinst.h"
#include "wx/hyperlink.h"
#include "wx/richtext/richtextctrl.h"
#include "wx/richtext/richtextstyles.h"

// GDI+
#include <GdiPlus.h>

// Editor config
#include "configManager.h"

// Rendering
#include "renderingWindow.h"
#include "renderingPanel.h"
#include "worldEditPanel.h"
#include "previewPanel.h"
#include "interactivePreviewPanel.h"

// Viewport widgets
#include "viewportWidgetBase.h"
#include "viewportWidgetLines.h"
#include "viewportWidgetManager.h"
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetMovePlane.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "viewportWidgetScalePlane.h"
#include "viewportWidgetScaleUniform.h"

// Widgets
#include "textEditor.h"
#include "widgetItemList.h"
#include "smartLayout.h"
#include "draggablePanel.h"
#include "colorPicker.h"
#include "colorPickerAdvanced.h"
#include "canvas.h"
#include "fileDlg.h"
#include "reloadDialog.h"
#include "toggleButton.h"
#include "toggleButtonPanel.h"
#include "draggedSpinButton.h"
#include "spinSliderControl.h"
#include "curveEditorCanvas.h"
#include "curveEditor.h"
#include "graphEditor.h"
#include "gradientEditor.h"
#include "submitDialog.h"
#include "saveDlg.h"
#include "clientDlg.h"
#include "fileHistoryDlg.h"
#include "textDialog.h"
#include "behaviorBoneSelectionDlg.h"
#include "cutsceneEditor.h"
#include "collisionMemUsageTool.h"
#include "massActionDialog.h"

// Property grid
#include "propertiesPage.h"
#include "propertiesBrowserWithStatusbar.h"
#include "propertiesFrame.h"
#include "selectionProperties.h"
#include "environmentProperties.h"

// Editor
#include "editorTool.h"
#include "utils.h"
#include "htmlUtils.h"
#include "clipboardData.h"
#include "objectClipboard.h"
#include "app.h"
#include "output.h"
#include "frame.h"
#include "resources.h"
#include "editorEngine.h"
#include "gameTimeEditor.h"
#include "assetBrowser.h"
#include "helpBubble.h"
#include "stringSelectorTool.h"

// Version control
#include "versionControlWrapper.h"

// properties
#include "selectionList.h"
#include "listBoxEditor.h"
