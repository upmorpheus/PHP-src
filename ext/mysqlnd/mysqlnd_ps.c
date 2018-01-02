/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@php.net>                             |
  |          Ulf Wendel <uw@php.net>                                     |
  +----------------------------------------------------------------------+
*/

#include "php.h"
#include "mysqlnd.h"
#include "mysqlnd_wireprotocol.h"
#include "mysqlnd_connection.h"
#include "mysqlnd_priv.h"
#include "mysqlnd_ps.h"
#include "mysqlnd_result.h"
#include "mysqlnd_result_meta.h"
#include "mysqlnd_statistics.h"
#include "mysqlnd_debug.h"
#include "mysqlnd_block_alloc.h"
#include "mysqlnd_ext_plugin.h"

const char * const mysqlnd_not_bound_as_blob = "Can't send long data for non-string/non-binary data types";
const char * const mysqlnd_stmt_not_prepared = "Statement not prepared";

/* Exported by mysqlnd_ps_codec.c */
enum_func_status mysqlnd_stmt_execute_generate_request(MYSQLND_STMT * const s, zend_uchar ** request, size_t *request_len, zend_bool * free_buffer);
enum_func_status mysqlnd_stmt_execute_batch_generate_request(MYSQLND_STMT * const s, zend_uchar ** request, size_t *request_len, zend_bool * free_buffer);

static void mysqlnd_stmt_separate_result_bind(MYSQLND_STMT * const stmt);
static void mysqlnd_stmt_separate_one_result_bind(MYSQLND_STMT * const stmt, const unsigned int param_no);

/* {{{ mysqlnd_stmt::store_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_stmt, store_result)(MYSQLND_STMT * const s)
{
	enum_func_status ret;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_RES * result;

	DBG_ENTER("mysqlnd_stmt::store_result");
	if (!stmt || !conn || !stmt->result) {
		DBG_RETURN(NULL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	/* be compliant with libmysql - NULL will turn */
	if (!stmt->field_count) {
		DBG_RETURN(NULL);
	}

	if (stmt->cursor_exists) {
		/* Silently convert buffered to unbuffered, for now */
		DBG_RETURN(s->m->use_result(s));
	}

	/* Nothing to store for UPSERT/LOAD DATA*/
	if (GET_CONNECTION_STATE(&conn->state) != CONN_FETCHING_DATA || stmt->state != MYSQLND_STMT_WAITING_USE_OR_STORE)
	{
		SET_CLIENT_ERROR(conn->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_RETURN(NULL);
	}

	stmt->default_rset_handler = s->m->store_result;

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);
	MYSQLND_INC_CONN_STATISTIC(conn->stats, STAT_PS_BUFFERED_SETS);

	result = stmt->result;
	result->type			= MYSQLND_RES_PS_BUF;
/*	result->m.row_decoder = php_mysqlnd_rowp_read_binary_protocol; */

	result->stored_data	= (MYSQLND_RES_BUFFERED *) mysqlnd_result_buffered_zval_init(result->field_count, TRUE, result->persistent);
	if (!result->stored_data) {
		SET_OOM_ERROR(conn->error_info);
		DBG_RETURN(NULL);
	}

	ret = result->m.store_result_fetch_data(conn, result, result->meta, &result->stored_data->row_buffers, TRUE);

	result->stored_data->m.fetch_row = mysqlnd_stmt_fetch_row_buffered;

	if (PASS == ret) {
		if (result->stored_data->type == MYSQLND_BUFFERED_TYPE_ZVAL) {
			MYSQLND_RES_BUFFERED_ZVAL * set = (MYSQLND_RES_BUFFERED_ZVAL *) result->stored_data;
			if (result->stored_data->row_count) {
				/* don't try to allocate more than possible - mnd_XXalloc expects size_t, and it can have narrower range than uint64_t */
				if (result->stored_data->row_count * result->meta->field_count * sizeof(zval *) > SIZE_MAX) {
					SET_OOM_ERROR(conn->error_info);
					DBG_RETURN(NULL);
				}
				/* if pecalloc is used valgrind barks gcc version 4.3.1 20080507 (prerelease) [gcc-4_3-branch revision 135036] (SUSE Linux) */
				set->data = mnd_emalloc((size_t)(result->stored_data->row_count * result->meta->field_count * sizeof(zval)));
				if (!set->data) {
					SET_OOM_ERROR(conn->error_info);
					DBG_RETURN(NULL);
				}
				memset(set->data, 0, (size_t)(result->stored_data->row_count * result->meta->field_count * sizeof(zval)));
			}
			/* Position at the first row */
			set->data_cursor = set->data;
		} else if (result->stored_data->type == MYSQLND_BUFFERED_TYPE_ZVAL) {
			/*TODO*/
		}

		/* libmysql API docs say it should be so for SELECT statements */
		UPSERT_STATUS_SET_AFFECTED_ROWS(stmt->upsert_status, stmt->result->stored_data->row_count);

		stmt->state = MYSQLND_STMT_USE_OR_STORE_CALLED;
	} else {
		COPY_CLIENT_ERROR(conn->error_info, result->stored_data->error_info);
		stmt->result->m.free_result_contents(stmt->result);
		mnd_pefree(stmt->result, stmt->result->persistent);
		stmt->result = NULL;
		stmt->state = MYSQLND_STMT_PREPARED;
	}

	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_stmt::get_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_stmt, get_result)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_RES * result;

	DBG_ENTER("mysqlnd_stmt::get_result");
	if (!stmt || !conn || !stmt->result) {
		DBG_RETURN(NULL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	/* be compliant with libmysql - NULL will turn */
	if (!stmt->field_count) {
		DBG_RETURN(NULL);
	}

	if (stmt->cursor_exists) {
		/* Silently convert buffered to unbuffered, for now */
		DBG_RETURN(s->m->use_result(s));
	}

	/* Nothing to store for UPSERT/LOAD DATA*/
	if (GET_CONNECTION_STATE(&conn->state) != CONN_FETCHING_DATA || stmt->state != MYSQLND_STMT_WAITING_USE_OR_STORE) {
		SET_CLIENT_ERROR(conn->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_RETURN(NULL);
	}

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);
	MYSQLND_INC_CONN_STATISTIC(conn->stats, STAT_BUFFERED_SETS);

	do {
		result = conn->m->result_init(stmt->result->field_count, stmt->persistent);
		if (!result) {
			SET_OOM_ERROR(conn->error_info);
			break;
		}

		result->meta = stmt->result->meta->m->clone_metadata(stmt->result->meta, FALSE);
		if (!result->meta) {
			SET_OOM_ERROR(conn->error_info);
			break;
		}

		if ((result = result->m.store_result(result, conn, MYSQLND_STORE_PS | MYSQLND_STORE_NO_COPY))) {
			UPSERT_STATUS_SET_AFFECTED_ROWS(stmt->upsert_status, result->stored_data->row_count);
			stmt->state = MYSQLND_STMT_PREPARED;
			result->type = MYSQLND_RES_PS_BUF;
		} else {
			COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);
			stmt->state = MYSQLND_STMT_PREPARED;
			break;
		}
		DBG_RETURN(result);
	} while (0);

	if (result) {
		result->m.free_result(result, TRUE);
	}
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_stmt::more_results */
static zend_bool
MYSQLND_METHOD(mysqlnd_stmt, more_results)(const MYSQLND_STMT * s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	DBG_ENTER("mysqlnd_stmt::more_results");
	/* (conn->state == CONN_NEXT_RESULT_PENDING) too */
	DBG_RETURN((stmt && conn && (conn->m->get_server_status(conn) & SERVER_MORE_RESULTS_EXISTS))? TRUE: FALSE);
}
/* }}} */


/* {{{ mysqlnd_stmt::next_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, next_result)(MYSQLND_STMT * s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::next_result");
	if (!stmt || !conn || !stmt->result) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	if (GET_CONNECTION_STATE(&conn->state) != CONN_NEXT_RESULT_PENDING || !(UPSERT_STATUS_GET_SERVER_STATUS(conn->upsert_status) & SERVER_MORE_RESULTS_EXISTS)) {
		DBG_RETURN(FAIL);
	}

	DBG_INF_FMT("server_status=%u cursor=%u", UPSERT_STATUS_GET_SERVER_STATUS(conn->upsert_status), UPSERT_STATUS_GET_SERVER_STATUS(conn->upsert_status) & SERVER_STATUS_CURSOR_EXISTS);

	/* Free space for next result */
	s->m->free_stmt_result(s);
	{
		enum_func_status ret = s->m->parse_execute_response(s, MYSQLND_PARSE_EXEC_RESPONSE_IMPLICIT_NEXT_RESULT);
		DBG_RETURN(ret);
	}
}
/* }}} */


