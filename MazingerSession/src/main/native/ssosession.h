#ifndef _SSO_SESSION_H

#define _SSO_SESSION_H

#include <ssoclient.h>

class KerberosIterator : public SeyconServiceIterator
{
	public:
		KerberosIterator (SeyconSession *s)
		{
			this->s = s;
		}
		virtual ServiceIteratorResult iterate (const char* hostName, size_t dwPort);
		virtual ~KerberosIterator ();
	private:
		SeyconSession *s;
};
#endif
