#include <switch.h>
#include <string>
#include <dirent.h>
#include <experimental/filesystem>
//#include <sys/stat.h>
#include "dump.h"
#include "cons.h"
#include "dir.h"

namespace fs = std::experimental::filesystem;
using namespace std;

dumpArgs* dumpArgsCreate(console* c, bool* status) {
	dumpArgs* ret = new dumpArgs;
	ret->c = c;
	ret->thFin = status;
	return ret;
}

void deleteDirectoryContents(const std::string& dir_path) {
	for (const auto& entry : fs::directory_iterator(dir_path))
		fs::remove(entry.path());
}

int countEntriesInDir(const char* dirname) {
	int files = 0;
	dirent* d;
	DIR* dir = opendir(dirname);
	if (dir == NULL) return 0;
	while ((d = readdir(dir)) != NULL) files++;
	closedir(dir);
	return files;
}

void daybreak() {	
	unsigned long min = 5633; //size in bytes
	struct dirent* ent;
	DIR* dir = opendir("sdmc:/Dumped-Firmware");
	while ((ent = readdir(dir)) != NULL) {
		FILE *f = NULL;	
		//this code block is to give the files in the dir a full path - or we get a crash.
		string partpath = "sdmc:/Dumped-Firmware/";
		const char* name = ent->d_name;
		partpath += name;
		const char *C = partpath.c_str();
		//end of code block
		
		f = fopen(C,"rb");
		fseek(f, 0, SEEK_END);
		unsigned long len = (unsigned long)ftell(f);
		fclose(f);

		if (len < min){
			//some debug code for checking file sizes...
			//string namesize = to_string(len);
			//c->out(namesize);
			//do some renaming here
			string newfile = C;
			string s2 = "cnmt.nca";
			
			if (strstr(newfile.c_str(),s2.c_str())) {
			//
			} else {
					string renamed = newfile.replace(newfile.find("nca"), sizeof("nca") - 1, "cnmt.nca");
					const char *D = renamed.c_str();
					rename(C, D);
			}
		}
	}
	closedir(dir);
}

void deletedump() {
	//remove the dumped files
	fs::path p = ("sdmc:/Dumped-Firmware");
	deleteDirectoryContents(p);
}

void dumpArgsDestroy(dumpArgs* a) {
	delete a;
}

void dumpThread(void* arg) {
	dumpArgs* a = (dumpArgs*)arg;
	console* c = a->c;
	
	DIR* dir = opendir("sdmc:/Dumped-Firmware");
	if (!dir) {
			FsFileSystem sys;
		if (R_SUCCEEDED(fsOpenBisFileSystem(&sys, FsBisPartitionId_System, ""))) {
			fsdevMountDevice("sys", sys);
			c->out("Starting Dump.");
			c->nl();

			//Del first for safety
			//delDir("sdmc:/Update/", false, c);
			mkdir("sdmc:/Dumped-Firmware", 777);

			//Copy whole contents folder
			copyDirToDir("sys:/Contents/registered/", "sdmc:/Dumped-Firmware/", c);

			fsdevUnmountDevice("sys");
			c->nl();
			c->out("The current NAND firmware files have been dumped successfully to your Micro SD card.");
			c->nl();
			
			//move/rename folder
			/*
			std::string name("/Update/registered");
			std::string new_name("/Dumped-Firmware");
			fs::path p = ("sdmc:");
			fs::rename(p / name, p / new_name);
			//remove the empty folders
			rmdir("/Update/placehld");
			rmdir("/Update/temp");
			rmdir("/Update");
			*/
			c->out("Renaming files for Daybreak, ^Please wait^ this will just take a few seconds.");
			c->nl();
			daybreak();
			c->out("Firmware files renamed to be Daybreak compatible. Dump sequence is now completed. &Have a nice day!&");
			c->nl();
		} else {
			c->out("*FAILED TO OPEN THE SYSTEM PARTITION!*");
			c->nl();
		}
	}
	closedir(dir);
	*a->thFin = true;
}

void cleanThread(void *arg) {
	dumpArgs* a = (dumpArgs*)arg;
	console* c = a->c;
	
	DIR* dir = opendir("sdmc:/Dumped-Firmware");
	if (dir) {
			closedir(dir);
			c->clear();
			c->out("Please wait while I remove the current firmware dump from your Micro SD card.");
			c->nl();
			deletedump();
			rmdir("sdmc:/Dumped-Firmware");
			c->out("^Dumped Firmware Removed!^");
			c->nl();
	} else {
			c->clear();
			c->out("Dumped firmware ^isn't^ present on the Micro SD Card!");
			c->nl();
	}
	*a->thFin = true;
}

void cleanPending(void *arg) {
	dumpArgs* a = (dumpArgs*)arg;
	console* c = a->c;
	
	int filenumber = 0;
	
	FsFileSystem sys;
	if (R_SUCCEEDED(fsOpenBisFileSystem(&sys, FsBisPartitionId_System, ""))) {
			fsdevMountDevice("sys", sys);
			DIR* dir = opendir("sys:/Contents/placehld/");
			if (dir) {
					filenumber = countEntriesInDir("sys:/Contents/placehld/");
					if (filenumber != 0) {
						string str = to_string(filenumber);
						c->out("Attemping to delete the ^" + str + "^ pending update nca's from your Nand - &Please Wait!&");
						c->nl();
						deleteDirectoryContents("sys:/Contents/placehld");
						closedir(dir);
						rmdir("sys:/Contents/placehld");
						c->out("^Pending update ncs's have been successfully removed from your Nand.^");
						c->nl();
					} else {
						string str = to_string(filenumber);
						c->out("The placehld folder contains ^" + str + "^ files - there's no nca files to remove!");
						c->nl();
					}
				} else {
					c->clear();
					c->out("No pending update nca files were found on the Nand.");
					c->nl();
				}
				fsdevUnmountDevice("sys");
		} else {
				c->clear();
				c->out("*FAILED TO OPEN THE SYSTEM PARTITION!*");
				c->nl();
		}
		
		*a->thFin = true;
}