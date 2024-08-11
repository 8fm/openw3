
#pragma once

class CEdRemappingDialog : private wxDialog
{
public:

	struct MappingEntry
	{
		MappingEntry( const String& original, const TDynArray< String >& possibilites, Int32 selectedIdx = -1, const String& toolTooltip = String::EMPTY )
			: m_original( original ), m_possibilities( possibilites ), m_selectedIdx( selectedIdx ), m_toolTip( toolTooltip ), m_arrowText ( TXT("-->") )
			{}

		String m_original;
		TDynArray< String > m_possibilities;
		Int32 m_selectedIdx;
		String m_toolTip;
		String m_arrowText;
		String m_iconResource;
	};

	typedef TDynArray< MappingEntry > Mappings;

	CEdRemappingDialog( wxWindow* parent, const String& title );

	using wxDialog::SetFocus;

	Bool Execute( Mappings& mappings );

	void ResetMappings( const Mappings& mappings );

	void SetupSpecialActionButton( const String& label, std::function< void ( CEdRemappingDialog* reamppingDlg  ) > handler );

	TDynArray< String > GetOriginals();

	void UpdateSelection( const String& original, const String& newValue, Bool appendIfNotExist = false );

private:
	Mappings							m_mappings;
	wxScrolled< wxPanel >*				m_slotsPanel;
	wxCheckBox*							m_selectAll;
	wxButton*							m_specialActionBtn;
	TDynArray< class CRemappingSlot* >	m_slots;

	std::function< void( CEdRemappingDialog* reamppingDlg ) > m_specialActionFunc;

	void OnSelectAll( wxCommandEvent& event );
	void OnSpecialActionButtonClicked( wxCommandEvent& event );
};
