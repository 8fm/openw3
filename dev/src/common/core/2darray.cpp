/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "2darray.h"
#include "depot.h"
#include "scriptStackFrame.h"
#include "scriptingSystem.h"
#include "gatheredResource.h"

#define ALIGN_SIZE 4

IMPLEMENT_ENGINE_CLASS( C2dArray );

C2dArray::C2dArray()
{
}

void C2dArray::GetSize(Uint32 &colSize, Uint32 &rowSize) const
{
	colSize = m_headers.Size();
	rowSize = m_data.Size();
}

Bool C2dArray::Empty() const
{
	return m_data.Size()==0 || m_headers.Size()==0;
}

void C2dArray::OnSerialize( IFile &file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// No serialization for GC is needed
	if ( file.IsGarbageCollector() )
	{
		return;
	}

	// Serialize internal data
	file << m_headers;
	file << m_data;
}

Uint32 C2dArray::CalculateDataLength()
{
	Uint32 dataLength = 0;
	for ( Uint32 i = 0; i < m_headers.Size(); ++i )
	{
		dataLength += static_cast< Uint32 >( m_headers[ i ].GetLength() + 2 + 1);
	}
	for ( Uint32 j = 0; j < m_data.Size(); ++j )
	{
		for ( Uint32 k = 0; k < m_data[ j ].Size(); ++k )
		{
			dataLength += static_cast< Uint32 >( m_data[ j ][ k ].GetLength() + 2 + 1 );
		}
	}
	return dataLength;
}

void C2dArray::PrepareData(String& strData, const String& separator/*=TXT(";")*/)
{
	strData = String::EMPTY;

	if ( Empty() ) 
		return;

	// Headers
	WriteRow( strData, separator, m_headers );
	strData = strData + TXT("\n");

	// Data
	for ( Uint32 j=0; j<m_data.Size(); j++ )
	{
		String str;
		WriteRow( str, separator, m_data[j] );
		strData = strData + str + TXT("\n");
	}

	strData = strData.LeftString( strData.Size() - 2 );
}

void C2dArray::PrepareData( Char* strData, const Char separator )
{
	if ( Empty() == true )
	{
		return;
	}

	Uint32 writtenChars = 0;

	for ( Uint32 i = 0; i < m_headers.Size(); ++i )
	{
		WriteCell
		(
			m_headers[ i ],
			strData,
			writtenChars,
			separator, 
			FindSpecialChar( m_headers[ i ] ),
			( i == m_headers.Size() - 1 )
		);
	}
	WriteCharacter( '\n', strData, writtenChars );

	for ( Uint32 j = 0; j < m_data.Size(); ++j )
	{
		for ( Uint32 k = 0; k < m_data[ j ].Size(); ++k )
		{
			WriteCell( m_data[ j ][ k ], strData, writtenChars, separator, 
				FindSpecialChar( m_data[ j ][ k ] ), ( k == m_data[ j ].Size() - 1));
		}
		WriteCharacter( '\n', strData, writtenChars );
	}

}

C2dArray* C2dArray::CreateFromString( const String& absPath, const String separator )
{
	String fileData;

	String depotPath;

	if( GDepot->ConvertToLocalPath( absPath, depotPath ) )
	{
		CDiskFile *diskFile = GDepot->FindFile( depotPath );
		if( diskFile )
		{
			IFile *file = diskFile->CreateReader();
			if( file )
			{
				if( GFileManager->LoadFileToString( file, fileData ) )
				{
					if ( !fileData.Empty() )
					{
						C2dArray::FactoryInfo< C2dArray > info;
						C2dArray* arr = info.CreateResource();

						arr->ParseData( fileData, separator );

						delete file;
						return arr;
					}
				}
				delete file;
			}
			
		}
	}
	
	GFileManager->LoadFileToString( absPath, fileData, true );

	if ( !fileData.Empty() )
	{
		C2dArray::FactoryInfo< C2dArray > info;
		C2dArray* arr = info.CreateResource();

		arr->ParseData( fileData, separator );

		return arr;
	}

	return NULL;
}

