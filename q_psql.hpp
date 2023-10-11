/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

// Postgres Support via Postgres/C lib. Libpqxx seems to not work. I tried it with no joy.

#pragma once

#include <libpq-fe.h>

#include "q_convert.hpp"
#include "q_log.hpp"

class q_psql : public q_convert {

	public:
    	static PGresult   *last_res;
        static PGresult   *last_ins;
    	static int        rec_count;
    	static int        row;
    	static int        col;

		q_psql (q_log& q);
		~q_psql ();
		int qpsql_getServerVers();
		// All globally static methods below this point. They are interrupt-driven......		
		static std::vector<uint8_t> qpsql_get_block_from_hash(std::string hash);
        static int qpsql_insert_hash(std::string hash, std::vector<uint8_t> *data_block);
		static int qpsql_incr_hash_count(std::string hash);
		static PGresult* qpsql_decr_hash_count(std::string hash);
		static void qpsql_release_query ();
		static void qpsql_do_exit(int ex);


	private:
		int	q_psql_vers;
    	static PGconn *conn;
		static bool psql_init;
		static std::string quoted_hash;
		static std::string quoted_block;
		static std::string quoted_count;
		static const char *qu_sql;
        static const char *ins_sql;
		static const char *use_incr;
		static const char *use_decr;

		static PGresult* qpsql_execQuery(std::string qube_query);
		static std::string qpsql_get_quoted_value(std::string in_string);
		static std::string qpsql_get_quoted_value(std::vector<uint8_t> *in_vec);
		static std::string qpsql_get_unquoted_value(char *in_string);
		static std::vector<uint8_t> qpsql_get_unquoted_value(std::vector<uint8_t> *in_vec);

};
