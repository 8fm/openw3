#pragma once
namespace Telemetry
{
	class IFileReadCallBack
	{
	public:

		//! virtual destructor
		virtual ~IFileReadCallBack() {};

		//! Reads an amount of bytes from the file.
		/** \param buffer: Pointer to buffer where to read bytes will be written to.
		\param sizeToRead: Amount of bytes to read from the file.
		\return Returns how much bytes were read. */
		virtual int read(void* buffer, int sizeToRead) = 0;

		//! Returns size of file in bytes
		virtual int getSize() = 0;
	};

	class IFileReadCallBackManager
	{
	public:
		virtual IFileReadCallBack* openFile( const_utf8 filePath ) = 0;
		virtual void   closeFile( IFileReadCallBack* file ) = 0;
	};

	struct GFileManager
	{
		static IFileReadCallBackManager* fileManager;
	};
}