/*
void C2dArray::LoadFromString( String absFilePath )
{
	String fileData;
	GFileManager->LoadFileToString( absFilePath, fileData );

	ParseData( fileData );
}


void C2dArray::SaveToString( String absFilePath )
{
	String saveData;
	PrepareData(saveData);

	// Save
	GFileManager->SaveStringToFile( absFilePath, saveData);
}
*/


void C2dArray::RemoveChars( String& source, const TDynArray<String> &chars)
{
	if (source.Empty()) return;

	Uint32 shift = 0;
	Bool searching;

	while ( (source.Size()-shift) > 0 )
	{
		searching = false;
		for (Uint32 i=0; i<chars.Size(); i++)
		{
			if (source.TypedData()[shift] == chars[i].TypedData()[0]) searching = true;
		}

		if( searching ) shift++;
		else break;
	}

	source = source.RightString( source.Size() - 1 - shift );
}


void C2dArray::ParseData(String inputData, const String separator/*=TXT("\t")*/)
{
	Uint32 rowIndex = 0;
	Bool readHeaders = true;

	Uint32 currentStart = 0;
	Uint32 currentEnd = 0;
	String currentString = TXT( "" );
	Char currentChar = 0;

	Bool isBracket = false;

	Uint32 inputDataLength = static_cast< Uint32 >( inputData.GetLength() );

	for ( currentEnd = 0; currentEnd < inputDataLength; ++currentEnd )
	{
		currentChar = inputData.TypedData()[ currentEnd ];
		if ( currentChar == separator.AsChar()[ 0 ] && isBracket == false )
		{
			if ( currentEnd == currentStart )
			{
				currentString = TXT( "" );
			}
			else
			{
				currentString = inputData.MidString( currentStart, currentEnd - currentStart );
			}
			
			// Hacky
			if ( currentString.GetLength() >= 2 )
			{
				const Char* currStr = currentString.AsChar();
				if ( currStr[0] == '\"' && currStr[ currentString.GetLength()-2 ] == '\"' )
				{
					currentString = currentString.MidString( 1, currentString.GetLength()-2 );
					ChangeInternalInvertedCommas(currentString, true);
				}
			}

			if ( readHeaders == true )
			{
				m_headers.PushBack( currentString );
			}
			else
			{
				if ( m_data.Size() < rowIndex + 1 )
				{
					//TDynArray< String > row;
					m_data.Grow( 1 );
				}
				m_data[ rowIndex ].PushBack( currentString );
			}
			currentStart = currentEnd + 1;
		}
		else if ( currentChar == 10 || currentEnd == inputDataLength - 1 )			// If 'new line' or 'last character'
		{
			if ( currentEnd > 0 && inputData.TypedData()[ currentEnd - 1 ] == 13 )	// If (...) and one character before is a 'carriage return'
			{
				currentString = inputData.MidString( currentStart, currentEnd - 1 - currentStart );
			}
			else if( currentChar != 10 )	// If not 'new line' (that means this is the 'last character' but without new line "\r\n")
			{
				currentString = inputData.MidString( currentStart, currentEnd + 1 - currentStart );
			}
			else	// 'new line' without 'carriage return'
			{
				currentString = inputData.MidString( currentStart, currentEnd - currentStart );
			}

			// Hacky
			if ( currentString.GetLength() >= 2 )
			{
				const Char* currStr = currentString.AsChar();
				if ( currStr[0] == '\"' && currStr[ currentString.GetLength()-2 ] == '\"' )
				{
					currentString = currentString.MidString( 1, currentString.GetLength()-2 );
					ChangeInternalInvertedCommas(currentString, true);
				}
			}

			if ( readHeaders == true )
			{
				m_headers.PushBack( currentString );
			}
			else
			{
				if ( m_data.Size() < rowIndex + 1 )
				{
					/*TDynArray< String > row;
					m_data.PushBack( row );*/
					m_data.Grow( 1 );
				}
				m_data[ rowIndex ].PushBack( currentString );
			}
			currentStart = currentEnd + 1;

			if ( readHeaders == false )
			{
				rowIndex += 1;
			}
			readHeaders = false;
			isBracket = false;
		}
		else if ( currentChar == 34 /* "\"" */ )
		{
			if ( isBracket == false )
			{
				isBracket = true;
			}
			else 
			{
				isBracket = false;
			}
		}
	}

	for ( Uint32 i = 0; i < m_data.Size(); ++i )
	{
		m_data[ i ].Resize( m_headers.Size() );
	}

	OnModified();
}

