#include "build.h"
#include "gridCustomTypes.h"
#include "gridCustomCellEditors.h"
#include "../../common/core/diskFile.h"
#include "../../common/core/gatheredResource.h"

namespace // anonymous
{
	CGatheredResource resGridStickers( TXT("gameplay\\globals\\stickers.csv"), 0 );
}

wxString CGridTagListDesc::ToString( void *data ) const
{
    TagList *tagList = static_cast<TagList *>( data );
    return tagList->ToString().AsChar();
}

Bool CGridTagListDesc::FromString( void *data, const wxString &text ) const
{
    TagList *tagList = static_cast<TagList *>( data );
    return tagList->FromString( text.wc_str() );
}

wxGridCellRenderer *CGridTagListDesc::GetCellRenderer() const
{
    return new CGridCellTagRenderer();
}

wxGridCellEditor *CGridTagListDesc::GetCellEditor( Int32 row, Int32 col ) const
{
    return new CGridCellTagEditor();
}

//////////////////////////////////////////////////////////////////////////

wxString CGridGameTimeDesc::ToString( void *data ) const 
{
	GameTime *gameTime = static_cast<GameTime *>( data );
	return ::ToString( *gameTime ).AsChar();
}

Bool CGridGameTimeDesc::FromString( void *data, const wxString &text ) const
{
	GameTime *gameTime = static_cast<GameTime *>( data );
	return ::FromString( text.wc_str(), *gameTime );
}

//////////////////////////////////////////////////////////////////////////

wxString CGridSpawnTypeDesc::ToString( void *data ) const 
{
	CSSpawnType *spawnType = static_cast<CSSpawnType *>( data );
	return spawnType->m_spawnType.AsString().AsChar();
}

Bool CGridSpawnTypeDesc::FromString( void *data, const wxString &text ) const
{
	CSSpawnType *spawnType = static_cast<CSSpawnType *>( data );
	spawnType->m_spawnType = CName( text.wc_str() );
	return true;
}

wxGridCellEditor *CGridSpawnTypeDesc::GetCellEditor( Int32 row, Int32 col ) const
{
	C2dArray *file = resGridStickers.LoadAndGet< C2dArray >();
	if (!file || file->Empty()) 
	{
		ASSERT( TXT( "Couldn't load 'globals\\stickers.csv'" ) );
		return NULL;
	}

	const Uint32 &numRows = file->GetNumberOfRows();
	wxArrayString choices;
	for ( Uint32 i = 0; i < numRows; ++i )
	{
		choices.Add( file->GetValue( 0, i ).AsChar() );
	}
	return new CGridCellChoiceEditor( choices, false);
}

//////////////////////////////////////////////////////////////////////////

void CGridStoryPhaseTypeDesc::AddSource( Int32 col, CGatheredResource& res )
{
	m_csvs.Insert( col, &res );
}

wxString CGridStoryPhaseTypeDesc::ToString( void *data ) const 
{
	CSStoryPhaseNames *storyPhaseTags = static_cast<CSStoryPhaseNames *>( data );

	if ( storyPhaseTags->m_tags.Empty() )
	{
		return wxString( TXT("<<Default phase>>") );
	}
	else
	{
		return storyPhaseTags->m_tags.GetTags()[ 0 ].AsString().AsChar();
	}
}

Bool CGridStoryPhaseTypeDesc::FromString( void *data, const wxString &text ) const
{
	CSStoryPhaseNames *storyPhaseTags = static_cast<CSStoryPhaseNames *>( data );

	TagList newTags;
	if ( text.Length() > 0 && text != wxT( "<<Default phase>>" ) )
	{
		newTags.AddTag( CName( text.wc_str() ) );
	}
	storyPhaseTags->m_tags = newTags;

	return true;
}

