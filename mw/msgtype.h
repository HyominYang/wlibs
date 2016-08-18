#ifndef MSG_TYPE_H__
#define MSG_TYPE_H__
struct msg_header
{
	int len;
	int command;
}__attribute__((packed));
typedef struct msg_header msg_header_t;

struct msg_param
{
	int len;
}__attribute__((packed));
typedef struct msg_param msg_param_t;

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
