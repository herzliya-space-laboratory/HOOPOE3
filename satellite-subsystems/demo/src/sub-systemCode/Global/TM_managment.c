#include <hal/Timing/Time.h>
#include <hcc/api_fat.h>
#include <hal/errors.h>
#include <hcc/api_hcc_mem.h>
#include <string.h>
#include <hcc/api_mdriver_atmel_mcipdc.h>
#include <hal/Storage/FRAM.h>
#include <at91/utility/trace.h>

#include <stdlib.h>

#include "TM_managment.h"
#include "../Global/Global.h"

#define SKIP_FILE_TIME_SEC 1000000
#define _SD_CARD 0
#define FIRST_TIME -1
#define FILE_NAME_WITH_INDEX_SIZE MAX_F_FILE_NAME_SIZE+sizeof(int)*2

//struct for filesystem info
typedef struct
{
	int num_of_files;
} FS;

//struct for chain file info
typedef struct
{
	int size_of_element;
	char name[FILE_NAME_WITH_INDEX_SIZE];
	unsigned int creation_time;
	unsigned int last_time_modified;
	int num_of_files;

} C_FILE;
#define C_FILES_BASE_ADDR (FSFRAM+sizeof(FS))


// return -1 for FRAM fail
static int getNumOfFilesInFS()
{
	FS fs;

	if(FRAM_read((unsigned char*)&fs,FSFRAM,sizeof(FS))!=0)
	{
		return -1;
	}
	return fs.num_of_files;
}
//return -1 on fail
static int setNumOfFilesInFS(int new_num_of_files)
{
	FS fs;
	fs.num_of_files = new_num_of_files;
	if(FRAM_write((unsigned char*)&fs,FSFRAM,sizeof(FS))!=0)
	{
		return -1;
	}
	return 0;
}
FileSystemResult InitializeFS(Boolean first_time)
{
	int ret;

	hcc_mem_init(); /* Initialize the memory to be used by filesystem */

	ret = fs_init(); /* Initialize the filesystem */
	if(ret != F_NO_ERROR )
	{
		TRACE_ERROR("fs_init pb: %d\n\r", ret);
		return -1;
	}

	ret = f_enterFS(); /* Register this task with filesystem */
	if(ret != F_NO_ERROR )
	{
		TRACE_ERROR("f_enterFS pb: %d\n\r", ret);
		return -1;
	}

	ret = f_initvolume( 0, atmel_mcipdc_initfunc, 0 ); /* Initialize volID as safe */

	if( F_ERR_NOTFORMATTED == ret )
	{
		TRACE_ERROR("Filesystem not formated!\n\r");
		return -1;
	}
	else if( F_NO_ERROR != ret)
	{
		TRACE_ERROR("f_initvolume pb: %d\n\r", ret);
		return -1;
	}

	if(first_time)
	{
		FS fs = {0};
		if(FRAM_write((unsigned char*)&fs,FSFRAM,sizeof(FS))!=0)
		{
			return FS_FAT_API_FAIL;
		}
	}
	return FS_SUCCSESS;
}