wxGridCellEditor *CGridStoryPhaseTypeDesc::GetCellEditor( Int32 row, Int32 col ) const
{
	C2dArray *arr = NULL;

	CSVResources::const_iterator it = m_csvs.Find( col );
	if ( it != m_csvs.End() )
	{
		arr = it->m_second->LoadAndGet< C2dArray >();
		if ( !arr || arr->Empty() ) 
		{
			ASSERT( TXT( "Couldn't load the specifed story phases CSV" ) );
			return NULL;
		}
	}
	else
	{
		WARN_EDITOR( TXT( "No CSV specified to aquire the data from" ) );
		return NULL;
	}

	Uint32 colsCount, rowsCount;
	arr->GetSize( colsCount, rowsCount );

	wxArrayString choices;
	for ( Uint32 rowIdx = 0; rowIdx < rowsCount; ++rowIdx )
	{
		choices.Add( arr->GetValue( 0, rowIdx ).AsChar() );
	}

	class myCellChoiceEditor : public CGridCellChoiceEditor
	{
	private:
		C2dArray&			m_arr;

	public:
		myCellChoiceEditor( const wxArrayString& choices, C2dArray& arr )
			: CGridCellChoiceEditor( choices, true )
			, m_arr( arr )
		{
		}

		virtual void BeginEdit( int row, int col, wxGrid* grid )
		{
			CGridCellChoiceEditor::BeginEdit( row, col, grid );

			wxString currVal = grid->GetCellValue( row, col );
			if ( currVal == wxString( TXT("<<Default phase>>") ) )
			{
				grid->SetCellValue( row, col, wxT("") );
			}
		}

		virtual bool EndEdit(int row, int col, const wxGrid* grid,
			const wxString& oldval, wxString *newVal)
		{
			// check if the value we've just set is not present in the 
			// original array - if so - add it to it.
			if ( newVal->Length() > 0 && *newVal != TXT("<<Default phase>>") )
			{
				// we can only add values that are not null

				// locate our value in the array (memorize its index once its found)
				Uint32 colsCount, rowsCount;
				m_arr.GetSize( colsCount, rowsCount );
				Uint32 rowIdx = 0;
				for ( rowIdx = 0; rowIdx < rowsCount; ++rowIdx )
				{
					if ( m_arr.GetValue( 0, rowIdx ) == *newVal )
					{
						// the value is there
						break;
					}
				}
				if ( rowIdx >= rowsCount )
				{
					// we've analyzed the entire array adn failed to find a match - which
					// means that this is a new value - add it to the array then
					TDynArray< String > rowData;
					rowData.PushBack( newVal->wc_str() );
					m_arr.AddRow( rowData );	

					if ( m_arr.GetFile() )
					{
						m_arr.GetFile()->MarkModified();
					}
				}
			}

			// set value in the cell
			Bool result = CGridCellChoiceEditor::EndEdit( row, col, grid, oldval, newVal );
			if ( newVal->Length() == 0 )
			{
				*newVal = wxString( TXT("<<Default phase>>") );
			}
			return result;
		}
	};

	return new myCellChoiceEditor(choices, *arr );
}

//////////////////////////////////////////////////////////////////////////

CArrayEditorTypeDescBase::CArrayEditorTypeDescBase( Int32 col, const C2dArray* res, Bool isSorted /* = false */ )
	: m_res( res ) 
	, m_col( col )
	, m_isSorted( isSorted )
{}

wxString CArrayEditorTypeDescBase::ToString( void *data ) const
{
	CName *acName = static_cast<CName *>( data );
	return acName->AsString().AsChar();
}

Bool CArrayEditorTypeDescBase::FromString( void *data, const wxString &text ) const
{
	CName *acName = static_cast<CName *>( data );
	*acName = CName( text.wc_str() );
	return true;
}

wxGridCellEditor *CArrayEditorTypeDescBase::GetCellEditor( Int32 row, Int32 col ) const
{
	if ( col != m_col )
	{
		return NULL;
	}

	const C2dArray* arr = m_res; 

	if ( !arr || arr->Empty() ) 
	{
		ASSERT( TXT( "Couldn't load the specifed story phases CSV" ) );
		return NULL;
	}

	Uint32 colsCount, rowsCount;
	arr->GetSize( colsCount, rowsCount );

	TDynArray< String > choices;
	for ( Uint32 rowIdx = 0; rowIdx < rowsCount; ++rowIdx )
	{
		choices.PushBack( arr->GetValue( 0, rowIdx ) );
	}

	if ( m_isSorted )
	{
		Sort( choices.Begin(), choices.End() );
	}

	return new CGridStringsArrayEditor( choices );
}

//////////////////////////////////////////////////////////////////////////

CArrayEditorTypeDesc::CArrayEditorTypeDesc( Int32 col, CGatheredResource& res, Bool isSorted /* = false */ ): CArrayEditorTypeDescBase( col, res.LoadAndGet< C2dArray >(), isSorted)
{}

//////////////////////////////////////////////////////////////////////////

wxString CGridLayerNameDesc::ToString( void *data ) const
{
	CSLayerName *layerName = static_cast<CSLayerName *>( data );
	return layerName->m_layerName.AsString().AsChar();
}

Bool CGridLayerNameDesc::FromString( void *data, const wxString &text ) const
{
	CSLayerName* layerName = static_cast<CSLayerName *>( data );
	layerName->m_layerName = CName( text.wc_str() );
	return true;
}

wxGridCellRenderer *CGridLayerNameDesc::GetCellRenderer() const
{
    return new CGridCellLayerNameRenderer;
}

wxGridCellEditor *CGridLayerNameDesc::GetCellEditor( Int32 row, Int32 col ) const
{
	return new CGridCellLayerNameEditor;
}

//////////////////////////////////////////////////////////////////////////

wxString CGridVectorTypeDesc::ToString( void *data ) const 
{
    Vector *v = static_cast<Vector *>( data );
    return ::ToString( *v ).AsChar();
}

Bool CGridVectorTypeDesc::FromString( void *data, const wxString &text ) const
{
    Vector *v = static_cast<Vector *>( data );
    return ::FromString( text.wc_str(), *v );
}