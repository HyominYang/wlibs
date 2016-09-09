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

struct msg_s
{
	int len;
	int command;
	int data_len;
	union {
		int number;
		char data[256];
		struct {
			int result;
			char data[256];
		} res;
	} d;
}__attribute__((packed));
typedef struct msg_s msg_t;

enum {
	REQ_GET_LICENCE=0,

	REQ_GET_VERSION,
	RES_GET_VERSION,
	REQ_UPDATE,
	RES_UPDATE,

	REQ_CHECK_LICENCE,
	REQ_DOWNLOAD_PACK,
	RES_DOWNLOAD_PACK,
	RES_OK,
	RES_FAILED,
};

#endif
