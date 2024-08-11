#ifndef GRB_ERROR_REPORTER_H
#define GRB_ERROR_REPORTER_H

struct GrbErrors
{
	enum
	{
		GRB_OUT_OF_MEMORY = 0,
		GRB_INVALID_PARAMETER
	};
};

class GrbErrorReporter
{
public:

	virtual void reportError(int code, const char * message, const char * file, int line) = 0;
};

#endif