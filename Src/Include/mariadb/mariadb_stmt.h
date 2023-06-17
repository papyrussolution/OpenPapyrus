/************************************************************************

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02111-1301, USA

   Part of this code includes code from PHP's mysqlnd extension
   (written by Andrey Hristov, Georg Richter and Ulf Wendel), freely
   available from http://www.php.net/software

 *************************************************************************/

#define MYSQL_NO_DATA 100
#define MYSQL_DATA_TRUNCATED 101
#define MYSQL_DEFAULT_PREFETCH_ROWS (ulong)1

/* Bind flags */
#define MADB_BIND_DUMMY 1

#define MARIADB_STMT_BULK_SUPPORTED(stmt) \
	((stmt)->mysql && \
	(!((stmt)->mysql->server_capabilities & CLIENT_MYSQL) && \
	((stmt)->mysql->extension->mariadb_server_capabilities & \
	(MARIADB_CLIENT_STMT_BULK_OPERATIONS >> 32))))

#define SET_CLIENT_STMT_ERROR(a, b, c, d) \
	do { \
		(a)->last_errno = (b); \
		strnzcpy((a)->sqlstate, (c), sizeof((a)->sqlstate)); \
		strnzcpy((a)->last_error, (d) ? (d) : ER((b)), sizeof((a)->last_error)); \
	} while(0)

#define CLEAR_CLIENT_STMT_ERROR(a) \
	do { \
		(a)->last_errno = 0; \
		strcpy((a)->sqlstate, "00000"); \
		(a)->last_error[0] = 0; \
	} while(0)

#define MYSQL_PS_SKIP_RESULT_W_LEN  -1
#define MYSQL_PS_SKIP_RESULT_STR    -2
#define STMT_ID_LENGTH 4

typedef struct st_mysql_stmt MYSQL_STMT;

typedef MYSQL_RES* (* mysql_stmt_use_or_store_func)(MYSQL_STMT *);

enum enum_stmt_attr_type {
	STMT_ATTR_UPDATE_MAX_LENGTH,
	STMT_ATTR_CURSOR_TYPE,
	STMT_ATTR_PREFETCH_ROWS,

	/* MariaDB only */
	STMT_ATTR_PREBIND_PARAMS = 200,
	STMT_ATTR_ARRAY_SIZE,
	STMT_ATTR_ROW_SIZE,
	STMT_ATTR_STATE,
	STMT_ATTR_CB_USER_DATA,
	STMT_ATTR_CB_PARAM,
	STMT_ATTR_CB_RESULT
};

enum enum_cursor_type {
	CURSOR_TYPE_NO_CURSOR = 0,
	CURSOR_TYPE_READ_ONLY = 1,
	CURSOR_TYPE_FOR_UPDATE = 2,
	CURSOR_TYPE_SCROLLABLE = 4
};

enum enum_indicator_type {
	STMT_INDICATOR_NTS = -1,
	STMT_INDICATOR_NONE = 0,
	STMT_INDICATOR_NULL = 1,
	STMT_INDICATOR_DEFAULT = 2,
	STMT_INDICATOR_IGNORE = 3,
	STMT_INDICATOR_IGNORE_ROW = 4
};
/*
   bulk PS flags
 */
#define STMT_BULK_FLAG_CLIENT_SEND_TYPES 128
#define STMT_BULK_FLAG_INSERT_ID_REQUEST 64

typedef enum mysql_stmt_state {
	MYSQL_STMT_INITTED = 0,
	MYSQL_STMT_PREPARED,
	MYSQL_STMT_EXECUTED,
	MYSQL_STMT_WAITING_USE_OR_STORE,
	MYSQL_STMT_USE_OR_STORE_CALLED,
	MYSQL_STMT_USER_FETCHING, /* fetch_row_buff or fetch_row_unbuf */
	MYSQL_STMT_FETCH_DONE
} enum_mysqlnd_stmt_state;

typedef struct st_mysql_bind {
	ulong * length; /* output length pointer */
	bool * is_null; /* Pointer to null indicator */
	void * buffer; /* buffer to get/put data */
	bool * error; /* set this if you want to track data truncations happened during fetch */
	union {
		uchar * row_ptr; /* for the current data position */
		char * indicator; /* indicator variable */
	} u;

	void (* store_param_func)(NET * net, struct st_mysql_bind * param);
	void (* fetch_result)(struct st_mysql_bind *, MYSQL_FIELD *, uchar ** row);
	void (* skip_result)(struct st_mysql_bind *, MYSQL_FIELD *, uchar ** row);
	ulong buffer_length; /* output buffer length, must be set when fetching str/binary */
	ulong offset; /* offset position for char/binary fetch */
	ulong length_value; /* Used if length is 0 */
	uint flags; /* special flags, e.g. for dummy bind  */
	uint pack_length; /* Internal length for packed data */
	enum   enum_field_types buffer_type; /* buffer type */
	bool   error_value; /* used if error is 0 */
	bool   is_unsigned; /* set if integer type is unsigned */
	bool   long_data_used; /* If used with mysql_send_long_data */
	bool   is_null_value; /* Used if is_null is 0 */
	void * extension;
} MYSQL_BIND;

