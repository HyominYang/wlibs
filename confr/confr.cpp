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

	conf.read("test.conf");

	cout<<conf["test"]<<endl;
	cout<<conf["value"]<<endl;
	cout<<conf["siibal"]<<endl;

	return 0;
}

#endif
