#include "build.h"
#include "meshStatsViewerFrame.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/worldIterators.h"

using namespace MeshStatsNamespace;

int CEdMeshStatsViewerTool::m_refCount = 0;

BEGIN_EVENT_TABLE( CEdMeshStatsViewerTool, wxFrame )
	EVT_BUTTON( XRCID("RefreshLoadedMeshes"), CEdMeshStatsViewerTool::OnRefresh )
	EVT_BUTTON( XRCID("SaveHtmlButton"), CEdMeshStatsViewerTool::SaveHtml )
	EVT_BUTTON( XRCID("ScrollToSelection"), CEdMeshStatsViewerTool::ScrollToSelection )
	EVT_HTML_LINK_CLICKED( XRCID("MeshesStats"),			CEdMeshStatsViewerTool::OnLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("MeshesStatsCollapsed"),	CEdMeshStatsViewerTool::OnLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("EntitiesStats"),			CEdMeshStatsViewerTool::OnLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("TexturesStats"),			CEdMeshStatsViewerTool::OnLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("AtlasesStats"),			CEdMeshStatsViewerTool::OnLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("FoliageStats"),			CEdMeshStatsViewerTool::OnLinkClicked )

	EVT_FIND( wxID_ANY, CEdMeshStatsViewerTool::OnFind )
	EVT_FIND_NEXT( wxID_ANY, CEdMeshStatsViewerTool::OnFind )
	EVT_FIND_CLOSE( wxID_ANY, CEdMeshStatsViewerTool::OnFind )

	EVT_MENU( XRCID( "Find" ), CEdMeshStatsViewerTool::OnFindOpen )

END_EVENT_TABLE()


ECmpMeshes MeshInfo::comparer1 = CmpMeshesByRefCount;
ECmpMeshes MeshInfo::comparer2 = CmpMeshesByDataSize;

ECmpEntities EntityInfo::comparer1 = CmpEntitiesByAttachedMeshes;
ECmpEntities EntityInfo::comparer2 = CmpEntitiesByName;


wxArrayString GetCmpMeshesFriendlyNames()
{
	wxArrayString result;

	result.Add( wxT("Reference count") );
	result.Add( wxT("Data Size") );
	result.Add( wxT("Chunks") );
	result.Add( wxT("Depot Path") );
	result.Add( wxT("File Name") );

	return result;
}


wxArrayString GetCmpEntitiesFriendlyNames()
{
	wxArrayString result;

	result.Add( wxT("Attached meshes") );
	result.Add( wxT("Name") );
	result.Add( wxT("Reference count") );
	result.Add( wxT("Error messages") );

	return result;
}



CEdMeshStatsViewerTool::CEdMeshStatsViewerTool( wxWindow* parent )
{
	m_refCount++;
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("MeshStatsViewerTool") );

	m_htmlViewMeshes = XRCCTRL( *this, "MeshesStats", wxHtmlWindow );
	m_htmlViewEntities = XRCCTRL( *this, "EntitiesStats", wxHtmlWindow );
	m_htmlViewMeshesCollapsed = XRCCTRL( *this, "MeshesStatsCollapsed", wxHtmlWindow );
	m_htmlViewTextures = XRCCTRL( *this, "TexturesStats", wxHtmlWindow );
	m_htmlViewAtlases = XRCCTRL( *this, "AtlasesStats", wxHtmlWindow );
	m_htmlViewFoliage = XRCCTRL( *this, "FoliageStats", wxHtmlWindow );

	m_notebook = XRCCTRL( *this, "MeshesTexturesNotebook", wxNotebook );

	m_listMeshes1 = XRCCTRL( *this, "SortMeshesChoice1", wxChoice );
	m_listMeshes2 = XRCCTRL( *this, "SortMeshesChoice2", wxChoice );

	m_listTextures1 = XRCCTRL( *this, "SortTexturesChoice1", wxChoice );
	m_listTextures2 = XRCCTRL( *this, "SortTexturesChoice2", wxChoice );

	m_listEntities1 = XRCCTRL( *this, "SortEntitiesChoice1", wxChoice );
	m_listEntities2 = XRCCTRL( *this, "SortEntitiesChoice2", wxChoice );

	m_wxFindDlg = NULL;
	m_wxFindData = NULL;

	// Fill list boxes
	m_listMeshes1->Append( GetCmpMeshesFriendlyNames() );
	m_listMeshes2->Append( GetCmpMeshesFriendlyNames() );
	m_listTextures1->Append( GetCmpTextureFriendlyNames() );
	m_listTextures2->Append( GetCmpTextureFriendlyNames() );
	m_listEntities1->Append( GetCmpEntitiesFriendlyNames() );
	m_listEntities2->Append( GetCmpEntitiesFriendlyNames() );

	m_listMeshes1->Select(0);
	m_listMeshes2->Select(1);
	m_listTextures1->Select(0);
	m_listTextures2->Select(1);
	m_listEntities1->Select(0);
	m_listEntities2->Select(1);

	// Connect list boxes
	//m_listMeshes1->Connect( wxEVT_COMMAND_CHOICE_SELECTED,   wxCommandEventHandler( CEdMeshStatsViewerTool::OnRefresh ), NULL, this );
	//m_listMeshes2->Connect( wxEVT_COMMAND_CHOICE_SELECTED,   wxCommandEventHandler( CEdMeshStatsViewerTool::OnRefresh ), NULL, this );
	//m_listTextures1->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshStatsViewerTool::OnRefresh ), NULL, this );
	//m_listTextures2->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshStatsViewerTool::OnRefresh ), NULL, this );

	RefreshHtmlControl();

	Layout();
	Show();

	CEdShortcutsEditor::Load(*this, *GetAccelerators(), TXT("MeshStatsDialog"));

	this->SetFocus();
}


