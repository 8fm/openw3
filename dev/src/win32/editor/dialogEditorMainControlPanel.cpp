// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogEditorMainControlPanel.h"
#include "dialogEditor.h"
#include "../../common/engine/localizationManager.h"

// =================================================================================================
namespace {
// =================================================================================================

class BaseInfo : public wxClientData
{
public:
	BaseInfo( Uint32 localeId );
	~BaseInfo();

	Uint32 m_localeId;
};

/*
Ctor.
*/
BaseInfo::BaseInfo(  Uint32 localeId  )
: m_localeId( localeId )
{}

/*
*/
BaseInfo::~BaseInfo()
{}

class VariantInfo : public wxClientData
{
public:
	VariantInfo( CStorySceneSectionVariantId variantId );
	virtual ~VariantInfo() override;

	CStorySceneSectionVariantId m_variantId;
};

/*
Ctor.
*/
VariantInfo::VariantInfo( CStorySceneSectionVariantId variantId )
: m_variantId( variantId )
{}

/*
Dtor.
*/
VariantInfo::~VariantInfo()
{}

/*
Gets speech locales that use specified variant.

\param section Section under consideration.
\param variantId Variant under consideration. Must not be -1.
\param outLocales (out) Container receiving list of speech locales that use specified section variant.
\return Number of speech locales using specified section variant.
*/
Uint32 GetSpeechLocalesUsingVariant( const CStorySceneSection& section, CStorySceneSectionVariantId variant, TDynArray< String >& outLocales )
{
	// TODO: We should get list of speech languages using CLocalizationManager::GetAllAvailableLanguages()
	// but it doesn't work right - it gives us list of speech languages that contains text languages.
	static String speechLocales[] = { TXT( "PL" ), TXT( "EN" ), TXT( "DE" ), TXT( "FR" ), TXT( "RU" ), TXT( "JP" ), TXT( "BR" ) };

	Uint32 numSpeechLocalesUsingVariant = 0;

	for( auto const& speechLang : speechLocales )
	{
		Uint32 id = -1;
		SLocalizationManager::GetInstance().FindLocaleId( speechLang, id );

		CStorySceneSectionVariantId mappedVariant = section.GetLocaleVariantMapping( id );
		if( ( mappedVariant == variant ) || ( mappedVariant == -1 && variant == section.GetDefaultVariant() ) )
		{
			++numSpeechLocalesUsingVariant;
			outLocales.PushBack( speechLang );
		}
	}

	return numSpeechLocalesUsingVariant;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

// =================================================================================================
namespace SceneEditor {
// =================================================================================================

/*
Ctor.
*/
CMainControlPanel::CMainControlPanel( wxPanel* parent, CEdSceneEditor* sceneEditor )
// TODO: fix order
: m_sceneEditor( sceneEditor )
, m_staticTextSectionName( nullptr )
, m_staticTextVariantName( nullptr )
, m_staticTextDefaultVariant( nullptr )
, m_staticTextForcedVariant( nullptr)
, m_listCtrlSectionVariants( nullptr )
, m_comboPlVariant( nullptr )
, m_comboEnVariant( nullptr )
, m_comboDeVariant( nullptr )
, m_comboFrVariant( nullptr )
, m_comboRuVariant( nullptr )
, m_comboJpVariant( nullptr )
, m_comboBrVariant( nullptr )
, m_sizerVariantPanel( nullptr )
, m_sizerLocalVoMatchApprovedVo( nullptr )
, m_sizerUseApprovedVoDurations( nullptr )
, m_sizerUseLocalVoDurations( nullptr )
, m_sizerVariantNonBaseLangView( nullptr )
, m_staticTextVariantStatus( nullptr )
, m_btnApproveLocalVo( nullptr )
, m_comboVariantBases( nullptr )
{
	ASSERT( sceneEditor );

	wxXmlResource::Get()->LoadPanel( this, parent, "MainControlPanel" );
	SetupControls();

	Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CMainControlPanel::OnVariantActivated, this );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnSetCurrentVariantAsDefault, this, XRCID("btnSetCurrentVariantAsDefault") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnToggleForceCurrentVariantInEditor, this, XRCID("btnToggleForceCurrentVariantInEditor") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnCreateEmptyVariant, this, XRCID("btnCreateEmptyVariant") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnCloneCurrentVariant, this, XRCID("btnCloneCurrentVariant") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnDeleteCurrentVariant, this, XRCID("btnDeleteCurrentVariant") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnCreateUnscaledVariant, this, XRCID("btnCreateUnscaledVariant1") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnCreateUnscaledVariant, this, XRCID("btnCreateUnscaledVariant2") );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnApproveLocalVo, this, XRCID("btnApproveLocalVo") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnBaseView, this, XRCID("btnBaseView") );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnViewUsingLocalVoDurations, this, XRCID("btnViewUsingLocalVoDurations") );
	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CMainControlPanel::OnBtnViewUsingApprovedVoDurations, this, XRCID("btnViewUsingApprovedVoDurations") );

	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboVariantBaseSelected, this, XRCID("comboVariantBases") );

	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboPlVariantSelected, this, XRCID("comboPlVariant") );
	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboEnVariantSelected, this, XRCID("comboEnVariant") );
	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboDeVariantSelected, this, XRCID("comboDeVariant") );
	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboFrVariantSelected, this, XRCID("comboFrVariant") );
	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboRuVariantSelected, this, XRCID("comboRuVariant") );
	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboJpVariantSelected, this, XRCID("comboJpVariant") );
	Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CMainControlPanel::OnComboBrVariantSelected, this, XRCID("comboBrVariant") );
}

