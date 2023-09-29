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

class qube_psql : public q_convert {

	public:


    	static PGresult   *last_res;
        static PGresult   *last_ins;
    	static int        rec_count;
    	static int        row;
    	static int        col;

		qube_psql () {
			TRACE("Psql Init------>");

			qu_sql = "SELECT block, use_count FROM hashes WHERE hash = %s";
			ins_sql = "INSERT INTO hashes VALUES (%s, %s, %d)";
			use_incr = "UPDATE hashes set use_count = use_count + 1 WHERE hash = %s";
			use_decr = "UPDATE hashes set use_count = use_count - 1 WHERE hash = %s";

			if (!psql_init) {
				conn = PQconnectdb(DBSTRING);
    			if (PQstatus(conn) == CONNECTION_BAD) {
					DEBUG("Connection to database failed with error: {}", PQerrorMessage(conn));
        			qpsql_do_exit(1);
					exit(1);
    			} else {
					TRACE("QUBE_PSQL::qube_psql: Connected to postrgres DB.");
                    psql_init = true;
				}
			}
			TRACE("Psql Init Leaving------>");
			FLUSH;
		}

		~qube_psql () { 
            PQclear(last_res);
    		PQfinish(conn);  //Close the DB connection gracefully before we shutdown.
          }

		int qpsql_getServerVers() {
		    int q_psql_vers = PQserverVersion(conn);
    		return q_psql_vers;
		}

		// All globally static methods below this point. They are interrupt-driven......		

		static std::vector<uint8_t> qpsql_get_block_from_hash(std::string hash) {
            //SELECT a data block from the DB given a hash value....
            TRACE("QUBE_FUSE::qpsql_get_block_from_hash---hash: {}--->", hash);
			std::vector<uint8_t> data_block;

            quoted_hash = qpsql_get_quoted_value(hash);
			char *quoted_sql = (char *) malloc(strlen(qu_sql) + quoted_hash.length() + 1);
    		std::sprintf(quoted_sql, qu_sql, quoted_hash.c_str());
 
            DEBUG("QUBE_FUSE::qpsql_get_block_from_hash: SELECT QUERY = {}", quoted_sql);
            last_res = qpsql_execQuery(quoted_sql);
			if (rec_count == 0) {
				WARN("QUBE_FUSE::qpsql_get_block_from_hash: No records returned for hash.");
				data_block.assign(NO_RECORDS, sizeof(NO_RECORDS));
			} else {
				std::string tmp_string = qpsql_get_unquoted_value(PQgetvalue(last_res, 0, 0));
				data_block.assign(tmp_string.begin(), tmp_string.end());
				DEBUG("QUBE_FUSE::qpsql_get_block_from_hash: Data block returned from DB. query = {} returned block = {}", quoted_sql, (char*)data_block.data()); 
			}
			TRACE("QUBE_FUSE::qpsql_get_block_from_hash: Leaving qpsql_get_block_from_hash with data block: {}", (char*)data_block.data());	
			FLUSH;
			return data_block;
		}

        static int qpsql_insert_hash(std::string hash, std::vector<uint8_t> *data_block) {
            //INSERT a hash into the DB....
            TRACE("QUBE_PSQL::qpsql_insert_hash---hash: {}---block: {}--->", hash, (char*)data_block->data());

            quoted_hash = qpsql_get_quoted_value(hash);
            quoted_block = qpsql_get_quoted_value(data_block);
			//quoted_count = qpsql_get_quoted_value("1");
            char *quoted_sql = (char *) malloc(strlen(ins_sql) + quoted_hash.length() + quoted_block.length() + quoted_count.length() + 1);
            std::sprintf(quoted_sql, ins_sql, quoted_hash.c_str(), quoted_block.c_str(), 1 );
			std::string tmp_sql(quoted_sql);
			int res;
			FLUSH;

            DEBUG("QUBE_PSQL::qpsql_insert_hash: INSERT QUERY = {}", tmp_sql);
            last_ins = qpsql_execInsert(tmp_sql);
			if (PQresultStatus(last_ins) != PGRES_COMMAND_OK) {
				res = 0;
				CRITICAL("QUBE_PSQL::qpsql_insert_hash: Data insert failed into DB. query = {}", tmp_sql);
			} else {
				char *tuples = PQcmdTuples(last_ins);
				res = atoi(tuples);
				DEBUG("QUBE_PSQL::qpsql_insert_hash: {:d} rows inserted into DB. query = {}", res, tmp_sql);
			}

            TRACE("QUBE_PSQL::qpsql_insert_hash: ---[Leaving]---with rows inserted: {:d}--->", res);
			FLUSH;
            return res;
        }