Bool C2dArray::Split( String& source, const String& separator, String *leftPart, String *rightPart)
{
	size_t commaPos = 0;
	size_t splitPos = 0;
	Bool FoundSplitPos = false;
	source.FindSubstring( TXT("\""), commaPos );
	

	if (commaPos==0)
	{
		// separator can be inside "", find end of word
		Uint32 i = 1;
		Int32 commaCounter = 0;
		while ( (source.Size()-i) > 0 )
		{
			if (source.TypedData()[i]==34)
			{
				commaCounter++;
			}
			else
			{
				if ( fmod( (Float)commaCounter, 2.0f) )
				{
					// End of word
					splitPos = i;
					break;
				}
				commaCounter = 0;
			}
			i++;
		}
	}
	else
	{
		FoundSplitPos = source.FindSubstring( separator, splitPos );
	}

	// Split
	if ( FoundSplitPos ) 
	{   
		// Get left part
		if ( leftPart ) 
		{
			*leftPart = source.LeftString( splitPos );
		}

		// Get right part
		if ( rightPart )
		{
			*rightPart = source.MidString( splitPos + separator.GetLength() );
		}
	}

	return FoundSplitPos;   
}

void C2dArray::WriteRow(String& data, const String& separator, const TStringArray& rowData)
{
	for (Uint32 i=0; i<rowData.Size(); i++)
	{
		String str = rowData[i];

		if (FindSpecialChar(str)) 
		{
			ChangeInternalInvertedCommas(str, false);

			str = TXT("\"") + str + TXT("\"");
		}

		if (i!=rowData.Size()-1) str = str + separator;

		data = data + str;
		OnModified();
	}
}

void C2dArray::ColumnPositions(TDynArray< Uint32 >& colPos )
{
	Uint32 pos = 0;

	for (Uint32 colNum=0; colNum<m_headers.Size(); colNum++)
	{
		// Header
		Uint32 size = m_headers[colNum].Size();
		if (FindSpecialChar(m_headers[colNum])) size += 2;	// Add ""

		// Data
		for (Uint32 i=0; i<m_data.Size(); i++)
		{
			if (m_data[i][colNum].Size() > size)
			{
				size = m_data[i][colNum].Size();
				if (FindSpecialChar(m_data[i][colNum])) size += 2;
			}
		}

		// Align
		Uint32 parts = (size-1)/ALIGN_SIZE;
		size = (parts+1)*ALIGN_SIZE;

		// Acc
		pos += size;

		colPos[colNum] = pos;
	}
}

Bool C2dArray::SetHeader(String value, Uint32 col)
{ 
	if (col >= m_headers.Size()) 
	{
		return false;
	}
	else
	{
		m_headers[col] = value; 
		OnModified(col);
		return true;
	}
}

Bool C2dArray::SetValue(String value, Uint32 col, Uint32 row)	
{
	if (Empty() || row >= m_data.Size() || col >= m_data[row].Size()) 
	{
		return false;
	}
	else
	{
		m_data[row][col] = value;
		OnModified(col, row);
		return true;
	}
}

Bool C2dArray::SetValue(String value, const String& header, Uint32 row)
{
	if (Empty() || row >= m_data.Size()) return false;

	for(Uint32 i=0; i<m_headers.Size(); i++)
	{
		if (m_headers[i]==header)
		{
			m_data[row][i] = value;
			OnModified(i, row);
			return true;
		}
	}

	return false;
}