/*
Dtor.
*/
 CMainControlPanel::~CMainControlPanel()
{}

/*
Performs initial setup of dialog controls.
*/
void CMainControlPanel::SetupControls()
{
	m_staticTextSectionName = XRCCTRL( *this, "staticTextSectionName", wxStaticText );
	m_staticTextVariantName = XRCCTRL( *this, "staticTextVariantName", wxStaticText );
	m_staticTextDefaultVariant = XRCCTRL( *this, "staticTextDefaultVariant", wxStaticText );
	m_staticTextForcedVariant = XRCCTRL( *this, "staticTextForcedVariant", wxStaticText );
	m_staticTextVariantStatus = XRCCTRL( *this, "staticTextVariantStatus", wxStaticText );
	m_listCtrlSectionVariants = XRCCTRL( *this, "listCtrlSectionVariants", wxListCtrl );
	// TODO: add btnToggleForceCurrentVariantInEditor, btnCreateEmptyVariant, btnCloneCurrentVariant
	// (or not if we don't need to use them besides binding event handlers..)

	ASSERT( m_staticTextSectionName );
	ASSERT( m_staticTextVariantName );
	ASSERT( m_staticTextDefaultVariant );
	ASSERT( m_staticTextForcedVariant );
	ASSERT( m_staticTextVariantStatus );
	ASSERT( m_listCtrlSectionVariants );

	m_comboPlVariant = XRCCTRL( *this, "comboPlVariant", wxComboBox );
	m_comboEnVariant = XRCCTRL( *this, "comboEnVariant", wxComboBox );
	m_comboDeVariant = XRCCTRL( *this, "comboDeVariant", wxComboBox );
	m_comboFrVariant = XRCCTRL( *this, "comboFrVariant", wxComboBox );
	m_comboRuVariant = XRCCTRL( *this, "comboRuVariant", wxComboBox );
	m_comboJpVariant = XRCCTRL( *this, "comboJpVariant", wxComboBox );
	m_comboBrVariant = XRCCTRL( *this, "comboBrVariant", wxComboBox );

	ASSERT( m_comboPlVariant );
	ASSERT( m_comboEnVariant );
	ASSERT( m_comboDeVariant );
	ASSERT( m_comboFrVariant );
	ASSERT( m_comboRuVariant );
	ASSERT( m_comboJpVariant );
	ASSERT( m_comboBrVariant );

	m_sizerVariantPanel = XRCCTRL( *this, "panelVariant", wxPanel )->GetSizer();
	m_sizerLocalVoMatchApprovedVo = XRCCTRL( *this, "staticTextPlaceholderLocalVoMatchApprovedVo", wxStaticText )->GetContainingSizer();
	m_sizerUseApprovedVoDurations = XRCCTRL( *this, "staticTextUseApprovedVoDurations", wxStaticText )->GetContainingSizer();
	m_sizerUseLocalVoDurations = XRCCTRL( *this, "staticTextUseLocalVoDurations", wxStaticText )->GetContainingSizer();
	m_sizerVariantNonBaseLangView = XRCCTRL( *this, "staticTextViewingVariantInNonbaseLang", wxStaticText )->GetContainingSizer();

	ASSERT( m_sizerVariantPanel );
	ASSERT( m_sizerLocalVoMatchApprovedVo );
	ASSERT( m_sizerUseApprovedVoDurations );
	ASSERT( m_sizerUseLocalVoDurations );
	ASSERT( m_sizerVariantNonBaseLangView );

	m_btnApproveLocalVo = XRCCTRL( *this, "btnApproveLocalVo", wxButton );
	ASSERT( m_btnApproveLocalVo );

	m_comboVariantBases = XRCCTRL( *this, "comboVariantBases", wxComboBox );
	ASSERT( m_comboVariantBases );
}

