/* A Bison parser, made by GNU Bison 3.7.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1
#define YYBISON_VERSION "3.7.1" /* Bison version.  */
#define YYSKELETON_NAME "yacc.c" /* Skeleton name.  */
#define YYPURE 2 /* Pure parsers.  */
#define YYPUSH 0 /* Push parsers.  */
#define YYPULL 1 /* Pull parsers.  */

/* Substitute the type names.  */
#define YYSTYPE         HSQL_STYPE
#define YYLTYPE         HSQL_LTYPE
/* Substitute the variable and function names.  */
#define yyparse         hsql_parse
#define yylex           hsql_lex
#define yyerror         hsql_error
#define yydebug         hsql_debug
#define yynerrs         hsql_nerrs

/* First part of user prologue.  */

  // clang-format on
  /**
 * bison_parser.y
 * defines bison_parser.h
 * outputs bison_parser.c
 *
 * Grammar File Spec: http://dinosaur.compilertools.net/bison/bison_6.html
 *
 */
  /*********************************
 ** Section 1: C Declarations
 *********************************/

#include <sql-parser.h>
#pragma hdrstop
#include "bison_parser.h"
#include "flex_lexer.h"

  using namespace hsql;

  int yyerror(YYLTYPE * llocp, SQLParserResult * result, yyscan_t scanner, const char* msg) {
    result->setIsValid(false);
    result->setErrorDetails(_strdup(msg), llocp->first_line, llocp->first_column);
    return 0;
  }
  // clang-format off


#ifndef YY_CAST
	#ifdef __cplusplus
		#define YY_CAST(Type, Val) static_cast<Type> (Val)
		#define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
	#else
		#define YY_CAST(Type, Val) ((Type) (Val))
		#define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
	#endif
#endif
#ifndef YY_NULLPTR
	#if defined __cplusplus
		#if 201103L <= __cplusplus
			#define YY_NULLPTR nullptr
		#else
			#define YY_NULLPTR 0
		#endif
	#else
		#define YY_NULLPTR ((void*)0)
	#endif
#endif

#include "bison_parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENTIFIER = 3,                 /* IDENTIFIER  */
  YYSYMBOL_STRING = 4,                     /* STRING  */
  YYSYMBOL_FLOATVAL = 5,                   /* FLOATVAL  */
  YYSYMBOL_INTVAL = 6,                     /* INTVAL  */
  YYSYMBOL_DEALLOCATE = 7,                 /* DEALLOCATE  */
  YYSYMBOL_PARAMETERS = 8,                 /* PARAMETERS  */
  YYSYMBOL_INTERSECT = 9,                  /* INTERSECT  */
  YYSYMBOL_TEMPORARY = 10,                 /* TEMPORARY  */
  YYSYMBOL_TIMESTAMP = 11,                 /* TIMESTAMP  */
  YYSYMBOL_DISTINCT = 12,                  /* DISTINCT  */
  YYSYMBOL_NVARCHAR = 13,                  /* NVARCHAR  */
  YYSYMBOL_RESTRICT = 14,                  /* RESTRICT  */
  YYSYMBOL_TRUNCATE = 15,                  /* TRUNCATE  */
  YYSYMBOL_ANALYZE = 16,                   /* ANALYZE  */
  YYSYMBOL_BETWEEN = 17,                   /* BETWEEN  */
  YYSYMBOL_CASCADE = 18,                   /* CASCADE  */
  YYSYMBOL_COLUMNS = 19,                   /* COLUMNS  */
  YYSYMBOL_CONTROL = 20,                   /* CONTROL  */
  YYSYMBOL_DEFAULT = 21,                   /* DEFAULT  */
  YYSYMBOL_EXECUTE = 22,                   /* EXECUTE  */
  YYSYMBOL_EXPLAIN = 23,                   /* EXPLAIN  */
  YYSYMBOL_INTEGER = 24,                   /* INTEGER  */
  YYSYMBOL_NATURAL = 25,                   /* NATURAL  */
  YYSYMBOL_PREPARE = 26,                   /* PREPARE  */
  YYSYMBOL_PRIMARY = 27,                   /* PRIMARY  */
  YYSYMBOL_SCHEMAS = 28,                   /* SCHEMAS  */
  YYSYMBOL_CHARACTER_VARYING = 29,         /* CHARACTER_VARYING  */
  YYSYMBOL_REAL = 30,                      /* REAL  */
  YYSYMBOL_DECIMAL = 31,                   /* DECIMAL  */
  YYSYMBOL_SMALLINT = 32,                  /* SMALLINT  */
  YYSYMBOL_BIGINT = 33,                    /* BIGINT  */
  YYSYMBOL_SPATIAL = 34,                   /* SPATIAL  */
  YYSYMBOL_VARCHAR = 35,                   /* VARCHAR  */
  YYSYMBOL_VIRTUAL = 36,                   /* VIRTUAL  */
  YYSYMBOL_DESCRIBE = 37,                  /* DESCRIBE  */
  YYSYMBOL_BEFORE = 38,                    /* BEFORE  */
  YYSYMBOL_COLUMN = 39,                    /* COLUMN  */
  YYSYMBOL_CREATE = 40,                    /* CREATE  */
  YYSYMBOL_DELETE = 41,                    /* DELETE  */
  YYSYMBOL_DIRECT = 42,                    /* DIRECT  */
  YYSYMBOL_DOUBLE = 43,                    /* DOUBLE  */
  YYSYMBOL_ESCAPE = 44,                    /* ESCAPE  */
  YYSYMBOL_EXCEPT = 45,                    /* EXCEPT  */
  YYSYMBOL_EXISTS = 46,                    /* EXISTS  */
  YYSYMBOL_EXTRACT = 47,                   /* EXTRACT  */
  YYSYMBOL_CAST = 48,                      /* CAST  */
  YYSYMBOL_FORMAT = 49,                    /* FORMAT  */
  YYSYMBOL_GLOBAL = 50,                    /* GLOBAL  */
  YYSYMBOL_HAVING = 51,                    /* HAVING  */
  YYSYMBOL_IMPORT = 52,                    /* IMPORT  */
  YYSYMBOL_INSERT = 53,                    /* INSERT  */
  YYSYMBOL_ISNULL = 54,                    /* ISNULL  */
  YYSYMBOL_OFFSET = 55,                    /* OFFSET  */
  YYSYMBOL_RENAME = 56,                    /* RENAME  */
  YYSYMBOL_SCHEMA = 57,                    /* SCHEMA  */
  YYSYMBOL_SELECT = 58,                    /* SELECT  */
  YYSYMBOL_SORTED = 59,                    /* SORTED  */
  YYSYMBOL_TABLES = 60,                    /* TABLES  */
  YYSYMBOL_UNIQUE = 61,                    /* UNIQUE  */
  YYSYMBOL_UNLOAD = 62,                    /* UNLOAD  */
  YYSYMBOL_UPDATE = 63,                    /* UPDATE  */
  YYSYMBOL_VALUES = 64,                    /* VALUES  */
  YYSYMBOL_AFTER = 65,                     /* AFTER  */
  YYSYMBOL_ALTER = 66,                     /* ALTER  */
  YYSYMBOL_CROSS = 67,                     /* CROSS  */
  YYSYMBOL_DELTA = 68,                     /* DELTA  */
  YYSYMBOL_FLOAT = 69,                     /* FLOAT  */
  YYSYMBOL_GROUP = 70,                     /* GROUP  */
  YYSYMBOL_INDEX = 71,                     /* INDEX  */
  YYSYMBOL_INNER = 72,                     /* INNER  */
  YYSYMBOL_LIMIT = 73,                     /* LIMIT  */
  YYSYMBOL_LOCAL = 74,                     /* LOCAL  */
  YYSYMBOL_MERGE = 75,                     /* MERGE  */
  YYSYMBOL_MINUS = 76,                     /* MINUS  */
  YYSYMBOL_ORDER = 77,                     /* ORDER  */
  YYSYMBOL_OVER = 78,                      /* OVER  */
  YYSYMBOL_OUTER = 79,                     /* OUTER  */
  YYSYMBOL_RIGHT = 80,                     /* RIGHT  */
  YYSYMBOL_TABLE = 81,                     /* TABLE  */
  YYSYMBOL_UNION = 82,                     /* UNION  */
  YYSYMBOL_USING = 83,                     /* USING  */
  YYSYMBOL_WHERE = 84,                     /* WHERE  */
  YYSYMBOL_CALL = 85,                      /* CALL  */
  YYSYMBOL_CASE = 86,                      /* CASE  */
  YYSYMBOL_CHAR = 87,                      /* CHAR  */
  YYSYMBOL_COPY = 88,                      /* COPY  */
  YYSYMBOL_DATE = 89,                      /* DATE  */
  YYSYMBOL_DATETIME = 90,                  /* DATETIME  */
  YYSYMBOL_DESC = 91,                      /* DESC  */
  YYSYMBOL_DROP = 92,                      /* DROP  */
  YYSYMBOL_ELSE = 93,                      /* ELSE  */
  YYSYMBOL_FILE = 94,                      /* FILE  */
  YYSYMBOL_FROM = 95,                      /* FROM  */
  YYSYMBOL_FULL = 96,                      /* FULL  */
  YYSYMBOL_HASH = 97,                      /* HASH  */
  YYSYMBOL_HINT = 98,                      /* HINT  */
  YYSYMBOL_INTO = 99,                      /* INTO  */
  YYSYMBOL_JOIN = 100,                     /* JOIN  */
  YYSYMBOL_LEFT = 101,                     /* LEFT  */
  YYSYMBOL_LIKE = 102,                     /* LIKE  */
  YYSYMBOL_LOAD = 103,                     /* LOAD  */
  YYSYMBOL_LONG = 104,                     /* LONG  */
  YYSYMBOL_NULL = 105,                     /* NULL  */
  YYSYMBOL_PARTITION = 106,                /* PARTITION  */
  YYSYMBOL_PLAN = 107,                     /* PLAN  */
  YYSYMBOL_SHOW = 108,                     /* SHOW  */
  YYSYMBOL_TEXT = 109,                     /* TEXT  */
  YYSYMBOL_THEN = 110,                     /* THEN  */
  YYSYMBOL_TIME = 111,                     /* TIME  */
  YYSYMBOL_VIEW = 112,                     /* VIEW  */
  YYSYMBOL_WHEN = 113,                     /* WHEN  */
  YYSYMBOL_WITH = 114,                     /* WITH  */
  YYSYMBOL_ADD = 115,                      /* ADD  */
  YYSYMBOL_ALL = 116,                      /* ALL  */
  YYSYMBOL_AND = 117,                      /* AND  */
  YYSYMBOL_ASC = 118,                      /* ASC  */
  YYSYMBOL_END = 119,                      /* END  */
  YYSYMBOL_FOR = 120,                      /* FOR  */
  YYSYMBOL_INT = 121,                      /* INT  */
  YYSYMBOL_KEY = 122,                      /* KEY  */
  YYSYMBOL_NOT = 123,                      /* NOT  */
  YYSYMBOL_OFF = 124,                      /* OFF  */
  YYSYMBOL_SET = 125,                      /* SET  */
  YYSYMBOL_TOP = 126,                      /* TOP  */
  YYSYMBOL_AS = 127,                       /* AS  */
  YYSYMBOL_BY = 128,                       /* BY  */
  YYSYMBOL_IF = 129,                       /* IF  */
  YYSYMBOL_IN = 130,                       /* IN  */
  YYSYMBOL_IS = 131,                       /* IS  */
  YYSYMBOL_OF = 132,                       /* OF  */
  YYSYMBOL_ON = 133,                       /* ON  */
  YYSYMBOL_OR = 134,                       /* OR  */
  YYSYMBOL_TO = 135,                       /* TO  */
  YYSYMBOL_NO = 136,                       /* NO  */
  YYSYMBOL_ARRAY = 137,                    /* ARRAY  */
  YYSYMBOL_CONCAT = 138,                   /* CONCAT  */
  YYSYMBOL_ILIKE = 139,                    /* ILIKE  */
  YYSYMBOL_SECOND = 140,                   /* SECOND  */
  YYSYMBOL_MINUTE = 141,                   /* MINUTE  */
  YYSYMBOL_HOUR = 142,                     /* HOUR  */
  YYSYMBOL_DAY = 143,                      /* DAY  */
  YYSYMBOL_MONTH = 144,                    /* MONTH  */
  YYSYMBOL_YEAR = 145,                     /* YEAR  */
  YYSYMBOL_SECONDS = 146,                  /* SECONDS  */
  YYSYMBOL_MINUTES = 147,                  /* MINUTES  */
  YYSYMBOL_HOURS = 148,                    /* HOURS  */
  YYSYMBOL_DAYS = 149,                     /* DAYS  */
  YYSYMBOL_MONTHS = 150,                   /* MONTHS  */
  YYSYMBOL_YEARS = 151,                    /* YEARS  */
  YYSYMBOL_INTERVAL = 152,                 /* INTERVAL  */
  YYSYMBOL_TRUE = 153,                     /* TRUE  */
  YYSYMBOL_FALSE = 154,                    /* FALSE  */
  YYSYMBOL_BOOLEAN = 155,                  /* BOOLEAN  */
  YYSYMBOL_TRANSACTION = 156,              /* TRANSACTION  */
  YYSYMBOL_BEGIN = 157,                    /* BEGIN  */
  YYSYMBOL_COMMIT = 158,                   /* COMMIT  */
  YYSYMBOL_ROLLBACK = 159,                 /* ROLLBACK  */
  YYSYMBOL_NOWAIT = 160,                   /* NOWAIT  */
  YYSYMBOL_SKIP = 161,                     /* SKIP  */
  YYSYMBOL_LOCKED = 162,                   /* LOCKED  */
  YYSYMBOL_SHARE = 163,                    /* SHARE  */
  YYSYMBOL_RANGE = 164,                    /* RANGE  */
  YYSYMBOL_ROWS = 165,                     /* ROWS  */
  YYSYMBOL_GROUPS = 166,                   /* GROUPS  */
  YYSYMBOL_UNBOUNDED = 167,                /* UNBOUNDED  */
  YYSYMBOL_FOLLOWING = 168,                /* FOLLOWING  */
  YYSYMBOL_PRECEDING = 169,                /* PRECEDING  */
  YYSYMBOL_CURRENT_ROW = 170,              /* CURRENT_ROW  */
  YYSYMBOL_BINARY = 171,                   /* BINARY  */
  YYSYMBOL_RAW = 172,                      /* RAW  */
  YYSYMBOL_173_ = 173,                     /* '='  */
  YYSYMBOL_EQUALS = 174,                   /* EQUALS  */
  YYSYMBOL_NOTEQUALS = 175,                /* NOTEQUALS  */
  YYSYMBOL_176_ = 176,                     /* '<'  */
  YYSYMBOL_177_ = 177,                     /* '>'  */
  YYSYMBOL_LESS = 178,                     /* LESS  */
  YYSYMBOL_GREATER = 179,                  /* GREATER  */
  YYSYMBOL_LESSEQ = 180,                   /* LESSEQ  */
  YYSYMBOL_GREATEREQ = 181,                /* GREATEREQ  */
  YYSYMBOL_NOTNULL = 182,                  /* NOTNULL  */
  YYSYMBOL_183_ = 183,                     /* '+'  */
  YYSYMBOL_184_ = 184,                     /* '-'  */
  YYSYMBOL_185_ = 185,                     /* '*'  */
  YYSYMBOL_186_ = 186,                     /* '/'  */
  YYSYMBOL_187_ = 187,                     /* '%'  */
  YYSYMBOL_188_ = 188,                     /* '^'  */
  YYSYMBOL_UMINUS = 189,                   /* UMINUS  */
  YYSYMBOL_190_ = 190,                     /* '['  */
  YYSYMBOL_191_ = 191,                     /* ']'  */
  YYSYMBOL_192_ = 192,                     /* '('  */
  YYSYMBOL_193_ = 193,                     /* ')'  */
  YYSYMBOL_194_ = 194,                     /* '.'  */
  YYSYMBOL_195_ = 195,                     /* ';'  */
  YYSYMBOL_196_ = 196,                     /* ','  */
  YYSYMBOL_197_ = 197,                     /* '?'  */
  YYSYMBOL_YYACCEPT = 198,                 /* $accept  */
  YYSYMBOL_input = 199,                    /* input  */
  YYSYMBOL_statement_list = 200,           /* statement_list  */
  YYSYMBOL_statement = 201,                /* statement  */
  YYSYMBOL_preparable_statement = 202,     /* preparable_statement  */
  YYSYMBOL_opt_hints = 203,                /* opt_hints  */
  YYSYMBOL_hint_list = 204,                /* hint_list  */
  YYSYMBOL_hint = 205,                     /* hint  */
  YYSYMBOL_transaction_statement = 206,    /* transaction_statement  */
  YYSYMBOL_opt_transaction_keyword = 207,  /* opt_transaction_keyword  */
  YYSYMBOL_prepare_statement = 208,        /* prepare_statement  */
  YYSYMBOL_prepare_target_query = 209,     /* prepare_target_query  */
  YYSYMBOL_execute_statement = 210,        /* execute_statement  */
  YYSYMBOL_import_statement = 211,         /* import_statement  */
  YYSYMBOL_file_type = 212,                /* file_type  */
  YYSYMBOL_file_path = 213,                /* file_path  */
  YYSYMBOL_opt_file_type = 214,            /* opt_file_type  */
  YYSYMBOL_export_statement = 215,         /* export_statement  */
  YYSYMBOL_show_statement = 216,           /* show_statement  */
  YYSYMBOL_create_statement = 217,         /* create_statement  */
  YYSYMBOL_opt_not_exists = 218,           /* opt_not_exists  */
  YYSYMBOL_table_elem_commalist = 219,     /* table_elem_commalist  */
  YYSYMBOL_table_elem = 220,               /* table_elem  */
  YYSYMBOL_column_def = 221,               /* column_def  */
  YYSYMBOL_column_type = 222,              /* column_type  */
  YYSYMBOL_opt_time_precision = 223,       /* opt_time_precision  */
  YYSYMBOL_opt_decimal_specification = 224, /* opt_decimal_specification  */
  YYSYMBOL_opt_column_constraints = 225,   /* opt_column_constraints  */
  YYSYMBOL_column_constraint_set = 226,    /* column_constraint_set  */
  YYSYMBOL_column_constraint = 227,        /* column_constraint  */
  YYSYMBOL_table_constraint = 228,         /* table_constraint  */
  YYSYMBOL_drop_statement = 229,           /* drop_statement  */
  YYSYMBOL_opt_exists = 230,               /* opt_exists  */
  YYSYMBOL_alter_statement = 231,          /* alter_statement  */
  YYSYMBOL_alter_action = 232,             /* alter_action  */
  YYSYMBOL_drop_action = 233,              /* drop_action  */
  YYSYMBOL_delete_statement = 234,         /* delete_statement  */
  YYSYMBOL_truncate_statement = 235,       /* truncate_statement  */
  YYSYMBOL_insert_statement = 236,         /* insert_statement  */
  YYSYMBOL_opt_column_list = 237,          /* opt_column_list  */
  YYSYMBOL_update_statement = 238,         /* update_statement  */
  YYSYMBOL_update_clause_commalist = 239,  /* update_clause_commalist  */
  YYSYMBOL_update_clause = 240,            /* update_clause  */
  YYSYMBOL_select_statement = 241,         /* select_statement  */
  YYSYMBOL_select_within_set_operation = 242, /* select_within_set_operation  */
  YYSYMBOL_select_within_set_operation_no_parentheses = 243, /* select_within_set_operation_no_parentheses  */
  YYSYMBOL_select_with_paren = 244,        /* select_with_paren  */
  YYSYMBOL_select_no_paren = 245,          /* select_no_paren  */
  YYSYMBOL_set_operator = 246,             /* set_operator  */
  YYSYMBOL_set_type = 247,                 /* set_type  */
  YYSYMBOL_opt_all = 248,                  /* opt_all  */
  YYSYMBOL_select_clause = 249,            /* select_clause  */
  YYSYMBOL_opt_distinct = 250,             /* opt_distinct  */
  YYSYMBOL_select_list = 251,              /* select_list  */
  YYSYMBOL_opt_from_clause = 252,          /* opt_from_clause  */
  YYSYMBOL_from_clause = 253,              /* from_clause  */
  YYSYMBOL_opt_where = 254,                /* opt_where  */
  YYSYMBOL_opt_group = 255,                /* opt_group  */
  YYSYMBOL_opt_having = 256,               /* opt_having  */
  YYSYMBOL_opt_order = 257,                /* opt_order  */
  YYSYMBOL_order_list = 258,               /* order_list  */
  YYSYMBOL_order_desc = 259,               /* order_desc  */
  YYSYMBOL_opt_order_type = 260,           /* opt_order_type  */
  YYSYMBOL_opt_top = 261,                  /* opt_top  */
  YYSYMBOL_opt_limit = 262,                /* opt_limit  */
  YYSYMBOL_expr_list = 263,                /* expr_list  */
  YYSYMBOL_opt_literal_list = 264,         /* opt_literal_list  */
  YYSYMBOL_literal_list = 265,             /* literal_list  */
  YYSYMBOL_expr_alias = 266,               /* expr_alias  */
  YYSYMBOL_expr = 267,                     /* expr  */
  YYSYMBOL_operand = 268,                  /* operand  */
  YYSYMBOL_scalar_expr = 269,              /* scalar_expr  */
  YYSYMBOL_unary_expr = 270,               /* unary_expr  */
  YYSYMBOL_binary_expr = 271,              /* binary_expr  */
  YYSYMBOL_logic_expr = 272,               /* logic_expr  */
  YYSYMBOL_in_expr = 273,                  /* in_expr  */
  YYSYMBOL_case_expr = 274,                /* case_expr  */
  YYSYMBOL_case_list = 275,                /* case_list  */
  YYSYMBOL_exists_expr = 276,              /* exists_expr  */
  YYSYMBOL_comp_expr = 277,                /* comp_expr  */
  YYSYMBOL_function_expr = 278,            /* function_expr  */
  YYSYMBOL_opt_window = 279,               /* opt_window  */
  YYSYMBOL_opt_partition = 280,            /* opt_partition  */
  YYSYMBOL_opt_frame_clause = 281,         /* opt_frame_clause  */
  YYSYMBOL_frame_type = 282,               /* frame_type  */
  YYSYMBOL_frame_bound = 283,              /* frame_bound  */
  YYSYMBOL_extract_expr = 284,             /* extract_expr  */
  YYSYMBOL_cast_expr = 285,                /* cast_expr  */
  YYSYMBOL_datetime_field = 286,           /* datetime_field  */
  YYSYMBOL_datetime_field_plural = 287,    /* datetime_field_plural  */
  YYSYMBOL_duration_field = 288,           /* duration_field  */
  YYSYMBOL_array_expr = 289,               /* array_expr  */
  YYSYMBOL_array_index = 290,              /* array_index  */
  YYSYMBOL_between_expr = 291,             /* between_expr  */
  YYSYMBOL_column_name = 292,              /* column_name  */
  YYSYMBOL_literal = 293,                  /* literal  */
  YYSYMBOL_string_literal = 294,           /* string_literal  */
  YYSYMBOL_bool_literal = 295,             /* bool_literal  */
  YYSYMBOL_num_literal = 296,              /* num_literal  */
  YYSYMBOL_int_literal = 297,              /* int_literal  */
  YYSYMBOL_null_literal = 298,             /* null_literal  */
  YYSYMBOL_date_literal = 299,             /* date_literal  */
  YYSYMBOL_interval_literal = 300,         /* interval_literal  */
  YYSYMBOL_param_expr = 301,               /* param_expr  */
  YYSYMBOL_table_ref = 302,                /* table_ref  */
  YYSYMBOL_table_ref_atomic = 303,         /* table_ref_atomic  */
  YYSYMBOL_nonjoin_table_ref_atomic = 304, /* nonjoin_table_ref_atomic  */
  YYSYMBOL_table_ref_commalist = 305,      /* table_ref_commalist  */
  YYSYMBOL_table_ref_name = 306,           /* table_ref_name  */
  YYSYMBOL_table_ref_name_no_alias = 307,  /* table_ref_name_no_alias  */
  YYSYMBOL_table_name = 308,               /* table_name  */
  YYSYMBOL_opt_index_name = 309,           /* opt_index_name  */
  YYSYMBOL_table_alias = 310,              /* table_alias  */
  YYSYMBOL_opt_table_alias = 311,          /* opt_table_alias  */
  YYSYMBOL_alias = 312,                    /* alias  */
  YYSYMBOL_opt_alias = 313,                /* opt_alias  */
  YYSYMBOL_opt_locking_clause = 314,       /* opt_locking_clause  */
  YYSYMBOL_opt_locking_clause_list = 315,  /* opt_locking_clause_list  */
  YYSYMBOL_locking_clause = 316,           /* locking_clause  */
  YYSYMBOL_row_lock_mode = 317,            /* row_lock_mode  */
  YYSYMBOL_opt_row_lock_policy = 318,      /* opt_row_lock_policy  */
  YYSYMBOL_opt_with_clause = 319,          /* opt_with_clause  */
  YYSYMBOL_with_clause = 320,              /* with_clause  */
  YYSYMBOL_with_description_list = 321,    /* with_description_list  */
  YYSYMBOL_with_description = 322,         /* with_description  */
  YYSYMBOL_join_clause = 323,              /* join_clause  */
  YYSYMBOL_opt_join_type = 324,            /* opt_join_type  */
  YYSYMBOL_join_condition = 325,           /* join_condition  */
  YYSYMBOL_opt_semicolon = 326,            /* opt_semicolon  */
  YYSYMBOL_ident_commalist = 327           /* ident_commalist  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif
// 
// On compilers that do not define __PTRDIFF_MAX__ etc., make sure
// <limits.h> and (if available) <stdint.h> are included
// so that the code can choose integer types of a good width.  
// 
#ifndef __PTRDIFF_MAX__
	#include <limits.h> /* INFRINGES ON USER NAME SPACE */
	#if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
		#include <stdint.h> /* INFRINGES ON USER NAME SPACE */
		#define YY_STDINT_H
	#endif