Bool C2dArray::SetValue(String value, const String& keyFirstColName, const String& keyFirstColValue, const String& keySecondColName)
{
	if(Empty()) return false;

	for(Uint32 colNum=0; colNum<m_headers.Size(); colNum++)
	{
		if (m_headers[colNum]==keyFirstColName)
		{
			for (Uint32 rowNum=0; rowNum<m_data.Size(); rowNum++)
			{
				if (m_data[rowNum][colNum]==keyFirstColValue)
				{
					for (Uint32 destColNum=0; destColNum<m_headers.Size(); destColNum++)
					{
						if (m_headers[destColNum]==keySecondColName)
						{
							// Found :)
							m_data[rowNum][destColNum] = value;
							OnModified(destColNum, rowNum);
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

String C2dArray::GetHeader(Uint32 col) const
{
	if ( col >= m_headers.Size() ) 
		return String::EMPTY;

	return m_headers[col]; 
}

String C2dArray::GetValue(Uint32 col, Uint32 row) const
{ 
	if (col >= m_headers.Size() || row >= m_data.Size())
		return String::EMPTY;

	return m_data[row][col]; 
}

const String& C2dArray::GetValueRef(Uint32 col, Uint32 row) const
{ 
	if (col >= m_headers.Size() || row >= m_data.Size())
		return String::EMPTY;

	return m_data[row][col]; 
}

String C2dArray::GetValue(const String &header, Uint32 row) const
{ 
	if (row >= m_data.Size())
		return String::EMPTY;

	Uint32 col;
	if ( !FindHeader( header, col ) )
		return String::EMPTY;

	return m_data[row][col]; 
}

String C2dArray::GetValue(const String &keyFirstColName, const String &keyFirstColValue, const String &keySecondColName) const
{
	if(Empty()) return String::EMPTY;

	for(Uint32 colNum=0; colNum<m_headers.Size(); colNum++)
	{
		if (m_headers[colNum]==keyFirstColName)
		{
			for (Uint32 rowNum=0; rowNum<m_data.Size(); rowNum++)
			{
				if (m_data[rowNum][colNum]==keyFirstColValue)
				{
					for (Uint32 destColNum=0; destColNum<m_headers.Size(); destColNum++)
					{
						if (m_headers[destColNum]==keySecondColName)
						{
							// Found :)
							return m_data[rowNum][destColNum];
						}
					}
				}
			}
		}
	}

	return String::EMPTY;
}

Bool C2dArray::FindHeader(const String &header, Uint32 &col) const
{
	for(Uint32 colNum=0; colNum < m_headers.Size(); colNum++)
		if ( m_headers[colNum] == header )
		{
			col = colNum;
			return true;
		}

		return false;
}

void C2dArray::AddColumn(const String& rowDefaultValue /* = String::EMPTY */)
{
	m_headers.PushBack( GenerateUniqueHeaderName() );

	for (Uint32 i=0; i<m_data.Size(); i++)
	{
		m_data[i].PushBack(rowDefaultValue);
	}
}

void C2dArray::AddColumn(String colName, const String& rowDefaultValue)
{
	m_headers.PushBack( colName );

	for (Uint32 i=0; i<m_data.Size(); i++)
	{
		m_data[i].PushBack(rowDefaultValue);
	}
}

void C2dArray::AddRow(const TDynArray<String> &rowData, const String& rowDefaultValue /* = String::EMPTY */)
{
	TStringArray* row = ::new ( m_data ) TStringArray;

	for (Uint32 i=0; i<m_headers.Size() && i<rowData.Size(); i++)
	{
		row->PushBack( rowData[i] );
	}

	if (rowData.Size()<m_headers.Size())
	{
		for (Uint32 i=rowData.Size(); i<m_headers.Size(); i++)
		{
			row->PushBack( rowDefaultValue );
		}
	}

	OnModified();
}

void C2dArray::AddRow(const String& rowDefaultValue /* = String::EMPTY */)
{
	TStringArray* row = ::new ( m_data ) TStringArray;

	for (Uint32 i=0; i<m_headers.Size(); i++)
	{
		row->PushBack( rowDefaultValue );
	}
	OnModified();
}

void C2dArray::InsertColumn(Uint32 colNum, const String& rowDefaultValue /* = String::EMPTY */)
{
	if (colNum >= m_headers.Size()) return;

	m_headers.Insert(colNum, GenerateUniqueHeaderName());

	for (Uint32 i=0; i<m_data.Size(); i++)
	{
		m_data[i].Insert(colNum, rowDefaultValue);
	}
	OnModified(colNum);
}

void C2dArray::InsertColumn(Uint32 colNum, String colName, const String& rowDefaultValue)
{
	if (colNum >= m_headers.Size()) return;

	m_headers.Insert(colNum, colName);

	for (Uint32 i=0; i<m_data.Size(); i++)
	{
		m_data[i].Insert(colNum, rowDefaultValue);
	}
	OnModified(colNum);
}

void C2dArray::InsertColumn(String colPrevName, String colName, const String& rowDefaultValue /* = String::EMPTY */)
{
	for (Uint32 i=0; i<m_headers.Size(); i++)
	{
		if (m_headers[i]==colPrevName)
		{
			m_headers.Insert(i, colName);

			for (Uint32 j=0; j<m_data.Size(); j++)
			{
				m_data[j].Insert(i, rowDefaultValue);
			}
			OnModified(i);
		}
	}
}

void C2dArray::InsertRow(Uint32 rowNum, const TDynArray<String>& rowData, const String& rowDefaultValue /* = String::EMPTY */)
{
	TStringArray row;
	m_data.Insert(rowNum, row);

	for (Uint32 i=0; i<m_headers.Size() && i<rowData.Size(); i++)
	{
		m_data[rowNum].PushBack( rowData[i] );
	}

	if (rowData.Size()<m_headers.Size())
	{
		for (Uint32 i=rowData.Size(); i<m_headers.Size(); i++)
		{
			m_data[rowNum].PushBack( rowDefaultValue );
		}
	}
	OnModified();
}

void C2dArray::InsertRow(Uint32 rowNum, const String& rowDefaultValue /* = String::EMPTY */)
{
	TStringArray row;
	m_data.Insert(rowNum, row);

	for (Uint32 i=0; i<m_headers.Size(); i++)
	{
		m_data[rowNum].PushBack( rowDefaultValue );
	}
	OnModified();
}

void C2dArray::DeleteColumn(String colName)
{
	for (Uint32 col=0; col<m_headers.Size(); col++)
	{
		if (m_headers[col]==colName)
		{
			// Headers
			m_headers.Erase( m_headers.Begin()+col );

			// Data
			for (Uint32 row=0; row<m_data.Size(); row++)
			{
				m_data[row].Erase( m_data[row].Begin()+col );
			}

			OnModified(col);
			break;
		}
	}
}

void C2dArray::DeleteColumn(Uint32 colNum)
{
	if (colNum >= m_headers.Size()) return;

	// Headers
	m_headers.Erase( m_headers.Begin()+colNum );

	// Data
	for (Uint32 row=0; row<m_data.Size(); row++)
	{
		m_data[row].Erase( m_data[row].Begin()+colNum );
	}
	OnModified(colNum);
}

void C2dArray::DeleteRow(Uint32 rowNum)
{
	if (Empty() || rowNum >= m_data.Size()) return;

	m_data.Erase( m_data.Begin() + rowNum );
	OnModified();
}

Bool C2dArray::HasUniqueColumnNames() const
{
	if (Empty()) return true;

	for (Uint32 col=0; col<m_headers.Size(); col++)
	{
		String value = m_headers[col];

		for (Uint32 i=col+1; i<m_headers.Size(); i++)
		{
			if (m_headers[i]==value)
			{
				return false;
			}
		}
	}

	return true;
}

Bool C2dArray::HasUniqueRow(Uint32 rowNum) const
{
	if (Empty()) return true;
	if (rowNum >= m_data.Size()) return false;

	if (rowNum == (Uint32)-1) return HasUniqueColumnNames();

	for (Uint32 row=0; row<m_data.Size(); row++)
	{
		String value = m_data[rowNum][row];

		for (Uint32 i=row+1; i<m_data[rowNum].Size(); i++)
		{
			if (m_data[rowNum][i]==value)
			{
				return false;
			}
		}
	}

	return true;
}

Bool C2dArray::HasUniqueColumn(Uint32 colNum) const
{
	if (Empty()) return true;
	if (colNum >= m_headers.Size()) return false;

	for (Uint32 row=0; row<m_data.Size(); row++)
	{
		String value = m_data[row][colNum];

		for (Uint32 i=row+1; i<m_data.Size(); i++)
		{
			if (m_data[i][colNum] == value)
			{
				return false;
			}
		}
	}

	return true;
}

Bool C2dArray::HasUniqueColumn(const String& colName) const
{
	if (Empty()) return true;

	for (Uint32 col=0; col<m_headers.Size(); col++)
	{
		if (colName == m_headers[col])
		{
			return HasUniqueColumn(col);
		}
	}

	return true;
}

void C2dArray::Clear()
{
	m_headers.Clear();

	for (Uint32 row=0; row<m_data.Size(); row++)
	{
		m_data[row].Clear();
	}
	m_data.Clear();
	OnModified();
}

void C2dArray::Refresh()
{
	String arrayString;

	IFile *file = GetFile()->CreateReader();
	if( file )
	{
		if( GFileManager->LoadFileToString( file, arrayString ) )
		{
			Clear();
			ParseData( arrayString );
		}
		delete file;
	}

}

String C2dArray::GenerateUniqueHeaderName()
{
	String name = TXT("Col ");
	Int32 num = 0;
	Bool searching = true;

	String uniqueName;

	while(searching)
	{
		searching = false;

		uniqueName = name + String::Printf(TXT("%d"), ++num);

		for (Uint32 i=0; i<m_headers.Size(); i++)
		{
			if (m_headers[i]==uniqueName)
			{
				searching = true;
				break;
			}
		}
	}

	return uniqueName;
}

Bool C2dArray::FindSpecialChar(String &str)
{
	return ( str.ContainsCharacter( ';' ) || ( str.ContainsCharacter( '\"' ) ) );
}

void C2dArray::WriteCell( String cellString, Char* dataBuffer, Uint32& position, const Char separator, Bool appendBrackets, Bool isLastCellInRow )
{
	if ( appendBrackets == true )
	{
		ChangeInternalInvertedCommas( cellString, false );
		WriteCharacter( '\"', dataBuffer, position );
	}

	Red::System::MemoryCopy( dataBuffer + position, cellString.TypedData(), cellString.GetLength() * sizeof( Char ) );
	position += cellString.GetLength();

	if ( appendBrackets == true )
	{
		WriteCharacter( '\"', dataBuffer, position );
	}

	if ( isLastCellInRow == false )
	{
		WriteCharacter( separator, dataBuffer, position );
	}
}
RED_INLINE void C2dArray::WriteCharacter( const Char& character, Char* dataBuffer, Uint32& position )
{
	dataBuffer[ position ] = character;
	position += 1;
}

void C2dArray::ChangeInternalInvertedCommas(String &str, Bool loading)
{
	if (loading)
	{
		// Loading
		Uint32 i = 1;
		while ( (str.Size()-i) > 0 )
		{
			if (str.TypedData()[i]==34 && str.TypedData()[i-1]==34 )
			{
				str = str.LeftString(i) + str.RightString( str.Size() - i - 2 );
			}
			i++;
		}
	}
	else
	{
		// Saving
		Uint32 i = 0;
		while ( (str.Size()-i) > 0 )
		{
			if (str.TypedData()[i]==34)
			{
				str = str.LeftString(i) + TXT("\"") + str.RightString( str.Size() - i - 1 );
				i++;
			}
			i++;
		}
	}
}

Int32 C2dArray::GetColumnIndex( const String &header ) const
{
	for ( Uint32 i = 0; i < m_headers.Size(); i++ )
	{
		if ( header == m_headers[ i ] )
		{
			return ( Int32 ) i;
		}
	}
	return -1;
}

Int32 C2dArray::GetRowIndex( Int32 col, const String &value ) const
{
	if (col == -1) return col;

	for (Uint32 row=0; row<m_data.Size(); row++)
	{
		if (m_data[row][col]==value)
		{
			return (Int32)row;
		}
	}

	return -1;
}

Int32 C2dArray::GetRowIndex(const String &header, const String &value) const
{
	return GetRowIndex( GetColumnIndex( header ), value );
}


Bool I2dArrayPropertyOwner::SortChoices() const
{
	return false;
}

// -------------------------------------------------------
// ------------------- scripting support -----------------
// -------------------------------------------------------

void C2dArray::funcGetValueAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, column, 0 );
	GET_PARAMETER( Int32, row, 0 );
	FINISH_PARAMETERS;

	RETURN_STRING( GetValue( column, row ) );
}

void C2dArray::funcGetValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, header, String::EMPTY );
	GET_PARAMETER( Int32, row, 0 );
	FINISH_PARAMETERS;

	RETURN_STRING( GetValue( header, row ) );
}

void C2dArray::funcGetValueAtAsName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, column, 0 );
	GET_PARAMETER( Int32, row, 0 );
	FINISH_PARAMETERS;

	CName nameVal( GetValue( column, row ) );

	RETURN_NAME( nameVal );
}
void C2dArray::funcGetValueAsName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, header, String::EMPTY );
	GET_PARAMETER( Int32, row, 0 );
	FINISH_PARAMETERS;

	CName nameVal( GetValue( header, row ) );

	RETURN_NAME( nameVal );
}