/* {{{ mysqlnd_stmt_skip_metadata */
static enum_func_status
mysqlnd_stmt_skip_metadata(MYSQLND_STMT * s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	/* Follows parameter metadata, we have just to skip it, as libmysql does */
	unsigned int i = 0;
	enum_func_status ret = FAIL;
	MYSQLND_PACKET_RES_FIELD * field_packet;

	DBG_ENTER("mysqlnd_stmt_skip_metadata");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	field_packet = conn->payload_decoder_factory->m.get_result_field_packet(conn->payload_decoder_factory, FALSE);
	if (!field_packet) {
		SET_OOM_ERROR(stmt->error_info);
		SET_OOM_ERROR(conn->error_info);
	} else {
		ret = PASS;
		field_packet->skip_parsing = TRUE;
		for (;i < stmt->param_count; i++) {
			if (FAIL == PACKET_READ(field_packet)) {
				ret = FAIL;
				break;
			}
		}
		PACKET_FREE(field_packet);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt_read_prepare_response */
static enum_func_status
mysqlnd_stmt_read_prepare_response(MYSQLND_STMT * s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_PACKET_PREPARE_RESPONSE * prepare_resp;
	enum_func_status ret = FAIL;

	DBG_ENTER("mysqlnd_stmt_read_prepare_response");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	prepare_resp = conn->payload_decoder_factory->m.get_prepare_response_packet(conn->payload_decoder_factory, FALSE);
	if (!prepare_resp) {
		SET_OOM_ERROR(stmt->error_info);
		SET_OOM_ERROR(conn->error_info);
		goto done;
	}

	if (FAIL == PACKET_READ(prepare_resp)) {
		goto done;
	}

	if (0xFF == prepare_resp->error_code) {
		COPY_CLIENT_ERROR(stmt->error_info, prepare_resp->error_info);
		COPY_CLIENT_ERROR(conn->error_info, prepare_resp->error_info);
		goto done;
	}
	ret = PASS;
	stmt->stmt_id = prepare_resp->stmt_id;
	UPSERT_STATUS_SET_WARNINGS(conn->upsert_status, prepare_resp->warning_count);
	UPSERT_STATUS_SET_AFFECTED_ROWS(stmt->upsert_status, 0);  /* be like libmysql */
	stmt->field_count = conn->field_count = prepare_resp->field_count;
	stmt->param_count = prepare_resp->param_count;
done:
	PACKET_FREE(prepare_resp);

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt_prepare_read_eof */
static enum_func_status
mysqlnd_stmt_prepare_read_eof(MYSQLND_STMT * s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_PACKET_EOF * fields_eof;
	enum_func_status ret = FAIL;

	DBG_ENTER("mysqlnd_stmt_prepare_read_eof");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	fields_eof = conn->payload_decoder_factory->m.get_eof_packet(conn->payload_decoder_factory, FALSE);
	if (!fields_eof) {
		SET_OOM_ERROR(stmt->error_info);
		SET_OOM_ERROR(conn->error_info);
	} else {
		if (FAIL == (ret = PACKET_READ(fields_eof))) {
			if (stmt->result) {
				stmt->result->m.free_result_contents(stmt->result);
				mnd_pefree(stmt->result, stmt->result->persistent);
				/* XXX: This will crash, because we will null also the methods.
					But seems it happens in extreme cases or doesn't. Should be fixed by exporting a function
					(from mysqlnd_driver.c?) to do the reset.
					This bad handling is also in mysqlnd_result.c
				*/
				memset(stmt, 0, sizeof(MYSQLND_STMT_DATA));
				stmt->state = MYSQLND_STMT_INITTED;
			}
		} else {
			UPSERT_STATUS_SET_SERVER_STATUS(stmt->upsert_status, fields_eof->server_status);
			UPSERT_STATUS_SET_WARNINGS(stmt->upsert_status, fields_eof->warning_count);
			stmt->state = MYSQLND_STMT_PREPARED;
		}
		PACKET_FREE(fields_eof);
	}

	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::prepare */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, prepare)(MYSQLND_STMT * const s, const char * const query, const size_t query_len)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_STMT * s_to_prepare = s;
	MYSQLND_STMT_DATA * stmt_to_prepare = stmt;

	DBG_ENTER("mysqlnd_stmt::prepare");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);
	DBG_INF_FMT("query=%s", query);

	UPSERT_STATUS_SET_AFFECTED_ROWS_TO_ERROR(stmt->upsert_status);
	UPSERT_STATUS_SET_AFFECTED_ROWS_TO_ERROR(conn->upsert_status);

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->state > MYSQLND_STMT_INITTED) {
		/* See if we have to clean the wire */
		if (stmt->state == MYSQLND_STMT_WAITING_USE_OR_STORE) {
			/* Do implicit use_result and then flush the result */
			stmt->default_rset_handler = s->m->use_result;
			stmt->default_rset_handler(s);
		}
		/* No 'else' here please :) */
		if (stmt->state > MYSQLND_STMT_WAITING_USE_OR_STORE && stmt->result) {
			stmt->result->m.skip_result(stmt->result);
		}
		/*
		  Create a new test statement, which we will prepare, but if anything
		  fails, we will scrap it.
		*/
		s_to_prepare = conn->m->stmt_init(conn);
		if (!s_to_prepare) {
			goto fail;
		}
		stmt_to_prepare = s_to_prepare->data;
	}

	{
		enum_func_status ret = FAIL;
		const MYSQLND_CSTRING query_string = {query, query_len};
		struct st_mysqlnd_protocol_command * command = conn->command_factory(COM_STMT_PREPARE, conn, query_string);
		if (command) {
			ret = command->run(command);
			command->free_command(command);
		}
		if (FAIL == ret) {
			goto fail;
		}
	}

	if (FAIL == mysqlnd_stmt_read_prepare_response(s_to_prepare)) {
		goto fail;
	}

	if (stmt_to_prepare->param_count) {
		if (FAIL == mysqlnd_stmt_skip_metadata(s_to_prepare) ||
			FAIL == mysqlnd_stmt_prepare_read_eof(s_to_prepare))
		{
			goto fail;
		}
	}

	/*
	  Read metadata only if there is actual result set.
	  Beware that SHOW statements bypass the PS framework and thus they send
	  no metadata at prepare.
	*/
	if (stmt_to_prepare->field_count) {
		MYSQLND_RES * result = conn->m->result_init(stmt_to_prepare->field_count, stmt_to_prepare->persistent);
		if (!result) {
			SET_OOM_ERROR(conn->error_info);
			goto fail;
		}
		/* Allocate the result now as it is needed for the reading of metadata */
		stmt_to_prepare->result = result;

		result->conn = conn->m->get_reference(conn);

		result->type = MYSQLND_RES_PS_BUF;

		if (FAIL == result->m.read_result_metadata(result, conn) ||
			FAIL == mysqlnd_stmt_prepare_read_eof(s_to_prepare))
		{
			goto fail;
		}
	}

	if (stmt_to_prepare != stmt) {
		/* swap */
		size_t real_size = sizeof(MYSQLND_STMT) + mysqlnd_plugin_count() * sizeof(void *);
		char * tmp_swap = mnd_malloc(real_size);
		memcpy(tmp_swap, s, real_size);
		memcpy(s, s_to_prepare, real_size);
		memcpy(s_to_prepare, tmp_swap, real_size);
		mnd_free(tmp_swap);
		{
			MYSQLND_STMT_DATA * tmp_swap_data = stmt_to_prepare;
			stmt_to_prepare = stmt;
			stmt = tmp_swap_data;
		}
		s_to_prepare->m->dtor(s_to_prepare, TRUE);
	}
	stmt->state = MYSQLND_STMT_PREPARED;
	DBG_INF("PASS");
	DBG_RETURN(PASS);

fail:
	if (stmt_to_prepare != stmt && s_to_prepare) {
		s_to_prepare->m->dtor(s_to_prepare, TRUE);
	}
	stmt->state = MYSQLND_STMT_INITTED;

	DBG_INF("FAIL");
	DBG_RETURN(FAIL);
}
/* }}} */


/* {{{ mysqlnd_stmt_execute_parse_response */
static enum_func_status
mysqlnd_stmt_execute_parse_response(MYSQLND_STMT * const s, enum_mysqlnd_parse_exec_response_type type)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	enum_func_status ret;

	DBG_ENTER("mysqlnd_stmt_execute_parse_response");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	SET_CONNECTION_STATE(&conn->state, CONN_QUERY_SENT);

	ret = conn->m->query_read_result_set_header(conn, s);
	if (ret == FAIL) {
		COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);
		UPSERT_STATUS_RESET(stmt->upsert_status);
		UPSERT_STATUS_SET_AFFECTED_ROWS(stmt->upsert_status, UPSERT_STATUS_GET_AFFECTED_ROWS(conn->upsert_status));
		if (GET_CONNECTION_STATE(&conn->state) == CONN_QUIT_SENT) {
			/* close the statement here, the connection has been closed */
		}
		stmt->state = MYSQLND_STMT_PREPARED;
		stmt->send_types_to_server = 1;
	} else {
		/*
		  stmt->send_types_to_server has already been set to 0 in
		  mysqlnd_stmt_execute_generate_request / mysqlnd_stmt_execute_store_params
		  In case there is a situation in which binding was done for integer and the
		  value is > LONG_MAX or < LONG_MIN, there is string conversion and we have
		  to resend the types. Next execution will also need to resend the type.
		*/
		SET_EMPTY_ERROR(stmt->error_info);
		SET_EMPTY_ERROR(conn->error_info);
		UPSERT_STATUS_SET_WARNINGS(stmt->upsert_status, UPSERT_STATUS_GET_WARNINGS(conn->upsert_status));
		UPSERT_STATUS_SET_AFFECTED_ROWS(stmt->upsert_status, UPSERT_STATUS_GET_AFFECTED_ROWS(conn->upsert_status));
		UPSERT_STATUS_SET_SERVER_STATUS(stmt->upsert_status, UPSERT_STATUS_GET_SERVER_STATUS(conn->upsert_status));
		UPSERT_STATUS_SET_LAST_INSERT_ID(stmt->upsert_status, UPSERT_STATUS_GET_LAST_INSERT_ID(conn->upsert_status));

		stmt->state = MYSQLND_STMT_EXECUTED;
		if (conn->last_query_type == QUERY_UPSERT || conn->last_query_type == QUERY_LOAD_LOCAL) {
			DBG_INF("PASS");
			DBG_RETURN(PASS);
		}

		stmt->result->type = MYSQLND_RES_PS_BUF;
		if (!stmt->result->conn) {
			/*
			  For SHOW we don't create (bypasses PS in server)
			  a result set at prepare and thus a connection was missing
			*/
			stmt->result->conn = conn->m->get_reference(conn);
		}

		/* Update stmt->field_count as SHOW sets it to 0 at prepare */
		stmt->field_count = stmt->result->field_count = conn->field_count;
		if (stmt->result->stored_data) {
			stmt->result->stored_data->lengths = NULL;
		} else if (stmt->result->unbuf) {
			stmt->result->unbuf->lengths = NULL;
		}
		if (stmt->field_count) {
			stmt->state = MYSQLND_STMT_WAITING_USE_OR_STORE;
			/*
			  We need to set this because the user might not call
			  use_result() or store_result() and we should be able to scrap the
			  data on the line, if he just decides to close the statement.
			*/
			DBG_INF_FMT("server_status=%u cursor=%u", UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status),
						UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status) & SERVER_STATUS_CURSOR_EXISTS);

			if (UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status) & SERVER_STATUS_CURSOR_EXISTS) {
				DBG_INF("cursor exists");
				stmt->cursor_exists = TRUE;
				SET_CONNECTION_STATE(&conn->state, CONN_READY);
				/* Only cursor read */
				stmt->default_rset_handler = s->m->use_result;
				DBG_INF("use_result");
			} else if (stmt->flags & CURSOR_TYPE_READ_ONLY) {
				DBG_INF("asked for cursor but got none");
				/*
				  We have asked for CURSOR but got no cursor, because the condition
				  above is not fulfilled. Then...

				  This is a single-row result set, a result set with no rows, EXPLAIN,
				  SHOW VARIABLES, or some other command which either a) bypasses the
				  cursors framework in the server and writes rows directly to the
				  network or b) is more efficient if all (few) result set rows are
				  precached on client and server's resources are freed.
				*/
				/* preferred is buffered read */
				stmt->default_rset_handler = s->m->store_result;
				DBG_INF("store_result");
			} else {
				DBG_INF("no cursor");
				/* preferred is unbuffered read */
				stmt->default_rset_handler = s->m->use_result;
				DBG_INF("use_result");
			}
		}
	}