//only register the chain, files will create dynamically
FileSystemResult c_fileCreate(char* c_file_name, int size_of_element, int num_of_files)
{
	if(strlen(c_file_name)>MAX_F_FILE_NAME_SIZE)//check len
	{
		return FS_TOO_LONG_NAME;
	}

	C_FILE c_file; //chain file descriptor
	c_file.size_of_element = size_of_element;
	strcpy(c_file.name,c_file_name);
	c_file.num_of_files = num_of_files;
	Time_getUnixEpoch(&c_file.creation_time);//get current time
	c_file.last_time_modified = FIRST_TIME;//no written yet
	int num_of_files_in_FS = getNumOfFilesInFS();
	if(num_of_files_in_FS == -1)
	{
		return FS_FRAM_FAIL;
	}
	int c_file_address =C_FILES_BASE_ADDR+num_of_files_in_FS*sizeof(C_FILE);
	if(FRAM_write((unsigned char*)&c_file,
			c_file_address,sizeof(C_FILE))!=0)//write c_file struct in FRAM
	{
			return FS_FRAM_FAIL;
	}
	if(setNumOfFilesInFS(num_of_files_in_FS+1)!=0)//TODO change to c_fil
	{
		return FS_FRAM_FAIL;
	}

	return FS_SUCCSESS;
}
//write element with timestamp to file
static void writewithEpochtime(F_FILE* file, byte* data, int size, time_unix time)
{
	GomEpsResetWDT(0);;

	int number_of_writes;
	number_of_writes = f_write( &time,sizeof(unsigned int),1, file );
	number_of_writes += f_write( data, size,1, file );
	//printf("writing element, time is: %u\n",time);
	if(number_of_writes!=2)
	{
		printf("writewithEpochtime error\n");
	}
	f_flush( file ); /* only after flushing can data be considered safe */
	f_close( file ); /* data is also considered safe when file is closed */
}
// get C_FILE struct from FRAM by name
static Boolean get_C_FILE_struct(char* name, C_FILE* c_file, unsigned int *address)
{

	GomEpsResetWDT(0);

	int i;
	unsigned int c_file_address = 0;
	int err_read=0;
	int num_of_files_in_FS = getNumOfFilesInFS();
	for(i =0; i < num_of_files_in_FS; i++)			//search correct c_file struct
	{
		c_file_address= C_FILES_BASE_ADDR+sizeof(C_FILE)*(i);
		err_read = FRAM_read((unsigned char*)c_file,c_file_address,sizeof(C_FILE));
		if(0 != err_read)
		{
			printf("FRAM error in 'get_C_FILE_struct()' error = %d\n",err_read);
			return FALSE;
		}

		if(!strcmp(c_file->name,name))
		{
			if(address != NULL)
			{
				*address = c_file_address;
			}
			return TRUE;//stop when found
		}
	}
	return FALSE;
}
//calculate index of file in chain file by time
static int getFileIndex(time_unix creation_time, time_unix current_time, int num_of_files)
{
	GomEpsResetWDT(0);;
	return ((current_time-creation_time)/SKIP_FILE_TIME_SEC)%num_of_files;
}
//write to curr_file_name
void get_file_name_by_index(char* c_file_name, int index, char* curr_file_name)
{
	GomEpsResetWDT(0);;
	int temp[2];//use to append name with index
	temp[0] = index;
	temp[1] = '\0';
	strcpy(curr_file_name,c_file_name);
	strcat(curr_file_name,(char*)temp);
}
FileSystemResult c_fileWrite(char* c_file_name, void* element)
{
	C_FILE c_file;
	unsigned int addr;//FRAM ADDRESS
	F_FILE *file;
	char curr_file_name[MAX_F_FILE_NAME_SIZE+sizeof(int)*2];//append with 2 ints;
	GomEpsResetWDT(0);;
	unsigned int curr_time;
	Time_getUnixEpoch(&curr_time);
	if(get_C_FILE_struct(c_file_name,&c_file,&addr)!=TRUE)//get c_file
	{
		return FS_NOT_EXIST;
	}
	int index_current = getFileIndex(c_file.creation_time,curr_time,c_file.num_of_files);
	int index_last_written = getFileIndex(c_file.creation_time,c_file.last_time_modified,c_file.num_of_files);
	if(index_last_written!=FIRST_TIME)
	{
		while((index_last_written++)%c_file.num_of_files!=index_current)// delete all fild between the last written and the current
		{
			get_file_name_by_index(c_file_name,index_last_written%c_file.num_of_files,curr_file_name);
			f_delete(curr_file_name);
		}
	}

	get_file_name_by_index(c_file_name,index_current,curr_file_name);
	file= f_open(curr_file_name,"a+");
	writewithEpochtime(file,element,c_file.size_of_element,curr_time);
	c_file.last_time_modified= curr_time;
	if(FRAM_write((unsigned char *)&c_file,addr,sizeof(C_FILE))!=0)//update last written
	{
		return FS_FRAM_FAIL;
	}
	f_close(file);
	return FS_SUCCSESS;
}
FileSystemResult c_fileRead(char* c_file_name, byte* buffer, int size_of_buffer, time_unix from_time, time_unix to_time, int* read)
{
	GomEpsResetWDT(0);;
	*read=0;
	C_FILE c_file;
	F_FILE* current_file;
	int buffer_index = 0;
	char curr_file_name[MAX_F_FILE_NAME_SIZE+sizeof(int)*2];//append with 2 ints;
	void* element;
	if(get_C_FILE_struct(c_file_name,&c_file,NULL)!=TRUE)
	{
		return FS_NOT_EXIST;
	}
	unsigned int size_elementWithTimeStamp = c_file.size_of_element+sizeof(unsigned int);
	element = malloc(size_elementWithTimeStamp);//store element and his timestamp
	int index_current = getFileIndex(c_file.creation_time,from_time,c_file.num_of_files);
	int index_to = getFileIndex(c_file.creation_time,to_time,c_file.num_of_files);
	do
	{
		//TODO FINISH CHECKING CODE c_fileRead
		get_file_name_by_index(c_file_name,index_current % c_file.num_of_files,curr_file_name);
		current_file=f_open(curr_file_name,"r");
		unsigned int length =f_filelength(curr_file_name)/(size_elementWithTimeStamp);//number of elements in currnet_file
		int err_fread=0;

		unsigned int j;
		for(j = 0; j < length; j++)
		{
			err_fread = f_read(element,(size_t)size_elementWithTimeStamp,(size_t)1,current_file);

			unsigned int element_time = *((unsigned int*)element);
			printf("read element, time is %u\r\n",element_time);
			if(element_time > to_time)
			{
				break;
			}

			if(element_time >= from_time)
			{
				if(buffer_index*(size_elementWithTimeStamp)>(unsigned int)size_of_buffer)
				{
					return FS_BUFFER_OVERFLOW;
				}
				(*read)++;
				memcpy(buffer + buffer_index,element,size_elementWithTimeStamp);
				buffer_index += size_elementWithTimeStamp;
			}
		}
		f_close(current_file);
	}
	while(((index_current++) % c_file.num_of_files) != index_to);

	free(element);

	return FS_SUCCSESS;
}
int c_fileGetSizeOfElement(char* c_file_name)
{
	C_FILE c_file;
	if(get_C_FILE_struct(c_file_name, &c_file, NULL) != TRUE)
	{
		return -1;
	}
	return c_file.size_of_element;
}
void print_file(char* c_file_name)
{
	C_FILE c_file;
	F_FILE* current_file;
	int i = 0;
	void* element;
	char curr_file_name[FILE_NAME_WITH_INDEX_SIZE];//store current file's name
	int temp[2];//use to append name with index
	temp[1] = '\0';
	if(get_C_FILE_struct(c_file_name,&c_file,NULL)!=TRUE)
	{
		printf("print_file_error\n");
	}
	GomEpsResetWDT(0);;
	element = malloc(c_file.size_of_element+sizeof(unsigned int));//store element and his timestamp
	for(i=0;i<c_file.num_of_files;i++)
	{
		printf("file %d:\n",i);//print file index
		get_file_name_by_index(c_file_name,i,curr_file_name);
		current_file=f_open(curr_file_name,"r");
		int j;
		for(j = 0; j<f_filelength(curr_file_name)/((int)c_file.size_of_element+(int)sizeof(unsigned int)); j++)
		{
			f_read(element,c_file.size_of_element,(size_t)c_file.size_of_element+sizeof(unsigned int),current_file);
			printf("time: %d\n data:",*((int*)element));//print element timestamp
			for(j =0;j<c_file.size_of_element;j++)
			{
				printf("%d ",*((char*)(element+sizeof(int)+j)));//print data
			}
			printf("\n");
		}
		f_close(current_file);
	}
}

