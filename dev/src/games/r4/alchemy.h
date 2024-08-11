#pragma once
class alchemy
{
public:
	alchemy(void);
	~alchemy(void);
};
#pragma once


struct SAlchemySubstanceData
{
	DECLARE_RTTI_STRUCT( SAlchemySubstanceData );

	CName m_type;
	Int32 m_level;
	Bool m_isPermanent;
};

BEGIN_CLASS_RTTI( SAlchemySubstanceData )
	PROPERTY( m_type )
	PROPERTY( m_level )
	PROPERTY( m_isPermanent )
END_CLASS_RTTI();