#ifndef MYSQLND_DONT_SKIP_OUT_PARAMS_RESULTSET
	if (UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status) & SERVER_PS_OUT_PARAMS) {
		s->m->free_stmt_content(s);
		DBG_INF("PS OUT Variable RSet, skipping");
		/* OUT params result set. Skip for now to retain compatibility */
		ret = mysqlnd_stmt_execute_parse_response(s, MYSQLND_PARSE_EXEC_RESPONSE_IMPLICIT_OUT_VARIABLES);
	}
#endif

	DBG_INF_FMT("server_status=%u cursor=%u", UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status), UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status) & SERVER_STATUS_CURSOR_EXISTS);

	if (ret == PASS && conn->last_query_type == QUERY_UPSERT && UPSERT_STATUS_GET_AFFECTED_ROWS(stmt->upsert_status)) {
		MYSQLND_INC_CONN_STATISTIC_W_VALUE(conn->stats, STAT_ROWS_AFFECTED_PS, UPSERT_STATUS_GET_AFFECTED_ROWS(stmt->upsert_status));
	}

	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::execute */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, execute)(MYSQLND_STMT * const s)
{
	DBG_ENTER("mysqlnd_stmt::execute");
	if (FAIL == s->m->send_execute(s, MYSQLND_SEND_EXECUTE_IMPLICIT, NULL, NULL) ||
		FAIL == s->m->parse_execute_response(s, MYSQLND_PARSE_EXEC_RESPONSE_IMPLICIT))
	{
		DBG_RETURN(FAIL);
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::send_execute */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, send_execute)(MYSQLND_STMT * const s, const enum_mysqlnd_send_execute_type type, zval * read_cb, zval * err_cb)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	enum_func_status ret;
	zend_uchar *request = NULL;
	size_t		request_len;
	zend_bool	free_request;

	DBG_ENTER("mysqlnd_stmt::send_execute");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	UPSERT_STATUS_SET_AFFECTED_ROWS_TO_ERROR(stmt->upsert_status);
	UPSERT_STATUS_SET_AFFECTED_ROWS_TO_ERROR(conn->upsert_status);

	if (stmt->result && stmt->state >= MYSQLND_STMT_PREPARED && stmt->field_count) {
		/*
		  We don need to copy the data from the buffers which we will clean.
		  Because it has already been copied. See
		  #ifndef WE_DONT_COPY_IN_BUFFERED_AND_UNBUFFERED_BECAUSEOF_IS_REF
		*/
#ifdef WE_DONT_COPY_IN_BUFFERED_AND_UNBUFFERED_BECAUSEOF_IS_REF
		if (stmt->result_bind &&
			stmt->result_zvals_separated_once == TRUE &&
			stmt->state >= MYSQLND_STMT_USER_FETCHING)
		{
			/*
			  We need to copy the data from the buffers which we will clean.
			  The bound variables point to them only if the user has started
			  to fetch data (MYSQLND_STMT_USER_FETCHING).
			  We need to check 'result_zvals_separated_once' or we will leak
			  in the following scenario
			  prepare("select 1 from dual");
			  execute();
			  fetch(); <-- no binding, but that's not a problem
			  bind_result();
			  execute(); <-- here we will leak because we separate without need
			  */
			unsigned int i;
			for (i = 0; i < stmt->field_count; i++) {
				if (stmt->result_bind[i].bound == TRUE) {
					zval *result = &stmt->result_bind[i].zv;
					ZVAL_DEREF(result);
					Z_TRY_ADDREF_P(result);
				}
			}
		}
#endif

		s->m->flush(s);

		/*
		  Executed, but the user hasn't started to fetch
		  This will clean also the metadata, but after the EXECUTE call we will
		  have it again.
		*/
		stmt->result->m.free_result_buffers(stmt->result);

		stmt->state = MYSQLND_STMT_PREPARED;
	} else if (stmt->state < MYSQLND_STMT_PREPARED) {
		/* Only initted - error */
		SET_CLIENT_ERROR(stmt->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_INF("FAIL");
		DBG_RETURN(FAIL);
	}

	if (stmt->param_count) {
		unsigned int i, not_bound = 0;
		if (!stmt->param_bind) {
			SET_CLIENT_ERROR(stmt->error_info, CR_PARAMS_NOT_BOUND, UNKNOWN_SQLSTATE, "No data supplied for parameters in prepared statement");
			DBG_INF("FAIL");
			DBG_RETURN(FAIL);
		}
		for (i = 0; i < stmt->param_count; i++) {
			if (Z_ISUNDEF(stmt->param_bind[i].zv)) {
				not_bound++;
			}
		}
		if (not_bound) {
			char * msg;
			mnd_sprintf(&msg, 0, "No data supplied for %u parameter%s in prepared statement",
						not_bound, not_bound>1 ?"s":"");
			SET_CLIENT_ERROR(stmt->error_info, CR_PARAMS_NOT_BOUND, UNKNOWN_SQLSTATE, msg);
			if (msg) {
				mnd_sprintf_free(msg);
			}
			DBG_INF("FAIL");
			DBG_RETURN(FAIL);
		}
	}
	ret = s->m->generate_execute_request(s, &request, &request_len, &free_request);
	if (ret == PASS) {
		const MYSQLND_CSTRING payload = {(const char*) request, request_len};
		struct st_mysqlnd_protocol_command * command = conn->command_factory(COM_STMT_EXECUTE, conn, payload);
		ret = FAIL;
		if (command) {
			ret = command->run(command);
			command->free_command(command);
		}	
	} else {
		SET_CLIENT_ERROR(stmt->error_info, CR_UNKNOWN_ERROR, UNKNOWN_SQLSTATE, "Couldn't generate the request. Possibly OOM.");
	}

	if (free_request) {
		mnd_efree(request);
	}

	if (ret == FAIL) {
		COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);
		DBG_INF("FAIL");
		DBG_RETURN(FAIL);
	}
	stmt->execute_count++;

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt_fetch_row_buffered */
enum_func_status
mysqlnd_stmt_fetch_row_buffered(MYSQLND_RES * result, void * param, const unsigned int flags, zend_bool * fetched_anything)
{
	MYSQLND_STMT * s = (MYSQLND_STMT *) param;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	const MYSQLND_RES_METADATA * const meta = result->meta;
	unsigned int field_count = meta->field_count;

	DBG_ENTER("mysqlnd_stmt_fetch_row_buffered");
	*fetched_anything = FALSE;
	DBG_INF_FMT("stmt=%lu", stmt != NULL ? stmt->stmt_id : 0L);

	/* If we haven't read everything */
	if (result->stored_data->type == MYSQLND_BUFFERED_TYPE_ZVAL) {
		MYSQLND_RES_BUFFERED_ZVAL * set = (MYSQLND_RES_BUFFERED_ZVAL *) result->stored_data;
		if (set->data_cursor &&
			(set->data_cursor - set->data) < (result->stored_data->row_count * field_count))
		{
			/* The user could have skipped binding - don't crash*/
			if (stmt->result_bind) {
				unsigned int i;
				zval *current_row = set->data_cursor;

				if (Z_ISUNDEF(current_row[0])) {
					uint64_t row_num = (set->data_cursor - set->data) / field_count;
					enum_func_status rc = result->stored_data->m.row_decoder(result->stored_data->row_buffers[row_num],
													current_row,
													meta->field_count,
													meta->fields,
													result->conn->options->int_and_float_native,
													result->conn->stats);
					if (PASS != rc) {
						DBG_RETURN(FAIL);
					}
					result->stored_data->initialized_rows++;
					if (stmt->update_max_length) {
						for (i = 0; i < result->field_count; i++) {
							/*
							  NULL fields are 0 length, 0 is not more than 0
							  String of zero size, definitely can't be the next max_length.
							  Thus for NULL and zero-length we are quite efficient.
							*/
							if (Z_TYPE(current_row[i]) == IS_STRING) {
								zend_ulong len = Z_STRLEN(current_row[i]);
								if (meta->fields[i].max_length < len) {
									meta->fields[i].max_length = len;
								}
							}
						}
					}
				}

				for (i = 0; i < result->field_count; i++) {
					zval *result = &stmt->result_bind[i].zv;

					ZVAL_DEREF(result);
					/* Clean what we copied last time */
#ifndef WE_DONT_COPY_IN_BUFFERED_AND_UNBUFFERED_BECAUSEOF_IS_REF
					zval_dtor(result);
#endif
					/* copy the type */
					if (stmt->result_bind[i].bound == TRUE) {
						DBG_INF_FMT("i=%u type=%u", i, Z_TYPE(current_row[i]));
						if (Z_TYPE(current_row[i]) != IS_NULL) {
							/*
							  Copy the value.
							  Pre-condition is that the zvals in the result_bind buffer
							  have been  ZVAL_NULL()-ed or to another simple type
							  (int, double, bool but not string). Because of the reference
							  counting the user can't delete the strings the variables point to.
							*/

							ZVAL_COPY_VALUE(result, &current_row[i]);
#ifndef WE_DONT_COPY_IN_BUFFERED_AND_UNBUFFERED_BECAUSEOF_IS_REF
							Z_TRY_ADDREF_P(result);
#endif
						} else {
							ZVAL_NULL(result);
						}
					}
				}
			}
			set->data_cursor += field_count;
			*fetched_anything = TRUE;
			/* buffered result sets don't have a connection */
			MYSQLND_INC_GLOBAL_STATISTIC(STAT_ROWS_FETCHED_FROM_CLIENT_PS_BUF);
			DBG_INF("row fetched");
		} else {
			set->data_cursor = NULL;
			DBG_INF("no more data");
		}
	} else if (result->stored_data->type == MYSQLND_BUFFERED_TYPE_C) {
		/*TODO*/
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt_fetch_row_unbuffered */
enum_func_status
mysqlnd_stmt_fetch_row_unbuffered(MYSQLND_RES * result, void * param, const unsigned int flags, zend_bool * fetched_anything)
{
	enum_func_status ret;
	MYSQLND_STMT * s = (MYSQLND_STMT *) param;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_PACKET_ROW * row_packet;
	MYSQLND_CONN_DATA * conn = result->conn;
	const MYSQLND_RES_METADATA * const meta = result->meta;

	DBG_ENTER("mysqlnd_stmt_fetch_row_unbuffered");

	*fetched_anything = FALSE;

	if (result->unbuf->eof_reached) {
		/* No more rows obviously */
		DBG_INF("EOF already reached");
		DBG_RETURN(PASS);
	}
	if (GET_CONNECTION_STATE(&conn->state) != CONN_FETCHING_DATA) {
		SET_CLIENT_ERROR(conn->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_ERR("command out of sync");
		DBG_RETURN(FAIL);
	}
	if (!(row_packet = result->unbuf->row_packet)) {
		DBG_RETURN(FAIL);
	}

	/* Let the row packet fill our buffer and skip additional malloc + memcpy */
	row_packet->skip_extraction = stmt && stmt->result_bind? FALSE:TRUE;

	/*
	  If we skip rows (stmt == NULL || stmt->result_bind == NULL) we have to
	  result->unbuf->m.free_last_data() before it. The function returns always true.
	*/
	if (PASS == (ret = PACKET_READ(row_packet)) && !row_packet->eof) {
		unsigned int i, field_count = result->field_count;

		if (!row_packet->skip_extraction) {
			result->unbuf->m.free_last_data(result->unbuf, conn->stats);

			result->unbuf->last_row_data = row_packet->fields;
			result->unbuf->last_row_buffer = row_packet->row_buffer;
			row_packet->fields = NULL;
			row_packet->row_buffer = NULL;

			if (PASS != result->unbuf->m.row_decoder(result->unbuf->last_row_buffer,
									result->unbuf->last_row_data,
									row_packet->field_count,
									row_packet->fields_metadata,
									conn->options->int_and_float_native,
									conn->stats))
			{
				DBG_RETURN(FAIL);
			}

			for (i = 0; i < field_count; i++) {
				if (stmt->result_bind[i].bound == TRUE) {
					zval *data = &result->unbuf->last_row_data[i];
					zval *result = &stmt->result_bind[i].zv;

					ZVAL_DEREF(result);
					/*
					  stmt->result_bind[i].zv has been already destructed
					  in result->unbuf->m.free_last_data()
					*/
#ifndef WE_DONT_COPY_IN_BUFFERED_AND_UNBUFFERED_BECAUSEOF_IS_REF
					zval_dtor(result);
#endif
					if (!Z_ISNULL_P(data)) {
						if ((Z_TYPE_P(data) == IS_STRING) && (meta->fields[i].max_length < (zend_ulong) Z_STRLEN_P(data))){
							meta->fields[i].max_length = Z_STRLEN_P(data);
						}
						ZVAL_COPY_VALUE(result, data);
						/* copied data, thus also the ownership. Thus null data */
						ZVAL_NULL(data);
					} else {
						ZVAL_NULL(result);
					}
				}
			}
			MYSQLND_INC_CONN_STATISTIC(conn->stats, STAT_ROWS_FETCHED_FROM_CLIENT_PS_UNBUF);
		} else {
			DBG_INF("skipping extraction");
			/*
			  Data has been allocated and usually result->unbuf->m.free_last_data()
			  frees it but we can't call this function as it will cause problems with
			  the bound variables. Thus we need to do part of what it does or Zend will
			  report leaks.
			*/
			row_packet->result_set_memory_pool->free_chunk(
				row_packet->result_set_memory_pool, row_packet->row_buffer);
			row_packet->row_buffer = NULL;
		}

		result->unbuf->row_count++;
		*fetched_anything = TRUE;
	} else if (ret == FAIL) {
		if (row_packet->error_info.error_no) {
			COPY_CLIENT_ERROR(conn->error_info, row_packet->error_info);
			COPY_CLIENT_ERROR(stmt->error_info, row_packet->error_info);
		}
		SET_CONNECTION_STATE(&conn->state, CONN_READY);
		result->unbuf->eof_reached = TRUE; /* so next time we won't get an error */
	} else if (row_packet->eof) {
		DBG_INF("EOF");
		/* Mark the connection as usable again */
		result->unbuf->eof_reached = TRUE;
		UPSERT_STATUS_RESET(conn->upsert_status);
		UPSERT_STATUS_SET_WARNINGS(conn->upsert_status, row_packet->warning_count);
		UPSERT_STATUS_SET_SERVER_STATUS(conn->upsert_status, row_packet->server_status);

		/*
		  result->row_packet will be cleaned when
		  destroying the result object
		*/
		if (UPSERT_STATUS_GET_SERVER_STATUS(conn->upsert_status) & SERVER_MORE_RESULTS_EXISTS) {
			SET_CONNECTION_STATE(&conn->state, CONN_NEXT_RESULT_PENDING);
		} else {
			SET_CONNECTION_STATE(&conn->state, CONN_READY);
		}
	}

	DBG_INF_FMT("ret=%s fetched_anything=%u", ret == PASS? "PASS":"FAIL", *fetched_anything);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::use_result */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_stmt, use_result)(MYSQLND_STMT * s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_RES * result;

	DBG_ENTER("mysqlnd_stmt::use_result");
	if (!stmt || !conn || !stmt->result) {
		DBG_RETURN(NULL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	if (!stmt->field_count ||
		(!stmt->cursor_exists && GET_CONNECTION_STATE(&conn->state) != CONN_FETCHING_DATA) ||
		(stmt->cursor_exists && GET_CONNECTION_STATE(&conn->state) != CONN_READY) ||
		(stmt->state != MYSQLND_STMT_WAITING_USE_OR_STORE))
	{
		SET_CLIENT_ERROR(conn->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_ERR("command out of sync");
		DBG_RETURN(NULL);
	}

	SET_EMPTY_ERROR(stmt->error_info);

	MYSQLND_INC_CONN_STATISTIC(conn->stats, STAT_PS_UNBUFFERED_SETS);
	result = stmt->result;

	result->m.use_result(stmt->result, TRUE);
	result->unbuf->m.fetch_row	= stmt->cursor_exists? mysqlnd_fetch_stmt_row_cursor:
											   mysqlnd_stmt_fetch_row_unbuffered;
	stmt->state = MYSQLND_STMT_USE_OR_STORE_CALLED;

	DBG_INF_FMT("%p", result);
	DBG_RETURN(result);
}
/* }}} */


/* {{{ mysqlnd_fetch_row_cursor */
enum_func_status
mysqlnd_fetch_stmt_row_cursor(MYSQLND_RES * result, void * param, const unsigned int flags, zend_bool * fetched_anything)
{
	enum_func_status ret;
	MYSQLND_STMT * s = (MYSQLND_STMT *) param;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	zend_uchar buf[MYSQLND_STMT_ID_LENGTH /* statement id */ + 4 /* number of rows to fetch */];
	MYSQLND_PACKET_ROW * row_packet;

	DBG_ENTER("mysqlnd_fetch_stmt_row_cursor");

	if (!stmt || !stmt->conn || !result || !result->conn || !result->unbuf) {
		DBG_ERR("no statement");
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu flags=%u", stmt->stmt_id, flags);

	if (stmt->state < MYSQLND_STMT_USER_FETCHING) {
		/* Only initted - error */
		SET_CLIENT_ERROR(conn->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_ERR("command out of sync");
		DBG_RETURN(FAIL);
	}
	if (!(row_packet = result->unbuf->row_packet)) {
		DBG_RETURN(FAIL);
	}

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	int4store(buf, stmt->stmt_id);
	int4store(buf + MYSQLND_STMT_ID_LENGTH, 1); /* for now fetch only one row */

	{
		const MYSQLND_CSTRING payload = {(const char*) buf, sizeof(buf)};
		struct st_mysqlnd_protocol_command * command = conn->command_factory(COM_STMT_FETCH, conn, payload);
		ret = FAIL;
		if (command) {
			ret = command->run(command);
			command->free_command(command);
			if (ret == FAIL) {
				COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);	
			}
		}
		if (FAIL == ret) {
			DBG_RETURN(FAIL);
		}

	}

	row_packet->skip_extraction = stmt->result_bind? FALSE:TRUE;

	UPSERT_STATUS_RESET(stmt->upsert_status);
	if (PASS == (ret = PACKET_READ(row_packet)) && !row_packet->eof) {
		const MYSQLND_RES_METADATA * const meta = result->meta;
		unsigned int i, field_count = result->field_count;

		if (!row_packet->skip_extraction) {
			result->unbuf->m.free_last_data(result->unbuf, conn->stats);

			result->unbuf->last_row_data = row_packet->fields;
			result->unbuf->last_row_buffer = row_packet->row_buffer;
			row_packet->fields = NULL;
			row_packet->row_buffer = NULL;

			if (PASS != result->unbuf->m.row_decoder(result->unbuf->last_row_buffer,
									  result->unbuf->last_row_data,
									  row_packet->field_count,
									  row_packet->fields_metadata,
									  conn->options->int_and_float_native,
									  conn->stats))
			{
				DBG_RETURN(FAIL);
			}

			/* If no result bind, do nothing. We consumed the data */
			for (i = 0; i < field_count; i++) {
				if (stmt->result_bind[i].bound == TRUE) {
					zval *data = &result->unbuf->last_row_data[i];
					zval *result = &stmt->result_bind[i].zv;

					ZVAL_DEREF(result);
					/*
					  stmt->result_bind[i].zv has been already destructed
					  in result->unbuf->m.free_last_data()
					*/
#ifndef WE_DONT_COPY_IN_BUFFERED_AND_UNBUFFERED_BECAUSEOF_IS_REF
					zval_dtor(result);
#endif
					DBG_INF_FMT("i=%u bound_var=%p type=%u refc=%u", i, &stmt->result_bind[i].zv,
								Z_TYPE_P(data), Z_REFCOUNTED(stmt->result_bind[i].zv)?
							   	Z_REFCOUNT(stmt->result_bind[i].zv) : 0);

					if (!Z_ISNULL_P(data)) {
						if ((Z_TYPE_P(data) == IS_STRING) &&
								(meta->fields[i].max_length < (zend_ulong) Z_STRLEN_P(data))) {
							meta->fields[i].max_length = Z_STRLEN_P(data);
						}
						ZVAL_COPY_VALUE(result, data);
						/* copied data, thus also the ownership. Thus null data */
						ZVAL_NULL(data);
					} else {
						ZVAL_NULL(result);
					}
				}
			}
		} else {
			DBG_INF("skipping extraction");
			/*
			  Data has been allocated and usually result->unbuf->m.free_last_data()
			  frees it but we can't call this function as it will cause problems with
			  the bound variables. Thus we need to do part of what it does or Zend will
			  report leaks.
			*/
			row_packet->result_set_memory_pool->free_chunk(
				row_packet->result_set_memory_pool, row_packet->row_buffer);
			row_packet->row_buffer = NULL;
		}
		/* We asked for one row, the next one should be EOF, eat it */
		ret = PACKET_READ(row_packet);
		if (row_packet->row_buffer) {
			row_packet->result_set_memory_pool->free_chunk(
				row_packet->result_set_memory_pool, row_packet->row_buffer);
			row_packet->row_buffer = NULL;
		}
		MYSQLND_INC_CONN_STATISTIC(conn->stats, STAT_ROWS_FETCHED_FROM_CLIENT_PS_CURSOR);

		result->unbuf->row_count++;
		*fetched_anything = TRUE;
	} else {
		*fetched_anything = FALSE;
		UPSERT_STATUS_SET_WARNINGS(stmt->upsert_status, row_packet->warning_count);
		UPSERT_STATUS_SET_WARNINGS(conn->upsert_status, row_packet->warning_count);

		UPSERT_STATUS_SET_SERVER_STATUS(stmt->upsert_status, row_packet->server_status);
		UPSERT_STATUS_SET_SERVER_STATUS(conn->upsert_status, row_packet->server_status);

		result->unbuf->eof_reached = row_packet->eof;
	}
	UPSERT_STATUS_SET_WARNINGS(stmt->upsert_status, row_packet->warning_count);
	UPSERT_STATUS_SET_WARNINGS(conn->upsert_status, row_packet->warning_count);

	UPSERT_STATUS_SET_SERVER_STATUS(stmt->upsert_status, row_packet->server_status);
	UPSERT_STATUS_SET_SERVER_STATUS(conn->upsert_status, row_packet->server_status);

	DBG_INF_FMT("ret=%s fetched=%u server_status=%u warnings=%u eof=%u",
				ret == PASS? "PASS":"FAIL", *fetched_anything,
				row_packet->server_status, row_packet->warning_count,
				result->unbuf->eof_reached);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::fetch */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, fetch)(MYSQLND_STMT * const s, zend_bool * const fetched_anything)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	enum_func_status ret;
	DBG_ENTER("mysqlnd_stmt::fetch");
	if (!stmt || !stmt->conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	if (!stmt->result || stmt->state < MYSQLND_STMT_WAITING_USE_OR_STORE) {
		SET_CLIENT_ERROR(stmt->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_ERR("command out of sync");
		DBG_RETURN(FAIL);
	} else if (stmt->state == MYSQLND_STMT_WAITING_USE_OR_STORE) {
		/* Execute only once. We have to free the previous contents of user's bound vars */

		stmt->default_rset_handler(s);
	}
	stmt->state = MYSQLND_STMT_USER_FETCHING;

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	DBG_INF_FMT("result_bind=%p separated_once=%u", &stmt->result_bind, stmt->result_zvals_separated_once);
	/*
	  The user might have not bound any variables for result.
	  Do the binding once she does it.
	*/
	if (stmt->result_bind && !stmt->result_zvals_separated_once) {
		unsigned int i;
		/*
		  mysqlnd_stmt_store_result() has been called free the bind
		  variables to prevent leaking of their previous content.
		*/
		for (i = 0; i < stmt->result->field_count; i++) {
			if (stmt->result_bind[i].bound == TRUE) {
				zval *result = &stmt->result_bind[i].zv;
				ZVAL_DEREF(result);
				zval_dtor(result);
				ZVAL_NULL(result);
			}
		}
		stmt->result_zvals_separated_once = TRUE;
	}

	ret = stmt->result->m.fetch_row(stmt->result, (void*)s, 0, fetched_anything);
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::reset */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, reset)(MYSQLND_STMT * const s)
{
	enum_func_status ret = PASS;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::reset");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->stmt_id) {
		MYSQLND_CONN_DATA * conn = stmt->conn;
		if (stmt->param_bind) {
			unsigned int i;
			DBG_INF("resetting long data");
			/* Reset Long Data */
			for (i = 0; i < stmt->param_count; i++) {
				if (stmt->param_bind[i].flags & MYSQLND_PARAM_BIND_BLOB_USED) {
					stmt->param_bind[i].flags &= ~MYSQLND_PARAM_BIND_BLOB_USED;
				}
			}
		}

		s->m->flush(s);

		/*
		  Don't free now, let the result be usable. When the stmt will again be
		  executed then the result set will be cleaned, the bound variables will
		  be separated before that.
		*/

		if (GET_CONNECTION_STATE(&conn->state) == CONN_READY) {
			size_t stmt_id = stmt->stmt_id;
			struct st_mysqlnd_protocol_command * command = stmt->conn->command_factory(COM_STMT_RESET, stmt->conn, stmt_id);
			ret = FAIL;
			if (command) {
				ret = command->run(command);
				command->free_command(command);

				if (ret == FAIL) {
					COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);
				}
			}
		}
		*stmt->upsert_status = *conn->upsert_status;
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::flush */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, flush)(MYSQLND_STMT * const s)
{
	enum_func_status ret = PASS;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::flush");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	if (stmt->stmt_id) {
		/*
		  If the user decided to close the statement right after execute()
		  We have to call the appropriate use_result() or store_result() and
		  clean.
		*/
		do {
			if (stmt->state == MYSQLND_STMT_WAITING_USE_OR_STORE) {
				DBG_INF("fetching result set header");
				stmt->default_rset_handler(s);
				stmt->state = MYSQLND_STMT_USER_FETCHING;
			}

			if (stmt->result) {
				DBG_INF("skipping result");
				stmt->result->m.skip_result(stmt->result);
			}
		} while (mysqlnd_stmt_more_results(s) && mysqlnd_stmt_next_result(s) == PASS);
	}
	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::send_long_data */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, send_long_data)(MYSQLND_STMT * const s, unsigned int param_no,
							 				 const char * const data, zend_ulong data_length)
{
	enum_func_status ret = FAIL;
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	zend_uchar * cmd_buf;

	DBG_ENTER("mysqlnd_stmt::send_long_data");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu param_no=%u data_len=%lu", stmt->stmt_id, param_no, data_length);

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->state < MYSQLND_STMT_PREPARED) {
		SET_CLIENT_ERROR(stmt->error_info, CR_NO_PREPARE_STMT, UNKNOWN_SQLSTATE, mysqlnd_stmt_not_prepared);
		DBG_ERR("not prepared");
		DBG_RETURN(FAIL);
	}
	if (!stmt->param_bind) {
		SET_CLIENT_ERROR(stmt->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, mysqlnd_out_of_sync);
		DBG_ERR("command out of sync");
		DBG_RETURN(FAIL);
	}
	if (param_no >= stmt->param_count) {
		SET_CLIENT_ERROR(stmt->error_info, CR_INVALID_PARAMETER_NO, UNKNOWN_SQLSTATE, "Invalid parameter number");
		DBG_ERR("invalid param_no");
		DBG_RETURN(FAIL);
	}
	if (stmt->param_bind[param_no].type != MYSQL_TYPE_LONG_BLOB) {
		SET_CLIENT_ERROR(stmt->error_info, CR_INVALID_BUFFER_USE, UNKNOWN_SQLSTATE, mysqlnd_not_bound_as_blob);
		DBG_ERR("param_no is not of a blob type");
		DBG_RETURN(FAIL);
	}

	if (GET_CONNECTION_STATE(&conn->state) == CONN_READY) {
		const size_t packet_len = MYSQLND_STMT_ID_LENGTH + 2 + data_length;
		cmd_buf = mnd_emalloc(packet_len);
		if (cmd_buf) {
			stmt->param_bind[param_no].flags |= MYSQLND_PARAM_BIND_BLOB_USED;

			int4store(cmd_buf, stmt->stmt_id);
			int2store(cmd_buf + MYSQLND_STMT_ID_LENGTH, param_no);
			memcpy(cmd_buf + MYSQLND_STMT_ID_LENGTH + 2, data, data_length);

			/* COM_STMT_SEND_LONG_DATA doesn't acknowledge with an OK packet */
			{
				const MYSQLND_CSTRING payload = {(const char *) cmd_buf, packet_len};
				struct st_mysqlnd_protocol_command * command = conn->command_factory(COM_STMT_SEND_LONG_DATA, conn, payload);
				ret = FAIL;
				if (command) {
					ret = command->run(command);
					command->free_command(command);
					if (ret == FAIL) {
						COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);
					}
				}
			}

			mnd_efree(cmd_buf);
		} else {
			ret = FAIL;
			SET_OOM_ERROR(stmt->error_info);
			SET_OOM_ERROR(conn->error_info);
		}
		/*
		  Cover protocol error: COM_STMT_SEND_LONG_DATA was designed to be quick and not
		  sent response packets. According to documentation the only way to get an error
		  is to have out-of-memory on the server-side. However, that's not true, as if
		  max_allowed_packet_size is smaller than the chunk being sent to the server, the
		  latter will complain with an error message. However, normally we don't expect
		  an error message, thus we continue. When sending the next command, which expects
		  response we will read the unexpected data and error message will look weird.
		  Therefore we do non-blocking read to clean the line, if there is a need.
		  Nevertheless, there is a built-in protection when sending a command packet, that
		  checks if the line is clear - useful for debug purposes and to be switched off
		  in release builds.

		  Maybe we can make it automatic by checking what's the value of
		  max_allowed_packet_size on the server and resending the data.
		*/
#ifdef MYSQLND_DO_WIRE_CHECK_BEFORE_COMMAND
#if HAVE_USLEEP && !defined(PHP_WIN32)
		usleep(120000);
#endif
		if ((packet_len = conn->protocol_frame_codec->m.consume_uneaten_data(conn->protocol_frame_codec, COM_STMT_SEND_LONG_DATA))) {
			php_error_docref(NULL, E_WARNING, "There was an error "
							 "while sending long data. Probably max_allowed_packet_size "
							 "is smaller than the data. You have to increase it or send "
							 "smaller chunks of data. Answer was "MYSQLND_SZ_T_SPEC" bytes long.", packet_len);
			SET_CLIENT_ERROR(stmt->error_info, CR_CONNECTION_ERROR, UNKNOWN_SQLSTATE,
							"Server responded to COM_STMT_SEND_LONG_DATA.");
			ret = FAIL;
		}
#endif
	}

	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::bind_parameters */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, bind_parameters)(MYSQLND_STMT * const s, MYSQLND_PARAM_BIND * const param_bind)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::bind_param");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu param_count=%u", stmt->stmt_id, stmt->param_count);

	if (stmt->state < MYSQLND_STMT_PREPARED) {
		SET_CLIENT_ERROR(stmt->error_info, CR_NO_PREPARE_STMT, UNKNOWN_SQLSTATE, mysqlnd_stmt_not_prepared);
		DBG_ERR("not prepared");
		if (param_bind) {
			s->m->free_parameter_bind(s, param_bind);
		}
		DBG_RETURN(FAIL);
	}

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->param_count) {
		unsigned int i = 0;

		if (!param_bind) {
			SET_CLIENT_ERROR(stmt->error_info, CR_COMMANDS_OUT_OF_SYNC, UNKNOWN_SQLSTATE, "Re-binding (still) not supported");
			DBG_ERR("Re-binding (still) not supported");
			DBG_RETURN(FAIL);
		} else if (stmt->param_bind) {
			DBG_INF("Binding");
			/*
			  There is already result bound.
			  Forbid for now re-binding!!
			*/
			for (i = 0; i < stmt->param_count; i++) {
				/*
				  We may have the last reference, then call zval_ptr_dtor() or we may leak memory.
				  Switching from bind_one_parameter to bind_parameters may result in zv being NULL
				*/
				zval_ptr_dtor(&stmt->param_bind[i].zv);
			}
			if (stmt->param_bind != param_bind) {
				s->m->free_parameter_bind(s, stmt->param_bind);
			}
		}

		stmt->param_bind = param_bind;
		for (i = 0; i < stmt->param_count; i++) {
			/* The client will use stmt_send_long_data */
			DBG_INF_FMT("%u is of type %u", i, stmt->param_bind[i].type);
			/* Prevent from freeing */
			/* Don't update is_ref, or we will leak during conversion */
			Z_TRY_ADDREF(stmt->param_bind[i].zv);
			stmt->param_bind[i].flags = 0;
			if (stmt->param_bind[i].type == MYSQL_TYPE_LONG_BLOB) {
				stmt->param_bind[i].flags &= ~MYSQLND_PARAM_BIND_BLOB_USED;
			}
		}
		stmt->send_types_to_server = 1;
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::bind_one_parameter */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, bind_one_parameter)(MYSQLND_STMT * const s, unsigned int param_no,
												 zval * const zv, zend_uchar type)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::bind_one_parameter");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu param_no=%u param_count=%u type=%u", stmt->stmt_id, param_no, stmt->param_count, type);

	if (stmt->state < MYSQLND_STMT_PREPARED) {
		SET_CLIENT_ERROR(stmt->error_info, CR_NO_PREPARE_STMT, UNKNOWN_SQLSTATE, mysqlnd_stmt_not_prepared);
		DBG_ERR("not prepared");
		DBG_RETURN(FAIL);
	}

	if (param_no >= stmt->param_count) {
		SET_CLIENT_ERROR(stmt->error_info, CR_INVALID_PARAMETER_NO, UNKNOWN_SQLSTATE, "Invalid parameter number");
		DBG_ERR("invalid param_no");
		DBG_RETURN(FAIL);
	}
	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->param_count) {
		if (!stmt->param_bind) {
			stmt->param_bind = mnd_pecalloc(stmt->param_count, sizeof(MYSQLND_PARAM_BIND), stmt->persistent);
			if (!stmt->param_bind) {
				DBG_RETURN(FAIL);
			}
		}

		/* Prevent from freeing */
		/* Don't update is_ref, or we will leak during conversion */
		Z_TRY_ADDREF_P(zv);
		DBG_INF("Binding");
		/* Release what we had, if we had */
		zval_ptr_dtor(&stmt->param_bind[param_no].zv);
		if (type == MYSQL_TYPE_LONG_BLOB) {
			/* The client will use stmt_send_long_data */
			stmt->param_bind[param_no].flags &= ~MYSQLND_PARAM_BIND_BLOB_USED;
		}
		ZVAL_COPY_VALUE(&stmt->param_bind[param_no].zv, zv);
		stmt->param_bind[param_no].type = type;

		stmt->send_types_to_server = 1;
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::refresh_bind_param */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, refresh_bind_param)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::refresh_bind_param");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu param_count=%u", stmt->stmt_id, stmt->param_count);

	if (stmt->state < MYSQLND_STMT_PREPARED) {
		SET_CLIENT_ERROR(stmt->error_info, CR_NO_PREPARE_STMT, UNKNOWN_SQLSTATE, mysqlnd_stmt_not_prepared);
		DBG_ERR("not prepared");
		DBG_RETURN(FAIL);
	}

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->param_count) {
		stmt->send_types_to_server = 1;
	}
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::bind_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, bind_result)(MYSQLND_STMT * const s,
										  MYSQLND_RESULT_BIND * const result_bind)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::bind_result");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu field_count=%u", stmt->stmt_id, stmt->field_count);

	if (stmt->state < MYSQLND_STMT_PREPARED) {
		SET_CLIENT_ERROR(stmt->error_info, CR_NO_PREPARE_STMT, UNKNOWN_SQLSTATE, mysqlnd_stmt_not_prepared);
		if (result_bind) {
			s->m->free_result_bind(s, result_bind);
		}
		DBG_ERR("not prepared");
		DBG_RETURN(FAIL);
	}

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->field_count) {
		unsigned int i = 0;

		if (!result_bind) {
			DBG_ERR("no result bind passed");
			DBG_RETURN(FAIL);
		}

		mysqlnd_stmt_separate_result_bind(s);
		stmt->result_zvals_separated_once = FALSE;
		stmt->result_bind = result_bind;
		for (i = 0; i < stmt->field_count; i++) {
			/* Prevent from freeing */
			Z_TRY_ADDREF(stmt->result_bind[i].zv);

			DBG_INF_FMT("ref of %p = %u", &stmt->result_bind[i].zv,
					Z_REFCOUNTED(stmt->result_bind[i].zv)? Z_REFCOUNT(stmt->result_bind[i].zv) : 0);
			/*
			  Don't update is_ref !!! it's not our job
			  Otherwise either 009.phpt or mysqli_stmt_bind_result.phpt
			  will fail.
			*/
			stmt->result_bind[i].bound = TRUE;
		}
	} else if (result_bind) {
		s->m->free_result_bind(s, result_bind);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::bind_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, bind_one_result)(MYSQLND_STMT * const s, unsigned int param_no)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::bind_result");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu field_count=%u", stmt->stmt_id, stmt->field_count);

	if (stmt->state < MYSQLND_STMT_PREPARED) {
		SET_CLIENT_ERROR(stmt->error_info, CR_NO_PREPARE_STMT, UNKNOWN_SQLSTATE, mysqlnd_stmt_not_prepared);
		DBG_ERR("not prepared");
		DBG_RETURN(FAIL);
	}

	if (param_no >= stmt->field_count) {
		SET_CLIENT_ERROR(stmt->error_info, CR_INVALID_PARAMETER_NO, UNKNOWN_SQLSTATE, "Invalid parameter number");
		DBG_ERR("invalid param_no");
		DBG_RETURN(FAIL);
	}

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	if (stmt->field_count) {
		mysqlnd_stmt_separate_one_result_bind(s, param_no);
		/* Guaranteed is that stmt->result_bind is NULL */
		if (!stmt->result_bind) {
			stmt->result_bind = mnd_pecalloc(stmt->field_count, sizeof(MYSQLND_RESULT_BIND), stmt->persistent);
		} else {
			stmt->result_bind = mnd_perealloc(stmt->result_bind, stmt->field_count * sizeof(MYSQLND_RESULT_BIND), stmt->persistent);
		}
		if (!stmt->result_bind) {
			DBG_RETURN(FAIL);
		}
		ZVAL_NULL(&stmt->result_bind[param_no].zv);
		/*
		  Don't update is_ref !!! it's not our job
		  Otherwise either 009.phpt or mysqli_stmt_bind_result.phpt
		  will fail.
		*/
		stmt->result_bind[param_no].bound = TRUE;
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::insert_id */
static uint64_t
MYSQLND_METHOD(mysqlnd_stmt, insert_id)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? UPSERT_STATUS_GET_LAST_INSERT_ID(stmt->upsert_status) : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::affected_rows */
static uint64_t
MYSQLND_METHOD(mysqlnd_stmt, affected_rows)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? UPSERT_STATUS_GET_AFFECTED_ROWS(stmt->upsert_status) : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::num_rows */
static uint64_t
MYSQLND_METHOD(mysqlnd_stmt, num_rows)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt && stmt->result? mysqlnd_num_rows(stmt->result):0;
}
/* }}} */