#endif
// 
// Narrow types that promote to a signed type and that can represent a
// signed or unsigned integer of at least N bits.  In tables they can
// save space and decrease cache pressure.  Promoting to a signed type
// helps avoid bugs in integer arithmetic.  
// 
#ifdef __INT_LEAST8_MAX__
	typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
	typedef int_least8_t yytype_int8;
#else
	typedef signed char yytype_int8;
#endif
#ifdef __INT_LEAST16_MAX__
	typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
	typedef int_least16_t yytype_int16;
#else
	typedef short yytype_int16;
#endif
#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
	typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H && UINT_LEAST8_MAX <= INT_MAX)
	typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
	typedef unsigned char yytype_uint8;
#else
	typedef short yytype_uint8;
#endif
#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
	typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H && UINT_LEAST16_MAX <= INT_MAX)
	typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
	typedef unsigned short yytype_uint16;
#else
	typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif
#ifndef YYSIZE_T
	#ifdef __SIZE_TYPE__
		#define YYSIZE_T __SIZE_TYPE__
	#elif defined size_t
		#define YYSIZE_T size_t
	#elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
		#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
		#define YYSIZE_T size_t
	#else
		#define YYSIZE_T unsigned
	#endif
#endif
#define YYSIZE_MAXIMUM YY_CAST (YYPTRDIFF_T, (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1) ? YYPTRDIFF_MAXIMUM : YY_CAST (YYSIZE_T, -1)))
#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
	#if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
		#define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
	#else
		#define YY_ATTRIBUTE_PURE
	#endif
#endif
#ifndef YY_ATTRIBUTE_UNUSED
	#if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
		#define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
	#else
		#define YY_ATTRIBUTE_UNUSED
	#endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
	#define YYUSE(E) ((void) (E))
