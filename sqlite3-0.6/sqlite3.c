/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2007 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Scott MacVicar <scottmac@php.net>                           |
   +----------------------------------------------------------------------+

   $Id: sqlite3.c,v 1.1 2007/08/14 03:13:22 scottmac Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_sqlite3.h"

#include <sqlite3.h>

#include "zend_exceptions.h"
#include "zend_interfaces.h"

ZEND_DECLARE_MODULE_GLOBALS(sqlite3)
static PHP_GINIT_FUNCTION(sqlite3);
static int php_sqlite3_authorizer(void *autharg, int access_type, const char *arg3, const char *arg4,
		const char *arg5, const char *arg6);
static void sqlite3_param_dtor(void *data);
static int php_sqlite3_compare_stmt_free( php_sqlite3_stmt_free_list **stmt_list, sqlite3_stmt *statement );

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("sqlite3.extension_dir",  NULL, PHP_INI_SYSTEM, OnUpdateString, extension_dir, zend_sqlite3_globals, sqlite3_globals)
PHP_INI_END()
/* }}} */

/* Handlers */
static zend_object_handlers sqlite3_object_handlers;
static zend_object_handlers sqlite3_stmt_object_handlers;
static zend_object_handlers sqlite3_result_object_handlers;

/* Class entries */
zend_class_entry *php_sqlite3_sc_entry;
zend_class_entry *php_sqlite3_stmt_entry;
zend_class_entry *php_sqlite3_result_entry;

/* {{{ proto bool SQLite3::open(String filename [, int Flags [, string Encryption Key]])
	Opens a SQLite 3 Database, if the build includes encryption then it will attempt to use the key
*/
PHP_METHOD(sqlite3, open)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	char *filename, *encryption_key, *fullpath;
	int filename_len, encryption_key_len, flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ls",
				&filename, &filename_len, &flags, &encryption_key, &encryption_key_len)) {
		return;
	}

	if (strncmp(filename, ":memory:", 8) != 0) {
		if (!(fullpath = expand_filepath(filename, NULL TSRMLS_CC))) {
			RETURN_FALSE;
		}

#if PHP_MAJOR_VERSION < 6
		if (PG(safe_mode) && (!php_checkuid(fullpath, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
			efree(fullpath);
			RETURN_FALSE;
		}
#endif

		if (php_check_open_basedir(fullpath TSRMLS_CC)) {
			efree(fullpath);
			RETURN_FALSE;
		}
	} else {
		fullpath = estrdup(filename);
	}

	if (db->db) {
		sqlite3_close(db->db);
		db->db = NULL;
	}

#if SQLITE_VERSION_NUMBER >= 3005000 && PHP_MAJOR_VERSION >= 5 && PHP_MINOR_VERSION >= 3
	if (sqlite3_open_v2(fullpath, &(db->db), flags, NULL) != SQLITE_OK) {
#else
	if (sqlite3_open(fullpath, &(db->db)) != SQLITE_OK) {
#endif
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unable to open database: %s", sqlite3_errmsg(db->db));
		if (fullpath) {
			efree(fullpath);
		}
		return;
	}

#if SQLITE_HAS_CODEC
	if (encryption_key_len > 0) {
		if (sqlite3_key(db->db, encryption_key, encryption_key_len) != SQLITE_OK) {
			return;
		}
	}
#endif

	if (
#if PHP_MAJOR_VERSION < 6
		PG(safe_mode) ||
#endif
			(PG(open_basedir) && *PG(open_basedir))) {
		sqlite3_set_authorizer(db->db, php_sqlite3_authorizer, NULL);
	}

	if (fullpath) {
		efree(fullpath);
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool SQLite3::close()
	Close a SQLite 3 Database.
*/
PHP_METHOD(sqlite3, close)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	int errcode;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	zend_llist_clean(&(db->stmt_list));
	errcode = sqlite3_close(db->db);
	if (errcode != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to close database: %d, %s", errcode, sqlite3_errmsg(db->db));
		RETURN_FALSE;
	}

	db->db = NULL;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool SQLite3::exec(String Query)
	Executes a result-less query against a given database
*/
PHP_METHOD(sqlite3, exec)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	char *sql, *errtext = NULL;
	int sql_len;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&sql, &sql_len)) {
		return;
	}

	if (sqlite3_exec(db->db, sql, NULL, NULL, &errtext) != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", errtext);
		sqlite3_free(errtext);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto Array SQLite3::version()
	Returns the SQLite3 Library version as a string constant and as a number.
*/
PHP_METHOD(sqlite3, version)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	array_init(return_value);

	add_assoc_string(return_value, "versionString", (char*)sqlite3_libversion(), 1);
	add_assoc_long(return_value, "versionNumber", sqlite3_libversion_number());

	return;
}
/* }}} */

/* {{{ proto int SQLite3::lastInsertRowID()
	Returns the rowid of the most recent INSERT into the database from the database connection.
*/
PHP_METHOD(sqlite3, lastInsertRowID)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	RETURN_LONG(sqlite3_last_insert_rowid(db->db));
}
/* }}} */

/* {{{ proto int SQLite3::lastErrorCode()
	Returns the numeric result code of the most recent failed sqlite API call for the database connection.
*/
PHP_METHOD(sqlite3, lastErrorCode)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	RETURN_LONG(sqlite3_errcode(db->db));
}
/* }}} */

/* {{{ proto string SQLite3::lastErrorMsg()
	Returns english text describing the most recent failed sqlite API call for the database connection.
*/
PHP_METHOD(sqlite3, lastErrorMsg)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	RETVAL_STRING((char *)sqlite3_errmsg(db->db), 1);
}
/* }}} */