void CEdMeshStatsViewerTool::OnLinkClicked( wxHtmlLinkEvent& event )
{
	// Get the link
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if( href.StartsWith( wxT("#m") ) )
	{
		m_notebook->SetSelection(2);
		Sleep(500);
		m_htmlViewMeshes->LoadPage( href );
		return;
	}

	if( href.StartsWith( wxT("#") ) )
	{
		m_notebook->SetSelection(3);
		Sleep(500);
		m_htmlViewTextures->LoadPage( href );
		return;
	}

	if( href.StartsWith( wxT("select:") ) )
	{
		String s = href.AfterFirst(':').wc_str();
		Select(s);
		return;
	}

	if( href.StartsWith( wxT("goto:") ) )
	{
		String s = href.AfterFirst(':').wc_str();
		Goto(s);
		return;
	}

	if( href.StartsWith( wxT("selectEntity:") ) )
	{
		String s = href.AfterFirst(':').wc_str();
		SelectEntity(s);
		return;
	}

	if( href.StartsWith( wxT("gotoEntity:") ) )
	{
		String s = href.AfterFirst(':').wc_str();
		GotoEntity(s);
		return;
	}

	// Select asset
	String depotPath = href.wc_str();
	SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
}

CEdMeshStatsViewerTool::~CEdMeshStatsViewerTool()
{
	m_refCount--;
}

void CEdMeshStatsViewerTool::OnRefresh( wxCommandEvent& event )
{
	MeshTextureInfo::comparer1 = (ECmpTexture)m_listTextures1->GetSelection();
	MeshTextureInfo::comparer2 = (ECmpTexture)m_listTextures2->GetSelection();

	MeshInfo::comparer1 = (ECmpMeshes)m_listMeshes1->GetSelection();
	MeshInfo::comparer2 = (ECmpMeshes)m_listMeshes2->GetSelection();

	EntityInfo::comparer1 = (ECmpEntities)m_listEntities1->GetSelection();
	EntityInfo::comparer2 = (ECmpEntities)m_listEntities2->GetSelection();

	RefreshHtmlControl();
}

