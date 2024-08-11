/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//! Base class for drop-down selectors. Works for properties of type String or CName.
class ISelectionEditor : public wxEvtHandler
					   , public ICustomPropertyEditor
{
protected:
	CEdChoice	*m_ctrlChoice;

public:
	ISelectionEditor( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void FillChoices() = 0;
	virtual void OnChoiceChanged( wxCommandEvent &event );
	virtual Bool IsTextEditable() const;

	virtual String DisplayedToStored( const String& val );
	virtual String StoredToDisplayed( const String& val );
	
	template< class T >
	T* GetPropertyOwnerObject() const { return m_propertyItem->GetRootObject( 0 ).As< T >(); }
};

//! Base class for drop-down selectors with different value shown than stored. Works for properties of type String or CName.
class CEdMappedSelectionEditor : public ISelectionEditor
{
public:
	CEdMappedSelectionEditor( CPropertyItem* item );

protected:
	//! Stored -> displayed. TDynArray is used instad of TMap to preserve the order, do not change this!
	virtual void FillMap( TDynArray< TPair< String, String > >& map ) =0;

private:
	virtual void FillChoices() override;
	virtual Bool IsTextEditable() const override;
	virtual String DisplayedToStored( const String& val ) override;
	virtual String StoredToDisplayed( const String& val ) override;

	TDynArray< TPair< String, String > > m_map;
};
