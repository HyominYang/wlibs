
#include <wesock.h>
void create_network_procedure()
{
	pthread_create(&pid, NULL, f_runner, (void *) &table);
}
