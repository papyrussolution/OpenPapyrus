// DBOCI.H
// Copyright (c) A.Sobolev 2008, 2009
//
#include <slib.h>
//
// PUBLIC ORACLE TYPES AND CONSTANTS
//
// -----------------------------Handle Types----------------------------------
// handle types range [1..49]
//
#define OCI_HTYPE_FIRST          1 // start value of handle type
#define OCI_HTYPE_ENV            1 // environment handle
#define OCI_HTYPE_ERROR          2 // error handle
#define OCI_HTYPE_SVCCTX         3 // service handle
#define OCI_HTYPE_STMT           4 // statement handle
#define OCI_HTYPE_BIND           5 // bind handle
#define OCI_HTYPE_DEFINE         6 // define handle
#define OCI_HTYPE_DESCRIBE       7 // describe handle
#define OCI_HTYPE_SERVER         8 // server handle
#define OCI_HTYPE_SESSION        9 // authentication handle
#define OCI_HTYPE_AUTHINFO      OCI_HTYPE_SESSION // SessionGet auth handle
#define OCI_HTYPE_TRANS         10 // transaction handle
#define OCI_HTYPE_COMPLEXOBJECT 11 // complex object retrieval handle
#define OCI_HTYPE_SECURITY      12 // security handle
#define OCI_HTYPE_SUBSCRIPTION  13 // subscription handle
#define OCI_HTYPE_DIRPATH_CTX   14 // direct path context
#define OCI_HTYPE_DIRPATH_COLUMN_ARRAY 15 // direct path column array
#define OCI_HTYPE_DIRPATH_STREAM       16 // direct path stream
#define OCI_HTYPE_PROC                 17 // process handle
#define OCI_HTYPE_DIRPATH_FN_CTX       18 // direct path function context
#define OCI_HTYPE_DIRPATH_FN_COL_ARRAY 19 // dp object column array
#define OCI_HTYPE_XADSESSION    20 // access driver session
#define OCI_HTYPE_XADTABLE      21 // access driver table
#define OCI_HTYPE_XADFIELD      22 // access driver field
#define OCI_HTYPE_XADGRANULE    23 // access driver granule
#define OCI_HTYPE_XADRECORD     24 // access driver record
#define OCI_HTYPE_XADIO         25 // access driver I/O
#define OCI_HTYPE_CPOOL         26 // connection pool handle
#define OCI_HTYPE_SPOOL         27 // session pool handle

#define OCI_HTYPE_LAST          27 // last value of a handle type
//
// -------------------------Descriptor Types----------------------------------
// descriptor values range [50..255]
#define OCI_DTYPE_FIRST             50 // start value of descriptor type
#define OCI_DTYPE_LOB               50 // lob  locator
#define OCI_DTYPE_SNAP              51 // snapshot descriptor
#define OCI_DTYPE_RSET              52 // result set descriptor
#define OCI_DTYPE_PARAM             53 // a parameter descriptor obtained from ocigparm
#define OCI_DTYPE_ROWID             54 // rowid descriptor
#define OCI_DTYPE_COMPLEXOBJECTCOMP 55 // complex object retrieval descriptor
#define OCI_DTYPE_FILE              56 // File Lob locator
#define OCI_DTYPE_AQENQ_OPTIONS     57 // enqueue options
#define OCI_DTYPE_AQDEQ_OPTIONS     58 // dequeue options
#define OCI_DTYPE_AQMSG_PROPERTIES  59 // message properties
#define OCI_DTYPE_AQAGENT           60 // aq agent
#define OCI_DTYPE_LOCATOR           61 // LOB locator
#define OCI_DTYPE_INTERVAL_YM       62 // Interval year month
#define OCI_DTYPE_INTERVAL_DS       63 // Interval day second
#define OCI_DTYPE_AQNFY_DESCRIPTOR  64 // AQ notify descriptor
#define OCI_DTYPE_DATE              65 // Date
#define OCI_DTYPE_TIME              66 // Time
#define OCI_DTYPE_TIME_TZ           67 // Time with timezone
#define OCI_DTYPE_TIMESTAMP         68 // Timestamp
#define OCI_DTYPE_TIMESTAMP_TZ      69 // Timestamp with timezone
#define OCI_DTYPE_TIMESTAMP_LTZ     70 // Timestamp with local tz
#define OCI_DTYPE_UCB               71 // user callback descriptor
#define OCI_DTYPE_SRVDN             72 // server DN list descriptor
#define OCI_DTYPE_SIGNATURE         73 // signature
#define OCI_DTYPE_RESERVED_1        74 // reserved for internal use
#define OCI_DTYPE_LAST              74 // last value of a descriptor type
//
// --------------------------------LOB types ---------------------------------
//
#define OCI_TEMP_BLOB 1 // LOB type - BLOB ------------------
#define OCI_TEMP_CLOB 2 // LOB type - CLOB ------------------
//
// -------------------------Object Ptr Types----------------------------------
//
#define OCI_OTYPE_NAME 1 // object name
#define OCI_OTYPE_REF  2 // REF to TDO
#define OCI_OTYPE_PTR  3 // PTR to TDO
//
// =============================Attribute Types===============================
// Note: All attributes are global.  New attibutes should be added to the end of the list.
// Before you add an attribute see if an existing one can be used for your handle.
//
// If you see any holes please use the holes first.
//
#define OCI_ATTR_FNCODE                  1 // the OCI function code
#define OCI_ATTR_OBJECT                  2 // is the environment initialized in object mode
#define OCI_ATTR_NONBLOCKING_MODE        3 // non blocking mode
#define OCI_ATTR_SQLCODE                 4 // the SQL verb
#define OCI_ATTR_ENV                     5 // the environment handle
#define OCI_ATTR_SERVER                  6 // the server handle
#define OCI_ATTR_SESSION                 7 // the user session handle
#define OCI_ATTR_TRANS                   8 // the transaction handle
#define OCI_ATTR_ROW_COUNT               9 // the rows processed so far
#define OCI_ATTR_SQLFNCODE              10 // the SQL verb of the statement
#define OCI_ATTR_PREFETCH_ROWS          11 // sets the number of rows to prefetch
#define OCI_ATTR_NESTED_PREFETCH_ROWS   12 // the prefetch rows of nested table
#define OCI_ATTR_PREFETCH_MEMORY        13 // memory limit for rows fetched
#define OCI_ATTR_NESTED_PREFETCH_MEMORY 14 // memory limit for nested rows
#define OCI_ATTR_CHAR_COUNT             15 // this specifies the bind and define size in characters
#define OCI_ATTR_PDSCL                  16 // packed decimal scale
#define OCI_ATTR_FSPRECISION            OCI_ATTR_PDSCL // fs prec for datetime data types
#define OCI_ATTR_PDPRC                  17 // packed decimal format
#define OCI_ATTR_LFPRECISION            OCI_ATTR_PDPRC // fs prec for datetime data types
#define OCI_ATTR_PARAM_COUNT            18 // number of column in the select list
#define OCI_ATTR_ROWID                  19 // the rowid
#define OCI_ATTR_CHARSET                20 // the character set value
#define OCI_ATTR_NCHAR                  21 // NCHAR type
#define OCI_ATTR_USERNAME               22 // username attribute
#define OCI_ATTR_PASSWORD               23 // password attribute
#define OCI_ATTR_STMT_TYPE              24 // statement type
#define OCI_ATTR_INTERNAL_NAME          25 // user friendly global name
#define OCI_ATTR_EXTERNAL_NAME          26 // the internal name for global txn
#define OCI_ATTR_XID                    27 // XOPEN defined global transaction id
#define OCI_ATTR_TRANS_LOCK             28 //
#define OCI_ATTR_TRANS_NAME             29 // string to identify a global transaction
#define OCI_ATTR_HEAPALLOC              30 // memory allocated on the heap
#define OCI_ATTR_CHARSET_ID             31 // Character Set ID
#define OCI_ATTR_CHARSET_FORM           32 // Character Set Form
#define OCI_ATTR_MAXDATA_SIZE           33 // Maximumsize of data on the server
#define OCI_ATTR_CACHE_OPT_SIZE         34 // object cache optimal size
#define OCI_ATTR_CACHE_MAX_SIZE         35 // object cache maximum size percentage
#define OCI_ATTR_PINOPTION              36 // object cache default pin option
#define OCI_ATTR_ALLOC_DURATION         37 // object cache default allocation duration
#define OCI_ATTR_PIN_DURATION           38 // object cache default pin duration
#define OCI_ATTR_FDO                    39 // Format Descriptor object attribute
#define OCI_ATTR_POSTPROCESSING_CALLBACK 40 // Callback to process outbind data
#define OCI_ATTR_POSTPROCESSING_CONTEXT  41 // Callback context to process outbind data
#define OCI_ATTR_ROWS_RETURNED           42 // Number of rows returned in current iter - for Bind handles
#define OCI_ATTR_FOCBK                   43 // Failover Callback attribute
#define OCI_ATTR_IN_V8_MODE              44 // is the server/service context in V8 mode
#define OCI_ATTR_LOBEMPTY                45 // empty lob ?
#define OCI_ATTR_SESSLANG                46 // session language handle
#define OCI_ATTR_VISIBILITY             47 // visibility
#define OCI_ATTR_RELATIVE_MSGID         48 // relative message id
#define OCI_ATTR_SEQUENCE_DEVIATION     49 // sequence deviation
#define OCI_ATTR_CONSUMER_NAME          50 // consumer name
#define OCI_ATTR_DEQ_MODE               51 // dequeue mode
#define OCI_ATTR_NAVIGATION             52 // navigation
#define OCI_ATTR_WAIT                   53 // wait
#define OCI_ATTR_DEQ_MSGID              54 // dequeue message id
#define OCI_ATTR_PRIORITY               55 // priority
#define OCI_ATTR_DELAY                  56 // delay
#define OCI_ATTR_EXPIRATION             57 // expiration
#define OCI_ATTR_CORRELATION            58 // correlation id
#define OCI_ATTR_ATTEMPTS               59 // # of attempts
#define OCI_ATTR_RECIPIENT_LIST         60 // recipient list
#define OCI_ATTR_EXCEPTION_QUEUE        61 // exception queue name
#define OCI_ATTR_ENQ_TIME               62 // enqueue time (only OCIAttrGet)
#define OCI_ATTR_MSG_STATE              63 // message state (only OCIAttrGet) NOTE: 64-66 used below
#define OCI_ATTR_AGENT_NAME             64 // agent name
#define OCI_ATTR_AGENT_ADDRESS          65 // agent address
#define OCI_ATTR_AGENT_PROTOCOL         66 // agent protocol
#define OCI_ATTR_SENDER_ID              68 // sender id
#define OCI_ATTR_ORIGINAL_MSGID         69 // original message id
#define OCI_ATTR_QUEUE_NAME             70 // queue name
#define OCI_ATTR_NFY_MSGID              71 // message id
#define OCI_ATTR_MSG_PROP               72 // message properties
#define OCI_ATTR_NUM_DML_ERRORS         73 // num of errs in array DML
#define OCI_ATTR_DML_ROW_OFFSET         74 // row offset in the array
#define OCI_ATTR_DATEFORMAT             75 // default date format string
#define OCI_ATTR_BUF_ADDR               76 // buffer address
#define OCI_ATTR_BUF_SIZE               77 // buffer size
#define OCI_ATTR_DIRPATH_MODE           78 // mode of direct path operation
#define OCI_ATTR_DIRPATH_NOLOG          79 // nologging option
#define OCI_ATTR_DIRPATH_PARALLEL       80 // parallel (temp seg) option
#define OCI_ATTR_NUM_ROWS               81 // number of rows in column array
 // NOTE that OCI_ATTR_NUM_COLS is a column array attribute too.
