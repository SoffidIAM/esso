
/**
 * Cifrador / descifrador AES
 */

#define AESKEY_SIZE 16
#define AESCHUNK_SIZE 16
#define EXPKEY_SIZE 176
// produce Nb bytes for each round
void AESExpandKey (unsigned char *key, unsigned char *expkey);
// encrypt one 128 bit block
void AESEncrypt (unsigned char *in, unsigned char *expkey, unsigned char *out);
// decrypt one 128 bit block
void AESDecrypt (unsigned char *in, unsigned char *expkey, unsigned char *out);
