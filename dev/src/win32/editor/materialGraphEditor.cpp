/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialGraphEditor.h"
#include "wxThumbnailImageLoader.h"
#include "../../common/matcompiler/mbMathMultiply.h"
#include "../../common/matcompiler/mbMathAdd.h"
#include "../../common/matcompiler/mbMathInterpolate.h"
#include "../../common/matcompiler/mbSamplerTexture.h"
#include "../../common/matcompiler/mbPackNormal.h"
#include "../../common/engine/mbParamScalar.h"
#include "../../common/engine/mbParamColor.h"
#include "../../common/engine/mbParamVector.h"
#include "../../common/engine/mbParamTexture.h"

#include "../../common/engine/mbParamColor.h"
#include "../../common/engine/mbParamTexture.h"
#include "../../common/engine/mbInput.h"
#include "../../common/engine/mbOutput.h"
#include "../../common/engine/mbEncapsulatedGraph.h"
#include "../../common/engine/texture.h"


CEdMaterialGraphEditor::CEdMaterialGraphEditor( wxWindow* parent )
	: CEdGraphEditor( parent, false, true )
{
	Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CEdMaterialGraphEditor::OnAKeyDown ), NULL, this );
}

void CEdMaterialGraphEditor::SetGraph( CMaterialGraph* graph )
{
	CEdGraphEditor::SetGraph( graph );
}

void CEdMaterialGraphEditor::CalcBlockInnerArea( CGraphBlock* block, wxSize& innerArea )
{
	// Color
	if ( block->IsA< CMaterialParameterColor >() )
	{
		innerArea = wxSize( 64, 32 );
	}

	// Texture block
	if ( block->IsA< CMaterialParameterTexture >() )
	{
		innerArea = wxSize( 64, 64 );
	}

	// Encapsulation blocks
	if ( block->IsA< CMaterialBlockInput >() || block->IsA< CMaterialBlockOutput >() )
	{
		innerArea = wxSize( 64, 32 );
	}
}

void CEdMaterialGraphEditor::DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect )
{
	// Constant color
	if ( block->IsA< CMaterialParameterColor >() )
	{
		CMaterialParameterColor* realBlock = SafeCast< CMaterialParameterColor >( block );

		// Draw rect with value color
		Color value = realBlock->m_color;
		wxRect colorRect( rect.x, rect.y, rect.width, 16 );
		FillRect( colorRect, wxColour( value.R, value.G, value.B ) );
		wxRect alphaRect( rect.x, rect.y+16, rect.width, 16 );
		FillRect( alphaRect, wxColour( value.A, value.A, value.A ) );

		// Done
		return;
	}

	ITexture* texture = NULL;

	// Texture block
	if ( block->IsA< CMaterialParameterTexture >() )
	{
		// Get block
		CMaterialParameterTexture* texBlock = SafeCast< CMaterialParameterTexture >( block );

		// Get bitmap resource
		texture = Cast< ITexture >( texBlock->m_texture.Get() );
	}

	if ( texture )
	{
		// Only resources loaded from file have thumbnails
		CDiskFile* file = texture->GetFile();
		if ( file )
		{
			// Preload thumbnail
			file->LoadThumbnail();

			// Draw it
			TDynArray<CThumbnail*> thumbnails = file->GetThumbnails();
			if ( thumbnails.Size() > 0 )
			{
				CWXThumbnailImage* image = (CWXThumbnailImage*)( thumbnails[0]->GetImage() );
				if ( image && image->GetBitmap() )
				{
					DrawImage( image->GetBitmap(), rect );
					DrawRect( rect, wxColour( 0,0,0 ) );
					return;
				}
			}
		}
	}

	// Fallback
	DrawRect( rect, wxColour( 255, 255, 255 ) );
}

void CEdMaterialGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
	CEdGraphEditor::InitLinkedDefaultContextMenu( menu );

	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );
	if( selection.Size() > 1 )
	{
		menu.AppendSeparator();
		wxMenuItem* menuItem = new wxMenuItem(&menu, 1111, "Encapsulate");
		menu.Append(menuItem);
		menu.Connect( 1111, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdMaterialGraphEditor::EncapsulateSelectedBlocks ), NULL, this );
	}
}

void CEdMaterialGraphEditor::EncapsulateSelectedBlocks( wxCommandEvent& event )
{
	LOG_EDITOR(TXT("Encapsulate!!!"));

	// Create special node with graph inside
	//CMaterialEncapsulatedGraph* capsule = new CMaterialEncapsulatedGraph( );

	CutSelection();
	
	Vector pastePosition = Vector::ZEROS;

	GraphBlockSpawnInfo info( CMaterialEncapsulatedGraph::GetStaticClass() );
	info.m_position = pastePosition;
	CMaterialEncapsulatedGraph* capsule = static_cast<CMaterialEncapsulatedGraph*>( m_graph->GraphCreateBlock( info ));

	// Open clipboard
	if ( wxTheClipboard->Open())
	{
		CClipboardData data( String(TXT("Blocks")) + ClipboardChannelName() );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{
			// Extract data from the clipboard
			if ( wxTheClipboard->GetData( data ) )
			{
				// Determine spawn position
				Bool relativePosition = false;
				Vector spawnPosition = Vector::ZEROS;

				// Paste entities
				TDynArray< CGraphBlock* > pastedBlocks;
				if ( capsule->GetGraph()->GraphPasteBlocks( data.GetData(), pastedBlocks, relativePosition, false, spawnPosition ) )
				{
					// Destroy current selection
					DeselectAllBlocks();

					// Select pasted blocks
					for ( Uint32 i=0; i<pastedBlocks.Size(); i++ )
					{
						if (i==0)
						{
							pastePosition = pastedBlocks[ i ]->GetPosition();
						}
						pastedBlocks[ i ]->OnPasted( data.IsCopy() );
					}
				}
			}
		}
		// Close clipboard
		wxTheClipboard->Close();
	}

	capsule->SetPosition(pastePosition);
	capsule->InvalidateLayout();
	m_graph->GraphGetBlocks().PushBack( capsule );

	GraphStructureModified();

	// create connections (sockets)
	// add selected graph nodes to new graph
	// connect nodes to sockets
	// remove selected graph nodes from original graph
	// add connections
}