#define OCI_ATTR_COL_COUNT              82 // columns of column array processed so far.
#define OCI_ATTR_STREAM_OFFSET          83 // str off of last row processed
#define OCI_ATTR_SHARED_HEAPALLOC       84 // Shared Heap Allocation Size
#define OCI_ATTR_SERVER_GROUP           85 // server group name
#define OCI_ATTR_MIGSESSION             86 // migratable session attribute
#define OCI_ATTR_NOCACHE                87 // Temporary LOBs
#define OCI_ATTR_MEMPOOL_SIZE           88 // Pool Size
#define OCI_ATTR_MEMPOOL_INSTNAME       89 // Instance name
#define OCI_ATTR_MEMPOOL_APPNAME        90 // Application name
#define OCI_ATTR_MEMPOOL_HOMENAME       91 // Home Directory name
#define OCI_ATTR_MEMPOOL_MODEL          92 // Pool Model (proc,thrd,both)
#define OCI_ATTR_MODES                  93 // Modes
#define OCI_ATTR_SUBSCR_NAME            94 // name of subscription
#define OCI_ATTR_SUBSCR_CALLBACK        95 // associated callback
#define OCI_ATTR_SUBSCR_CTX             96 // associated callback context
#define OCI_ATTR_SUBSCR_PAYLOAD         97 // associated payload
#define OCI_ATTR_SUBSCR_NAMESPACE       98 // associated namespace
#define OCI_ATTR_PROXY_CREDENTIALS      99 // Proxy user credentials
#define OCI_ATTR_INITIAL_CLIENT_ROLES  100 // Initial client role list
#define OCI_ATTR_UNK                   101 // unknown attribute
#define OCI_ATTR_NUM_COLS              102 // number of columns
#define OCI_ATTR_LIST_COLUMNS          103 // parameter of the column list
#define OCI_ATTR_RDBA                  104 // DBA of the segment header
#define OCI_ATTR_CLUSTERED             105 // whether the table is clustered
#define OCI_ATTR_PARTITIONED           106 // whether the table is partitioned
#define OCI_ATTR_INDEX_ONLY            107 // whether the table is index only
#define OCI_ATTR_LIST_ARGUMENTS        108 // parameter of the argument list
#define OCI_ATTR_LIST_SUBPROGRAMS      109 // parameter of the subprogram list
#define OCI_ATTR_REF_TDO               110 // REF to the type descriptor
#define OCI_ATTR_LINK                  111 // the database link name
#define OCI_ATTR_MIN                   112 // minimum value
#define OCI_ATTR_MAX                   113 // maximum value
#define OCI_ATTR_INCR                  114 // increment value
#define OCI_ATTR_CACHE                 115 // number of sequence numbers cached
#define OCI_ATTR_ORDER                 116 // whether the sequence is ordered
#define OCI_ATTR_HW_MARK               117 // high-water mark
#define OCI_ATTR_TYPE_SCHEMA           118 // type's schema name
#define OCI_ATTR_TIMESTAMP             119 // timestamp of the object
#define OCI_ATTR_NUM_ATTRS             120 // number of sttributes
#define OCI_ATTR_NUM_PARAMS            121 // number of parameters
#define OCI_ATTR_OBJID                 122 // object id for a table or view
#define OCI_ATTR_PTYPE                 123 // type of info described by
#define OCI_ATTR_PARAM                 124 // parameter descriptor
#define OCI_ATTR_OVERLOAD_ID           125 // overload ID for funcs and procs
#define OCI_ATTR_TABLESPACE            126 // table name space
#define OCI_ATTR_TDO                   127 // TDO of a type
#define OCI_ATTR_LTYPE                 128 // list type
#define OCI_ATTR_PARSE_ERROR_OFFSET    129 // Parse Error offset
#define OCI_ATTR_IS_TEMPORARY          130 // whether table is temporary
#define OCI_ATTR_IS_TYPED              131 // whether table is typed
#define OCI_ATTR_DURATION              132 // duration of temporary table
#define OCI_ATTR_IS_INVOKER_RIGHTS     133 // is invoker rights
#define OCI_ATTR_OBJ_NAME              134 // top level schema obj name
#define OCI_ATTR_OBJ_SCHEMA            135 // schema name
#define OCI_ATTR_OBJ_ID                136 // top level schema object id
#define OCI_ATTR_DIRPATH_SORTED_INDEX  137 // index that data is sorted on
	// direct path index maint method (see oci8dp.h)
#define OCI_ATTR_DIRPATH_INDEX_MAINT_METHOD 138
	// parallel load: db file, initial and next extent sizes
#define OCI_ATTR_DIRPATH_FILE               139 // DB file to load into
#define OCI_ATTR_DIRPATH_STORAGE_INITIAL    140 // initial extent size
#define OCI_ATTR_DIRPATH_STORAGE_NEXT       141 // next extent size
#define OCI_ATTR_TRANS_TIMEOUT              142 // transaction timeout
#define OCI_ATTR_SERVER_STATUS              143// state of the server handle
#define OCI_ATTR_STATEMENT                  144 // statement txt in stmt hdl. statement should not be executed in cache
#define OCI_ATTR_NO_CACHE                   145
#define OCI_ATTR_DEQCOND                    146 // dequeue condition
#define OCI_ATTR_RESERVED_2                 147 // reserved
#define OCI_ATTR_SUBSCR_RECPT               148 // recepient of subscription
#define OCI_ATTR_SUBSCR_RECPTPROTO          149 // protocol for recepient. 8.2 dpapi support of ADTs
#define OCI_ATTR_DIRPATH_EXPR_TYPE  150 // expr type of OCI_ATTR_NAME
#define OCI_ATTR_DIRPATH_INPUT      151 // input in text or stream format
	#define OCI_DIRPATH_INPUT_TEXT     0x01
	#define OCI_DIRPATH_INPUT_STREAM   0x02
	#define OCI_DIRPATH_INPUT_UNKNOWN  0x04
#define OCI_ATTR_LDAP_HOST       153 // LDAP host to connect to
#define OCI_ATTR_LDAP_PORT       154 // LDAP port to connect to
#define OCI_ATTR_BIND_DN         155 // bind DN
#define OCI_ATTR_LDAP_CRED       156 // credentials to connect to LDAP
#define OCI_ATTR_WALL_LOC        157 // client wallet location
#define OCI_ATTR_LDAP_AUTH       158 // LDAP authentication method
#define OCI_ATTR_LDAP_CTX        159 // LDAP adminstration context DN
#define OCI_ATTR_SERVER_DNS      160 // list of registration server DNs
#define OCI_ATTR_DN_COUNT        161 // the number of server DNs
#define OCI_ATTR_SERVER_DN       162 // server DN attribute
#define OCI_ATTR_MAXCHAR_SIZE               163 // max char size of data
#define OCI_ATTR_CURRENT_POSITION           164 // for scrollable result sets

// Added to get attributes for ref cursor to statement handle
#define OCI_ATTR_RESERVED_3                 165 // reserved
#define OCI_ATTR_RESERVED_4                 166 // reserved
#define OCI_ATTR_DIRPATH_FN_CTX             167 // fn ctx ADT attrs or args
#define OCI_ATTR_DIGEST_ALGO                168 // digest algorithm
#define OCI_ATTR_CERTIFICATE                169 // certificate
#define OCI_ATTR_SIGNATURE_ALGO             170 // signature algorithm
#define OCI_ATTR_CANONICAL_ALGO             171 // canonicalization algo.
#define OCI_ATTR_PRIVATE_KEY                172 // private key
#define OCI_ATTR_DIGEST_VALUE               173 // digest value
#define OCI_ATTR_SIGNATURE_VAL              174 // signature value
#define OCI_ATTR_SIGNATURE                  175 // signature

// attributes for setting OCI stmt caching specifics in svchp
#define OCI_ATTR_STMTCACHESIZE              176 // size of the stm cache

// --------------------------- Connection Pool Attributes ------------------
#define OCI_ATTR_CONN_NOWAIT               178
#define OCI_ATTR_CONN_BUSY_COUNT            179
#define OCI_ATTR_CONN_OPEN_COUNT            180
#define OCI_ATTR_CONN_TIMEOUT               181
#define OCI_ATTR_STMT_STATE                 182
#define OCI_ATTR_CONN_MIN                   183
#define OCI_ATTR_CONN_MAX                   184
#define OCI_ATTR_CONN_INCR                  185

#define OCI_ATTR_DIRPATH_OID                187 // loading into an OID col

#define OCI_ATTR_NUM_OPEN_STMTS             188 // open stmts in session
#define OCI_ATTR_DESCRIBE_NATIVE            189 // get native info via desc

#define OCI_ATTR_BIND_COUNT                 190 // number of bind postions
#define OCI_ATTR_HANDLE_POSITION            191 // pos of bind/define handle
#define OCI_ATTR_RESERVED_5                 192 // reserverd
#define OCI_ATTR_SERVER_BUSY                193 // call in progress on server

#define OCI_ATTR_DIRPATH_SID                194 // loading into an SID col
// notification presentation for recipient
#define OCI_ATTR_SUBSCR_RECPTPRES           195
#define OCI_ATTR_TRANSFORMATION             196 // AQ message transformation

#define OCI_ATTR_ROWS_FETCHED               197 // rows fetched in last call

// --------------------------- Snapshot attributes -------------------------
#define OCI_ATTR_SCN_BASE                   198 // snapshot base
#define OCI_ATTR_SCN_WRAP                   199 // snapshot wrap

// --------------------------- Miscellanous attributes ---------------------
#define OCI_ATTR_RESERVED_6                 200 // reserved
#define OCI_ATTR_READONLY_TXN               201 // txn is readonly
#define OCI_ATTR_RESERVED_7                 202 // reserved
#define OCI_ATTR_ERRONEOUS_COLUMN           203 // position of erroneous col
#define OCI_ATTR_RESERVED_8                 204 // reserved

// -------------------- 8.2 dpapi support of ADTs continued ----------------
#define OCI_ATTR_DIRPATH_OBJ_CONSTR         206 // obj type of subst obj tbl

// ***********************FREE attribute     207      ************************
// ***********************FREE attribute     208      ************************
#define OCI_ATTR_ENV_UTF16                  209 // is env in utf16 mode?
#define OCI_ATTR_RESERVED_9                 210 // reserved for TMZ
#define OCI_ATTR_RESERVED_10                211 // reserved

// Attr to allow setting of the stream version PRIOR to calling Prepare
#define OCI_ATTR_DIRPATH_STREAM_VERSION     212 // version of the stream
#define OCI_ATTR_RESERVED_11                213 // reserved

#define OCI_ATTR_RESERVED_12                214 // reserved
#define OCI_ATTR_RESERVED_13                215 // reserved

// ------------- Supported Values for Direct Path Stream Version -------------
#define OCI_DIRPATH_STREAM_VERSION_1 100
#define OCI_DIRPATH_STREAM_VERSION_2 200

// -------- client side character and national character set ids -----------
#define OCI_ATTR_ENV_CHARSET_ID   OCI_ATTR_CHARSET_ID // charset id in env
#define OCI_ATTR_ENV_NCHARSET_ID  OCI_ATTR_NCHARSET_ID // ncharset id in env

// ------------- Supported Values for protocol for recepient -----------------
#define OCI_SUBSCR_PROTO_OCI                0 // oci
#define OCI_SUBSCR_PROTO_MAIL               1 // mail
#define OCI_SUBSCR_PROTO_SERVER             2 // server
#define OCI_SUBSCR_PROTO_HTTP               3 // http
#define OCI_SUBSCR_PROTO_MAX                4 // max current protocols

// ------------- Supported Values for presentation for recepient -------------
#define OCI_SUBSCR_PRES_DEFAULT             0 // default
#define OCI_SUBSCR_PRES_XML                 1 // xml
#define OCI_SUBSCR_PRES_MAX                 2 // max current presentations


// ----- Temporary attribute value for UCS2/UTF16 character set ID --------
#define OCI_UCS2ID            1000 // UCS2 charset ID
#define OCI_UTF16ID           1000 // UTF16 charset ID

// ============================== End OCI Attribute Types ====================

// ---------------- Server Handle Attribute Values ---------------------------

// OCI_ATTR_SERVER_STATUS
#define OCI_SERVER_NOT_CONNECTED        0x0
#define OCI_SERVER_NORMAL               0x1

// ---------------------------------------------------------------------------

