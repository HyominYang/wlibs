#ifndef CONFR_H__
#define CONFR_H__

#include <iostream>
#include <fstream>

using namespace std;
using std::ofstream;
using std::ifstream;
using std::ios;

class WConf
{
public:
	WConf();
	~WConf();
	bool read(string);
	bool backup();
	string & operator [] (string);
private:
	ifstream m_rd;
	string m_name_list[100];
	string m_name_val[100];
	int m_index_max;
	string nil;
	string m_filepath;
};

#endif
