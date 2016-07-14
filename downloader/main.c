#include <wesock.h>

int main(int argc, char **argv)
{
	struct wsock_table table;
	struct wsock *wsock;
	pthread_t pid;
	char *buff;
	int buff_len, i;
	buff = malloc(1024 * 1024 * 10);
	if(buff == NULL) {
		printf("malloc() is failed.\n");
		return -1;
	}

	return 0;
}