/* {{{ mysqlnd_stmt::warning_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_stmt, warning_count)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? UPSERT_STATUS_GET_WARNINGS(stmt->upsert_status) : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::server_status */
static unsigned int
MYSQLND_METHOD(mysqlnd_stmt, server_status)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? UPSERT_STATUS_GET_SERVER_STATUS(stmt->upsert_status) : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::field_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_stmt, field_count)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? stmt->field_count : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::param_count */
static unsigned int
MYSQLND_METHOD(mysqlnd_stmt, param_count)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? stmt->param_count : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::errno */
static unsigned int
MYSQLND_METHOD(mysqlnd_stmt, errno)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? stmt->error_info->error_no : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::error */
static const char *
MYSQLND_METHOD(mysqlnd_stmt, error)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt? stmt->error_info->error : 0;
}
/* }}} */


/* {{{ mysqlnd_stmt::sqlstate */
static const char *
MYSQLND_METHOD(mysqlnd_stmt, sqlstate)(const MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt && stmt->error_info->sqlstate[0] ? stmt->error_info->sqlstate:MYSQLND_SQLSTATE_NULL;
}
/* }}} */


/* {{{ mysqlnd_stmt::data_seek */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, data_seek)(const MYSQLND_STMT * const s, uint64_t row)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	return stmt && stmt->result? stmt->result->m.seek_data(stmt->result, row) : FAIL;
}
/* }}} */


