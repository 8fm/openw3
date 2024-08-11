/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "lodAssigner.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/apexResource.h"
#include "../../common/engine/meshTypeResource.h"


// Check if the given path has an extension used by a supported resource.
static Bool IsPathSupportedType( const String& path )
{
	return path.EndsWith( ResourceExtension< CMesh >() ) || path.EndsWith( ResourceExtension< CApexResource >() );
}


// Event table
BEGIN_EVENT_TABLE( CLODAssigner, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("Assign"), CLODAssigner::OnAssign )
	EVT_CHECKBOX( XRCID("lodModify0"), CLODAssigner::OnModifyChanged )
	EVT_CHECKBOX( XRCID("lodModify1"), CLODAssigner::OnModifyChanged )
	EVT_CHECKBOX( XRCID("lodModify2"), CLODAssigner::OnModifyChanged )
	EVT_CHECKBOX( XRCID("lodModify3"), CLODAssigner::OnModifyChanged )
END_EVENT_TABLE()

CLODAssigner::CLODAssigner( wxWindow* parent, CDirectory* dir )
: wxSmartLayoutPanel( parent, TXT("LODAssigner"), false )
, m_directory( dir )
{
	// Set title
	String title = GetTitle().wc_str();
	SetTitle( String::Printf( TEXT("%s - %s"), title.AsChar(), dir->GetDepotPath().AsChar() ).AsChar() );

	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );
}

CLODAssigner::~CLODAssigner()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CLODAssigner::OnModifyChanged( wxCommandEvent& event )
{
	if ( event.GetId() == XRCID("lodModify0") )
	{
		if ( XRCCTRL( *this, "lodModify0", wxCheckBox )->GetValue() )
		{
			XRCCTRL( *this, "distPC0", wxTextCtrl )->SetValue( wxT( "0.0" ) );
		}
		else
		{
			XRCCTRL( *this, "distPC0", wxTextCtrl )->SetValue( wxT( "" ) );
		}
	}
	else if ( event.GetId() == XRCID("lodModify1") )
	{
		if ( XRCCTRL( *this, "lodModify1", wxCheckBox )->GetValue() )
		{
			XRCCTRL( *this, "distPC1", wxTextCtrl )->SetValue( wxT( "0.0" ) );
		}
		else
		{
			XRCCTRL( *this, "distPC1", wxTextCtrl )->SetValue( wxT( "" ) );
		}
	}
	else if ( event.GetId() == XRCID("lodModify2") )
	{
		if ( XRCCTRL( *this, "lodModify2", wxCheckBox )->GetValue() )
		{
			XRCCTRL( *this, "distPC2", wxTextCtrl )->SetValue( wxT( "0.0" ) );
		}
		else
		{
			XRCCTRL( *this, "distPC2", wxTextCtrl )->SetValue( wxT( "" ) );
		}
	}
	else if ( event.GetId() == XRCID("lodModify3") )
	{
		if ( XRCCTRL( *this, "lodModify3", wxCheckBox )->GetValue() )
		{
			XRCCTRL( *this, "distPC3", wxTextCtrl )->SetValue( wxT( "0.0" ) );
		}
		else
		{
			XRCCTRL( *this, "distPC3", wxTextCtrl )->SetValue( wxT( "" ) );
		}
	}	
}

void CLODAssigner::OnAssign( wxCommandEvent& event )
{
	Bool processSubdirectories = XRCCTRL( *this, "ProcessSubdirs", wxCheckBox )->GetValue();

	Uint32 max = 0;
	CountMeshes( m_directory, processSubdirectories, max );

	if ( max <= 0 )
	{
		return;
	}

	XRCCTRL( *this, "ProgressBar", wxGauge )->SetValue( 0 );
	XRCCTRL( *this, "ProgressBar", wxGauge )->SetRange( max );

	TDynArray< CLODAssigner::LODData > lodLevels;
	CollectLODData( lodLevels );
	ASSERT( lodLevels.Size() == 4 );

	/*
	for ( Uint32 i = 0; i < 4; ++i )
	{
		LOG_ALWAYS( TXT( "[%d] modified [%d]"), i, lodLevels[ i ].m_modify );
		LOG_ALWAYS( TXT( "  PC used [%d]"), lodLevels[i].m_lodLevel.m_useOnPC );
		LOG_ALWAYS( TXT( "  PC distance [%1.2f]"), lodLevels[i].m_lodLevel.m_distancePC );
		LOG_ALWAYS( TXT( "  Xbox used [%d]"), lodLevels[i].m_lodLevel.m_useOnXenon );
		LOG_ALWAYS( TXT( "  Xbox distance [%1.2f]"), lodLevels[i].m_lodLevel.m_distanceXenon );
	}
	*/

	Uint32 processedMeshes = 0;
	LogDir( m_directory, 0, lodLevels, processSubdirectories, processedMeshes, max );

	wxMessageBox( wxT( "Done." ) );
	this->Close();
}

