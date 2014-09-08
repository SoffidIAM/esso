#include <windows.h>
#include <resource.h>

#include "nossoriDialog.h"

class NossoriHandler {
public:
	bool performRequest ();
	bool responseChallenge ();
	bool resetPassword ();

	bool perform();

private:
	NossoriDialog dialog;
	std::wstring domain;
	std::wstring requestId;
};