typedef struct st_mysqlnd_upsert_result {
	uint warning_count;
	uint server_status;
	uint64 affected_rows;
	uint64 last_insert_id;
} mysql_upsert_status;

typedef struct st_mysql_cmd_buffer {
	uchar   * buffer;
	size_t length;
} MYSQL_CMD_BUFFER;

typedef struct st_mysql_error_info {
	uint error_no;
	char error[MYSQL_ERRMSG_SIZE+1];
	char sqlstate[SQLSTATE_LENGTH + 1];
} mysql_error_info;

struct st_mysqlnd_stmt_methods {
	bool (* prepare)(const MYSQL_STMT * stmt, const char * const query, size_t query_len);
	bool (* execute)(const MYSQL_STMT * stmt);
	MYSQL_RES * (* use_result)(const MYSQL_STMT * stmt);
	MYSQL_RES * (* store_result)(const MYSQL_STMT * stmt);
	MYSQL_RES * (* get_result)(const MYSQL_STMT * stmt);
	bool (* free_result)(const MYSQL_STMT * stmt);
	bool (* seek_data)(const MYSQL_STMT * stmt, uint64 row);
	bool (* reset)(const MYSQL_STMT * stmt);
	bool (* close)(const MYSQL_STMT * stmt); /* private */
	bool (* dtor)(const MYSQL_STMT * stmt); /* use this for mysqlnd_stmt_close */
	bool (* fetch)(const MYSQL_STMT * stmt, bool * const fetched_anything);
	bool (* bind_param)(const MYSQL_STMT * stmt, const MYSQL_BIND bind);
	bool (* refresh_bind_param)(const MYSQL_STMT * stmt);
	bool (* bind_result)(const MYSQL_STMT * stmt, const MYSQL_BIND * bind);
	bool (* send_long_data)(const MYSQL_STMT * stmt, uint param_num, const char * const data, size_t length);
	MYSQL_RES *(* get_parameter_metadata)(const MYSQL_STMT * stmt);
	MYSQL_RES *(* get_result_metadata)(const MYSQL_STMT * stmt);
	uint64 (* get_last_insert_id)(const MYSQL_STMT * stmt);
	uint64 (* get_affected_rows)(const MYSQL_STMT * stmt);
	uint64 (* get_num_rows)(const MYSQL_STMT * stmt);
	uint (* get_param_count)(const MYSQL_STMT * stmt);
	uint (* get_field_count)(const MYSQL_STMT * stmt);
	uint (* get_warning_count)(const MYSQL_STMT * stmt);
	uint (* get_error_no)(const MYSQL_STMT * stmt);
	const char * (* get_error_str)(const MYSQL_STMT * stmt);
	const char * (* get_sqlstate)(const MYSQL_STMT * stmt);
	bool (* get_attribute)(const MYSQL_STMT * stmt, enum enum_stmt_attr_type attr_type, const void * value);
	bool (* set_attribute)(const MYSQL_STMT * stmt, enum enum_stmt_attr_type attr_type, const void * value);
	void (* set_error)(MYSQL_STMT * stmt, uint error_nr, const char * sqlstate, const char * format, ...);
};

typedef int (* mysql_stmt_fetch_row_func)(MYSQL_STMT * stmt, uchar ** row);
typedef void (* ps_result_callback)(void * data, uint column, uchar ** row);
typedef bool *(* ps_param_callback)(void * data, MYSQL_BIND * bind, uint row_nr);

struct st_mysql_stmt {
	MA_MEM_ROOT mem_root;
	MYSQL * mysql;
	ulong stmt_id;
	ulong flags; /* cursor is set here */
	enum_mysqlnd_stmt_state state;
	MYSQL_FIELD * fields;
	uint field_count;
	uint param_count;
	uchar send_types_to_server;
	MYSQL_BIND * params;
	MYSQL_BIND * bind;
	MYSQL_DATA result; /* we don't use mysqlnd's result set logic */
	MYSQL_ROWS * result_cursor;
	bool bind_result_done;
	bool bind_param_done;
	mysql_upsert_status upsert_status;
	uint last_errno;
	char last_error[MYSQL_ERRMSG_SIZE+1];
	char sqlstate[SQLSTATE_LENGTH + 1];
	bool update_max_length;
	ulong prefetch_rows;
	LIST list;
	bool cursor_exists;
	void * extension;
	mysql_stmt_fetch_row_func fetch_row_func;
	uint execute_count; /* count how many times the stmt was executed */
	mysql_stmt_use_or_store_func default_rset_handler;
	struct st_mysqlnd_stmt_methods  * m;
	uint array_size;
	size_t row_size;
	uint prebind_params;
	void * user_data;
	ps_result_callback result_callback;
	ps_param_callback param_callback;
};

