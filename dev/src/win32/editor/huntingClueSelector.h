#pragma once

class CEdJournalHuntingClueSelector : public ICustomPropertyEditor
{
public:
	CEdJournalHuntingClueSelector( CPropertyItem* propertyItem );
	virtual ~CEdJournalHuntingClueSelector();

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	// 	virtual Bool OnSaveValue();
	// 	

private:

	const CJournalCreatureHuntingClueGroup* GetHuntingClueGroup() const;
	
	CJournalCreatureHuntingClueGroup* GetHuntingClueGroup()
	{
		return const_cast< CJournalCreatureHuntingClueGroup* >( static_cast< const CEdJournalHuntingClueSelector* >( this )->GetHuntingClueGroup() );
	}

	void OnClueSelected( wxCommandEvent& event );

	wxChoice* m_choice;
};