//class CMaterialBlockMathMultiply;

void CEdMaterialGraphEditor::OnAKeyDown( wxKeyEvent& event )
{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	if ( event.GetKeyCode() == 'M' )
	{
		SpawnBlock( ClassID<CMaterialBlockMathMultiply>(), ScreenToClient( wxGetMousePosition() ) );
	}
	if ( event.GetKeyCode() == 'A' )
	{
		SpawnBlock( ClassID<CMaterialBlockMathAdd>(), ScreenToClient( wxGetMousePosition() ) );
	}
	if ( event.GetKeyCode() == 'I' )
	{
		SpawnBlock( ClassID<CMaterialBlockMathInterpolate>(), ScreenToClient( wxGetMousePosition() ) );
	}
	if ( event.GetKeyCode() == 'S' )
	{
		SpawnBlock( ClassID<CMaterialParameterScalar>(), ScreenToClient( wxGetMousePosition() ) );
	}
	if ( event.GetKeyCode() == 'C' )
	{
		SpawnBlock( ClassID<CMaterialParameterColor>(), ScreenToClient( wxGetMousePosition() ) );
	}
	if ( event.GetKeyCode() == 'V' )
	{
		SpawnBlock( ClassID<CMaterialParameterVector>(), ScreenToClient( wxGetMousePosition() ) );
	}
	if ( event.GetKeyCode() == 'T' )
	{
		Uint32 amountOfTSamplers = 0;
		if ( m_graph )
		{
			amountOfTSamplers = GetAmountOfBlockWithName( TXT("Texture Sampler") );
		}
		// Creates a Texture block with a texture sampler.
		CGraphBlock* texSampler = SpawnBlock( ClassID<CMaterialBlockSamplerTexture>(), ScreenToClient( wxGetMousePosition() ) );
		CGraphBlock* texParam = SpawnBlock( ClassID<CMaterialParameterTexture>(), ScreenToClient( wxGetMousePosition() + wxPoint( 160 * GetScale( ), 0 ) ) );
		
		CMaterialParameter* param = Cast< CMaterialParameter >( texParam );
		param->SetParameterName( CName( TXT("Sampler")+ToString(amountOfTSamplers+1) ) );
		
		TDynArray< CGraphSocket* > sock = texParam->GetSockets();
		//sock[0]->get
		CGraphSocket* texParamSocket = texParam->FindSocket( TXT("Texture") );
		CGraphSocket* texSamplerSocket = texSampler->FindSocket( TXT("Texture") );
		texParamSocket->ConnectTo( texSamplerSocket, true );
	}
	if ( event.GetKeyCode() == 'N' )
	{
		Uint32 amountOfTSamplers = 0;
		if ( m_graph )
		{
			amountOfTSamplers = GetAmountOfBlockWithName( TXT("Texture Sampler") );
		}
		// Creates a Texture block with a texture sampler and an unpack normals block.
		CGraphBlock* texSampler = SpawnBlock( ClassID<CMaterialBlockSamplerTexture>(), ScreenToClient( wxGetMousePosition() ) );
		CGraphBlock* texParam = SpawnBlock( ClassID<CMaterialParameterTexture>(), ScreenToClient( wxGetMousePosition() + wxPoint( 160 * GetScale( ), 0 ) ) );
		CGraphBlock* unpackNorm = SpawnBlock( ClassID<CMaterialBlockPackNormal>(), ScreenToClient( wxGetMousePosition() + wxPoint( -160 * GetScale( ), 0 )  ) );
		
		CMaterialParameter* param = Cast< CMaterialParameter >( texParam );
		param->SetParameterName( CName( TXT("Sampler")+ToString(amountOfTSamplers+1) ) );

		CGraphSocket* texParamSocket = texParam->FindSocket( TXT("Texture") );
		CGraphSocket* texSamplerSocket = texSampler->FindSocket( TXT("Texture") );
		CGraphSocket* texSamplerColorSocket = texSampler->FindSocket( TXT("Color") );
		CGraphSocket* unPacknormSocket = unpackNorm->FindSocket( TXT("In") );
		texParamSocket->ConnectTo( texSamplerSocket, true );
		texSamplerColorSocket->ConnectTo( unPacknormSocket, true );
	}
#endif // NO_RUNTIME_MATERIAL_COMPILATION
	event.Skip();
}

Uint32 CEdMaterialGraphEditor::GetAmountOfBlockWithName( String nameOfBlock )
{
	Uint32 amountOfBlocks = 0;
	
		TDynArray< CGraphBlock* > gb = m_graph->GraphGetBlocks();
		for( Uint32 i=0; i<gb.Size(); i++ )
		{
			CGraphBlock* cb = Cast< CGraphBlock >(gb[i]);
			//String blockName = cb->GetBlockName();
			if( cb && ( cb->GetBlockName() == TXT("Texture Sampler") ) )
			{
				amountOfBlocks += 1;
			}
		}
	
	return amountOfBlocks;
}