#else
	#define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
	#define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	#define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
	#define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
	#define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
	#define YY_IGNORE_USELESS_CAST_BEGIN
	#define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS && ! ((defined YYMALLOC || defined malloc) && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (!defined yyoverflow && (! defined __cplusplus \
         || (defined HSQL_LTYPE_IS_TRIVIAL && HSQL_LTYPE_IS_TRIVIAL \
             && defined HSQL_STYPE_IS_TRIVIAL && HSQL_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc {
	yy_state_t yyss_alloc;
	YYSTYPE yyvs_alloc;
	YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with N elements.  */
# define YYSTACK_BYTES(N) ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) + YYSIZEOF (YYLTYPE)) + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do {                                                                \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY(&yyptr->Stack_alloc, Stack, yysize);                     \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      } while(0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do {                                      \
          YYPTRDIFF_T yyi;                      \
          for(yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        } while(0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

#define YYFINAL  70 /* YYFINAL -- State number of the termination state.  */
#define YYLAST   873 /* YYLAST -- Last index in YYTABLE.  */
#define YYNTOKENS  198 /* YYNTOKENS -- Number of terminals.  */
#define YYNNTS  130 /* YYNNTS -- Number of nonterminals.  */
#define YYNRULES  339 /* YYNRULES -- Number of rules.  */
#define YYNSTATES  623 /* YYNSTATES -- Number of states.  */
#define YYMAXUTOK   435  /* YYMAXUTOK -- Last valid token kind.  */


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX) (0 <= (YYX) && (YYX) <= YYMAXUTOK ? YY_CAST(yysymbol_kind_t, yytranslate[YYX]) : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,   187,     2,     2,
     192,   193,   185,   183,   196,   184,   194,   186,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   195,
     176,   173,   177,   197,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   190,     2,   191,   188,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   174,   175,
     178,   179,   180,   181,   182,   189
};

#if HSQL_DEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   321,   321,   340,   346,   353,   357,   361,   362,   363,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     380,   381,   383,   387,   392,   396,   406,   407,   408,   410,
     410,   416,   422,   424,   428,   439,   445,   453,   469,   473,
     478,   478,   484,   490,   501,   502,   507,   518,   531,   543,
     550,   558,   566,   575,   575,   577,   581,   586,   586,   588,
     600,   601,   602,   603,   604,   605,   606,   610,   611,   612,
     613,   614,   615,   616,   617,   618,   619,   620,   621,   622,
     624,   624,   626,   627,   628,   630,   630,   632,   636,   641,
     642,   643,   644,   646,   647,   655,   661,   667,   673,   679,
     679,   686,   692,   694,   704,   711,   722,   729,   737,   738,
     745,   752,   756,   761,   771,   775,   779,   791,   791,   793,
     794,   803,   804,   806,   820,   832,   837,   841,   845,   850,
     851,   853,   863,   864,   866,   868,   869,   871,   873,   874,
     876,   881,   883,   884,   886,   887,   889,   893,   898,   900,
     901,   902,   906,   907,   909,   910,   911,   912,   913,   914,
     919,   923,   928,   929,   931,   935,   940,   948,   948,   948,
     948,   948,   950,   951,   951,   951,   951,   951,   951,   951,
     951,   952,   952,   956,   956,   958,   959,   960,   961,   962,
     964,   964,   965,   966,   967,   968,   969,   970,   971,   972,
     973,   975,   976,   978,   979,   980,   981,   985,   986,   987,
     988,   990,   991,   993,   994,   996,   997,   998,   999,  1000,
    1001,  1002,  1006,  1007,  1011,  1012,  1014,  1015,  1020,  1021,
    1022,  1026,  1027,  1028,  1030,  1031,  1032,  1033,  1034,  1036,
    1038,  1040,  1041,  1042,  1043,  1044,  1045,  1047,  1048,  1049,
    1050,  1051,  1052,  1054,  1054,  1056,  1058,  1060,  1062,  1063,
    1064,  1065,  1067,  1067,  1067,  1067,  1067,  1067,  1067,  1068,
    1069,  1069,  1070,  1070,  1071,  1072,  1074,  1085,  1089,  1100,
    1137,  1146,  1146,  1153,  1153,  1155,  1155,  1162,  1166,  1171,
    1179,  1185,  1189,  1194,  1195,  1197,  1197,  1199,  1199,  1201,
    1202,  1204,  1204,  1210,  1211,  1213,  1217,  1222,  1228,  1235,
    1236,  1237,  1238,  1240,  1241,  1242,  1248,  1248,  1250,  1252,
    1256,  1261,  1271,  1278,  1286,  1310,  1311,  1312,  1313,  1314,
    1315,  1316,  1317,  1318,  1319,  1321,  1327,  1327,  1330,  1334
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char * yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "IDENTIFIER", "STRING",
  "FLOATVAL", "INTVAL", "DEALLOCATE", "PARAMETERS", "INTERSECT",
  "TEMPORARY", "TIMESTAMP", "DISTINCT", "NVARCHAR", "RESTRICT", "TRUNCATE",
  "ANALYZE", "BETWEEN", "CASCADE", "COLUMNS", "CONTROL", "DEFAULT",
  "EXECUTE", "EXPLAIN", "INTEGER", "NATURAL", "PREPARE", "PRIMARY",
  "SCHEMAS", "CHARACTER_VARYING", "REAL", "DECIMAL", "SMALLINT", "BIGINT",
  "SPATIAL", "VARCHAR", "VIRTUAL", "DESCRIBE", "BEFORE", "COLUMN",
  "CREATE", "DELETE", "DIRECT", "DOUBLE", "ESCAPE", "EXCEPT", "EXISTS",
  "EXTRACT", "CAST", "FORMAT", "GLOBAL", "HAVING", "IMPORT", "INSERT",
  "ISNULL", "OFFSET", "RENAME", "SCHEMA", "SELECT", "SORTED", "TABLES",
  "UNIQUE", "UNLOAD", "UPDATE", "VALUES", "AFTER", "ALTER", "CROSS",
  "DELTA", "FLOAT", "GROUP", "INDEX", "INNER", "LIMIT", "LOCAL", "MERGE",
  "MINUS", "ORDER", "OVER", "OUTER", "RIGHT", "TABLE", "UNION", "USING",
  "WHERE", "CALL", "CASE", "CHAR", "COPY", "DATE", "DATETIME", "DESC",
  "DROP", "ELSE", "FILE", "FROM", "FULL", "HASH", "HINT", "INTO", "JOIN",
  "LEFT", "LIKE", "LOAD", "LONG", "NULL", "PARTITION", "PLAN", "SHOW",
  "TEXT", "THEN", "TIME", "VIEW", "WHEN", "WITH", "ADD", "ALL", "AND",
  "ASC", "END", "FOR", "INT", "KEY", "NOT", "OFF", "SET", "TOP", "AS",
  "BY", "IF", "IN", "IS", "OF", "ON", "OR", "TO", "NO", "ARRAY", "CONCAT",
  "ILIKE", "SECOND", "MINUTE", "HOUR", "DAY", "MONTH", "YEAR", "SECONDS",
  "MINUTES", "HOURS", "DAYS", "MONTHS", "YEARS", "INTERVAL", "TRUE",
  "FALSE", "BOOLEAN", "TRANSACTION", "BEGIN", "COMMIT", "ROLLBACK",
  "NOWAIT", "SKIP", "LOCKED", "SHARE", "RANGE", "ROWS", "GROUPS",
  "UNBOUNDED", "FOLLOWING", "PRECEDING", "CURRENT_ROW", "BINARY", "RAW",
  "'='", "EQUALS", "NOTEQUALS", "'<'", "'>'", "LESS", "GREATER", "LESSEQ",
  "GREATEREQ", "NOTNULL", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'",
  "UMINUS", "'['", "']'", "'('", "')'", "'.'", "';'", "','", "'?'",
  "$accept", "input", "statement_list", "statement",
  "preparable_statement", "opt_hints", "hint_list", "hint",
  "transaction_statement", "opt_transaction_keyword", "prepare_statement",
  "prepare_target_query", "execute_statement", "import_statement",
  "file_type", "file_path", "opt_file_type", "export_statement",
  "show_statement", "create_statement", "opt_not_exists",
  "table_elem_commalist", "table_elem", "column_def", "column_type",
  "opt_time_precision", "opt_decimal_specification",
  "opt_column_constraints", "column_constraint_set", "column_constraint",
  "table_constraint", "drop_statement", "opt_exists", "alter_statement",
  "alter_action", "drop_action", "delete_statement", "truncate_statement",
  "insert_statement", "opt_column_list", "update_statement",
  "update_clause_commalist", "update_clause", "select_statement",
  "select_within_set_operation",
  "select_within_set_operation_no_parentheses", "select_with_paren",
  "select_no_paren", "set_operator", "set_type", "opt_all",
  "select_clause", "opt_distinct", "select_list", "opt_from_clause",
  "from_clause", "opt_where", "opt_group", "opt_having", "opt_order",
  "order_list", "order_desc", "opt_order_type", "opt_top", "opt_limit",
  "expr_list", "opt_literal_list", "literal_list", "expr_alias", "expr",
  "operand", "scalar_expr", "unary_expr", "binary_expr", "logic_expr",
  "in_expr", "case_expr", "case_list", "exists_expr", "comp_expr",
  "function_expr", "opt_window", "opt_partition", "opt_frame_clause",
  "frame_type", "frame_bound", "extract_expr", "cast_expr",
  "datetime_field", "datetime_field_plural", "duration_field",
  "array_expr", "array_index", "between_expr", "column_name", "literal",
  "string_literal", "bool_literal", "num_literal", "int_literal",
  "null_literal", "date_literal", "interval_literal", "param_expr",
  "table_ref", "table_ref_atomic", "nonjoin_table_ref_atomic",
  "table_ref_commalist", "table_ref_name", "table_ref_name_no_alias",
  "table_name", "opt_index_name", "table_alias", "opt_table_alias",
  "alias", "opt_alias", "opt_locking_clause", "opt_locking_clause_list",
  "locking_clause", "row_lock_mode", "opt_row_lock_policy",
  "opt_with_clause", "with_clause", "with_description_list",
  "with_description", "join_clause", "opt_join_type", "join_condition",
  "opt_semicolon", "ident_commalist", YY_NULLPTR
};

static const char * yysymbol_name (yysymbol_kind_t yysymbol)
{
	return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,    61,   428,   429,    60,    62,   430,   431,
     432,   433,   434,    43,    45,    42,    47,    37,    94,   435,
      91,    93,    40,    41,    46,    59,    44,    63
};
#endif

#define YYPACT_NINF (-469)
#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)
#define YYTABLE_NINF (-337)
#define yytable_value_is_error(Yyn) ((Yyn) == YYTABLE_NINF)

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     624,    53,   137,   224,   243,   137,    -6,    89,   159,    49,
     137,   142,    18,   158,   128,   265,   123,   123,   123,   307,
     116,  -469,   216,  -469,   216,  -469,  -469,  -469,  -469,  -469,
    -469,  -469,  -469,  -469,  -469,  -469,  -469,   -33,  -469,   314,
     166,  -469,   175,   267,  -469,   298,   239,   239,   239,   137,
      20,   137,   256,  -469,   259,   -33,   271,   -19,   259,   259,
     259,   137,  -469,   284,   221,  -469,  -469,  -469,  -469,  -469,
    -469,   607,  -469,   321,  -469,  -469,   286,    37,  -469,   181,
    -469,   419,   145,   423,   239,   308,   427,   137,   137,   349,
    -469,  -469,   341,   244,   434,   392,   137,   246,   247,   437,
     437,   437,   440,   137,   137,  -469,   254,   265,  -469,   255,
     446,   441,  -469,  -469,  -469,   -33,   339,   328,   -33,    -1,
    -469,  -469,  -469,  -469,   454,  -469,   455,  -469,  -469,  -469,
     268,   266,  -469,  -469,  -469,  -469,   722,  -469,  -469,  -469,
    -469,  -469,  -469,   427,   417,  -469,   331,   -54,   244,   343,
    -469,   437,   466,     6,   299,   -55,  -469,  -469,   378,  -469,
    -469,   357,  -469,   357,   357,  -469,  -469,  -469,  -469,  -469,
     471,  -469,  -469,   343,   398,  -469,  -469,    37,  -469,  -469,
     343,   398,   343,   167,   356,  -469,   235,  -469,   145,  -469,
    -469,  -469,  -469,  -469,  -469,  -469,  -469,  -469,  -469,  -469,
    -469,  -469,  -469,  -469,   344,  -469,   137,   475,   365,    81,
     358,    72,   291,   292,   294,   173,   368,   300,   420,  -469,
     297,   140,   411,  -469,  -469,  -469,  -469,  -469,  -469,  -469,
    -469,  -469,  -469,  -469,  -469,  -469,  -469,  -469,  -469,   388,
    -469,   -30,   296,  -469,   343,   434,  -469,   453,  -469,  -469,
     444,  -469,   349,  -469,   306,   136,  -469,   404,   304,  -469,
      91,    -1,   -33,   305,  -469,   126,    -1,   140,   448,    41,
      17,  -469,   356,  -469,  -469,  -469,   137,   312,   414,  -469,
     547,   389,   318,   138,  -469,  -469,  -469,   365,    23,    24,
     458,   235,   343,   343,   174,   152,   320,   420,   637,   343,
      45,   324,   -32,   343,   343,   420,  -469,   420,   -49,   327,
     -46,   420,   420,   420,   420,   420,   420,   420,   420,   420,
     420,   420,   420,   420,   420,   420,   446,   137,  -469,   520,
     145,   140,  -469,   259,    20,  -469,   145,  -469,   471,    19,
     349,  -469,   343,  -469,   521,  -469,  -469,  -469,  -469,   343,
    -469,  -469,  -469,   356,   343,   343,  -469,   363,   407,  -469,
     -74,  -469,   338,   466,   437,  -469,  -469,   340,  -469,   345,
    -469,  -469,   346,  -469,  -469,   347,  -469,  -469,  -469,  -469,
     352,  -469,  -469,   353,   354,    51,   355,   466,  -469,    81,
    -469,   470,   343,  -469,  -469,   361,   438,   188,   -43,   156,
     343,   343,  -469,   458,   449,  -135,  -469,  -469,  -469,   439,
     569,   656,   420,   367,   297,  -469,   450,   371,   656,   656,
     656,   656,   671,   671,   671,   671,    45,    45,   -89,   -89,
     -89,   -87,   372,  -469,  -469,   160,   559,  -469,   170,  -469,
     365,  -469,    85,  -469,   370,  -469,    33,  -469,   494,  -469,
    -469,  -469,  -469,   140,   140,  -469,   504,   466,  -469,   408,
    -469,   466,   189,  -469,   563,   575,  -469,   577,   583,   587,
    -469,   594,   596,   481,  -469,  -469,   501,  -469,    51,  -469,
     466,   191,  -469,   383,  -469,   202,  -469,   343,   547,   343,
     343,  -469,   199,   178,   415,  -469,   420,   656,   297,   416,
     203,  -469,  -469,  -469,  -469,  -469,   418,   510,  -469,  -469,
    -469,   534,   536,   539,   519,    19,   617,  -469,  -469,  -469,
     493,  -469,  -469,    80,  -469,   207,  -469,   431,   211,   432,
     433,   442,   445,   447,  -469,  -469,  -469,   212,  -469,   522,
     470,   -25,   452,   140,   220,  -469,   343,  -469,   637,   456,
     217,  -469,  -469,    33,    19,  -469,  -469,  -469,    19,   213,
     451,   343,  -469,  -469,  -469,  -469,   621,  -469,  -469,  -469,
    -469,  -469,  -469,   502,   398,  -469,  -469,  -469,  -469,   140,
    -469,  -469,  -469,  -469,   435,   466,   -27,   459,   343,   228,
     461,   343,   225,   343,  -469,  -469,   304,  -469,  -469,  -469,
     462,    26,    27,   140,  -469,  -469,   140,  -469,    34,    31,
     155,  -469,  -469,   460,   464,  -469,  -469,   515,  -469,  -469,
    -469,    31,  -469
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
     317,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    30,    30,    30,     0,
     337,     3,    21,    19,    21,    18,     8,     9,     7,    11,
      16,    17,    13,    14,    12,    15,    10,     0,   316,     0,
     291,   105,    33,     0,    46,     0,    54,    54,    54,     0,
       0,     0,     0,   290,   100,     0,     0,     0,   100,   100,
     100,     0,    44,     0,   318,   319,    29,    26,    28,    27,
       1,   317,     2,     0,     6,     5,   153,   114,   115,   145,
      97,     0,   163,     0,    54,     0,   294,     0,     0,   139,
      37,    38,     0,   109,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,     0,     0,     4,     0,
       0,   133,   127,   128,   126,     0,   130,     0,     0,   159,
     292,   269,   272,   274,     0,   275,     0,   270,   271,   280,
       0,   162,   164,   262,   263,   264,   273,   265,   266,   267,
     268,    32,    31,   294,     0,   293,     0,     0,   109,     0,
     104,     0,     0,     0,     0,   139,   111,    99,     0,   122,
     121,    41,    39,    41,    41,    98,    95,    96,   321,   320,
       0,   152,   132,     0,   145,   118,   117,   119,   129,   125,
       0,   145,     0,     0,   304,   276,   279,    34,     0,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   277,     0,    53,     0,     0,   317,     0,
       0,   258,     0,     0,     0,     0,     0,     0,     0,   260,
       0,   138,   167,   174,   175,   176,   169,   171,   177,   170,
     190,   178,   179,   180,   181,   173,   168,   183,   184,     0,
     338,     0,     0,   107,     0,     0,   110,     0,   101,   102,
       0,    43,   139,    42,    24,     0,    22,   136,   134,   160,
     302,   159,     0,   144,   146,   151,   159,   155,   157,   154,
       0,   123,   303,   305,   278,   165,     0,     0,     0,    49,
       0,     0,     0,     0,    55,    57,    58,   317,   133,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   186,     0,
     185,     0,     0,     0,     0,     0,   187,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   108,     0,
       0,   113,   112,   100,     0,    36,     0,    20,     0,     0,
     139,   135,     0,   300,     0,   301,   166,   116,   120,     0,
     150,   149,   148,   304,     0,     0,   309,     0,     0,   311,
     315,   306,     0,     0,     0,    76,    70,     0,    72,    84,
      73,    60,     0,    67,    68,     0,    64,    65,    71,    74,
      81,    69,    61,     0,     0,    86,     0,     0,    48,     0,
      52,   225,     0,   259,   261,     0,     0,     0,     0,     0,
       0,     0,   209,     0,     0,     0,   182,   172,   201,   202,
       0,   197,     0,     0,     0,   188,     0,   200,   199,   215,
     216,   217,   218,   219,   220,   221,   192,   191,   194,   193,
     195,   196,     0,    35,   339,     0,     0,    40,     0,    23,
     317,   137,   281,   283,     0,   285,   298,   284,   141,   161,
     299,   147,   124,   158,   156,   312,     0,     0,   314,     0,
     307,     0,     0,    47,     0,     0,    66,     0,     0,     0,
      75,     0,     0,     0,    90,    91,     0,    59,    85,    87,
       0,     0,    56,     0,   222,     0,   213,     0,     0,     0,
       0,   207,     0,     0,     0,   255,     0,   198,     0,     0,
       0,   189,   256,   106,   103,    25,     0,     0,   333,   325,
     331,   329,   332,   327,     0,     0,     0,   297,   289,   295,
       0,   131,   310,   315,   313,     0,    50,     0,     0,     0,
       0,     0,     0,     0,    89,    92,    88,     0,    94,   227,
     225,     0,     0,   211,     0,   210,     0,   214,   257,     0,
       0,   205,   203,   298,     0,   328,   330,   326,     0,   282,
     299,     0,   308,    51,    63,    83,     0,    77,    62,    80,
      78,    79,    93,     0,   145,   223,   239,   240,   208,   212,
     206,   204,   286,   322,   334,     0,   143,     0,     0,   230,
       0,     0,     0,     0,   140,    82,   226,   231,   232,   233,
       0,     0,     0,   335,   323,   296,   142,   224,     0,     0,
       0,   238,   228,   258,     0,   237,   235,     0,   236,   234,
     324,     0,   229
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -469,  -469,  -469,   570,  -469,   618,  -469,   325,  -469,   177,
    -469,  -469,  -469,  -469,   332,   -91,   172,  -469,  -469,  -469,
     200,  -469,   273,  -469,   179,  -469,  -469,  -469,  -469,   194,
    -469,  -469,   -44,  -469,  -469,  -469,  -469,  -469,  -469,   526,
    -469,  -469,   424,  -204,   -80,  -469,    56,   -53,   -31,  -469,
    -469,   -73,   387,  -469,  -469,  -469,  -116,  -469,  -469,  -173,
    -469,   329,  -469,  -469,   -69,  -294,  -469,  -273,   337,  -149,
    -190,  -469,  -469,  -469,  -469,  -469,  -469,   386,  -469,  -469,
    -469,   141,  -469,  -469,  -469,  -422,  -469,  -469,  -146,  -469,
    -469,  -469,  -469,  -469,    82,   -75,   -82,  -469,  -469,   -98,
    -469,  -469,  -469,  -469,  -469,  -468,   129,  -469,  -469,  -469,
       1,   542,  -469,   135,   422,  -469,   336,  -469,   421,  -469,
     171,  -469,  -469,  -469,   589,  -469,  -469,  -469,  -469,  -343
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,    21,    22,    74,   255,   256,    23,    67,
      24,   142,    25,    26,    92,   161,   251,    27,    28,    29,
      86,   283,   284,   285,   385,   470,   466,   477,   478,   479,
     286,    30,    96,    31,   248,   249,    32,    33,    34,   153,
      35,   155,   156,    36,   174,   175,   176,    78,   115,   116,
     179,    79,   173,   257,   340,   341,   150,   521,   594,   119,
     263,   264,   352,   111,   184,   258,   130,   131,   259,   260,
     222,   223,   224,   225,   226,   227,   228,   295,   229,   230,
     231,   484,   574,   600,   601,   612,   232,   233,   201,   202,
     203,   234,   235,   236,   237,   238,   133,   134,   135,   136,
     137,   138,   139,   140,   441,   442,   443,   444,   445,    52,
     446,   146,   517,   518,   519,   346,   271,   272,   273,   360,
     460,    37,    38,    64,    65,   447,   514,   604,    72,   241
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     221,   261,    98,    41,   279,   405,    44,   132,   266,   163,
     164,    53,   171,    57,   102,   103,   104,   162,   162,   162,
     462,    40,    40,    90,   593,    76,   298,   393,   300,   149,
     613,   265,   608,   267,   269,   172,   343,   608,   181,   246,
     274,   207,   177,   609,   481,   177,   112,   559,   118,   311,
      89,   311,    93,   412,   182,    45,   495,   435,   457,   415,
     239,   342,   105,   438,    76,    46,   294,   489,    56,   162,
     242,   302,   183,   208,   303,    47,   100,   416,   473,    39,
     356,   413,   113,   390,   280,   303,   458,   459,   147,   148,
     584,   304,   303,    77,   343,   331,   355,   158,   485,   325,
     243,   326,   304,   326,   166,   167,    48,   298,   281,   304,
     507,    97,   474,   275,   523,   410,   101,   411,   525,   114,
     500,   417,   418,   419,   420,   421,   422,   423,   424,   425,
     426,   427,   428,   429,   430,   431,   335,   537,   209,   357,
      40,   245,   282,   397,   398,   396,   262,    61,    51,   121,
     122,   123,   508,   358,   408,   409,   475,   509,   303,    55,
     516,   407,   168,   328,   510,   511,   329,   301,   576,   342,
     211,   121,   122,   123,   476,   304,   211,   121,   122,   123,
     359,   512,   348,   311,    49,  -334,   513,   617,    62,   177,
     112,    91,   347,   610,    68,    69,   611,   353,   610,   622,
     265,   611,   615,   616,   550,   453,   454,   277,   303,   394,
      55,   440,   219,   212,   213,   214,   391,   350,   344,   212,
     213,   214,   497,    54,   448,   304,   113,    42,   432,    58,
     322,   323,   324,   325,   124,   326,   506,   395,   507,    59,
     458,   459,   592,   303,   351,   400,    43,    87,    88,   490,
     125,   492,   493,   215,    50,   132,   124,   303,   117,   215,
     304,   132,   124,   114,   288,   401,   289,   586,    63,   401,
      60,   402,   125,   463,   304,   491,   329,   362,   125,    66,
     508,  -287,   162,   268,   143,   509,   293,   293,   546,   436,
     216,   303,   510,   511,   596,   303,   216,   126,   127,   128,
     211,   121,   122,   123,   217,   303,   548,    70,   304,   512,
     217,    71,   304,  -334,   513,   488,   303,    80,   545,   126,
     127,   128,   304,   618,   619,   126,   127,   128,   433,   337,
      73,   388,   338,   304,   389,   252,   253,   303,   541,   578,
     543,   544,   129,   212,   213,   214,   211,   121,   122,   123,
     494,   218,   219,   503,   304,    76,   188,   218,   219,   220,
      81,   499,    83,   505,   129,   220,   188,    82,    85,    84,
     129,   211,   121,   122,   123,   189,   190,   191,   192,   193,
     194,    94,   526,   215,   538,   329,   124,   329,    95,   212,
     213,   214,   597,   598,   599,   540,   552,   579,   342,   342,
     563,   589,   125,   329,   565,   572,    99,   566,   329,  -288,
     581,   106,   110,   342,   296,   213,   214,   107,   605,   109,
     216,   329,   120,   211,   121,   122,   123,   141,   305,   215,
     145,   144,   124,   149,   217,   151,   152,   154,   157,   159,
     160,   121,   603,   165,   606,   549,    55,   170,   125,   126,
     127,   128,   123,   172,   215,   178,   180,   124,   185,   186,
     507,   187,   188,   205,   206,   306,   216,   213,   214,   240,
     247,   250,   244,   125,   254,   117,   270,   276,   278,    15,
     217,   218,   219,   290,   291,   287,   292,   327,   330,   220,
     299,   297,   333,   334,   129,   126,   127,   128,   336,   339,
     342,   349,   508,   354,   363,   217,   215,   509,   364,   124,
     387,   386,   403,   307,   510,   511,    76,   406,   590,   414,
     126,   127,   128,   434,   450,   125,   455,   218,   219,   456,
     461,   512,   464,   487,   308,   220,   513,   465,   467,   468,
     129,   309,   310,   297,   469,   471,   472,   480,   483,   311,
     312,   412,   218,   219,   486,   501,   303,   217,   365,   498,
     220,   326,   504,   502,   520,   129,   515,   522,   591,   527,
     524,   366,   126,   127,   128,   539,   367,   368,   369,   370,
     371,   528,   372,   529,   313,   314,   315,   316,   317,   530,
     373,   318,   319,   531,   320,   321,   322,   323,   324,   325,
     532,   326,   533,   534,   218,   219,   535,  -336,   547,   551,
     554,   553,   220,   555,     1,   556,   374,   129,   557,   558,
     560,   561,     2,   306,   564,   567,   568,   587,   573,     3,
     588,     1,   621,     4,   375,   569,   376,   377,   570,     2,
     571,   108,    75,   585,     5,   577,     3,     6,     7,   580,
       4,   378,   595,   602,   289,   607,   379,   620,   380,     8,
       9,     5,   482,   439,     6,     7,   437,   542,   381,   332,
      10,   307,   536,    11,   210,   392,     8,     9,   451,   449,
     399,   575,   345,   583,   614,   204,   496,    10,   582,   452,
      11,   306,   404,   361,   562,    12,   169,     0,     0,    13,
     310,     0,   382,     0,     0,     0,     0,   311,   312,     0,
     306,     0,    12,     0,     0,    14,    13,     0,   383,   384,
       0,    15,     0,     0,     0,   306,     0,     0,     0,     0,
       0,     0,    14,     0,     0,     0,     0,     0,    15,   307,
       0,     0,   313,   314,   315,   316,   317,     0,     0,   318,
     319,     0,   320,   321,   322,   323,   324,   325,  -337,   326,
     404,     0,     0,     0,    16,    17,    18,     0,   310,     0,
       0,     0,     0,     0,     0,   311,   312,     0,     0,     0,
       0,    16,    17,    18,     0,     0,     0,   310,     0,     0,
       0,     0,     0,     0,   311,  -337,     0,     0,     0,     0,
       0,     0,   310,     0,     0,     0,     0,     0,     0,   311,
     313,   314,   315,   316,   317,     0,     0,   318,   319,     0,
     320,   321,   322,   323,   324,   325,     0,   326,     0,  -337,
    -337,  -337,   316,   317,     0,     0,   318,   319,     0,   320,
     321,   322,   323,   324,   325,     0,   326,  -337,  -337,     0,
       0,  -337,  -337,     0,   320,   321,   322,   323,   324,   325,
       0,   326,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200
};