// ------------------------- Supported Namespaces  ---------------------------
#define OCI_SUBSCR_NAMESPACE_ANONYMOUS   0 // Anonymous Namespace
#define OCI_SUBSCR_NAMESPACE_AQ          1 // Advanced Queues
#define OCI_SUBSCR_NAMESPACE_MAX         2 // Max Name Space Number


// -------------------------Credential Types----------------------------------
#define OCI_CRED_RDBMS      1 // database username/password
#define OCI_CRED_EXT        2 // externally provided credentials
#define OCI_CRED_PROXY      3 // proxy authentication
#define OCI_CRED_RESERVED_1 4 // reserved
// ---------------------------------------------------------------------------

// ------------------------Error Return Values--------------------------------
#define OCI_SUCCESS                 0 // maps to SQL_SUCCESS of SAG CLI
#define OCI_SUCCESS_WITH_INFO       1 // maps to SQL_SUCCESS_WITH_INFO
#define OCI_RESERVED_FOR_INT_USE  200 // reserved
#define OCI_NO_DATA               100 // maps to SQL_NO_DATA
#define OCI_ERROR                  -1 // maps to SQL_ERROR
#define OCI_INVALID_HANDLE         -2 // maps to SQL_INVALID_HANDLE
#define OCI_NEED_DATA              99 // maps to SQL_NEED_DATA
#define OCI_STILL_EXECUTING     -3123 // OCI would block error
#define OCI_CONTINUE           -24200 // Continue with the body of the OCI function
// ---------------------------------------------------------------------------

// ------------------DateTime and Interval check Error codes------------------

// DateTime Error Codes used by OCIDateTimeCheck()
#define   OCI_DT_INVALID_DAY         0x1 // Bad day
#define   OCI_DT_DAY_BELOW_VALID     0x2 // Bad DAy Low/high bit (1=low)
#define   OCI_DT_INVALID_MONTH       0x4 // Bad MOnth
#define   OCI_DT_MONTH_BELOW_VALID   0x8 // Bad MOnth Low/high bit (1=low)
#define   OCI_DT_INVALID_YEAR        0x10 // Bad YeaR
#define   OCI_DT_YEAR_BELOW_VALID    0x20 // Bad YeaR Low/high bit (1=low)
#define   OCI_DT_INVALID_HOUR        0x40 // Bad HouR
#define   OCI_DT_HOUR_BELOW_VALID    0x80 // Bad HouR Low/high bit (1=low)
#define   OCI_DT_INVALID_MINUTE      0x100 // Bad MiNute
#define   OCI_DT_MINUTE_BELOW_VALID  0x200 // Bad MiNute Low/high bit (1=low)
#define   OCI_DT_INVALID_SECOND      0x400 // Bad SeCond
#define   OCI_DT_SECOND_BELOW_VALID  0x800 // bad second Low/high bit (1=low)
#define   OCI_DT_DAY_MISSING_FROM_1582 0x1000 // Day is one of those "missing" from 1582
#define   OCI_DT_YEAR_ZERO             0x2000 // Year may not equal zero
#define   OCI_DT_INVALID_TIMEZONE      0x4000 // Bad Timezone
#define   OCI_DT_INVALID_FORMAT        0x8000 // Bad date format input


// Interval Error Codes used by OCIInterCheck()
#define   OCI_INTER_INVALID_DAY           0x1 // Bad day
#define   OCI_INTER_DAY_BELOW_VALID       0x2 // Bad DAy Low/high bit (1=low)
#define   OCI_INTER_INVALID_MONTH         0x4 // Bad MOnth
#define   OCI_INTER_MONTH_BELOW_VALID     0x8 // Bad MOnth Low/high bit (1=low)
#define   OCI_INTER_INVALID_YEAR         0x10 // Bad YeaR
#define   OCI_INTER_YEAR_BELOW_VALID     0x20 // Bad YeaR Low/high bit (1=low)
#define   OCI_INTER_INVALID_HOUR         0x40 // Bad HouR
#define   OCI_INTER_HOUR_BELOW_VALID     0x80 // Bad HouR Low/high bit (1=low)
#define   OCI_INTER_INVALID_MINUTE      0x100 // Bad MiNute
#define   OCI_INTER_MINUTE_BELOW_VALID  0x200 // Bad MiNute Low/high bit(1=low)
#define   OCI_INTER_INVALID_SECOND      0x400 // Bad SeCond
#define   OCI_INTER_SECOND_BELOW_VALID  0x800 // bad second Low/high bit(1=low)
#define   OCI_INTER_INVALID_FRACSEC     0x1000 // Bad Fractional second
#define   OCI_INTER_FRACSEC_BELOW_VALID 0x2000 // Bad fractional second Low/High


// ------------------------Parsing Syntax Types-------------------------------
#define OCI_V7_SYNTAX 2 // V815 language - for backwards compatibility
#define OCI_V8_SYNTAX 3 // V815 language - for backwards compatibility
#define OCI_NTV_SYNTAX 1 // Use what so ever is the native lang of server
					 // these values must match the values defined in kpul.h
// ---------------------------------------------------------------------------

//------------------------Scrollable Cursor Fetch Options-------------------
// For non-scrollable cursor, the only valid (and default) orientation is OCI_FETCH_NEXT
//
#define OCI_FETCH_CURRENT    0x01 // refetching current position
#define OCI_FETCH_NEXT       0x02 // next row
#define OCI_FETCH_FIRST      0x04 // first row of the result set
#define OCI_FETCH_LAST       0x08 // the last row of the result set
#define OCI_FETCH_PRIOR      0x10 // the previous row relative to current
#define OCI_FETCH_ABSOLUTE   0x20 // absolute offset from first
#define OCI_FETCH_RELATIVE   0x40 // offset relative to current
#define OCI_FETCH_RESERVED_1 0x80 // reserved

// ---------------------------------------------------------------------------

// ------------------------Bind and Define Options----------------------------
#define OCI_SB2_IND_PTR       0x01 // unused
#define OCI_DATA_AT_EXEC      0x02 // data at execute time
#define OCI_DYNAMIC_FETCH     0x02 // fetch dynamically
#define OCI_PIECEWISE         0x04 // piecewise DMLs or fetch
#define OCI_DEFINE_RESERVED_1 0x08 // reserved
#define OCI_BIND_RESERVED_2   0x10 // reserved
#define OCI_DEFINE_RESERVED_2 0x20 // reserved
// ---------------------------------------------------------------------------

// -----------------------------  Various Modes ------------------------------
#define OCI_DEFAULT         0x00000000 // the default value for parameters and attributes
// -------------OCIInitialize Modes / OCICreateEnvironment Modes -------------
#define OCI_THREADED        0x00000001 // appl. in threaded environment
#define OCI_OBJECT          0x00000002 // application in object environment
#define OCI_EVENTS          0x00000004 // application is enabled for events
#define OCI_RESERVED1       0x00000008 // reserved
#define OCI_SHARED          0x00000010 // the application is in shared mode
#define OCI_RESERVED2       0x00000020 // reserved
// The following *TWO* are only valid for OCICreateEnvironment call
#define OCI_NO_UCB          0x00000040 // No user callback called during ini
#define OCI_NO_MUTEX        0x00000080 // the environment handle will not be protected by a mutex internally
#define OCI_SHARED_EXT      0x00000100 // Used for shared forms
#define OCI_CACHE           0x00000200 // turn on  DB Cache
#define OCI_ALWAYS_BLOCKING 0x00000400 // all connections always blocking
#define OCI_NO_CACHE        0x00000800 // turn off DB Cache mode
#define OCI_USE_LDAP        0x00001000 // allow  LDAP connections
#define OCI_REG_LDAPONLY    0x00002000 // only register to LDAP
#define OCI_UTF16           0x00004000 // mode for all UTF16 metadata
#define OCI_AFC_PAD_ON      0x00008000 // turn on AFC blank padding when rlenp present
#define OCI_ENVCR_RESERVED3 0x00010000 // reserved
#define OCI_NEW_LENGTH_SEMANTICS  0x00020000 // adopt new length semantics the new length semantics, always bytes,
	// is used by OCIEnvNlsCreate
#define OCI_NO_MUTEX_STMT   0x00040000 // Do not mutex stmt handle
#define OCI_MUTEX_ENV_ONLY  0x00080000 // Mutex only the environment handle
#define OCI_STM_RESERVED4   0x00100000 // reserved

// ---------------------------------------------------------------------------
// ------------------------OCIConnectionpoolCreate Modes----------------------

#define OCI_CPOOL_REINITIALIZE 0x111

// ---------------------------------------------------------------------------
// --------------------------------- OCILogon2 Modes -------------------------

#define OCI_LOGON2_SPOOL       0x0001     // Use session pool
#define OCI_LOGON2_CPOOL       OCI_CPOOL  // Use connection pool
#define OCI_LOGON2_STMTCACHE   0x0004     // Use Stmt Caching
#define OCI_LOGON2_PROXY       0x0008     // Proxy authentiaction

// ---------------------------------------------------------------------------
// ------------------------- OCISessionPoolCreate Modes ----------------------

#define OCI_SPC_REINITIALIZE 0x0001   // Reinitialize the session pool
#define OCI_SPC_HOMOGENEOUS  0x0002   // Session pool is homogeneneous
#define OCI_SPC_STMTCACHE    0x0004   // Session pool has stmt cache

// ---------------------------------------------------------------------------
// --------------------------- OCISessionGet Modes ---------------------------

#define OCI_SESSGET_SPOOL      0x0001     // SessionGet called in SPOOL mode
#define OCI_SESSGET_CPOOL      OCI_CPOOL  // SessionGet called in CPOOL mode
#define OCI_SESSGET_STMTCACHE  0x0004                 // Use statement cache
#define OCI_SESSGET_CREDPROXY  0x0008     // SessionGet called in proxy mode
#define OCI_SESSGET_CREDEXT    0x0010
#define OCI_SESSGET_SPOOL_MATCHANY 0x0020
// ---------------------------------------------------------------------------
// ------------------------ATTR Values for Session Pool-----------------------
// Attribute values for OCI_ATTR_SPOOL_GETMODE
#define OCI_SPOOL_ATTRVAL_WAIT     0         // block till you get a session
#define OCI_SPOOL_ATTRVAL_NOWAIT   1    // error out if no session avaliable
#define OCI_SPOOL_ATTRVAL_FORCEGET 2  // get session even if max is exceeded

// ---------------------------------------------------------------------------
// --------------------------- OCISessionRelease Modes -----------------------

#define OCI_SESSRLS_DROPSESS 0x0001                    // Drop the Session
#define OCI_SESSRLS_RETAG    0x0002                   // Retag the session

// ---------------------------------------------------------------------------
// ----------------------- OCISessionPoolDestroy Modes -----------------------

#define OCI_SPD_FORCE        0x0001       // Force the sessions to terminate. Even if there are some busy sessions close them

// ---------------------------------------------------------------------------
// ----------------------------- Statement States ----------------------------

#define OCI_STMT_STATE_INITIALIZED  0x0001
#define OCI_STMT_STATE_EXECUTED     0x0002
#define OCI_STMT_STATE_END_OF_FETCH 0x0003

// ---------------------------------------------------------------------------

// ----------------------------- OCIMemStats Modes ---------------------------
#define OCI_MEM_INIT        0x01
#define OCI_MEM_CLN         0x02
#define OCI_MEM_FLUSH       0x04
#define OCI_DUMP_HEAP       0x80

#define OCI_CLIENT_STATS    0x10
#define OCI_SERVER_STATS    0x20

// ----------------------------- OCIEnvInit Modes ----------------------------
// NOTE: NO NEW MODES SHOULD BE ADDED HERE BECAUSE THE RECOMMENDED METHOD IS TO USE THE NEW OCICreateEnvironment MODES.
//
#define OCI_ENV_NO_UCB 0x01   // A user callback will not be called in OCIEnvInit()
#define OCI_ENV_NO_MUTEX 0x08 // the environment handle will not be protected by a mutex internally

// ---------------------------------------------------------------------------

