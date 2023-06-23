/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */
 
#define FUSE_USE_VERSION 30

// Legacy C Libs
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
// C++ Libs (well, this is supposed to be in C++, but a lot os in C.)
#include <iostream>
#include <cstdio>
#include <chrono>
// Postgres Support via Postgres/C lib. Libpqxx seems to be trash, no worky, worky.
#include <libpq-fe.h>
// For Logging!
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variableinclude "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/sinks/daily_file_sink.h"



// Constants
//#############################
#define BLOCK_SIZE 512
#define HASH_SIZE 512
// Database configuration
//#############################
#define DATABASE "fuse"
#define USER "postgres"
#define PASSWORD "Postgres!909"
#define HOST "10.100.122.20"
#define PORT "5432"

#define DBSTRING "user=" USER " dbname=" DATABASE " password=" PASSWORD " hostaddr=" HOST " port=" PORT


char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;



void do_exit(PGconn *conn, int exStatus) {
    
    PQfinish(conn);  //Close the DB connection gracefully before we shutdown.
    exit(exStatus);  //Exit with 0 for good, or whatever error status was passed here.
}



auto daily_log_init()
{
    // Create a daily logger - a new file is created every day on 2:30am
    auto logger = spdlog::daily_logger_st("daily_logger", "/var/log/qube.log", 0, 1 );
	return logger;
}

void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
}

int is_dir( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

void add_file( const char *filename )
{
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	
	curr_file_content_idx++;
	strcpy( files_content[ curr_file_content_idx ], "" );
}

int is_file( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_file_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 ) // No such file
		return;
		
	strcpy( files_content[ file_idx ], new_content ); 
}

// ... //

static int do_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else if ( is_file( path ) == 1 )
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	else
	{
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
			filler( buffer, dir_list[ curr_idx ], NULL, 0 );
	
		for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
			filler( buffer, files_list[ curr_idx ], NULL, 0 );
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	
	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );
		
	return strlen( content ) - offset;
}

static int do_mkdir( const char *path, mode_t mode )
{
	path++;
	add_dir( path );
	
	return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	spdlog::qlog->debug("Write---{}---{:f}---{:f}--->", *path, mode, rdev );
	path++;
	add_file( path );
	
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	spdlog::qlog->debug("Write---{}---{}---{:d}---{:d}-->", *path, *buffer, size, offset );
	write_to_file( path, buffer );
	return size;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .mknod		= do_mknod,
    .mkdir		= do_mkdir,
    .read		= do_read,
    .write		= do_write,
    .readdir	= do_readdir,
};

int main( int argc, char *argv[] )
{

    PGconn   *conn = PQconnectdb(DBSTRING);
    PGresult *res;
    int      rec_count;
    int      row;
    int      col;

	/* Init Logger */
	auto log = daily_log_init();
	log->set_level(spdlog::level::debug); // Set global log level to debug
	                                        // TODO: This should be set via Command-line
											// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	log->qlog->debug("Loggins Started.");

    if (PQstatus(conn) == CONNECTION_BAD) {
        
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        do_exit(conn, 1);
    }

    int ver = PQserverVersion(conn);
    printf("Server version: %d\n", ver);

    /* Create SQL statement */
    res = PQexec(conn, "SELECT * FROM public.hashes ORDER BY hash ASC");
	printf("PGresultStatus(res):  %d PGRES_TUPLES_OK: %d \n", PQresultStatus(res), PGRES_TUPLES_OK);
	printf("Error message: %s\n",  PQerrorMessage(conn));
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        puts("We did not get any data!");
        exit(0);
    }
    rec_count = PQntuples(res);
    printf("We received %d records.\n", rec_count);
    puts("==========================");
 
    for (row=0; row<rec_count; row++) {
        for (col=0; col<3; col++) {
            printf("%s\t", PQgetvalue(res, row, col));
        }
        puts("\n");
    }
 
    puts("==========================");
    PQclear(res);
    do_exit(conn, 0);

	return fuse_main( argc, argv, &operations, NULL );
}