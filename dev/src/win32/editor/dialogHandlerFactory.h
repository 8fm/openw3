#pragma once

#include "dialogEditorHandlerAware.h"
class CEdSceneEditorScreenplayPanel;
class LocalizedString;

class CEdStorySceneHandlerFactory : protected IEdDialogHandlerAware 
{
private:
	THashMap< Int32, IEdDialogEditorHandler* > m_commonHandlers;
	CEdSceneEditorScreenplayPanel* m_dialogEditor;
	
public:
	IEdDialogEditorHandler* CreateSectionTagEditorHandler( IEdDialogHandlerAware* owner );
	IEdDialogEditorHandler* CreateScrollOnFocusHandler();
	IEdDialogEditorHandler* CreateArrowTraverseHandler();
	IEdDialogEditorHandler* CreateCaretOnFocusHandler();
	IEdDialogEditorHandler* CreateHyperlinkHandler();
	IEdDialogEditorHandler* CreateAutoExpandHandler();
	IEdDialogEditorHandler* CreateManualMouseScrollHandler();
	IEdDialogEditorHandler* CreatePseudoButtonHandler();
	IEdDialogEditorHandler* CreateTranslationHelperHandler( LocalizedString* localizedContent, IEdDialogHandlerAware* owner );

	RED_INLINE void SetDialogEditor( CEdSceneEditorScreenplayPanel* newValue ) { m_dialogEditor = newValue; }

protected:
	void RegisterHandler( IEdDialogEditorHandler* handler, IEdDialogHandlerAware* owner );
};