/* {{{ proto bool SQLite3::loadExtension(String Shared Library)
	Attempts to load an SQLite extension library
*/
PHP_METHOD(sqlite3, loadExtension)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	char *extension, *lib_path, *extension_dir, *errtext = NULL;
	char fullpath[MAXPATHLEN];
	int extension_len, extension_dir_len;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&extension, &extension_len)) {
		return;
	}

	if (!SQLITE3G(extension_dir)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "SQLite Extension are disabled");
		RETURN_FALSE;
	}

	if (extension_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty string as an extension");
		RETURN_FALSE;
	}

	extension_dir = SQLITE3G(extension_dir);
	extension_dir_len = strlen(SQLITE3G(extension_dir));

	if (IS_SLASH(extension_dir[extension_dir_len-1])) {
		spprintf(&lib_path, 0, "%s%s", extension_dir, extension);
	} else {
		spprintf(&lib_path, 0, "%s%c%s", extension_dir, DEFAULT_SLASH, extension);
	}

	if (!VCWD_REALPATH(lib_path, fullpath)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to load extension at '%s'", lib_path);
		RETURN_FALSE;
	}

	if (strncmp(fullpath, extension_dir, extension_dir_len) != 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to open extensions outside the defined directory");
		RETURN_FALSE;
	}

	/* Extension loading should only be enabled for when we attempt to load */
	sqlite3_enable_load_extension(db->db, 1);
	if (sqlite3_load_extension(db->db, fullpath, 0, &errtext) != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", errtext);
		sqlite3_free(errtext);
		sqlite3_enable_load_extension(db->db, 0);
		RETURN_FALSE;
	}
	sqlite3_enable_load_extension(db->db, 0);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int SQLite3::changes()
	Returns the number of database rows that were changed (or inserted or deleted) by the most recent SQL statement.
*/
PHP_METHOD(sqlite3, changes)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	RETURN_LONG(sqlite3_changes(db->db));
}
/* }}} */

/* {{{ proto String SQLite3::escapeString(String value)
	Returns a string that has been properly escaped
*/
PHP_METHOD(sqlite3, escapeString)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	char *sql, *ret;
	int sql_len;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&sql, &sql_len)) {
		return;
	}

	if (sql_len) {
		ret = sqlite3_mprintf("%q", sql);
		if (ret) {
			RETVAL_STRING(ret, 1);
			sqlite3_free(ret);
		}
	} else {
		RETURN_EMPTY_STRING();
	}
}
/* }}} */

/* {{{ proto sqlite3_stmt SQLite3::prepare(String Query)
	Returns a prepared SQL statement for execution
*/
PHP_METHOD(sqlite3, prepare)
{
	php_sqlite3_db *db;
	php_sqlite3_stmt *internp;
	zval *object = getThis();
	char *sql;
	int sql_len, errcode;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&sql, &sql_len)) {
		return;
	}

	if (!sql_len) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, php_sqlite3_stmt_entry);
	internp = (php_sqlite3_stmt *)zend_object_store_get_object(return_value TSRMLS_CC);
	internp->db_object = db;

	errcode = sqlite3_prepare_v2(db->db, sql, sql_len, &(internp->stmt), NULL);
	if (errcode != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to prepare statement: %d, %s", errcode, sqlite3_errmsg(db->db));
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto sqlite3_result SQLite3::query(String Query)
	Returns true or false, for queries that return data it will return a sqlite3_result object
*/
PHP_METHOD(sqlite3, query)
{
	php_sqlite3_db *db;
	php_sqlite3_result *result;
	zval *object = getThis();
	char *sql, *errtext = NULL;
	int sql_len, return_code;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
				&sql, &sql_len)) {
		return;
	}

	if (!sql_len) {
		RETURN_FALSE;
	}

	/* If there was no return value then just execute the query */
	if (!return_value_used) {
		if (sqlite3_exec(db->db, sql, NULL, NULL, &errtext) != SQLITE_OK) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", errtext);
			sqlite3_free(errtext);
		}
		return;
	}

	object_init_ex(return_value, php_sqlite3_result_entry);
	result = (php_sqlite3_result *)zend_object_store_get_object(return_value TSRMLS_CC);
	result->db_object = db;

	return_code = sqlite3_prepare_v2(db->db, sql, sql_len, result->intern_stmt, NULL);
	if (return_code != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to prepare statement: %d, %s", return_code, sqlite3_errmsg(db->db));
		efree(result->intern_stmt);
		result->intern_stmt = NULL;
		RETURN_FALSE;
	}


	return_code = sqlite3_step(*(result->intern_stmt));
	result->num_rows = 0;

	switch (return_code) {
		case SQLITE_ROW: /* Valid Row */
		{
#ifdef scottmac_0
			/* loop through to fill numRows */
			do
			{
				result->num_rows++;
			} while (sqlite3_step(*(result->intern_stmt)) == SQLITE_ROW);
#endif
		}
		/* No break is intentional */
		case SQLITE_DONE: /* Valid but no results */
		{
			php_sqlite3_stmt_free_list *free_item;
			free_item = emalloc(sizeof(php_sqlite3_stmt_free_list));
			free_item->stmt = *(result->intern_stmt);
			free_item->statement_object = NULL;
			free_item->result_object = return_value;
			Z_ADDREF_P(return_value);
			zend_llist_add_element(&(db->stmt_list), &free_item);
			sqlite3_reset(*(result->intern_stmt));
			break;
		}
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute statement: %s", sqlite3_errmsg(db->db));
			sqlite3_finalize(*(result->intern_stmt));
			RETURN_FALSE;
	}
}
/* }}} */

static zval* sqlite_value_to_zval(sqlite3_stmt *stmt, int column)
{
	zval *data;
	MAKE_STD_ZVAL(data);
	switch (sqlite3_column_type(stmt, column)) {
		case SQLITE_INTEGER:
			if ((sqlite3_column_int64(stmt, column)) >= INT_MAX || sqlite3_column_int64(stmt, column) <= INT_MIN) {
				ZVAL_STRINGL(data, (char *)sqlite3_column_text(stmt, column), sqlite3_column_bytes(stmt, column), 1);
			} else {
				ZVAL_LONG(data, sqlite3_column_int64(stmt, column));
			}
			break;

		case SQLITE_FLOAT:
			ZVAL_DOUBLE(data, sqlite3_column_double(stmt, column));
			break;

		case SQLITE_NULL:
			ZVAL_NULL(data);
			break;

		case SQLITE3_TEXT:
			ZVAL_STRING(data, (char*)sqlite3_column_text(stmt, column), 1);
			break;

		case SQLITE_BLOB:
		default:
			ZVAL_STRINGL(data, (char*)sqlite3_column_blob(stmt, column), sqlite3_column_bytes(stmt, column), 1);
	}
	return data;
}

