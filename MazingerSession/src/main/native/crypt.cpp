#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ssoclient.h>

#include <string>

static char base64Array[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

std::string SeyconCommon::toBase64 (const unsigned char* source, int len)
{
	std::string result;
	int i;
	result.clear();
	for (i = 0; i < len; i += 3)
	{
		int c1, c2, c3;
		int index;
		c1 = source[i];
		if (i + 1 < len)
			c2 = source[i + 1];
		else
			c2 = 0;
		if (i + 2 < len)
			c3 = source[i + 2];
		else
			c3 = 0;
		index = (c1 & 0xfc) >> 2;
		result.append(1, base64Array[index]);
		index = ((c1 & 0x03) << 4) | ((c2 & 0xf0) >> 4);
		result.append(1, base64Array[index]);
		if (i + 1 >= len)
			result.append(1, '=');
		else
		{
			index = ((c2 & 0x0f) << 2) | ((c3 & 0xc0) >> 6);
			result.append(1, base64Array[index]);
		}
		if (i + 2 >= len)
			result.append(1, '=');
		else
		{
			index = (c3 & 0x3f);
			result.append(1, base64Array[index]);
		}
	}
	return result;
}

static int decode64char (unsigned char b)
{
	const char* x = strchr(base64Array, b);
	if (x == NULL)
		return 0;
	else
		return (x - base64Array);
}

std::string SeyconCommon::fromBase64 (const char* source)
{
	std::string result;
	int i;
	int len = strlen((char*) source);
	int len2 = (len / 4) * 3;
	if (source[len - 1] == '=')
	{
		len2--;
		if (source[len - 2] == '=')
			len2--;
	}
//	result = malloc (len2);
	for (i = 0; i < len; i += 4)
	{
		unsigned char c1, c2, c3, c4;
		int c1d, c2d, c3d, c4d;
		c1 = source[i];
		c2 = source[i + 1];
		c3 = source[i + 2];
		c4 = source[i + 3];
		c1d = decode64char(c1);
		c2d = decode64char(c2);
		c3d = decode64char(c3);
		c4d = decode64char(c4);
		result.append(1, (char) ((c1d << 2) | ((c2d & 0x30) >> 4)));
		if (c3 != '=')
		{
			result.append(1, (char) (((c2d & 0x0f) << 4) | ((c3d & 0x3c) >> 2)));
			if (c4 != '=')
			{
				result.append(1, (char) (((c3d & 0x03) << 6) | c4d));
			}
		}
	}
	return result;
}

std::string SeyconCommon::crypt (const char* source)
{
	std::string result;
	unsigned char contents[4096];
	int b1 = (int) floor(256.0 * rand());
	int b2 = (int) floor(256.0 * rand());
	unsigned int i;

	srand((unsigned) time(NULL));
	contents[0] = (char) b1;
	contents[1] = (char) b2;
	for (i = 0; i < strlen(source); i++)
	{
		int c = source[i];
		c = ((c ^ b1) + b2);
		contents[i + 2] = (char) c;
		b1 = (b1 + b2) % 256;
		b2 = b2 ^ c;
	}
	return toBase64(contents, i + 2);
}

std::string SeyconCommon::uncrypt (const char* source)
{
	std::string result;
	std::string contents = fromBase64(source);
	int len = contents.size();
	int b1 = contents.at(0);
	int b2 = contents.at(1);
	int i;
	for (i = 0; i < len - 2; i++)
	{
		int c = contents.at(i + 2);
		while (b2 > c)
			c = c + 256;
		c = c - b2;
		c = (c ^ b1);
		result.append(1, (char) c);
		b1 = (b1 + b2) % 256;
		b2 = (b2 ^ (int) contents.at(i + 2));
	}
	return result;
}