static const yytype_int16 yycheck[] =
{
     149,   174,    55,     2,   208,   299,     5,    82,   181,   100,
     101,    10,   110,    12,    58,    59,    60,    99,   100,   101,
     363,     3,     3,     3,    51,    58,   216,     3,   218,    84,
       3,   180,     6,   182,   183,    12,     3,     6,   118,   155,
     186,    95,   115,    17,   387,   118,     9,   515,    79,   138,
      49,   138,    51,   102,    55,    61,   191,   330,   132,   105,
     151,   196,    61,   336,    58,    71,   215,   110,    12,   151,
      64,   220,    73,   127,   117,    81,    95,   123,    27,    26,
      63,   130,    45,   287,     3,   117,   160,   161,    87,    88,
     558,   134,   117,    37,     3,   244,    55,    96,   392,   188,
     153,   190,   134,   190,   103,   104,   112,   297,    27,   134,
      25,    55,    61,   188,   457,   305,   135,   307,   461,    82,
     414,   311,   312,   313,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   252,   480,   192,   122,
       3,   196,    61,   292,   293,   291,   177,    19,    99,     4,
       5,     6,    67,   136,   303,   304,   105,    72,   117,   192,
     127,   193,   106,   193,    79,    80,   196,   220,   193,   196,
       3,     4,     5,     6,   123,   134,     3,     4,     5,     6,
     163,    96,   262,   138,    95,   100,   101,   609,    60,   262,
       9,   171,   261,   167,    17,    18,   170,   266,   167,   621,
     349,   170,   168,   169,   498,   354,   355,   206,   117,   185,
     192,   192,   185,    46,    47,    48,   193,    91,   127,    46,
      47,    48,   412,    81,   340,   134,    45,     3,   326,    71,
     185,   186,   187,   188,    89,   190,   440,   290,    25,    81,
     160,   161,   585,   117,   118,    93,     3,    47,    48,    93,
     105,   400,   401,    86,    95,   330,    89,   117,    77,    86,
     134,   336,    89,    82,   192,   113,   194,   561,     3,   113,
     112,   119,   105,   364,   134,   119,   196,   276,   105,   156,
      67,   196,   364,   116,    84,    72,   113,   113,   110,   333,
     123,   117,    79,    80,   588,   117,   123,   152,   153,   154,
       3,     4,     5,     6,   137,   117,   496,     0,   134,    96,
     137,   195,   134,   100,   101,   127,   117,     3,   119,   152,
     153,   154,   134,   168,   169,   152,   153,   154,   327,   193,
     114,   193,   196,   134,   196,   163,   164,   117,   487,   119,
     489,   490,   197,    46,    47,    48,     3,     4,     5,     6,
     403,   184,   185,   193,   134,    58,   196,   184,   185,   192,
     194,   414,    95,   193,   197,   192,   196,   192,   129,    71,
     197,     3,     4,     5,     6,   140,   141,   142,   143,   144,
     145,   125,   193,    86,   193,   196,    89,   196,   129,    46,
      47,    48,   164,   165,   166,   193,   193,   546,   196,   196,
     193,   574,   105,   196,   193,   193,   135,   196,   196,   196,
     193,   127,   126,   196,    46,    47,    48,   196,   193,    98,
     123,   196,     3,     3,     4,     5,     6,     4,    17,    86,
       3,   123,    89,    84,   137,    94,   192,     3,    46,   193,
     193,     4,   591,     3,   593,   498,   192,   192,   105,   152,
     153,   154,     6,    12,    86,   116,   128,    89,     4,     4,
      25,   193,   196,    46,   133,    54,   123,    47,    48,     3,
      92,   114,   173,   105,     3,    77,   120,   133,     3,   114,
     137,   184,   185,   192,   192,   127,   192,    99,   192,   192,
     190,   123,    39,    49,   197,   152,   153,   154,   192,    95,
     196,   196,    67,    55,   192,   137,    86,    72,    94,    89,
     192,   122,   192,   102,    79,    80,    58,   193,    83,   192,
     152,   153,   154,     3,     3,   105,   163,   184,   185,   122,
     192,    96,   192,    95,   123,   192,   101,   192,   192,   192,
     197,   130,   131,   123,   192,   192,   192,   192,    78,   138,
     139,   102,   184,   185,   193,   105,   117,   137,    11,   192,
     192,   190,     3,   191,    70,   197,   196,    63,   133,     6,
     162,    24,   152,   153,   154,   192,    29,    30,    31,    32,
      33,     6,    35,     6,   173,   174,   175,   176,   177,     6,
      43,   180,   181,     6,   183,   184,   185,   186,   187,   188,
       6,   190,     6,   122,   184,   185,   105,     0,   193,   193,
     100,   193,   192,    79,     7,    79,    69,   197,    79,   100,
       3,   128,    15,    54,   193,   193,   193,     6,   106,    22,
     128,     7,   117,    26,    87,   193,    89,    90,   193,    15,
     193,    71,    24,   192,    37,   193,    22,    40,    41,   193,
      26,   104,   193,   192,   194,   193,   109,   193,   111,    52,
      53,    37,   389,   338,    40,    41,   334,   488,   121,   245,
      63,   102,   478,    66,   148,   288,    52,    53,   349,   342,
     294,   540,   260,   554,   602,   143,   117,    63,   553,   353,
      66,    54,   123,   272,   523,    88,   107,    -1,    -1,    92,
     131,    -1,   155,    -1,    -1,    -1,    -1,   138,   139,    -1,
      54,    -1,    88,    -1,    -1,   108,    92,    -1,   171,   172,
      -1,   114,    -1,    -1,    -1,    54,    -1,    -1,    -1,    -1,
      -1,    -1,   108,    -1,    -1,    -1,    -1,    -1,   114,   102,
      -1,    -1,   173,   174,   175,   176,   177,    -1,    -1,   180,
     181,    -1,   183,   184,   185,   186,   187,   188,   102,   190,
     123,    -1,    -1,    -1,   157,   158,   159,    -1,   131,    -1,
      -1,    -1,    -1,    -1,    -1,   138,   139,    -1,    -1,    -1,
      -1,   157,   158,   159,    -1,    -1,    -1,   131,    -1,    -1,
      -1,    -1,    -1,    -1,   138,   139,    -1,    -1,    -1,    -1,
      -1,    -1,   131,    -1,    -1,    -1,    -1,    -1,    -1,   138,
     173,   174,   175,   176,   177,    -1,    -1,   180,   181,    -1,
     183,   184,   185,   186,   187,   188,    -1,   190,    -1,   173,
     174,   175,   176,   177,    -1,    -1,   180,   181,    -1,   183,
     184,   185,   186,   187,   188,    -1,   190,   176,   177,    -1,
      -1,   180,   181,    -1,   183,   184,   185,   186,   187,   188,
      -1,   190,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,     7,    15,    22,    26,    37,    40,    41,    52,    53,
      63,    66,    88,    92,   108,   114,   157,   158,   159,   199,
     200,   201,   202,   206,   208,   210,   211,   215,   216,   217,
     229,   231,   234,   235,   236,   238,   241,   319,   320,    26,
       3,   308,     3,     3,   308,    61,    71,    81,   112,    95,
      95,    99,   307,   308,    81,   192,   244,   308,    71,    81,
     112,    19,    60,     3,   321,   322,   156,   207,   207,   207,
       0,   195,   326,   114,   203,   203,    58,   244,   245,   249,
       3,   194,   192,    95,    71,   129,   218,   218,   218,   308,
       3,   171,   212,   308,   125,   129,   230,   244,   245,   135,
      95,   135,   230,   230,   230,   308,   127,   196,   201,    98,
     126,   261,     9,    45,    82,   246,   247,    77,   246,   257,
       3,     4,     5,     6,    89,   105,   152,   153,   154,   197,
     264,   265,   293,   294,   295,   296,   297,   298,   299,   300,
     301,     4,   209,   218,   123,     3,   309,   308,   308,    84,
     254,    94,   192,   237,     3,   239,   240,    46,   308,   193,
     193,   213,   294,   213,   213,     3,   308,   308,   244,   322,
     192,   297,    12,   250,   242,   243,   244,   249,   116,   248,
     128,   242,    55,    73,   262,     4,     4,   193,   196,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   286,   287,   288,   309,    46,   133,    95,   127,   192,
     237,     3,    46,    47,    48,    86,   123,   137,   184,   185,
     192,   267,   268,   269,   270,   271,   272,   273,   274,   276,
     277,   278,   284,   285,   289,   290,   291,   292,   293,   213,
       3,   327,    64,   245,   173,   196,   254,    92,   232,   233,
     114,   214,   214,   214,     3,   204,   205,   251,   263,   266,
     267,   257,   246,   258,   259,   267,   257,   267,   116,   267,
     120,   314,   315,   316,   286,   293,   133,   308,     3,   241,
       3,    27,    61,   219,   220,   221,   228,   127,   192,   194,
     192,   192,   192,   113,   267,   275,    46,   123,   268,   190,
     268,   245,   267,   117,   134,    17,    54,   102,   123,   130,
     131,   138,   139,   173,   174,   175,   176,   177,   180,   181,
     183,   184,   185,   186,   187,   188,   190,    99,   193,   196,
     192,   267,   240,    39,    49,   254,   192,   193,   196,    95,
     252,   253,   196,     3,   127,   312,   313,   262,   242,   196,
      91,   118,   260,   262,    55,    55,    63,   122,   136,   163,
     317,   316,   308,   192,    94,    11,    24,    29,    30,    31,
      32,    33,    35,    43,    69,    87,    89,    90,   104,   109,
     111,   121,   155,   171,   172,   222,   122,   192,   193,   196,
     241,   193,   250,     3,   185,   245,   286,   267,   267,   275,
      93,   113,   119,   192,   123,   263,   193,   193,   267,   267,
     268,   268,   102,   130,   192,   105,   123,   268,   268,   268,
     268,   268,   268,   268,   268,   268,   268,   268,   268,   268,
     268,   268,   297,   308,     3,   265,   230,   212,   265,   205,
     192,   302,   303,   304,   305,   306,   308,   323,   254,   266,
       3,   259,   314,   267,   267,   163,   122,   132,   160,   161,
     318,   192,   327,   213,   192,   192,   224,   192,   192,   192,
     223,   192,   192,    27,    61,   105,   123,   225,   226,   227,
     192,   327,   220,    78,   279,   263,   193,    95,   127,   110,
      93,   119,   267,   267,   245,   191,   117,   268,   192,   245,
     263,   105,   191,   193,     3,   193,   241,    25,    67,    72,
      79,    80,    96,   101,   324,   196,   127,   310,   311,   312,
      70,   255,    63,   327,   162,   327,   193,     6,     6,     6,
       6,     6,     6,     6,   122,   105,   227,   327,   193,   192,
     193,   267,   222,   267,   267,   119,   110,   193,   268,   245,
     263,   193,   193,   193,   100,    79,    79,    79,   100,   303,
       3,   128,   318,   193,   193,   193,   196,   193,   193,   193,
     193,   193,   193,   106,   280,   279,   193,   193,   119,   267,
     193,   193,   311,   304,   303,   192,   263,     6,   128,   257,
      83,   133,   327,    51,   256,   193,   263,   164,   165,   166,
     281,   282,   192,   267,   325,   193,   267,   193,     6,    17,
     167,   170,   283,     3,   292,   168,   169,   283,   168,   169,
     193,   117,   283
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int16 yyr1[] =
{
       0,   198,   199,   200,   200,   201,   201,   201,   201,   201,
     202,   202,   202,   202,   202,   202,   202,   202,   202,   202,
     203,   203,   204,   204,   205,   205,   206,   206,   206,   207,
     207,   208,   209,   210,   210,   211,   211,   212,   212,   213,
     214,   214,   215,   215,   216,   216,   216,   217,   217,   217,
     217,   217,   217,   218,   218,   219,   219,   220,   220,   221,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     223,   223,   224,   224,   224,   225,   225,   226,   226,   227,
     227,   227,   227,   228,   228,   229,   229,   229,   229,   230,
     230,   231,   232,   233,   234,   235,   236,   236,   237,   237,
     238,   239,   239,   240,   241,   241,   241,   242,   242,   243,
     243,   244,   244,   245,   245,   246,   247,   247,   247,   248,
     248,   249,   250,   250,   251,   252,   252,   253,   254,   254,
     255,   255,   256,   256,   257,   257,   258,   258,   259,   260,
     260,   260,   261,   261,   262,   262,   262,   262,   262,   262,
     263,   263,   264,   264,   265,   265,   266,   267,   267,   267,
     267,   267,   268,   268,   268,   268,   268,   268,   268,   268,
     268,   268,   268,   269,   269,   270,   270,   270,   270,   270,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   272,   272,   273,   273,   273,   273,   274,   274,   274,
     274,   275,   275,   276,   276,   277,   277,   277,   277,   277,
     277,   277,   278,   278,   279,   279,   280,   280,   281,   281,
     281,   282,   282,   282,   283,   283,   283,   283,   283,   284,
     285,   286,   286,   286,   286,   286,   286,   287,   287,   287,
     287,   287,   287,   288,   288,   289,   290,   291,   292,   292,
     292,   292,   293,   293,   293,   293,   293,   293,   293,   294,
     295,   295,   296,   296,   297,   298,   299,   300,   300,   300,
     301,   302,   302,   303,   303,   304,   304,   305,   305,   306,
     307,   308,   308,   309,   309,   310,   310,   311,   311,   312,
     312,   313,   313,   314,   314,   315,   315,   316,   316,   317,
     317,   317,   317,   318,   318,   318,   319,   319,   320,   321,
     321,   322,   323,   323,   323,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   325,   326,   326,   327,   327
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     3,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       5,     0,     1,     3,     1,     4,     2,     2,     2,     1,
       0,     4,     1,     2,     5,     7,     6,     1,     1,     1,
       3,     0,     5,     5,     2,     3,     2,     8,     7,     6,
       9,    10,     7,     3,     0,     1,     3,     1,     1,     3,
       1,     1,     4,     4,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     4,     4,     4,
       3,     0,     5,     3,     0,     1,     0,     1,     2,     2,
       1,     1,     2,     5,     4,     4,     4,     3,     4,     2,
       0,     5,     1,     4,     4,     2,     8,     5,     3,     0,
       5,     1,     3,     3,     2,     2,     6,     1,     1,     1,
       3,     3,     3,     4,     6,     2,     1,     1,     1,     1,
       0,     7,     1,     0,     1,     1,     0,     2,     2,     0,
       4,     0,     2,     0,     3,     0,     1,     3,     2,     1,
       1,     0,     2,     0,     2,     2,     4,     2,     4,     0,
       1,     3,     1,     0,     1,     3,     2,     1,     1,     1,
       1,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     1,     2,     2,     2,     3,     4,
       1,     3,     3,     3,     3,     3,     3,     3,     4,     3,
       3,     3,     3,     5,     6,     5,     6,     4,     6,     3,
       5,     4,     5,     4,     5,     3,     3,     3,     3,     3,
       3,     3,     4,     6,     6,     0,     3,     0,     2,     5,
       0,     1,     1,     1,     2,     2,     2,     2,     1,     6,
       6,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     5,     1,     3,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     2,
       1,     1,     3,     1,     1,     1,     4,     1,     3,     2,
       1,     1,     3,     1,     0,     1,     5,     1,     0,     2,
       1,     1,     0,     1,     0,     1,     2,     3,     5,     1,
       3,     1,     2,     2,     1,     0,     1,     0,     2,     1,
       3,     3,     4,     6,     8,     1,     2,     1,     2,     1,
       2,     1,     1,     1,     0,     1,     1,     0,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = SQL_HSQL_EMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if(yychar == SQL_HSQL_EMPTY) {                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else {                                                        \
        yyerror (&yylloc, result, scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use SQL_HSQL_error or SQL_HSQL_UNDEF. */
#define YYERRCODE SQL_HSQL_UNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if(N) {                                                           \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else {                                                            \
          (Current).first_line   = (Current).last_line   = YYRHSLOC(Rhs, 0).last_line; \
          (Current).first_column = (Current).last_column = YYRHSLOC(Rhs, 0).last_column; \
        }                                                               \
    while(0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if HSQL_DEBUG

#ifndef YYFPRINTF
	#include <stdio.h> /* INFRINGES ON USER NAME SPACE */
	#define YYFPRINTF fprintf
#endif
#define YYDPRINTF(Args) do { if(yydebug) YYFPRINTF Args; } while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YY_LOCATION_PRINT
#  if defined HSQL_LTYPE_IS_TRIVIAL && HSQL_LTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED static int yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
	int res = 0;
	int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
	if(0 <= yylocp->first_line) {
		res += YYFPRINTF (yyo, "%d", yylocp->first_line);
		if(0 <= yylocp->first_column)
			res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
	}
	if(0 <= yylocp->last_line) {
		if(yylocp->first_line < yylocp->last_line) {
			res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
			if(0 <= end_col)
				res += YYFPRINTF (yyo, ".%d", end_col);
		}
		else if(0 <= end_col && yylocp->first_column < end_col)
			res += YYFPRINTF (yyo, "-%d", end_col);
	}
	return res;
}

#define YY_LOCATION_PRINT(File, Loc) yy_location_print_ (File, &(Loc))

#  else
#   define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#  endif
# endif /* !defined YY_LOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug) {                                                          \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr, Kind, Value, Location, result, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void yy_symbol_value_print(FILE *yyo, yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, hsql::SQLParserResult* result, yyscan_t scanner)
{
	FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  YYUSE (result);
  YYUSE (scanner);
  if(!yyvaluep)
    return;
# ifdef YYPRINT
  if(yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void yy_symbol_print(FILE *yyo, yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, hsql::SQLParserResult* result, yyscan_t scanner)
{
	YYFPRINTF(yyo, "%s %s (", yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));
  YY_LOCATION_PRINT (yyo, *yylocationp);
	YYFPRINTF(yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, result, scanner);
	YYFPRINTF(yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void yy_stack_print(yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++) {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

#define YY_STACK_PRINT(Bottom, Top) do { if(yydebug) yy_stack_print ((Bottom), (Top)); } while(0)

/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,int yyrule, hsql::SQLParserResult* result, yyscan_t scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF(stderr, "Reducing stack by rule %d (line %d):\n", yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for(yyi = 0; yyi < yynrhs; yyi++) {
      YYFPRINTF(stderr, "   $%d = ", yyi + 1);
      yy_symbol_print(stderr, YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]), &yyvsp[(yyi + 1) - (yynrhs)], &(yylsp[(yyi + 1) - (yynrhs)]), result, scanner);
      YYFPRINTF(stderr, "\n");
	}
}

#define YY_REDUCE_PRINT(Rule) do { if(yydebug) yy_reduce_print (yyssp, yyvsp, yylsp, Rule, result, scanner); } while(0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !HSQL_DEBUG */
	#define YYDPRINTF(Args) ((void) 0)
	#define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
	#define YY_STACK_PRINT(Bottom, Top)
	#define YY_REDUCE_PRINT(Rule)
#endif /* !HSQL_DEBUG */
/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
	#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
	#define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int yypcontext_expected_tokens (const yypcontext_t *yyctx, yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if(!yypact_value_is_default (yyn)) {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror && !yytable_value_is_error (yytable[yyx + yyn])) {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T yystrlen(const char *yystr)
{
	YYPTRDIFF_T yylen;
	for(yylen = 0; yystr[yylen]; yylen++)
		continue;
	return yylen;
}
# endif
#endif
#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char * yystpcpy(char *yydest, const char *yysrc)
{
	char * yyd = yydest;
	const char *yys = yysrc;
	while((*yyd++ = *yys++) != '\0')
		continue;
	return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T yytnamerr(char *yyres, const char *yystr)
{
	if(*yystr == '"') {
		YYPTRDIFF_T yyn = 0;
		char const *yyp = yystr;
		for(;;)
			switch (*++yyp) {
				case '\'':
				case ',':
					goto do_not_strip_quotes;
				case '\\':
					if(*++yyp != '\\')
						goto do_not_strip_quotes;
					else
						goto append;
append:
				default:
					if(yyres)
						yyres[yyn] = *yyp;
					yyn++;
					break;
				case '"':
					if(yyres)
						yyres[yyn] = '\0';
					return yyn;
			}
do_not_strip_quotes: ;
	}
	if(yyres)
		return yystpcpy(yyres, yystr) - yyres;
	else
		return yystrlen(yystr);
}
#endif


static int yy_syntax_error_arguments (const yypcontext_t *yyctx, yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if(yyctx->yytoken != YYSYMBOL_YYEMPTY) {
      int yyn;
      if(yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx, yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if(yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg, const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  const char *yyformat = YY_NULLPTR; /* Internationalized format string. */
  yysymbol_kind_t yyarg[YYARGS_MAX]; /* Arguments of yyformat: reported tokens (one for the "unexpected", one per "expected"). */
  YYPTRDIFF_T yysize = 0; /* Cumulated lengths of YYARG.  */
  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments(yyctx, yyarg, YYARGS_MAX);
  if(yycount == YYENOMEM)
    return YYENOMEM;
  switch(yycount) {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for(yyi = 0; yyi < yycount; ++yyi) {
        YYPTRDIFF_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if(yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }
  if(*yymsg_alloc < yysize) {
      *yymsg_alloc = 2 * yysize;
      if(!(yysize <= *yymsg_alloc && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }
  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
	char * yyp = *yymsg;
	int yyi = 0;
	while((*yyp = *yyformat) != '\0')
		if(*yyp == '%' && yyformat[1] == 's' && yyi < yycount) {
			
			yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
			yyformat += 2;
		}
		else {
			++yyp;
			++yyformat;
		}
  }
  return 0;
}


//
// Release the memory associated to this symbol. 
//
static void yydestruct(const char *yymsg, yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, hsql::SQLParserResult* result, yyscan_t scanner)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (result);
  YYUSE (scanner);
  if(!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_IDENTIFIER: /* IDENTIFIER  */
                { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_STRING: /* STRING  */
                { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_FLOATVAL: /* FLOATVAL  */
                { }
        break;

    case YYSYMBOL_INTVAL: /* INTVAL  */
                { }
        break;

    case YYSYMBOL_statement_list: /* statement_list  */
                {
      if(((*yyvaluep).stmt_vec)) {
        for(auto ptr : *(((*yyvaluep).stmt_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).stmt_vec));
    }
        break;

    case YYSYMBOL_statement: /* statement  */
                { delete (((*yyvaluep).statement)); }
        break;

    case YYSYMBOL_preparable_statement: /* preparable_statement  */
                { delete (((*yyvaluep).statement)); }
        break;

    case YYSYMBOL_opt_hints: /* opt_hints  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_hint_list: /* hint_list  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_hint: /* hint  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_transaction_statement: /* transaction_statement  */
                { delete (((*yyvaluep).transaction_stmt)); }
        break;

    case YYSYMBOL_prepare_statement: /* prepare_statement  */
                { delete (((*yyvaluep).prep_stmt)); }
        break;

    case YYSYMBOL_prepare_target_query: /* prepare_target_query  */
                { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_execute_statement: /* execute_statement  */
                { delete (((*yyvaluep).exec_stmt)); }
        break;

    case YYSYMBOL_import_statement: /* import_statement  */
                { delete (((*yyvaluep).import_stmt)); }
        break;

    case YYSYMBOL_file_type: /* file_type  */
                { }
        break;

    case YYSYMBOL_file_path: /* file_path  */
                { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_opt_file_type: /* opt_file_type  */
                { }
        break;

    case YYSYMBOL_export_statement: /* export_statement  */
                { delete (((*yyvaluep).export_stmt)); }
        break;

    case YYSYMBOL_show_statement: /* show_statement  */
                { delete (((*yyvaluep).show_stmt)); }
        break;

    case YYSYMBOL_create_statement: /* create_statement  */
                { delete (((*yyvaluep).create_stmt)); }
        break;

    case YYSYMBOL_opt_not_exists: /* opt_not_exists  */
                { }
        break;

    case YYSYMBOL_table_elem_commalist: /* table_elem_commalist  */
                {
      if(((*yyvaluep).table_element_vec)) {
        for(auto ptr : *(((*yyvaluep).table_element_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).table_element_vec));
    }
        break;

    case YYSYMBOL_table_elem: /* table_elem  */
                { delete (((*yyvaluep).table_element_t)); }
        break;

    case YYSYMBOL_column_def: /* column_def  */
                { delete (((*yyvaluep).column_t)); }
        break;

    case YYSYMBOL_column_type: /* column_type  */
                { }
        break;

    case YYSYMBOL_opt_time_precision: /* opt_time_precision  */
                { }
        break;

    case YYSYMBOL_opt_decimal_specification: /* opt_decimal_specification  */
                { delete (((*yyvaluep).ival_pair)); }
        break;

    case YYSYMBOL_opt_column_constraints: /* opt_column_constraints  */
                { }
        break;

    case YYSYMBOL_column_constraint_set: /* column_constraint_set  */
                { }
        break;

    case YYSYMBOL_column_constraint: /* column_constraint  */
                { }
        break;

    case YYSYMBOL_table_constraint: /* table_constraint  */
                { delete (((*yyvaluep).table_constraint_t)); }
        break;

    case YYSYMBOL_drop_statement: /* drop_statement  */
                { delete (((*yyvaluep).drop_stmt)); }
        break;

    case YYSYMBOL_opt_exists: /* opt_exists  */
                { }
        break;

    case YYSYMBOL_alter_statement: /* alter_statement  */
                { delete (((*yyvaluep).alter_stmt)); }
        break;

    case YYSYMBOL_alter_action: /* alter_action  */
                { delete (((*yyvaluep).alter_action_t)); }
        break;

    case YYSYMBOL_drop_action: /* drop_action  */
                { delete (((*yyvaluep).drop_action_t)); }
        break;

    case YYSYMBOL_delete_statement: /* delete_statement  */
                { delete (((*yyvaluep).delete_stmt)); }
        break;

    case YYSYMBOL_truncate_statement: /* truncate_statement  */
                { delete (((*yyvaluep).delete_stmt)); }
        break;

    case YYSYMBOL_insert_statement: /* insert_statement  */
                { delete (((*yyvaluep).insert_stmt)); }
        break;

    case YYSYMBOL_opt_column_list: /* opt_column_list  */
                {
      if(((*yyvaluep).str_vec)) {
        for(auto ptr : *(((*yyvaluep).str_vec))) {
          SAlloc::F(ptr);
        }
      }
      delete (((*yyvaluep).str_vec));
    }
        break;

    case YYSYMBOL_update_statement: /* update_statement  */
                { delete (((*yyvaluep).update_stmt)); }
        break;

    case YYSYMBOL_update_clause_commalist: /* update_clause_commalist  */
                {
      if(((*yyvaluep).update_vec)) {
        for(auto ptr : *(((*yyvaluep).update_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).update_vec));
    }
        break;

    case YYSYMBOL_update_clause: /* update_clause  */
                { delete (((*yyvaluep).update_t)); }
        break;

    case YYSYMBOL_select_statement: /* select_statement  */
                { delete (((*yyvaluep).select_stmt)); }
        break;

    case YYSYMBOL_select_within_set_operation: /* select_within_set_operation  */
                { delete (((*yyvaluep).select_stmt)); }
        break;

    case YYSYMBOL_select_within_set_operation_no_parentheses: /* select_within_set_operation_no_parentheses  */
                { delete (((*yyvaluep).select_stmt)); }
        break;

    case YYSYMBOL_select_with_paren: /* select_with_paren  */
                { delete (((*yyvaluep).select_stmt)); }
        break;

    case YYSYMBOL_select_no_paren: /* select_no_paren  */
                { delete (((*yyvaluep).select_stmt)); }
        break;

    case YYSYMBOL_set_operator: /* set_operator  */
                { delete (((*yyvaluep).set_operator_t)); }
        break;

    case YYSYMBOL_set_type: /* set_type  */
                { delete (((*yyvaluep).set_operator_t)); }
        break;

    case YYSYMBOL_opt_all: /* opt_all  */
                { }
        break;

    case YYSYMBOL_select_clause: /* select_clause  */
                { delete (((*yyvaluep).select_stmt)); }
        break;

    case YYSYMBOL_opt_distinct: /* opt_distinct  */
                { }
        break;

    case YYSYMBOL_select_list: /* select_list  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_opt_from_clause: /* opt_from_clause  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_from_clause: /* from_clause  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_opt_where: /* opt_where  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_opt_group: /* opt_group  */
                { delete (((*yyvaluep).group_t)); }
        break;

    case YYSYMBOL_opt_having: /* opt_having  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_opt_order: /* opt_order  */
                {
      if(((*yyvaluep).order_vec)) {
        for(auto ptr : *(((*yyvaluep).order_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).order_vec));
    }
        break;

    case YYSYMBOL_order_list: /* order_list  */
                {
      if(((*yyvaluep).order_vec)) {
        for(auto ptr : *(((*yyvaluep).order_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).order_vec));
    }
        break;

    case YYSYMBOL_order_desc: /* order_desc  */
                { delete (((*yyvaluep).order)); }
        break;

    case YYSYMBOL_opt_order_type: /* opt_order_type  */
                { }
        break;

    case YYSYMBOL_opt_top: /* opt_top  */
                { delete (((*yyvaluep).limit)); }
        break;

    case YYSYMBOL_opt_limit: /* opt_limit  */
                { delete (((*yyvaluep).limit)); }
        break;

    case YYSYMBOL_expr_list: /* expr_list  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_opt_literal_list: /* opt_literal_list  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_literal_list: /* literal_list  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_expr_alias: /* expr_alias  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_expr: /* expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_operand: /* operand  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_scalar_expr: /* scalar_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_unary_expr: /* unary_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_binary_expr: /* binary_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_logic_expr: /* logic_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_in_expr: /* in_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_case_expr: /* case_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_case_list: /* case_list  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_exists_expr: /* exists_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_comp_expr: /* comp_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_function_expr: /* function_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_opt_window: /* opt_window  */
                { delete (((*yyvaluep).window_description)); }
        break;

    case YYSYMBOL_opt_partition: /* opt_partition  */
                {
      if(((*yyvaluep).expr_vec)) {
        for(auto ptr : *(((*yyvaluep).expr_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).expr_vec));
    }
        break;

    case YYSYMBOL_opt_frame_clause: /* opt_frame_clause  */
                { delete (((*yyvaluep).frame_description)); }
        break;

    case YYSYMBOL_frame_type: /* frame_type  */
                { }
        break;

    case YYSYMBOL_frame_bound: /* frame_bound  */
                { delete (((*yyvaluep).frame_bound)); }
        break;

    case YYSYMBOL_extract_expr: /* extract_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_cast_expr: /* cast_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_datetime_field: /* datetime_field  */
                { }
        break;

    case YYSYMBOL_datetime_field_plural: /* datetime_field_plural  */
                { }
        break;

    case YYSYMBOL_duration_field: /* duration_field  */
                { }
        break;

    case YYSYMBOL_array_expr: /* array_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_array_index: /* array_index  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_between_expr: /* between_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_column_name: /* column_name  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_literal: /* literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_string_literal: /* string_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_bool_literal: /* bool_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_num_literal: /* num_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_int_literal: /* int_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_null_literal: /* null_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_date_literal: /* date_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_interval_literal: /* interval_literal  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_param_expr: /* param_expr  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_table_ref: /* table_ref  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_table_ref_atomic: /* table_ref_atomic  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_nonjoin_table_ref_atomic: /* nonjoin_table_ref_atomic  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_table_ref_commalist: /* table_ref_commalist  */
                {
      if(((*yyvaluep).table_vec)) {
        for(auto ptr : *(((*yyvaluep).table_vec))) {
          delete ptr;
        }
      }
      delete (((*yyvaluep).table_vec));
    }
        break;

    case YYSYMBOL_table_ref_name: /* table_ref_name  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_table_ref_name_no_alias: /* table_ref_name_no_alias  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_table_name: /* table_name  */
                {
      SAlloc::F((((*yyvaluep).table_name).name));
      SAlloc::F((((*yyvaluep).table_name).schema));
    }
        break;

    case YYSYMBOL_opt_index_name: /* opt_index_name  */
                { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_table_alias: /* table_alias  */
                { delete (((*yyvaluep).alias_t)); }
        break;

    case YYSYMBOL_opt_table_alias: /* opt_table_alias  */
                { delete (((*yyvaluep).alias_t)); }
        break;

    case YYSYMBOL_alias: /* alias  */
                { delete (((*yyvaluep).alias_t)); }
        break;

    case YYSYMBOL_opt_alias: /* opt_alias  */
                { delete (((*yyvaluep).alias_t)); }
        break;

    case YYSYMBOL_opt_locking_clause: /* opt_locking_clause  */
                { delete (((*yyvaluep).locking_clause_vec)); }
        break;

    case YYSYMBOL_opt_locking_clause_list: /* opt_locking_clause_list  */
                { delete (((*yyvaluep).locking_clause_vec)); }
        break;

    case YYSYMBOL_locking_clause: /* locking_clause  */
                { delete (((*yyvaluep).locking_t)); }
        break;

    case YYSYMBOL_row_lock_mode: /* row_lock_mode  */
                { }
        break;

    case YYSYMBOL_opt_row_lock_policy: /* opt_row_lock_policy  */
                { }
        break;

    case YYSYMBOL_opt_with_clause: /* opt_with_clause  */
                { delete (((*yyvaluep).with_description_vec)); }
        break;

    case YYSYMBOL_with_clause: /* with_clause  */
                { delete (((*yyvaluep).with_description_vec)); }
        break;

    case YYSYMBOL_with_description_list: /* with_description_list  */
                { delete (((*yyvaluep).with_description_vec)); }
        break;

    case YYSYMBOL_with_description: /* with_description  */
                { delete (((*yyvaluep).with_description_t)); }
        break;

    case YYSYMBOL_join_clause: /* join_clause  */
                { delete (((*yyvaluep).table)); }
        break;

    case YYSYMBOL_opt_join_type: /* opt_join_type  */
                { }
        break;

    case YYSYMBOL_join_condition: /* join_condition  */
                { delete (((*yyvaluep).expr)); }
        break;

    case YYSYMBOL_ident_commalist: /* ident_commalist  */
                {
      if(((*yyvaluep).str_vec)) {
        for(auto ptr : *(((*yyvaluep).str_vec))) {
          SAlloc::F(ptr);
        }
      }
      delete (((*yyvaluep).str_vec));
    }
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






// 
// yyparse.
// 
int yyparse(hsql::SQLParserResult* result, yyscan_t scanner)
{
int yychar; /* Lookahead token kind.  */


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined HSQL_LTYPE_IS_TRIVIAL && HSQL_LTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    int yyerrstatus = 0; /* Number of tokens to shift before error messages enabled.  */
    /* Refer to the stacks through separate pointers, to allow yyoverflow to reallocate them elsewhere.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH; /* Their size.  */
    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;
    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;
    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  int yyresult; /* The return value of yyparse.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY; /* Lookahead symbol kind.  */
  YYSTYPE yyval; /* The variables used to return semantic value and location from the action routines.  */
  YYLTYPE yyloc;
  YYLTYPE yyerror_range[3]; /* The locations where the error started and ended.  */
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = SQL_HSQL_EMPTY; /* Cause a token to be read.  */

/* User initialization code.  */
{
  // Initialize
  yylloc.first_column = 0;
  yylloc.last_column = 0;
  yylloc.first_line = 0;
  yylloc.last_line = 0;
  yylloc.total_column = 0;
  yylloc.string_length = 0;
}


  yylsp[0] = yylloc;
  goto yysetstate;
/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;
/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
	YYDPRINTF((stderr, "Entering state %d\n", yystate));
	YY_ASSERT(0 <= yystate && yystate < YYNSTATES);
	YY_IGNORE_USELESS_CAST_BEGIN
	*yyssp = YY_CAST (yy_state_t, yystate);
	YY_IGNORE_USELESS_CAST_END
	YY_STACK_PRINT(yyss, yyssp);
	if(yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
		goto yyexhaustedlab;
#else
	{
		/* Get the current used size of the three stacks, in elements.  */
		YYPTRDIFF_T yysize = yyssp - yyss + 1;
# if defined yyoverflow
		{
			// Give user a chance to reallocate the stack.  Use copies of
			// these so that the &'s don't force the real ones into memory.
			yy_state_t *yyss1 = yyss;
			YYSTYPE *yyvs1 = yyvs;
			YYLTYPE *yyls1 = yyls;
			// Each stack pointer address is followed by the size of the
			// data in use in that stack, in bytes.  This used to be a
			// conditional around just the two extra args, but that might
			// be undefined if yyoverflow is a macro.
			yyoverflow(YY_("memory exhausted"), &yyss1, yysize * YYSIZEOF(*yyssp), &yyvs1, yysize * YYSIZEOF(*yyvsp),&yyls1, yysize * YYSIZEOF(*yylsp),&yystacksize);
			yyss = yyss1;
			yyvs = yyvs1;
			yyls = yyls1;
		}
# else /* defined YYSTACK_RELOCATE */
		/* Extend the stack our own way.  */
		if(YYMAXDEPTH <= yystacksize)
			goto yyexhaustedlab;
		yystacksize *= 2;
		if(YYMAXDEPTH < yystacksize)
			yystacksize = YYMAXDEPTH;
		{
			yy_state_t *yyss1 = yyss;
			union yyalloc *yyptr = YY_CAST(union yyalloc *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
			if(!yyptr)
				goto yyexhaustedlab;
			YYSTACK_RELOCATE(yyss_alloc, yyss);
			YYSTACK_RELOCATE(yyvs_alloc, yyvs);
			YYSTACK_RELOCATE(yyls_alloc, yyls);
	#undef YYSTACK_RELOCATE
			if(yyss1 != yyssa)
				YYSTACK_FREE(yyss1);
		}
# endif
		yyssp = yyss + yysize - 1;
		yyvsp = yyvs + yysize - 1;
		yylsp = yyls + yysize - 1;
		YY_IGNORE_USELESS_CAST_BEGIN
		YYDPRINTF ((stderr, "Stack size increased to %ld\n", YY_CAST (long, yystacksize)));
		YY_IGNORE_USELESS_CAST_END
		if(yyss + yystacksize - 1 <= yyssp)
			YYABORT;
	}
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */
	if(yystate == YYFINAL)
		YYACCEPT;
	goto yybackup;
/*-----------.
| yybackup.  |
`-----------*/
yybackup:
	/* Do appropriate processing given the current state.  Read a
	lookahead token if we need one and don't already have one.  */
	/* First try to decide what to do without reference to lookahead token.  */
	yyn = yypact[yystate];
	if(yypact_value_is_default (yyn))
		goto yydefault;
  /* Not known => get a lookahead token if don't already have one.  */
  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if(yychar == SQL_HSQL_EMPTY)
    {
      YYDPRINTF((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }
  if(yychar <= SQL_YYEOF) {
      yychar = SQL_YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == SQL_HSQL_error) {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = SQL_HSQL_UNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }
  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if(yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if(yyn <= 0) {
      if(yytable_value_is_error(yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  // Count tokens shifted since error; after three, turn off error status. 
  if(yyerrstatus)
    yyerrstatus--;
  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  /* Discard the shifted token.  */
  yychar = SQL_HSQL_EMPTY;
  goto yynewstate;

/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if(yyn == 0)
    goto yyerrlab;
  goto yyreduce;
/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];
  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];
  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT(yyn);
  switch(yyn) {
	case 2: /* input: statement_list opt_semicolon  */
	                                     {
  for (SQLStatement* stmt : *(yyvsp[-1].stmt_vec)) {
    // Transfers ownership of the statement.
    result->addStatement(stmt);
  }

  unsigned param_id = 0;
  for (void* param : yyloc.param_list) {
    if(param) {
      Expr* expr = (Expr*)param;
      expr->ival = param_id;
      result->addParameter(expr);
      ++param_id;
    }
  }
    delete (yyvsp[-1].stmt_vec);
  }
		break;

	case 3: /* statement_list: statement  */
	                           {
  (yyvsp[0].statement)->stringLength = yylloc.string_length;
  yylloc.string_length = 0;
  (yyval.stmt_vec) = new std::vector<SQLStatement*>();
  (yyval.stmt_vec)->push_back((yyvsp[0].statement));
}
		break;

	case 4: /* statement_list: statement_list ';' statement  */
	                               {
  (yyvsp[0].statement)->stringLength = yylloc.string_length;
  yylloc.string_length = 0;
  (yyvsp[-2].stmt_vec)->push_back((yyvsp[0].statement));
  (yyval.stmt_vec) = (yyvsp[-2].stmt_vec);
}
		break;

	case 5: /* statement: prepare_statement opt_hints  */
	                                        {
  (yyval.statement) = (yyvsp[-1].prep_stmt);
  (yyval.statement)->hints = (yyvsp[0].expr_vec);
}
		break;

	case 6: /* statement: preparable_statement opt_hints  */
	                                 {
  (yyval.statement) = (yyvsp[-1].statement);
  (yyval.statement)->hints = (yyvsp[0].expr_vec);
}
		break;

	case 7: /* statement: show_statement  */
	                 { (yyval.statement) = (yyvsp[0].show_stmt); }
		break;

	case 8: /* statement: import_statement  */
	                   { (yyval.statement) = (yyvsp[0].import_stmt); }
		break;

	case 9: /* statement: export_statement  */
	                   { (yyval.statement) = (yyvsp[0].export_stmt); }
		break;

	case 10: /* preparable_statement: select_statement  */
	                                        { (yyval.statement) = (yyvsp[0].select_stmt); }
		break;

	case 11: /* preparable_statement: create_statement  */
	                   { (yyval.statement) = (yyvsp[0].create_stmt); }
		break;

	case 12: /* preparable_statement: insert_statement  */
	                   { (yyval.statement) = (yyvsp[0].insert_stmt); }
		break;

	case 13: /* preparable_statement: delete_statement  */
	                   { (yyval.statement) = (yyvsp[0].delete_stmt); }
		break;

	case 14: /* preparable_statement: truncate_statement  */
	                     { (yyval.statement) = (yyvsp[0].delete_stmt); }
		break;

	case 15: /* preparable_statement: update_statement  */
	                   { (yyval.statement) = (yyvsp[0].update_stmt); }
		break;

	case 16: /* preparable_statement: drop_statement  */
	                 { (yyval.statement) = (yyvsp[0].drop_stmt); }
		break;

	case 17: /* preparable_statement: alter_statement  */
	                  { (yyval.statement) = (yyvsp[0].alter_stmt); }
		break;

	case 18: /* preparable_statement: execute_statement  */
	                    { (yyval.statement) = (yyvsp[0].exec_stmt); }
		break;

	case 19: /* preparable_statement: transaction_statement  */
	                        { (yyval.statement) = (yyvsp[0].transaction_stmt); }
		break;

	case 20: /* opt_hints: WITH HINT '(' hint_list ')'  */
	                                        { (yyval.expr_vec) = (yyvsp[-1].expr_vec); }
		break;

	case 21: /* opt_hints: %empty  */
	              { (yyval.expr_vec) = nullptr; }
		break;

	case 22: /* hint_list: hint  */
	                 {
  (yyval.expr_vec) = new std::vector<Expr*>();
  (yyval.expr_vec)->push_back((yyvsp[0].expr));
}
		break;

	case 23: /* hint_list: hint_list ',' hint  */
	                     {
  (yyvsp[-2].expr_vec)->push_back((yyvsp[0].expr));
  (yyval.expr_vec) = (yyvsp[-2].expr_vec);
}
		break;

	case 24: /* hint: IDENTIFIER  */
	                  {
  (yyval.expr) = Expr::make(kExprHint);
  (yyval.expr)->name = (yyvsp[0].sval);
}
		break;

	case 25: /* hint: IDENTIFIER '(' literal_list ')'  */
	                                  {
  (yyval.expr) = Expr::make(kExprHint);
  (yyval.expr)->name = (yyvsp[-3].sval);
  (yyval.expr)->exprList = (yyvsp[-1].expr_vec);
}
		break;

	case 26: /* transaction_statement: BEGIN opt_transaction_keyword  */
	                                                      { (yyval.transaction_stmt) = new TransactionStatement(kBeginTransaction); }
		break;

	case 27: /* transaction_statement: ROLLBACK opt_transaction_keyword  */
	                                   { (yyval.transaction_stmt) = new TransactionStatement(kRollbackTransaction); }
		break;

	case 28: /* transaction_statement: COMMIT opt_transaction_keyword  */
	                                 { (yyval.transaction_stmt) = new TransactionStatement(kCommitTransaction); }
		break;

	case 31: /* prepare_statement: PREPARE IDENTIFIER FROM prepare_target_query  */
	                                                                 {
  (yyval.prep_stmt) = new PrepareStatement();
  (yyval.prep_stmt)->name = (yyvsp[-2].sval);
  (yyval.prep_stmt)->query = (yyvsp[0].sval);
}
		break;

	case 33: /* execute_statement: EXECUTE IDENTIFIER  */
	                                                                  {
  (yyval.exec_stmt) = new ExecuteStatement();
  (yyval.exec_stmt)->name = (yyvsp[0].sval);
}
		break;

	case 34: /* execute_statement: EXECUTE IDENTIFIER '(' opt_literal_list ')'  */
	                                              {
  (yyval.exec_stmt) = new ExecuteStatement();
  (yyval.exec_stmt)->name = (yyvsp[-3].sval);
  (yyval.exec_stmt)->parameters = (yyvsp[-1].expr_vec);
}
		break;

	case 35: /* import_statement: IMPORT FROM file_type FILE file_path INTO table_name  */
	                                                                        {
    (yyval.import_stmt) = new ImportStatement((yyvsp[-4].import_type_t));
    (yyval.import_stmt)->filePath = (yyvsp[-2].sval);
    (yyval.import_stmt)->schema = (yyvsp[0].table_name).schema;
    (yyval.import_stmt)->tableName = (yyvsp[0].table_name).name;
}
		break;

	case 36: /* import_statement: COPY table_name FROM file_path opt_file_type opt_where  */
	                                                         {
    (yyval.import_stmt) = new ImportStatement((yyvsp[-1].import_type_t));
    (yyval.import_stmt)->filePath = (yyvsp[-2].sval);
    (yyval.import_stmt)->schema = (yyvsp[-4].table_name).schema;
    (yyval.import_stmt)->tableName = (yyvsp[-4].table_name).name;
    (yyval.import_stmt)->whereClause = (yyvsp[0].expr);
}
		break;

	case 37: /* file_type: IDENTIFIER  */
	                       {
  if(sstreqi_ascii((yyvsp[0].sval), "csv")) {
    (yyval.import_type_t) = kImportCSV;
  } 
  else if(sstreqi_ascii((yyvsp[0].sval), "tbl")) {
    (yyval.import_type_t) = kImportTbl;
  } 
  else if(sstreqi_ascii((yyvsp[0].sval), "binary") || sstreqi_ascii((yyvsp[0].sval), "bin")) {
    (yyval.import_type_t) = kImportBinary;
  } 
  else {
    SAlloc::F((yyvsp[0].sval));
    yyerror(&yyloc, result, scanner, "File type is unknown.");
    YYERROR;
  }
  SAlloc::F((yyvsp[0].sval));
}
		break;

	case 38: /* file_type: BINARY  */
	           { // @sobolev
    (yyval.import_type_t) = kImportBinary;
}
		break;

	case 39: /* file_path: string_literal  */
	                           {
    (yyval.sval) = _strdup((yyvsp[0].expr)->name);
    delete (yyvsp[0].expr);
}
		break;

	case 40: /* opt_file_type: WITH FORMAT file_type  */
	                                      { (yyval.import_type_t) = (yyvsp[0].import_type_t); }
		break;

	case 41: /* opt_file_type: %empty  */
	                                                                 { (yyval.import_type_t) = kImportAuto; }
		break;

	case 42: /* export_statement: COPY table_name TO file_path opt_file_type  */
	                                                              {
    (yyval.export_stmt) = new ExportStatement((yyvsp[0].import_type_t));
    (yyval.export_stmt)->filePath = (yyvsp[-1].sval);
    (yyval.export_stmt)->schema = (yyvsp[-3].table_name).schema;
    (yyval.export_stmt)->tableName = (yyvsp[-3].table_name).name;
}
		break;

	case 43: /* export_statement: COPY select_with_paren TO file_path opt_file_type  */
	                                                    {
    (yyval.export_stmt) = new ExportStatement((yyvsp[0].import_type_t));
    (yyval.export_stmt)->filePath = (yyvsp[-1].sval);
    (yyval.export_stmt)->select = (yyvsp[-3].select_stmt);
}
		break;

	case 44: /* show_statement: SHOW TABLES  */
	                             { (yyval.show_stmt) = new ShowStatement(kShowTables); }
		break;

	case 45: /* show_statement: SHOW COLUMNS table_name  */
	                          {
  (yyval.show_stmt) = new ShowStatement(kShowColumns);
  (yyval.show_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.show_stmt)->name = (yyvsp[0].table_name).name;
}
		break;

	case 46: /* show_statement: DESCRIBE table_name  */
	                      {
  (yyval.show_stmt) = new ShowStatement(kShowColumns);
  (yyval.show_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.show_stmt)->name = (yyvsp[0].table_name).name;
}
		break;

	case 47: /* create_statement: CREATE TABLE opt_not_exists table_name FROM IDENTIFIER FILE file_path  */
	                                                                                         {
  (yyval.create_stmt) = new CreateStatement(kCreateTableFromTbl);
  (yyval.create_stmt)->ifNotExists = (yyvsp[-5].bval);
  (yyval.create_stmt)->schema = (yyvsp[-4].table_name).schema;
  (yyval.create_stmt)->tableName = (yyvsp[-4].table_name).name;
  if(!sstreqi_ascii((yyvsp[-2].sval), "tbl")) {
    SAlloc::F((yyvsp[-2].sval));
    yyerror(&yyloc, result, scanner, "File type is unknown.");
    YYERROR;
  }
  SAlloc::F((yyvsp[-2].sval));
  (yyval.create_stmt)->filePath = (yyvsp[0].sval);
}
		break;

	case 48: /* create_statement: CREATE TABLE opt_not_exists table_name '(' table_elem_commalist ')'  */
	                                                                      {
  (yyval.create_stmt) = new CreateStatement(kCreateTable);
  (yyval.create_stmt)->ifNotExists = (yyvsp[-4].bval);
  (yyval.create_stmt)->schema = (yyvsp[-3].table_name).schema;
  (yyval.create_stmt)->tableName = (yyvsp[-3].table_name).name;
  (yyval.create_stmt)->setColumnDefsAndConstraints((yyvsp[-1].table_element_vec));
  delete (yyvsp[-1].table_element_vec);
  if(result->errorMsg()) {
    delete (yyval.create_stmt);
    YYERROR;
  }
}
		break;

	case 49: /* create_statement: CREATE TABLE opt_not_exists table_name AS select_statement  */
	                                                             {
  (yyval.create_stmt) = new CreateStatement(kCreateTable);
  (yyval.create_stmt)->ifNotExists = (yyvsp[-3].bval);
  (yyval.create_stmt)->schema = (yyvsp[-2].table_name).schema;
  (yyval.create_stmt)->tableName = (yyvsp[-2].table_name).name;
  (yyval.create_stmt)->select = (yyvsp[0].select_stmt);
}
		break;

	case 50: /* create_statement: CREATE INDEX opt_not_exists opt_index_name ON table_name '(' ident_commalist ')'  */
	                                                                                   {
    (yyval.create_stmt) = new CreateStatement(kCreateIndex);
    (yyval.create_stmt)->indexName = (yyvsp[-5].sval);
    (yyval.create_stmt)->ifNotExists = (yyvsp[-6].bval);
    (yyval.create_stmt)->fUnique = false;
    (yyval.create_stmt)->tableName = (yyvsp[-3].table_name).name;
    (yyval.create_stmt)->indexColumns = (yyvsp[-1].str_vec);
}
		break;

	case 51: /* create_statement: CREATE UNIQUE INDEX opt_not_exists opt_index_name ON table_name '(' ident_commalist ')'  */
	                                                                                          { // @sobolev
    (yyval.create_stmt) = new CreateStatement(kCreateIndex);
    (yyval.create_stmt)->indexName = (yyvsp[-5].sval);
    (yyval.create_stmt)->ifNotExists = (yyvsp[-6].bval);
    (yyval.create_stmt)->fUnique = true;
    (yyval.create_stmt)->tableName = (yyvsp[-3].table_name).name;
    (yyval.create_stmt)->indexColumns = (yyvsp[-1].str_vec);
}
		break;

	case 52: /* create_statement: CREATE VIEW opt_not_exists table_name opt_column_list AS select_statement  */
	                                                                            {
  (yyval.create_stmt) = new CreateStatement(kCreateView);
  (yyval.create_stmt)->ifNotExists = (yyvsp[-4].bval);
  (yyval.create_stmt)->schema = (yyvsp[-3].table_name).schema;
  (yyval.create_stmt)->tableName = (yyvsp[-3].table_name).name;
  (yyval.create_stmt)->viewColumns = (yyvsp[-2].str_vec);
  (yyval.create_stmt)->select = (yyvsp[0].select_stmt);
}
		break;

	case 53: /* opt_not_exists: IF NOT EXISTS  */
	                               { (yyval.bval) = true; }
		break;

	case 54: /* opt_not_exists: %empty  */
	                                                            { (yyval.bval) = false; }
		break;

	case 55: /* table_elem_commalist: table_elem  */
	                                  {
  (yyval.table_element_vec) = new std::vector<TableElement*>();
  (yyval.table_element_vec)->push_back((yyvsp[0].table_element_t));
}
		break;

	case 56: /* table_elem_commalist: table_elem_commalist ',' table_elem  */
	                                      {
  (yyvsp[-2].table_element_vec)->push_back((yyvsp[0].table_element_t));
  (yyval.table_element_vec) = (yyvsp[-2].table_element_vec);
}
		break;

	case 57: /* table_elem: column_def  */
	                        { (yyval.table_element_t) = (yyvsp[0].column_t); }
		break;

	case 58: /* table_elem: table_constraint  */
	                                                        { (yyval.table_element_t) = (yyvsp[0].table_constraint_t); }
		break;

	case 59: /* column_def: IDENTIFIER column_type opt_column_constraints  */
	                                                           {
  (yyval.column_t) = new ColumnDefinition((yyvsp[-2].sval), (yyvsp[-1].column_type_t), (yyvsp[0].column_constraint_set));
  if(!(yyval.column_t)->trySetNullableExplicit()) {
    yyerror(&yyloc, result, scanner, ("Conflicting nullability constraints for " + std::string{(yyvsp[-2].sval)}).c_str());
  }
}
		break;

	case 60: /* column_type: BIGINT  */
	                     { (yyval.column_type_t) = ColumnType{hsql::DataType::BIGINT}; }
		break;

	case 61: /* column_type: BOOLEAN  */
	          { (yyval.column_type_t) = ColumnType{hsql::DataType::BOOLEAN}; }
		break;

	case 62: /* column_type: CHAR '(' INTVAL ')'  */
	                      { (yyval.column_type_t) = ColumnType{hsql::DataType::CHAR, (yyvsp[-1].ival)}; }
		break;

	case 63: /* column_type: CHARACTER_VARYING '(' INTVAL ')'  */
	                                   { (yyval.column_type_t) = ColumnType{hsql::DataType::VARCHAR, (yyvsp[-1].ival)}; }
		break;

	case 64: /* column_type: DATE  */
	       { (yyval.column_type_t) = ColumnType{hsql::DataType::DATE}; }
		break;

	case 65: /* column_type: DATETIME  */
	           { (yyval.column_type_t) = ColumnType{hsql::DataType::DATETIME}; }
		break;

	case 66: /* column_type: DECIMAL opt_decimal_specification  */
	                                    {
  (yyval.column_type_t) = ColumnType{hsql::DataType::DECIMAL, 0, (yyvsp[0].ival_pair)->first, (yyvsp[0].ival_pair)->second};
  delete (yyvsp[0].ival_pair);
}
		break;

	case 67: /* column_type: DOUBLE  */
	         { (yyval.column_type_t) = ColumnType{hsql::DataType::DOUBLE}; }
		break;

	case 68: /* column_type: FLOAT  */
	        { (yyval.column_type_t) = ColumnType{hsql::DataType::FLOAT}; }
		break;

	case 69: /* column_type: INT  */
	      { (yyval.column_type_t) = ColumnType{hsql::DataType::INT}; }
		break;

	case 70: /* column_type: INTEGER  */
	          { (yyval.column_type_t) = ColumnType{hsql::DataType::INT}; }
		break;

	case 71: /* column_type: LONG  */
	       { (yyval.column_type_t) = ColumnType{hsql::DataType::LONG}; }
		break;

	case 72: /* column_type: REAL  */
	       { (yyval.column_type_t) = ColumnType{hsql::DataType::REAL}; }
		break;

	case 73: /* column_type: SMALLINT  */
	           { (yyval.column_type_t) = ColumnType{hsql::DataType::SMALLINT}; }
		break;

	case 74: /* column_type: TEXT  */
	       { (yyval.column_type_t) = ColumnType{hsql::DataType::TEXT}; }
		break;

	case 75: /* column_type: TIME opt_time_precision  */
	                          { (yyval.column_type_t) = ColumnType{hsql::DataType::TIME, 0, (yyvsp[0].ival)}; }
		break;

	case 76: /* column_type: TIMESTAMP  */
	            { (yyval.column_type_t) = ColumnType{hsql::DataType::DATETIME}; }
		break;

	case 77: /* column_type: VARCHAR '(' INTVAL ')'  */
	                         { (yyval.column_type_t) = ColumnType{hsql::DataType::VARCHAR, (yyvsp[-1].ival)}; }
		break;

	case 78: /* column_type: BINARY '(' INTVAL ')'  */
	                        { (yyval.column_type_t) = ColumnType{hsql::DataType::BINARY, (yyvsp[-1].ival)}; }
		break;

	case 79: /* column_type: RAW '(' INTVAL ')'  */
	                     { (yyval.column_type_t) = ColumnType{hsql::DataType::BINARY, (yyvsp[-1].ival)}; }
		break;

	case 80: /* opt_time_precision: '(' INTVAL ')'  */
	                                    { (yyval.ival) = (yyvsp[-1].ival); }
		break;

	case 81: /* opt_time_precision: %empty  */
	                                                               { (yyval.ival) = 0; }
		break;

	case 82: /* opt_decimal_specification: '(' INTVAL ',' INTVAL ')'  */
	                                                      { (yyval.ival_pair) = new std::pair<int64_t, int64_t>{(yyvsp[-3].ival), (yyvsp[-1].ival)}; }
		break;

	case 83: /* opt_decimal_specification: '(' INTVAL ')'  */
	                 { (yyval.ival_pair) = new std::pair<int64_t, int64_t>{(yyvsp[-1].ival), 0}; }
		break;

	case 84: /* opt_decimal_specification: %empty  */
	              { (yyval.ival_pair) = new std::pair<int64_t, int64_t>{0, 0}; }
		break;

	case 85: /* opt_column_constraints: column_constraint_set  */
	                                               { (yyval.column_constraint_set) = (yyvsp[0].column_constraint_set); }
		break;

	case 86: /* opt_column_constraints: %empty  */
	                                                                          { (yyval.column_constraint_set) = new std::unordered_set<ConstraintType>(); }
		break;

	case 87: /* column_constraint_set: column_constraint  */
	                                          {
  (yyval.column_constraint_set) = new std::unordered_set<ConstraintType>();
  (yyval.column_constraint_set)->insert((yyvsp[0].column_constraint_t));
}
		break;

	case 88: /* column_constraint_set: column_constraint_set column_constraint  */
	                                          {
  (yyvsp[-1].column_constraint_set)->insert((yyvsp[0].column_constraint_t));
  (yyval.column_constraint_set) = (yyvsp[-1].column_constraint_set);
}
		break;

	case 89: /* column_constraint: PRIMARY KEY  */
	                                { (yyval.column_constraint_t) = ConstraintType::PrimaryKey; }
		break;

	case 90: /* column_constraint: UNIQUE  */
	         { (yyval.column_constraint_t) = ConstraintType::Unique; }
		break;

	case 91: /* column_constraint: NULL  */
	       { (yyval.column_constraint_t) = ConstraintType::Null; }
		break;

	case 92: /* column_constraint: NOT NULL  */
	           { (yyval.column_constraint_t) = ConstraintType::NotNull; }
		break;

	case 93: /* table_constraint: PRIMARY KEY '(' ident_commalist ')'  */
	                                                       { (yyval.table_constraint_t) = new TableConstraint(ConstraintType::PrimaryKey, (yyvsp[-1].str_vec)); }
		break;

	case 94: /* table_constraint: UNIQUE '(' ident_commalist ')'  */
	                                 { (yyval.table_constraint_t) = new TableConstraint(ConstraintType::Unique, (yyvsp[-1].str_vec)); }
		break;

	case 95: /* drop_statement: DROP TABLE opt_exists table_name  */
	                                                  {
  (yyval.drop_stmt) = new DropStatement(kDropTable);
  (yyval.drop_stmt)->ifExists = (yyvsp[-1].bval);
  (yyval.drop_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.drop_stmt)->name = (yyvsp[0].table_name).name;
}
		break;

	case 96: /* drop_statement: DROP VIEW opt_exists table_name  */
	                                  {
  (yyval.drop_stmt) = new DropStatement(kDropView);
  (yyval.drop_stmt)->ifExists = (yyvsp[-1].bval);
  (yyval.drop_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.drop_stmt)->name = (yyvsp[0].table_name).name;
}
		break;

	case 97: /* drop_statement: DEALLOCATE PREPARE IDENTIFIER  */
	                                {
  (yyval.drop_stmt) = new DropStatement(kDropPreparedStatement);
  (yyval.drop_stmt)->ifExists = false;
  (yyval.drop_stmt)->name = (yyvsp[0].sval);
}
		break;

	case 98: /* drop_statement: DROP INDEX opt_exists IDENTIFIER  */
	                                   {
  (yyval.drop_stmt) = new DropStatement(kDropIndex);
  (yyval.drop_stmt)->ifExists = (yyvsp[-1].bval);
  (yyval.drop_stmt)->indexName = (yyvsp[0].sval);
}
		break;

	case 99: /* opt_exists: IF EXISTS  */
	                       { (yyval.bval) = true; }
		break;

	case 100: /* opt_exists: %empty  */
	                                                    { (yyval.bval) = false; }
		break;

	case 101: /* alter_statement: ALTER TABLE opt_exists table_name alter_action  */
	                                                                 {
  (yyval.alter_stmt) = new AlterStatement((yyvsp[-1].table_name).name, (yyvsp[0].alter_action_t));
  (yyval.alter_stmt)->ifTableExists = (yyvsp[-2].bval);
  (yyval.alter_stmt)->schema = (yyvsp[-1].table_name).schema;
}
		break;

	case 102: /* alter_action: drop_action  */
	                           { (yyval.alter_action_t) = (yyvsp[0].drop_action_t); }
		break;

	case 103: /* drop_action: DROP COLUMN opt_exists IDENTIFIER  */
	                                                {
  (yyval.drop_action_t) = new DropColumnAction((yyvsp[0].sval));
  (yyval.drop_action_t)->ifExists = (yyvsp[-1].bval);
}
		break;

	case 104: /* delete_statement: DELETE FROM table_name opt_where  */
	                                                    {
  (yyval.delete_stmt) = new DeleteStatement();
  (yyval.delete_stmt)->schema = (yyvsp[-1].table_name).schema;
  (yyval.delete_stmt)->tableName = (yyvsp[-1].table_name).name;
  (yyval.delete_stmt)->expr = (yyvsp[0].expr);
}
		break;

	case 105: /* truncate_statement: TRUNCATE table_name  */
	                                         {
  (yyval.delete_stmt) = new DeleteStatement();
  (yyval.delete_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.delete_stmt)->tableName = (yyvsp[0].table_name).name;
}
		break;

	case 106: /* insert_statement: INSERT INTO table_name opt_column_list VALUES '(' literal_list ')'  */
	                                                                                      {
  (yyval.insert_stmt) = new InsertStatement(kInsertValues);
  (yyval.insert_stmt)->schema = (yyvsp[-5].table_name).schema;
  (yyval.insert_stmt)->tableName = (yyvsp[-5].table_name).name;
  (yyval.insert_stmt)->columns = (yyvsp[-4].str_vec);
  (yyval.insert_stmt)->values = (yyvsp[-1].expr_vec);
}
		break;

	case 107: /* insert_statement: INSERT INTO table_name opt_column_list select_no_paren  */
	                                                         {
  (yyval.insert_stmt) = new InsertStatement(kInsertSelect);
  (yyval.insert_stmt)->schema = (yyvsp[-2].table_name).schema;
  (yyval.insert_stmt)->tableName = (yyvsp[-2].table_name).name;
  (yyval.insert_stmt)->columns = (yyvsp[-1].str_vec);
  (yyval.insert_stmt)->select = (yyvsp[0].select_stmt);
}
		break;

	case 108: /* opt_column_list: '(' ident_commalist ')'  */
	                                          { (yyval.str_vec) = (yyvsp[-1].str_vec); }
		break;

	case 109: /* opt_column_list: %empty  */
	              { (yyval.str_vec) = nullptr; }
		break;

	case 110: /* update_statement: UPDATE table_ref_name_no_alias SET update_clause_commalist opt_where  */
	                                                                                        {
  (yyval.update_stmt) = new UpdateStatement();
  (yyval.update_stmt)->table = (yyvsp[-3].table);
  (yyval.update_stmt)->updates = (yyvsp[-1].update_vec);
  (yyval.update_stmt)->where = (yyvsp[0].expr);
}
		break;

	case 111: /* update_clause_commalist: update_clause  */
	                                        {
  (yyval.update_vec) = new std::vector<UpdateClause*>();
  (yyval.update_vec)->push_back((yyvsp[0].update_t));
}
		break;

	case 112: /* update_clause_commalist: update_clause_commalist ',' update_clause  */
	                                            {
  (yyvsp[-2].update_vec)->push_back((yyvsp[0].update_t));
  (yyval.update_vec) = (yyvsp[-2].update_vec);
}
		break;

	case 113: /* update_clause: IDENTIFIER '=' expr  */
	                                    {
  (yyval.update_t) = new UpdateClause();
  (yyval.update_t)->column = (yyvsp[-2].sval);
  (yyval.update_t)->value = (yyvsp[0].expr);
}
		break;

	case 114: /* select_statement: opt_with_clause select_with_paren  */
	                                                     {
  (yyval.select_stmt) = (yyvsp[0].select_stmt);
  (yyval.select_stmt)->withDescriptions = (yyvsp[-1].with_description_vec);
}
		break;

	case 115: /* select_statement: opt_with_clause select_no_paren  */
	                                  {
  (yyval.select_stmt) = (yyvsp[0].select_stmt);
  (yyval.select_stmt)->withDescriptions = (yyvsp[-1].with_description_vec);
}
		break;

	case 116: /* select_statement: opt_with_clause select_with_paren set_operator select_within_set_operation opt_order opt_limit  */
	                                                                                                 {
  (yyval.select_stmt) = (yyvsp[-4].select_stmt);
  if((yyval.select_stmt)->setOperations == nullptr) {
    (yyval.select_stmt)->setOperations = new std::vector<SetOperation*>();
  }
  (yyval.select_stmt)->setOperations->push_back((yyvsp[-3].set_operator_t));
  (yyval.select_stmt)->setOperations->back()->nestedSelectStatement = (yyvsp[-2].select_stmt);
  (yyval.select_stmt)->setOperations->back()->resultOrder = (yyvsp[-1].order_vec);
  (yyval.select_stmt)->setOperations->back()->resultLimit = (yyvsp[0].limit);
  (yyval.select_stmt)->withDescriptions = (yyvsp[-5].with_description_vec);
}
		break;

	case 119: /* select_within_set_operation_no_parentheses: select_clause  */
	                                                           { (yyval.select_stmt) = (yyvsp[0].select_stmt); }
		break;

	case 120: /* select_within_set_operation_no_parentheses: select_clause set_operator select_within_set_operation  */
	                                                         {
  (yyval.select_stmt) = (yyvsp[-2].select_stmt);
  if((yyval.select_stmt)->setOperations == nullptr) {
    (yyval.select_stmt)->setOperations = new std::vector<SetOperation*>();
  }
  (yyval.select_stmt)->setOperations->push_back((yyvsp[-1].set_operator_t));
  (yyval.select_stmt)->setOperations->back()->nestedSelectStatement = (yyvsp[0].select_stmt);
}
		break;

	case 121: /* select_with_paren: '(' select_no_paren ')'  */
	                                            { (yyval.select_stmt) = (yyvsp[-1].select_stmt); }
		break;

	case 122: /* select_with_paren: '(' select_with_paren ')'  */
	                            { (yyval.select_stmt) = (yyvsp[-1].select_stmt); }
		break;

	case 123: /* select_no_paren: select_clause opt_order opt_limit opt_locking_clause  */
	                                                                       {
  (yyval.select_stmt) = (yyvsp[-3].select_stmt);
  (yyval.select_stmt)->order = (yyvsp[-2].order_vec);

  // Limit could have been set by TOP.
  if((yyvsp[-1].limit)) {
    delete (yyval.select_stmt)->limit;
    (yyval.select_stmt)->limit = (yyvsp[-1].limit);
  }

  if((yyvsp[0].locking_clause_vec)) {
    (yyval.select_stmt)->lockings = (yyvsp[0].locking_clause_vec);
  }
}
		break;

	case 124: /* select_no_paren: select_clause set_operator select_within_set_operation opt_order opt_limit opt_locking_clause  */
	                                                                                                {
  (yyval.select_stmt) = (yyvsp[-5].select_stmt);
  if((yyval.select_stmt)->setOperations == nullptr) {
    (yyval.select_stmt)->setOperations = new std::vector<SetOperation*>();
  }
  (yyval.select_stmt)->setOperations->push_back((yyvsp[-4].set_operator_t));
  (yyval.select_stmt)->setOperations->back()->nestedSelectStatement = (yyvsp[-3].select_stmt);
  (yyval.select_stmt)->setOperations->back()->resultOrder = (yyvsp[-2].order_vec);
  (yyval.select_stmt)->setOperations->back()->resultLimit = (yyvsp[-1].limit);
  (yyval.select_stmt)->lockings = (yyvsp[0].locking_clause_vec);
}
		break;

	case 125: /* set_operator: set_type opt_all  */
	                                {
  (yyval.set_operator_t) = (yyvsp[-1].set_operator_t);
  (yyval.set_operator_t)->isAll = (yyvsp[0].bval);
}
		break;

	case 126: /* set_type: UNION  */
	                 {
  (yyval.set_operator_t) = new SetOperation();
  (yyval.set_operator_t)->setType = SetType::kSetUnion;
}
		break;

	case 127: /* set_type: INTERSECT  */
	            {
  (yyval.set_operator_t) = new SetOperation();
  (yyval.set_operator_t)->setType = SetType::kSetIntersect;
}
		break;

	case 128: /* set_type: EXCEPT  */
	         {
  (yyval.set_operator_t) = new SetOperation();
  (yyval.set_operator_t)->setType = SetType::kSetExcept;
}
		break;

	case 129: /* opt_all: ALL  */
	              { (yyval.bval) = true; }
		break;

	case 130: /* opt_all: %empty  */
	              { (yyval.bval) = false; }
		break;

	case 131: /* select_clause: SELECT opt_top opt_distinct select_list opt_from_clause opt_where opt_group  */
	                                                                                            {
  (yyval.select_stmt) = new SelectStatement();
  (yyval.select_stmt)->limit = (yyvsp[-5].limit);
  (yyval.select_stmt)->selectDistinct = (yyvsp[-4].bval);
  (yyval.select_stmt)->selectList = (yyvsp[-3].expr_vec);
  (yyval.select_stmt)->fromTable = (yyvsp[-2].table);
  (yyval.select_stmt)->whereClause = (yyvsp[-1].expr);
  (yyval.select_stmt)->groupBy = (yyvsp[0].group_t);
}
		break;

	case 132: /* opt_distinct: DISTINCT  */
	                        { (yyval.bval) = true; }
		break;

	case 133: /* opt_distinct: %empty  */
	              { (yyval.bval) = false; }
		break;

	case 135: /* opt_from_clause: from_clause  */
	                              { (yyval.table) = (yyvsp[0].table); }
		break;

	case 136: /* opt_from_clause: %empty  */
	              { (yyval.table) = nullptr; }
		break;

	case 137: /* from_clause: FROM table_ref  */
	                             { (yyval.table) = (yyvsp[0].table); }
		break;

	case 138: /* opt_where: WHERE expr  */
	                       { (yyval.expr) = (yyvsp[0].expr); }
		break;

	case 139: /* opt_where: %empty  */
	              { (yyval.expr) = nullptr; }
		break;

	case 140: /* opt_group: GROUP BY expr_list opt_having  */
	                                          {
  (yyval.group_t) = new GroupByDescription();
  (yyval.group_t)->columns = (yyvsp[-1].expr_vec);
  (yyval.group_t)->having = (yyvsp[0].expr);
}
		break;

	case 141: /* opt_group: %empty  */
	              { (yyval.group_t) = nullptr; }
		break;

	case 142: /* opt_having: HAVING expr  */
	                         { (yyval.expr) = (yyvsp[0].expr); }
		break;

	case 143: /* opt_having: %empty  */
	              { (yyval.expr) = nullptr; }
		break;

	case 144: /* opt_order: ORDER BY order_list  */
	                                { (yyval.order_vec) = (yyvsp[0].order_vec); }
		break;

	case 145: /* opt_order: %empty  */
	              { (yyval.order_vec) = nullptr; }
		break;

	case 146: /* order_list: order_desc  */
	                        {
  (yyval.order_vec) = new std::vector<OrderDescription*>();
  (yyval.order_vec)->push_back((yyvsp[0].order));
}
		break;

	case 147: /* order_list: order_list ',' order_desc  */
	                            {
  (yyvsp[-2].order_vec)->push_back((yyvsp[0].order));
  (yyval.order_vec) = (yyvsp[-2].order_vec);
}
		break;

	case 148: /* order_desc: expr opt_order_type  */
	                                 { (yyval.order) = new OrderDescription((yyvsp[0].order_type), (yyvsp[-1].expr)); }
		break;

	case 149: /* opt_order_type: ASC  */
	                     { (yyval.order_type) = kOrderAsc; }
		break;

	case 150: /* opt_order_type: DESC  */
	       { (yyval.order_type) = kOrderDesc; }
		break;

	case 151: /* opt_order_type: %empty  */
	              { (yyval.order_type) = kOrderAsc; }
		break;

	case 152: /* opt_top: TOP int_literal  */
	                          { (yyval.limit) = new LimitDescription((yyvsp[0].expr), nullptr); }
		break;

	case 153: /* opt_top: %empty  */
	              { (yyval.limit) = nullptr; }
		break;

	case 154: /* opt_limit: LIMIT expr  */
	                       { (yyval.limit) = new LimitDescription((yyvsp[0].expr), nullptr); }
		break;

	case 155: /* opt_limit: OFFSET expr  */
	              { (yyval.limit) = new LimitDescription(nullptr, (yyvsp[0].expr)); }
		break;

	case 156: /* opt_limit: LIMIT expr OFFSET expr  */
	                         { (yyval.limit) = new LimitDescription((yyvsp[-2].expr), (yyvsp[0].expr)); }
		break;

	case 157: /* opt_limit: LIMIT ALL  */
	            { (yyval.limit) = new LimitDescription(nullptr, nullptr); }
		break;

	case 158: /* opt_limit: LIMIT ALL OFFSET expr  */
	                        { (yyval.limit) = new LimitDescription(nullptr, (yyvsp[0].expr)); }
		break;

	case 159: /* opt_limit: %empty  */
	              { (yyval.limit) = nullptr; }
		break;

	case 160: /* expr_list: expr_alias  */
	                       {
  (yyval.expr_vec) = new std::vector<Expr*>();
  (yyval.expr_vec)->push_back((yyvsp[0].expr));
}
		break;

	case 161: /* expr_list: expr_list ',' expr_alias  */
	                           {
  (yyvsp[-2].expr_vec)->push_back((yyvsp[0].expr));
  (yyval.expr_vec) = (yyvsp[-2].expr_vec);
}
		break;

	case 162: /* opt_literal_list: literal_list  */
	                                { (yyval.expr_vec) = (yyvsp[0].expr_vec); }
		break;

	case 163: /* opt_literal_list: %empty  */
	              { (yyval.expr_vec) = nullptr; }
		break;

	case 164: /* literal_list: literal  */
	                       {
  (yyval.expr_vec) = new std::vector<Expr*>();
  (yyval.expr_vec)->push_back((yyvsp[0].expr));
}
		break;

	case 165: /* literal_list: literal_list ',' literal  */
	                           {
  (yyvsp[-2].expr_vec)->push_back((yyvsp[0].expr));
  (yyval.expr_vec) = (yyvsp[-2].expr_vec);
}
		break;

	case 166: /* expr_alias: expr opt_alias  */
	                            {
  (yyval.expr) = (yyvsp[-1].expr);
  if((yyvsp[0].alias_t)) {
    (yyval.expr)->alias = _strdup((yyvsp[0].alias_t)->name);
    delete (yyvsp[0].alias_t);
  }
}
		break;

	case 172: /* operand: '(' expr ')'  */
	                       { (yyval.expr) = (yyvsp[-1].expr); }
		break;

	case 182: /* operand: '(' select_no_paren ')'  */
	                                         {
  (yyval.expr) = Expr::makeSelect((yyvsp[-1].select_stmt));
}
		break;

	case 185: /* unary_expr: '-' operand  */
	                         { (yyval.expr) = Expr::makeOpUnary(kOpUnaryMinus, (yyvsp[0].expr)); }
		break;

	case 186: /* unary_expr: NOT operand  */
	              { (yyval.expr) = Expr::makeOpUnary(kOpNot, (yyvsp[0].expr)); }
		break;

	case 187: /* unary_expr: operand ISNULL  */
	                 { (yyval.expr) = Expr::makeOpUnary(kOpIsNull, (yyvsp[-1].expr)); }
		break;

	case 188: /* unary_expr: operand IS NULL  */
	                  { (yyval.expr) = Expr::makeOpUnary(kOpIsNull, (yyvsp[-2].expr)); }
		break;

	case 189: /* unary_expr: operand IS NOT NULL  */
	                      { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeOpUnary(kOpIsNull, (yyvsp[-3].expr))); }
		break;

	case 191: /* binary_expr: operand '-' operand  */
	                                              { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpMinus, (yyvsp[0].expr)); }
		break;

	case 192: /* binary_expr: operand '+' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpPlus, (yyvsp[0].expr)); }
		break;

	case 193: /* binary_expr: operand '/' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpSlash, (yyvsp[0].expr)); }
		break;

	case 194: /* binary_expr: operand '*' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpAsterisk, (yyvsp[0].expr)); }
		break;

	case 195: /* binary_expr: operand '%' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpPercentage, (yyvsp[0].expr)); }
		break;

	case 196: /* binary_expr: operand '^' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpCaret, (yyvsp[0].expr)); }
		break;

	case 197: /* binary_expr: operand LIKE operand  */
	                       { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpLike, (yyvsp[0].expr)); }
		break;

	case 198: /* binary_expr: operand NOT LIKE operand  */
	                           { (yyval.expr) = Expr::makeOpBinary((yyvsp[-3].expr), kOpNotLike, (yyvsp[0].expr)); }
		break;

	case 199: /* binary_expr: operand ILIKE operand  */
	                        { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpILike, (yyvsp[0].expr)); }
		break;

	case 200: /* binary_expr: operand CONCAT operand  */
	                         { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpConcat, (yyvsp[0].expr)); }
		break;

	case 201: /* logic_expr: expr AND expr  */
	                           { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpAnd, (yyvsp[0].expr)); }
		break;

	case 202: /* logic_expr: expr OR expr  */
	               { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpOr, (yyvsp[0].expr)); }
		break;

	case 203: /* in_expr: operand IN '(' expr_list ')'  */
	                                       { (yyval.expr) = Expr::makeInOperator((yyvsp[-4].expr), (yyvsp[-1].expr_vec)); }
		break;

	case 204: /* in_expr: operand NOT IN '(' expr_list ')'  */
	                                   { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeInOperator((yyvsp[-5].expr), (yyvsp[-1].expr_vec))); }
		break;

	case 205: /* in_expr: operand IN '(' select_no_paren ')'  */
	                                     { (yyval.expr) = Expr::makeInOperator((yyvsp[-4].expr), (yyvsp[-1].select_stmt)); }
		break;

	case 206: /* in_expr: operand NOT IN '(' select_no_paren ')'  */
	                                         { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeInOperator((yyvsp[-5].expr), (yyvsp[-1].select_stmt))); }
		break;

	case 207: /* case_expr: CASE expr case_list END  */
	                                    { (yyval.expr) = Expr::makeCase((yyvsp[-2].expr), (yyvsp[-1].expr), nullptr); }
		break;

	case 208: /* case_expr: CASE expr case_list ELSE expr END  */
	                                    { (yyval.expr) = Expr::makeCase((yyvsp[-4].expr), (yyvsp[-3].expr), (yyvsp[-1].expr)); }
		break;

	case 209: /* case_expr: CASE case_list END  */
	                     { (yyval.expr) = Expr::makeCase(nullptr, (yyvsp[-1].expr), nullptr); }
		break;

	case 210: /* case_expr: CASE case_list ELSE expr END  */
	                               { (yyval.expr) = Expr::makeCase(nullptr, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
		break;

	case 211: /* case_list: WHEN expr THEN expr  */
	                                { (yyval.expr) = Expr::makeCaseList(Expr::makeCaseListElement((yyvsp[-2].expr), (yyvsp[0].expr))); }
		break;

	case 212: /* case_list: case_list WHEN expr THEN expr  */
	                                { (yyval.expr) = Expr::caseListAppend((yyvsp[-4].expr), Expr::makeCaseListElement((yyvsp[-2].expr), (yyvsp[0].expr))); }
		break;

	case 213: /* exists_expr: EXISTS '(' select_no_paren ')'  */
	                                             { (yyval.expr) = Expr::makeExists((yyvsp[-1].select_stmt)); }
		break;

	case 214: /* exists_expr: NOT EXISTS '(' select_no_paren ')'  */
	                                     { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeExists((yyvsp[-1].select_stmt))); }
		break;

	case 215: /* comp_expr: operand '=' operand  */
	                                { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpEquals, (yyvsp[0].expr)); }
		break;

	case 216: /* comp_expr: operand EQUALS operand  */
	                         { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpEquals, (yyvsp[0].expr)); }
		break;

	case 217: /* comp_expr: operand NOTEQUALS operand  */
	                            { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpNotEquals, (yyvsp[0].expr)); }
		break;

	case 218: /* comp_expr: operand '<' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpLess, (yyvsp[0].expr)); }
		break;

	case 219: /* comp_expr: operand '>' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpGreater, (yyvsp[0].expr)); }
		break;

	case 220: /* comp_expr: operand LESSEQ operand  */
	                         { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpLessEq, (yyvsp[0].expr)); }
		break;

	case 221: /* comp_expr: operand GREATEREQ operand  */
	                            { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpGreaterEq, (yyvsp[0].expr)); }
		break;

	case 222: /* function_expr: IDENTIFIER '(' ')' opt_window  */
	                                              { (yyval.expr) = Expr::makeFunctionRef((yyvsp[-3].sval), new std::vector<Expr*>(), false, (yyvsp[0].window_description)); }
		break;

	case 223: /* function_expr: IDENTIFIER '(' opt_distinct expr_list ')' opt_window  */
	                                                       { (yyval.expr) = Expr::makeFunctionRef((yyvsp[-5].sval), (yyvsp[-2].expr_vec), (yyvsp[-3].bval), (yyvsp[0].window_description)); }
		break;

	case 224: /* opt_window: OVER '(' opt_partition opt_order opt_frame_clause ')'  */
	                                                                   { (yyval.window_description) = new WindowDescription((yyvsp[-3].expr_vec), (yyvsp[-2].order_vec), (yyvsp[-1].frame_description)); }
		break;

	case 225: /* opt_window: %empty  */
	              { (yyval.window_description) = nullptr; }
		break;

	case 226: /* opt_partition: PARTITION BY expr_list  */
	                                       { (yyval.expr_vec) = (yyvsp[0].expr_vec); }
		break;

	case 227: /* opt_partition: %empty  */
	              { (yyval.expr_vec) = nullptr; }
		break;

	case 228: /* opt_frame_clause: frame_type frame_bound  */
	                                          { (yyval.frame_description) = new FrameDescription{(yyvsp[-1].frame_type), (yyvsp[0].frame_bound), new FrameBound{0, kCurrentRow, false}}; }
		break;

	case 229: /* opt_frame_clause: frame_type BETWEEN frame_bound AND frame_bound  */
	                                                 { (yyval.frame_description) = new FrameDescription{(yyvsp[-4].frame_type), (yyvsp[-2].frame_bound), (yyvsp[0].frame_bound)}; }
		break;

	case 230: /* opt_frame_clause: %empty  */
	              {
  (yyval.frame_description) = new FrameDescription{kRange, new FrameBound{0, kPreceding, true}, new FrameBound{0, kCurrentRow, false}};
}
		break;

	case 231: /* frame_type: RANGE  */
	                   { (yyval.frame_type) = kRange; }
		break;

	case 232: /* frame_type: ROWS  */
	       { (yyval.frame_type) = kRows; }
		break;

	case 233: /* frame_type: GROUPS  */
	         { (yyval.frame_type) = kGroups; }
		break;

	case 234: /* frame_bound: UNBOUNDED PRECEDING  */
	                                  { (yyval.frame_bound) = new FrameBound{0, kPreceding, true}; }
		break;

	case 235: /* frame_bound: INTVAL PRECEDING  */
	                   { (yyval.frame_bound) = new FrameBound{(yyvsp[-1].ival), kPreceding, false}; }
		break;

	case 236: /* frame_bound: UNBOUNDED FOLLOWING  */
	                      { (yyval.frame_bound) = new FrameBound{0, kFollowing, true}; }
		break;

	case 237: /* frame_bound: INTVAL FOLLOWING  */
	                   { (yyval.frame_bound) = new FrameBound{(yyvsp[-1].ival), kFollowing, false}; }
		break;

	case 238: /* frame_bound: CURRENT_ROW  */
	              { (yyval.frame_bound) = new FrameBound{0, kCurrentRow, false}; }
		break;

	case 239: /* extract_expr: EXTRACT '(' datetime_field FROM expr ')'  */
	                                                        { (yyval.expr) = Expr::makeExtract((yyvsp[-3].datetime_field), (yyvsp[-1].expr)); }
		break;

	case 240: /* cast_expr: CAST '(' expr AS column_type ')'  */
	                                             { (yyval.expr) = Expr::makeCast((yyvsp[-3].expr), (yyvsp[-1].column_type_t)); }
		break;

	case 241: /* datetime_field: SECOND  */
	                        { (yyval.datetime_field) = kDatetimeSecond; }
		break;

	case 242: /* datetime_field: MINUTE  */
	         { (yyval.datetime_field) = kDatetimeMinute; }
		break;

	case 243: /* datetime_field: HOUR  */
	       { (yyval.datetime_field) = kDatetimeHour; }
		break;

	case 244: /* datetime_field: DAY  */
	      { (yyval.datetime_field) = kDatetimeDay; }
		break;

	case 245: /* datetime_field: MONTH  */
	        { (yyval.datetime_field) = kDatetimeMonth; }
		break;

	case 246: /* datetime_field: YEAR  */
	       { (yyval.datetime_field) = kDatetimeYear; }
		break;

	case 247: /* datetime_field_plural: SECONDS  */
	                                { (yyval.datetime_field) = kDatetimeSecond; }
		break;

	case 248: /* datetime_field_plural: MINUTES  */
	          { (yyval.datetime_field) = kDatetimeMinute; }
		break;

	case 249: /* datetime_field_plural: HOURS  */
	        { (yyval.datetime_field) = kDatetimeHour; }
		break;

	case 250: /* datetime_field_plural: DAYS  */
	       { (yyval.datetime_field) = kDatetimeDay; }
		break;

	case 251: /* datetime_field_plural: MONTHS  */
	         { (yyval.datetime_field) = kDatetimeMonth; }
		break;

	case 252: /* datetime_field_plural: YEARS  */
	        { (yyval.datetime_field) = kDatetimeYear; }
		break;

	case 255: /* array_expr: ARRAY '[' expr_list ']'  */
	                                     { (yyval.expr) = Expr::makeArray((yyvsp[-1].expr_vec)); }
		break;

	case 256: /* array_index: operand '[' int_literal ']'  */
	                                          { (yyval.expr) = Expr::makeArrayIndex((yyvsp[-3].expr), (yyvsp[-1].expr)->ival); }
		break;

	case 257: /* between_expr: operand BETWEEN operand AND operand  */
	                                                   { (yyval.expr) = Expr::makeBetween((yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
		break;

	case 258: /* column_name: IDENTIFIER  */
	                         { (yyval.expr) = Expr::makeColumnRef((yyvsp[0].sval)); }
		break;

	case 259: /* column_name: IDENTIFIER '.' IDENTIFIER  */
	                            { (yyval.expr) = Expr::makeColumnRef((yyvsp[-2].sval), (yyvsp[0].sval)); }
		break;

	case 260: /* column_name: '*'  */
	      { (yyval.expr) = Expr::makeStar(); }
		break;

	case 261: /* column_name: IDENTIFIER '.' '*'  */
	                     { (yyval.expr) = Expr::makeStar((yyvsp[-2].sval)); }
		break;

	case 269: /* string_literal: STRING  */
	                        { (yyval.expr) = Expr::makeLiteral((yyvsp[0].sval)); }
		break;

	case 270: /* bool_literal: TRUE  */
	                    { (yyval.expr) = Expr::makeLiteral(true); }
		break;

	case 271: /* bool_literal: FALSE  */
	                                                              { (yyval.expr) = Expr::makeLiteral(false); }
		break;

	case 272: /* num_literal: FLOATVAL  */
	                       { (yyval.expr) = Expr::makeLiteral((yyvsp[0].fval)); }
		break;

	case 274: /* int_literal: INTVAL  */
	                     { (yyval.expr) = Expr::makeLiteral((yyvsp[0].ival)); }
		break;

	case 275: /* null_literal: NULL  */
	                    { (yyval.expr) = Expr::makeNullLiteral(); }
		break;

	case 276: /* date_literal: DATE STRING  */
	                           {
  int day{0}, month{0}, year{0}, chars_parsed{0};
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if(sscanf((yyvsp[0].sval), "%4d-%2d-%2d%n", &day, &month, &year, &chars_parsed) != 3 || (yyvsp[0].sval)[chars_parsed] != 0) {
    SAlloc::F((yyvsp[0].sval));
    yyerror(&yyloc, result, scanner, "Found incorrect date format. Expected format: YYYY-MM-DD");
    YYERROR;
  }
  (yyval.expr) = Expr::makeDateLiteral((yyvsp[0].sval));
}
		break;

	case 277: /* interval_literal: int_literal duration_field  */
	                                              {
  (yyval.expr) = Expr::makeIntervalLiteral((yyvsp[-1].expr)->ival, (yyvsp[0].datetime_field));
  delete (yyvsp[-1].expr);
}
		break;

	case 278: /* interval_literal: INTERVAL STRING datetime_field  */
	                                 {
  int duration{0}, chars_parsed{0};
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if(sscanf((yyvsp[-1].sval), "%d%n", &duration, &chars_parsed) != 1 || (yyvsp[-1].sval)[chars_parsed] != 0) {
    SAlloc::F((yyvsp[-1].sval));
    yyerror(&yyloc, result, scanner, "Found incorrect interval format. Expected format: INTEGER");
    YYERROR;
  }
  SAlloc::F((yyvsp[-1].sval));
  (yyval.expr) = Expr::makeIntervalLiteral(duration, (yyvsp[0].datetime_field));
}
		break;

	case 279: /* interval_literal: INTERVAL STRING  */
	                  {
  int duration{0}, chars_parsed{0};
  // 'seconds' and 'minutes' are the longest accepted interval qualifiers (7 chars) + null byte
  char unit_string[8];
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if(sscanf((yyvsp[0].sval), "%d %7s%n", &duration, unit_string, &chars_parsed) != 2 || (yyvsp[0].sval)[chars_parsed] != 0) {
    SAlloc::F((yyvsp[0].sval));
    yyerror(&yyloc, result, scanner, "Found incorrect interval format. Expected format: INTEGER INTERVAL_QUALIIFIER");
    YYERROR;
  }
  SAlloc::F((yyvsp[0].sval));
  DatetimeField unit;
  if(sstreqi_ascii(unit_string, "second") || sstreqi_ascii(unit_string, "seconds")) {
    unit = kDatetimeSecond;
  } 
  else if(sstreqi_ascii(unit_string, "minute") || sstreqi_ascii(unit_string, "minutes")) {
    unit = kDatetimeMinute;
  } 
  else if(sstreqi_ascii(unit_string, "hour") || sstreqi_ascii(unit_string, "hours")) {
    unit = kDatetimeHour;
  } 
  else if(sstreqi_ascii(unit_string, "day") || sstreqi_ascii(unit_string, "days")) {
    unit = kDatetimeDay;
  } 
  else if(sstreqi_ascii(unit_string, "month") || sstreqi_ascii(unit_string, "months")) {
    unit = kDatetimeMonth;
  } 
  else if(sstreqi_ascii(unit_string, "year") || sstreqi_ascii(unit_string, "years")) {
    unit = kDatetimeYear;
  } 
  else {
    yyerror(&yyloc, result, scanner, "Interval qualifier is unknown.");
    YYERROR;
  }
  (yyval.expr) = Expr::makeIntervalLiteral(duration, unit);
}
		break;

	case 280: /* param_expr: '?'  */
	                 {
  (yyval.expr) = Expr::makeParameter(yylloc.total_column);
  (yyval.expr)->ival2 = yyloc.param_list.size();
  yyloc.param_list.push_back((yyval.expr));
}
		break;

	case 282: /* table_ref: table_ref_commalist ',' table_ref_atomic  */
	                                                                        {
  (yyvsp[-2].table_vec)->push_back((yyvsp[0].table));
  auto tbl = new TableRef(kTableCrossProduct);
  tbl->list = (yyvsp[-2].table_vec);
  (yyval.table) = tbl;
}
		break;

	case 286: /* nonjoin_table_ref_atomic: '(' select_statement ')' opt_table_alias  */
	                                                                                     {
  auto tbl = new TableRef(kTableSelect);
  tbl->select = (yyvsp[-2].select_stmt);
  tbl->alias = (yyvsp[0].alias_t);
  (yyval.table) = tbl;
}
		break;

	case 287: /* table_ref_commalist: table_ref_atomic  */
	                                       {
  (yyval.table_vec) = new std::vector<TableRef*>();
  (yyval.table_vec)->push_back((yyvsp[0].table));
}
		break;

	case 288: /* table_ref_commalist: table_ref_commalist ',' table_ref_atomic  */
	                                           {
  (yyvsp[-2].table_vec)->push_back((yyvsp[0].table));
  (yyval.table_vec) = (yyvsp[-2].table_vec);
}
		break;

	case 289: /* table_ref_name: table_name opt_table_alias  */
	                                            {
  auto tbl = new TableRef(kTableName);
  tbl->schema = (yyvsp[-1].table_name).schema;
  tbl->name = (yyvsp[-1].table_name).name;
  tbl->alias = (yyvsp[0].alias_t);
  (yyval.table) = tbl;
}
		break;

	case 290: /* table_ref_name_no_alias: table_name  */
	                                     {
  (yyval.table) = new TableRef(kTableName);
  (yyval.table)->schema = (yyvsp[0].table_name).schema;
  (yyval.table)->name = (yyvsp[0].table_name).name;
}
		break;

	case 291: /* table_name: IDENTIFIER  */
	                        {
  (yyval.table_name).schema = nullptr;
  (yyval.table_name).name = (yyvsp[0].sval);
}
		break;

	case 292: /* table_name: IDENTIFIER '.' IDENTIFIER  */
	                            {
  (yyval.table_name).schema = (yyvsp[-2].sval);
  (yyval.table_name).name = (yyvsp[0].sval);
}
		break;

	case 293: /* opt_index_name: IDENTIFIER  */
	                            { (yyval.sval) = (yyvsp[0].sval); }
		break;

	case 294: /* opt_index_name: %empty  */
	              { (yyval.sval) = nullptr; }
		break;

	case 296: /* table_alias: AS IDENTIFIER '(' ident_commalist ')'  */
	                                                            { (yyval.alias_t) = new Alias((yyvsp[-3].sval), (yyvsp[-1].str_vec)); }
		break;

	case 298: /* opt_table_alias: %empty  */
	                                            { (yyval.alias_t) = nullptr; }
		break;

	case 299: /* alias: AS IDENTIFIER  */
	                      { (yyval.alias_t) = new Alias((yyvsp[0].sval)); }
		break;

	case 300: /* alias: IDENTIFIER  */
	             { (yyval.alias_t) = new Alias((yyvsp[0].sval)); }
		break;

	case 302: /* opt_alias: %empty  */
	                                { (yyval.alias_t) = nullptr; }
		break;

	case 303: /* opt_locking_clause: opt_locking_clause_list  */
	                                             { (yyval.locking_clause_vec) = (yyvsp[0].locking_clause_vec); }
		break;

	case 304: /* opt_locking_clause: %empty  */
	              { (yyval.locking_clause_vec) = nullptr; }
		break;

	case 305: /* opt_locking_clause_list: locking_clause  */
	                                         {
  (yyval.locking_clause_vec) = new std::vector<LockingClause*>();
  (yyval.locking_clause_vec)->push_back((yyvsp[0].locking_t));
}
		break;

	case 306: /* opt_locking_clause_list: opt_locking_clause_list locking_clause  */
	                                         {
  (yyvsp[-1].locking_clause_vec)->push_back((yyvsp[0].locking_t));
  (yyval.locking_clause_vec) = (yyvsp[-1].locking_clause_vec);
}
		break;

	case 307: /* locking_clause: FOR row_lock_mode opt_row_lock_policy  */
	                                                       {
  (yyval.locking_t) = new LockingClause();
  (yyval.locking_t)->rowLockMode = (yyvsp[-1].lock_mode_t);
  (yyval.locking_t)->rowLockWaitPolicy = (yyvsp[0].lock_wait_policy_t);
  (yyval.locking_t)->tables = nullptr;
}
		break;

	case 308: /* locking_clause: FOR row_lock_mode OF ident_commalist opt_row_lock_policy  */
	                                                           {
  (yyval.locking_t) = new LockingClause();
  (yyval.locking_t)->rowLockMode = (yyvsp[-3].lock_mode_t);
  (yyval.locking_t)->tables = (yyvsp[-1].str_vec);
  (yyval.locking_t)->rowLockWaitPolicy = (yyvsp[0].lock_wait_policy_t);
}
		break;

	case 309: /* row_lock_mode: UPDATE  */
	                       { (yyval.lock_mode_t) = RowLockMode::ForUpdate; }
		break;

	case 310: /* row_lock_mode: NO KEY UPDATE  */
	                { (yyval.lock_mode_t) = RowLockMode::ForNoKeyUpdate; }
		break;

	case 311: /* row_lock_mode: SHARE  */
	        { (yyval.lock_mode_t) = RowLockMode::ForShare; }
		break;

	case 312: /* row_lock_mode: KEY SHARE  */
	            { (yyval.lock_mode_t) = RowLockMode::ForKeyShare; }
		break;

	case 313: /* opt_row_lock_policy: SKIP LOCKED  */
	                                  { (yyval.lock_wait_policy_t) = RowLockWaitPolicy::SkipLocked; }
		break;

	case 314: /* opt_row_lock_policy: NOWAIT  */
	         { (yyval.lock_wait_policy_t) = RowLockWaitPolicy::NoWait; }
		break;

	case 315: /* opt_row_lock_policy: %empty  */
	              { (yyval.lock_wait_policy_t) = RowLockWaitPolicy::None; }
		break;

	case 317: /* opt_with_clause: %empty  */
	                                            { (yyval.with_description_vec) = nullptr; }
		break;

	case 318: /* with_clause: WITH with_description_list  */
	                                         { (yyval.with_description_vec) = (yyvsp[0].with_description_vec); }
		break;

	case 319: /* with_description_list: with_description  */
	                                         {
  (yyval.with_description_vec) = new std::vector<WithDescription*>();
  (yyval.with_description_vec)->push_back((yyvsp[0].with_description_t));
}
		break;

	case 320: /* with_description_list: with_description_list ',' with_description  */
	                                             {
  (yyvsp[-2].with_description_vec)->push_back((yyvsp[0].with_description_t));
  (yyval.with_description_vec) = (yyvsp[-2].with_description_vec);
}
		break;

	case 321: /* with_description: IDENTIFIER AS select_with_paren  */
	                                                   {
  (yyval.with_description_t) = new WithDescription();
  (yyval.with_description_t)->alias = (yyvsp[-2].sval);
  (yyval.with_description_t)->select = (yyvsp[0].select_stmt);
}
		break;

	case 322: /* join_clause: table_ref_atomic NATURAL JOIN nonjoin_table_ref_atomic  */
	                                                                     {
  (yyval.table) = new TableRef(kTableJoin);
  (yyval.table)->join = new JoinDefinition();
  (yyval.table)->join->type = kJoinNatural;
  (yyval.table)->join->left = (yyvsp[-3].table);
  (yyval.table)->join->right = (yyvsp[0].table);
}
		break;

	case 323: /* join_clause: table_ref_atomic opt_join_type JOIN table_ref_atomic ON join_condition  */
	                                                                         {
  (yyval.table) = new TableRef(kTableJoin);
  (yyval.table)->join = new JoinDefinition();
  (yyval.table)->join->type = (JoinType)(yyvsp[-4].join_type);
  (yyval.table)->join->left = (yyvsp[-5].table);
  (yyval.table)->join->right = (yyvsp[-2].table);
  (yyval.table)->join->condition = (yyvsp[0].expr);
}
		break;

	case 324: /* join_clause: table_ref_atomic opt_join_type JOIN table_ref_atomic USING '(' column_name ')'  */
	                                                                                 {
  (yyval.table) = new TableRef(kTableJoin);
  (yyval.table)->join = new JoinDefinition();
  (yyval.table)->join->type = (JoinType)(yyvsp[-6].join_type);
  (yyval.table)->join->left = (yyvsp[-7].table);
  (yyval.table)->join->right = (yyvsp[-4].table);
  auto left_col = Expr::makeColumnRef(_strdup((yyvsp[-1].expr)->name));
  if((yyvsp[-1].expr)->alias) {
    left_col->alias = _strdup((yyvsp[-1].expr)->alias);
  }
  if((yyvsp[-7].table)->getName()) {
    left_col->table = _strdup((yyvsp[-7].table)->getName());
  }
  auto right_col = Expr::makeColumnRef(_strdup((yyvsp[-1].expr)->name));
  if((yyvsp[-1].expr)->alias) {
    right_col->alias = _strdup((yyvsp[-1].expr)->alias);
  }
  if((yyvsp[-4].table)->getName()) {
    right_col->table = _strdup((yyvsp[-4].table)->getName());
  }
  (yyval.table)->join->condition = Expr::makeOpBinary(left_col, kOpEquals, right_col);
  delete (yyvsp[-1].expr);
}
		break;

	case 325: /* opt_join_type: INNER  */
	                      { (yyval.join_type) = kJoinInner; }
		break;

	case 326: /* opt_join_type: LEFT OUTER  */
	             { (yyval.join_type) = kJoinLeft; }
		break;

	case 327: /* opt_join_type: LEFT  */
	       { (yyval.join_type) = kJoinLeft; }
		break;

	case 328: /* opt_join_type: RIGHT OUTER  */
	              { (yyval.join_type) = kJoinRight; }
		break;

	case 329: /* opt_join_type: RIGHT  */
	        { (yyval.join_type) = kJoinRight; }
		break;

	case 330: /* opt_join_type: FULL OUTER  */
	             { (yyval.join_type) = kJoinFull; }
		break;

	case 331: /* opt_join_type: OUTER  */
	        { (yyval.join_type) = kJoinFull; }
		break;

	case 332: /* opt_join_type: FULL  */
	       { (yyval.join_type) = kJoinFull; }
		break;

	case 333: /* opt_join_type: CROSS  */
	        { (yyval.join_type) = kJoinCross; }
		break;

	case 334: /* opt_join_type: %empty  */
	                       { (yyval.join_type) = kJoinInner; }
		break;

	case 338: /* ident_commalist: IDENTIFIER  */
	                             {
  (yyval.str_vec) = new std::vector<char*>();
  (yyval.str_vec)->push_back((yyvsp[0].sval));
}
		break;

	case 339: /* ident_commalist: ident_commalist ',' IDENTIFIER  */
	                                 {
  (yyvsp[-2].str_vec)->push_back((yyvsp[0].sval));
  (yyval.str_vec) = (yyvsp[-2].str_vec);
}
		break;



      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);
  YYPOPSTACK(yylen);
  yylen = 0;
  *++yyvsp = yyval;
  *++yylsp = yyloc;
  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp ? yytable[yyi] : yydefgoto[yylhs]);
  }
  goto yynewstate;
/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == SQL_HSQL_EMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if(!yyerrstatus) {
      ++yynerrs;
      {
        yypcontext_t yyctx = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if(yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if(yysyntax_error_status == -1) {
            if(yymsg != yymsgbuf)
              YYSTACK_FREE(yymsg);
            yymsg = YY_CAST(char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if(yymsg) {
                yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror(&yylloc, result, scanner, yymsgp);
        if(yysyntax_error_status == YYENOMEM)
          goto yyexhaustedlab;
      }
    }

  yyerror_range[1] = yylloc;
  if(yyerrstatus == 3) {
      /* If just tried and failed to reuse lookahead token after an error, discard it.  */
      if(yychar <= SQL_YYEOF) {
          /* Return failure if at end of input.  */
          if(yychar == SQL_YYEOF)
            YYABORT;
        }
      else {
          yydestruct("Error: discarding", yytoken, &yylval, &yylloc, result, scanner);
          yychar = SQL_HSQL_EMPTY;
        }
    }
  // Else will try to reuse lookahead token after shifting the error token.
  goto yyerrlab1;
/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if(0)
    YYERROR;
  // Do not reclaim the symbols of the rule whose action triggered this YYERROR. 
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;
/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */
  /* Pop stack until we find a state that shifts the error token.  */
  for(;;) {
      yyn = yypact[yystate];
      if(!yypact_value_is_default (yyn)) {
          yyn += YYSYMBOL_YYerror;
          if(0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror) {
              yyn = yytable[yyn];
              if(0 < yyn)
                break;
            }
        }
      /* Pop the current state because it cannot handle the error token.  */
      if(yyssp == yyss)
        YYABORT;
      yyerror_range[1] = *yylsp;
      yydestruct("Error: popping", YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, result, scanner);
      YYPOPSTACK(1);
      yystate = *yyssp;
      YY_STACK_PRINT(yyss, yyssp);
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);
  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);
  yystate = yyn;
  goto yynewstate;

/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
	yyresult = 0;
	goto yyreturn;
/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
	yyresult = 1;
	goto yyreturn;
#if 1
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
	yyerror(&yylloc, result, scanner, YY_("memory exhausted"));
	yyresult = 2;
	goto yyreturn;
#endif
/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
  if(yychar != SQL_HSQL_EMPTY) {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct("Cleanup: discarding lookahead", yytoken, &yylval, &yylloc, result, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while(yyssp != yyss) {
      yydestruct("Cleanup: popping", YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, result, scanner);
      YYPOPSTACK(1);
    }
#ifndef yyoverflow
  if(yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if(yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}


    // clang-format on
    /*********************************
 ** Section 4: Additional C code
 *********************************/

    /* empty */
