/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formation.h"
#include "moveSteeringBehavior.h"
#include "../core/factory.h"

///////////////////////////////////////////////////////////////////////////////
// CFormationFactory
///////////////////////////////////////////////////////////////////////////////
class CFormationFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CFormationFactory, IFactory, 0 );

public:
	CFormationFactory()
	{
		m_resourceClass = ClassID< CFormation >();
	}

	CResource* DoCreate( const FactoryOptions& options ) override
	{
		return ::CreateObject< CFormation >( options.m_parentObject );		
	}
};

BEGIN_CLASS_RTTI( CFormationFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CFormationFactory );

///////////////////////////////////////////////////////////////////////////////
// CFormation
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CFormation );

CFormation::CFormation()
	: m_uniqueFormationName( CNAME( FORMATION_ ) )
	, m_formationLogic( NULL )
	, m_steeringGraph( NULL )
{

}
CFormation::~CFormation()
{

}

void CFormation::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "uniqueFormationName" ) )
	{
		if ( !m_uniqueFormationName.AsString().BeginsWith( CNAME( FORMATION_ ).AsString() ) )
		{
			m_uniqueFormationName = CNAME( FORMATION_ );
		}
	}
}