		static int qpsql_incr_hash_count(std::string hash) {
			TRACE("QUBE_PSQL::qpsql_incr_hash_count---hash {}--->", hash);
			//PGresult *res;
			
			quoted_hash = qpsql_get_quoted_value(hash);
			char *quoted_sql = (char *) malloc(strlen(use_incr) + quoted_hash.length() + 1);
            std::sprintf(quoted_sql, use_incr, quoted_hash.c_str(), 1 );

			qpsql_execQuery(quoted_sql);
			rec_count = 0;
			TRACE("QUBE_PSQL::qpsql_incr_hash_count---[Leaving]---with record count: {:d}--->", qube_psql::rec_count);
			FLUSH;
			return rec_count;
		}

		static PGresult* qpsql_decr_hash_count(std::string hash) {
			TRACE("QUBE_PSQL::qpsql_decr_hash_count---hash {}--->", hash);
			PGresult *res;
			
			quoted_hash = qpsql_get_quoted_value(hash);
			char *quoted_sql = (char *) malloc(strlen(use_decr) + quoted_hash.length() + 1);
            std::sprintf(quoted_sql, use_decr, quoted_hash.c_str(), 1 );

			res = qpsql_execQuery(quoted_sql);

			TRACE("QUBE_PSQL::qpsql_incr_hash_count---[Leaving]---with record count: {:d}--->", qube_psql::rec_count);
			FLUSH;
			return res;
		}

