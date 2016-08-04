#include <stdio.h>
typedef unsigned char Byte;
extern int encrypt(Byte *mk, const char *src, const char *dst);
extern int decrypt_fm(Byte *mk, const char *src, char *dst, int *wrote_len);

int main()
{
	unsigned char *mk = "goodknight";
	int wrote_len;
	char buff[100], buff2[100];
	encrypt(mk, "a.txt", "b.txt");
	decrypt_fm(mk, "b.txt", buff2, &wrote_len);
	buff2[wrote_len] = '\0';

	printf("DATA: %s\n", buff2);
	return 0;
}
