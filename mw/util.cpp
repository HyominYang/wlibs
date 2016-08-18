#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

void hexdump(char *fname, unsigned char *data, int data_len)
{
	FILE *fp;
	int i,j;

	if((fp = fopen(fname, "w+")) == NULL) {
		printf("dump file open is failed.\n");
		return;
	}

	for(i=0; i<data_len;){
		for(j=0; j<8 && i<data_len; i++, j++) {
			fprintf(fp, "%02x ", (int)((unsigned char)data[i]));
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
}

