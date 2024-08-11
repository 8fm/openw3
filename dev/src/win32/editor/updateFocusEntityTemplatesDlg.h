struct SFocusEntityDefinition
{
	CName					m_actionName;
	CClass*					m_entityClass;
	CName					m_includedComponent;
	TDynArray< CClass* >	m_exceptClasses;
};

class CUpdateFocusEntityTemplatesDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CUpdateFocusEntityTemplatesDlg( wxWindow* parent );
	~CUpdateFocusEntityTemplatesDlg();

private:
	void OnStart( wxCommandEvent &event );
	void OnClose( wxCommandEvent &event );
	void OnClosed( wxCloseEvent &event );

	void UpdateFocusEntityTemplates( const Char* path, Int32 action, Bool extendedInfo );
	Bool CanBeModified( CResource* res );
	Bool HasLightChannelsEnabled( const CEntity* entity );
	Bool GetActionNameForEntityTemplate( const CEntityTemplate* entityTemplate, CName& actionName );
	Bool ModifyEntityTemplate( bool addOrUpdate, CEntityTemplate* entityTemplate, const CName& correctActionName );

private:
	TDynArray< SFocusEntityDefinition > m_definitions;

	wxButton*			m_start;
	wxButton*			m_close;
	wxDirPickerCtrl*	m_browsePath;
	wxRadioBox*			m_actions;
	wxCheckBox*			m_printExtendedInformation;
};