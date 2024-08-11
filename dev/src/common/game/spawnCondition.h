#pragma once


//////////////////////////////////////////////////////////////////////////

class ISpawnCondition : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( ISpawnCondition );

public:
	ISpawnCondition()														{ EnableReferenceCounting( true ); }

	virtual Bool Test( CSpawnTreeInstance& instance );
};
BEGIN_ABSTRACT_CLASS_RTTI( ISpawnCondition );
	PARENT_CLASS( IScriptable );
END_CLASS_RTTI();

class ISpawnScriptCondition : public ISpawnCondition
{
	DECLARE_RTTI_SIMPLE_CLASS( ISpawnScriptCondition );

public:
	Bool Test( CSpawnTreeInstance& instance ) override;
};
BEGIN_CLASS_RTTI( ISpawnScriptCondition );
	PARENT_CLASS( ISpawnCondition );
END_CLASS_RTTI();

