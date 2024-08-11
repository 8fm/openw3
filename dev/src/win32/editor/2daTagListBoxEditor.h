#pragma once

class CEd2daTagListBoxEditor : public CListBoxEditor
{
private:
	String m_2daFilename;
	String m_2daValueColumn;

public:
	CEd2daTagListBoxEditor( CPropertyItem* item, String& filename, String& valueColumn );
	virtual ~CEd2daTagListBoxEditor(void);

	virtual Bool GrabValue( String& displayValue ) override;

protected:
	virtual wxArrayString GetListElements();
	virtual void SelectPropertyElements();

	virtual void SelectElement( wxString element );
	virtual void DeselectElement( wxString element );

private:
	void ToggleTag( String& element, Bool selected );
};