void CEdMeshStatsViewerTool::RefreshHtmlControl()
{
	GFeedback->BeginTask(TXT("Enumerating all meshes"), false);

	m_potentialDuplicateEntityCount = 0;

	EnumAllMeshes();
	EnumAllFoliage();

	String output;
	String outputCollapsed;

	output +=			TXT("<table border=\"1\"><tr><td>Name</td><td>Number of occurences</td><td>Stats</td><td>Textures</td></tr>");
	outputCollapsed +=	TXT("<table border=\"1\"><tr><td>Name</td><td>Number of occurences</td><td>Stats</td></tr>");


	m_totalDifferentMeshes = 0;
	m_totalChunks = 0;
	m_totalMeshData = 0;

	m_totalDifferentMeshesOccuringOnce = 0;
	m_totalChunksOccuringOnce = 0;
	m_totalMeshDataOccuringOnce = 0;

	m_totalTextures = 0;
	m_totalTextureData = 0;

	m_totalTexturesOccuringOnce = 0;
	m_totalTextureDataOccuringOnce = 0;	

	m_totalAtlasTexturesLoaded = 0;
	m_totalAtlasTexturesUsed = 0;
	m_totalAtlasData = 0;
	m_totalAtlasTextureData = 0;

	GFeedback->UpdateTaskInfo( TXT("Generating info about each mesh") );

	for( Uint32 i = 0; i < m_allMeshes.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress(i, m_allMeshes.Size());
		GenerateMeshInfo( i, output, outputCollapsed );
	}

	output += TXT("</table><br /><br />");
	output += TXT("</body></html>");

	outputCollapsed += TXT("</table><br /><br />");
	outputCollapsed += TXT("</body></html>");

	String totalMeshStats = String::Printf(
		TXT("Total different meshes <b>%u</b><br />Total different chunks <b>%u</b><br />Total different mesh data <b>%s</b><br /><br />Total different meshes occuring once <b>%u</b><br />Total different chunks in meshes occuring once <b>%u</b><br />Total different mesh data in meshes occuring once <b>%s</b><br /><br /><br />"), 
		m_totalDifferentMeshes,
		m_totalChunks,
		MemSizeToText(m_totalMeshData).wc_str(),
		m_totalDifferentMeshesOccuringOnce,
		m_totalChunksOccuringOnce,
		MemSizeToText(m_totalMeshDataOccuringOnce).wc_str()
		);

	output = TXT("<html><body>") + totalMeshStats + output;
	outputCollapsed = TXT("<html><body>") + totalMeshStats + outputCollapsed;

	m_meshesHtml = output.AsChar();
	m_meshesCollapsedHtml = outputCollapsed.AsChar();

	outputCollapsed.Clear();
	output = TXT("<html><body>");
	output += String::Printf(TXT("Potential duplicate entities: %u<br/><br/>"), m_potentialDuplicateEntityCount);
	output += TXT("<table border=\"1\"><tr><td>Name</td><td>Reference count</td><td>Meshes</td></tr>");

	GFeedback->UpdateTaskInfo( TXT("Generating info about each entity") );

	for( Uint32 i = 0; i < m_allEntities.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress(i, m_allEntities.Size());
		GenerateEntityInfo( m_allEntities[i], output );
	}

	output += TXT("</table><br /><br />");
	output += TXT("</body></html>");

	m_entitiesHtml = output.AsChar();


	TDynArray<MeshTextureInfo*> textures;

	textures.Reserve(m_allUsedTextures.Size());
	m_allUsedTextures.GetValues( textures );
	m_allUsedTextures.Clear();
	
	qsort( textures.TypedData(), textures.Size(), sizeof( MeshTextureInfo* ), &MeshTextureInfo::DoubleCompare );
	
	output.Clear();

	output = TXT("<table border=\"1\"><tr><td>Name</td><td>Occurences</td><td>Stats</td><td>Used by meshes</td><td>Thumbnail</td></tr>");



	GFeedback->UpdateTaskInfo( TXT("Generating info about each texture") );

	for (Uint32 i = 0; i < textures.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress(i, textures.Size());
		GenerateTextureInfo( textures[i], output );
	}


	output += TXT("</table><br />");

	String totalTextureStats = String::Printf(
		TXT("Total different textures <b>%u</b><br />Total different texture data <b>%s</b><br /><br />Total different textures occuring once <b>%u</b><br />Total different data in textures occuring once <b>%s</b><br /><br />"), 
		m_totalTextures,
		MemSizeToText(m_totalTextureData).wc_str(),
		m_totalTexturesOccuringOnce,
		MemSizeToText(m_totalTextureDataOccuringOnce).wc_str()
		);


	output += TXT("<table border=\"1\"><tr><th>Cubemap name</th></tr>");
	for ( Uint32 i = 0; i < m_allCubeNames.Size(); ++i )
	{
		output += String::Printf( TXT("<tr><td valign=\"top\"><a href=\"%s\">%s</a></td></tr>"), 
			m_allCubeNames[i].AsChar(),
			m_allCubeNames[i].AsChar());
	}
	output += TXT("</table><br />");


	output += TXT("</body></html>");

	output = TXT("<html><body>") + totalTextureStats + output;

	m_texturesHtml = output.AsChar();


	output.Clear();
	
	// TODO: Foliage
	//m_foliageHtml = output.AsChar();
		
	textures.ClearPtr();
	m_allMeshes.ClearPtr();
	m_allMeshesUnsorted.Clear();
	m_allEntities.ClearPtr();
	m_textureAnchorCounter = 0;

	
	output.Clear();

	GFeedback->UpdateTaskInfo( TXT("Generating html controls, this can take long time...") );

	m_htmlViewMeshes->SetPage(m_meshesHtml);

	GFeedback->UpdateTaskProgress(1, 6);

	m_htmlViewMeshesCollapsed->SetPage(m_meshesCollapsedHtml);

	GFeedback->UpdateTaskProgress(2, 6);

	m_htmlViewTextures->SetPage(m_texturesHtml);

	GFeedback->UpdateTaskProgress(3, 6);

	m_htmlViewEntities->SetPage(m_entitiesHtml);
	
	GFeedback->UpdateTaskProgress(5, 6);

	m_htmlViewFoliage->SetPage(m_foliageHtml);

	GFeedback->UpdateTaskProgress(6, 6);

	GFeedback->EndTask();

}

