#include "build.h"
#include "updateFocusEntityTemplatesDlg.h"

#include "../../common/core/feedback.h"
#include "../../common/core/depot.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/meshComponent.h"
#include "../../games/r4/focusModeController.h"

BEGIN_EVENT_TABLE( CUpdateFocusEntityTemplatesDlg, wxDialog )
	EVT_BUTTON( XRCID("m_selectPath"), CUpdateFocusEntityTemplatesDlg::OnStart )
	EVT_BUTTON( XRCID("m_start"), CUpdateFocusEntityTemplatesDlg::OnStart )
	EVT_BUTTON( XRCID("m_close"), CUpdateFocusEntityTemplatesDlg::OnClose )
	EVT_CLOSE( CUpdateFocusEntityTemplatesDlg::OnClosed )
END_EVENT_TABLE()

RED_DEFINE_STATIC_NAME( CFocusActionComponent )

#define ACTION_PRINT_ONLY_ABOUT_MISSING_OR_INCORRECT_COMPONENTS	0
#define ACTION_PRINT_ONLY_ABOUT_MISSING_LIGHT_CHANNELS			1
#define ACTION_ADD_COMPONENT_WHERE_MISSING						2
#define ACTION_UPDATE_COMPONENT_WHERE_NOT_CORRECT				3
#define ACTION_REMOVE_COMPONENTS								4

CUpdateFocusEntityTemplatesDlg::CUpdateFocusEntityTemplatesDlg( wxWindow* parent )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("UpdateFocusEntityTemplates") );

	m_start						= XRCCTRL( *this, "m_start", wxButton );
	m_close						= XRCCTRL( *this, "m_close", wxButton );
	m_browsePath				= XRCCTRL( *this, "m_browsePath", wxDirPickerCtrl );
	m_browsePath->SetDirName( wxFileName( wxString( GDepot->GetRootDataPath().AsChar() ) /*+ wxString( wxT("qa\\pawelm_test\\") )*/ ) );
	m_actions					= XRCCTRL( *this, "m_actions", wxRadioBox );
	m_printExtendedInformation	= XRCCTRL( *this, "m_printExtendedInformation", wxCheckBox );

	C2dArray *list = LoadResource< C2dArray >( TXT("engine/misc/focus_entities.csv") );
	if ( !list )
	{
		return;
	}

	for ( Uint32 i = 0; i < list->GetNumberOfRows(); ++i )
	{
		SFocusEntityDefinition definition;

		// action name
		definition.m_actionName	= CName( list->GetValue( TXT("ActionName"), i ) );

		// entity class
		String className = list->GetValue( TXT("ClassName"), i );
		if ( className.Empty() )
		{
			definition.m_entityClass = nullptr;
		}
		else
		{
			definition.m_entityClass = SRTTI::GetInstance().FindClass( CName( className ) );
		}

		// included components
		definition.m_includedComponent	= CName( list->GetValue( TXT("IncludedComponent"), i ) );

		// class exceptions
		String exceptClassesString = list->GetValue( TXT("ClassNameExceptions"), i );
		if ( !exceptClassesString.Empty() )
		{
			TDynArray< String > exceptClasses;
			exceptClassesString.GetTokens( ' ', true, exceptClasses );
			for ( Uint32 j = 0; j < exceptClasses.Size(); ++j )
			{
				if ( !exceptClasses[ j ].Empty() )
				{
					CClass* exceptClass = SRTTI::GetInstance().FindClass( CName( exceptClasses[ j ] ) );
					if ( exceptClass )
					{
						definition.m_exceptClasses.PushBackUnique( exceptClass );
					}
				}
			}
		}

		m_definitions.PushBack( definition );
	}
}

CUpdateFocusEntityTemplatesDlg::~CUpdateFocusEntityTemplatesDlg()
{
}