/* {{{ proto sqlite3_result SQLite3::querySingle(String Query [, entire_row = false])
	Returns a string of the first column, or an array of the entire row
*/
PHP_METHOD(sqlite3, querySingle)
{
	php_sqlite3_db *db;
	php_sqlite3_result *result;
	zval *object = getThis();
	char *sql, *errtext = NULL;
	int sql_len, return_code, entire_row = 0;
	sqlite3_stmt *stmt;
	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",
				&sql, &sql_len, &entire_row)) {
		return;
	}

	if (!sql_len) {
		RETURN_FALSE;
	}

	/* If there was no return value then just execute the query */
	if (!return_value_used) {
		if (sqlite3_exec(db->db, sql, NULL, NULL, &errtext) != SQLITE_OK) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", errtext);
			sqlite3_free(errtext);
		}
		return;
	}

	return_code = sqlite3_prepare_v2(db->db, sql, sql_len, &stmt, NULL);
	if (return_code != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to prepare statement: %d, %s", return_code, sqlite3_errmsg(db->db));
		RETURN_FALSE;
	}


	return_code = sqlite3_step(stmt);

	switch (return_code) {
		case SQLITE_ROW: /* Valid Row */
		{
			if (!entire_row) {
				zval *data;
				data = sqlite_value_to_zval(stmt, 0);
				*return_value = *data;
				zval_copy_ctor(return_value);
				zval_dtor(data);
				FREE_ZVAL(data);
			} else {
				int i = 0;
				array_init(return_value);
				for (i = 0; i < sqlite3_data_count(stmt); i++) {
					zval *data;
					data = sqlite_value_to_zval(stmt, i);
					add_assoc_zval(return_value, (char*)sqlite3_column_name(stmt, i), data);
				}
			}
			break;
		}
		case SQLITE_DONE: /* Valid but no results */
		{
			if (!entire_row) {
				ZVAL_EMPTY_STRING(return_value);
			} else {
				array_init(return_value);
			}
		}
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute statement: %s", sqlite3_errmsg(db->db));
			RETURN_FALSE;
	}
	sqlite3_finalize(stmt);	
}
/* }}} */

static int sqlite3_do_callback(struct php_sqlite3_fci *fc, zval *cb,
		int argc, sqlite3_value **argv, sqlite3_context *context,
		int is_agg TSRMLS_DC)
{
	zval ***zargs = NULL;
	zval *retval = NULL;
	int i;
	int ret;
	int fake_argc;
	zval **agg_context = NULL;

	if (is_agg) {
		is_agg = 2;
	}

	fake_argc = argc + is_agg;

	fc->fci.size = sizeof(fc->fci);
	fc->fci.function_table = EG(function_table);
	fc->fci.function_name = cb;
	fc->fci.symbol_table = NULL;
	fc->fci.object_pp = NULL;
	fc->fci.retval_ptr_ptr = &retval;
	fc->fci.param_count = fake_argc;

	/* build up the params */

	if (fake_argc) {
		zargs = (zval ***)safe_emalloc(fake_argc, sizeof(zval **), 0);
	}

	if (is_agg) {
		/* summon the aggregation context */
		agg_context = (zval**)sqlite3_aggregate_context(context, sizeof(zval*));
		if (!*agg_context) {
			MAKE_STD_ZVAL(*agg_context);
			ZVAL_NULL(*agg_context);
		}
		zargs[0] = agg_context;

		zargs[1] = emalloc(sizeof(zval*));
		MAKE_STD_ZVAL(*zargs[1]);
		ZVAL_LONG(*zargs[1], sqlite3_aggregate_count(context));
	}

	for (i = 0; i < argc; i++) {
		zargs[i + is_agg] = emalloc(sizeof(zval *));
		MAKE_STD_ZVAL(*zargs[i + is_agg]);

		switch (sqlite3_value_type(argv[i])) {
			case SQLITE_INTEGER:
				ZVAL_LONG(*zargs[i + is_agg], sqlite3_value_int(argv[i]));
				break;

			case SQLITE_FLOAT:
				ZVAL_DOUBLE(*zargs[i + is_agg], sqlite3_value_double(argv[i]));
				break;

			case SQLITE_NULL:
				ZVAL_NULL(*zargs[i + is_agg]);
				break;

			case SQLITE_BLOB:
			case SQLITE3_TEXT:
			default:
				ZVAL_STRINGL(*zargs[i + is_agg], (char*)sqlite3_value_text(argv[i]),
						sqlite3_value_bytes(argv[i]), 1);
				break;
		}
	}

	fc->fci.params = zargs;

	if ((ret = zend_call_function(&fc->fci, &fc->fcc TSRMLS_CC)) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "An error occurred while invoking the callback");
	}

	/* clean up the params */
	if (argc) {
		for (i = is_agg; i < argc; i++) {
			zval_ptr_dtor(zargs[i]);
			efree(zargs[i]);
		}
		if (is_agg) {
			zval_ptr_dtor(zargs[1]);
			efree(zargs[1]);
		}
		efree(zargs);
	}

	if (!is_agg || !argv) {
		/* only set the sqlite return value if we are a scalar function,
		 * or if we are finalizing an aggregate */
		if (retval) {
			switch (Z_TYPE_P(retval)) {
				case IS_LONG:
					sqlite3_result_int(context, Z_LVAL_P(retval));
					break;

				case IS_NULL:
					sqlite3_result_null(context);
					break;

				case IS_DOUBLE:
					sqlite3_result_double(context, Z_DVAL_P(retval));
					break;

				default:
					convert_to_string_ex(&retval);
					sqlite3_result_text(context, Z_STRVAL_P(retval),
						Z_STRLEN_P(retval), SQLITE_TRANSIENT);
					break;
			}
		} else {
			sqlite3_result_error(context, "failed to invoke callback", 0);
		}

		if (agg_context) {
			zval_ptr_dtor(agg_context);
		}
	} else {
		/* we're stepping in an aggregate; the return value goes into
		 * the context */
		if (agg_context) {
			zval_ptr_dtor(agg_context);
		}
		if (retval) {
			*agg_context = retval;
			retval = NULL;
		} else {
			*agg_context = NULL;
		}
	}

	if (retval) {
		zval_ptr_dtor(&retval);
	}

	return ret;
}