/* {{{ mysqlnd_stmt::param_metadata */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_stmt, param_metadata)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	if (!stmt || !stmt->param_count) {
		return NULL;
	}
	return NULL;
}
/* }}} */


/* {{{ mysqlnd_stmt::result_metadata */
static MYSQLND_RES *
MYSQLND_METHOD(mysqlnd_stmt, result_metadata)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	MYSQLND_RES * result_meta = NULL;

	DBG_ENTER("mysqlnd_stmt::result_metadata");
	if (!stmt || ! conn) {
		DBG_RETURN(NULL);
	}
	DBG_INF_FMT("stmt=%u field_count=%u", stmt->stmt_id, stmt->field_count);

	if (!stmt->field_count || !stmt->result || !stmt->result->meta) {
		DBG_INF("NULL");
		DBG_RETURN(NULL);
	}

	if (stmt->update_max_length && stmt->result->stored_data) {
		/* stored result, we have to update the max_length before we clone the meta data :( */
		stmt->result->stored_data->m.initialize_result_set_rest(stmt->result->stored_data,
																stmt->result->meta,
																conn->stats,
																conn->options->int_and_float_native);
	}
	/*
	  TODO: This implementation is kind of a hack,
			find a better way to do it. In different functions I have put
			fuses to check for result->m.fetch_row() being NULL. This should
			be handled in a better way.
	*/
	do {
		result_meta = conn->m->result_init(stmt->field_count, stmt->persistent);
		if (!result_meta) {
			break;
		}
		result_meta->type = MYSQLND_RES_NORMAL;
		result_meta->unbuf = mysqlnd_result_unbuffered_init(stmt->field_count, TRUE, result_meta->persistent);
		if (!result_meta->unbuf) {
			break;
		}
		result_meta->unbuf->eof_reached = TRUE;
		result_meta->meta = stmt->result->meta->m->clone_metadata(stmt->result->meta, FALSE);
		if (!result_meta->meta) {
			break;
		}

		DBG_INF_FMT("result_meta=%p", result_meta);
		DBG_RETURN(result_meta);
	} while (0);

	SET_OOM_ERROR(conn->error_info);
	if (result_meta) {
		result_meta->m.free_result(result_meta, TRUE);
	}
	DBG_RETURN(NULL);
}
/* }}} */