void CLODAssigner::CollectLODData( TDynArray< LODData >& data )
{
	char controlName[20];
	for ( Uint32 i = 0; i < 4; ++i )
	{
		CLODAssigner::LODData lodData;

		Red::System::SNPrintF( controlName, ARRAY_COUNT( controlName ), "lodModify%d", i );
		lodData.m_modify = XRCCTRL( *this, controlName, wxCheckBox )->GetValue();

		if ( lodData.m_modify )
		{
			// fill LOD data with actual data
			Red::System::SNPrintF( controlName, ARRAY_COUNT( controlName ), "distPC%d", i );
			Double distance;
			wxString str = XRCCTRL( *this, controlName, wxTextCtrl )->GetValue();
			if ( !str.IsEmpty() )
			{
				VERIFY( str.ToDouble( &distance ), TXT("Distance invalid, try using dot instead of comma.") );
				lodData.m_lodLevel.m_distance = distance;
			}
		}
		data.PushBack( lodData );
	}
}

void CLODAssigner::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
}

void CLODAssigner::LogDir( CDirectory* dir, Uint32 level, const TDynArray< LODData >& lodData, Bool processSubdirectories, Uint32& processedMeshes, const Uint32& max )
{
	ListMeshes( dir, lodData, processedMeshes, max );

	if ( !processSubdirectories )
	{
		return;
	}
	
	for ( CDirectory* child : dir->GetDirectories() )
	{
		LogDir( child, level + 1, lodData, processSubdirectories, processedMeshes, max );
	}
}

void CLODAssigner::ListMeshes( CDirectory* dir, const TDynArray< LODData >& lodData, Uint32& processedMeshes, const Uint32& max )
{
	ASSERT( dir );
	const TFiles& files = dir->GetFiles();

	for( TFiles::const_iterator iter = files.Begin(); iter != files.End(); ++iter )
	{
		CDiskFile* diskFile = const_cast< CDiskFile* >(*iter);
		const String path = diskFile->GetDepotPath();
		if ( !IsPathSupportedType( path ) )
		{
			continue;
		}
		
		if ( diskFile )
		{
			if ( !diskFile->IsLoaded() )
			{
				if ( !diskFile->Load() )
				{
					LOG_EDITOR( TXT( "  Loading resource [%s] not successful." ), path );
					continue;
				}
			}

			CResource* resource = diskFile->GetResource();
			if ( !resource )
			{
				continue;
			}
			if ( !resource->IsA< CMeshTypeResource >() )
			{
				continue;
			}

			++processedMeshes;
			if ( !diskFile->IsCheckedOut() )
			{
				if ( !diskFile->CheckOut() )
				{
					WARN_EDITOR( TXT( "LOD Assigner - could not checkout mesh file: [%s]" ), path.AsChar() );
					continue;
				}
			}

			ASSERT( diskFile->IsCheckedOut() );
			UpdateMeshLODs( static_cast< CMeshTypeResource* >( resource ), lodData );
			diskFile->Save();
		}
	}
	XRCCTRL( *this, "ProgressBar", wxGauge )->SetValue( processedMeshes );
	XRCCTRL( *this, "ProgressBar", wxGauge )->Refresh();
	XRCCTRL( *this, "ProgressBar", wxGauge )->Update();
	Sleep( 100 );
}

void CLODAssigner::CountMeshes( CDirectory* dir, Bool recursive, Uint32& count )
{
	ASSERT( dir );
	const TFiles& files = dir->GetFiles();
	for( TFiles::const_iterator iter = files.Begin(); iter != files.End(); ++iter )
	{
		CDiskFile* diskFile = const_cast< CDiskFile* >(*iter);
		const String path = diskFile->GetDepotPath();
		
		if ( IsPathSupportedType( path ) )
		{
			++count;
		}
	}

	if ( !recursive )
	{
		return;
	}

	for ( CDirectory* child : dir->GetDirectories() )
	{
		CountMeshes( child, recursive, count );
	}
}

void CLODAssigner::UpdateMeshLODs( CMeshTypeResource* mesh, const TDynArray< LODData >& lods )
{
	Uint32 lodLevelsCount = mesh->GetNumLODLevels();

	for ( Uint32 i = 0; i < lodLevelsCount; ++i )
	{
		if ( lods[ i ].m_modify )
		{
			SMeshTypeResourceLODLevel level = lods[ i ].m_lodLevel;
			mesh->UpdateLODSettings( i, level );
		}
	}
}
