#include "build.h"
#include "dialogHandlerFactory.h"
#include "editorExternalResources.h"

#include "tagEditorHandler.h"
#include "dialogUtilityHandlers.h"
#include "2daTagListUpdater.h"

#include "dialogEditorPage.h"

enum ECommonHandlersKey
{
	CHK_ArrowTraverse,
	CHK_ScrollOnFocus,
	CHK_AutoComplete,
	CHK_AliasAutoComplete,
	CHK_CaretOnFocus,
	CHK_Hyperlink,
	CHK_AutoExpand,
	CHK_DelegateScroll,
	CHK_WrapText,
	CHK_PseudoButton
};

#define RETRIEVE_COMMON_HANDLER( handlerKey, newHandler )	\
if ( m_commonHandlers[ handlerKey ] == NULL )				\
{															\
	IEdDialogEditorHandler* handler = newHandler;			\
	RegisterHandler( handler, this );						\
	m_commonHandlers[ handlerKey ] = handler;				\
}															\
return m_commonHandlers[ handlerKey ];

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateSectionTagEditorHandler( IEdDialogHandlerAware* owner )
{
	CEdTagEditorHandler* handler = new CEdTagEditorHandler( m_dialogEditor, new CEdConversationTagListUpdater() );
	RegisterHandler( handler, owner );
	return handler;
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateScrollOnFocusHandler()
{
	RETRIEVE_COMMON_HANDLER( CHK_ScrollOnFocus, new CEdDialogScrollOnFocusHandler( m_dialogEditor ) );
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateArrowTraverseHandler()
{
	RETRIEVE_COMMON_HANDLER( CHK_ArrowTraverse, new CEdDialogArrowTraverseHandler() );
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateCaretOnFocusHandler()
{
	if ( m_commonHandlers[ CHK_CaretOnFocus ] == NULL )
	{
		CEdDialogCaretOnFocusHandler* handler = new CEdDialogCaretOnFocusHandler();
		RegisterHandler( handler, this );
		m_commonHandlers[ CHK_CaretOnFocus ] = handler;
	}

	return m_commonHandlers[ CHK_CaretOnFocus ];
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateHyperlinkHandler()
{
	if ( m_commonHandlers[ CHK_Hyperlink ] == NULL )
	{
		CEdDialogHyperlinkHandler* handler = new CEdDialogHyperlinkHandler();
		RegisterHandler( handler, this );
		m_commonHandlers[ CHK_Hyperlink ] = handler;
	}

	return m_commonHandlers[ CHK_Hyperlink ];
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateAutoExpandHandler()
{
	RETRIEVE_COMMON_HANDLER( CHK_AutoExpand, new CEdDialogAutoExpandHandler( m_dialogEditor ) );
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateManualMouseScrollHandler()
{
	RETRIEVE_COMMON_HANDLER( CHK_DelegateScroll, 
		new CEdDialogManualScriptScrollHandler( m_dialogEditor->GetDialogTextPanel() ) );
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreatePseudoButtonHandler()
{
	RETRIEVE_COMMON_HANDLER( CHK_PseudoButton, new CEdPseudoButtonHandler() );
}

void CEdStorySceneHandlerFactory::RegisterHandler( IEdDialogEditorHandler* handler, IEdDialogHandlerAware* owner )
{
	if ( owner == NULL )
	{
		owner = this;
	}
	owner->RegisterDialogHandler( handler );
}

IEdDialogEditorHandler* CEdStorySceneHandlerFactory::CreateTranslationHelperHandler( LocalizedString* localizedContent, IEdDialogHandlerAware* owner )
{
	ASSERT( localizedContent != NULL );

	CEdDialogTranslationHelperHandler* handler = new CEdDialogTranslationHelperHandler( localizedContent );
	RegisterHandler( handler, owner );
	return handler;
}