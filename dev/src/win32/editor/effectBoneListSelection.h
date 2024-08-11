/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEffectBoneListSelection : public CListSelection
{
private:
	// Hack for holding entity to retrieve bones from
	static THandle< CEntity > m_editedEntity;

	CPropertyItem *m_propertyItem;

public:
	static void SetEntity( CEntity* entity );

	CEffectBoneListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	void LoadData();

	virtual Bool CanSupportType( const CName &typeName ) {return true;}
};

class CSlotBoneListSelection : public wxEvtHandler, public ICustomPropertyEditor
{
public:
	CSlotBoneListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour ) override;
	virtual Bool SaveValue() override;
	virtual Bool GrabValue( String& displayValue ) override;

protected:
	CComponent* GetSlotComponent( CEntity** temporaryEntity );

protected:
	void OnBonePick( wxCommandEvent& event );
	void OnBoneReset( wxCommandEvent& event );
};
