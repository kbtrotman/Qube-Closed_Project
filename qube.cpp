/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 **/

// Globals:  Private Defs, Then Classes
#include "qube.hpp"
#include "q_log.hpp"
    q_log& QLOG = q_log::getInstance();
	static std::shared_ptr<spdlog::logger> _Qlog = QLOG.get_log_instance();
#include "q_fuse.hpp"



// Define all the static variables that need to always be available.

PGconn *qube_psql::conn = NULL;
PGresult *qube_psql::last_res = NULL;
PGresult *qube_psql::last_ins = NULL;
int qube_psql::rec_count = 0;
int qube_psql::row= 0;
int qube_psql::col= 0;
bool qube_psql::psql_init = false;
std::string qube_psql::quoted_hash = "";
std::string qube_psql::quoted_block = "";
std::string qube_psql::quoted_count = "";
const char *qube_psql::qu_sql = "";
const char *qube_psql::ins_sql = "";
const char *qube_psql::use_incr = "";
const char *qube_psql::use_decr = "";



// Main entry
int main( int argc, char *argv[] )
{
	static std::shared_ptr<spdlog::logger> _Qlog = QLOG.get_log_instance();
	qube_fuse qf;

	INFO("====================================");
	INFO("===  PG Server version = {:d}  ===", qf.qpsql_getServerVers());
	INFO("====================================");	

    INFO("=================================");
    INFO("=====  Filesystem Starting ======");
    INFO("=================================");

	// Startup the filesystem
  	return qf.run(argc, argv);

}