void CEdMeshStatsViewerTool::EnumAllMeshes()
{
	m_allMeshes.ClearPtr();
	m_allUsedTextures.ClearPtr();
	m_textureAnchorCounter = 0;
	m_allEntities.ClearPtr();

	m_findEntriesEntities.ClearFast();
	m_findEntriesMeshes.ClearFast();
	m_findEntriesTextures.ClearFast();	


	TDynArray<CComponent*> attachedComponents;
	TDynArray<CMeshComponent*> meshComponents;
	THashMap< String, EntityInfo* > entityInfoMap;

	//GGame->GetActiveWorld()->GetAttachedComponents(attachedComponents);

	//for ( Uint32 i=0; i<attachedComponents.Size(); ++i )
	//{
	//	if ( attachedComponents[i]->IsA<CMeshComponent>() )
	//	{
	//		meshComponents.PushBackUnique( static_cast<CMeshComponent*>(attachedComponents[i]) );
	//	}
	//}

	// this should be the same as above only faster and smaller in memory
	GGame->GetActiveWorld()->GetAttachedComponentsOfClass(meshComponents);

	for ( Uint32 i=0; i < meshComponents.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress( i, meshComponents.Size() );

		Uint32 index = 0;

		if ( meshComponents[i]->GetMeshNow() )
		{

			// Gather CMesh* info
			{
				Bool found = false;

				for( Uint32 j = 0; j < m_allMeshes.Size(); ++j )
				{
					if ( m_allMeshes[j]->m_mesh == meshComponents[i]->GetMeshNow() )
					{
						m_allMeshes[j]->m_refCount++;
						found = true;
						index = j;
						ASSERT( index == m_allMeshes[j]->m_index );

						break;
					}
				}

				if ( !found )
				{
					index = m_allMeshes.Size();
					m_allMeshes.PushBack( new MeshInfo( meshComponents[i]->GetMeshNow(), index ) );
				}
			}

			// Gather CEntity* info
			if ( meshComponents[i]->GetEntity() )
			{
				EntityInfo** info = entityInfoMap.FindPtr( ( static_cast< const CEntity* >( meshComponents[i]->GetEntity() ))->GetDisplayName() );
				
				if ( info == NULL )
				{
					String errorMessage = String::EMPTY;

					Vector currentPos = meshComponents[i]->GetWorldPosition();
					CEntity* currentEntity = (CEntity*)meshComponents[i]->GetEntity();

					for ( Uint32 j=i+1; j < meshComponents.Size(); ++j )
					{
						Vector checkPos = meshComponents[j]->GetWorldPosition();

						if ( (currentPos - checkPos).Mag3() < 0.001 && meshComponents[i]->GetMeshNow() == meshComponents[j]->GetMeshNow() ) 
						{
							CEntity* checkEntity = (CEntity*)meshComponents[j]->GetEntity();
							if (checkEntity != currentEntity && checkEntity->GetEntityTemplate() == currentEntity->GetEntityTemplate() )
							{
								errorMessage = String::Printf(TXT("Same type of entity in same position, it may be duplicated"));
								m_potentialDuplicateEntityCount++;
								break;
							}
						}
					}

					entityInfoMap.Insert(( static_cast< const CEntity* >( meshComponents[i]->GetEntity() ))->GetDisplayName(), new EntityInfo( static_cast< const CEntity* >( meshComponents[i]->GetEntity() ), index, errorMessage ) );
				}
				else
				{
					if ( (*info)->m_entity != ( static_cast< const CEntity* >( meshComponents[i]->GetEntity() ) ) )
					{
						(*info)->m_refCount++;
					}

					(*info)->m_usedMeshes.PushBackUnique( index );
				}

			}


		}
	}

	m_allMeshesUnsorted = m_allMeshes;
	entityInfoMap.GetValues( m_allEntities );

	qsort( m_allMeshes.TypedData(), m_allMeshes.Size(), sizeof( MeshInfo* ), &MeshInfo::DoubleCompare );
	qsort( m_allEntities.TypedData(), m_allEntities.Size(), sizeof( EntityInfo* ), &EntityInfo::DoubleCompare );
}

