/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */
 
#define FUSE_USE_VERSION 34

// Legacy C Libs
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
// C++ Libs (well, this is supposed to be in C++, but some has to be in C.)
#include <string>
#include <iostream>
#include <cstdio>
#include <chrono>
// Postgres Support via Postgres/C lib. Libpqxx seems to be trash, no worky, worky.
#include <libpq-fe.h>
// For Logging!
#include "spdlog/spdlog.h"
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

// Globals
char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;


// Functions & classes
void do_exit(PGconn *conn, int exStatus);


//Classes -> Lowest Level to highest.
class qube_log : public spdlog::logger {

	public:
		bool logger_init = false;

		qube_log() : spdlog::logger("logger", std::make_shared<spdlog::sinks::daily_file_sink_mt>("/var/log/qube.log", 0, 0)) {
			/* Init Logger */
			if (logger_init == false) {
				set_level(spdlog::level::debug);
				qlog->debug("Qube logging started.");
				logger_init = true;
			}			
		}
};

class qube_psql : public qube_log {

	public:
    	PGconn   *conn;
    	PGresult *last_res;
		int		 q_psql_vers;
    	int      rec_count;
    	int      row;
    	int      col;

		qube_psql () {
			conn = PQconnectdb(DBSTRING);
    		if (PQstatus(conn) == CONNECTION_BAD) {
				qlog->debug("Connection to database failed: " + *PQerrorMessage(conn));
        		do_exit(conn, 1);
    		} else {
				qlog->debug("Connected to postrgres DB.");
			}
		}

		int qpsql_getServerVers() {
		    int q_psql_vers = PQserverVersion(conn);
    		return q_psql_vers;
		}

		PGresult* qpsql_execQuery( std::string qube_query ) {
			/* Create SQL statement */
			qlog->debug(qube_query);
    		last_res = PQexec(conn, qube_query.c_str());

    		if (PQresultStatus(last_res) != PGRES_TUPLES_OK) {
        		error("No data was returned from the SQL Query just posted.");
				qlog->debug("Error message: " + (*PQerrorMessage(conn)) );
        		do_exit(conn, 0);
    		}
    		rec_count = PQntuples(last_res);
			return last_res;
		}

		void qpsql_release_query () { PQclear(last_res); }
};


void do_exit(PGconn *conn, int exStatus) {
	qube_log q_log;

	q_log.qlog->debug("do_exit---[" + std::to_string(exStatus) + "]--->" );
    PQfinish(conn);  //Close the DB connection gracefully before we shutdown.
    exit(exStatus);  //Exit with 0 for good, or whatever error status was passed here.
}


class qube_fuse : public fuse_operations, public qube_psql {

	public:
		qube_fuse() {

			// set the methods of the fuse_operations struct to the methods of the QubeFileSystem class
        	memset(this, 0, sizeof(fuse_operations));
        	getattr = &qube_fuse::do_getattr;
			mknod	= &qube_fuse::do_mknod;
			mkdir	= &qube_fuse::do_mkdir;
			read	= &qube_fuse::do_read;
			write	= &qube_fuse::do_write;
        	readdir = &qube_fuse::do_readdir;
		}


		/*

		Fuse Over-rides - To make it a real FS, these would be the layer to start a--the fuse inclusions--and go downward to the hardware layer. 
		This isn't nearly as hard as it sounds. It's easier to test on fuse and then migrate to physical disk sectors later. This can be done by
		migrating the code to being a device-driver that loads a filesystem module.

		*/

    	// Run the filesystem
    	int mount(const char *mountpoint) {
        	// Set up the FUSE options
        	const char* fuse_argv[] = {"qubeFS", "-o", "default_permissions", mountpoint};
        	int fuse_argc = 4;

        	// Mount the filesystem
       	 	return fuse_main(fuse_argc, const_cast<char**>(fuse_argv), this, nullptr);
    	}

		static int do_getattr( const char *path, struct stat *st )
		{
			spdlog::qlog->debug(std::string("FUSE::Getattr---[") + *path + "]--->" );
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
			spdlog::qlog->debug(std::string("FUSE::Readdir---[") + *path + "]---[" + std::to_string(offset) + "]--->" );
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
			spdlog::qlog->debug(std::string("FUSE::Read---[") + *path + "]---[" + std::to_string(*buffer) + "]---[" + std::to_string(size) + 
				"]---[" + std::to_string(offset) + "]--->" );
			int file_idx = get_file_index( path );
			
			if ( file_idx == -1 )
				return -1;
			
			char *content = files_content[ file_idx ];
			
			memcpy( buffer, content + offset, size );
				
			return strlen( content ) - offset;
		}