FileSystemResult fileWrite(char* file_name, void* element,int size)
{
	F_FILE *file;
	unsigned int curr_time;
	Time_getUnixEpoch(&curr_time);
	file= f_open(file_name,"a+");
	if (file == NULL)
	{
		printf("error, file_open:%d\n", f_getlasterror());
	}
	writewithEpochtime(file,element,size,curr_time);
	f_flush(file);
	f_close(file);
	return FS_SUCCSESS;
}

FileSystemResult fileRead(char* c_file_name,char* buffer, int size_of_buffer, unsigned long from_time, unsigned long to_time, int* read, int element_size)
{
	*read=0;
	F_FILE* current_file= f_open(c_file_name,"r+");
	int buffer_index = 0;
	void* element;
	unsigned int size_elementWithTimeStamp = element_size+sizeof(unsigned int);
	element = malloc(size_elementWithTimeStamp);//store element and his timestamp
	unsigned int length =f_filelength(c_file_name)/(size_elementWithTimeStamp);//number of elements in currnet_file
	int err_fread=0;

	f_seek( current_file, 0L , SEEK_SET );
	for(unsigned int j=0;j < length;j++)
	{
		err_fread = f_read(element,(size_t)size_elementWithTimeStamp,(size_t)1,current_file);

		unsigned int element_time = *((unsigned int*)element);
		printf("read element, time is %u\n",element_time);
		if(element_time > to_time)
		{
			break;
		}

		if(element_time >= from_time)
		{
			if(buffer_index>(unsigned int)size_of_buffer)
			{
				return FS_BUFFER_OVERFLOW;
			}
			(*read)++;
			memcpy(buffer + buffer_index,element,size_elementWithTimeStamp);
			buffer_index += size_elementWithTimeStamp;
		}
	}
	f_close(current_file);


	free(element);

	return FS_SUCCSESS;
}

