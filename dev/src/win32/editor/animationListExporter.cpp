
#include "build.h"
#include "animationListExporter.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

CEdAnimationListExporter::CEdAnimationListExporter( wxWindow* parent )
	: m_parent( parent )
{

}

CEdAnimationListExporter::~CEdAnimationListExporter()
{

}

void CEdAnimationListExporter::ExportToCSV()
{
	// TODO Merge all GFeedback
	GFeedback->BeginTask( TXT("Export list of animations"), false );
	GFeedback->UpdateTaskInfo( TXT("Collecting all animsets...") );

	CollectAllAnimsets();

	GFeedback->UpdateTaskInfo( TXT("Loading all animsets...") );

	LoadAllAnimsets();

	GFeedback->EndTask();

	String path = GetCSVFilePath();
	if ( !path.Empty() )
	{
		WriteToCSV( path );
	}
}

void CEdAnimationListExporter::CollectAllAnimsets()
{
	//GFeedback->BeginTask( TXT("Collecting all animsets..."), false );

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String animsetEx = TXT("*."); 
	animsetEx += CSkeletalAnimationSet::GetFileExtension();

	TDynArray< String > depotFilesAnimsets;
	GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );

	const Uint32 size = depotFilesAnimsets.Size();
	m_items.Resize( size );

	for ( Uint32 i=0; i<size; ++i )
	{
		GDepot->ConvertToLocalPath( depotFilesAnimsets[ i ], m_items[ i ].m_path );

		//GFeedback->UpdateTaskProgress( i, size );
	}

	//GFeedback->EndTask();
}

void CEdAnimationListExporter::LoadAllAnimsets()
{
	//GFeedback->BeginTask( TXT("Loading all animsets..."), false );

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		AnimsetItem& item = m_items[ i ];

		item.m_animset = Cast< CSkeletalAnimationSet >( GDepot->LoadResource( item.m_path ) );

		//GFeedback->UpdateTaskProgress( i, size );
	}

	//GFeedback->EndTask();
}

String CEdAnimationListExporter::GetCSVFilePath() const
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CEdAnimationListExporter_CSV") );
	dlg.AddFormat( TXT("csv"), TXT( "CSV" ) );

	String filePath;

	if ( dlg.DoSave( (HWND)m_parent->GetHandle() ) )
	{				
		filePath = dlg.GetFile();
	}

	return filePath;
}

void CEdAnimationListExporter::WriteToCSV( const String& filePath )
{
	GFeedback->BeginTask( TXT("Export list of animations"), false );
	GFeedback->UpdateTaskInfo( TXT("Dumping data to file...") );

	{
		String data = TXT("Animset_name;Animation_name;Animset_path;\n");
		GFileManager->SaveStringToFile( filePath, data );
	}

	const Uint32 size = m_items.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		AnimsetItem& item = m_items[ i ];

		CSkeletalAnimationSet* animset = m_items[ i ].m_animset;
		if ( animset )
		{
			String animsetRawString;

			String dataA = animset->GetFile()->GetFileName().StringBefore( TXT("."), true );
			String dataC = animset->GetFile()->GetDepotPath();

			dataA.ReplaceAll( TXT(" "), TXT("_") );
			dataC.ReplaceAll( TXT(" "), TXT("_") );
			
			const TDynArray< CSkeletalAnimationSetEntry* >& anims = animset->GetAnimations();
			const Uint32 animsSize = anims.Size();
			for ( Uint32 j=0; j<animsSize; ++j )
			{
				CSkeletalAnimationSetEntry* animation = anims[ j ];
				if ( animation && animation->GetAnimation() )
				{
					String dataB = animation->GetName().AsString();

					dataB.ReplaceAll( TXT(" "), TXT("_") );

					animsetRawString += String::Printf( TXT("%s;%s;%s;\n"), dataA.AsChar(), dataB.AsChar(), dataC.AsChar() );
				}
			}

			if ( !animsetRawString.Empty() )
			{
				GFileManager->SaveStringToFile( filePath, animsetRawString, true );
			}
		}

		GFeedback->UpdateTaskProgress( i, size );
	}

	GFeedback->EndTask();
}