// ------------------------ Prepare Modes ------------------------------------
#define OCI_NO_SHARING        0x01 // turn off statement handle sharing
#define OCI_PREP_RESERVED_1   0x02 // reserved
#define OCI_PREP_AFC_PAD_ON   0x04 // turn on blank padding for AFC
#define OCI_PREP_AFC_PAD_OFF  0x08 // turn off blank padding for AFC
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

// ----------------------- Execution Modes -----------------------------------
#define OCI_BATCH_MODE        0x01 // batch the oci statement for execution
#define OCI_EXACT_FETCH       0x02 // fetch the exact rows specified
#define OCI_KEEP_FETCH_STATE  0x04 // unused
#define OCI_STMT_SCROLLABLE_READONLY 0x08 // if result set is scrollable
#define OCI_DESCRIBE_ONLY     0x10 // only describe the statement
#define OCI_COMMIT_ON_SUCCESS 0x20 // commit, if successful execution
#define OCI_NON_BLOCKING      0x40 // non-blocking
#define OCI_BATCH_ERRORS      0x80 // batch errors in array dmls
#define OCI_PARSE_ONLY        0x100 // only parse the statement
#define OCI_EXACT_FETCH_RESERVED_1 0x200 // reserved
#define OCI_SHOW_DML_WARNINGS 0x400 // return OCI_SUCCESS_WITH_INFO for delete/update w/no where clause
#define OCI_EXEC_RESERVED_2   0x800 // reserved
#define OCI_DESC_RESERVED_1   0x1000 // reserved

// ---------------------------------------------------------------------------

// ------------------------Authentication Modes-------------------------------
#define OCI_MIGRATE         0x0001 // migratable auth context
#define OCI_SYSDBA          0x0002 // for SYSDBA authorization
#define OCI_SYSOPER         0x0004 // for SYSOPER authorization
#define OCI_PRELIM_AUTH     0x0008 // for preliminary authorization
#define OCIP_ICACHE         0x0010 // Private OCI cache mode to notify cache
#define OCI_AUTH_RESERVED_1 0x0020 // reserved
#define OCI_STMT_CACHE      0x0040 // enable OCI Stmt Caching

// ---------------------------------------------------------------------------

// ------------------------Session End Modes----------------------------------
#define OCI_SESSEND_RESERVED_1 0x0001 // reserved
// ---------------------------------------------------------------------------

// ------------------------Attach Modes---------------------------------------
//
// The following attach modes are the same as the UPI modes defined in UPIDEF.H.  Do not use these values externally.
//
#define OCI_FASTPATH         0x0010 // Attach in fast path mode
#define OCI_ATCH_RESERVED_1  0x0020 // reserved
#define OCI_ATCH_RESERVED_2  0x0080 // reserved
#define OCI_ATCH_RESERVED_3  0x0100 // reserved
#define OCI_CPOOL            0x0200 // Attach using server handle from pool
#define OCI_ATCH_RESERVED_4  0x0400 // reserved

// ---------------------OCIStmtPrepare2 Modes---------------------------------
#define OCI_PREP2_CACHE_SEARCHONLY  0x0010 // ONly Search
// ---------------------OCIStmtRelease Modes----------------------------------
#define OCI_STRLS_CACHE_DELETE      0x0010 // Delete from Cache
// ------------------------Piece Information----------------------------------
#define OCI_PARAM_IN           0x01 // in parameter
#define OCI_PARAM_OUT          0x02 // out parameter
// ---------------------------------------------------------------------------

// ------------------------ Transaction Start Flags --------------------------
// NOTE: OCI_TRANS_JOIN and OCI_TRANS_NOMIGRATE not supported in 8.0.X
#define OCI_TRANS_NEW          0x00000001 // starts a new transaction branch
#define OCI_TRANS_JOIN         0x00000002 // join an existing transaction
#define OCI_TRANS_RESUME       0x00000004 // resume this transaction
#define OCI_TRANS_STARTMASK    0x000000ff

#define OCI_TRANS_READONLY     0x00000100 // starts a readonly transaction
#define OCI_TRANS_READWRITE    0x00000200 // starts a read-write transaction
#define OCI_TRANS_SERIALIZABLE 0x00000400 // starts a serializable transaction
#define OCI_TRANS_ISOLMASK     0x0000ff00

#define OCI_TRANS_LOOSE        0x00010000 // a loosely coupled branch
#define OCI_TRANS_TIGHT        0x00020000 // a tightly coupled branch
#define OCI_TRANS_TYPEMASK     0x000f0000

#define OCI_TRANS_NOMIGRATE    0x00100000 // non migratable transaction
#define OCI_TRANS_SEPARABLE    0x00200000 // separable transaction (8.1.6+)


// ---------------------------------------------------------------------------

// ------------------------ Transaction End Flags ----------------------------
#define OCI_TRANS_TWOPHASE      0x01000000           // use two phase commit
// ---------------------------------------------------------------------------
//
//------------------------- AQ Constants ------------------------------------
// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
// The following constants must match the PL/SQL dbms_aq constants
// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
//
// ------------------------- Visibility flags -------------------------------
#define OCI_ENQ_IMMEDIATE        1 // enqueue is an independent transaction
#define OCI_ENQ_ON_COMMIT        2 // enqueue is part of current transaction
// ----------------------- Dequeue mode flags -------------------------------
#define OCI_DEQ_BROWSE           1 // read message without acquiring a lock
#define OCI_DEQ_LOCKED           2 // read and obtain write lock on message
#define OCI_DEQ_REMOVE           3 // read the message and delete it
#define OCI_DEQ_REMOVE_NODATA    4 // delete message w'o returning payload
#define OCI_DEQ_GETSIG           5 // get signature only
// ----------------- Dequeue navigation flags -------------------------------
#define OCI_DEQ_FIRST_MSG        1 // get first message at head of queue
#define OCI_DEQ_NEXT_MSG         3 // next message that is available
#define OCI_DEQ_NEXT_TRANSACTION 2 // get first message of next txn group
// ----------------- Dequeue Option Reserved flags -------------------------
#define OCI_DEQ_RESERVED_1      0x000001
// --------------------- Message states -------------------------------------
#define OCI_MSG_WAITING          1 // the message delay has not yet completed
#define OCI_MSG_READY            0 // the message is ready to be processed
#define OCI_MSG_PROCESSED        2 // the message has been processed
#define OCI_MSG_EXPIRED          3 // message has moved to exception queue
// --------------------- Sequence deviation ---------------------------------
#define OCI_ENQ_BEFORE           2 // enqueue message before another message
#define OCI_ENQ_TOP              3 // enqueue message before all messages
// ------------------------- Visibility flags -------------------------------
#define OCI_DEQ_IMMEDIATE        1 // dequeue is an independent transaction
#define OCI_DEQ_ON_COMMIT        2 // dequeue is part of current transaction
// ------------------------ Wait --------------------------------------------
#define OCI_DEQ_WAIT_FOREVER    -1 // wait forever if no message available
#define OCI_DEQ_NO_WAIT          0 // do not wait if no message is available
// ------------------------ Delay -------------------------------------------
#define OCI_MSG_NO_DELAY         0 // message is available immediately
// ------------------------- Expiration -------------------------------------
#define OCI_MSG_NO_EXPIRATION   -1 // message will never expire

// ------------------------- Reserved ---------------------------------------
#define OCI_AQ_RESERVED_1      0x0002
#define OCI_AQ_RESERVED_2      0x0004
#define OCI_AQ_RESERVED_3      0x0008
//
// -----------------------Object Types----------------------------------------
//
// -----------Object Types **** Not to be Used **** --------------------------
// Deprecated
#define OCI_OTYPE_UNK           0
#define OCI_OTYPE_TABLE         1
#define OCI_OTYPE_VIEW          2
#define OCI_OTYPE_SYN           3
#define OCI_OTYPE_PROC          4
#define OCI_OTYPE_FUNC          5
#define OCI_OTYPE_PKG           6
#define OCI_OTYPE_STMT          7
//
// =======================Describe Handle Parameter Attributes ===============
//
// These attributes are orthogonal to the other set of attributes defined
// above.  These attrubutes are tobe used only for the desscribe handle
//
// ===========================================================================
// Attributes common to Columns and Stored Procs
#define OCI_ATTR_DATA_SIZE      1 // maximum size of the data
#define OCI_ATTR_DATA_TYPE      2 // the SQL type of the column/argument
#define OCI_ATTR_DISP_SIZE      3 // the display size
#define OCI_ATTR_NAME           4 // the name of the column/argument
#define OCI_ATTR_PRECISION      5 // precision if number type
#define OCI_ATTR_SCALE          6 // scale if number type
#define OCI_ATTR_IS_NULL        7 // is it null ?
#define OCI_ATTR_TYPE_NAME      8 // name of the named data type or a package name for package private types
#define OCI_ATTR_SCHEMA_NAME    9 // the schema name
#define OCI_ATTR_SUB_NAME       10 // type name if package private type
#define OCI_ATTR_POSITION       11
					// relative position of col/arg in the list of cols/args
// complex object retrieval parameter attributes
#define OCI_ATTR_COMPLEXOBJECTCOMP_TYPE         50
#define OCI_ATTR_COMPLEXOBJECTCOMP_TYPE_LEVEL   51
#define OCI_ATTR_COMPLEXOBJECT_LEVEL            52
#define OCI_ATTR_COMPLEXOBJECT_COLL_OUTOFLINE   53

// Only Columns
#define OCI_ATTR_DISP_NAME      100 // the display name

// Only Stored Procs
#define OCI_ATTR_OVERLOAD       210 // is this position overloaded
#define OCI_ATTR_LEVEL          211 // level for structured types
#define OCI_ATTR_HAS_DEFAULT    212 // has a default value
#define OCI_ATTR_IOMODE         213 // in, out inout
#define OCI_ATTR_RADIX          214 // returns a radix
#define OCI_ATTR_NUM_ARGS       215 // total number of arguments

// only named type attributes
#define OCI_ATTR_TYPECODE                  216 // object or collection
#define OCI_ATTR_COLLECTION_TYPECODE       217 // varray or nested table
#define OCI_ATTR_VERSION                   218 // user assigned version
#define OCI_ATTR_IS_INCOMPLETE_TYPE        219 // is this an incomplete type
#define OCI_ATTR_IS_SYSTEM_TYPE            220 // a system type
#define OCI_ATTR_IS_PREDEFINED_TYPE        221 // a predefined type
#define OCI_ATTR_IS_TRANSIENT_TYPE         222 // a transient type
#define OCI_ATTR_IS_SYSTEM_GENERATED_TYPE  223 // system generated type
#define OCI_ATTR_HAS_NESTED_TABLE          224 // contains nested table attr
#define OCI_ATTR_HAS_LOB                   225 // has a lob attribute
#define OCI_ATTR_HAS_FILE                  226 // has a file attribute
#define OCI_ATTR_COLLECTION_ELEMENT        227 // has a collection attribute
#define OCI_ATTR_NUM_TYPE_ATTRS            228 // number of attribute types
#define OCI_ATTR_LIST_TYPE_ATTRS           229 // list of type attributes
#define OCI_ATTR_NUM_TYPE_METHODS          230 // number of type methods
#define OCI_ATTR_LIST_TYPE_METHODS         231 // list of type methods
#define OCI_ATTR_MAP_METHOD                232 // map method of type
#define OCI_ATTR_ORDER_METHOD              233 // order method of type

// only collection element
#define OCI_ATTR_NUM_ELEMS                 234 // number of elements

// only type methods
#define OCI_ATTR_ENCAPSULATION             235 // encapsulation level
#define OCI_ATTR_IS_SELFISH                236 // method selfish
#define OCI_ATTR_IS_VIRTUAL                237 // virtual
#define OCI_ATTR_IS_INLINE                 238 // inline
#define OCI_ATTR_IS_CONSTANT               239 // constant
#define OCI_ATTR_HAS_RESULT                240 // has result
#define OCI_ATTR_IS_CONSTRUCTOR            241 // constructor
#define OCI_ATTR_IS_DESTRUCTOR             242 // destructor
#define OCI_ATTR_IS_OPERATOR               243 // operator
#define OCI_ATTR_IS_MAP                    244 // a map method
#define OCI_ATTR_IS_ORDER                  245 // order method
#define OCI_ATTR_IS_RNDS                   246 // read no data state method
#define OCI_ATTR_IS_RNPS                   247 // read no process state
#define OCI_ATTR_IS_WNDS                   248 // write no data state method
#define OCI_ATTR_IS_WNPS                   249 // write no process state