typedef void (* ps_field_fetch_func)(MYSQL_BIND * r_param, const MYSQL_FIELD * field, uchar ** row);
typedef struct st_mysql_perm_bind {
	ps_field_fetch_func func;
	/* should be signed int */
	int pack_len;
	ulong max_len;
} MYSQL_PS_CONVERSION;

extern MYSQL_PS_CONVERSION mysql_ps_fetch_functions[MYSQL_TYPE_GEOMETRY + 1];
ulong FASTCALL ma_net_safe_read(MYSQL * mysql);
void mysql_init_ps_subsystem(void);
ulong FASTCALL net_field_length(uchar ** packet);
int ma_simple_command(MYSQL * mysql, enum enum_server_command command, const char * arg,
    size_t length, bool skipp_check, void * opt_arg);
/*
 *  function prototypes
 */
MYSQL_STMT * STDCALL mysql_stmt_init(MYSQL * mysql);
int STDCALL mysql_stmt_prepare(MYSQL_STMT * stmt, const char * query, ulong length);
int STDCALL mysql_stmt_execute(MYSQL_STMT * stmt);
int STDCALL mysql_stmt_fetch(MYSQL_STMT * stmt);
int STDCALL mysql_stmt_fetch_column(MYSQL_STMT * stmt, MYSQL_BIND * bind_arg, uint column, ulong offset);
int STDCALL mysql_stmt_store_result(MYSQL_STMT * stmt);
ulong STDCALL mysql_stmt_param_count(MYSQL_STMT * stmt);
bool STDCALL mysql_stmt_attr_set(MYSQL_STMT * stmt, enum enum_stmt_attr_type attr_type, const void * attr);
bool STDCALL mysql_stmt_attr_get(MYSQL_STMT * stmt, enum enum_stmt_attr_type attr_type, void * attr);
bool STDCALL mysql_stmt_bind_param(MYSQL_STMT * stmt, const MYSQL_BIND * bnd);
bool STDCALL mysql_stmt_bind_result(MYSQL_STMT * stmt, const MYSQL_BIND * bnd);
bool STDCALL mysql_stmt_close(MYSQL_STMT * stmt);
bool STDCALL mysql_stmt_reset(MYSQL_STMT * stmt);
bool STDCALL mysql_stmt_free_result(MYSQL_STMT * stmt);
bool STDCALL mysql_stmt_send_long_data(MYSQL_STMT * stmt, uint param_number, const char * data, ulong length);
MYSQL_RES * STDCALL mysql_stmt_result_metadata(MYSQL_STMT * stmt);
MYSQL_RES * STDCALL mysql_stmt_param_metadata(MYSQL_STMT * stmt);
uint STDCALL mysql_stmt_errno(MYSQL_STMT * stmt);
const char * STDCALL mysql_stmt_error(MYSQL_STMT * stmt);
const char * STDCALL mysql_stmt_sqlstate(MYSQL_STMT * stmt);
MYSQL_ROW_OFFSET STDCALL mysql_stmt_row_seek(MYSQL_STMT * stmt, MYSQL_ROW_OFFSET offset);
MYSQL_ROW_OFFSET STDCALL mysql_stmt_row_tell(MYSQL_STMT * stmt);
void STDCALL mysql_stmt_data_seek(MYSQL_STMT * stmt, uint64 offset);
uint64 STDCALL mysql_stmt_num_rows(MYSQL_STMT * stmt);
uint64 STDCALL mysql_stmt_affected_rows(MYSQL_STMT * stmt);
uint64 STDCALL mysql_stmt_insert_id(MYSQL_STMT * stmt);
uint STDCALL mysql_stmt_field_count(MYSQL_STMT * stmt);
int STDCALL mysql_stmt_next_result(MYSQL_STMT * stmt);
bool STDCALL mysql_stmt_more_results(MYSQL_STMT * stmt);
int STDCALL mariadb_stmt_execute_direct(MYSQL_STMT * stmt, const char * stmt_str, size_t length);
MYSQL_FIELD * STDCALL mariadb_stmt_fetch_fields(MYSQL_STMT * stmt);
