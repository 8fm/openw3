
#include "build.h"
#include "dialogEditorModificationCtrl.h"
#include "dialogEditor.h"

CEdSceneModCtrl::CEdSceneModCtrl()
	: m_editor( nullptr )
{

}

void CEdSceneModCtrl::Init( const CEdSceneEditor* editor )
{
	SCENE_ASSERT( !m_editor );
	m_editor = editor;
}