/* {{{ mysqlnd_stmt::attr_set */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, attr_set)(MYSQLND_STMT * const s,
									   enum mysqlnd_stmt_attr attr_type,
									   const void * const value)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt::attr_set");
	if (!stmt) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu attr_type=%u", stmt->stmt_id, attr_type);

	switch (attr_type) {
		case STMT_ATTR_UPDATE_MAX_LENGTH:{
			zend_uchar bval = *(zend_uchar *) value;
			/*
			  XXX : libmysql uses my_bool, but mysqli uses ulong as storage on the stack
			  and mysqlnd won't be used out of the scope of PHP -> use ulong.
			*/
			stmt->update_max_length = bval? TRUE:FALSE;
			break;
		}
		case STMT_ATTR_CURSOR_TYPE: {
			unsigned int ival = *(unsigned int *) value;
			if (ival > (zend_ulong) CURSOR_TYPE_READ_ONLY) {
				SET_CLIENT_ERROR(stmt->error_info, CR_NOT_IMPLEMENTED, UNKNOWN_SQLSTATE, "Not implemented");
				DBG_INF("FAIL");
				DBG_RETURN(FAIL);
			}
			stmt->flags = ival;
			break;
		}
		case STMT_ATTR_PREFETCH_ROWS: {
			unsigned int ival = *(unsigned int *) value;
			if (ival == 0) {
				ival = MYSQLND_DEFAULT_PREFETCH_ROWS;
			} else if (ival > 1) {
				SET_CLIENT_ERROR(stmt->error_info, CR_NOT_IMPLEMENTED, UNKNOWN_SQLSTATE, "Not implemented");
				DBG_INF("FAIL");
				DBG_RETURN(FAIL);
			}
			stmt->prefetch_rows = ival;
			break;
		}
		default:
			SET_CLIENT_ERROR(stmt->error_info, CR_NOT_IMPLEMENTED, UNKNOWN_SQLSTATE, "Not implemented");
			DBG_RETURN(FAIL);
	}
	DBG_INF("PASS");
	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt::attr_get */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, attr_get)(const MYSQLND_STMT * const s,
									   enum mysqlnd_stmt_attr attr_type,
									   void * const value)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt::attr_set");
	if (!stmt) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu attr_type=%u", stmt->stmt_id, attr_type);

	switch (attr_type) {
		case STMT_ATTR_UPDATE_MAX_LENGTH:
			*(zend_bool *) value= stmt->update_max_length;
			break;
		case STMT_ATTR_CURSOR_TYPE:
			*(zend_ulong *) value= stmt->flags;
			break;
		case STMT_ATTR_PREFETCH_ROWS:
			*(zend_ulong *) value= stmt->prefetch_rows;
			break;
		default:
			DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("value=%lu", value);
	DBG_RETURN(PASS);
}
/* }}} */