static void php_sqlite3_callback_func(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	php_sqlite3_func *func = (php_sqlite3_func *)sqlite3_user_data(context);
	TSRMLS_FETCH();

	sqlite3_do_callback(&func->afunc, func->func, argc, argv, context, 0 TSRMLS_CC);
}

static void php_sqlite3_callback_step(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	php_sqlite3_func *func = (php_sqlite3_func *)sqlite3_user_data(context);
	TSRMLS_FETCH();

	sqlite3_do_callback(&func->astep, func->step, argc, argv, context, 1 TSRMLS_CC);
}

static void php_sqlite3_callback_final(sqlite3_context *context)
{
	php_sqlite3_func *func = (php_sqlite3_func *)sqlite3_user_data(context);
	TSRMLS_FETCH();

	sqlite3_do_callback(&func->afini, func->fini, 0, NULL, context, 1 TSRMLS_CC);
}

/* {{{ proto bool SQLite3::createFunction(string name, mixed callback [, int argcount]))
	Allows registration of a PHP function as a SQLite UDF that can be called within SQL statements
*/
PHP_METHOD(sqlite3, createFunction)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	php_sqlite3_func *func;
	char *sql_func, *callback_name;
	int sql_func_len;
	zval *callback_func;
	long sql_func_num_args = -1;

	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &sql_func, &sql_func_len, &callback_func, &sql_func_num_args) == FAILURE) {
		return;
	}

	if (!sql_func_len) {
		RETURN_FALSE;
	}

	if (!zend_is_callable(callback_func, 0, &callback_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a valid callback function %s", callback_name);
		efree(callback_name);
		RETURN_FALSE;
	}
	efree(callback_name);

	func = (php_sqlite3_func *)ecalloc(1, sizeof(*func));

	if (sqlite3_create_function(db->db, sql_func, sql_func_num_args, SQLITE_UTF8, func, php_sqlite3_callback_func, NULL, NULL) == SQLITE_OK) {
		func->func_name = estrdup(sql_func);

		MAKE_STD_ZVAL(func->func);
		*(func->func) = *callback_func;
		zval_copy_ctor(func->func);

		func->argc = sql_func_num_args;
		func->next = db->funcs;
		db->funcs = func;

		RETURN_TRUE;
	}
	efree(func);

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool SQLite3::createAggregate(string name, mixed step, mixed final [, int argcount]))
	Allows registration of a PHP function for use as an aggregate
*/
PHP_METHOD(sqlite3, createAggregate)
{
	php_sqlite3_db *db;
	zval *object = getThis();
	php_sqlite3_func *func;
	char *sql_func, *callback_name;
	int sql_func_len;
	zval *step_callback, *fini_callback;
	long sql_func_num_args = -1;

	db = (php_sqlite3_db *)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "szz|l", &sql_func, &sql_func_len, &step_callback, &fini_callback, &sql_func_num_args) == FAILURE) {
		return;
	}

	if (!sql_func_len) {
		RETURN_FALSE;
	}

	if (!zend_is_callable(step_callback, 0, &callback_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a valid callback function %s", callback_name);
		efree(callback_name);
		RETURN_FALSE;
	}
	efree(callback_name);

	if (!zend_is_callable(fini_callback, 0, &callback_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a valid callback function %s", callback_name);
		efree(callback_name);
		RETURN_FALSE;
	}
	efree(callback_name);

	func = (php_sqlite3_func *)ecalloc(1, sizeof(*func));

	if (sqlite3_create_function(db->db, sql_func, sql_func_num_args, SQLITE_UTF8, func, NULL, php_sqlite3_callback_step, php_sqlite3_callback_final) == SQLITE_OK) {
		func->func_name = estrdup(sql_func);

		MAKE_STD_ZVAL(func->step);
		*(func->step) = *step_callback;
		zval_copy_ctor(func->step);

		MAKE_STD_ZVAL(func->fini);
		*(func->fini) = *fini_callback;
		zval_copy_ctor(func->fini);

		func->argc = sql_func_num_args;
		func->next = db->funcs;
		db->funcs = func;

		RETURN_TRUE;
	}
	efree(func);

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto int SQLite3_stmt::paramCount()
	Returns the number of parameters within the prepared statement
*/
PHP_METHOD(sqlite3_stmt, paramCount)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();

	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	RETURN_LONG(sqlite3_bind_parameter_count(internp->stmt));
}
/* }}} */

/* {{{ proto bool SQLite3_stmt::close()
	Closes the prepared statement
*/
PHP_METHOD(sqlite3_stmt, close)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();

	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	zend_llist_del_element(&(internp->db_object->stmt_list), internp->stmt, 
							(int (*)(void *, void *)) php_sqlite3_compare_stmt_free);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool SQLite3_stmt::reset()
	Reset the prepared statement to the state before it was executed, bindings still remain.
*/
PHP_METHOD(sqlite3_stmt, reset)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();

	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (sqlite3_reset(internp->stmt) != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute statement: %s", sqlite3_errmsg(sqlite3_db_handle(internp->stmt)));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool SQLite3_stmt::clear()
	Clear all current bound parameters
*/
PHP_METHOD(sqlite3_stmt, clear)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();

	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (sqlite3_clear_bindings(internp->stmt) != SQLITE_OK) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute statement: %s", sqlite3_errmsg(sqlite3_db_handle(internp->stmt)));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

#if scottmac_0
/* {{{ proto bool SQLite3_stmt::bind_params(string types, mixed variable [,mixed,....])
	Bind Paramater to a stmt variable
*/
PHP_METHOD(sqlite3_stmt, bind_params)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();
	int argc = ZEND_NUM_ARGS();
	char *types;
	int types_len, num_varargs, i, ofs = 0;
	zval ***varargs = NULL;

	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	if (argc < 2) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(1 TSRMLS_CC, "s",
				&types, &types_len) == FAILURE) {
                return;
        }

	num_varargs = argc - 1;

	if (!types_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No parameter types specified");
		RETURN_FALSE;
	}

	if (types_len != num_varargs) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Number of parameter types specified does not match number of bind variables");
		RETURN_FALSE;
	}

	if (types_len != sqlite3_bind_parameter_count(internp->stmt)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Number of bind variables does not match number of parameters in prepared statement");
		RETURN_FALSE;
	}

	varargs = (zval ***)safe_emalloc(argc, sizeof(zval **), 0);

	if (zend_get_parameters_array_ex(argc, varargs) == FAILURE) {
		zend_wrong_param_count(TSRMLS_C);
		efree(varargs);
		RETURN_FALSE;
	}

	for (i = 1; i <= sqlite3_bind_parameter_count(internp->stmt); i++) {
		switch (types[ofs]) {
			case 'i': /* Integer */
				sqlite3_bind_int(internp->stmt, i, Z_LVAL_PP(varargs[i]));
			break;

			case 'd': /* double */
				sqlite3_bind_double(internp->stmt, i, Z_DVAL_PP(varargs[i]));
			break;

			case 'b': /* blob */
			case 's': /* string */
				sqlite3_bind_text(internp->stmt, i, Z_STRVAL_PP(varargs[i]), Z_STRLEN_PP(varargs[i]), SQLITE_STATIC);
			break;

			case 'n': /* null */
				sqlite3_bind_null(internp->stmt, i);
			break;

		}
		ofs++;
	}

	efree(varargs);
	RETURN_TRUE;
}
/* }}} */
#endif