#define OCI_ATTR_DESC_PUBLIC               250 // public object

// Object Cache Enhancements : attributes for User Constructed Instances
#define OCI_ATTR_CACHE_CLIENT_CONTEXT      251
#define OCI_ATTR_UCI_CONSTRUCT             252
#define OCI_ATTR_UCI_DESTRUCT              253
#define OCI_ATTR_UCI_COPY                  254
#define OCI_ATTR_UCI_PICKLE                255
#define OCI_ATTR_UCI_UNPICKLE              256
#define OCI_ATTR_UCI_REFRESH               257

// for type inheritance
#define OCI_ATTR_IS_SUBTYPE                258
#define OCI_ATTR_SUPERTYPE_SCHEMA_NAME     259
#define OCI_ATTR_SUPERTYPE_NAME            260

// for schemas
#define OCI_ATTR_LIST_OBJECTS              261 // list of objects in schema

// for database
#define OCI_ATTR_NCHARSET_ID               262 // char set id
#define OCI_ATTR_LIST_SCHEMAS              263 // list of schemas
#define OCI_ATTR_MAX_PROC_LEN              264 // max procedure length
#define OCI_ATTR_MAX_COLUMN_LEN            265 // max column name length
#define OCI_ATTR_CURSOR_COMMIT_BEHAVIOR    266 // cursor commit behavior
#define OCI_ATTR_MAX_CATALOG_NAMELEN       267 // catalog namelength
#define OCI_ATTR_CATALOG_LOCATION          268 // catalog location
#define OCI_ATTR_SAVEPOINT_SUPPORT         269 // savepoint support
#define OCI_ATTR_NOWAIT_SUPPORT            270 // nowait support
#define OCI_ATTR_AUTOCOMMIT_DDL            271 // autocommit DDL
#define OCI_ATTR_LOCKING_MODE              272 // locking mode

// for externally initialized context
#define OCI_ATTR_APPCTX_SIZE               273 // count of context to be init
#define OCI_ATTR_APPCTX_LIST               274 // count of context to be init
#define OCI_ATTR_APPCTX_NAME               275 // name  of context to be init
#define OCI_ATTR_APPCTX_ATTR               276 // attr  of context to be init
#define OCI_ATTR_APPCTX_VALUE              277 // value of context to be init

// for client id propagation
#define OCI_ATTR_CLIENT_IDENTIFIER         278 // value of client id to set

// for inheritance - part 2
#define OCI_ATTR_IS_FINAL_TYPE             279 // is final type ?
#define OCI_ATTR_IS_INSTANTIABLE_TYPE      280 // is instantiable type ?
#define OCI_ATTR_IS_FINAL_METHOD           281 // is final method ?
#define OCI_ATTR_IS_INSTANTIABLE_METHOD    282 // is instantiable method ?
#define OCI_ATTR_IS_OVERRIDING_METHOD      283 // is overriding method ?

// slot 284 available

#define OCI_ATTR_CHAR_USED                 285 // char length semantics
#define OCI_ATTR_CHAR_SIZE                 286 // char length

// SQLJ support
#define OCI_ATTR_IS_JAVA_TYPE              287 // is java implemented type ?

// N-Tier support
#define OCI_ATTR_DISTINGUISHED_NAME        300 // use DN as user name
#define OCI_ATTR_KERBEROS_TICKET           301 // Kerberos ticket as cred.

// for multilanguage debugging
#define OCI_ATTR_ORA_DEBUG_JDWP            302 // ORA_DEBUG_JDWP attribute

#define OCI_ATTR_RESERVED_14               303 // reserved


// ---------------------------End Describe Handle Attributes -----------------

// ------------- Supported Values for Direct Path Date cache -----------------
#define OCI_ATTR_DIRPATH_DCACHE_NUM        303 // date cache entries
#define OCI_ATTR_DIRPATH_DCACHE_SIZE       304 // date cache limit
#define OCI_ATTR_DIRPATH_DCACHE_MISSES     305 // date cache misses
#define OCI_ATTR_DIRPATH_DCACHE_HITS       306 // date cache hits
#define OCI_ATTR_DIRPATH_DCACHE_DISABLE    307 // on set: disable datecache on overflow.
	// on get: datecache disabled?
	// could be due to overflow or others

// ----------------------- Session Pool Attributes -------------------------
#define OCI_ATTR_SPOOL_TIMEOUT              308 // session timeout
#define OCI_ATTR_SPOOL_GETMODE              309 // session get mode
#define OCI_ATTR_SPOOL_BUSY_COUNT           310 // busy session count
#define OCI_ATTR_SPOOL_OPEN_COUNT           311 // open session count
#define OCI_ATTR_SPOOL_MIN                  312 // min session count
#define OCI_ATTR_SPOOL_MAX                  313 // max session count
#define OCI_ATTR_SPOOL_INCR                 314 // session increment count
// ------------------------------End Session Pool Attributes -----------------
// ---------------------------- For XML Types -------------------------------
// For table, view and column
#define OCI_ATTR_IS_XMLTYPE          315 // Is the type an XML type?
#define OCI_ATTR_XMLSCHEMA_NAME      316 // Name of XML Schema
#define OCI_ATTR_XMLELEMENT_NAME     317 // Name of XML Element
#define OCI_ATTR_XMLSQLTYPSCH_NAME   318 // SQL type's schema for XML Ele
#define OCI_ATTR_XMLSQLTYPE_NAME     319 // Name of SQL type for XML Ele
#define OCI_ATTR_XMLTYPE_STORED_OBJ  320 // XML type stored as object?
//
// ---------------------------- For Subtypes -------------------------------
// For type
#define OCI_ATTR_HAS_SUBTYPES        321 // Has subtypes?
#define OCI_ATTR_NUM_SUBTYPES        322 // Number of subtypes
#define OCI_ATTR_LIST_SUBTYPES       323 // List of subtypes
// XML flag
#define OCI_ATTR_XML_HRCHY_ENABLED   324 // hierarchy enabled?
// Method flag
#define OCI_ATTR_IS_OVERRIDDEN_METHOD 325 // Method is overridden?
//
// ---------------- Describe Handle Parameter Attribute Values ---------------
//
// OCI_ATTR_CURSOR_COMMIT_BEHAVIOR
#define OCI_CURSOR_OPEN   0
#define OCI_CURSOR_CLOSED 1

// OCI_ATTR_CATALOG_LOCATION
#define OCI_CL_START 0
#define OCI_CL_END   1

// OCI_ATTR_SAVEPOINT_SUPPORT
#define OCI_SP_SUPPORTED   0
#define OCI_SP_UNSUPPORTED 1

// OCI_ATTR_NOWAIT_SUPPORT
#define OCI_NW_SUPPORTED   0
#define OCI_NW_UNSUPPORTED 1

// OCI_ATTR_AUTOCOMMIT_DDL
#define OCI_AC_DDL    0
#define OCI_NO_AC_DDL 1

// OCI_ATTR_LOCKING_MODE
#define OCI_LOCK_IMMEDIATE 0
#define OCI_LOCK_DELAYED   1
//
// ---------------------------OCIPasswordChange-------------------------------
//
#define OCI_AUTH                0x08 // Change the password but do not login
//
// ------------------------Other Constants------------------------------------
//
#define OCI_MAX_FNS              100 // max number of OCI Functions
#define OCI_SQLSTATE_SIZE          5
#define OCI_ERROR_MAXMSG_SIZE   1024 // max size of an error message
#define OCI_LOBMAXSIZE          MINUB4MAXVAL // maximum lob data size
#define OCI_ROWID_LEN             23
//
// ------------------------ Fail Over Events ---------------------------------
//
#define OCI_FO_END          0x00000001
#define OCI_FO_ABORT        0x00000002
#define OCI_FO_REAUTH       0x00000004
#define OCI_FO_BEGIN        0x00000008
#define OCI_FO_ERROR        0x00000010
//
// ------------------------ Fail Over Callback Return Codes ------------------
//
#define OCI_FO_RETRY        25410
//
// ------------------------- Fail Over Types ---------------------------------
//
#define OCI_FO_NONE           0x00000001
#define OCI_FO_SESSION        0x00000002
#define OCI_FO_SELECT         0x00000004
#define OCI_FO_TXNAL          0x00000008
//
// -----------------------Function Codes--------------------------------------
//
#define OCI_FNCODE_INITIALIZE           1 // OCIInitialize
#define OCI_FNCODE_HANDLEALLOC          2 // OCIHandleAlloc
#define OCI_FNCODE_HANDLEFREE           3 // OCIHandleFree
#define OCI_FNCODE_DESCRIPTORALLOC      4 // OCIDescriptorAlloc
#define OCI_FNCODE_DESCRIPTORFREE       5 // OCIDescriptorFree
#define OCI_FNCODE_ENVINIT              6 // OCIEnvInit
#define OCI_FNCODE_SERVERATTACH         7 // OCIServerAttach
#define OCI_FNCODE_SERVERDETACH         8 // OCIServerDetach
// unused 9
#define OCI_FNCODE_SESSIONBEGIN        10 // OCISessionBegin
#define OCI_FNCODE_SESSIONEND          11 // OCISessionEnd
#define OCI_FNCODE_PASSWORDCHANGE      12 // OCIPasswordChange
#define OCI_FNCODE_STMTPREPARE         13 // OCIStmtPrepare
// unused 14..16
#define OCI_FNCODE_BINDDYNAMIC         17 // OCIBindDynamic
#define OCI_FNCODE_BINDOBJECT          18 // OCIBindObject
// unused 19
#define OCI_FNCODE_BINDARRAYOFSTRUCT   20 // OCIBindArrayOfStruct
#define OCI_FNCODE_STMTEXECUTE         21 // OCIStmtExecute
// unused 22..24
#define OCI_FNCODE_DEFINEOBJECT        25 // OCIDefineObject
#define OCI_FNCODE_DEFINEDYNAMIC       26 // OCIDefineDynamic
#define OCI_FNCODE_DEFINEARRAYOFSTRUCT 27 // OCIDefineArrayOfStruct
#define OCI_FNCODE_STMTFETCH           28 // OCIStmtFetch
#define OCI_FNCODE_STMTGETBIND         29 // OCIStmtGetBindInfo
// unused 30, 31
#define OCI_FNCODE_DESCRIBEANY         32 // OCIDescribeAny
#define OCI_FNCODE_TRANSSTART          33 // OCITransStart
#define OCI_FNCODE_TRANSDETACH         34 // OCITransDetach
#define OCI_FNCODE_TRANSCOMMIT         35 // OCITransCommit
// unused 36
#define OCI_FNCODE_ERRORGET            37 // OCIErrorGet
#define OCI_FNCODE_LOBOPENFILE         38 // OCILobFileOpen
#define OCI_FNCODE_LOBCLOSEFILE        39 // OCILobFileClose
// was LOBCREATEFILE, unused 40
// was OCILobFileDelete, unused 41
#define OCI_FNCODE_LOBCOPY             42 // OCILobCopy
#define OCI_FNCODE_LOBAPPEND           43 // OCILobAppend
#define OCI_FNCODE_LOBERASE            44 // OCILobErase
#define OCI_FNCODE_LOBLENGTH           45 // OCILobGetLength
#define OCI_FNCODE_LOBTRIM             46 // OCILobTrim
#define OCI_FNCODE_LOBREAD             47 // OCILobRead
#define OCI_FNCODE_LOBWRITE            48 // OCILobWrite
// unused 49
#define OCI_FNCODE_SVCCTXBREAK         50 // OCIBreak
#define OCI_FNCODE_SERVERVERSION       51 // OCIServerVersion
// unused 52, 53
#define OCI_FNCODE_ATTRGET             54 // OCIAttrGet
#define OCI_FNCODE_ATTRSET             55 // OCIAttrSet
#define OCI_FNCODE_PARAMSET            56 // OCIParamSet
#define OCI_FNCODE_PARAMGET            57 // OCIParamGet
#define OCI_FNCODE_STMTGETPIECEINFO    58 // OCIStmtGetPieceInfo
#define OCI_FNCODE_LDATOSVCCTX         59 // OCILdaToSvcCtx
// unused 60
#define OCI_FNCODE_STMTSETPIECEINFO    61 // OCIStmtSetPieceInfo
#define OCI_FNCODE_TRANSFORGET         62 // OCITransForget
#define OCI_FNCODE_TRANSPREPARE        63 // OCITransPrepare
#define OCI_FNCODE_TRANSROLLBACK       64 // OCITransRollback
#define OCI_FNCODE_DEFINEBYPOS         65 // OCIDefineByPos
#define OCI_FNCODE_BINDBYPOS           66 // OCIBindByPos
#define OCI_FNCODE_BINDBYNAME          67 // OCIBindByName
#define OCI_FNCODE_LOBASSIGN           68 // OCILobAssign
#define OCI_FNCODE_LOBISEQUAL          69 // OCILobIsEqual
#define OCI_FNCODE_LOBISINIT           70 // OCILobLocatorIsInit
// 71 was lob locator size in beta2
#define OCI_FNCODE_LOBENABLEBUFFERING  71 // OCILobEnableBuffering
#define OCI_FNCODE_LOBCHARSETID        72 // OCILobCharSetID
#define OCI_FNCODE_LOBCHARSETFORM      73 // OCILobCharSetForm
#define OCI_FNCODE_LOBFILESETNAME      74 // OCILobFileSetName
#define OCI_FNCODE_LOBFILEGETNAME      75 // OCILobFileGetName
#define OCI_FNCODE_LOGON               76 // OCILogon
#define OCI_FNCODE_LOGOFF              77 // OCILogoff
#define OCI_FNCODE_LOBDISABLEBUFFERING 78 // OCILobDisableBuffering
#define OCI_FNCODE_LOBFLUSHBUFFER      79 // OCILobFlushBuffer
#define OCI_FNCODE_LOBLOADFROMFILE     80 // OCILobLoadFromFile