void CUpdateFocusEntityTemplatesDlg::OnStart( wxCommandEvent &event )
{
	const String& rootPath = GDepot->GetRootDataPath();
	String path = m_browsePath->GetDirName().GetFullPath().wc_str();
	path.Replace( rootPath, String::EMPTY );

	m_start->Enable( false );
	m_close->Enable( false );
	GFeedback->BeginTask( TXT( "Checking entity templates" ), false );

	UpdateFocusEntityTemplates( path.AsChar(), m_actions->GetSelection(), m_printExtendedInformation->IsChecked() );

	m_start->Enable( true );
	m_close->Enable( true );
	GFeedback->EndTask();
}

void CUpdateFocusEntityTemplatesDlg::OnClose( wxCommandEvent &event )
{
	Close();
}

void CUpdateFocusEntityTemplatesDlg::OnClosed( wxCloseEvent &event )
{
	DestroyLater( this );
}

void CUpdateFocusEntityTemplatesDlg::UpdateFocusEntityTemplates( const Char* path, Int32 action, Bool extendedInfo )
{
	Uint32 templatesWithLightChannel = 0;
	Uint32 templatesSaved = 0;
	Uint32 templatesNotSaved = 0;
	Uint32 templatesWithMissingLightChannel = 0;

	CDirectory* dir = GDepot->FindPath( path );
	if ( !dir )
	{
		RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("Invalid path [%s]"), path );
		return;
	}

	TDynArray< String > resources;
	dir->FindResourcesByExtension( TXT("w2ent"), resources, true, true );

	RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("--- UPDATING FOCUS ENTITIES --------------------------") );

	static TDynArray< String > infoLines;
	for ( Uint32 r = 0; r < resources.Size(); ++r )
	{
		GFeedback->UpdateTaskProgress( r, resources.Size() );
		GFeedback->UpdateTaskInfo( resources[ r ].AsChar() );

		CResource* res = GDepot->LoadResource( resources[ r ] );
		if ( !res )
		{
			continue;
		}
		CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( res );
		if ( !entityTemplate )
		{
			continue;
		}
		CEntity* templateEntity = entityTemplate->GetEntityObject();
		if ( !templateEntity )
		{
			continue;
		}

		infoLines.ClearFast();
		infoLines.PushBack( String::Printf( TXT("[%s]   [%s]"), entityTemplate->GetEntityClassName().AsChar(), entityTemplate->GetDepotPath().AsChar() ) );

		switch( action )
		{
			case ACTION_PRINT_ONLY_ABOUT_MISSING_OR_INCORRECT_COMPONENTS:
			case ACTION_ADD_COMPONENT_WHERE_MISSING:
			case ACTION_UPDATE_COMPONENT_WHERE_NOT_CORRECT:
				{
					Bool lightChannelsEnabled = HasLightChannelsEnabled( templateEntity );
					if ( lightChannelsEnabled )
					{
						templatesWithLightChannel++;

						static TDynArray< CFocusActionComponent* > focusComponents;
						focusComponents.Clear();
						for ( ComponentIterator< CFocusActionComponent > it( templateEntity ); it; ++it )
						{
							focusComponents.PushBack( *it );
						}

						if ( focusComponents.Size() > 1 )
						{
							infoLines.PushBack( String::Printf( TXT("    - multiple CFocusActionComponent") ) );
						}
						else if ( focusComponents.Size() == 1 )
						{
							CFocusActionComponent* focusComponent = focusComponents[ 0 ];
							if ( focusComponent )
							{
								CName correctActionName;
								if ( GetActionNameForEntityTemplate( entityTemplate, correctActionName ) )
								{
									if ( focusComponent->GetActionName() != correctActionName )
									{
										infoLines.PushBack( String::Printf( TXT("    - incorrect actionName property ('%s' instead of '%s')"), focusComponent->GetActionName().AsChar(), correctActionName.AsChar() ) );

										if ( action == ACTION_UPDATE_COMPONENT_WHERE_NOT_CORRECT )
										{
											if ( CanBeModified( res ) && ModifyEntityTemplate( true, entityTemplate, correctActionName ) )
											{
												templatesSaved++;
												infoLines.PushBack( String::Printf( TXT("    - updated component and saved") ) );
											}
											else
											{
												templatesNotSaved++;
												infoLines.PushBack( String::Printf( TXT("    - cannot update component or save") ) );
											}
										}
									}
								}
								else
								{
									infoLines.PushBack( String::Printf( TXT("    - no actionName defined for class '%s'"), entityTemplate->GetEntityClassName().AsChar() ) );
								}
							}
						}
						else if ( focusComponents.Size() == 0 )
						{
							CName correctActionName;
							Bool actionFound = GetActionNameForEntityTemplate( entityTemplate, correctActionName );

							infoLines.PushBack( String::Printf( TXT("    - missing CFocusActionComponent with actionName '%s'"), correctActionName.AsChar() ) );

							if ( action == ACTION_ADD_COMPONENT_WHERE_MISSING )
							{
								if ( actionFound )
								{
									if ( CanBeModified( res ) && ModifyEntityTemplate( true, entityTemplate, correctActionName ) )
									{
										templatesSaved++;
										infoLines.PushBack( String::Printf( TXT("    - added component and saved") ) );
									}
									else
									{
										templatesNotSaved++;
										infoLines.PushBack( String::Printf( TXT("    - cannot add component or save") ) );
									}
								}
								else
								{
									infoLines.PushBack( String::Printf( TXT("    - no actionName defined for class '%s'"), entityTemplate->GetEntityClassName().AsChar() ) );
								}
							}
						}
					}
				}
				break;
			case ACTION_PRINT_ONLY_ABOUT_MISSING_LIGHT_CHANNELS:
				{
					if ( !HasLightChannelsEnabled( templateEntity ) )
					{
						CName correctActionName;
						if ( GetActionNameForEntityTemplate( entityTemplate, correctActionName ) )
						{
							templatesWithMissingLightChannel++;
							infoLines.PushBack( String::Printf( TXT("    - no light channel enabled, but possibly should have - actionName '%s'"), correctActionName.AsChar() ) );
						}
					}
				}
				break;
			case ACTION_REMOVE_COMPONENTS:
				{
					static TDynArray< CFocusActionComponent* > focusComponents;
					focusComponents.Clear();
					for ( ComponentIterator< CFocusActionComponent > it( templateEntity ); it; ++it )
					{
						focusComponents.PushBack( *it );
					}

					if ( focusComponents.Size() > 0 )
					{
						infoLines.PushBack( String::Printf( TXT("[%s]   [%s]"), entityTemplate->GetEntityClassName().AsChar(), entityTemplate->GetDepotPath().AsChar() ) );

						if ( CanBeModified( res ) && ModifyEntityTemplate( false, entityTemplate, CName::NONE ) )
						{
							templatesSaved++;
							infoLines.PushBack( String::Printf( TXT("    - removed components and saved") ) );
						}
						else
						{
							templatesNotSaved++;
							infoLines.PushBack( String::Printf( TXT("    - cannot remove component or save") ) );
						}
					}
				}
				break;
		}

		if ( extendedInfo || infoLines.Size() > 1 )
		{
			for ( Uint32 i = 0; i < infoLines.Size(); ++i )
			{
				RED_LOG( RED_LOG_CHANNEL( FocusEntities ), infoLines[ i ].AsChar() );
			}
		}
	}

	RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("Templates processed:                  %d"), resources.Size() );
	if ( action == ACTION_PRINT_ONLY_ABOUT_MISSING_OR_INCORRECT_COMPONENTS || 
		 action == ACTION_ADD_COMPONENT_WHERE_MISSING || 
		 action == ACTION_UPDATE_COMPONENT_WHERE_NOT_CORRECT )
	{
		RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("Templates with light channel:         %d"), templatesWithLightChannel );
	}
	if ( action == ACTION_PRINT_ONLY_ABOUT_MISSING_LIGHT_CHANNELS )
	{
		RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("Templates with missing light channel: %d"), templatesWithMissingLightChannel );
	}
	RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("Templates saved:                      %d"), templatesSaved );
	RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("Templates NOT saved due to errors:    %d"), templatesNotSaved );

}

