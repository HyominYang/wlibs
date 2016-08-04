//#include "global.h"

#include <iostream>
#include <fstream>
using namespace std;
using std::ofstream;
using std::ifstream;

#define SEED "830916"
#define PROC_LIST_MAX 10

string proc_list[PROC_LIST_MAX];
int proc_list_max;
string my_mac;

#define PROC_LIST_FILEPATH "proc_list.lst"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <string.h>
string get_mac()
{
	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	char buff[20];

	if(fd < 0) {
		cout<<"socket open is failed."<<endl;
		return "<none>";
	}
	strcpy(s.ifr_name, "eth0");
	if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
		sprintf(buff, "%.02X%.02X%.02X%.02X%.02X%.02X",
				(unsigned char)s.ifr_hwaddr.sa_data[0],
				(unsigned char)s.ifr_hwaddr.sa_data[1],
				(unsigned char)s.ifr_hwaddr.sa_data[2],
				(unsigned char)s.ifr_hwaddr.sa_data[3],
				(unsigned char)s.ifr_hwaddr.sa_data[4],
				(unsigned char)s.ifr_hwaddr.sa_data[5]);
		my_mac = buff;
	} else {
		close(fd);
		return "<none>";
	}
	close(fd);

	return buff;
}

int read_proc_list()
{
	ifstream rd;
	rd.open(PROC_LIST_FILEPATH, ios::in);

	if(!rd.is_open()) {
		cout<<PROC_LIST_FILEPATH<<" cannot open."<<endl;
		return -1;
	}
	
	int i=0;
	while(!rd.eof()) {
		string line;
		rd>>line;
		if(line == "") {
			//cout<<"<blank>"<<endl;
		} else {
			proc_list[i++] = line;
		}
	}
	rd.close();

	proc_list_max = i;

	for(i=0; i<proc_list_max; i++) {
		cout<<"PROC["<<i<<"] "<<proc_list[i]<<endl;
	}

	return 0;
}

int main(int argc, char **argv)
{
	get_mac();
	if(read_proc_list() < 0) { return -1; }

	cout<<"MAC: "<<my_mac<<endl;
	return 0;
}