#define OCI_FNCODE_LOBOPEN             81 // OCILobOpen
#define OCI_FNCODE_LOBCLOSE            82 // OCILobClose
#define OCI_FNCODE_LOBISOPEN           83 // OCILobIsOpen
#define OCI_FNCODE_LOBFILEISOPEN       84 // OCILobFileIsOpen
#define OCI_FNCODE_LOBFILEEXISTS       85 // OCILobFileExists
#define OCI_FNCODE_LOBFILECLOSEALL     86 // OCILobFileCloseAll
#define OCI_FNCODE_LOBCREATETEMP       87 // OCILobCreateTemporary
#define OCI_FNCODE_LOBFREETEMP         88 // OCILobFreeTemporary
#define OCI_FNCODE_LOBISTEMP           89 // OCILobIsTemporary

#define OCI_FNCODE_AQENQ               90 // OCIAQEnq
#define OCI_FNCODE_AQDEQ               91 // OCIAQDeq
#define OCI_FNCODE_RESET               92 // OCIReset
#define OCI_FNCODE_SVCCTXTOLDA         93 // OCISvcCtxToLda
#define OCI_FNCODE_LOBLOCATORASSIGN    94 // OCILobLocatorAssign
#define OCI_FNCODE_UBINDBYNAME         95
#define OCI_FNCODE_AQLISTEN            96 // OCIAQListen
#define OCI_FNCODE_SVC2HST             97 // reserved
#define OCI_FNCODE_SVCRH               98 // reserved
						 // 97 and 98 are reserved for Oracle internal use

#define OCI_FNCODE_TRANSMULTIPREPARE   99 // OCITransMultiPrepare

#define OCI_FNCODE_CPOOLCREATE        100 // OCIConnectionPoolCreate
#define OCI_FNCODE_CPOOLDESTROY       101 // OCIConnectionPoolDestroy
#define OCI_FNCODE_LOGON2             102 // OCILogon2
#define OCI_FNCODE_ROWIDTOCHAR        103 // OCIRowidToChar

#define OCI_FNCODE_SPOOLCREATE        104 // OCISessionPoolCreate
#define OCI_FNCODE_SPOOLDESTROY       105 // OCISessionPoolDestroy
#define OCI_FNCODE_SESSIONGET         106 // OCISessionGet
#define OCI_FNCODE_SESSIONRELEASE     107 // OCISessionRelease
#define OCI_FNCODE_STMTPREPARE2       108 // OCIStmtPrepare2
#define OCI_FNCODE_STMTRELEASE        109 // OCIStmtRelease
#define OCI_FNCODE_MAXFCN             109 // maximum OCI function code
//
// OCI_*_PIECE defines the piece types that are returned or set
//
#define OCI_ONE_PIECE                   0 // there or this is the only piece
#define OCI_FIRST_PIECE                 1 // the first of many pieces
#define OCI_NEXT_PIECE                  2 // the next of many pieces
#define OCI_LAST_PIECE                  3 // the last piece of this column
//
// SQLT_XXX Constant
//
// input data types
//
#define SQLT_CHR                        1 // (ORANET TYPE) character string
#define SQLT_NUM                        2 // (ORANET TYPE) oracle numeric
#define SQLT_INT                        3 // (ORANET TYPE) integer
#define SQLT_FLT                        4 // (ORANET TYPE) Floating point number
#define SQLT_STR                        5 // zero terminated string
#define SQLT_VNU                        6 // NUM with preceding length byte
#define SQLT_PDN                        7 // (ORANET TYPE) Packed Decimal Numeric
#define SQLT_LNG                        8 // long
#define SQLT_VCS                        9 // Variable character string
#define SQLT_NON                       10 // Null/empty PCC Descriptor entry
#define SQLT_RID                       11 // rowid
#define SQLT_DAT                       12 // date in oracle format
#define SQLT_VBI                       15 // binary in VCS format
#define SQLT_BIN                       23 // binary data(DTYBIN)
#define SQLT_LBI                       24 // long binary
#define SQLT_UIN                       68 // unsigned integer
#define SQLT_SLS                       91 // Display sign leading separate
#define SQLT_LVC                       94 // Longer longs (char)
#define SQLT_LVB                       95 // Longer long binary
#define SQLT_AFC                       96 // Ansi fixed char
#define SQLT_AVC                       97 // Ansi Var char
#define SQLT_CUR                      102 // cursor  type
#define SQLT_RDD                      104 // rowid descriptor
#define SQLT_LAB                      105 // label type
#define SQLT_OSL                      106 // oslabel type

#define SQLT_NTY                      108 // named object type
#define SQLT_REF                      110 // ref type
#define SQLT_CLOB                     112 // character lob
#define SQLT_BLOB                     113 // binary lob
#define SQLT_BFILEE                   114 // binary file lob
#define SQLT_CFILEE                   115 // character file lob
#define SQLT_RSET                     116 // result set type
#define SQLT_NCO                      122 // named collection type (varray or nested table)
#define SQLT_VST                      155 // OCIString type
#define SQLT_ODT                      156 // OCIDate type
//
// datetimes and intervals
//
#define SQLT_DATE                     184 // ANSI Date
#define SQLT_TIME                     185 // TIME
#define SQLT_TIME_TZ                  186 // TIME WITH TIME ZONE
#define SQLT_TIMESTAMP                187 // TIMESTAMP
#define SQLT_TIMESTAMP_TZ             188 // TIMESTAMP WITH TIME ZONE
#define SQLT_INTERVAL_YM              189 // INTERVAL YEAR TO MONTH
#define SQLT_INTERVAL_DS              190 // INTERVAL DAY TO SECOND
#define SQLT_TIMESTAMP_LTZ            232 // TIMESTAMP WITH LOCAL TZ

#define SQLT_PNTY   241 // pl/sql representation of named types
//
// cxcheng: this has been added for backward compatibility -
// it needs to be here because ocidfn.h can get included ahead of sqldef.h
//
#define SQLT_FILE SQLT_BFILEE                              // binary file lob
#define SQLT_CFILE SQLT_CFILEE
#define SQLT_BFILE SQLT_BFILEE
//
// CHAR/NCHAR/VARCHAR2/NVARCHAR2/CLOB/NCLOB char set "form" information
//
#define SQLCS_IMPLICIT                  1 // for CHAR, VARCHAR2, CLOB w/o a specified set
#define SQLCS_NCHAR                     2 // for NCHAR, NCHAR VARYING, NCLOB
#define SQLCS_EXPLICIT                  3 // for CHAR, etc, with "CHARACTER SET ..." syntax
#define SQLCS_FLEXIBLE                  4 // for PL/SQL "flexible" parameters
#define SQLCS_LIT_NULL                  5 // for typecheck of NULL and empty_clob() lits
//
// Handle Definitions
//
typedef struct OCIEnv           OCIEnv;         // OCI environment handle
typedef struct OCIError         OCIError;       // OCI error handle
typedef struct OCISvcCtx        OCISvcCtx;      // OCI service handle
typedef struct OCIStmt          OCIStmt;        // OCI statement handle
typedef struct OCIBind          OCIBind;        // OCI bind handle
typedef struct OCIDefine        OCIDefine;      // OCI Define handle
typedef struct OCIDescribe      OCIDescribe;    // OCI Describe handle
typedef struct OCIServer        OCIServer;      // OCI Server handle
typedef struct OCISession       OCISession;     // OCI Authentication handle
typedef struct OCITrans         OCITrans;       // OCI Transaction handle
//
// Descriptor Definitions
//
typedef struct OCISnapshot      OCISnapshot;    // OCI snapshot descriptor
typedef struct OCIResult        OCIResult;      // OCI Result Set Descriptor
typedef struct OCILobLocator    OCILobLocator;  // OCI Lob Locator descriptor
typedef struct OCIParam         OCIParam;       // OCI PARameter descriptor
typedef struct OCIRowid         OCIRowid;       // OCI ROWID descriptor
typedef struct OCIDateTime      OCIDateTime;    // OCI DateTime descriptor
typedef struct OCIInterval      OCIInterval;    // OCI Interval descriptor
//
// Objects Definitions
//
typedef struct OCIString        OCIString;
typedef struct OCIRaw           OCIRaw;
//
//
//
struct OCITime {
	uint8  /*OCITimeHH*/H;      // hours; range is 0 <= hours <=23
	uint8  /*OCITimeMI*/M;      // minutes; range is 0 <= minutes <= 59
	uint8  /*OCITimeSS*/S;      // seconds; range is 0 <= seconds <= 59
};
typedef struct OCITime OCITime;

struct OCIDate {
	int16  /*OCIDateYYYY*/Y;    // gregorian year; range is -4712 <= year <= 9999
	uint8  /*OCIDateMM*/M;      // month; range is 1 <= month < 12
	uint8  /*OCIDateDD*/D;      // day; range is 1 <= day <= 31
	OCITime /*OCIDateTime*/Tm;   // time
};
typedef struct OCIDate OCIDate;
//
//
//
typedef uchar  oratext;
typedef uchar  text;
typedef uchar  OraText;
typedef uint16 OCIDuration;
//
// oci api protoptypes
//
typedef void * (*OciMallocProc)(void *ctxp, size_t size);
typedef void * (*OciReallocProc)(void *ctxp, void *memptr, size_t newsize);
typedef void   (*OciFreeProc)(void *ctxp, void *memptr);
typedef int32  (*OciLobReadProc)(void *ctxp, const void *bufp, uint32 len, uint8 piece);
typedef int32  (*OciLobWriteProc)(void *ctxp, void *bufp, uint32 *len, uint8 *piece);