void C2dArray::funcGetNumRows( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( static_cast< Int32 >( GetNumberOfRows() ) );
}

void C2dArray::funcGetNumColumns( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( static_cast< Int32 >( GetNumberOfColumns() ) );
}

void C2dArray::funcGetRowIndexAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, column, 0 );
	GET_PARAMETER( String, value, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_INT( GetRowIndex( column, value ) );
}

void C2dArray::funcGetRowIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, header, String::EMPTY );
	GET_PARAMETER( String, value, String::EMPTY );
	FINISH_PARAMETERS;
	RETURN_INT( GetRowIndex( header, value ) );
}

void C2dArray::Copy( C2dArray& dest, const C2dArray& src )
{
	dest.m_headers = src.m_headers;
	dest.m_data = src.m_data;
}

Bool C2dArray::Concatenate( C2dArray& dest, const C2dArray& src )
{
	Uint32 colCount = dest.GetNumberOfColumns();
	if( colCount != src.GetNumberOfColumns() )
	{
		return false;
	}

	//! check are headers the same
	for( Uint32 columnIndex = 0; columnIndex < colCount; ++columnIndex )
	{
		if( Red::System::StringCompareNoCase( dest.GetHeader( columnIndex ).AsChar(), src.GetHeader( columnIndex ).AsChar() ) != 0 )
		{
			return false;
		}
	}

	Uint32 rowCount = src.GetNumberOfRows();

	TDynArray<String> rowData;
	rowData.Reserve( colCount );

	for( Uint32 rowIndex = 0; rowIndex < rowCount; ++rowIndex )
	{
		rowData.ClearFast();
		for( Uint32 columnIndex = 0; columnIndex < colCount; ++columnIndex )
		{
			rowData.PushBack( src.GetValueRef( columnIndex, rowIndex ) );
		}
		dest.AddRow( rowData );			
	}

	return true;
}