static int register_bound_parameter_to_sqlite(struct php_sqlite3_bound_param *param, php_sqlite3_stmt *stmt TSRMLS_DC) /* {{{ */
{
	HashTable *hash;
	hash = stmt->bound_params;

	if (!hash) {
		ALLOC_HASHTABLE(hash);
		zend_hash_init(hash, 13, NULL, sqlite3_param_dtor, 0);
		stmt->bound_params = hash;
	}

	/* We need a : prefix to resolve a name to a parameter number */
	if (param->name) {
		if (param->name[0] != ':') {
			/* pre-increment for character + 1 for null */
			char *temp = emalloc(++param->name_len + 1);
			temp[0] = ':';
			memmove(temp+1, param->name, param->name_len);
			param->name = temp;
		} else {
			param->name = estrndup(param->name, param->name_len);
		}
		// do lookup
		param->param_number = sqlite3_bind_parameter_index(stmt->stmt, param->name);
	}

	if (param->param_number < 1) {
		efree(param->name);
		return 0;
	}

	if (param->param_number >= 1) {
		zend_hash_index_del(hash, param->param_number);
	}

	if (param->name) {
		zend_hash_update(hash, param->name, param->name_len, param, sizeof(*param), NULL);
	} else {
		zend_hash_index_update(hash, param->param_number, param, sizeof(*param), NULL);
	}

	return 1;
}
/* }}} */

/* {{{ proto bool SQLite3_stmt::bindParam(int parameter_number, mixed parameter [, int type])
	Bind Paramater to a stmt variable
*/
PHP_METHOD(sqlite3_stmt, bindParam)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();
	struct php_sqlite3_bound_param param = {0};
	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	param.param_number = -1;
	param.type = SQLITE3_TEXT;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "lz|l",
				&param.param_number, &param.parameter, &param.type) == FAILURE) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l",
					 &param.name, &param.name_len, &param.parameter, &param.type) == FAILURE) {
			return;
		}
	}

	Z_ADDREF_P(param.parameter);

	if (!register_bound_parameter_to_sqlite(&param, internp TSRMLS_CC)) {
		if (param.parameter) {
			zval_ptr_dtor(&(param.parameter));
			param.parameter = NULL;
		}
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool SQLite3_stmt::bindValue(inte parameter_number, mixed parameter [, int type])
	Bind Value of a parameter to a stmt variable
*/
PHP_METHOD(sqlite3_stmt, bindValue)
{
	php_sqlite3_stmt *internp;
	zval *object = getThis();
	struct php_sqlite3_bound_param param = {0};
	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	param.param_number = -1;
	param.type = SQLITE3_TEXT;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "lz/|l",
				&param.param_number, &param.parameter, &param.type) == FAILURE) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz/|l",
				&param.name, &param.name_len, &param.parameter, &param.type) == FAILURE) {
			return;
		}
	}

	Z_ADDREF_P(param.parameter);

	if (!register_bound_parameter_to_sqlite(&param, internp TSRMLS_CC)) {
		if (param.parameter) {
			zval_ptr_dtor(&(param.parameter));
			param.parameter = NULL;
		}
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto SQLite3_result SQLite3_stmt::execute()
	Executes a prepared statement and returns a result set object
*/
PHP_METHOD(sqlite3_stmt, execute)
{
	php_sqlite3_stmt *internp;
	php_sqlite3_result *result;
	zval *object = getThis();
	int return_code = 0, num_rows = 0;
	struct php_sqlite3_bound_param *param;
	internp = (php_sqlite3_stmt *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (internp->bound_params) {
		zend_hash_internal_pointer_reset(internp->bound_params);
		while (zend_hash_get_current_data(internp->bound_params, (void **)&param) == SUCCESS) {
			/* If the ZVAL is null then it should be bound as that */
			if (Z_TYPE_P(param->parameter) == IS_NULL) {
				sqlite3_bind_null(internp->stmt, param->param_number);
				continue;
			}

			switch (param->type) {
				case SQLITE_INTEGER:
					convert_to_long(param->parameter);
					sqlite3_bind_int(internp->stmt, param->param_number, Z_LVAL_P(param->parameter));
					break;

				case SQLITE_FLOAT:
					/* convert_to_double(param->parameter);*/
					sqlite3_bind_double(internp->stmt, param->param_number, Z_DVAL_P(param->parameter));
					break;

				case SQLITE_BLOB:
				{
					php_stream *stream = NULL;
					int blength;
					char *buffer = NULL;
					if (Z_TYPE_P(param->parameter) == IS_RESOURCE) {
						php_stream_from_zval_no_verify(stream, &param->parameter);
						if (stream == NULL) {
							php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to read stream for parameter %ld", param->param_number);
							RETURN_FALSE;
						}
						blength = php_stream_copy_to_mem(stream, &buffer, PHP_STREAM_COPY_ALL, 0);
					} else {
						convert_to_string(param->parameter);
						blength =  Z_STRLEN_P(param->parameter);
						buffer = Z_STRVAL_P(param->parameter);
					}

					sqlite3_bind_blob(internp->stmt, param->param_number, buffer, blength, SQLITE_TRANSIENT);

					if (stream) {
						pefree(buffer, 0);
					}
					break;
				}

				case SQLITE3_TEXT:
					convert_to_string(param->parameter);
					sqlite3_bind_text(internp->stmt, param->param_number, Z_STRVAL_P(param->parameter), Z_STRLEN_P(param->parameter), SQLITE_STATIC);
					break;

				case SQLITE_NULL:
					sqlite3_bind_null(internp->stmt, param->param_number);
					break;

				default:
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown parameter type: %d for parameter %ld", param->type, param->param_number);
					RETURN_FALSE;
			}
			zend_hash_move_forward(internp->bound_params);
		}
	}

	return_code = sqlite3_step(internp->stmt);

	switch (return_code) {
		case SQLITE_ROW: /* Valid Row */
#ifdef scottmac_0
			/* loop through to fill numRows */
			do
			{
				num_rows++;
			} while (sqlite3_step(internp->stmt) == SQLITE_ROW);
#endif
		case SQLITE_DONE: /* Valid but no results */
		{
			php_sqlite3_stmt_free_list *free_item;
			
			sqlite3_reset(internp->stmt);
			object_init_ex(return_value, php_sqlite3_result_entry);
			result = (php_sqlite3_result *)zend_object_store_get_object(return_value TSRMLS_CC);

			free_item = emalloc(sizeof(php_sqlite3_stmt_free_list));
			free_item->stmt = internp->stmt;
			free_item->statement_object = object;
			free_item->result_object = return_value;
			Z_ADDREF_P(object);
			Z_ADDREF_P(return_value);
			
			zend_llist_add_element(&(internp->db_object->stmt_list), &free_item);
			/* We don't need the default one that came with it now */

			efree(result->intern_stmt);
			result->is_prepared_statement = 1;
			result->intern_stmt = &internp->stmt;
			result->num_rows = num_rows;
			result->db_object = internp->db_object;
			break;
		}
		case SQLITE_ERROR:
			sqlite3_reset(internp->stmt);

		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute statement: %s", sqlite3_errmsg(sqlite3_db_handle(internp->stmt)));
			RETURN_FALSE;
	}

	return;
}
/* }}} */

/* {{{ proto int SQLite3_result::numColumns()
	Number of columns in the result set
*/
PHP_METHOD(sqlite3_result, numColumns)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	RETURN_LONG(sqlite3_column_count(*(internp->intern_stmt)));
}
/* }}} */

/* {{{ proto string SQLite3_result::columnName(int column)
	Returns the name of the nth column
*/
PHP_METHOD(sqlite3_result, columnName)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	int column = 0;
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &column) == FAILURE) {
		return;
	}

	RETVAL_STRING((char *)sqlite3_column_name(*(internp->intern_stmt), column), 1);
}
/* }}} */