void CMainControlPanel::UpdateUI()
{
	wxFont fontSectionName( 12, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Tahoma" );
	wxFont fontVariantName( 11, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Tahoma" );
	wxFont fontSectionVariantNormal( 9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Tahoma" );
	wxFont fontSectionVariantChosen( 9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Tahoma" );

	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

		Uint32 localeId = -1;
		m_comboVariantBases->Clear();
		
		SLocalizationManager::GetInstance().FindLocaleId( TXT("PL"), localeId );
		m_comboVariantBases->Append( "PL", new BaseInfo( localeId ) );
		SLocalizationManager::GetInstance().FindLocaleId( TXT("EN"), localeId );
		m_comboVariantBases->Append( "EN", new BaseInfo( localeId ) );
		SLocalizationManager::GetInstance().FindLocaleId( TXT("DE"), localeId );
		m_comboVariantBases->Append( "DE", new BaseInfo( localeId ) );
		SLocalizationManager::GetInstance().FindLocaleId( TXT("FR"), localeId );
		m_comboVariantBases->Append( "FR", new BaseInfo( localeId ) );
		SLocalizationManager::GetInstance().FindLocaleId( TXT("RU"), localeId );
		m_comboVariantBases->Append( "RU", new BaseInfo( localeId ) );
		SLocalizationManager::GetInstance().FindLocaleId( TXT("JP"), localeId );
		m_comboVariantBases->Append( "JP", new BaseInfo( localeId ) );
		SLocalizationManager::GetInstance().FindLocaleId( TXT("BR"), localeId );
		m_comboVariantBases->Append( "BR", new BaseInfo( localeId ) );

		// select appropriate item in variant base combobox
		const Uint32 currentVariantBaseLocaleId = currentSection->GetVariantBaseLocale( currentVariantId );
		for( Uint32 i = 0, numItems = m_comboVariantBases->GetCount(); i < numItems; ++i )
		{
			BaseInfo* baseInfo = static_cast< BaseInfo* >( m_comboVariantBases->GetClientObject( i ) );
			if( baseInfo->m_localeId == currentVariantBaseLocaleId )
			{
				m_comboVariantBases->SetSelection( i );
				break;
			}
		}

		m_comboPlVariant->Clear();
		m_comboEnVariant->Clear();
		m_comboDeVariant->Clear();
		m_comboFrVariant->Clear();
		m_comboRuVariant->Clear();
		m_comboJpVariant->Clear();
		m_comboBrVariant->Clear();

		// add entry for default variant into each lang combobox
		m_comboPlVariant->Append( "Default Variant", new VariantInfo( -1 ) );
		m_comboEnVariant->Append( "Default Variant", new VariantInfo( -1 ) );
		m_comboDeVariant->Append( "Default Variant", new VariantInfo( -1 ) );
		m_comboFrVariant->Append( "Default Variant", new VariantInfo( -1 ) );
		m_comboRuVariant->Append( "Default Variant", new VariantInfo( -1 ) );
		m_comboJpVariant->Append( "Default Variant", new VariantInfo( -1 ) );
		m_comboBrVariant->Append( "Default Variant", new VariantInfo( -1 ) );

		m_staticTextSectionName->SetFont( fontSectionName );
		m_staticTextSectionName->SetLabel( currentSection->GetName().AsChar() );

		wxString variantName;
		variantName.Printf( "Variant %u", currentVariantId );	// TODO: We do this in two places, make this into single function.
		m_staticTextVariantName->SetFont( fontVariantName );
		m_staticTextVariantName->SetLabel( variantName );

		// if variant default
		if( currentVariantId == currentSection->GetDefaultVariant() )
		{
			m_staticTextDefaultVariant->Show( true );
		}
		else
		{
			m_staticTextDefaultVariant->Show( false );
		}

		// if variant forced
		if( currentVariantId == currentSection->GetVariantForcedInEditor() )
		{
			m_staticTextForcedVariant->Show( true );
		}
		else
		{
			m_staticTextForcedVariant->Show( false );
		}

		if( currentLocaleId == currentSection->GetVariantBaseLocale( currentVariantId ) )
		{
			// Variant is viewed in base lang.

			m_sizerVariantNonBaseLangView->ShowItems( false );

			if( m_sceneEditor->OnMainControlPanel_LocalVoMatchApprovedVoInCurrentSectionVariant() )
			{
				m_sizerLocalVoMatchApprovedVo->ShowItems( true );
				m_sizerUseApprovedVoDurations->ShowItems( false );
				m_sizerUseLocalVoDurations->ShowItems( false );
			}
			else
			{
				if( m_sceneEditor->GetConfigUseApprovedVoDurations() )
				{
					m_sizerLocalVoMatchApprovedVo->ShowItems( false );
					m_sizerUseApprovedVoDurations->ShowItems( true );
					m_sizerUseLocalVoDurations->ShowItems( false );
				}
				else
				{
					m_sizerLocalVoMatchApprovedVo->ShowItems( false );
					m_sizerUseApprovedVoDurations->ShowItems( false );
					m_sizerUseLocalVoDurations->ShowItems( true );
				}
			}
		}
		else
		{
			m_sizerLocalVoMatchApprovedVo->ShowItems( false );
			m_sizerUseApprovedVoDurations->ShowItems( false );
			m_sizerUseLocalVoDurations->ShowItems( false );
			m_sizerVariantNonBaseLangView->ShowItems( true );
		}
		
		//bla bla sizerLocalVoDontMatchApprovedVo_UseApprovedVoDurations
		
		//m_sizerVariantOutOfDate->Layout();
		//m_sizerVariantNonBaseLangView->Layout();
		m_sizerVariantPanel->Layout();
		GetSizer()->Layout();

		m_listCtrlSectionVariants->ClearAll();

		m_listCtrlSectionVariants->SetFont( fontSectionVariantNormal );

		m_listCtrlSectionVariants->InsertColumn( 0, "variant" );
		m_listCtrlSectionVariants->InsertColumn( 1, "base" );
		m_listCtrlSectionVariants->InsertColumn( 2, "used by" );

		TDynArray< CStorySceneSectionVariantId > variants;
		currentSection->EnumerateVariants( variants );
		Sort( variants.Begin(), variants.End() );

		for( auto variantId : variants )
		{
			wxString variantName;
			variantName.Printf( "Variant %u", variantId );

			const long itemIndex = m_listCtrlSectionVariants->GetItemCount();
			const long assignedItemIndex = m_listCtrlSectionVariants->InsertItem( itemIndex, variantName );
			// wxWidgets docs are not clear whether itemIndex and assignedItemIndex may differ or not. Let's see..
			ASSERT( itemIndex == assignedItemIndex );

			m_listCtrlSectionVariants->SetItemPtrData( itemIndex, variantId );

			String baseLang;
			SLocalizationManager::GetInstance().FindLocaleStr( currentSection->GetVariantBaseLocale( variantId ), baseLang );
			m_listCtrlSectionVariants->SetItem( itemIndex, 1, baseLang.AsChar() );

			TDynArray< String > associatedSpeechLangs;
			GetSpeechLocalesUsingVariant( *currentSection, variantId, associatedSpeechLangs );
			m_listCtrlSectionVariants->SetItem( itemIndex, 2, String::Join( associatedSpeechLangs, TXT(" ") ).AsChar() );

			if( variantId == currentVariantId )
			{
				m_listCtrlSectionVariants->SetItemFont( itemIndex, fontSectionVariantChosen );
				m_listCtrlSectionVariants->EnsureVisible( itemIndex );
			}

			if( variantId == currentSection->GetDefaultVariant() )
			{
				m_listCtrlSectionVariants->SetItemTextColour( itemIndex, *wxBLUE );
			}

			// note that if variant is both default and forced then it will be drawn red
			if( variantId == currentSection->GetVariantForcedInEditor() )
			{
				m_listCtrlSectionVariants->SetItemTextColour( itemIndex, *wxRED );
			}

			// add entry for this variant into each lang combobox
			m_comboPlVariant->Append( variantName, new VariantInfo( variantId ) );
			m_comboEnVariant->Append( variantName, new VariantInfo( variantId ) );
			m_comboDeVariant->Append( variantName, new VariantInfo( variantId ) );
			m_comboFrVariant->Append( variantName, new VariantInfo( variantId ) );
			m_comboRuVariant->Append( variantName, new VariantInfo( variantId ) );
			m_comboJpVariant->Append( variantName, new VariantInfo( variantId ) );
			m_comboBrVariant->Append( variantName, new VariantInfo( variantId ) );
		}

		m_listCtrlSectionVariants->SetColumnWidth( 0, wxLIST_AUTOSIZE );
		m_listCtrlSectionVariants->SetColumnWidth( 1, wxLIST_AUTOSIZE_USEHEADER );
		m_listCtrlSectionVariants->SetColumnWidth( 2, wxLIST_AUTOSIZE_USEHEADER );

		auto selectVariantInCombo = [ this, currentSection ] ( const String& localeName, wxComboBox* combo )
		{
			Uint32 localeId = -1;
			SLocalizationManager::GetInstance().FindLocaleId( localeName, localeId );

			CStorySceneSectionVariantId variantIdToFind = currentSection->GetLocaleVariantMapping( localeId );
			for( Uint32 i = 0, numItems = m_comboPlVariant->GetCount(); i < numItems; ++i )
			{
				VariantInfo* variantInfo = static_cast< VariantInfo* >( combo->GetClientObject( i ) );
				if( variantInfo->m_variantId == variantIdToFind )
				{
					combo->SetSelection( i );
				}
			}
		};

		selectVariantInCombo( TXT("PL"), m_comboPlVariant );
		selectVariantInCombo( TXT("EN"), m_comboEnVariant );
		selectVariantInCombo( TXT("DE"), m_comboDeVariant );
		selectVariantInCombo( TXT("FR"), m_comboFrVariant );
		selectVariantInCombo( TXT("RU"), m_comboRuVariant );
		selectVariantInCombo( TXT("JP"), m_comboJpVariant );
		selectVariantInCombo( TXT("BR"), m_comboBrVariant );
	}
}

void CMainControlPanel::OnVariantActivated( wxListEvent& event )
{
	CStorySceneSectionVariantId variantId = m_listCtrlSectionVariants->GetItemData( event.GetIndex() );

	m_sceneEditor->OnMainControlPanel_RequestSectionVariant( variantId );
}

void CMainControlPanel::OnBtnToggleForceCurrentVariantInEditor( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

		CStorySceneSectionVariantId variantIdToForce = currentVariantId;
		if( currentVariantId == currentSection->GetVariantForcedInEditor() )
		{
			// current variant is already forced so we want to unforce it
			variantIdToForce = -1;
		}

		m_sceneEditor->OnMainControlPanel_SetVariantForcedInEditor( variantIdToForce );
	}
}

void CMainControlPanel::OnBtnSetCurrentVariantAsDefault( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

		m_sceneEditor->OnMainControlPanel_SetVariantAsDefault( currentVariantId );
	}
}

/*
Creates empty variant in current locale.
*/
void CMainControlPanel::OnBtnCreateEmptyVariant( wxCommandEvent& event )
{
	// TODO: We should get list of speech languages using CLocalizationManager::GetAllAvailableLanguages()
	// but it doesn't work right - it gives us list of speech languages that contains text languages.
	static String speechLocales[] = { TXT( "PL" ), TXT( "EN" ), TXT( "DE" ), TXT( "FR" ), TXT( "RU" ), TXT( "JP" ), TXT( "BR" ) };

	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();

	String loc;
	SLocalizationManager::GetInstance().FindLocaleStr( currentLocaleId, loc );
	if( loc != TXT("PL") && loc != TXT("EN") && loc != TXT("DE") && loc != TXT("FR") && loc != TXT("RU") && loc != TXT("JP") && loc != TXT("BR") )
	{
		wxMessageBox( "Can't create variant for currently selected language. Please choose one of voice languages." );
		return;
	}

	m_sceneEditor->OnMainControlPanel_RequestCreateEmptyVariant( currentLocaleId );
}

void CMainControlPanel::OnBtnCloneCurrentVariant( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

		m_sceneEditor->OnMainControlPanel_RequestCloneVariant( currentVariantId );
	}
}

