#include <unordered_map>
#include <iostream>
#include <string>
#include <string.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <stack>
#include <glob.h>

typedef int trans_t;

class mods
{
public:
	int size;
	int offset;
	char *data;
	mods(int s, int o):size(s),offset(o)
	{
		data=NULL;
	}
};
class seg
{
public:
    int current_size;
    std::string name;
    std::vector<char> complete_data;//used for storing data from pre-existing log or segment files
    std::vector<char> new_data; //data in segment or log
    char *buf_ptr;
    int mapped;
    int destroyed;
    int tid;
    int a_to_m_offset;
    std::vector<mods> m_mods;
    std:: string log_path;
    std:: string seg_path;
    seg(const char* s_name, int size):name(std::string(s_name)),current_size(size),mapped(1),destroyed(0)
    {
        tid=0;
        a_to_m_offset=0;
    }
	
};
class rvm
{
public:
    std::string direct_name;
    std::unordered_map<std::string,seg> segments;
    std::unordered_set<std::string> transactions;
    std::unordered_map<char*, std::string> ptr_to_seg;
    trans_t tid;
    void create_directory(const char* name);
    rvm() {
    
    }
    ~rvm(){}
    
    
};

class rvm_t
{
    
public:
    rvm *ds;
    rvm_t(){

    }
    rvm_t(const char *directory)
    {
        ds=new rvm;
        ds->direct_name=std::string(directory);
        ds->create_directory(directory);
        ds->tid=0;

    }
    rvm_t(const rvm_t &rhs)
    {
        ds=rhs.ds;
    }
    ~rvm_t(){}
    
};



extern std::unordered_map<int,rvm*> trans_map;
