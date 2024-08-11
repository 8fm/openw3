#include "build.h"
#include "entityTemplateParams.h"

IMPLEMENT_ENGINE_CLASS( CEntityTemplateParam );
IMPLEMENT_ENGINE_CLASS( CGameplayEntityParam );

CEntityTemplateParam::CEntityTemplateParam()
	: m_wasIncluded( false )
{
}

void CEntityTemplateParam::SetWasIncluded()
{
	m_wasIncluded = true;
}

#ifndef NO_RESOURCE_COOKING
Bool CEntityTemplateParam::IsCooked()
{
	return true;
}
#endif


/////////////////////////////////////////////////////////////
// CTemplateListParam
IMPLEMENT_ENGINE_CLASS( CTemplateListParam );
CTemplateListParam::CTemplateListParam()
	: CGameplayEntityParam( TXT("Template list param"), true )
{
}

#ifndef NO_RESOURCE_COOKING
Bool CTemplateListParam::IsCooked()
{
	return false;
}
#endif