void CMainControlPanel::OnBtnDeleteCurrentVariant( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId variantToDelete = currentSection->GetVariantUsedByLocale( currentLocaleId );

		// Don't allow deleting variant that is associated with any language or that is set as default variant
		// (this will also make it impossible to delete last existing variant as it will be set as default).
		TDynArray< String > associatedSpeechLangs;
		GetSpeechLocalesUsingVariant( *currentSection, variantToDelete, associatedSpeechLangs );
		if( variantToDelete == currentSection->GetDefaultVariant() || !associatedSpeechLangs.Empty() )
		{
			wxMessageBox( "Can't delete variant that is used by some language or is set as default variant." );
			return;
		}

		// Choose variant to which we will switch.
		CStorySceneSectionVariantId variantToSwitchTo = currentSection->GetVariantForcedInEditor();
		if( variantToSwitchTo == -1 || variantToSwitchTo == variantToDelete )
		{
			variantToSwitchTo = currentSection->GetLocaleVariantMapping( currentLocaleId );
			if( variantToSwitchTo == -1 )
			{
				variantToSwitchTo = currentSection->GetDefaultVariant();
			}
		}

		// Switch to variant associated with current language before deleting current variant.
		m_sceneEditor->OnMainControlPanel_RequestSectionVariant( variantToSwitchTo );
		m_sceneEditor->OnMainControlPanel_RequestRebuildImmediate();

		// Delete variant that was current before the switch.
		m_sceneEditor->OnMainControlPanel_RequestDeleteAllEvents( variantToDelete );
		m_sceneEditor->OnMainControlPanel_RequestDeleteVariant( variantToDelete );
	}
}

