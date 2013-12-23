#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/proc.h"


static int pstreeFS_getattr(const char *path, struct stat *st_data)
{
	int res = 0;
	memset(st_data, 0, sizeof(struct stat));

	char p[2048];
	char dest[100];
	strcpy(p,path);
	parse_path(p,dest);
	// if dest -1 -> wrong path
	// if dest is empty its root
	// others are proc folders
	if(strcmp(dest,"")==0 || strcmp(dest,"0")==0){
		st_data->st_mode = S_IFDIR | 0755;
		st_data->st_nlink = 2;
	}else if(strcmp(dest,"-1")==0){
		res = -ENOENT;
	}else{
		node *head = (struct node*) malloc(sizeof(struct node));
		read_proc_list(head);
		while(head!=NULL){
			if(strcmp(dest,head->name)==0){
				st_data->st_mode = S_IFDIR | 0755;
				st_data->st_nlink = 2;
				break;
			}
			head = head->next;
		}
	}

	return 0;
}

static int pstreeFS_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	int res = 0;

	char p[2048];
	char dest[100];
	strcpy(p,path);
	parse_path(p,dest);
	// if dest -1 -> wrong path
	// if dest is empty its root
	// others are proc folders

	if(strcmp(dest, "")==0){
		filler(buf, "0", NULL, 0);
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
	}
	else if(strcmp(dest, "-1")==0){
		res = -ENOENT;
	}else{
		node *head = (struct node*) malloc(sizeof(struct node));
		read_proc_list(head);
		while(head!=NULL){
			char stat[2048];
			read_proc_stat(head->name,stat);
			char ppid[20];
			get_ppid_from_stat(stat,ppid);
			if(strcmp(dest,ppid)==0){
				filler(buf, head->name, NULL, 0);
				filler(buf, ".", NULL, 0);
				filler(buf, "..", NULL, 0);
				filler(buf, "info.txt", NULL, 0);
			}
			head = head->next;
		}
	}

	return res;
}
static int pstreeFS_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int pstreeFS_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	char *info_str = "Hello World!\n";
	size_t len;
	(void) fi;
	if(strcmp(path, info_path) != 0)
		return -ENOENT;
	len = strlen(info_str);
	if(offset < len){
		if( offset + size > len ){
			size = len - offset;
		}
		memcpy(buf, info_str + offset, size);
	}else{
		size = 0;
	}
	return size;
}

static struct fuse_operations pstreeFS_oper = {
    .getattr	= pstreeFS_getattr,
    .readdir	= pstreeFS_readdir,
    .open	= pstreeFS_open,
    .read	= pstreeFS_read,
};

int main(int argc, char *argv[]){
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	fuse_main(args.argc, args.argv, &pstreeFS_oper, NULL);

    return 0;
}
