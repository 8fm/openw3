/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once


// This editor allows to edit values from the specified column in a 2d array
// and select them.
//
// Params are specified using the S2daValueProperties structure - the editor
// will acquire its instance using I2dArrayPropertyOwner::Get2dArrayPropertyAdditionalProperties
// method.
//
// The editor can work using an option-exclusiveness mode. What this means is that you
// can specify an additional column name using the S2daValueProperties::m_valueColumnName
// param. The editor will put additional information there, specifying whether
// a given option is still available for selection. This works like a filter - 
// options once selected will not be available for further selection.
//
// S2daValueProperties params specification:
//	@param m_array				array being edited
//  @param m_descrColumnName	column o which the names will be added
//  @param m_valueColumnName	specify the column which holds the option availability information
//  @param m_filters			not used

// 
class C2dArrayExclusiveValueEdition : public CListSelection
{
protected:
	CGatheredResource&	m_csvResource;
	Bool				m_exclusive;
	CEdChoice*			m_ctrl;
	String				m_oldValue;

	String				m_descrColumnName;

public:
	C2dArrayExclusiveValueEdition( CPropertyItem* item, Bool	exclusive, CGatheredResource& csvResource );

	// Set column name from 2dArray which is used for values shown. Default is "tag", which was previously the only thing used.
	void SetDescriptionColumnName( const String& columnName );

	virtual void CloseControls() override;
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );
	virtual void OnTextEntered( wxCommandEvent &event );

private:
	Bool GetProperties( S2daValueProperties& properties );
	void RefreshValues( S2daValueProperties& properties );
	Int32 FindElemRow( C2dArray& arr, const String& colName, const String& val ) const;
	void AddEntry( S2daValueProperties& properties, const String& newVal ) const;
	void ReleaseValue( S2daValueProperties& properties );
	void SelectNewValue( S2daValueProperties& properties, Uint32 newValueRowIdx, const String& newVal );
};
