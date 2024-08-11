
#pragma once

#include "entityTemplateParams.h"

class CAnimConstraintsParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CAnimConstraintsParam, CEntityTemplateParam, 0 );

protected:
	THandle< CBehaviorGraph >		m_graph;

public:
	CAnimConstraintsParam();

public:
	const CBehaviorGraph* GetConstraintGraph() const;
};

BEGIN_CLASS_RTTI( CAnimConstraintsParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_graph, TXT("Constraint graph") );
END_CLASS_RTTI();
