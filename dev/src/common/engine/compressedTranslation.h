
#pragma once

#include "compressedBuffers.h"

enum ECompressTranslationType
{
	CT_8 = 0,
	CT_16,
	CT_None,
};

BEGIN_ENUM_RTTI( ECompressTranslationType );
	ENUM_OPTION( CT_8 );
	ENUM_OPTION( CT_16 );
	ENUM_OPTION( CT_None );
END_ENUM_RTTI();

class CompressedTranslation
{
public:
	virtual ECompressTranslationType GetType() const = 0;

	virtual Int32 Size() const = 0;
	virtual Uint32 DataSize() const = 0;
#ifdef USE_HAVOK_ANIMATION
	virtual void AddTranslation( const hkVector4& vec ) = 0;
	virtual void GetTranslation( const Int32 index, hkVector4& vec ) const = 0;
#else
	virtual void AddTranslation( const RedVector4& vec ) = 0;
	virtual void GetTranslation( const Int32 index, RedVector4& vec ) const = 0;
#endif

	virtual void Serialize( IFile& file ) = 0;
	virtual void Init( Float min, Float max, Uint32 size ) = 0;

public:
	static CompressedTranslation* CreateCompressTranslation( ECompressTranslationType type );

	virtual ~CompressedTranslation(){};
};

template< typename T, ECompressTranslationType type >
class TCompressTranslation : public CompressedTranslation
{
	CompressedVectorBuffer< T >		m_buffer;

public:
	virtual ECompressTranslationType GetType() const						{ return type; }

	virtual Int32 Size() const												{ return m_buffer.Size(); }
	virtual Uint32 DataSize() const											{ return m_buffer.DataSize(); }

#ifdef USE_HAVOK_ANIMATION
	virtual void AddTranslation( const hkVector4& vec )						{ m_buffer.AddVector( vec ); }
	virtual void GetTranslation( const Int32 index, hkVector4& vec ) const	{ return m_buffer.GetVector( index, vec ); }
#else
	virtual void AddTranslation( const RedVector4& vec )					{ m_buffer.AddVector( vec ); }
	virtual void GetTranslation( const Int32 index, RedVector4& vec ) const	{ return m_buffer.GetVector( index, vec ); }
#endif
	virtual void Serialize( IFile& file )									{ m_buffer.Serialize( file ); }
	virtual void Init( Float min, Float max, Uint32 size )					{ m_buffer.SetMinMax( min, max ); m_buffer.Reserve( size ); }
};

typedef TCompressTranslation< Uint8, CT_8 > CompressTranslation8;
typedef TCompressTranslation< Uint16, CT_16 > CompressTranslation16;
