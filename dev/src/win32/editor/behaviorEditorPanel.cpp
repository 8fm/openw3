/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorEditorPanel.h"
#include "behaviorEditor.h"

CEdBehaviorEditorPanel::CEdBehaviorEditorPanel( CEdBehaviorEditor* editor ) 
	: m_editor( editor ) 
{

}

CEdBehaviorEditor* CEdBehaviorEditorPanel::GetEditor() const
{
	return m_editor;
}

CBehaviorGraph* CEdBehaviorEditorPanel::GetBehaviorGraph() const					
{ 
	return m_editor->GetBehaviorGraph(); 
}

CBehaviorGraphInstance* CEdBehaviorEditorPanel::GetBehaviorGraphInstance() const	
{ 
	return m_editor->GetBehaviorGraphInstance(); 
}

CAnimatedComponent* CEdBehaviorEditorPanel::GetAnimatedComponent() const
{
	return m_editor->GetAnimatedComponent(); 
}

CEntity* CEdBehaviorEditorPanel::GetEntity() const
{
	return m_editor->GetEntity();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdBehaviorEditorSimplePanel, wxPanel )
END_EVENT_TABLE()

CEdBehaviorEditorSimplePanel::CEdBehaviorEditorSimplePanel( CEdBehaviorEditor* editor, wxWindow* parent )
	: wxPanel( parent ? parent : editor )
	, CEdBehaviorEditorPanel( editor )
{

}

//////////////////////////////////////////////////////////////////////////