void CEdMeshStatsViewerTool::EnumAllFoliage()
{
	// TODO
}

void CEdMeshStatsViewerTool::GenerateMeshInfo( const Int32 index, String &outputFull, String &outputCollapsed )
{
	CMesh* mesh = m_allMeshes[index]->m_mesh;
	Uint32 occurences = m_allMeshes[index]->m_refCount;

	Uint32 chunks = mesh->CountLODChunks(0);
	Uint32 meshRenderData = m_allMeshes[index]->m_dataSize;

	Uint32 unsortedIndex = m_allMeshes[index]->m_index;

	m_totalDifferentMeshes++;
	m_totalChunks += chunks;
	m_totalMeshData += meshRenderData;

	if ( occurences == 1 )
	{
		m_totalDifferentMeshesOccuringOnce++;
		m_totalChunksOccuringOnce += chunks;
		m_totalMeshDataOccuringOnce += meshRenderData;
	}

	String meshNameToDisplay = TXT("Unnamed mesh");

	if ( mesh->GetFile() )
	{
		meshNameToDisplay = mesh->GetFile()->GetDepotPath();
	}

	// Gather used cubes
	GatherCubeNamesUsedByMesh( mesh, m_allCubeNames );

	// Gather used textures
	TDynArray< MeshTextureInfo* > usedTextures;
	GatherTexturesUsedByMesh( mesh, usedTextures );

	// Analyze textures
	Uint32 textureDataSize = 0;

	String usedTexturesString;

	for ( Uint32 i=0; i<usedTextures.Size(); i++ )
	{
		// Accumulate shit
		MeshTextureInfo* texInfo = usedTextures[i];

		// Find if info already exists in all textures map
		MeshTextureInfo* texInfoInMap;
		if ( m_allUsedTextures.Find(texInfo->m_texture->GetFile()->GetDepotPath(), texInfoInMap) )
		{
			texInfoInMap->m_refCount++;
			texInfoInMap->m_isUsedByMeshes.PushBack( unsortedIndex );
			texInfo->m_anchorCounter = texInfoInMap->m_anchorCounter;
		}
		else
		{
			texInfo->m_anchorCounter = m_textureAnchorCounter++;
			texInfo->m_isUsedByMeshes.PushBack( unsortedIndex );
			m_allUsedTextures.Insert(texInfo->m_texture->GetFile()->GetDepotPath(), texInfo);
		}
		usedTexturesString += String::Printf( TXT("<a href=\"#%u\">%s</a><br />"), texInfo->m_anchorCounter, texInfo->m_texture->GetFile()->GetDepotPath().AsChar());


		textureDataSize += texInfo->m_dataSize;
	}
	
	outputFull += String::Printf( TXT("<tr><td valign=\"top\"><font size=\"small\"><a name=\"m%u\" /><a name=\"f%u\" /><br /><a href=\"%s\">%s</a>"), 
		unsortedIndex,
		m_findEntriesMeshes.Size(),
		meshNameToDisplay.AsChar(), 
		meshNameToDisplay.AsChar()
		);
	outputFull += String::Printf( TXT("<br /><br /><a href=\"select:%s\">Select all</a><br /><a href=\"goto:%s\">Go to first</a><br /></font></td><td valign=\"top\">%u</td><td valign=\"top\">Chunks: %u<br />"), 
		meshNameToDisplay.AsChar(),
		meshNameToDisplay.AsChar(),
		occurences, 
		chunks );
	outputFull += String::Printf( TXT("Mesh data: %s<br />"), MemSizeToText( meshRenderData ).wc_str());
	outputFull += String::Printf( TXT("Texture data: %s<br />"), MemSizeToText( textureDataSize ).wc_str());
	outputFull += String::Printf( TXT("</td><td valign=\"top\">") );
	outputFull += usedTexturesString;
	outputFull += String::Printf( TXT("</td></tr>") );


	outputCollapsed += String::Printf( TXT("<tr><td valign=\"top\"><font size=\"small\"><a name=\"f%u\" /><a name=\"%s\" /><br /><a href=\"#m%u\">%s</a>"), 
		m_findEntriesMeshes.Size(),
		meshNameToDisplay.AsChar(),
		unsortedIndex, 
		meshNameToDisplay.AsChar() 
		);

	outputCollapsed += String::Printf( TXT("<br /><br /><br /><a href=\"select:%s\">Select all</a><br /><a href=\"goto:%s\">Go to first</a><br /></font></td>"), 
		meshNameToDisplay.AsChar(),
		meshNameToDisplay.AsChar() );

	outputCollapsed += String::Printf( TXT("<td valign=\"top\">%u</td><td valign=\"top\">Chunks: %u<br />"), occurences, chunks );
	outputCollapsed += String::Printf( TXT("Mesh data: %s<br />"), MemSizeToText( meshRenderData ).wc_str());
	outputCollapsed += String::Printf( TXT("Texture data: %s<br /></td></tr>"), MemSizeToText( textureDataSize ).wc_str());
	
	meshNameToDisplay.MakeLower();
	m_findEntriesMeshes.PushBack( meshNameToDisplay );

}