void CMainControlPanel::OnBtnCreateUnscaledVariant( wxCommandEvent& event )
{
	Int32 result = m_sceneEditor->OnMainControlPanel_CreateUnscaledVariantFromCurrentView();

	if( const Bool success = ( result == 0 ) )
	{
		// all ok, nothing to do
	}
	else if( const Bool invalidElApprDur = ( result == 1 ) )
	{
		wxMessageBox( "Unscaled variant not created - at least one scene element was never approved.\n"
			          "Please switch to approved view, approve durations of all elements and try again.\n");
	}
	else if( const Bool rsltViewTooShort = ( result == 2 ) )
	{
		wxMessageBox( "Unscaled variant not created - it would be too short to fit all the events.\n"
					  "Please add pause element at the end of a section and try again. Feel free\n"
					  "to delete that pause (or leave it be) after you adjust positions of events." );
	}
	else
	{
		// Unknown result status.
		RED_FATAL_ASSERT( false, "" );
	}
}

void CMainControlPanel::OnBtnApproveLocalVo( wxCommandEvent& event )
{
	const Bool privilegeApproveVo = m_sceneEditor->GetRedUserPrivileges().m_approveVo;
	if( privilegeApproveVo )
	{
		const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
		if( currentSection )
		{
			const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
			const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

			if( currentLocaleId != currentSection->GetVariantBaseLocale( currentVariantId ) )
			{
				wxMessageBox( "Please switch to variant language before updating it." );
				return;
			}

			m_sceneEditor->OnMainControlPanel_UpdateCurrentVariantBase();
		}
	}
	else
	{
		wxMessageBox( "Local VO may only be approved by users with approve_vo privilege." );
	}
}