/* {{{ proto string SQLite3_result::columnType(int column)
	Returns the type of the nth column
*/
PHP_METHOD(sqlite3_result, columnType)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	int column = 0;
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &column) == FAILURE) {
		return;
	}

	RETURN_LONG(sqlite3_column_type(*(internp->intern_stmt), column));
}
/* }}} */

/* {{{ proto array SQLite3_result::fetchArray([int mode])
	Fetch a result row as both an associative or numerically indexed array or both
*/
PHP_METHOD(sqlite3_result, fetchArray)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	int i, ret, mode = PHP_SQLITE3_BOTH;
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &mode) == FAILURE) {
		return;
	}

	if (internp->complete == 1) {
		RETURN_FALSE;
	}

	ret = sqlite3_step(*(internp->intern_stmt));
	switch (ret) {
		case SQLITE_ROW:
			/* If there was no return value then just skip fetching */
			if (!return_value_used) {
				return;
			}

			array_init(return_value);

			for (i = 0; i < sqlite3_data_count(*(internp->intern_stmt)); i++) {
				zval *data;

				data = sqlite_value_to_zval(*(internp->intern_stmt), i);
				
				if (mode & PHP_SQLITE3_NUM) {
					add_index_zval(return_value, i, data);
				}

				if (mode & PHP_SQLITE3_ASSOC) {
					if (mode & PHP_SQLITE3_NUM) {
						Z_ADDREF_P(data);
					}
					add_assoc_zval(return_value, (char*)sqlite3_column_name(*(internp->intern_stmt), i), data);
				}
			}
		break;
		case SQLITE_DONE:
			/* Can't call sqlite3_step again, so store the value */
			internp->complete = 1;
			RETURN_FALSE;

		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to execute statement: %s", sqlite3_errmsg(sqlite3_db_handle(*(internp->intern_stmt))));
	}
}
/* }}} */

/* {{{ proto bool SQLite3_result::reset()
	Resets the result set back to the first row
*/
PHP_METHOD(sqlite3_result, reset)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (sqlite3_reset(*(internp->intern_stmt)) != SQLITE_OK) {
		RETURN_FALSE;
	}

	internp->complete = 0;

	RETURN_TRUE;
}
/* }}} */

#ifdef scottmac_0
/* {{{ proto bool SQLite3_result::numRows()
	Returns the number of rows in a result set
*/
PHP_METHOD(sqlite3_result, numRows)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);
                        
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}
	RETURN_LONG(internp->num_rows);
}
/* }}} */
#endif

/* {{{ proto bool SQLite3_result::finalize()
	Closes the result set
*/
PHP_METHOD(sqlite3_result, finalize)
{
	php_sqlite3_result *internp;
	zval *object = getThis();
	internp = (php_sqlite3_result *)zend_object_store_get_object(object TSRMLS_CC);

	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	/* We need to finalize an internal statement */
	if (internp->is_prepared_statement == 0) {
		zend_llist_del_element(&(internp->db_object->stmt_list), *(internp->intern_stmt), 
							(int (*)(void *, void *)) php_sqlite3_compare_stmt_free);
	} else {
		sqlite3_reset(*(internp->intern_stmt));
	}

	/*zval_dtor(object);
	ZVAL_NULL(object);*/

	RETURN_TRUE;
}
/* }}} */

