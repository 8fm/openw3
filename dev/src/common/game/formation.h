/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CFormation : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CFormation, CResource, "formation", "Formation definition" );
protected:
	CName								m_uniqueFormationName;										// unique name
	IFormationLogic*					m_formationLogic;											// ai formation algorithm setup
	THandle< CMoveSteeringBehavior >	m_steeringGraph;											// to customize formation at steering layer

public:
	CFormation();
	~CFormation();

	CName							GetUniqueName() const										{ return m_uniqueFormationName; }
	IFormationLogic*				GetFormationLogic() const									{ return m_formationLogic; }
	CMoveSteeringBehavior*			GetSteeringGraph() const									{ return m_steeringGraph.Get(); }

	void							OnPropertyPostChange( IProperty* property ) override;
};

BEGIN_CLASS_RTTI( CFormation )
	PARENT_CLASS( CResource )
	PROPERTY_EDIT( m_uniqueFormationName, TXT("Unique name of formation") )
	PROPERTY_INLINED( m_formationLogic, TXT("Formation AI algorithms setup") )
	PROPERTY_EDIT( m_steeringGraph, TXT("Formation members steering graph") )
END_CLASS_RTTI()