Bool CUpdateFocusEntityTemplatesDlg::CanBeModified( CResource* res )
{
	if ( !res )
	{
		return false;
	}
	CDiskFile* file = res->GetFile();
	if ( !file )
	{
		return false;
	}

	if ( file->IsNotSynced() )
	{
		RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("    - cannot resave, file is not synced") );
		return false;
	}
	else if ( file->IsLocal() )
	{
		return true;
	}
	else if ( !file->IsCheckedOut() )
	{
		if ( !file->SilentCheckOut( true ) )
		{
			RED_LOG( RED_LOG_CHANNEL( FocusEntities ), TXT("    - cannot check out") );
			return false;
		}
	}
	return file->IsCheckedOut();
}

Bool CUpdateFocusEntityTemplatesDlg::HasLightChannelsEnabled( const CEntity* entity )
{
	for ( ComponentIterator< CDrawableComponent > it( entity ); it; ++it )
	{
		CDrawableComponent* drawableComponent = *it;
		if ( drawableComponent && drawableComponent->AreLightChannelsEnabled( LC_Interactive | LC_Custom0 ) )
		{
			return true;
		}
	}
	return false;
}

Bool CUpdateFocusEntityTemplatesDlg::GetActionNameForEntityTemplate( const CEntityTemplate* entityTemplate, CName& actionName )
{
	const CName& entityClassName = entityTemplate->GetEntityClassName();

	const CClass* entityClass = SRTTI::GetInstance().FindClass( entityClassName );
	if ( !entityClass )
	{
		return false;
	}

	CEntity* templateEntity = entityTemplate->GetEntityObject();
	if ( !templateEntity )
	{
		return false;
	}

	for ( Uint32 i = 0; i < m_definitions.Size(); ++i )
	{
		const SFocusEntityDefinition& def = m_definitions[ i ];

		if ( def.m_entityClass )
		{
			if ( entityClass->IsA( def.m_entityClass ) )
			{
				actionName = def.m_actionName;
				return true;
			}
		}
		else if ( !def.m_includedComponent.Empty() )
		{
			Bool componentFound = false;
			for ( ComponentIterator< CComponent > it( templateEntity ); it; ++it )
			{
				CComponent* component = *it;
				if ( component && component->GetClass()->GetName() == def.m_includedComponent )
				{
					componentFound = true;
					break;
				}
			}
			if ( componentFound )
			{
				Bool classExceptionFound = false;
				for ( Uint32 j = 0; j < def.m_exceptClasses.Size(); ++j )
				{
					if ( entityClass->IsA( def.m_exceptClasses[ j ] ) )
					{
						classExceptionFound = true;
						break;
					}
				}
				if ( classExceptionFound )
				{
					continue;
				}
				actionName = def.m_actionName;
				return true;
			}
		}
	}

	return false;
}