/* {{{ arginfo */
static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_open, 0)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, flags)
	ZEND_ARG_INFO(0, encryption_key)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_close, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_exec, 0)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_version, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_lastinsertrowid, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_lasterrorcode, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_lasterrormsg, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_loadextension, 0)
	ZEND_ARG_INFO(0, shared_library)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_changes, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_escapestring, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_prepare, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_query, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_querysingle, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, entire_row)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_createfunction, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, argument_count)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_createaggregate, 0, 0, 3)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, step_callback)
	ZEND_ARG_INFO(0, final_callback)
	ZEND_ARG_INFO(0, argument_count)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_stmt_paramcount, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_stmt_close, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_stmt_reset, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_stmt_clear, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_stmt_execute, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_stmt_bindparam, 0, 0, 2)
	ZEND_ARG_INFO(0, param_number)
	ZEND_ARG_INFO(1, param)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_stmt_bindvalue, 0, 0, 2)
	ZEND_ARG_INFO(0, param_number)
	ZEND_ARG_INFO(0, param)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_result_numcolumns, 0)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_result_columnname, 0, 0, 1)
	ZEND_ARG_INFO(0, column_number)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_result_columntype, 0, 0, 1)
	ZEND_ARG_INFO(0, column_number)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_sqlite3_result_fetcharray, 0, 0, 1)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_result_reset, 0)
ZEND_END_ARG_INFO()

#ifdef scottmac_0
static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_result_numrows, 0)
ZEND_END_ARG_INFO()
#endif