void CMainControlPanel::OnBtnBaseView( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

		const Uint32 variantBaseLocaleId = currentSection->GetVariantBaseLocale( currentVariantId );
		m_sceneEditor->OnMainControlPanel_RequestChangeLocale( variantBaseLocaleId );
	}
}

void CMainControlPanel::OnBtnViewUsingApprovedVoDurations( wxCommandEvent& event )
{
	m_sceneEditor->OnMainControlPanel_SetConfigUseApprovedDurations( true );
}

void CMainControlPanel::OnBtnViewUsingLocalVoDurations( wxCommandEvent& event )
{
	m_sceneEditor->OnMainControlPanel_SetConfigUseApprovedDurations( false );
}

void CMainControlPanel::OnComboVariantBaseSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		// get current variant
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

		// get locale id associated with selected item
		const BaseInfo* baseInfo = static_cast< const BaseInfo* >( event.GetClientObject() );
		const Uint32 newBaseLocaleId = baseInfo->m_localeId;

		if( newBaseLocaleId != currentSection->GetVariantBaseLocale( currentVariantId ) )
		{
			m_sceneEditor->OnMainControlPanel_SetVariantBase( currentVariantId, newBaseLocaleId );
			m_sceneEditor->OnMainControlPanel_RequestChangeLocale( newBaseLocaleId );
			m_sceneEditor->OnMainControlPanel_RequestRebuildImmediate();
		}
	}
}

void CMainControlPanel::OnComboPlVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "PL" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

void CMainControlPanel::OnComboEnVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "EN" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

void CMainControlPanel::OnComboDeVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "DE" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

void CMainControlPanel::OnComboFrVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "FR" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

void CMainControlPanel::OnComboRuVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "RU" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

void CMainControlPanel::OnComboJpVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "JP" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

void CMainControlPanel::OnComboBrVariantSelected( wxCommandEvent& event )
{
	const CStorySceneSection* currentSection = m_sceneEditor->OnMainControlPanel_GetCurrentSection();
	if( currentSection )
	{
		Uint32 localeId = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT( "BR" ), localeId );

		// get variant id associated with selected item
		const VariantInfo* variantInfo = static_cast< const VariantInfo* >( event.GetClientObject() );
		const CStorySceneSectionVariantId variantId = variantInfo->m_variantId;

		m_sceneEditor->OnMainControlPanel_SetLocaleVariantMapping( localeId, variantId );
	}
}

// =================================================================================================
} // namespace SceneEditor
// =================================================================================================