/* free_result() doesn't actually free stmt->result but only the buffers */
/* {{{ mysqlnd_stmt::free_result */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, free_result)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;

	DBG_ENTER("mysqlnd_stmt::free_result");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	if (!stmt->result) {
		DBG_INF("no result");
		DBG_RETURN(PASS);
	}

	/*
	  If right after execute() we have to call the appropriate
	  use_result() or store_result() and clean.
	*/
	if (stmt->state == MYSQLND_STMT_WAITING_USE_OR_STORE) {
		DBG_INF("fetching result set header");
		/* Do implicit use_result and then flush the result */
		stmt->default_rset_handler = s->m->use_result;
		stmt->default_rset_handler(s);
	}

	if (stmt->state > MYSQLND_STMT_WAITING_USE_OR_STORE) {
		DBG_INF("skipping result");
		/* Flush if anything is left and unbuffered set */
		stmt->result->m.skip_result(stmt->result);
		/*
		  Separate the bound variables, which point to the result set, then
		  destroy the set.
		*/
		mysqlnd_stmt_separate_result_bind(s);

		/* Now we can destroy the result set */
		stmt->result->m.free_result_buffers(stmt->result);
	}

	if (stmt->state > MYSQLND_STMT_PREPARED) {
		/* As the buffers have been freed, we should go back to PREPARED */
		stmt->state = MYSQLND_STMT_PREPARED;
	}

	if (GET_CONNECTION_STATE(&conn->state) != CONN_QUIT_SENT) {
		SET_CONNECTION_STATE(&conn->state, CONN_READY);
	}

	DBG_RETURN(PASS);
}
/* }}} */