Bool CUpdateFocusEntityTemplatesDlg::ModifyEntityTemplate( bool addOrUpdate, CEntityTemplate* entityTemplate, const CName& correctActionName )
{
	CEntity* entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
	if ( !entity )
	{
		return false;
	}
	entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );

	static TDynArray< CFocusActionComponent* > focusComponents;
	focusComponents.ClearFast();
	for ( ComponentIterator< CFocusActionComponent > it( entity ); it; ++it )
	{
		focusComponents.PushBack( *it );
	}

	if ( addOrUpdate )
	{
		if ( focusComponents.Empty() )
		{
			CClass* componentClass = SRTTI::GetInstance().FindClass( CNAME( CFocusActionComponent ) );
			if ( !componentClass )
			{
				entity->Discard();
				return false;
			}

			CFocusActionComponent* focusComponent = Cast< CFocusActionComponent >( entity->CreateComponent( componentClass, SComponentSpawnInfo() ) );
			if ( !focusComponent )
			{
				entity->Discard();
				return false;
			}
			focusComponent->SetActionName( correctActionName );
		}
		else
		{
			focusComponents[ 0 ]->SetActionName( correctActionName );
		}
	}
	else
	{
		for ( Uint32 i = 0; i < focusComponents.Size(); ++i )
		{
			entity->RemoveComponent( focusComponents[ i ] );
		}
	}

	entityTemplate->MarkModified();

	entity->PrepareEntityForTemplateSaving();
	entity->DetachTemplate();
	entityTemplate->CaptureData( entity );

	entity->Discard();

	return entityTemplate->Save();
}