static void funcLoadCSV( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, filePath, String::EMPTY );
	FINISH_PARAMETERS;

	CResource* arrayRes = GScriptingSystem->LoadScriptResource( filePath );
	C2dArray* loadedArray = Cast< C2dArray >( arrayRes );

	RETURN_OBJECT( loadedArray );
}

void RegisterCSVFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "LoadCSV", funcLoadCSV );
}

C2dArraysResourcesManager::C2dArraysResourcesManager( CGatheredResource& defaultGatheredResource ) : m_defaultGatheredResource( defaultGatheredResource )
{
#ifndef NO_EDITOR
	if( GIsEditor )
	{
		CDirectory* dlc = GDepot->FindPath( TXT("dlc\\") );
		if ( dlc != nullptr )
		{
			String dlcFolderName;
			String fullFilePathDLC;

			const String strFileToLoad = m_defaultGatheredResource.GetPath().ToString();
			const Char* csvFileToLoad = strFileToLoad.AsChar(); 

			for ( CDirectory* dlcDir : dlc->GetDirectories() )
			{
				dlcFolderName = dlcDir->GetName();
				fullFilePathDLC = String::Printf( TXT("dlc\\%ls\\data\\%ls"), dlcFolderName.AsChar(), csvFileToLoad );
				if( GDepot->FileExist( fullFilePathDLC ) )
				{				
					m_2dArrayFilePaths.PushBack( fullFilePathDLC );
					C2dArray* actionPointCategoriesArray = LoadResource< C2dArray >( fullFilePathDLC );
					m_2dArrays.PushBack( actionPointCategoriesArray );
					actionPointCategoriesArray->AddToRootSet();
				}			
			}
		}
	}
#endif
	Reload2dArray();
}