void CEdMeshStatsViewerTool::GenerateTextureInfo( MeshTextureInfo* texInfo, String &output )
{
	texInfo->m_thumbnailFile = ExtractTextureThumbnail( texInfo->m_texture );

	String dimensionInfo = String::Printf( TXT("%u x %u<br />%u mipmap(s)"), texInfo->m_texture->GetWidth(), texInfo->m_texture->GetHeight(), texInfo->m_texture->GetMipCount() );

	String usedByMeshes;

	m_totalTextures++;
	m_totalTextureData += texInfo->m_dataSize;

	if ( texInfo->m_refCount == 1 )
	{
		m_totalTexturesOccuringOnce++;
		m_totalTextureDataOccuringOnce += texInfo->m_dataSize;
	}

	for( Uint32 i = 0; i < texInfo->m_isUsedByMeshes.Size(); ++i )
	{
		//String meshPath = m_allMeshes[texInfo->m_isUsedByMeshes[i]]->m_mesh->GetFile()->GetDepotPath();
		String meshFileName = m_allMeshesUnsorted[texInfo->m_isUsedByMeshes[i]]->m_mesh->GetFile()->GetFileName();
		usedByMeshes += String::Printf( TXT( "<a href=\"#m%u\">%s</a><br />" ), 
			texInfo->m_isUsedByMeshes[i], 
			meshFileName.AsChar() );
	}

	output += String::Printf( TXT("<tr><td valign=\"top\">%s<br /><a href=\"%s\">%s</a></td><td valign=\"top\">%u</td><td valign=\"top\">%s<br />%s</td><td valign=\"top\">"), 
		texInfo->m_texture->GetTextureGroupName().AsString().AsChar(),
		texInfo->m_texture->GetFile()->GetDepotPath().AsChar(),
		texInfo->m_texture->GetFile()->GetDepotPath().AsChar(),
		texInfo->m_refCount,
		MemSizeToText( texInfo->m_dataSize ).wc_str(),
		dimensionInfo.AsChar());
	
	output += usedByMeshes.AsChar();
		
	output+= String::Printf( TXT("</td><td valign=\"top\"><a name=\"f%u\" /><a name=\"%u\" /><br /><img src=\"%s\"></td></tr>"),
		m_findEntriesTextures.Size(),
		texInfo->m_anchorCounter,
		texInfo->m_thumbnailFile.wc_str()
		);

	String entry = texInfo->m_texture->GetTextureGroupName().AsString() + texInfo->m_texture->GetFile()->GetDepotPath();
	entry.MakeLower();
	m_findEntriesTextures.PushBack( entry );

}

void CEdMeshStatsViewerTool::Select( String s )
{
	CSelectionManager *selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction(*selectionManager);
	selectionManager->DeselectAll();

	for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CMeshComponent *comp = Cast< CMeshComponent > ( *it );
		if ( comp && comp->GetMeshNow() && comp->GetMeshNow()->GetFile() && comp->GetMeshNow()->GetFile()->GetDepotPath() == s )
		{
			selectionManager->Select( comp );
		}
	}

}

void CEdMeshStatsViewerTool::Goto( String s )
{
	for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CMeshComponent *comp = Cast< CMeshComponent > ( *it );
		if ( comp && comp->GetMeshNow() && comp->GetMeshNow()->GetFile() && comp->GetMeshNow()->GetFile()->GetDepotPath() == s )
		{
			wxTheFrame->GetWorldEditPanel()->LookAtNode( comp );
			return;
		}
	}
}

