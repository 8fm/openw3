/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wizardDefinition.h"

IMPLEMENT_ENGINE_CLASS( CWizardBaseNode );
IMPLEMENT_ENGINE_CLASS( CWizardQuestionNode );
IMPLEMENT_ENGINE_CLASS( CWizardDefinition );
IMPLEMENT_ENGINE_CLASS( CWizardDefinitionFactory );


///////////////////////////////////////////////////////////////
// CWizardDefinition
CWizardDefinition::CWizardDefinition()
{

}

////////////////////////////////////////////////////////////////
// CWizardDefinitionFactory
CResource* CWizardDefinitionFactory::DoCreate( const FactoryOptions& options )
{
	CWizardDefinition *wizDef = ::CreateObject< CWizardDefinition >( options.m_parentObject );
	return wizDef;
}