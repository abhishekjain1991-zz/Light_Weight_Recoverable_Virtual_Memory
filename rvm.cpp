
/* 
 * File:   rvm.cpp
 * Author: abhishekjain91
 *
 * Created on April 15, 2015, 8:06 PM
 */

#include "rvm.h"



using namespace std;


unordered_map<int,rvm*> trans_map;

int file_exist (const char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void rvm::create_directory(const char* directory)
{
    //cout<<"in create directory"<<endl;
    if(!file_exist(directory))
    {
            if(mkdir(directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
            {
                cout<<"Failed to create directory\n";
                exit(1);
            }
    }
        
}

rvm_t rvm_init(const char *directory)
{
    
    return rvm_t(directory);
}


void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
    //check if same process is calling twice, return an error
    if(rvm.ds->segments.find(string(segname))!=rvm.ds->segments.end() && rvm.ds->segments.find(string(segname))->second.mapped==1)
    {
        cout<<"MAPPING SAME SEGMENT TWICE !! NOT ALLOWED EXITING \n"<<endl;
        exit(1);
    }
    if(rvm.ds->segments.find(string(segname))!=rvm.ds->segments.end())
	{
	if(size_to_create>rvm.ds->segments.find(string(segname))->second.current_size)
	{
	
	char *temp_buffer=new char[size_to_create];
	memcpy((void*) temp_buffer,(void*)rvm.ds->segments.find(string(segname))->second.buf_ptr,rvm.ds->segments.find(string(segname))->second.current_size);
	rvm.ds->ptr_to_seg.erase(rvm.ds->segments.find(string(segname))->second.buf_ptr);
	delete rvm.ds->segments.find(string(segname))->second.buf_ptr;
	rvm.ds->segments.find(string(segname))->second.buf_ptr=temp_buffer;
	rvm.ds->ptr_to_seg.insert(pair<char*,string>(rvm.ds->segments.find(string(segname))->second.buf_ptr,string(segname)));
	while(rvm.ds->segments.find(string(segname))->second.complete_data.size()<size_to_create)
		rvm.ds->segments.find(string(segname))->second.complete_data.push_back(' ');
	for(int size=rvm.ds->segments.find(string(segname))->second.current_size;size<size_to_create;size++)
		rvm.ds->segments.find(string(segname))->second.buf_ptr[size]=' ';
	}
	rvm.ds->segments.find(string(segname))->second.current_size=size_to_create;
	return rvm.ds->segments.find(string(segname))->second.buf_ptr;
}
    
    seg segment(segname,size_to_create);
    
        //create seg and log file names
    
    segment.log_path=rvm.ds->direct_name+"/"+string(segname)+".log";
    segment.seg_path=rvm.ds->direct_name+"/"+string(segname)+".seg";

    //if not create segment from previous segment if it exists
    if(file_exist(segment.seg_path.c_str()))
    {
	
	rvm_truncate_log(rvm);
	//cout<<"called truncate"<<endl;
        ifstream fin;
        char m_char;
        
           fin.open(segment.seg_path.c_str(), ios::in | ios::binary);
	   fin.unsetf(ios_base::skipws);
            while (fin>>m_char ) {
                //fin.get(m_char);
                segment.complete_data.push_back(m_char);
            }
            fin.close(); 
        
        //check whose size is bigger , previous  or new 
        if(segment.complete_data.size()>size_to_create)
            segment.current_size=segment.complete_data.size();
    }
    else//create a segment file
    {
        FILE *segment_file;
        segment_file= fopen (segment.seg_path.c_str(),"w");
        fclose(segment_file);
    }
    //store complete data
    while(segment.complete_data.size()<segment.current_size)
	segment.complete_data.push_back(' ');
    
    
    //create buffer
    //check if need to store extra space for null
    char *buffer=new char[segment.current_size];
    
    //cout<<"SEGMENT SIZE IS "<<segment.current_size<<endl;
    
    
    //copy existing data if any into buffer
    if(segment.complete_data.size())
    {
    vector<char>::iterator c_it=segment.complete_data.begin();
	int i;
    for( i=0;i<segment.current_size && c_it!=segment.complete_data.end();i++,c_it++)
    {
	
	
        buffer[i]=*c_it;
    }
	//buffer[i]='\0';
    }

    //reverse map buffer pointer to segment name
    rvm.ds->ptr_to_seg.insert(pair<char*,string>(buffer,string(segname)));
    
    //store buffer pointer in corresponding segment data structure
    segment.buf_ptr=buffer;
    

    //put it in rvm object's map
    rvm.ds->segments.insert(pair<string,seg>(string(segname),segment));
    
    
    //return segment name
    return (void*)buffer;
    
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
    //check if segment is unmapped
    if(rvm.ds->segments.find(string(segname))==rvm.ds->segments.end() || rvm.ds->segments.find(string(segname))->second.mapped==0)
    {
        //delete log file 
	rvm.ds->segments.erase(string(segname));
	//cout<<"deleteing log and seg files"<<endl;
        string path=rvm.ds->direct_name+"/"+string(segname)+".log";
        if(file_exist(path.c_str()))
        {
            if( remove(path.c_str()) != 0 )
            {
                cout<<"ERROR WHILE DELETING LOG FILE FOR UNMAPPED SEGMENT !!!!  "<<endl;
                exit(1);
            }
        }
  
        //delete segment file
        path=rvm.ds->direct_name+"/"+string(segname)+".seg";
        if(file_exist(path.c_str()))
        {
            if( remove(path.c_str()) != 0 )
            {
                cout<<"ERROR WHILE DELETING SEG FILE FOR UNMAPPED SEGMENT !!!!  "<<endl;
                exit(1);
            }
        }
    }
}


trans_t rvm_begin_trans(rvm_t rvm_ob, int numsegs, void **segbases)
{
    int index;
    //check if any transaction is going on in any segment mentioned in segbase
    string buffer;
    for(index=0;index<numsegs;index++)
    {
        buffer=rvm_ob.ds->ptr_to_seg.find((char*)segbases[index])->second;
        if(rvm_ob.ds->transactions.find(buffer)!=rvm_ob.ds->transactions.end())
            return (trans_t)-1;
    }
    
    //if no segment in segbases is being used anywhere else, increment tid and assign it to 
    //all segments in segbases
    rvm_ob.ds->tid++;
    index=0;
    for (index=0;index<numsegs;index++)
    {
        buffer=rvm_ob.ds->ptr_to_seg.find((char*)segbases[index])->second;
        rvm_ob.ds->transactions.insert(buffer);
        rvm_ob.ds->segments.find(buffer)->second.tid=rvm_ob.ds->tid;
    }
    trans_map.insert(pair<int,rvm*>(rvm_ob.ds->tid,rvm_ob.ds));
    return rvm_ob.ds->tid;
}



void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
    //check if such a tid even exists
    if(trans_map.find(tid)==trans_map.end())
    {
        cout<<"BOGUS TRANSACTION ID !!!"<<endl;
        exit(1);
    }
    //check if segment actually belongs to this transaction id or not
    //check if segment actually exists
    //check if offset +size < segment's current size
    //if not return error
    rvm *rvm_ptr=trans_map.find(tid)->second;
    string base=rvm_ptr->ptr_to_seg.find((char*)segbase)->second;
    //cout<<"initially this guy is "<<rvm_ptr->segments.find(base)->second.buf_ptr<<endl;
    
    
    if(rvm_ptr->segments.find(base)!= rvm_ptr->segments.end() && 
            rvm_ptr->segments.find(base)->second.tid==tid  &&
            offset+size<=rvm_ptr->segments.find(base)->second.current_size)
    {
        //indicate about to commit called 
        rvm_ptr->segments.find(base)->second.a_to_m_offset=size;

        //if not change current segments offset and modification size
        rvm_ptr->segments.find(base)->second.m_mods.push_back(mods(size,offset));
	
    }
    else
    {
        cout<<"RVM ABOUT TO MODIFY FAILED FOR SOME REASON"<<endl;
        exit(1);
    }
    
    
}


void rvm_commit_trans(trans_t tid)
{
    //get the rvm object
    rvm *rvm_ptr=trans_map.find(tid)->second;
    
    stack<string> tr_stack;
    //find all segments with the transaction id mentioned as arguments
    unordered_set<string>::iterator ts=rvm_ptr->transactions.begin();
    while(ts!=rvm_ptr->transactions.end())
    {
        if(rvm_ptr->segments.find(*ts)->second.tid==tid)
            tr_stack.push(*ts);
        ts++;
    }
    
    //for each segment that we got above
    //1)erase them from rvm's transaction map
    //2)create a log file and insert the transaction
    //3)reset the transaction id.. can do without it
    while(tr_stack.size())
    {
        //get segment's original data
        int index=0;
        char *r_buf=rvm_ptr->segments.find(tr_stack.top())->second.buf_ptr;
       
	    //clear prev segment data
        if(!(rvm_ptr->segments.find(tr_stack.top())->second.a_to_m_offset==0))
        {
	       //do nothing and move on
        }
        else
        {
            int index=0;
            while(index<rvm_ptr->segments.find(tr_stack.top())->second.current_size)
            {
                rvm_ptr->segments.find(tr_stack.top())->second.buf_ptr[index]=rvm_ptr->segments.find(tr_stack.top())->second.complete_data[index];
                index++;
            }
            rvm_ptr->transactions.erase(tr_stack.top());
            tr_stack.pop();
            continue;

        }
        //create a log file
        string log_path=rvm_ptr->segments.find(tr_stack.top())->second.log_path;
        FILE *log_file;
        log_file= fopen (log_path.c_str(),"a");
        //flush data to log file

	    vector<mods>::iterator  it = rvm_ptr->segments.find(tr_stack.top())->second.m_mods.begin();
	    while(it!=rvm_ptr->segments.find(tr_stack.top())->second.m_mods.end())
	    {
		fprintf(log_file, "%d\n", (*it).offset);
		fprintf(log_file, "%d\n", (*it).size);
		for(int i=(*it).offset;i<(*it).size+(*it).offset;i++)
		{
			fputc(rvm_ptr->segments.find(tr_stack.top())->second.buf_ptr[i],log_file);
            rvm_ptr->segments.find(tr_stack.top())->second.complete_data[i]=rvm_ptr->segments.find(tr_stack.top())->second.buf_ptr[i];
		}
		fprintf(log_file, "\n");
		it++;
	    }
        fclose(log_file);

        //remove all mods that have been commited from virtual memory (experimental)
        rvm_ptr->segments.find(tr_stack.top())->second.m_mods.clear();

        //change about to commit flag
        rvm_ptr->segments.find(tr_stack.top())->second.a_to_m_offset=0;

        //remove transaction related data to that segment
        rvm_ptr->transactions.erase(tr_stack.top());
        tr_stack.pop();


    }
}


void rvm_abort_trans(trans_t tid)
{
    //get the rvm object
    rvm *rvm_ptr=trans_map.find(tid)->second;
    
    stack<string> tr_stack;
    //find all segments with the transaction id mentioned as arguments
    unordered_set<string>::iterator ts=rvm_ptr->transactions.begin();
    while(ts!=rvm_ptr->transactions.end())
    {
        if(rvm_ptr->segments.find(*ts)->second.tid==tid)
            tr_stack.push(*ts);
        ts++;
    }
    


    //jsut remove them from the transaction map and restore segment to original state
    while(tr_stack.size())
    {
	
	int index=0;
	
	while(index<rvm_ptr->segments.find(tr_stack.top())->second.current_size)
	{
		rvm_ptr->segments.find(tr_stack.top())->second.buf_ptr[index]=rvm_ptr->segments.find(tr_stack.top())->second.complete_data[index];
		index++;
	}	
        rvm_ptr->transactions.erase(tr_stack.top());
        tr_stack.pop();
    }
}


void rvm_unmap(rvm_t rvm, void *segbase)
{

    //find segment name corresponding to buffer and remove it from segments db
    //rvm.ds->segments.erase(rvm.ds->ptr_to_seg.find((char*)segbase)->second);
    rvm.ds->segments.find(rvm.ds->ptr_to_seg.find((char*)segbase)->second)->second.mapped=0;
    
    //leave log files and segment files untouched
}

void rvm_truncate_log(rvm_t rvm)
{
    string dir_name=rvm.ds->direct_name;
    
    
    if(file_exist(dir_name.c_str()))
    {
	vector<char> seg_data;
        vector<string> log_file_paths;
        vector<string> seg_file_paths;
        //check for all log files in the directory
        string lname=dir_name+"/"+"*.log";
        glob_t globbuf;
        string og;
        string re="seg";
        glob(lname.c_str(),NULL,NULL,&globbuf);
        for (size_t i = 0; i < globbuf.gl_pathc; i++)
        {
            log_file_paths.push_back(string(globbuf.gl_pathv[i]));
            og=string( globbuf.gl_pathv[i]);
            og.replace(strlen(og.c_str())-3,3,re);
            seg_file_paths.push_back(og);
        }
        globfree(&globbuf);

	//for each log file read corresponding segment file, apply changes and delete log file

	for(int i=0;i<log_file_paths.size();i++)
	{
		
		ifstream fin;
		int offset;
		int size;
        	char m_char;
		int data_index=0;
		
		fin.open(seg_file_paths[i].c_str(),ios::in|ios::binary);
		while (fin>>m_char ) 
		{	
			seg_data.push_back(m_char);
		}
		fin.close();
		fin.open(log_file_paths[i].c_str(),ios::in|ios::binary);
		
		while(fin>>offset)
		{
			

			fin>>size;

			if(offset+size>seg_data.size())
			{
				while(seg_data.size()<offset+size)
				{
						seg_data.push_back(' ');
				}
			}
			fin.unsetf(ios_base::skipws);
			fin>>m_char;
			for(data_index=offset;data_index<offset+size;data_index++)
				{	
					fin>>m_char;
					seg_data[data_index]=m_char;

				}
			fin.setf(ios_base::skipws);
			
		}
		fin.close();
		FILE *seg_file;
		seg_file=fopen(seg_file_paths[i].c_str(),"w");
		data_index=0;
		while(data_index<seg_data.size())
			{
				fputc(seg_data[data_index],seg_file);
				data_index++;
			}
		fputc('\n',seg_file);
		seg_data.clear();
		fclose(seg_file);
	}
		
	for(int i=0;i<log_file_paths.size();i++)
        {
            if( file_exist(log_file_paths[i].c_str() ) && remove( log_file_paths[i].c_str() ) != 0 )
            {
		//cout<<"trying to delete "<<log_file_paths[i].c_str()<<endl;
                cout<<"SOMETHING WENT WRONG WHILE TRUNCATING SEG FILES "<<endl;
                exit(1);
            }
        }

	} 

    else
    {
        cout<<"NO SUCH BACKING STORE EXISTS !!!!"<<endl;
        exit(1);
    }
}



