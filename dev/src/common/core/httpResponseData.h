
#include "basicDataBlob.h"

//---------------------------------

/// HTTP response data - raw data, no header
class CHTTPResponseData : public Red::NonCopyable
{
public:
	enum EResultCode
	{
		eResultCode_OK = 200,
		eResultCode_BadRequest = 400,
		eResultCode_NotFound = 404,
		eResultCode_ServerError = 500,
		eResultCode_NotImplemented = 501,
	};

	CHTTPResponseData( const StringAnsi& contentType, DataBlobPtr data ); // assumes eResultCode_OK
	CHTTPResponseData( const EResultCode code );
	~CHTTPResponseData();

	// Get result code
	RED_INLINE const EResultCode GetResultCode() const { return m_resultCode; }

	// Get content type
	RED_INLINE const StringAnsi& GetContentType() const { return m_contentType; }

	// Get response data
	RED_INLINE DataBlobPtr GetData() const { return m_data; }

private:
	EResultCode		m_resultCode;
	StringAnsi		m_contentType;
	DataBlobPtr		m_data;
};

//---------------------------------