		static PGresult* qpsql_execInsert(std::string qube_query) {
			/* Create SQL statement */
			TRACE("QUBE_PSQL::qpsql_execInsert---qube_query {}--->", qube_query);
			DEBUG(qube_query);
			PGresult *res;

    		res = PQexec(conn, qube_query.c_str());
    		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        		ERROR("QUBE_PSQL::qpsql_execInsert: INSERT failed from the SQL Query just posted. SQL Return Error message: {}", PQerrorMessage(conn));
    		}
			TRACE("QUBE_PSQL::qpsql_execInsert:---[Leaving]--->");
			FLUSH;
			return res;
		}

		static void qpsql_release_query () { PQclear(last_res); PQclear(last_ins); }

		static void qpsql_do_exit(int ex) {
            //exStatus = ex;
			//DEBUG("do_exit---[{:d}]--->", exStatus );
		}


	private:
		int		q_psql_vers;
    	static PGconn     *conn;
		static bool 	  psql_init;
		static std::string quoted_hash;
		static std::string quoted_block;
		static std::string quoted_count;
		static const char *qu_sql;
        static const char *ins_sql;
		static const char *use_incr;
		static const char *use_decr;

		static PGresult* qpsql_execQuery(std::string qube_query) {
			/* Create SQL statement */
			TRACE("QUBE_PSQL::qpsql_execQuery---qube_query {}--->", qube_query);
			PGresult *res;
			
    		res = PQexec(conn, qube_query.c_str());
    		if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        		ERROR("QUBE_PSQL::qpsql_execQuery: No data was returned from the SQL Query just posted. SQL Return Error message: {}", PQerrorMessage(conn));
        		rec_count = -1;
    		} else {
    			qube_psql::rec_count = PQntuples(res);
			}

			TRACE("QUBE_PSQL::qpsql_execQuery---[Leaving]---with record count: {:d}--->", qube_psql::rec_count);
			FLUSH;
			return res;
		}

		static std::string qpsql_get_quoted_value(std::string in_string) {
    		TRACE("QUBE_PSQL::qpsql_get_quoted_value---instr: {}--->", in_string);
			char *escaped_val = PQescapeLiteral(conn, in_string.c_str(), in_string.length());
    		if (escaped_val == NULL) {
        		ERROR("QUBE_PSQL::qpsql_get_quoted_value: Failed to escape a value: {} with connection {:d}.", in_string, PQerrorMessage(conn));
				qpsql_do_exit(4);
    		}
			std::string tmp_quote(escaped_val);
			PQfreemem(escaped_val);
			TRACE("QUBE_PSQL::qpsql_get_quoted_value---[Leaving]---with quoted string: {}--->", tmp_quote);
			FLUSH;
			return tmp_quote;
		}

		static std::string qpsql_get_quoted_value(std::vector<uint8_t> *in_vec) {
    		TRACE("QUBE_FUSE::qpsql_get_unquoted_value---binary instr: {}--->", (char*)in_vec->data());
			FLUSH;
			char *escaped_val = PQescapeLiteral(conn, ((const char*)q_convert::vect2char(in_vec)), in_vec->size());

     		if (escaped_val == NULL) {
        		ERROR("QUBE_PSQL::qpsql_get_quoted_value: Failed to escape a value: {} with connection {:d}.", (char*)in_vec->data(),PQerrorMessage(conn));
				qpsql_do_exit(4);
    		}
			std::string tmp_quote(escaped_val);
			PQfreemem(escaped_val);
			TRACE("QUBE_PSQL::qpsql_get_quoted_value---[Leaving]---with quoted string: {}--->", tmp_quote);
			FLUSH;
			return tmp_quote;
		}

		static std::string qpsql_get_unquoted_value(char *in_string) {
    		TRACE("QUBE_FUSE::qpsql_get_unquoted_value---binary instr: {}--->", in_string);
		 	char *escaped_binary_field = in_string;
        	size_t escaped_binary_field_size = PQgetlength(last_res, 0, 0);
        	char *binary_field = reinterpret_cast<char *>(PQunescapeBytea((const unsigned char *)escaped_binary_field, &escaped_binary_field_size));
        	if (!binary_field) {
            	ERROR("QUBE_FUSE::qpsql_get_unquoted_value: Failed to unescape binary data: {}", PQerrorMessage(conn));
				FLUSH;
				return "";
			}else {
				DEBUG("QUBE_FUSE::qpsql_get_unquoted_value: unescaped string value = {}", binary_field);
				std::string tmp_field(binary_field);
				PQfreemem(binary_field);
				TRACE("QUBE_FUSE::qpsql_get_unquoted_value---[Leaving]---with binary data string: {}--->", tmp_field);
				FLUSH;
				return tmp_field;
			}
		}

		static std::vector<uint8_t> qpsql_get_unquoted_value(std::vector<uint8_t> *in_vec) {
    		TRACE("QUBE_FUSE::qpsql_get_unquoted_value---binary instr: {}--->", (char*)in_vec->data());
		 	const unsigned char *escaped_binary_field = q_convert::vect2char(in_vec);
        	size_t escaped_binary_field_size = in_vec->size();
        	unsigned char *binary_field = PQunescapeBytea(escaped_binary_field, &escaped_binary_field_size);
			std::vector<uint8_t> ret_binary;

        	if (!binary_field) {
            	ERROR("QUBE_FUSE::qpsql_get_unquoted_value: Failed to unescape binary data: {}", PQerrorMessage(conn));
				FLUSH;
				return ret_binary;
			}else {
				DEBUG("QUBE_FUSE::qpsql_get_unquoted_value: unescaped string value = {}", (char*)binary_field);
				ret_binary = q_convert::char2vect(binary_field);
				PQfreemem(binary_field);
				TRACE("QUBE_FUSE::qpsql_get_unquoted_value---[Leaving]---with binary data string: {}--->", (char*)ret_binary.data());
				FLUSH;
				return ret_binary;
			}
		}

};
/**
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
**/