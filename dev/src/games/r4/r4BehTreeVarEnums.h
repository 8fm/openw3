#pragma  once
#include "../../common/game/behTreeVarsEnums.h"
#include "w3GenericVehicle.h"
class CBehTreeValEHorseMoveType : public TBehTreeValEnum< EHorseMoveType, HMT_Gallop >
{
	DECLARE_RTTI_STRUCT( CBehTreeValEHorseMoveType );

private:
	typedef TBehTreeValEnum< EHorseMoveType, HMT_Gallop > TBaseClass;

public:
	CBehTreeValEHorseMoveType( EHorseMoveType e = HMT_Gallop )
		: TBaseClass( e )
	{}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValEHorseMoveType );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Horse Move type") );
END_CLASS_RTTI();