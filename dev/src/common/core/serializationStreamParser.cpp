/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "rttiSystem.h"
#include "object.h"
#include "serializationStreamOpcodes.h"
#include "serializationStreamParser.h"
#include "configVar.h"

//#pragma optimize ("",off)

//#define DEBUG_SERIALIZATION  1

#ifdef DEBUG_SERIALIZATION
namespace Config
{
	TConfigVar< Bool > cvDumpSerializationStream( "Debug", "DumpSerializationStream", false );
}
#endif

CSerializationStreamParser::CSerializationStreamParser()
{
}

namespace Helper
{
	RED_FORCE_INLINE static Uint8 Read8( const Uint8*& ptr )
	{
		const Uint8 ret = *(const Uint8*) ptr;
		ptr += 1;
		return ret;
	}

	RED_FORCE_INLINE static Uint16 Read16( const Uint8*& ptr )
	{
		const Uint16 ret = *(const Uint16*) ptr;
		ptr += 2;
		return ret;
	}

	RED_FORCE_INLINE static Uint32 Read32( const Uint8*& ptr )
	{
		const Uint32 ret = *(const Uint32*) ptr;
		ptr += 4;
		return ret;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Write8NoAlias( void* targetPtr, const Uint8*& src )
	{
		*(Uint8*) targetPtr = *(const Uint8*) src;
		src += 1;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Write16NoAlias( void* targetPtr, const Uint8*& src )
	{
		*(Uint16*) targetPtr = *(const Uint16*) src;
		src += 2;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Write32NoAlias( void* targetPtr, const Uint8*& src )
	{
		*(Uint32*) targetPtr = *(const Uint32*) src;
		src += 4;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Write64NoAlias( void* targetPtr, const Uint8*& src )
	{
		*(Uint64*) targetPtr = *(const Uint64*) src;
		src += 8;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Zero8( void* targetPtr )
	{
		*(Uint8*) targetPtr = 0;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Zero16( void* targetPtr )
	{
		*(Uint16*) targetPtr = 0;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Zero32( void* targetPtr )
	{
		*(Uint32*) targetPtr = 0;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void Zero64( void* targetPtr )
	{
		*(Uint64*) targetPtr = 0;
	}

	RED_FORCE_INLINE RED_RESTRICT_PARAMS static void WriteFloat( void* targetPtr, const Float val )
	{
		*(Float*) targetPtr = val;
	}

	RED_FORCE_INLINE const CName ReadName( const Uint8*& ptr, const CFileDirectSerializationTables& tables )
	{
		const Uint16 nameIndex = Read16(ptr);
		RED_FATAL_ASSERT( nameIndex < tables.m_numMappedNames, "Invalid name index %d in serialization stream", nameIndex );
		return tables.m_mappedNames[ nameIndex ];
	}

	RED_FORCE_INLINE const IRTTIType* ReadType( const Uint8*& ptr, const CFileDirectSerializationTables& tables )
	{
		const Uint16 typeIndex = Read16(ptr);
		RED_FATAL_ASSERT( typeIndex < tables.m_numMappedTypes, "Invalid type index %d in serialization stream", typeIndex );
		const IRTTIType* type = tables.m_mappedTypes[typeIndex];
		if ( !type )
		{
			const CName typeName = tables.m_mappedNames[ typeIndex ];
			type = SRTTI::GetInstance().FindType( typeName );
			RED_FATAL_ASSERT( type != nullptr, "Invalid type '%ls' index %d used in serialization stream", typeName.AsChar(), typeIndex );
			tables.m_mappedTypes[typeIndex] = type;
		}

		return type;
	}

	static const Char* GetOpcodeName( const ESerializationOpcode op )
	{
#define TEST(x) case x: return TXT(#x) + 21;
		switch (op)
		{
			TEST(eSerializationOpcode_Pop);
			TEST(eSerializationOpcode_Advance);
			TEST(eSerializationOpcode_PushCurrent);
			TEST(eSerializationOpcode_PushProp);
			TEST(eSerializationOpcode_PushOffset);
			TEST(eSerializationOpcode_PushScript);
			TEST(eSerializationOpcode_PushDynArray8);
			TEST(eSerializationOpcode_PushDynArray32);
			TEST(eSerializationOpcode_PushStaticArray8);
			TEST(eSerializationOpcode_PushStaticArray32);
			TEST(eSerializationOpcode_PushNativeArray8);
			TEST(eSerializationOpcode_PushNativeArray32);
			TEST(eSerializationOpcode_InlinedStruct);
			TEST(eSerializationOpcode_DataTrue);
			TEST(eSerializationOpcode_DataFalse);
			TEST(eSerializationOpcode_Data8);
			TEST(eSerializationOpcode_Data16);
			TEST(eSerializationOpcode_Data32);
			TEST(eSerializationOpcode_Data64);
			TEST(eSerializationOpcode_Zero8);
			TEST(eSerializationOpcode_Zero16);
			TEST(eSerializationOpcode_Zero32);
			TEST(eSerializationOpcode_Zero64);
			TEST(eSerializationOpcode_RawBlock);
			TEST(eSerializationOpcode_StringEmpty);
			TEST(eSerializationOpcode_StringUnicode);
			TEST(eSerializationOpcode_StringAnsi);
			TEST(eSerializationOpcode_StringUnicode32);
			TEST(eSerializationOpcode_StringAnsi32);
			TEST(eSerializationOpcode_Name);
			TEST(eSerializationOpcode_Pointer);
			TEST(eSerializationOpcode_Handle);
			TEST(eSerializationOpcode_SoftHandle);
			TEST(eSerializationOpcode_Enum1);
			TEST(eSerializationOpcode_Enum2);
			TEST(eSerializationOpcode_Enum4);
			TEST(eSerializationOpcode_BitField1);
			TEST(eSerializationOpcode_BitField2);
			TEST(eSerializationOpcode_BitField4);
			TEST(eSerializationOpcode_Generic);
			TEST(eSerializationOpcode_Start);
			TEST(eSerializationOpcode_End);
		}
#undef TEST

		return TXT("Unknown");
	}

	static void DecodeOpcode( const Uint8* opCur, const CFileDirectSerializationTables& tables, String& outTxt )
	{
		// opcode name
		const ESerializationOpcode op = (ESerializationOpcode) *opCur++;
		outTxt += GetOpcodeName(op);

		// addition stuff
		if ( op == eSerializationOpcode_PushProp )
		{
			Uint16 propIndex = Helper::Read16( opCur );

			outTxt += TXT(", index=");
			outTxt += ToString( propIndex );

			const CProperty* prop = tables.m_mappedProperties[ propIndex ];
			if ( prop )
			{
				outTxt += TXT(", prop=");
				outTxt += prop->GetName().AsChar();

				outTxt += TXT(", class=");
				outTxt += prop->GetParent()->GetName().AsChar();
			}
		}
		else if ( op == eSerializationOpcode_RawBlock )
		{
			const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( Helper::ReadType( opCur, tables ) );
			if ( arrayType )
			{
				outTxt += TXT(", type=");
				outTxt += arrayType->GetName().AsChar();
			}

			const Uint32 count = Helper::Read32( opCur );
			outTxt += TXT(", count=");
			outTxt += ToString(count);

			const Uint8 elemSize = Helper::Read8( opCur );
			outTxt += TXT(", elemSize=");
			outTxt += ToString(elemSize);
		}
		else if ( op == eSerializationOpcode_PushDynArray8 || op == eSerializationOpcode_PushDynArray32 )
		{
			const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( Helper::ReadType( opCur, tables ) );
			if ( arrayType )
			{
				outTxt += TXT(", type=");
				outTxt += arrayType->GetName().AsChar();
			}

			// load the array count
			Uint32 count = 0;
			if ( op == eSerializationOpcode_PushDynArray8 )
				count = Helper::Read8( opCur );
			else if ( op == eSerializationOpcode_PushDynArray32 )
				count = Helper::Read32( opCur );

			outTxt += TXT(", count=");
			outTxt += ToString(count);
		}
		else if ( op == eSerializationOpcode_PushStaticArray8 || op == eSerializationOpcode_PushStaticArray32 )
		{
			const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( Helper::ReadType( opCur, tables ) );
			if ( arrayType )
			{
				outTxt += TXT(", type=");
				outTxt += arrayType->GetName().AsChar();
			}

			// load the array count
			Uint32 count = 0;
			if ( op == eSerializationOpcode_PushStaticArray8 )
				count = Helper::Read8( opCur );
			else if ( op == eSerializationOpcode_PushStaticArray32 )
				count = Helper::Read32( opCur );

			outTxt += TXT(", count=");
			outTxt += ToString(count);
		}
		else if ( op == eSerializationOpcode_PushNativeArray8 || op == eSerializationOpcode_PushNativeArray32 )
		{
			const CRTTINativeArrayType* arrayType = static_cast< const CRTTINativeArrayType* >( Helper::ReadType( opCur, tables ) );
			if ( arrayType )
			{
				outTxt += TXT(", type=");
				outTxt += arrayType->GetName().AsChar();
			}

			// load the array count
			Uint32 count = 0;
			if ( op == eSerializationOpcode_PushNativeArray8 )
				count = Helper::Read8( opCur );
			else if ( op == eSerializationOpcode_PushNativeArray32 )
				count = Helper::Read32( opCur );

			outTxt += TXT(", count=");
			outTxt += ToString(count);
		}
		else if ( op == eSerializationOpcode_InlinedStruct )
		{
			const CClass* classType = static_cast< const CClass* >( Helper::ReadType( opCur, tables ) );
			if ( classType )
			{
				outTxt += TXT(", type=");
				outTxt += classType->GetName().AsChar();
			}

			// load the array count
			Uint16 count = Helper::Read16( opCur );
			outTxt += TXT(", size=");
			outTxt += ToString(count);
		}
	}

	// Skip stream
	void SkipStream( const Uint8*& opCur )
	{
		Uint32 level = 1;

		while ( level > 0 )
		{
			const ESerializationOpcode op = (ESerializationOpcode) *opCur++;

			switch ( op )
			{
				case eSerializationOpcode_PushCurrent:
				{
					level += 1;
					break;
				}

				case eSerializationOpcode_PushOffset:
				{
					Helper::Read16(opCur);
					level += 1;
					break;
				}

				case eSerializationOpcode_PushProp:
				{
					Read16(opCur); // prop index
					level += 1;
					break;
				}

				case eSerializationOpcode_PushScript:
				{
					level += 1;
					break;
				}

				case eSerializationOpcode_RawBlock:
				{
					Read16(opCur); // type

					const Uint32 count = Helper::Read32( opCur );
					const Uint8 elemSize = Helper::Read8( opCur );
					opCur += (elemSize * count);
					break;
				}

				case eSerializationOpcode_PushDynArray8:
				case eSerializationOpcode_PushDynArray32:
				{
					Read16(opCur); // type

					// load the array count
					Uint32 count = 0;
					if ( op == eSerializationOpcode_PushDynArray8 )
						count = Helper::Read8( opCur );
					else if ( op == eSerializationOpcode_PushDynArray32 )
						count = Helper::Read32( opCur );

					// cleanup the array
					if ( count > 0 )
					{
						level += 1;
					}
					break;
				}

				case eSerializationOpcode_PushStaticArray8:
				case eSerializationOpcode_PushStaticArray32:
				{
					Read16(opCur); // type

					// load the array count
					Uint32 count = 0;
					if ( op == eSerializationOpcode_PushStaticArray8 )
						count = Helper::Read8( opCur );
					else if ( op == eSerializationOpcode_PushStaticArray32 )
						count = Helper::Read32( opCur );

					// cleanup the array
					if ( count > 0 )
					{
						level += 1;
					}
					break;
				}

				case eSerializationOpcode_PushNativeArray8:
				case eSerializationOpcode_PushNativeArray32:
				{
					Read16(opCur); // type

					// load the array count
					Uint32 count = 0;
					if ( op == eSerializationOpcode_PushNativeArray8 )
						count = Helper::Read8( opCur );
					else if ( op == eSerializationOpcode_PushNativeArray32 )
						count = Helper::Read32( opCur );

					// Push the array pointer as the scope
					if ( count > 0 )
					{
						level += 1;
					}
					break;
				}

				case eSerializationOpcode_InlinedStruct:
				{
					Read16(opCur); // type

					const Uint16 classSize = Helper::Read16( opCur );
					opCur += classSize;
					break;
				}

				case eSerializationOpcode_Pop:
				{
					level -= 1;
					break;
				}
			
				case eSerializationOpcode_Data8:
				{
					opCur += 1;
					break;
				}

				case eSerializationOpcode_Data16:
				{
					opCur += 2;
					break;
				}

				case eSerializationOpcode_Data32:
				{
					opCur += 4;
					break;
				}

				case eSerializationOpcode_Data64:
				{
					opCur += 8;
					break;
				}

				case eSerializationOpcode_BitField1:
				case eSerializationOpcode_Enum1:
				{
					opCur += 1;
					Read16(opCur); // name
					break;
				}

				case eSerializationOpcode_BitField2:
				case eSerializationOpcode_Enum2:
				{
					opCur += 2;
					Read16(opCur); // name
					break;
				}

				case eSerializationOpcode_BitField4:
				case eSerializationOpcode_Enum4:
				{
					opCur += 4;
					Read16(opCur); // name
					break;
				}

				case eSerializationOpcode_Name:
				{
					Read16(opCur); // name
					break;
				}

				case eSerializationOpcode_Generic:
				{
					Read16(opCur); // name

					// read skip offset that was saved in the stream so we can skip the data even if the type is uknown
					const Uint8* opBlockStart = opCur;
					const Uint32 skipOffset = Helper::Read32( opCur );
					opCur = opBlockStart + skipOffset;
					break;
				}

				case eSerializationOpcode_StringUnicode:
				case eSerializationOpcode_StringUnicode32:
				{
					const Uint32 len = (op == eSerializationOpcode_StringUnicode)
						? Helper::Read8(opCur) : Helper::Read32(opCur);
					opCur += (len+1) * sizeof(Char);
					break;
				}

				case eSerializationOpcode_StringAnsi:
				case eSerializationOpcode_StringAnsi32:
				{
					const Uint32 len = (op == eSerializationOpcode_StringAnsi)
						? Helper::Read8(opCur) : Helper::Read32(opCur);
					opCur += (len+1) * sizeof(AnsiChar);
					break;
				}

				case eSerializationOpcode_Pointer:
				case eSerializationOpcode_Handle:
				{
					Read16(opCur); // name
					opCur += 4;
					break;
				}

				case eSerializationOpcode_SoftHandle:
				{
					Read16(opCur); // name
					opCur += 2;
					break;
				}
			}
		}
	}

} // Helper

void CSerializationStreamParser::ParseStream( IFile& fallbackReader, const CFileDirectSerializationTables& tables, const void* streamData, const Uint32 streamSize, void* targetPtr, const CClass* targetClass )
{
	const static Uint32 MAX_STACK = 16;

	// clear stack
	void* stackPtr[ MAX_STACK ];
	Uint32 stackStride[ MAX_STACK ];
	Uint32 stackCount = 0;
	Uint32 startEndCount = 0;

	// start with current target pointer
	void* current = targetPtr;
	Uint32 currentStride = 0;

	// base offset
	const Uint64 baseOffset = fallbackReader.GetOffset();

	// we should start with a "start" token
	const Uint8* opStart = (const Uint8*) streamData;
	RED_FATAL_ASSERT( *opStart == eSerializationOpcode_Start, "Serialization stream is corrupted" );

	// execute the stream
	const Uint8* opCur = (const Uint8*) streamData;
	const Uint8* opEnd = (const Uint8*) streamData + streamSize;
	bool valid = true;
	while ( opCur < opEnd && valid )
	{
		// parse op
		const ESerializationOpcode op = (ESerializationOpcode) *opCur++;

		// debug
#if DEBUG_SERIALIZATION
		if ( Config::cvDumpSerializationStream.Get() )
		{
			String txt;
			Helper::DecodeOpcode( opCur-1, tables, txt );

			Char levelBuffer[32];
			for ( Uint32 i=0; i<startEndCount; ++i )
			{
				levelBuffer[i] = ' ';
			}
			levelBuffer[startEndCount] = 0;
			LOG_CORE( TXT("[%04d]: %ls%ls"), (opCur-opStart)-1, levelBuffer, txt.AsChar() );
		}
#endif

		// process op
		switch ( op )
		{
			case eSerializationOpcode_Start:
			{
				startEndCount += 1;
				break;
			}

			case eSerializationOpcode_End:
			{
				startEndCount -= 1;
				if ( !startEndCount )
				{
					valid = false;
				}
				break;
			}

			case eSerializationOpcode_PushCurrent:
			{
				RED_FATAL_ASSERT( stackCount < MAX_STACK, "Internal serialization stream stack is full" );
				stackPtr[ stackCount ] = current;
				stackStride[ stackCount ] = currentStride;
				stackCount += 1;
				currentStride = 0;
				break;
			}

			case eSerializationOpcode_PushOffset:
			{
				RED_FATAL_ASSERT( stackCount < MAX_STACK, "Internal serialization stream stack is full" );
				const Uint32 offset = Helper::Read16(opCur);
				stackPtr[ stackCount ] = current;
				stackStride[ stackCount ] = currentStride;
				stackCount += 1;
				currentStride = 0;
				current = OffsetPtr( current, offset );
				break;
			}

			case eSerializationOpcode_PushProp:
			{
				RED_FATAL_ASSERT( stackCount < MAX_STACK, "Internal serialization stream stack is full" );
				const Uint16 propIndex = Helper::Read16(opCur);
				RED_FATAL_ASSERT( propIndex < tables.m_numMappedProperties, "Internal serialization stream is corrupted: property index %d out of bounds (%d)", propIndex, tables.m_mappedProperties );
				const CProperty* prop = tables.m_mappedProperties[ propIndex ];

				if ( prop )
				{
					const Int32 offset = tables.m_mappedPropertyOffsets[ propIndex ];
					RED_FATAL_ASSERT( offset != -1, "Internal serialization stream is corrupted: property %d is not mapped", propIndex );

					stackPtr[ stackCount ] = current;
					stackStride[ stackCount ] = currentStride;
					currentStride = 0;
					stackCount += 1;

					current = OffsetPtr( current, offset );
				}
				else
				{
#if defined(RED_PLATFORM_WIN64) && defined(RED_FINAL_BUILD)
/*					Char buf[256];
					Red::SNPrintF( buf, 256, TXT("Missing property '%ls' in '%ls' - THIS PROPERTY SHOULD NOT BE COOKED\n"), propName.AsChar(), parentClass->GetName().AsChar() );
					OutputDebugStringW(buf);*/
#endif
					Helper::SkipStream( opCur );
				}
				break;
			}

			case eSerializationOpcode_PushScript:
			{
				IScriptable* assumedScriptable = (IScriptable*) current;
				stackPtr[ stackCount ] = current;
				stackStride[ stackCount ] = currentStride;
				currentStride = 0;
				stackCount += 1;
				current = assumedScriptable->GetScriptPropertyData();
				break;
			}

			case eSerializationOpcode_Advance:
			{
				RED_FATAL_ASSERT( currentStride != 0, "Advance called with zero stride. Stream error." );
				current = OffsetPtr( current, currentStride );
				break;
			}

			case eSerializationOpcode_RawBlock:
			{
				RED_FATAL_ASSERT( stackCount < MAX_STACK, "Internal serialization stream stack is full" );
				const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( Helper::ReadType( opCur, tables ) );
				IRTTIType* innerType = arrayType->ArrayGetInnerType();

				const Uint32 count = Helper::Read32( opCur );
				const Uint8 elemSize = Helper::Read8( opCur );
				RED_FATAL_ASSERT( count > 2, "Invalid raw block size" );

				// clear existing data
				CBaseArray& ar = *(CBaseArray*)current;

				innerType->ResizeBuffer( ar, 0 );

				// load new data
				const Uint32 currentElementSize = innerType->GetSize();
				if ( currentElementSize == elemSize )
				{
					// read data
					innerType->ResizeBuffer( ar, count );
					Red::MemoryCopy( ar.Data(), opCur, count * elemSize );
				}

				// go past the data
				opCur += (elemSize * count);
				break;
			}

			case eSerializationOpcode_PushDynArray8:
			case eSerializationOpcode_PushDynArray32:
			{
				RED_FATAL_ASSERT( stackCount < MAX_STACK, "Internal serialization stream stack is full" );
				const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( Helper::ReadType( opCur, tables ) );
				IRTTIType* innerType = arrayType->ArrayGetInnerType();

				// load the array count
				Uint32 count = 0;
				if ( op == eSerializationOpcode_PushDynArray8 )
					count = Helper::Read8( opCur );
				else if ( op == eSerializationOpcode_PushDynArray32 )
					count = Helper::Read32( opCur );

				// cleanup the array
				arrayType->Clean( current );
				if ( count > 0 )
				{
					// Allocate memory
					CBaseArray& ar = *(CBaseArray*)current;
					const Uint32 elementSize = innerType->GetSize();
					innerType->ResizeBuffer( ar, count );
					Red::MemoryZero( ar.Data(), elementSize * count );

					// Create the elements
					if ( innerType->GetType() != RT_Simple && innerType->GetType() != RT_Fundamental )
					{
						for ( Uint32 i=0; i<count; ++i )
						{
							void* itemData = arrayType->GetArrayElement( current, i );
							innerType->Construct( itemData );
						}
					}

					// Push the array pointer as the scope
					stackPtr[ stackCount ] = current;
					stackStride[ stackCount ] = currentStride;
					currentStride = innerType->GetSize();
					current = ar.Data();
					stackCount += 1;
				}
				break;
			}

			case eSerializationOpcode_PushStaticArray8:
			case eSerializationOpcode_PushStaticArray32:
			{
				RED_FATAL_ASSERT( stackCount < MAX_STACK, "Internal serialization stream stack is full" );
				const CRTTIStaticArrayType* arrayType = static_cast< const CRTTIStaticArrayType* >( Helper::ReadType( opCur, tables ) );
				const IRTTIType* innerType = arrayType->ArrayGetInnerType();

				// load the array count
				Uint32 count = 0;
				if ( op == eSerializationOpcode_PushStaticArray8 )
					count = Helper::Read8( opCur );
				else if ( op == eSerializationOpcode_PushStaticArray32 )
					count = Helper::Read32( opCur );

				// cleanup the array
				IBaseStaticArray* staticArrayData = static_cast< IBaseStaticArray* >( current );
				staticArrayData->Clear();
				if ( count > 0 )
				{
					// Allocate memory
					const Uint32 elementSize = innerType->GetSize();
					staticArrayData->Grow( count );
					Red::MemoryZero( staticArrayData->GetElement(elementSize, 0), elementSize * count );

					// Create the elements
					if ( innerType->GetType() != RT_Simple && innerType->GetType() != RT_Fundamental )
					{
						for ( Uint32 i=0; i<count; ++i )
						{
							void* itemData = arrayType->ArrayGetArrayElement( current, i );
							innerType->Construct( itemData );
						}
					}

					// Push the array pointer as the scope
					stackPtr[ stackCount ] = current;
					stackStride[ stackCount ] = currentStride;
					currentStride = innerType->GetSize();
					current = staticArrayData->GetElement(currentStride, 0);
					stackCount += 1;
				}

				break;
			}

			case eSerializationOpcode_PushNativeArray8:
			case eSerializationOpcode_PushNativeArray32:
			{
				const CRTTINativeArrayType* arrayType = static_cast< const CRTTINativeArrayType* >( Helper::ReadType( opCur, tables ) );
				const IRTTIType* innerType = arrayType->ArrayGetInnerType();

				// load the array count
				Uint32 count = 0;
				if ( op == eSerializationOpcode_PushNativeArray8 )
					count = Helper::Read8( opCur );
				else if ( op == eSerializationOpcode_PushNativeArray32 )
					count = Helper::Read32( opCur );

				// cleanup the array
				const Uint32 actualCount = arrayType->ArrayGetArraySize(current);
				RED_FATAL_ASSERT( actualCount == count, "Mismatched native array size, saved=%d, current=%d", count, actualCount );

				// Create the element
				arrayType->Construct( current );

				// Push the array pointer as the scope
				if ( actualCount > 0 )
				{
					stackPtr[ stackCount ] = current;
					stackStride[ stackCount ] = currentStride;
					currentStride = innerType->GetSize();
					stackCount += 1;
				}
				break;
			}

			case eSerializationOpcode_InlinedStruct:
			{
				const CClass* classType = static_cast< const CClass* >( Helper::ReadType( opCur, tables ) );
				const Uint16 classSize = Helper::Read16( opCur );

				// load data
				if ( classType && classType->GetSize() == classSize )
				{
					Red::MemoryCopy( current, opCur, classSize );
				}

				// move past the data
				opCur += classSize;
				break;
			}

			case eSerializationOpcode_Pop:
			{
				RED_FATAL_ASSERT( stackCount > 0, "Internal serialization stream stack underflow" );
				stackCount -= 1;
				current = stackPtr[ stackCount ];
				currentStride = stackStride[ stackCount ];
				break;
			}

			case eSerializationOpcode_DataTrue:
			{
				*(Bool*)current = true;
				break;
			}

			case eSerializationOpcode_DataFalse:
			{
				*(Bool*)current = false;
				break;
			}

			case eSerializationOpcode_Data8:
			{
				Helper::Write8NoAlias( current, opCur );
				break;
			}

			case eSerializationOpcode_Data16:
			{
				Helper::Write16NoAlias( current, opCur );
				break;
			}

			case eSerializationOpcode_Data32:
			{
				Helper::Write32NoAlias( current, opCur );
				break;
			}

			case eSerializationOpcode_Data64:
			{
				Helper::Write64NoAlias( current, opCur );
				break;
			}

			case eSerializationOpcode_BitField1:
			case eSerializationOpcode_Enum1:
			{
				Helper::Write8NoAlias( current, opCur );
				Helper::ReadName( opCur, tables );
				break;
			}

			case eSerializationOpcode_BitField2:
			case eSerializationOpcode_Enum2:
			{
				Helper::Write16NoAlias( current, opCur );
				Helper::ReadName( opCur, tables );
				break;
			}

			case eSerializationOpcode_BitField4:
			case eSerializationOpcode_Enum4:
			{
				Helper::Write32NoAlias( current, opCur );
				Helper::ReadName( opCur, tables );
				break;
			}

			case eSerializationOpcode_Zero8:
			{
				Helper::Zero8( current );
				break;
			}

			case eSerializationOpcode_Zero16:
			{
				Helper::Zero16( current );
				break;
			}

			case eSerializationOpcode_Zero32:
			{
				Helper::Zero32( current );
				break;
			}

			case eSerializationOpcode_Zero64:
			{
				Helper::Zero64( current );
				break;
			}

			case eSerializationOpcode_Name:
			{
				*(CName*)current = Helper::ReadName( opCur, tables );
				break;
			}

			case eSerializationOpcode_Generic:
			{
				const IRTTIType* type = Helper::ReadType( opCur, tables );

				// read skip offset that was saved in the stream so we can skip the data even if the type is uknown
				const Uint8* opBlockStart = opCur;
				const Uint32 skipOffset = Helper::Read32( opCur );
				RED_FATAL_ASSERT( skipOffset >= 4, "Invalid skip offset: %d", skipOffset );

				// skip or load the data
				if ( type != nullptr )
				{
					const Int64 relativeOffset = (opCur - opStart);
					fallbackReader.Seek( baseOffset + relativeOffset );
					type->Serialize( fallbackReader, current );
				}

				// skip over the data
				opCur = opBlockStart + skipOffset;
				break;
			}

			case eSerializationOpcode_StringEmpty:
			{
				*(String*) current = String::EMPTY;
				break;
			}

			case eSerializationOpcode_StringUnicode:
			case eSerializationOpcode_StringUnicode32:
			{
				const Uint32 len = (op == eSerializationOpcode_StringUnicode)
					? Helper::Read8(opCur) : Helper::Read32(opCur);
				const Char* txt = (const Char*)opCur;
				*(String*) current = txt;
				opCur += (len+1) * sizeof(Char);
				break;
			}

			case eSerializationOpcode_StringAnsi:
			case eSerializationOpcode_StringAnsi32:
			{
				const Uint32 len = (op == eSerializationOpcode_StringAnsi)
					? Helper::Read8(opCur) : Helper::Read32(opCur);
				const AnsiChar* txt = (const AnsiChar*)opCur;
				*(String*) current = ANSI_TO_UNICODE( txt );
				opCur += (len+1) * sizeof(AnsiChar);
				break;
			}

			case eSerializationOpcode_Pointer:
			case eSerializationOpcode_Handle:
			{
				const IRTTIType* type = Helper::ReadType( opCur, tables );

				// skip or load the data
				if ( type != nullptr )
				{
					const Int64 relativeOffset = (opCur - opStart);
					fallbackReader.Seek( baseOffset + relativeOffset );
					type->Serialize( fallbackReader, current );
				}

				// skip the data that was read (or not)
				opCur += 4;
				break;
			}

			case eSerializationOpcode_SoftHandle:
			{
				const IRTTIType* type = Helper::ReadType( opCur, tables );

				// skip or load the data
				if ( type != nullptr )
				{
					const Int64 relativeOffset = (opCur - opStart);
					fallbackReader.Seek( baseOffset + relativeOffset );
					type->Serialize( fallbackReader, current );
				}

				// skip the data that was read (or not)
				opCur += 2;
				break;
			}

			default:
				RED_FATAL( "Invalid serialization stream opcode: %d", op );
		}
	}

	// advance
	const Int64 streamDataSize = (opCur - opStart);
	fallbackReader.Seek( baseOffset + streamDataSize );
}

void CSerializationStream::Parse( IFile& reader, void* targetPtr, const CClass* targetClass )
{
	CSerializationStreamParser parser;

	IFileDirectMemoryAccess* dma = reader.QueryDirectMemoryAccess();
	RED_FATAL_ASSERT( dma != nullptr, "Trying to load serialization stream from a file '%ls' this is not mapped to memory", reader.GetFileNameForDebug() );

	const void* baseMemory = dma->GetBufferBase();
	RED_FATAL_ASSERT( baseMemory != nullptr, "Trying to load serialization stream from a file '%ls' that has no data", reader.GetFileNameForDebug() );

	const CFileDirectSerializationTables* tables = reader.QuerySerializationTables();
	RED_FATAL_ASSERT( tables != nullptr, "Trying to load serialization stream from a file '%ls' that has no dependency tables", reader.GetFileNameForDebug() );

	const Uint64 currentOffset = reader.GetOffset();
	const void* objectMemory = OffsetPtr( baseMemory, currentOffset );

	parser.ParseStream( reader, *tables, objectMemory, (Uint32)(dma->GetBufferSize() - currentOffset), targetPtr, targetClass );
}