Bool C2dArraysResourcesManager::Load2dArray( const String& filePath )
{	
	if( m_2dArrayFilePaths.FindPtr( filePath ) == nullptr )
	{
		m_2dArrayFilePaths.PushBack( filePath );
		THandle<C2dArray> attitudeGroupsArray = LoadResource< C2dArray >( filePath );
		m_2dArrays.PushBack( attitudeGroupsArray );

		Reload2dArray();
		return true;
	}	
	return false;
}

Bool C2dArraysResourcesManager::Unload2dArray( const String& filePath )
{
	ptrdiff_t foundIndex = m_2dArrayFilePaths.GetIndex( filePath );
	if( foundIndex != -1 )
	{
		m_2dArrayFilePaths.RemoveAt( foundIndex );
#ifndef NO_EDITOR
		if ( THandle< C2dArray > arr = m_2dArrays[ (Int32)foundIndex ] )
		{
			if ( arr.IsValid() && arr->IsInRootSet() )
			{
				arr->RemoveFromRootSet();
			}
		}
#endif
		m_2dArrays.RemoveAt( foundIndex );

		Reload2dArray();
		return true;
	}	
	return false;
}

const C2dArray& C2dArraysResourcesManager::Reload2dArray()
{
	m_2dArray.Clear();

	C2dArray::Copy( m_2dArray, *m_defaultGatheredResource.LoadAndGet< C2dArray >() );

	for( const THandle<C2dArray> attitudeGroupsArray : m_2dArrays )
	{
		if( attitudeGroupsArray )
		{
			C2dArray::Concatenate( m_2dArray, *attitudeGroupsArray );
		}		
	}

	return m_2dArray;
}

#ifndef NO_EDITOR 
void C2dArraysResourcesManager::GetFileNameAndOffset( Uint32 globalIndex, String& fileName, Uint32& localIndex )
{
	Uint32 arraysCount = m_2dArrays.Size();
	Uint32 index = m_defaultGatheredResource.LoadAndGet< C2dArray >()->GetNumberOfRows();
	for( Uint32 i = 0; i < arraysCount; ++i)
	{
		C2dArray* attitudeGroupsArray = m_2dArrays[i];
		if( attitudeGroupsArray )
		{ 
			Uint32 numberOfRows = attitudeGroupsArray->GetNumberOfRows();			
			if( globalIndex < (index + numberOfRows) )
			{
				localIndex = globalIndex - index;
				fileName = m_2dArrayFilePaths[i];
				return;
			}
			index += numberOfRows;
		}
	}
}
#endif //! NO_EDITOR
