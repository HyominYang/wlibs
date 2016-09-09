#include <iostream>
#include <fstream>

#include <confr.h>

#include "defines.h"
#include "msgtype.h"
#include "state.h"

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
#ifdef MAKE_LIC
	if(make_lic() < 0) {
		cout<<"\nGenerate a license is failed!. Need a 'license.dec'\n"<<endl;
	} else {
		cout<<"\nGenerate a license is succeed.\n"<<endl;
	}
	return 0;
#endif

	if(read_proc_list() < 0) { return -1; }

	if(create_network_procedure() < 0) {
		cout<<"create_network_procedure() error."<<endl;
	}

	state_machine();

	return 0;
}


int g_connected = 0;
int g_state = NONE;
WConf g_conf, g_ver;

#define CONF_FILEPATH "./test.conf"
#define VER_FILEPATH "/home/wind/.kds_ver"
void state_machine()
{
	struct tm t;
	time_t curr_time;
	int bak_day = 0;

	int correct = 0;
	sleep(1);
	cout<<"\n\n"<<endl;
	while(1) {
		sleep(3);
#if 0
		struct tm {
			int tm_sec;         /* seconds */
			int tm_min;         /* minutes */
			int tm_hour;        /* hours */
			int tm_mday;        /* day of the month */
			int tm_mon;         /* month */
			int tm_year;        /* year */
			int tm_wday;        /* day of the week */
			int tm_yday;        /* day in the year */
			int tm_isdst;       /* daylight saving time */
		};
#endif
		// GET CURRENT DATETIME
		curr_time = time(0);
		localtime_r(&curr_time, &t);
		if(bak_day != t.tm_yday) {
			cout<<"DATE: "<<t.tm_year+1900<<"-"<<t.tm_mon+1<<"-"<<t.tm_mday<<endl;
			bak_day = t.tm_yday;
			g_state = NONE; // init state
		}
		// --- end GET CURRENT DATETIME

		if(g_state == NONE) {
			g_state = LOAD_CONF;
		}
		else if(g_state == LOAD_CONF) {
			if(g_conf.read(CONF_FILEPATH) == true) {
				if(g_conf["client"] == "lotteria") {
					g_state = VERSION_CHECK;
				} else {
					g_state = LICENSE_CHECK;
				}
			}
		}
		else if(g_state == LICENSE_CHECK) {
			struct license lic;
			if(get_lic(lic) < 0) {
				cout<<"Get a lisence is failed."<<endl;
			} else {
				cout<<"LIC: "<<lic.mac<<" - "<<lic.code<<" - "<<lic.info<<endl;
				string mac = get_mac();
				cout<<"MAC: "<<mac<<endl;
				if(lic.mac == mac && lic.code == "valid") {
					cout<<"A license is correct"<<endl;
					g_state = VERSION_CHECK;
				} else {
					cout<<"A license is incorrect."<<endl;
					g_state = LICENSE_GET;
				}
			}
		}
		else if(g_state == VERSION_CHECK) {
			if(g_ver.read(VER_FILEPATH) == true) {
				cout<<"KDS version: "<<g_ver["version"]<<endl;
			} else {
				cout<<"Check version is failed."<<endl;
			}
			g_state = VERSION_CHECK_GET;
		}
		else if(g_state == SERVICE_CHECK) {
			cout<<"Check Services...."<<endl;
		}
	}
}