static
ZEND_BEGIN_ARG_INFO(arginfo_sqlite3_result_finalize, 0)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ php_sqlite3_class_methods */
static zend_function_entry php_sqlite3_class_methods[] = {
	PHP_ME(sqlite3,		open,				arginfo_sqlite3_open, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		close,				arginfo_sqlite3_close, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		exec,				arginfo_sqlite3_exec, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		version,			arginfo_sqlite3_version, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		lastInsertRowID,	arginfo_sqlite3_lastinsertrowid, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		lastErrorCode,		arginfo_sqlite3_lasterrorcode, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		lastErrorMsg,		arginfo_sqlite3_lasterrormsg, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		loadExtension,		arginfo_sqlite3_loadextension, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		changes,			arginfo_sqlite3_changes, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		escapeString,		arginfo_sqlite3_escapestring, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		prepare,			arginfo_sqlite3_prepare, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		query,				arginfo_sqlite3_query, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		querySingle,		arginfo_sqlite3_querysingle, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		createFunction,		arginfo_sqlite3_createfunction, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3,		createAggregate,	arginfo_sqlite3_createaggregate, ZEND_ACC_PUBLIC)
	/* Aliases */
	PHP_MALIAS(sqlite3,	__construct, open, arginfo_sqlite3_open, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ php_sqlite3_stmt_class_methods */
static zend_function_entry php_sqlite3_stmt_class_methods[] = {
	PHP_ME(sqlite3_stmt, paramCount,	arginfo_sqlite3_stmt_paramcount, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_stmt, close,			arginfo_sqlite3_stmt_close, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_stmt, reset,			arginfo_sqlite3_stmt_reset, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_stmt, clear,			arginfo_sqlite3_stmt_clear, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_stmt, execute,		arginfo_sqlite3_stmt_execute, ZEND_ACC_PUBLIC)
#if scottmac_0
	PHP_ME(sqlite3_stmt, bind_params,	NULL, ZEND_ACC_PUBLIC)
#endif
	PHP_ME(sqlite3_stmt, bindParam,	arginfo_sqlite3_stmt_bindparam, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_stmt, bindValue,	arginfo_sqlite3_stmt_bindvalue, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ php_sqlite3_result_class_methods */
static zend_function_entry php_sqlite3_result_class_methods[] = {
	PHP_ME(sqlite3_result, numColumns,		arginfo_sqlite3_result_numcolumns, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_result, columnName,		arginfo_sqlite3_result_columnname, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_result, columnType,		arginfo_sqlite3_result_columntype, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_result, fetchArray,		arginfo_sqlite3_result_fetcharray, ZEND_ACC_PUBLIC)
	PHP_ME(sqlite3_result, reset,			arginfo_sqlite3_result_reset, ZEND_ACC_PUBLIC)
#ifdef scottmac_0
	PHP_ME(sqlite3_result, numRows,			arginfo_sqlite3_result_numrows, ZEND_ACC_PUBLIC)
#endif
	PHP_ME(sqlite3_result, finalize,		arginfo_sqlite3_result_finalize, ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ Authorization Callback */
static int php_sqlite3_authorizer(void *autharg, int access_type, const char *arg3, const char *arg4,
		const char *arg5, const char *arg6)
{
	TSRMLS_FETCH();
	switch (access_type) {
		case SQLITE_ATTACH:
		{
			if (strncmp(arg3, ":memory:", sizeof(":memory:")-1)) {
#if PHP_MAJOR_VERSION < 6
				if (PG(safe_mode) && (!php_checkuid(arg3, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
					return SQLITE_DENY;
				}
#endif
				if (php_check_open_basedir(arg3 TSRMLS_CC)) {
					return SQLITE_DENY;
				}
			}
			return SQLITE_OK;
		}

		default:
			/* access allowed */
			return SQLITE_OK;
	}
}
/* }}} */

/* {{{ php_sqlite3_stmt_free
 */
static void php_sqlite3_stmt_free(void **item)
{
	php_sqlite3_stmt_free_list *free_item = (php_sqlite3_stmt_free_list *)*item;
	
	zval_dtor(free_item->result_object);
	Z_TYPE_P(free_item->result_object) = IS_NULL;
	if (free_item->statement_object) {
		zval_dtor(free_item->statement_object);
		Z_TYPE_P(free_item->statement_object) = IS_NULL;
	}
	efree(*item);
}
/* }}} */


static int php_sqlite3_compare_stmt_free( php_sqlite3_stmt_free_list **stmt_list, sqlite3_stmt *statement )  /* {{{ */
{
	return (statement == (*stmt_list)->stmt);
}

static void php_sqlite3_object_free_storage(void *object TSRMLS_DC)
{
	php_sqlite3_db *intern = (php_sqlite3_db *)object;
	php_sqlite3_func *func;

	if (!intern) {
		return;
	}

	while (intern->funcs) {
		func = intern->funcs;
		intern->funcs = func->next;
		if (intern->db) {
			sqlite3_create_function(intern->db, func->func_name, func->argc, SQLITE_UTF8, func, NULL, NULL, NULL);
		}

		efree((char*)func->func_name);

		if (func->func) {
			zval_ptr_dtor(&func->func);
		}
		if (func->step) {
			zval_ptr_dtor(&func->step);
		}
		if (func->fini) {
			zval_ptr_dtor(&func->fini);
		}
		efree(func);
	}

	zend_llist_clean(&(intern->stmt_list));

	if (intern->db) {
		sqlite3_close(intern->db);
		intern->db = NULL;
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static void php_sqlite3_stmt_object_free_storage(void *object TSRMLS_DC)
{
	php_sqlite3_stmt *intern = (php_sqlite3_stmt *)object;

	if (!intern) {
		return;
	}

	if (intern->bound_params) {
		zend_hash_destroy(intern->bound_params);
		FREE_HASHTABLE(intern->bound_params);
		intern->bound_params = NULL;
	}

	if (intern->stmt) {
		sqlite3_finalize(intern->stmt);
		intern->stmt = NULL;
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static void php_sqlite3_result_object_free_storage(void *object TSRMLS_DC)
{
	php_sqlite3_result *intern = (php_sqlite3_result *)object;

	if (!intern) {
		return;
	}

	if (intern->intern_stmt) {
		sqlite3_reset(*(intern->intern_stmt));
	}

	/* The result set occured from a straight execute statement */
	if (intern->intern_stmt && intern->is_prepared_statement == 0) {
		sqlite3_finalize(*(intern->intern_stmt));
		efree(intern->intern_stmt);
	}

	intern->intern_stmt = NULL;

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static zend_object_value php_sqlite3_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_sqlite3_db *intern;

	/* Allocate memory for it */
	intern = emalloc(sizeof(php_sqlite3_db));
	memset(&intern->zo, 0, sizeof(php_sqlite3_db));

	zend_llist_init(&(intern->stmt_list),   sizeof(php_sqlite3_stmt_free_list *), (llist_dtor_func_t)php_sqlite3_stmt_free, 0);	
	intern->db = NULL;
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_sqlite3_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &sqlite3_object_handlers;

	return retval;
}

static zend_object_value php_sqlite3_stmt_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_sqlite3_stmt *intern;

	/* Allocate memory for it */
	intern = emalloc(sizeof(php_sqlite3_stmt));
	memset(&intern->zo, 0, sizeof(php_sqlite3_stmt));

	intern->stmt = NULL;
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_sqlite3_stmt_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &sqlite3_stmt_object_handlers;

	return retval;
}

static zend_object_value php_sqlite3_result_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_sqlite3_result *intern;

	/* Allocate memory for it */
	intern = emalloc(sizeof(php_sqlite3_result));
	memset(&intern->zo, 0, sizeof(php_sqlite3_result));

	intern->intern_stmt = emalloc(sizeof(sqlite3_stmt *));
	intern->buffered = 0;
	intern->complete = 0;
	intern->is_prepared_statement = 0;
	intern->num_rows = 0;

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_sqlite3_result_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &sqlite3_result_object_handlers;

	return retval;
}

static void sqlite3_param_dtor(void *data)
{
	struct php_sqlite3_bound_param *param = (struct php_sqlite3_bound_param*)data;

	if (param->name) {
		efree(param->name);
	}

	if (param->parameter) {
		zval_ptr_dtor(&(param->parameter));
		param->parameter = NULL;
	}
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(sqlite3)
{
	zend_class_entry ce;

#if defined(ZTS)
	/* Refuse to load if this wasn't a threasafe library loaded */
	if (!sqlite3_threadsafe()) {
		return FAILURE;
	}
#endif

	memcpy(&sqlite3_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&sqlite3_stmt_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&sqlite3_result_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	/* Register SQLite 3 Class */
	INIT_CLASS_ENTRY(ce, "SQLite3", php_sqlite3_class_methods);
	ce.create_object = php_sqlite3_object_new;
	sqlite3_object_handlers.clone_obj = NULL;
	php_sqlite3_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/* Register SQLite 3 Prepared Statement Class */
	INIT_CLASS_ENTRY(ce, "SQLite3_stmt", php_sqlite3_stmt_class_methods);
	ce.create_object = php_sqlite3_stmt_object_new;
	sqlite3_stmt_object_handlers.clone_obj = NULL;
	php_sqlite3_stmt_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/* Register SQLite 3 Result Class */
	INIT_CLASS_ENTRY(ce, "SQLite3_result", php_sqlite3_result_class_methods);
	ce.create_object = php_sqlite3_result_object_new;
	sqlite3_result_object_handlers.clone_obj = NULL;
	php_sqlite3_result_entry = zend_register_internal_class(&ce TSRMLS_CC);

	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("SQLITE3_ASSOC", PHP_SQLITE3_ASSOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_NUM", PHP_SQLITE3_NUM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_BOTH", PHP_SQLITE3_BOTH, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SQLITE3_INTEGER", SQLITE_INTEGER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_FLOAT", SQLITE_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_TEXT", SQLITE3_TEXT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_BLOB", SQLITE_BLOB, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_NULL", SQLITE_NULL, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SQLITE3_OPEN_READONLY", SQLITE_OPEN_READONLY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_OPEN_READWRITE", SQLITE_OPEN_READWRITE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLITE3_OPEN_CREATE", SQLITE_OPEN_CREATE, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(sqlite3)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(sqlite3)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "SQLite3 support", "enabled");
	php_info_print_table_row(2, "SQLite3 module version", PHP_SQLITE3_VERSION);
	php_info_print_table_row(2, "SQLite Library", sqlite3_libversion());
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(sqlite3)
{
	memset(sqlite3_globals, 0, sizeof(*sqlite3_globals));
}
/* }}} */

/* {{{ sqlite3_module_entry
 */
zend_module_entry sqlite3_module_entry = {
	STANDARD_MODULE_HEADER,
	"sqlite3",
	NULL,
	PHP_MINIT(sqlite3),
	PHP_MSHUTDOWN(sqlite3),
	NULL,
	NULL,
	PHP_MINFO(sqlite3),
	PHP_SQLITE3_VERSION,
#if ZEND_MODULE_API_NO >= 20060613
	PHP_MODULE_GLOBALS(sqlite3),
	PHP_GINIT(sqlite3),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
#else
	STANDARD_MODULE_PROPERTIES
#endif
};
/* }}} */

#ifdef COMPILE_DL_SQLITE3
ZEND_GET_MODULE(sqlite3)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