		static int do_mkdir( const char *path, mode_t mode )
		{
			spdlog::qlog->debug("FUSE::Mkdir---[" + std::to_string(*path) + "]---[" + std::to_string(mode) + "]--->" );
			path++;
			add_dir( path );
			
			return 0;
		}

		static int do_mknod( const char *path, mode_t mode, dev_t rdev )
		{
			spdlog::qlog->debug("FUSE::Mknod---[" + std::to_string(*path) + "]---[" + std::to_string(mode) + "]---[" + std::to_string(rdev) + "]--->" );
			path++;
			add_file( path );
			
			return 0;
		}

		static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
		{
			spdlog::qlog->debug("FUSE::Write---[" + std::to_string(*path) + "]---[" + std::to_string(*buffer) + "]---[" + std::to_string(size) + 
				"]---[" + std::to_string(offset) + "]-->" );
			write_to_file( path, buffer );
			return size;
		}


	// These obviously are not meant to be accessible outside the class ------------------>
	private:

		static void add_dir( const char *dir_name )
		{
			spdlog::qlog->debug(std::string("add_dir---[") + dir_name + "]--->" );
			curr_dir_idx++;
			strcpy( dir_list[ curr_dir_idx ], dir_name );
		}

		static int is_dir( const char *path )
		{
			spdlog::qlog->debug(std::string("is_dir---[") + path + "]--->" );
			path++; // Eliminating "/" in the path
		
			for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
				if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
					return 1;
			
			return 0;
		}

		static void add_file( const char *filename )
		{
			spdlog::qlog->debug(std::string("add_file---[") + filename + "]--->" );
			curr_file_idx++;
			strcpy( files_list[ curr_file_idx ], filename );
			
			curr_file_content_idx++;
			strcpy( files_content[ curr_file_content_idx ], "" );
		}

		static int is_file( const char *path )
		{
			spdlog::qlog->debug(std::string("is_file---[") + *path + "]--->" );
			path++; // Eliminating "/" in the path
			
			for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
				if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
					return 1;
			
			return 0;
		}

		static int get_file_index( const char *path )
		{
			spdlog::qlog->debug(std::string("get_file_index---[") + *path + "]--->" );
			path++; // Eliminating "/" in the path
			
			for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
				if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
					return curr_idx;
			
			return -1;
		}

		static void write_to_file( const char *path, const char *new_content )
		{
			spdlog::qlog->debug(std::string("Write_to_file---[") + *path + "]---[" + new_content + "]--->");
			int file_idx = get_file_index( path );
			
			if ( file_idx == -1 ) // No such file
				return;
				
			strcpy( files_content[ file_idx ], new_content ); 
		}


};



// Main entry
int main( int argc, char *argv[] )
{
//	PGresult *res;
//	int rec_count;
//	int row;
//	int col;
	qube_psql q_pgsql;

//    std::string query = "SELECT * FROM public.hashes ORDER BY hash ASC";
//	q_pgsql.qlog->debug(query);
    q_pgsql.qlog->debug("==========================");
	q_pgsql.qlog->debug(std::string("server version = ") + std::to_string(q_pgsql.qpsql_getServerVers()));
	q_pgsql.qlog->debug("==========================");
//	res = q_pgsql.qpsql_execQuery(query);

//	row = PQntuples(res);
//  	col = PQnfields(res);
//	rec_count = q_pgsql.rec_count;

// 	q_pgsql.qlog->debug("We received " + std::to_string(rec_count) + " records.\n");
//	q_pgsql.qlog->debug("==========================");

//    for (row=0; row<rec_count; row++) {
//        for (col=0; col<2; col++) {
//            q_pgsql.qlog->debug( "row:" + std::to_string(row) + " col: " + std::to_string(col) + " value: " + PQgetvalue(res, row, col) );
//        }
//    }
 
    q_pgsql.qlog->debug("=============================");
    q_pgsql.qlog->debug("===  Filesystem Starting  ===");
    q_pgsql.qlog->debug("=============================");

	// Startup the filesystem
	qube_fuse q_fuse;
	q_fuse.mount(argv[1]);

//    do_exit(q_pgsql.conn, 0);

}