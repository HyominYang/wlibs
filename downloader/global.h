#include <wesock.h>

#ifndef __GLOBAL_H__
#define __GLOBAL_H__


#define BUFF_MAX 1024 * 1024 * 10
#define DEBUG__ printf("%s:%d\n", __func__, __LINE__)

struct msg_header
{
	int len;
	int command;
}__attribute__((packed));
typedef struct msg_header msg_header_t;

enum {
	REQ_GET_LICENCE=0,
	REQ_GET_VERSION,
	REQ_CHECK_LICENCE,

	REQ_DOWNLOAD_PACK,
	RES_DOWNLOAD_PACK,

	RES_OK,
	RES_FAILED,
};

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

#endif
