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

#endif
