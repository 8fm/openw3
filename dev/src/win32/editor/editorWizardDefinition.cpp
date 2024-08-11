#include "build.h"
#include "editorWizardDefinition.h"


IMPLEMENT_ENGINE_CLASS( CWizardOption );
IMPLEMENT_ENGINE_CLASS( CWizardNode );
IMPLEMENT_ENGINE_CLASS( CWizardCNameQuestionNode );

//////////////////////////////////////////////////////////////////
// CWizardNode
CWizardBaseNode* CWizardNode::GetChild( Uint32 index )
{ 
	return index < m_options.Size() && index >= 0 ? static_cast< CWizardBaseNode* >( m_options[index] ) : NULL; 
}


//////////////////////////////////////////////////////////////////
// CWizardOption
String CWizardOption::GetValueName() const
{
	if( m_customText.Empty() && m_value )
	{
		return m_value->GetValueName();
	}

	return m_customText;
}

CWizardBaseNode* CWizardOption::GetChild( Uint32 index )
{ 
	return index < m_subNodes.Size() && index >= 0 ? m_subNodes[index] : nullptr; 
}