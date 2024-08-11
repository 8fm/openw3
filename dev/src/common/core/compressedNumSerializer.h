#pragma once

class CCompressedNumSerializer
{
private:
	Int32&		m_num;

public:
#ifdef RED_ARCH_X64
	CCompressedNumSerializer( size_t& num ) : m_num( reinterpret_cast<Int32&>( num ) ){};
#endif
	CCompressedNumSerializer( Uint32& num ) : m_num( reinterpret_cast<Int32&>( num ) ){};
	CCompressedNumSerializer( Int32& num ) : m_num( num ){};

	
	friend IFile& operator<<( IFile& file, CCompressedNumSerializer c );

	CCompressedNumSerializer &operator=( CCompressedNumSerializer& c )
	{
		m_num = c.m_num;
		return *this;
	}
};

IFile& operator<<( IFile& file, CCompressedNumSerializer c );
