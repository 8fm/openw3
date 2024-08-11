#pragma once

class ReFileString
{
public:
	ReFileString();
	ReFileString( const char* str );
	ReFileString( const ReFileString& s );
	~ReFileString();

public:
	const char*	getData() const;
	void		set( const char* str );
	void		write( class ReFileBuffer* buf );
	void		read( class ReFileBuffer* buf );
	int			getLength() const { return mLenght; }
	void		Reset();

private:
	char*		mData;
	int			mLenght;

	//OPERATORS
public:
	inline ReFileString& operator= (const ReFileString& str)
	{
		this->set( str.getData() );
		return *this;
	}

	inline bool IsGood() const { return mData != nullptr; }

	inline bool operator < ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) < 0;
	}

	inline bool operator <= ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) <= 0;
	}

	inline bool operator > ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) > 0;
	}

	inline bool operator >= ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) >= 0;
	}

	inline bool operator == ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) == 0;
	}

	inline bool operator != ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) != 0;
	}

	/////
	inline bool operator < ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) < 0;
	}

	inline bool operator <= ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) <= 0;
	}

	inline bool operator > ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) > 0;
	}

	inline bool operator >= ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) >= 0;
	}

	inline bool operator == ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) == 0;
	}

	inline bool operator != ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) != 0;
	}
};