typedef int (*OCIENVCREATE)(OCIEnv **envhpp, uint32 mode, void *ctxp, OciMallocProc, OciReallocProc, OciFreeProc, size_t xtramem_sz, void **usrmempp);
typedef int (*OCIHANDLEALLOC)(const void *parenth, void **hndlpp, const uint32 type, const size_t xtramem_sz, void **usrmempp);
typedef int (*OCIHANDLEFREE)(void *hndlp, const uint32 type);
typedef int (*OCIDESCRIPTORALLOC)(const void *parenth, void **descpp, const uint32 type, const size_t xtramem_sz, void **usrmempp);
typedef int (*OCIDESCRIPTORFREE)(void *descp, const uint32 type);
typedef int (*OCIENVINIT)(OCIEnv **envp, uint32 mode, size_t xtramem_sz, void **usrmempp);
typedef int (*OCISERVERATTACH)(OCIServer *srvhp, OCIError *, const OraText *dblink, int32 dblink_len, uint32 mode);
typedef int (*OCISERVERDETACH)(OCIServer *srvhp, OCIError *, uint32 mode);
typedef int (*OCISESSIONBEGIN)(OCISvcCtx *, OCIError *, OCISession *usrhp, uint32 credt, uint32 mode);
typedef int (*OCISESSIONEND)(OCISvcCtx *, OCIError *, OCISession *usrhp, uint32 mode);
typedef int (*OCISTMTPREPARE)(OCIStmt *, OCIError *, const OraText *stmt, uint32 stmt_len, uint32 language, uint32 mode);
typedef int (*OCIBINDBYPOS)(OCIStmt *, OCIBind **bindp, OCIError *, uint32 position, void *valuep, int32 value_sz, uint16 dty, void *indp, uint16 *alenp, uint16 *rcodep, uint32 maxarr_len, uint32 *curelep, uint32 mode);
typedef int (*OCIBINDBYNAME)(OCIStmt *, OCIBind **bindp, OCIError *, const OraText *placeholder, int32 placeh_len, void *valuep, int32 value_sz, uint16 dty, void *indp, uint16 *alenp, uint16 *rcodep, uint32 maxarr_len, uint32 *curelep, uint32 mode);
typedef int (*OCIBINDARRAYOFSTRUCT)(OCIBind *, OCIError *, uint32 pvskip, uint32 indskip, uint32 alskip, uint32 rcskip);
typedef int (*OCISTMTGETPIECEINFO)(OCIStmt *, OCIError *, void **hndlpp, uint32 *typep, uint8 *in_outp, uint32 *iterp, uint32 *idxp, uint8 *piecep);
typedef int (*OCISTMTSETPIECEINFO)(void *hndlp, uint32 type, OCIError *, const void *bufp, uint32 *alenp, uint8 piece, const void *indp, uint16 *rcodep);
typedef int (*OCISTMTEXECUTE)(OCISvcCtx *, OCIStmt *, OCIError *, uint32 iters, uint32 rowoff, const OCISnapshot *snap_in, OCISnapshot *snap_out, uint32 mode);
typedef int (*OCIDEFINEBYPOS)(OCIStmt *, OCIDefine **defnp, OCIError *, uint32 position, void *valuep, int32 value_sz, uint16 dty, void *indp, uint16 *rlenp, uint16 *rcodep, uint32 mode);
typedef int (*OCIDEFINEARRAYOFSTRUCT)(OCIDefine *, OCIError *, uint32 pvskip, uint32 indskip, uint32 rlskip, uint32 rcskip);
typedef int (*OCISTMTFETCH )(OCIStmt *, OCIError *, uint32 nrows, uint16 orientation, uint32 mode);
typedef int (*OCISTMTFETCH2)(OCIStmt *, OCIError *, uint32 nrows, uint16 orientation, int32 fetchOffset, uint32 mode);
typedef int (*OCIPARAMGET)(const void *hndlp, uint32 htype, OCIError *, void **parmdpp, uint32 pos);
typedef int (*OCIPARAMSET)(void *, uint32 htyp, OCIError *, const void *dscp, uint32 dtyp, uint32 pos);
typedef int (*OCITRANSSTART)(OCISvcCtx *, OCIError *, uint timeout, uint32 flags);
typedef int (*OCITRANSDETACH)(OCISvcCtx *, OCIError *, uint32 flags);
typedef int (*OCITRANSPREPARE)(OCISvcCtx *, OCIError *, uint32 flags);
typedef int (*OCITRANSFORGET)(OCISvcCtx *, OCIError *, uint32 flags);
typedef int (*OCITRANSCOMMIT)(OCISvcCtx *, OCIError *, uint32 flags);
typedef int (*OCITRANSROLLBACK)(OCISvcCtx *, OCIError *, uint32 flags);
typedef int (*OCIERRORGET)(void *hndlp, uint32 recordno, OraText *sqlstate, int32 *errcodep, OraText *bufp, uint32 bufsiz, uint32 type);
typedef int (*OCILOBCREATETEMPORARY)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint16 csid, uint8 csfrm, uint8 lobtype, boolean cache, OCIDuration duration);
typedef int (*OCILOBFREETEMPORARY)(OCISvcCtx *, OCIError *, OCILobLocator *locp);
typedef int (*OCILOBISTEMPORARY)(OCIEnv *envp, OCIError *, OCILobLocator *locp, boolean *is_temporary);
typedef int (*OCILOBAPPEND)(OCISvcCtx *, OCIError *, OCILobLocator *dst_locp, OCILobLocator *src_locp);
typedef int (*OCILOBCOPY)(OCISvcCtx *, OCIError *, OCILobLocator *dst_locp, OCILobLocator *src_locp, uint32 amount, uint32 dst_offset, uint32 src_offset);
typedef int (*OCILOBREAD)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint32 *amtp, uint32 offset, void *bufp, uint32 bufl, void *ctxp, OciLobReadProc, uint16 csid, uint8 csfrm);
typedef int (*OCILOBTRIM)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint32 newlen);
typedef int (*OCILOBERASE)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint32 *amount, uint32 offset);
typedef int (*OCILOBWRITE)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint32 *amtp, uint32 offset, void *bufp, uint32 buflen, uint8 piece, void *ctxp, OciLobWriteProc, uint16 csid, uint8 csfrm);
typedef int (*OCILOBGETLENGTH)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint32 *lenp);
typedef int (*OCILOBOPEN)(OCISvcCtx *, OCIError *, OCILobLocator *locp, uint8 mode);
typedef int (*OCILOBCLOSE)(OCISvcCtx *, OCIError *, OCILobLocator *locp);
typedef int (*OCILOBGETCHUNKSIZE)(OCISvcCtx *, OCIError *, OCILobLocator *, uint32 *chunk_size);
typedef int (*OCILOBFILEOPEN)(OCISvcCtx *, OCIError *, OCILobLocator *filep, uint8 mode);
typedef int (*OCILOBFILECLOSE)(OCISvcCtx *, OCIError *, OCILobLocator *filep);
typedef int (*OCILOBFILECLOSEALL)(OCISvcCtx *, OCIError *);
typedef int (*OCILOBFILEISOPEN)(OCISvcCtx *, OCIError *, OCILobLocator *filep, boolean *flag);
typedef int (*OCILOBFILEEXISTS)(OCISvcCtx *, OCIError *, OCILobLocator *filep, boolean *flag);
typedef int (*OCILOBFIELGETNAME)(OCIEnv *, OCIError *, const OCILobLocator *filep, OraText *dir_alias, uint16 *d_length, OraText *filename, uint16 *f_length);
typedef int (*OCILOBFILESETNAME)(OCIEnv *, OCIError *, OCILobLocator **filepp, const OraText *dir_alias, uint16 d_length, const OraText *filename, uint16 f_length);
typedef int (*OCILOBLOADFROMFILE)(OCISvcCtx *, OCIError *, OCILobLocator *dst_locp, OCILobLocator *src_filep, uint32 amount, uint32 dst_offset, uint32 src_offset);
typedef int (*OCILOBWRITEAPPEND)(OCISvcCtx *, OCIError *, OCILobLocator *lobp, uint32 *amtp, void *bufp, uint32 bufl, uint8 piece, void *ctxp, OciLobWriteProc, uint16 csid, uint8 csfrm);
typedef int (*OCILOBISEQUAL)(OCIEnv *, const OCILobLocator *x, const OCILobLocator *y, boolean *is_equal);
typedef int (*OCILOBASSIGN)(OCIEnv *, OCIError *, const OCILobLocator *src_locp, OCILobLocator **dst_locpp);
typedef int (*OCISERVERVERSION)(void *hndlp, OCIError *, OraText *bufp, uint32 bufsz, uint8 hndltype);
typedef int (*OCIATTRGET)(const void *trgthndlp, uint32 trghndltyp, void *attributep, uint32 *sizep, uint32 attrtype, OCIError *);
typedef int (*OCIATTRSET)(void *trgthndlp, uint32 trghndltyp, void *attributep, uint32 size, uint32 attrtype, OCIError *);
typedef int (*OCIDATEASSIGN)(OCIError *, const OCIDate *from, OCIDate *to);
typedef int (*OCIDATETOTEXT)(OCIError *, const OCIDate *date, const text *fmt, uint8 fmt_length, const text *lang_name, uint32 lang_length, uint32 *buf_size, text *buf);
typedef int (*OCIDATEFROMTEXT)(OCIError *, const text *date_str, uint32 d_str_length, const text *fmt, uint8 fmt_length, const text *lang_name, uint32 lang_length, OCIDate *date);
typedef int (*OCIDATECOMPARE)(OCIError *, const OCIDate *date1, const OCIDate *date2, int *result);
typedef int (*OCIDATEADDMONTHS)(OCIError *, const OCIDate *date, int32 num_months, OCIDate *result);
typedef int (*OCIDATEADDDAYS)(OCIError *, const OCIDate *date, int32 num_days, OCIDate *result);
typedef int (*OCIDATELASTDAY)(OCIError *, const OCIDate *date, OCIDate *last_day);
typedef int (*OCIDATEDAYSBETWEEN)(OCIError *, const OCIDate *date1, const OCIDate *date2, int32 *num_days);
typedef int (*OCIDATEZONETOZONE)(OCIError *, const OCIDate *date1, const text *zon1, uint32 zon1_length, const text *zon2, uint32 zon2_length, OCIDate *date2);
typedef int (*OCIDATENEXTDAY)(OCIError *, const OCIDate *date, const text *day_p, uint32 day_length, OCIDate *next_day);
typedef int (*OCIDATECHECK)(OCIError *, const OCIDate *date, uint *valid);
typedef int (*OCIDATESYSDATE)(OCIError *, OCIDate *sys_date);
typedef int (*OCIDESCRIBEANY)(OCISvcCtx *, OCIError *, void *objptr, uint32 objnm_len, uint8 objptr_typ, uint8 info_level, uint8 objtyp, OCIDescribe *dschp);
typedef int (*OCIINTERVALASSIGN)(void *, OCIError *, const OCIInterval *inpinter, OCIInterval *outinter);
typedef int (*OCIINTERVALCHECK)(void *, OCIError *, const OCIInterval *interval, uint32 *valid);
typedef int (*OCIINTERVALCOMPARE)(void *, OCIError *, OCIInterval *inter1, OCIInterval *inter2, int *result);
typedef int (*OCIINTERVALTOTEXT)(void *, OCIError *, const OCIInterval *, uint8 lfprec, uint8 fsprec, OraText *buffer, size_t buflen, size_t *resultlen);
typedef int (*OCIINTERVALFROMTEXT)(void *, OCIError *, const OraText *inpstring, size_t str_len, OCIInterval *result);
typedef int (*OCIINTERVALFROMTZ)(void *, OCIError *, const oratext *inpstring, size_t str_len, OCIInterval *result);
typedef int (*OCIINTERVALGETDAYSECOND)(void *, OCIError *, int32 *dy, int32 *hr, int32 *mm, int32 *ss, int32 *fsec, const OCIInterval *);
typedef int (*OCIINTERVALGETYEARMONTH)(void *, OCIError *, int32 *yr, int32 *mnth, const OCIInterval *interval);
typedef int (*OCIINTERVALSETDAYSECOND)(void *, OCIError *, int32 dy, int32 hr, int32 mm, int32 ss, int32 fsec, OCIInterval *result);
typedef int (*OCIINTERVALSETYEARMONTH)(void *, OCIError *, int32 yr, int32 mnth, OCIInterval *result);
typedef int (*OCIINTERVALADD)(void *, OCIError *, OCIInterval *addend1, OCIInterval *addend2, OCIInterval *result);
typedef int (*OCIINTERVALSUBTRACT)(void *, OCIError *, OCIInterval *minuend, OCIInterval *subtrahend, OCIInterval *result);
typedef int (*OCIDATETIMEASSIGN)(void *, OCIError *, const OCIDateTime *from, OCIDateTime *to);
typedef int (*OCIDATETIMECHECK)(void *, OCIError *, const OCIDateTime *date, uint32 *valid);
typedef int (*OCIDATETIMECOMPARE)(void *, OCIError *, const OCIDateTime *date1, const OCIDateTime *date2, int *result);
typedef int (*OCIDATETIMECONSTRUCT)(void *, OCIError *, OCIDateTime *datetime, int16 year, uint8 month, uint8 day, uint8 hour, uint8 min, uint8 sec, uint32 fsec, OraText *timezone, size_t timezone_length);
typedef int (*OCIDATETIMECONVERT)(void *, OCIError *, OCIDateTime *indate, OCIDateTime *outdate);
typedef int (*OCIDATETIMEFROMARRAY)(void *, OCIError *, const uint8 *inarray, uint32 *len, uint8 type, OCIDateTime *datetime, const OCIInterval *reftz, uint8 fsprec);
typedef int (*OCIDATETIMETOARRAY)(void *, OCIError *, const OCIDateTime *datetime, const OCIInterval *reftz, uint8 *outarray, uint32 *len, uint8 fsprec);
typedef int (*OCIDATETIMEFROMTEXT)(void *, OCIError *, const OraText *date_str, size_t dstr_length, const OraText *fmt, uint8 fmt_length, const OraText *lang_name, size_t lang_length, OCIDateTime *datetime);
typedef int (*OCIDATETIMETOTEXT)(void *, OCIError *, const OCIDateTime *date, const OraText *fmt, uint8 fmt_length, uint8 fsprec, const OraText *lang_name, size_t lang_length, uint32 *buf_size, OraText *buf);
typedef int (*OCIDATETIMEGETDATE)(void *, OCIError *, const OCIDateTime *datetime, int16 *year, uint8 *month, uint8 *day);
typedef int (*OCIDATETIMEGETTIME)(void *, OCIError *, OCIDateTime *datetime, uint8 *hour, uint8 *min, uint8 *sec, uint32 *fsec);
typedef int (*OCIDATETIMEGETTIMEZONENAME)(void *, OCIError *, const OCIDateTime *datetime, uint8 *buf, uint32 *buflen);
typedef int (*OCIDATETIMEGETTIMEZONEOFFSET)(void *, OCIError *, const OCIDateTime *datetime, int8 *hour, int8 *min);
typedef int (*OCIDATETIMEINTERVALADD)(void *, OCIError *, OCIDateTime *datetime, OCIInterval *inter, OCIDateTime *outdatetime);
typedef int (*OCIDATETIMEINTERVALSUB)(void *, OCIError *, OCIDateTime *datetime, OCIInterval *inter, OCIDateTime *outdatetime);
typedef int (*OCIDATETIMESUBTRACT)(void *, OCIError *, OCIDateTime *indate1, OCIDateTime *indate2, OCIInterval *inter);
typedef int (*OCIDATETIMESYSTIMESTAMP)(void *, OCIError *, OCIDateTime *sys_date);
/* Oracle 10g test */
typedef void (*OCICLIENTVERSION)(int *major_version, int *minor_version, int *update_num, int *patch_num, int *port_update_num);
/* Oracle 11g test */
typedef int (*OCIARRAYDESCRIPTORFREE)(void **descp, const uint32 type);