typedef struct
{
	int a;
	int b;
	int c;
} test_struct;
void DeInitializeFS( void )
{
	printf("deinitializig file system \n");
	int err = f_delvolume( _SD_CARD ); /* delete the volID */

	printf("1\n");
	GomEpsResetWDT(0);;
	if(err != 0)
	{
		printf("f_delvolume err %d\n", err);
	}

	f_releaseFS(); /* release this task from the filesystem */

	printf("2\n");

	err = fs_delete(); /* delete the filesystem */

	printf("3\n");

	if(err != 0)
	{
		printf("fs_delete err , %d\n", err);
	}
	err = hcc_mem_delete(); /* free the memory used by the filesystem */

	printf("4\n");

	if(err != 0)
	{
		printf("hcc_mem_delete err , %d\n", err);
	}
	printf("deinitializig file system end \n");

}
typedef struct
{
	unsigned int a;
	int b;
} struct_test2;
static void i_test2()
{
	F_FILE* file = f_open("test2","a+");
	unsigned int time;
	struct_test2 a;
	a.a=0x0FFFFFFF;
	a.b=0x0FFFFFFF;

	Time_getUnixEpoch(&time);
	 int bw=0;
	 char element[12]={1,2,3,4,5,6,7,8,9,10,11,12};
	bw=f_write(&time,sizeof(time),1,file);
	bw+=f_write(&a,sizeof(a),1,file);
	if(bw!=2)
	{
		printf("errror\n");
	}

	f_flush(file);
	f_seek(file,0,SEEK_SET);
	int ERR_READ=f_read(element,12,1,file);
	printf("time = %d, a= %d; ERR_READ=%d",*((unsigned int*)element),*((int*)(element+4)),ERR_READ);
	f_close(file);
	f_delete("test2");
}
static void i_test3()
{
	unsigned int time;
	char element[sizeof(struct_test2)+sizeof(int)];
	Time_getUnixEpoch(&time);
	int number_of_writes;
	F_FILE* file = f_open("test999","a+");

	struct_test2 data = {2,3};
	number_of_writes = f_write( &time,sizeof(unsigned int),1, file );
	number_of_writes += f_write( &data, sizeof(data),1, file );

	if(number_of_writes!=2)
	{
		printf("sanity check fail error1\n");
	}
	f_flush( file ); /* only after flushing can data be considered safe */
	f_close( file ); /* data is also considered safe when file is closed */
	file = f_open("test999","r");
	number_of_writes = f_read( element,sizeof(struct_test2)+sizeof(int),1, file );
	if(number_of_writes!=1)
	{
		printf("sanity check fail 2\n");
	}
	if(*((unsigned int*)element)!=time)
	{
		printf("sanity check fail 3\n");
	}
	f_delete("test999");
	printf("sanity check end time is %d",*((unsigned int*)element));
}

int FS_test()
{
	Time tm={0,0,0,1,1,1,0,0};
	int err_time=Time_start(&tm,0);


	unsigned int t_time_t=0;
	Time_getUnixEpoch(&t_time_t);
	printf("err_time=%d\nthe current time is:%d\n",err_time,t_time_t);
	GomEPSdemoInit();
	GomEpsResetWDT(0);;
	if(InitializeFS(TRUE) != FS_SUCCSESS)
	{
		printf("filed to InitializeFS\n");
		return TRUE;
	}
	//i_test3();

	//i_test2();

	if(c_fileCreate("testfile5",sizeof(test_struct),1)!=FS_SUCCSESS)
	{
		printf("filed to create chain\n");
		DeInitializeFS();
		return TRUE;
	}
	unsigned int time;
	Time_getUnixEpoch(&time);
	int read = 0;
	test_struct ts = {1,2,3};

	test_struct arr[7];
	c_fileWrite("testfile5",&ts);
	c_fileWrite("testfile5",&ts);
	c_fileWrite("testfile5",&ts);
	c_fileWrite("testfile5",&ts);
	c_fileWrite("testfile5",&ts);
	c_fileWrite("testfile5",&ts);
	c_fileWrite("testfile5",&ts);
	c_fileRead("testfile5",(char*)arr,7,time,time,&read);
	if(read != 7)
	{
		printf("error read\n");
	}
	if(arr[4].b !=2)
	{
		printf("error");
	}
	//print_file("testfile5");
	DeInitializeFS();
	return TRUE;
}
