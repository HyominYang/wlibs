#include "confr.h"

WConf::WConf()
{
	m_index_max = 0;
	nil = "";
}

WConf::~WConf()
{
	if(m_rd.is_open()) {
		m_rd.close();
	}
}

bool WConf::read(string filepath)
{
	m_rd.open(filepath.data(), ios::in);
	if(!m_rd.is_open()) {
		return false;
	}

	int offset=0;

	m_filepath = filepath;
	while(!m_rd.eof())
	{
		string line;
		m_rd>>line;
		if(line == "") { continue; }
		else if(line.at(0) == '#') { continue; }
		else {
			size_t pos = line.find("=");
			if(pos == string::npos) { continue; }
			m_name_list[offset] = line.substr(0, pos);
			m_name_val [offset++] = line.substr(pos+1);
			if(offset == 100) {
				break;
			}
		}
	}
	m_rd.close();

	m_index_max = offset;

	return true;
}

#include <time.h>
#include <stdio.h>

bool WConf::backup()
{
	time_t now = time(0);
	struct tm t;
	char buff[50];

	localtime_r(&now, &t);
	snprintf(buff, 50, ".%d%d%d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

	string new_filepath = m_filepath + buff;

	ifstream srce(m_filepath.data(), std::ios::binary);
	ofstream dest(new_filepath.data(), std::ios::binary);
	dest << srce.rdbuf();

	return true;
}

string & WConf::operator [] (string name)
{
	for(int i=0; i<m_index_max; i++) {
		if(m_name_list[i] == name) {
			return m_name_val[i];
		}
	}
	return nil;
}

#ifdef TEST_MAIN__

int main(int argc, char **argv)
{
	WConf conf;

	if(conf.read("test.conf") == false) {
		cout<<"file open error!!"<<endl;
		return -1;
	}

	cout<<conf["test"]<<endl;
	cout<<conf["value"]<<endl;
	cout<<conf["siibal"]<<endl;

	conf.backup();

	return 0;
}

#endif
