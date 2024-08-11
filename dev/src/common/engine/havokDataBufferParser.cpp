/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "havokDataBufferParser.h"

#ifdef USE_HAVOK_ANIMATION
template < typename T >
struct _DummyArray
{
	T* data;
	int size;
};

CHavokDataBufferParser::CHavokDataBufferParser()
{
	Clear();
}

CHavokDataBufferParser::CHavokDataBufferParser( const HavokDataBuffer& data )
{
	SetRoot( data );
}

void CHavokDataBufferParser::Clear()
{
	m_root.m_object = NULL;
	m_root.m_class = NULL;
}

const hkVariant& CHavokDataBufferParser::SetRoot( const HavokDataBuffer& data )
{
	m_root.m_object = const_cast< void* >( data.GetHavokObjectData() );
	m_root.m_class = data.GetHavokClass();
	return m_root;
}

const hkVariant& CHavokDataBufferParser::SetRoot( const HavokDataBuffer* data )
{
	m_root.m_object = const_cast< void* >( data->GetHavokObjectData() );
	m_root.m_class = data->GetHavokClass();
	return m_root;
}

const hkVariant& CHavokDataBufferParser::GetRoot() const
{
	return m_root;
}

void CHavokDataBufferParser::GetItemData( const hkVariant& var, hkString& name, hkArray< hkVariant >& children ) const
{
	ASSERT( m_root.m_object != NULL );
	ASSERT( m_root.m_class != NULL );

	name ="";
	children.clear();

	// Generic klass walker
	const hkClass& klass = *var.m_class;
	const void* obj = var.m_object;
	for (int mi=0; mi < klass.getNumMembers(); ++mi)
	{
		const hkClassMember& m = klass.getMember(mi);
		switch ( m.getType() )
		{
		case hkClassMember::TYPE_VARIANT:
			{
				hkVariant& mvar = *(hkVariant*)(((char*)obj) + m.getOffset()); // ptr to a variant
				if (mvar.m_object && mvar.m_class)
				{
					hkVariant& var = children.expandOne();
					HK_ASSERT( 0x0, mvar.m_class );
					var = mvar;
				}
				break;
			}

		case hkClassMember::TYPE_STRUCT:
			{
				const hkClass* c = m.hasClass()? &(m.getStructClass()) : HK_NULL;
				if (c)
				{
					// EXP-980 : Handle fixed size C arrays of structs
					int size = m.getCstyleArraySize();

					// Pretend it's a C-array of 1 even if it's not an array
					if (size==0) size=1;

					char* arrayStart = ((char*)obj) + m.getOffset(); // ptr to a struct
					for (int ai=0; ai<size; ai++)
					{
						char* objectStart = arrayStart + c->getObjectSize() * ai;

						hkVariant& var = children.expandOne();

						var.m_object = objectStart;
						var.m_class = c;
					}

				}
				break;
			}

		case hkClassMember::TYPE_POINTER:
			{
				if (m.getSubType() >= hkClassMember::TYPE_POINTER)
				{
					void* vobj = *(void**)( ((char*)obj) + m.getOffset() ); // ptr to a ptr.
					if (vobj)
					{
						HK_ASSERT( 0x0, m.hasClass() );
						const hkClass& c = m.getStructClass();
						hkVariant& var = children.expandOne();
						var.m_class = &c;
						var.m_object = vobj;
					}
				}
				break;
			}

			// EXP-650 : C strings are now a new type
		case hkClassMember::TYPE_CSTRING:
			{
				if ( hkString::strCmp( m.getName(), "name" ) == 0 ) // any member that is called name that is a char* ptr we will uses as the name ;)
				{
					char* vobj = *(char**)( ((char*)obj) + m.getOffset() ); // ptr to a ptr.
					name = vobj; // should be null terminated.
				}
				break;
			}

		case hkClassMember::TYPE_ARRAY: //hkarray<>
		case hkClassMember::TYPE_INPLACEARRAY:
		case hkClassMember::TYPE_SIMPLEARRAY: // void*,int
			{
				const hkClass* c = m.hasClass()? &(m.getStructClass()) : HK_NULL;
				_DummyArray<char>* array = (_DummyArray<char>*)( ((char*)obj) + m.getOffset() );

				for (int ai=0; ai < array->size; ++ai)
				{
					char* bytePtr = array->data + (m.getArrayMemberSize()*ai);
					if (m.getSubType() == hkClassMember::TYPE_VARIANT)
					{
						hkVariant& avar = *(hkVariant*)( bytePtr ); // ptr to a variant
						if (avar.m_object && avar.m_class)
						{
							hkVariant& var = children.expandOne();
							var = avar;
						}
					}
					else if (m.getSubType() == hkClassMember::TYPE_STRUCT)
					{
						void* vobj = bytePtr; // ptr to a struct
						if (vobj)
						{
							hkVariant& var = children.expandOne();
							var.m_object = vobj;
							var.m_class = c;
							HK_ASSERT( 0x0, c );

						}
					}
					else if (m.getSubType() == hkClassMember::TYPE_POINTER)
					{
						void* vobj = *(void**)(bytePtr); // ptr to ptr
						if (vobj)
						{
							hkVariant& var = children.expandOne();
							var.m_object = vobj;
							var.m_class = c;
							HK_ASSERT( 0x0, c );

						}
					}
				}
				break;
			}
		}
	}

	// use address if not named
	if( name.getLength() < 1 )
	{
		name.printf("%s @ 0x%x", klass.getName(), obj);
	}
}

#endif
