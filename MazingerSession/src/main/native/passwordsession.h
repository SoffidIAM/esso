#ifndef _PASSWORD_SESSION_H

#define _PASSWORD_SESSION_H

#include <ssoclient.h>

class PasswordIterator : public SeyconServiceIterator
{
	public:
		PasswordIterator (SeyconSession *s, bool prepare)
		{
			this->s = s;
			this->prepare = prepare;
		}
		virtual ServiceIteratorResult iterate (const char* hostName, size_t dwPort);
		virtual ~PasswordIterator ();
	private:
		SeyconSession *s;
		bool prepare;
};

#endif

