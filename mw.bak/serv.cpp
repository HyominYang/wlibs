#include <iostream>
#include <fstream>
using namespace std;
using std::ofstream;
using std::ifstream;

#define SEED "SMARTKDSC8424334"
#define PROC_LIST_MAX 10

struct license {
	string mac;
	string code;
	string info;
};

string proc_list[PROC_LIST_MAX];
int proc_list_max;

#define PROC_LIST_FILEPATH "proc_list.lst"
#define LIC_FILEPATH "license.lic"
#define LIC_DEC_FILEPATH "license.dec"

// extern aria.cpp
#ifndef BYTE_WORD
#define BYTE_WORD
typedef unsigned char Byte;
typedef unsigned int  Word;
#endif

extern int decrypt_fm(Byte *mk, const char *src, char *dst, int *wrote_len);
extern int decrypt(Byte *mk, const char *src, const char *dst);
extern int encrypt(Byte *mk, const char *src, const char *dst);
// end

#include <string.h>
int make_lic()
{
	if(encrypt((Byte *)SEED, LIC_DEC_FILEPATH, LIC_FILEPATH) < 0) {
		cout<<"Make license is failed."<<endl;
		return -1;
	}

	return 0;
}

int get_lic(struct license &lic)
{
	char buff[255];
	int wrote_len;
	char * tmp;
	char * token;
	if(decrypt_fm((Byte *)SEED, LIC_FILEPATH, buff, &wrote_len) < 0) {
		return -1;
	}
	buff[wrote_len-1] = '\0';

	token = strtok_r(buff, "|", &tmp);
	if(token != NULL) {
		lic.mac = token;
	} else {
		return -1;
	}

	token = strtok_r(NULL, "|", &tmp);
	if(token != NULL) {
		lic.code = token;
	} else {
		return -1;
	}

	token = strtok_r(NULL, "|", &tmp);
	if(token != NULL) {
		lic.info = token;
	} else {
		lic.info = "<unknown>";
	}
	return 0;
}

#include <unistd.h>
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

void state_machine();
extern int create_network_procedure();
int main(int argc, char **argv)
{
#if 0
#ifdef MAKE_LIC
	if(make_lic() < 0) {
		cout<<"\nGenerate a license is failed!. Need a 'license.dec'\n"<<endl;
	} else {
		cout<<"\nGenerate a license is succeed.\n"<<endl;
	}
	return 0;
#endif

	if(read_proc_list() < 0) { return -1; }
#endif

	if(create_network_procedure() < 0) {
		cout<<"create_network_procedure() error."<<endl;
	}

	state_machine();

	return 0;
}

enum {
	NONE=0, 
	LICENSE_CHECK,
	SERVICE_CHECK,
};

void state_machine()
{
	int state = LICENSE_CHECK;
	int correct = 0;
	sleep(1);
	cout<<"\n\n"<<endl;
	cout<<"A Server is running..."<<endl;
	while(1) {
		sleep(3);
	}
}