typedef oratext * (*OCISTRINGPTR)(OCIEnv *, const OCIString *vs);
typedef int   (*OCISTRINGASSIGNTEXT)(OCIEnv *, OCIError *, const oratext *rhs, uint32 rhs_len, OCIString **lhs);
typedef uint8 * (*OCIRAWPTR)(OCIEnv *, CONST OCIRaw *);
typedef int (*OCIRAWASSIGNBYTES)(OCIEnv *, OCIError *, const uint8 *rhs, uint32 rhs_len, OCIRaw **lhs);
typedef int (*OCIRAWALLOCSIZE)(OCIEnv *, OCIError *, const OCIRaw *raw, uint32 *allocsize);
typedef int (*OCIRAWRESIZE)(OCIEnv *, OCIError *, uint16 new_size, OCIRaw **raw);
typedef int (*OCIROWIDTOCHAR)(OCIRowid *, OraText *outbfp, uint16 *outbflp, OCIError *);

class Ocif {
public:
	static int Load();
	static int Release();

	static OCIENVCREATE                 OCIEnvCreate;
	static OCISERVERATTACH              OCIServerAttach;
	static OCISERVERDETACH              OCIServerDetach;
	static OCIHANDLEALLOC               OCIHandleAlloc;
	static OCIHANDLEFREE                OCIHandleFree;
	static OCIDESCRIPTORALLOC           OCIDescriptorAlloc;
	static OCIDESCRIPTORFREE            OCIDescriptorFree;
	static OCISESSIONBEGIN              OCISessionBegin;
	static OCISESSIONEND                OCISessionEnd;
	static OCIBINDBYPOS                 OCIBindByPos;
	static OCIBINDBYNAME                OCIBindByName;
	static OCIBINDARRAYOFSTRUCT         OCIBindArrayOfStruct;
	static OCIDEFINEBYPOS               OCIDefineByPos;
	static OCIDEFINEARRAYOFSTRUCT       OCIDefineArrayOfStruct;
	static OCISTMTPREPARE               OCIStmtPrepare;
	static OCISTMTEXECUTE               OCIStmtExecute;
	static OCISTMTFETCH                 OCIStmtFetch;
	static OCISTMTFETCH2                OCIStmtFetch2;
	static OCISTMTGETPIECEINFO          OCIStmtGetPieceInfo;
	static OCISTMTSETPIECEINFO          OCIStmtSetPieceInfo;
	static OCIPARAMGET                  OCIParamGet;
	static OCIPARAMSET                  OCIParamSet;
	static OCITRANSSTART                OCITransStart;
	static OCITRANSDETACH               OCITransDetach;
	static OCITRANSPREPARE              OCITransPrepare;
	static OCITRANSFORGET               OCITransForget;
	static OCITRANSCOMMIT               OCITransCommit;
	static OCITRANSROLLBACK             OCITransRollback;
	static OCIERRORGET                  OCIErrorGet;
	static OCILOBCREATETEMPORARY        OCILobCreateTemporary;
	static OCILOBFREETEMPORARY          OCILobFreeTemporary;
	static OCILOBISTEMPORARY            OCILobIsTemporary;
	static OCILOBAPPEND                 OCILobAppend;
	static OCILOBCOPY                   OCILobCopy;
	static OCILOBGETLENGTH              OCILobGetLength;
	static OCILOBREAD                   OCILobRead;
	static OCILOBWRITE                  OCILobWrite;
	static OCILOBTRIM                   OCILobTrim;
	static OCILOBERASE                  OCILobErase;
	static OCILOBOPEN                   OCILobOpen;
	static OCILOBCLOSE                  OCILobClose;
	static OCILOBGETCHUNKSIZE           OCILobGetChunkSize; // @v6.1.5
	static OCILOBFILEOPEN               OCILobFileOpen;
	static OCILOBFILECLOSE              OCILobFileClose;
	static OCILOBFILECLOSEALL           OCILobFileCloseAll;
	static OCILOBFILEISOPEN             OCILobFileIsOpen;
	static OCILOBFILEEXISTS             OCILobFileExists;
	static OCILOBFIELGETNAME            OCILobFileGetName;
	static OCILOBFILESETNAME            OCILobFileSetName;
	static OCILOBLOADFROMFILE           OCILobLoadFromFile;
	static OCILOBWRITEAPPEND            OCILobWriteAppend;
	static OCILOBISEQUAL                OCILobIsEqual;
	static OCILOBASSIGN                 OCILobAssign;       // @v6.1.6
	static OCISERVERVERSION             OCIServerVersion;
	static OCIATTRGET                   OCIAttrGet;
	static OCIATTRSET                   OCIAttrSet;
	static OCIDATEASSIGN                OCIDateAssign;
	static OCIDATETOTEXT                OCIDateToText;
	static OCIDATEFROMTEXT              OCIDateFromText;
	static OCIDATECOMPARE               OCIDateCompare;
	static OCIDATEADDMONTHS             OCIDateAddMonths;
	static OCIDATEADDDAYS               OCIDateAddDays;
	static OCIDATELASTDAY               OCIDateLastDay;
	static OCIDATEDAYSBETWEEN           OCIDateDaysBetween;
	static OCIDATEZONETOZONE            OCIDateZoneToZone;
	static OCIDATENEXTDAY               OCIDateNextDay;
	static OCIDATECHECK                 OCIDateCheck;
	static OCIDATESYSDATE               OCIDateSysDate;
	static OCIDESCRIBEANY               OCIDescribeAny;
	static OCIINTERVALASSIGN            OCIIntervalAssign;
	static OCIINTERVALCHECK             OCIIntervalCheck;
	static OCIINTERVALCOMPARE           OCIIntervalCompare;
	static OCIINTERVALFROMTEXT          OCIIntervalFromText;
	static OCIINTERVALTOTEXT            OCIIntervalToText;
	static OCIINTERVALFROMTZ            OCIIntervalFromTZ;
	static OCIINTERVALGETDAYSECOND      OCIIntervalGetDaySecond;
	static OCIINTERVALGETYEARMONTH      OCIIntervalGetYearMonth;
	static OCIINTERVALSETDAYSECOND      OCIIntervalSetDaySecond;
	static OCIINTERVALSETYEARMONTH      OCIIntervalSetYearMonth;
	static OCIINTERVALSUBTRACT          OCIIntervalSubtract;
	static OCIINTERVALADD               OCIIntervalAdd;
	static OCIDATETIMEASSIGN            OCIDateTimeAssign;
	static OCIDATETIMECHECK             OCIDateTimeCheck;
	static OCIDATETIMECOMPARE           OCIDateTimeCompare;
	static OCIDATETIMECONSTRUCT         OCIDateTimeConstruct;
	static OCIDATETIMECONVERT           OCIDateTimeConvert;
	static OCIDATETIMEFROMARRAY         OCIDateTimeFromArray;
	static OCIDATETIMETOARRAY           OCIDateTimeToArray;
	static OCIDATETIMEFROMTEXT          OCIDateTimeFromText;
	static OCIDATETIMETOTEXT            OCIDateTimeToText;
	static OCIDATETIMEGETDATE           OCIDateTimeGetDate;
	static OCIDATETIMEGETTIME           OCIDateTimeGetTime;
	static OCIDATETIMEGETTIMEZONENAME   OCIDateTimeGetTimeZoneName;
	static OCIDATETIMEGETTIMEZONEOFFSET OCIDateTimeGetTimeZoneOffset;
	static OCIDATETIMEINTERVALADD       OCIDateTimeIntervalAdd;
	static OCIDATETIMEINTERVALSUB       OCIDateTimeIntervalSub;
	static OCIDATETIMESUBTRACT          OCIDateTimeSubtract;
	static OCIDATETIMESYSTIMESTAMP      OCIDateTimeSysTimeStamp;
	static OCIARRAYDESCRIPTORFREE       OCIArrayDescriptorFree;
	static OCICLIENTVERSION             OCIClientVersion;

	static OCISTRINGPTR                 OCIStringPtr;
	static OCISTRINGASSIGNTEXT          OCIStringAssignText;
	static OCIRAWPTR                    OCIRawPtr;
	static OCIRAWASSIGNBYTES            OCIRawAssignBytes;
	static OCIRAWALLOCSIZE              OCIRawAllocSize;
	static OCIRAWRESIZE                 OCIRawResize;
	static OCIROWIDTOCHAR               OCIRowidToChar;
private:
	static SDynLibrary * P_Lib;
};