/* {{{ mysqlnd_stmt_separate_result_bind */
static void
mysqlnd_stmt_separate_result_bind(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	unsigned int i;

	DBG_ENTER("mysqlnd_stmt_separate_result_bind");
	if (!stmt) {
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("stmt=%lu result_bind=%p field_count=%u", stmt->stmt_id, stmt->result_bind, stmt->field_count);

	if (!stmt->result_bind) {
		DBG_VOID_RETURN;
	}

	/*
	  Because only the bound variables can point to our internal buffers, then
	  separate or free only them. Free is possible because the user could have
	  lost reference.
	*/
	for (i = 0; i < stmt->field_count; i++) {
		/* Let's try with no cache */
		if (stmt->result_bind[i].bound == TRUE) {
			DBG_INF_FMT("%u has refcount=%u", i, Z_REFCOUNTED(stmt->result_bind[i].zv)? Z_REFCOUNT(stmt->result_bind[i].zv) : 0);
			zval_ptr_dtor(&stmt->result_bind[i].zv);
		}
	}

	s->m->free_result_bind(s, stmt->result_bind);
	stmt->result_bind = NULL;

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_stmt_separate_one_result_bind */
static void
mysqlnd_stmt_separate_one_result_bind(MYSQLND_STMT * const s, const unsigned int param_no)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt_separate_one_result_bind");
	if (!stmt) {
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("stmt=%lu result_bind=%p field_count=%u param_no=%u", stmt->stmt_id, stmt->result_bind, stmt->field_count, param_no);

	if (!stmt->result_bind) {
		DBG_VOID_RETURN;
	}

	/*
	  Because only the bound variables can point to our internal buffers, then
	  separate or free only them. Free is possible because the user could have
	  lost reference.
	*/
	/* Let's try with no cache */
	if (stmt->result_bind[param_no].bound == TRUE) {
		DBG_INF_FMT("%u has refcount=%u", param_no, Z_REFCOUNTED(stmt->result_bind[param_no].zv)? Z_REFCOUNT(stmt->result_bind[param_no].zv) : 0);
		zval_ptr_dtor(&stmt->result_bind[param_no].zv);
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_stmt::free_stmt_result */
static void
MYSQLND_METHOD(mysqlnd_stmt, free_stmt_result)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt::free_stmt_result");
	if (!stmt) {
		DBG_VOID_RETURN;
	}

	/*
	  First separate the bound variables, which point to the result set, then
	  destroy the set.
	*/
	mysqlnd_stmt_separate_result_bind(s);
	/* Not every statement has a result set attached */
	if (stmt->result) {
		stmt->result->m.free_result_internal(stmt->result);
		stmt->result = NULL;
	}
	if (stmt->error_info->error_list) {
		zend_llist_clean(stmt->error_info->error_list);
		mnd_pefree(stmt->error_info->error_list, s->persistent);
		stmt->error_info->error_list = NULL;
	}

	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_stmt::free_stmt_content */
static void
MYSQLND_METHOD(mysqlnd_stmt, free_stmt_content)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt::free_stmt_content");
	if (!stmt) {
		DBG_VOID_RETURN;
	}
	DBG_INF_FMT("stmt=%lu param_bind=%p param_count=%u", stmt->stmt_id, stmt->param_bind, stmt->param_count);

	/* Destroy the input bind */
	if (stmt->param_bind) {
		unsigned int i;
		/*
		  Because only the bound variables can point to our internal buffers, then
		  separate or free only them. Free is possible because the user could have
		  lost reference.
		*/
		for (i = 0; i < stmt->param_count; i++) {
			/*
			  If bind_one_parameter was used, but not everything was
			  bound and nothing was fetched, then some `zv` could be NULL
			*/
			zval_ptr_dtor(&stmt->param_bind[i].zv);
		}
		s->m->free_parameter_bind(s, stmt->param_bind);
		stmt->param_bind = NULL;
	}

	s->m->free_stmt_result(s);
	DBG_VOID_RETURN;
}
/* }}} */


/* {{{ mysqlnd_stmt::close_on_server */
static enum_func_status
MYSQLND_METHOD_PRIVATE(mysqlnd_stmt, close_on_server)(MYSQLND_STMT * const s, zend_bool implicit)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	MYSQLND_CONN_DATA * conn = stmt? stmt->conn : NULL;
	enum_mysqlnd_collected_stats statistic = STAT_LAST;

	DBG_ENTER("mysqlnd_stmt::close_on_server");
	if (!stmt || !conn) {
		DBG_RETURN(FAIL);
	}
	DBG_INF_FMT("stmt=%lu", stmt->stmt_id);

	SET_EMPTY_ERROR(stmt->error_info);
	SET_EMPTY_ERROR(conn->error_info);

	/*
	  If the user decided to close the statement right after execute()
	  We have to call the appropriate use_result() or store_result() and
	  clean.
	*/
	do {
		if (stmt->state == MYSQLND_STMT_WAITING_USE_OR_STORE) {
			DBG_INF("fetching result set header");
			stmt->default_rset_handler(s);
			stmt->state = MYSQLND_STMT_USER_FETCHING;
		}

		/* unbuffered set not fetched to the end ? Clean the line */
		if (stmt->result) {
			DBG_INF("skipping result");
			stmt->result->m.skip_result(stmt->result);
		}
	} while (mysqlnd_stmt_more_results(s) && mysqlnd_stmt_next_result(s) == PASS);
	/*
	  After this point we are allowed to free the result set,
	  as we have cleaned the line
	*/
	if (stmt->stmt_id) {
		MYSQLND_INC_GLOBAL_STATISTIC(implicit == TRUE?	STAT_FREE_RESULT_IMPLICIT:
														STAT_FREE_RESULT_EXPLICIT);

		if (GET_CONNECTION_STATE(&conn->state) == CONN_READY) {
			enum_func_status ret = FAIL;
			size_t stmt_id = stmt->stmt_id;
			struct st_mysqlnd_protocol_command * command = conn->command_factory(COM_STMT_CLOSE, conn, stmt_id);
			if (command) {
				ret = command->run(command);
				command->free_command(command);

				if (ret == FAIL) {
					COPY_CLIENT_ERROR(stmt->error_info, *conn->error_info);
				}
			}
			if (ret == FAIL) {
				DBG_RETURN(FAIL);
			}
		}
	}
	switch (stmt->execute_count) {
		case 0:
			statistic = STAT_PS_PREPARED_NEVER_EXECUTED;
			break;
		case 1:
			statistic = STAT_PS_PREPARED_ONCE_USED;
			break;
		default:
			break;
	}
	if (statistic != STAT_LAST) {
		MYSQLND_INC_CONN_STATISTIC(conn->stats, statistic);
	}

	if (stmt->execute_cmd_buffer.buffer) {
		mnd_pefree(stmt->execute_cmd_buffer.buffer, stmt->persistent);
		stmt->execute_cmd_buffer.buffer = NULL;
	}

	s->m->free_stmt_content(s);

	if (conn) {
		conn->m->free_reference(conn);
		stmt->conn = NULL;
	}

	DBG_RETURN(PASS);
}
/* }}} */

/* {{{ mysqlnd_stmt::dtor */
static enum_func_status
MYSQLND_METHOD(mysqlnd_stmt, dtor)(MYSQLND_STMT * const s, zend_bool implicit)
{
	MYSQLND_STMT_DATA * stmt = (s != NULL) ? s->data:NULL;
	enum_func_status ret = FAIL;
	zend_bool persistent = (s != NULL) ? s->persistent : 0;

	DBG_ENTER("mysqlnd_stmt::dtor");
	if (stmt) {
		DBG_INF_FMT("stmt=%p", stmt);

		MYSQLND_INC_GLOBAL_STATISTIC(implicit == TRUE?	STAT_STMT_CLOSE_IMPLICIT:
														STAT_STMT_CLOSE_EXPLICIT);

		ret = s->m->close_on_server(s, implicit);
		mnd_pefree(stmt, persistent);
	}
	mnd_pefree(s, persistent);

	DBG_INF(ret == PASS? "PASS":"FAIL");
	DBG_RETURN(ret);
}
/* }}} */


/* {{{ mysqlnd_stmt::alloc_param_bind */
static MYSQLND_PARAM_BIND *
MYSQLND_METHOD(mysqlnd_stmt, alloc_param_bind)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt::alloc_param_bind");
	if (!stmt) {
		DBG_RETURN(NULL);
	}
	DBG_RETURN(mnd_pecalloc(stmt->param_count, sizeof(MYSQLND_PARAM_BIND), stmt->persistent));
}
/* }}} */


/* {{{ mysqlnd_stmt::alloc_result_bind */
static MYSQLND_RESULT_BIND *
MYSQLND_METHOD(mysqlnd_stmt, alloc_result_bind)(MYSQLND_STMT * const s)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	DBG_ENTER("mysqlnd_stmt::alloc_result_bind");
	if (!stmt) {
		DBG_RETURN(NULL);
	}
	DBG_RETURN(mnd_pecalloc(stmt->field_count, sizeof(MYSQLND_RESULT_BIND), stmt->persistent));
}
/* }}} */


/* {{{ param_bind::free_parameter_bind */
PHPAPI void
MYSQLND_METHOD(mysqlnd_stmt, free_parameter_bind)(MYSQLND_STMT * const s, MYSQLND_PARAM_BIND * param_bind)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	if (stmt) {
		mnd_pefree(param_bind, stmt->persistent);
	}
}
/* }}} */


/* {{{ mysqlnd_stmt::free_result_bind */
PHPAPI void
MYSQLND_METHOD(mysqlnd_stmt, free_result_bind)(MYSQLND_STMT * const s, MYSQLND_RESULT_BIND * result_bind)
{
	MYSQLND_STMT_DATA * stmt = s? s->data : NULL;
	if (stmt) {
		mnd_pefree(result_bind, stmt->persistent);
	}
}
/* }}} */



MYSQLND_CLASS_METHODS_START(mysqlnd_stmt)
	MYSQLND_METHOD(mysqlnd_stmt, prepare),
	MYSQLND_METHOD(mysqlnd_stmt, send_execute),
	MYSQLND_METHOD(mysqlnd_stmt, execute),
	MYSQLND_METHOD(mysqlnd_stmt, use_result),
	MYSQLND_METHOD(mysqlnd_stmt, store_result),
	MYSQLND_METHOD(mysqlnd_stmt, get_result),
	MYSQLND_METHOD(mysqlnd_stmt, more_results),
	MYSQLND_METHOD(mysqlnd_stmt, next_result),
	MYSQLND_METHOD(mysqlnd_stmt, free_result),
	MYSQLND_METHOD(mysqlnd_stmt, data_seek),
	MYSQLND_METHOD(mysqlnd_stmt, reset),
	MYSQLND_METHOD_PRIVATE(mysqlnd_stmt, close_on_server),
	MYSQLND_METHOD(mysqlnd_stmt, dtor),

	MYSQLND_METHOD(mysqlnd_stmt, fetch),

	MYSQLND_METHOD(mysqlnd_stmt, bind_parameters),
	MYSQLND_METHOD(mysqlnd_stmt, bind_one_parameter),
	MYSQLND_METHOD(mysqlnd_stmt, refresh_bind_param),
	MYSQLND_METHOD(mysqlnd_stmt, bind_result),
	MYSQLND_METHOD(mysqlnd_stmt, bind_one_result),
	MYSQLND_METHOD(mysqlnd_stmt, send_long_data),
	MYSQLND_METHOD(mysqlnd_stmt, param_metadata),
	MYSQLND_METHOD(mysqlnd_stmt, result_metadata),

	MYSQLND_METHOD(mysqlnd_stmt, insert_id),
	MYSQLND_METHOD(mysqlnd_stmt, affected_rows),
	MYSQLND_METHOD(mysqlnd_stmt, num_rows),

	MYSQLND_METHOD(mysqlnd_stmt, param_count),
	MYSQLND_METHOD(mysqlnd_stmt, field_count),
	MYSQLND_METHOD(mysqlnd_stmt, warning_count),

	MYSQLND_METHOD(mysqlnd_stmt, errno),
	MYSQLND_METHOD(mysqlnd_stmt, error),
	MYSQLND_METHOD(mysqlnd_stmt, sqlstate),

	MYSQLND_METHOD(mysqlnd_stmt, attr_get),
	MYSQLND_METHOD(mysqlnd_stmt, attr_set),


	MYSQLND_METHOD(mysqlnd_stmt, alloc_param_bind),
	MYSQLND_METHOD(mysqlnd_stmt, alloc_result_bind),
	MYSQLND_METHOD(mysqlnd_stmt, free_parameter_bind),
	MYSQLND_METHOD(mysqlnd_stmt, free_result_bind),
	MYSQLND_METHOD(mysqlnd_stmt, server_status),
	mysqlnd_stmt_execute_generate_request,
	mysqlnd_stmt_execute_parse_response,
	MYSQLND_METHOD(mysqlnd_stmt, free_stmt_content),
	MYSQLND_METHOD(mysqlnd_stmt, flush),
	MYSQLND_METHOD(mysqlnd_stmt, free_stmt_result)
MYSQLND_CLASS_METHODS_END;


/* {{{ _mysqlnd_init_ps_subsystem */
void _mysqlnd_init_ps_subsystem()
{
	mysqlnd_stmt_set_methods(&MYSQLND_CLASS_METHOD_TABLE_NAME(mysqlnd_stmt));
	_mysqlnd_init_ps_fetch_subsystem();
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
