/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEffectParameterListSelection : public CListSelection
{
protected:
	CEntity *m_entity;
	CFXTrackGroup *m_fxTrackGroup;
	CName m_componentName;

public:
	CEffectParameterListSelection( CPropertyItem* item );
	virtual Bool CanSupportType( const CName &typeName );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};

class CEffectParameterColorListSelection : public CEffectParameterListSelection
{
protected:

public:
	CEffectParameterColorListSelection( CPropertyItem* item )
		: CEffectParameterListSelection( item ) {}

	virtual Bool CanSupportType( const CName &typeName );
};