void CEdMeshStatsViewerTool::GenerateEntityInfo( EntityInfo* info, String &output )
{
	String usedMeshes;

	for( Uint32 i = 0; i < info->m_usedMeshes.Size(); ++i )
	{
		String meshFileName = m_allMeshesUnsorted[info->m_usedMeshes[i]]->m_mesh->GetFile()->GetFileName();
		usedMeshes += String::Printf( TXT( "<a href=\"#m%u\">%s</a><br />" ), 
			info->m_usedMeshes[i],  
			meshFileName.AsChar() );
	}

	String depotPath = info->m_entity->GetEntityTemplate()?info->m_entity->GetEntityTemplate()->GetFile()->GetDepotPath():String();
	String errorColor = !info->m_errorMessage.Empty()?TXT("bgcolor=\"#FF0000\""):TXT("");

	output += String::Printf( TXT("<tr %s><td valign=\"top\"><a name=\"f%u\" /><a name=\"%s\" /><br />%s<br /><a href=\"%s\">%s</a><br /><br />"), 
		errorColor.AsChar(),
		m_findEntriesEntities.Size(),
		info->m_entity->GetDisplayName().AsChar(),
		info->m_entity->GetDisplayName().AsChar(),
		depotPath.AsChar(),
		depotPath.AsChar()
		);

	output += String::Printf( TXT("<a href=\"selectEntity:%s\">Select</a><br /><a href=\"gotoEntity:%s\">Goto</a><br />%s</td><td valign=\"top\">%u</td><td valign=\"top\">"),
		info->m_entity->GetDisplayName().AsChar(),
		info->m_entity->GetDisplayName().AsChar(),
		info->m_errorMessage.AsChar(),
		info->m_refCount
		);

	output += usedMeshes;
	output += TXT("<br /></td></tr>");

	depotPath += info->m_entity->GetDisplayName();
	depotPath.MakeLower();

	m_findEntriesEntities.PushBack( depotPath );
}

void CEdMeshStatsViewerTool::SelectEntity( String s )
{
	CSelectionManager *selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction(*selectionManager);
	selectionManager->DeselectAll();

	for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CMeshComponent *comp = Cast< CMeshComponent > ( *it );
		if ( comp && comp->GetEntity() && comp->GetEntity()->GetDisplayName() == s )
		{
			selectionManager->Select( comp );
		}
	}
}

void CEdMeshStatsViewerTool::GotoEntity( String s )
{
	for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CMeshComponent *comp = Cast< CMeshComponent > ( *it );
		if ( comp && comp->GetEntity() && comp->GetEntity()->GetDisplayName() == s )
		{
			wxTheFrame->GetWorldEditPanel()->LookAtNode( comp );
			return;
		}
	}
}

void CEdMeshStatsViewerTool::SaveHtml( wxCommandEvent& event )
{
	wxFileDialog* saveDialog = new wxFileDialog(
		this, _("Choose a file to save"), wxEmptyString, wxEmptyString, 
		TXT("Html files (*.html)|*.html"),
		wxFD_SAVE, wxDefaultPosition);

	if ( saveDialog->ShowModal() == wxID_OK )
	{
		wxString fileName = saveDialog->GetPath();
		int page = m_notebook->GetSelection();

		wxString textToSave;
		
		switch ( page )
		{
		case 0:
			textToSave = m_entitiesHtml;
			break;
		case 1:
			textToSave = m_meshesCollapsedHtml;
			break;
		case 2:
			textToSave = m_meshesHtml;
			break;
		case 3:
			textToSave = m_texturesHtml;
			break;
		}

		wxFile file;
		file.Open( fileName.wc_str(), wxFile::write );
		file.Write( textToSave );
		file.Close();

	}

	saveDialog->Destroy();
}

void CEdMeshStatsViewerTool::ScrollToSelection( wxCommandEvent& event )
{
	CSelectionManager *selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
	TDynArray< CComponent* > components;
	selectionManager->GetSelectedComponentsFiltered( ClassID<CMeshComponent>(), components );


	int page = m_notebook->GetSelection();

	switch ( page )
	{
	case 0:
		{
			for( Uint32 i = 0; i < components.Size(); ++i )
			{
				if ( static_cast<CMeshComponent*>( components[i] )->GetEntity() )
				{
					String anchorName = TXT("#") + static_cast<CMeshComponent*>( components[i] )->GetEntity()->GetDisplayName();
					m_htmlViewEntities->LoadPage( anchorName.AsChar() );
					break;
				}
			}

		}
		break;

	case 2:
		m_notebook->SetSelection(1);
	case 1:
		{
			for( Uint32 i = 0; i < components.Size(); ++i )
			{
				if ( static_cast<CMeshComponent*>( components[i] )->GetMeshNow() && static_cast<CMeshComponent*>( components[i] )->GetMeshNow()->GetFile() )
				{
					String anchorName = TXT("#") + static_cast<CMeshComponent*>( components[i] )->GetMeshNow()->GetFile()->GetDepotPath();
					m_htmlViewMeshesCollapsed->LoadPage( anchorName.AsChar() );
					break;
				}
			}

		}
		
		break;

	case 3:
		break;
	}
}

TEdShortcutArray* CEdMeshStatsViewerTool::GetAccelerators()
{
	if (m_shortcuts.Empty())
	{
		m_shortcuts.PushBack(SEdShortcut(TXT("MeshStatsDialog\\Find"),wxAcceleratorEntry(wxACCEL_CTRL,'F', XRCID( "Find" ))) );
	}

	return &m_shortcuts;
}

void CEdMeshStatsViewerTool::OnFindOpen( wxCommandEvent& event )
{
	if ( m_wxFindDlg )
	{
		delete m_wxFindDlg;
		delete m_wxFindData;
		m_wxFindDlg = NULL;
		m_wxFindData = NULL;
	}
	else
	{
		m_wxFindData = new wxFindReplaceData( wxFR_NOMATCHCASE | wxFR_NOWHOLEWORD );
		m_wxFindDlg = new wxFindReplaceDialog( this, m_wxFindData, TXT("Find"), wxFR_NOMATCHCASE | wxFR_NOWHOLEWORD );
		m_wxFindDlg->Show();
		m_findPageIndex = m_notebook->GetSelection();
		m_findLastIndex = 0;
	}
}

void CEdMeshStatsViewerTool::OnFind( wxFindDialogEvent& event )
{
	wxEventType type = event.GetEventType();

	if ( type == wxEVT_COMMAND_FIND || type == wxEVT_COMMAND_FIND_NEXT )
	{
		String findString = m_wxFindData->GetFindString().wc_str();
		findString.MakeLower();

		TDynArray<String> *findArray = NULL;

		switch ( m_findPageIndex )
		{
		case 0:
			findArray = &m_findEntriesEntities;
			break;
		case 1:
		case 2:
			findArray = &m_findEntriesMeshes;
			break;
		case 3:
			findArray = &m_findEntriesTextures;
			break;
		}

		Uint32 foundIndex = findArray->Size();

		if ( m_wxFindData->GetFlags() & wxFR_DOWN )
		{
			for ( Uint32 i = m_findLastIndex; i < findArray->Size(); ++i )
			{
				if ( (*findArray)[i].ContainsSubstring( findString ) )
				{
					foundIndex = i;
					m_findLastIndex = foundIndex + 1;
					break;
				}
			}
		}
		else
		{
			if ( m_findLastIndex == findArray->Size() )
			{
				--m_findLastIndex;
			}

			for ( Uint32 i = m_findLastIndex + 1; i > 0; --i )
			{
				if ( (*findArray)[i - 1].ContainsSubstring( findString ) )
				{
					foundIndex = i - 1;
					m_findLastIndex = foundIndex - 1;
					break;
				}
			}
		}

		if ( foundIndex != findArray->Size() )
		{
			String location = String::Printf( TXT("#f%u"), foundIndex );

			switch ( m_findPageIndex )
			{
			case 0:
				m_htmlViewEntities->LoadPage( location.AsChar() );
				break;
			case 1:
				m_htmlViewMeshesCollapsed->LoadPage( location.AsChar() );
				break;
			case 2:
				m_htmlViewMeshes->LoadPage( location.AsChar() );
				break;
			case 3:
				m_htmlViewTextures->LoadPage( location.AsChar() );
				break;
			}

			m_notebook->SetSelection( m_findPageIndex );
		}
		else
		{
			wxMessageBox( TXT( "Not found" ), TXT( "Not found" ) );
		}


	}
	else if ( type == wxEVT_COMMAND_FIND_CLOSE )
	{
		delete m_wxFindDlg;
		delete m_wxFindData;
		m_wxFindDlg = NULL;
		m_wxFindData = NULL;
	}
}
