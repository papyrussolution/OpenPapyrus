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
//
// Section 1: C Declarations
//
#include <sql-parser.h>
#pragma hdrstop
#include "bison_parser.h"
#include "flex_lexer.h"

using namespace hsql;

int yyerror(YYLTYPE * llocp, SQLParserResult * result, yyscan_t scanner, const char* msg) 
{
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
  YYSYMBOL_DISTINCT = 10,                  /* DISTINCT  */
  YYSYMBOL_RESTRICT = 11,                  /* RESTRICT  */
  YYSYMBOL_TRUNCATE = 12,                  /* TRUNCATE  */
  YYSYMBOL_ANALYZE = 13,                   /* ANALYZE  */
  YYSYMBOL_BETWEEN = 14,                   /* BETWEEN  */
  YYSYMBOL_CASCADE = 15,                   /* CASCADE  */
  YYSYMBOL_COLUMNS = 16,                   /* COLUMNS  */
  YYSYMBOL_CONTROL = 17,                   /* CONTROL  */
  YYSYMBOL_DEFAULT = 18,                   /* DEFAULT  */
  YYSYMBOL_EXECUTE = 19,                   /* EXECUTE  */
  YYSYMBOL_EXPLAIN = 20,                   /* EXPLAIN  */
  YYSYMBOL_NATURAL = 21,                   /* NATURAL  */
  YYSYMBOL_PREPARE = 22,                   /* PREPARE  */
  YYSYMBOL_PRIMARY = 23,                   /* PRIMARY  */
  YYSYMBOL_SCHEMAS = 24,                   /* SCHEMAS  */
  YYSYMBOL_SPATIAL = 25,                   /* SPATIAL  */
  YYSYMBOL_VIRTUAL = 26,                   /* VIRTUAL  */
  YYSYMBOL_DESCRIBE = 27,                  /* DESCRIBE  */
  YYSYMBOL_BEFORE = 28,                    /* BEFORE  */
  YYSYMBOL_COLUMN = 29,                    /* COLUMN  */
  YYSYMBOL_CREATE = 30,                    /* CREATE  */
  YYSYMBOL_DELETE = 31,                    /* DELETE  */
  YYSYMBOL_DIRECT = 32,                    /* DIRECT  */
  YYSYMBOL_ESCAPE = 33,                    /* ESCAPE  */
  YYSYMBOL_EXCEPT = 34,                    /* EXCEPT  */
  YYSYMBOL_EXISTS = 35,                    /* EXISTS  */
  YYSYMBOL_EXTRACT = 36,                   /* EXTRACT  */
  YYSYMBOL_CAST = 37,                      /* CAST  */
  YYSYMBOL_WITH_FORMAT = 38,               /* WITH_FORMAT  */
  YYSYMBOL_GLOBAL = 39,                    /* GLOBAL  */
  YYSYMBOL_HAVING = 40,                    /* HAVING  */
  YYSYMBOL_IMPORT = 41,                    /* IMPORT  */
  YYSYMBOL_INSERT = 42,                    /* INSERT  */
  YYSYMBOL_ISNULL = 43,                    /* ISNULL  */
  YYSYMBOL_OFFSET = 44,                    /* OFFSET  */
  YYSYMBOL_RENAME = 45,                    /* RENAME  */
  YYSYMBOL_SCHEMA = 46,                    /* SCHEMA  */
  YYSYMBOL_SELECT = 47,                    /* SELECT  */
  YYSYMBOL_SORTED = 48,                    /* SORTED  */
  YYSYMBOL_TABLES = 49,                    /* TABLES  */
  YYSYMBOL_UNIQUE = 50,                    /* UNIQUE  */
  YYSYMBOL_AUTO_INCREMENT = 51,            /* AUTO_INCREMENT  */
  YYSYMBOL_UNLOAD = 52,                    /* UNLOAD  */
  YYSYMBOL_UPDATE = 53,                    /* UPDATE  */
  YYSYMBOL_VALUES = 54,                    /* VALUES  */
  YYSYMBOL_AFTER = 55,                     /* AFTER  */
  YYSYMBOL_ALTER = 56,                     /* ALTER  */
  YYSYMBOL_CROSS = 57,                     /* CROSS  */
  YYSYMBOL_DELTA = 58,                     /* DELTA  */
  YYSYMBOL_GROUP = 59,                     /* GROUP  */
  YYSYMBOL_INDEX = 60,                     /* INDEX  */
  YYSYMBOL_INNER = 61,                     /* INNER  */
  YYSYMBOL_LIMIT = 62,                     /* LIMIT  */
  YYSYMBOL_LOCAL = 63,                     /* LOCAL  */
  YYSYMBOL_MERGE = 64,                     /* MERGE  */
  YYSYMBOL_MINUS = 65,                     /* MINUS  */
  YYSYMBOL_ORDER = 66,                     /* ORDER  */
  YYSYMBOL_OVER = 67,                      /* OVER  */
  YYSYMBOL_OUTER = 68,                     /* OUTER  */
  YYSYMBOL_RIGHT = 69,                     /* RIGHT  */
  YYSYMBOL_TABLE = 70,                     /* TABLE  */
  YYSYMBOL_UNION = 71,                     /* UNION  */
  YYSYMBOL_USING = 72,                     /* USING  */
  YYSYMBOL_WHERE = 73,                     /* WHERE  */
  YYSYMBOL_CALL = 74,                      /* CALL  */
  YYSYMBOL_CASE = 75,                      /* CASE  */
  YYSYMBOL_COPY = 76,                      /* COPY  */
  YYSYMBOL_DESC = 77,                      /* DESC  */
  YYSYMBOL_DROP = 78,                      /* DROP  */
  YYSYMBOL_ELSE = 79,                      /* ELSE  */
  YYSYMBOL_FILE = 80,                      /* FILE  */
  YYSYMBOL_FROM = 81,                      /* FROM  */
  YYSYMBOL_FULL = 82,                      /* FULL  */
  YYSYMBOL_HASH = 83,                      /* HASH  */
  YYSYMBOL_HINT = 84,                      /* HINT  */
  YYSYMBOL_INTO = 85,                      /* INTO  */
  YYSYMBOL_JOIN = 86,                      /* JOIN  */
  YYSYMBOL_LEFT = 87,                      /* LEFT  */
  YYSYMBOL_LIKE = 88,                      /* LIKE  */
  YYSYMBOL_LOAD = 89,                      /* LOAD  */
  YYSYMBOL_NULL = 90,                      /* NULL  */
  YYSYMBOL_PARTITION = 91,                 /* PARTITION  */
  YYSYMBOL_PLAN = 92,                      /* PLAN  */
  YYSYMBOL_SHOW = 93,                      /* SHOW  */
  YYSYMBOL_TEXT = 94,                      /* TEXT  */
  YYSYMBOL_THEN = 95,                      /* THEN  */
  YYSYMBOL_VIEW = 96,                      /* VIEW  */
  YYSYMBOL_WHEN = 97,                      /* WHEN  */
  YYSYMBOL_WITH = 98,                      /* WITH  */
  YYSYMBOL_ADD = 99,                       /* ADD  */
  YYSYMBOL_ALL = 100,                      /* ALL  */
  YYSYMBOL_AND = 101,                      /* AND  */
  YYSYMBOL_ASC = 102,                      /* ASC  */
  YYSYMBOL_FOR = 103,                      /* FOR  */
  YYSYMBOL_KEY = 104,                      /* KEY  */
  YYSYMBOL_NOT = 105,                      /* NOT  */
  YYSYMBOL_OFF = 106,                      /* OFF  */
  YYSYMBOL_SET = 107,                      /* SET  */
  YYSYMBOL_TOP = 108,                      /* TOP  */
  YYSYMBOL_AS = 109,                       /* AS  */
  YYSYMBOL_BY = 110,                       /* BY  */
  YYSYMBOL_IF = 111,                       /* IF  */
  YYSYMBOL_IN = 112,                       /* IN  */
  YYSYMBOL_IS = 113,                       /* IS  */
  YYSYMBOL_OF = 114,                       /* OF  */
  YYSYMBOL_ON = 115,                       /* ON  */
  YYSYMBOL_OR = 116,                       /* OR  */
  YYSYMBOL_TO = 117,                       /* TO  */
  YYSYMBOL_NO = 118,                       /* NO  */
  YYSYMBOL_ARRAY = 119,                    /* ARRAY  */
  YYSYMBOL_CONCAT = 120,                   /* CONCAT  */
  YYSYMBOL_ILIKE = 121,                    /* ILIKE  */
  YYSYMBOL_SECOND = 122,                   /* SECOND  */
  YYSYMBOL_MINUTE = 123,                   /* MINUTE  */
  YYSYMBOL_HOUR = 124,                     /* HOUR  */
  YYSYMBOL_DAY = 125,                      /* DAY  */
  YYSYMBOL_MONTH = 126,                    /* MONTH  */
  YYSYMBOL_YEAR = 127,                     /* YEAR  */
  YYSYMBOL_SECONDS = 128,                  /* SECONDS  */
  YYSYMBOL_MINUTES = 129,                  /* MINUTES  */
  YYSYMBOL_HOURS = 130,                    /* HOURS  */
  YYSYMBOL_DAYS = 131,                     /* DAYS  */
  YYSYMBOL_MONTHS = 132,                   /* MONTHS  */
  YYSYMBOL_YEARS = 133,                    /* YEARS  */
  YYSYMBOL_INTERVAL = 134,                 /* INTERVAL  */
  YYSYMBOL_TRUE = 135,                     /* TRUE  */
  YYSYMBOL_FALSE = 136,                    /* FALSE  */
  YYSYMBOL_TRANSACTION = 137,              /* TRANSACTION  */
  YYSYMBOL_BEGIN = 138,                    /* BEGIN  */
  YYSYMBOL_COMMIT = 139,                   /* COMMIT  */
  YYSYMBOL_ROLLBACK = 140,                 /* ROLLBACK  */
  YYSYMBOL_NOWAIT = 141,                   /* NOWAIT  */
  YYSYMBOL_SKIP = 142,                     /* SKIP  */
  YYSYMBOL_LOCKED = 143,                   /* LOCKED  */
  YYSYMBOL_SHARE = 144,                    /* SHARE  */
  YYSYMBOL_RANGE = 145,                    /* RANGE  */
  YYSYMBOL_ROWS = 146,                     /* ROWS  */
  YYSYMBOL_GROUPS = 147,                   /* GROUPS  */
  YYSYMBOL_UNBOUNDED = 148,                /* UNBOUNDED  */
  YYSYMBOL_FOLLOWING = 149,                /* FOLLOWING  */
  YYSYMBOL_PRECEDING = 150,                /* PRECEDING  */
  YYSYMBOL_CURRENT_ROW = 151,              /* CURRENT_ROW  */
  YYSYMBOL_CHARACTER_SET = 152,            /* CHARACTER_SET  */
  YYSYMBOL_CHARACTER_VARYING = 153,        /* CHARACTER_VARYING  */
  YYSYMBOL_154_ = 154,                     /* '='  */
  YYSYMBOL_EQUALS = 155,                   /* EQUALS  */
  YYSYMBOL_NOTEQUALS = 156,                /* NOTEQUALS  */
  YYSYMBOL_157_ = 157,                     /* '<'  */
  YYSYMBOL_158_ = 158,                     /* '>'  */
  YYSYMBOL_LESS = 159,                     /* LESS  */
  YYSYMBOL_GREATER = 160,                  /* GREATER  */
  YYSYMBOL_LESSEQ = 161,                   /* LESSEQ  */
  YYSYMBOL_GREATEREQ = 162,                /* GREATEREQ  */
  YYSYMBOL_NOTNULL = 163,                  /* NOTNULL  */
  YYSYMBOL_164_ = 164,                     /* '+'  */
  YYSYMBOL_165_ = 165,                     /* '-'  */
  YYSYMBOL_166_ = 166,                     /* '*'  */
  YYSYMBOL_167_ = 167,                     /* '/'  */
  YYSYMBOL_168_ = 168,                     /* '%'  */
  YYSYMBOL_169_ = 169,                     /* '^'  */
  YYSYMBOL_UMINUS = 170,                   /* UMINUS  */
  YYSYMBOL_171_ = 171,                     /* '['  */
  YYSYMBOL_172_ = 172,                     /* ']'  */
  YYSYMBOL_173_ = 173,                     /* '('  */
  YYSYMBOL_174_ = 174,                     /* ')'  */
  YYSYMBOL_175_ = 175,                     /* '.'  */
  YYSYMBOL_176_ = 176,                     /* ';'  */
  YYSYMBOL_177_ = 177,                     /* ','  */
  YYSYMBOL_178_BINARY_ = 178,              /* "BINARY"  */
  YYSYMBOL_179_END_ = 179,                 /* "END"  */
  YYSYMBOL_180_ = 180,                     /* '?'  */
  YYSYMBOL_YYACCEPT = 181,                 /* $accept  */
  YYSYMBOL_input = 182,                    /* input  */
  YYSYMBOL_statement_list = 183,           /* statement_list  */
  YYSYMBOL_statement = 184,                /* statement  */
  YYSYMBOL_preparable_statement = 185,     /* preparable_statement  */
  YYSYMBOL_opt_hints = 186,                /* opt_hints  */
  YYSYMBOL_hint_list = 187,                /* hint_list  */
  YYSYMBOL_hint = 188,                     /* hint  */
  YYSYMBOL_transaction_statement = 189,    /* transaction_statement  */
  YYSYMBOL_opt_transaction_keyword = 190,  /* opt_transaction_keyword  */
  YYSYMBOL_prepare_statement = 191,        /* prepare_statement  */
  YYSYMBOL_prepare_target_query = 192,     /* prepare_target_query  */
  YYSYMBOL_execute_statement = 193,        /* execute_statement  */
  YYSYMBOL_import_statement = 194,         /* import_statement  */
  YYSYMBOL_file_type = 195,                /* file_type  */
  YYSYMBOL_file_path = 196,                /* file_path  */
  YYSYMBOL_opt_file_type = 197,            /* opt_file_type  */
  YYSYMBOL_export_statement = 198,         /* export_statement  */
  YYSYMBOL_show_statement = 199,           /* show_statement  */
  YYSYMBOL_create_statement = 200,         /* create_statement  */
  YYSYMBOL_opt_not_exists = 201,           /* opt_not_exists  */
  YYSYMBOL_table_elem_commalist = 202,     /* table_elem_commalist  */
  YYSYMBOL_table_elem = 203,               /* table_elem  */
  YYSYMBOL_column_def = 204,               /* column_def  */
  YYSYMBOL_column_type_id_ = 205,          /* column_type_id_  */
  YYSYMBOL_opt_column_constraints = 206,   /* opt_column_constraints  */
  YYSYMBOL_opt_character_set = 207,        /* opt_character_set  */
  YYSYMBOL_column_constraint_set = 208,    /* column_constraint_set  */
  YYSYMBOL_column_constraint = 209,        /* column_constraint  */
  YYSYMBOL_table_constraint = 210,         /* table_constraint  */
  YYSYMBOL_drop_statement = 211,           /* drop_statement  */
  YYSYMBOL_opt_exists = 212,               /* opt_exists  */
  YYSYMBOL_alter_statement = 213,          /* alter_statement  */
  YYSYMBOL_alter_action = 214,             /* alter_action  */
  YYSYMBOL_drop_action = 215,              /* drop_action  */
  YYSYMBOL_delete_statement = 216,         /* delete_statement  */
  YYSYMBOL_truncate_statement = 217,       /* truncate_statement  */
  YYSYMBOL_insert_statement = 218,         /* insert_statement  */
  YYSYMBOL_opt_column_list = 219,          /* opt_column_list  */
  YYSYMBOL_update_statement = 220,         /* update_statement  */
  YYSYMBOL_update_clause_commalist = 221,  /* update_clause_commalist  */
  YYSYMBOL_update_clause = 222,            /* update_clause  */
  YYSYMBOL_select_statement = 223,         /* select_statement  */
  YYSYMBOL_select_within_set_operation = 224, /* select_within_set_operation  */
  YYSYMBOL_select_within_set_operation_no_parentheses = 225, /* select_within_set_operation_no_parentheses  */
  YYSYMBOL_select_with_paren = 226,        /* select_with_paren  */
  YYSYMBOL_select_no_paren = 227,          /* select_no_paren  */
  YYSYMBOL_set_operator = 228,             /* set_operator  */
  YYSYMBOL_set_type = 229,                 /* set_type  */
  YYSYMBOL_opt_all = 230,                  /* opt_all  */
  YYSYMBOL_select_clause = 231,            /* select_clause  */
  YYSYMBOL_opt_distinct = 232,             /* opt_distinct  */
  YYSYMBOL_select_list = 233,              /* select_list  */
  YYSYMBOL_opt_from_clause = 234,          /* opt_from_clause  */
  YYSYMBOL_from_clause = 235,              /* from_clause  */
  YYSYMBOL_opt_where = 236,                /* opt_where  */
  YYSYMBOL_opt_group = 237,                /* opt_group  */
  YYSYMBOL_opt_having = 238,               /* opt_having  */
  YYSYMBOL_opt_order = 239,                /* opt_order  */
  YYSYMBOL_order_list = 240,               /* order_list  */
  YYSYMBOL_order_desc = 241,               /* order_desc  */
  YYSYMBOL_opt_order_type = 242,           /* opt_order_type  */
  YYSYMBOL_opt_top = 243,                  /* opt_top  */
  YYSYMBOL_opt_limit = 244,                /* opt_limit  */
  YYSYMBOL_expr_list = 245,                /* expr_list  */
  YYSYMBOL_opt_literal_list = 246,         /* opt_literal_list  */
  YYSYMBOL_literal_list = 247,             /* literal_list  */
  YYSYMBOL_expr_alias = 248,               /* expr_alias  */
  YYSYMBOL_expr = 249,                     /* expr  */
  YYSYMBOL_operand = 250,                  /* operand  */
  YYSYMBOL_scalar_expr = 251,              /* scalar_expr  */
  YYSYMBOL_unary_expr = 252,               /* unary_expr  */
  YYSYMBOL_binary_expr = 253,              /* binary_expr  */
  YYSYMBOL_logic_expr = 254,               /* logic_expr  */
  YYSYMBOL_in_expr = 255,                  /* in_expr  */
  YYSYMBOL_case_expr = 256,                /* case_expr  */
  YYSYMBOL_case_list = 257,                /* case_list  */
  YYSYMBOL_exists_expr = 258,              /* exists_expr  */
  YYSYMBOL_comp_expr = 259,                /* comp_expr  */
  YYSYMBOL_function_expr = 260,            /* function_expr  */
  YYSYMBOL_opt_window = 261,               /* opt_window  */
  YYSYMBOL_opt_partition = 262,            /* opt_partition  */
  YYSYMBOL_opt_frame_clause = 263,         /* opt_frame_clause  */
  YYSYMBOL_frame_type = 264,               /* frame_type  */
  YYSYMBOL_frame_bound = 265,              /* frame_bound  */
  YYSYMBOL_extract_expr = 266,             /* extract_expr  */
  YYSYMBOL_cast_expr = 267,                /* cast_expr  */
  YYSYMBOL_datetime_field = 268,           /* datetime_field  */
  YYSYMBOL_datetime_field_plural = 269,    /* datetime_field_plural  */
  YYSYMBOL_duration_field = 270,           /* duration_field  */
  YYSYMBOL_array_expr = 271,               /* array_expr  */
  YYSYMBOL_array_index = 272,              /* array_index  */
  YYSYMBOL_between_expr = 273,             /* between_expr  */
  YYSYMBOL_column_name = 274,              /* column_name  */
  YYSYMBOL_literal = 275,                  /* literal  */
  YYSYMBOL_string_literal = 276,           /* string_literal  */
  YYSYMBOL_bool_literal = 277,             /* bool_literal  */
  YYSYMBOL_num_literal = 278,              /* num_literal  */
  YYSYMBOL_int_literal = 279,              /* int_literal  */
  YYSYMBOL_null_literal = 280,             /* null_literal  */
  YYSYMBOL_interval_literal = 281,         /* interval_literal  */
  YYSYMBOL_param_expr = 282,               /* param_expr  */
  YYSYMBOL_table_ref = 283,                /* table_ref  */
  YYSYMBOL_table_ref_atomic = 284,         /* table_ref_atomic  */
  YYSYMBOL_nonjoin_table_ref_atomic = 285, /* nonjoin_table_ref_atomic  */
  YYSYMBOL_table_ref_commalist = 286,      /* table_ref_commalist  */
  YYSYMBOL_table_ref_name = 287,           /* table_ref_name  */
  YYSYMBOL_table_ref_name_no_alias = 288,  /* table_ref_name_no_alias  */
  YYSYMBOL_table_name = 289,               /* table_name  */
  YYSYMBOL_opt_index_name = 290,           /* opt_index_name  */
  YYSYMBOL_table_alias = 291,              /* table_alias  */
  YYSYMBOL_opt_table_alias = 292,          /* opt_table_alias  */
  YYSYMBOL_alias = 293,                    /* alias  */
  YYSYMBOL_opt_alias = 294,                /* opt_alias  */
  YYSYMBOL_opt_locking_clause = 295,       /* opt_locking_clause  */
  YYSYMBOL_opt_locking_clause_list = 296,  /* opt_locking_clause_list  */
  YYSYMBOL_locking_clause = 297,           /* locking_clause  */
  YYSYMBOL_row_lock_mode = 298,            /* row_lock_mode  */
  YYSYMBOL_opt_row_lock_policy = 299,      /* opt_row_lock_policy  */
  YYSYMBOL_opt_with_clause = 300,          /* opt_with_clause  */
  YYSYMBOL_with_clause = 301,              /* with_clause  */
  YYSYMBOL_with_description_list = 302,    /* with_description_list  */
  YYSYMBOL_with_description = 303,         /* with_description  */
  YYSYMBOL_join_clause = 304,              /* join_clause  */
  YYSYMBOL_opt_join_type = 305,            /* opt_join_type  */
  YYSYMBOL_join_condition = 306,           /* join_condition  */
  YYSYMBOL_opt_semicolon = 307,            /* opt_semicolon  */
  YYSYMBOL_ident_commalist = 308,          /* ident_commalist  */
  YYSYMBOL_index_segment_flags = 309,      /* index_segment_flags  */
  YYSYMBOL_index_segment = 310,            /* index_segment  */
  YYSYMBOL_index_segment_list = 311        /* index_segment_list  */
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
#define YYLAST   806 /* YYLAST -- Last index in YYTABLE.  */
#define YYNTOKENS  181 /* YYNTOKENS -- Number of terminals.  */
#define YYNNTS  131 /* YYNNTS -- Number of nonterminals.  */
#define YYNRULES  325 /* YYNRULES -- Number of rules.  */
#define YYNSTATES  595 /* YYNSTATES -- Number of states.  */
#define YYMAXUTOK   418  /* YYMAXUTOK -- Last valid token kind.  */


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
       2,     2,     2,     2,     2,     2,     2,   168,     2,     2,
     173,   174,   166,   164,   177,   165,   175,   167,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   176,
     157,   154,   158,   180,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   171,     2,   172,   169,     2,     2,     2,     2,     2,
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
     145,   146,   147,   148,   149,   150,   151,   152,   153,   155,
     156,   159,   160,   161,   162,   163,   170,   178,   179
};

#if HSQL_DEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   472,   472,   490,   495,   502,   505,   508,   508,   508,
     510,   511,   512,   513,   514,   515,   516,   517,   518,   519,
     523,   523,   525,   528,   533,   536,   544,   545,   546,   548,
     548,   552,   558,   558,   561,   571,   576,   584,   600,   604,
     609,   609,   614,   619,   628,   630,   634,   644,   656,   667,
     673,   683,   693,   702,   702,   704,   707,   712,   712,   715,
     724,   731,   738,   745,   784,   784,   785,   785,   787,   790,
     795,   796,   797,   798,   799,   802,   803,   810,   815,   820,
     824,   830,   830,   835,   841,   843,   852,   859,   869,   875,
     883,   883,   889,   896,   899,   904,   912,   915,   918,   930,
     930,   932,   933,   942,   942,   944,   955,   967,   972,   975,
     978,   983,   983,   985,   995,   995,   996,   997,   997,   999,
    1000,  1000,  1002,  1006,  1008,  1008,  1009,  1009,  1011,  1014,
    1019,  1020,  1020,  1020,  1024,  1024,  1026,  1027,  1028,  1029,
    1030,  1031,  1035,  1038,  1043,  1043,  1045,  1048,  1053,  1061,
    1061,  1061,  1061,  1061,  1063,  1064,  1064,  1064,  1064,  1064,
    1064,  1064,  1064,  1065,  1065,  1069,  1069,  1071,  1072,  1073,
    1074,  1075,  1077,  1077,  1078,  1079,  1080,  1081,  1082,  1083,
    1084,  1085,  1086,  1088,  1089,  1091,  1092,  1093,  1094,  1098,
    1099,  1100,  1101,  1103,  1104,  1106,  1107,  1109,  1110,  1111,
    1112,  1113,  1114,  1115,  1119,  1120,  1124,  1125,  1127,  1128,
    1133,  1134,  1135,  1139,  1140,  1141,  1143,  1144,  1145,  1146,
    1147,  1149,  1151,  1153,  1154,  1155,  1156,  1157,  1158,  1160,
    1161,  1162,  1163,  1164,  1165,  1167,  1167,  1168,  1169,  1170,
    1172,  1173,  1174,  1175,  1177,  1177,  1177,  1177,  1177,  1177,
    1178,  1179,  1179,  1180,  1180,  1181,  1182,  1196,  1199,  1209,
    1246,  1254,  1254,  1261,  1261,  1263,  1263,  1270,  1273,  1278,
    1286,  1292,  1295,  1300,  1300,  1301,  1301,  1302,  1302,  1303,
    1303,  1304,  1304,  1308,  1309,  1311,  1314,  1319,  1324,  1331,
    1332,  1333,  1334,  1336,  1337,  1338,  1342,  1342,  1344,  1346,
    1350,  1355,  1363,  1369,  1376,  1400,  1401,  1402,  1403,  1404,
    1405,  1406,  1407,  1408,  1409,  1411,  1415,  1415,  1417,  1420,
    1425,  1427,  1429,  1433,  1440,  1443
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
  "DISTINCT", "RESTRICT", "TRUNCATE", "ANALYZE", "BETWEEN", "CASCADE",
  "COLUMNS", "CONTROL", "DEFAULT", "EXECUTE", "EXPLAIN", "NATURAL",
  "PREPARE", "PRIMARY", "SCHEMAS", "SPATIAL", "VIRTUAL", "DESCRIBE",
  "BEFORE", "COLUMN", "CREATE", "DELETE", "DIRECT", "ESCAPE", "EXCEPT",
  "EXISTS", "EXTRACT", "CAST", "WITH_FORMAT", "GLOBAL", "HAVING", "IMPORT",
  "INSERT", "ISNULL", "OFFSET", "RENAME", "SCHEMA", "SELECT", "SORTED",
  "TABLES", "UNIQUE", "AUTO_INCREMENT", "UNLOAD", "UPDATE", "VALUES",
  "AFTER", "ALTER", "CROSS", "DELTA", "GROUP", "INDEX", "INNER", "LIMIT",
  "LOCAL", "MERGE", "MINUS", "ORDER", "OVER", "OUTER", "RIGHT", "TABLE",
  "UNION", "USING", "WHERE", "CALL", "CASE", "COPY", "DESC", "DROP",
  "ELSE", "FILE", "FROM", "FULL", "HASH", "HINT", "INTO", "JOIN", "LEFT",
  "LIKE", "LOAD", "NULL", "PARTITION", "PLAN", "SHOW", "TEXT", "THEN",
  "VIEW", "WHEN", "WITH", "ADD", "ALL", "AND", "ASC", "FOR", "KEY", "NOT",
  "OFF", "SET", "TOP", "AS", "BY", "IF", "IN", "IS", "OF", "ON", "OR",
  "TO", "NO", "ARRAY", "CONCAT", "ILIKE", "SECOND", "MINUTE", "HOUR",
  "DAY", "MONTH", "YEAR", "SECONDS", "MINUTES", "HOURS", "DAYS", "MONTHS",
  "YEARS", "INTERVAL", "TRUE", "FALSE", "TRANSACTION", "BEGIN", "COMMIT",
  "ROLLBACK", "NOWAIT", "SKIP", "LOCKED", "SHARE", "RANGE", "ROWS",
  "GROUPS", "UNBOUNDED", "FOLLOWING", "PRECEDING", "CURRENT_ROW",
  "CHARACTER_SET", "CHARACTER_VARYING", "'='", "EQUALS", "NOTEQUALS",
  "'<'", "'>'", "LESS", "GREATER", "LESSEQ", "GREATEREQ", "NOTNULL", "'+'",
  "'-'", "'*'", "'/'", "'%'", "'^'", "UMINUS", "'['", "']'", "'('", "')'",
  "'.'", "';'", "','", "\"BINARY\"", "\"END\"", "'?'", "$accept", "input",
  "statement_list", "statement", "preparable_statement", "opt_hints",
  "hint_list", "hint", "transaction_statement", "opt_transaction_keyword",
  "prepare_statement", "prepare_target_query", "execute_statement",
  "import_statement", "file_type", "file_path", "opt_file_type",
  "export_statement", "show_statement", "create_statement",
  "opt_not_exists", "table_elem_commalist", "table_elem", "column_def",
  "column_type_id_", "opt_column_constraints", "opt_character_set",
  "column_constraint_set", "column_constraint", "table_constraint",
  "drop_statement", "opt_exists", "alter_statement", "alter_action",
  "drop_action", "delete_statement", "truncate_statement",
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
  "null_literal", "interval_literal", "param_expr", "table_ref",
  "table_ref_atomic", "nonjoin_table_ref_atomic", "table_ref_commalist",
  "table_ref_name", "table_ref_name_no_alias", "table_name",
  "opt_index_name", "table_alias", "opt_table_alias", "alias", "opt_alias",
  "opt_locking_clause", "opt_locking_clause_list", "locking_clause",
  "row_lock_mode", "opt_row_lock_policy", "opt_with_clause", "with_clause",
  "with_description_list", "with_description", "join_clause",
  "opt_join_type", "join_condition", "opt_semicolon", "ident_commalist",
  "index_segment_flags", "index_segment", "index_segment_list", YY_NULLPTR
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
     405,   406,   407,   408,    61,   409,   410,    60,    62,   411,
     412,   413,   414,   415,    43,    45,    42,    47,    37,    94,
     416,    91,    93,    40,    41,    46,    59,    44,   417,   418,
      63
};
#endif

#define YYPACT_NINF (-443)
#define yypact_value_is_default(Yyn) ((Yyn) == YYPACT_NINF)
#define YYTABLE_NINF (-317)
#define yytable_value_is_error(Yyn) ((Yyn) == YYTABLE_NINF)

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     587,    41,    77,   104,   192,    77,   171,    12,   120,   129,
      77,   170,    20,   175,    59,   254,   131,   131,   131,   274,
     125,  -443,   198,  -443,   198,  -443,  -443,  -443,  -443,  -443,
    -443,  -443,  -443,  -443,  -443,  -443,  -443,   -11,  -443,   307,
     146,  -443,   179,   262,  -443,   287,   251,   251,   251,    77,
      16,    77,   266,  -443,   265,   -11,   261,    90,   265,   265,
     265,    77,  -443,   280,   216,  -443,  -443,  -443,  -443,  -443,
    -443,   581,  -443,   327,  -443,  -443,   304,   166,  -443,   189,
    -443,   420,    49,   423,   251,   323,   426,    77,    77,   363,
    -443,  -443,   362,   270,   441,   411,    77,   275,   283,   454,
     454,   454,   445,    77,    77,  -443,   286,   254,  -443,   289,
     457,   456,  -443,  -443,  -443,   -11,   364,   357,   -11,    17,
    -443,  -443,  -443,  -443,  -443,   465,  -443,  -443,  -443,   296,
     295,  -443,  -443,  -443,  -443,   276,  -443,  -443,  -443,  -443,
    -443,   426,   438,  -443,   361,   -38,   270,   355,  -443,   454,
     472,   248,   324,   -27,  -443,  -443,   399,  -443,  -443,   442,
    -443,   442,   442,  -443,  -443,  -443,  -443,  -443,   478,  -443,
    -443,   355,   416,  -443,  -443,   166,  -443,  -443,   355,   416,
     355,   173,   381,   444,  -443,    49,  -443,  -443,  -443,  -443,
    -443,  -443,  -443,  -443,  -443,  -443,  -443,  -443,  -443,  -443,
    -443,   372,  -443,    77,   485,   394,    55,   389,   147,   326,
     329,   330,   214,   378,   333,   360,  -443,   320,   161,   418,
    -443,  -443,  -443,  -443,  -443,  -443,  -443,  -443,  -443,  -443,
    -443,  -443,  -443,  -443,  -443,  -443,   424,  -443,   126,   332,
    -443,   355,   441,  -443,   481,  -443,  -443,    16,  -443,   363,
    -443,   342,   141,  -443,   435,   340,  -443,    36,    17,   -11,
     347,  -443,   -29,    17,   161,   483,    58,    98,  -443,   381,
    -443,  -443,  -443,    77,   356,   461,  -443,    27,   430,   374,
     153,  -443,  -443,  -443,   394,    14,    25,   489,   444,   355,
     355,   197,   -30,   375,   360,   576,   355,   119,   386,   -33,
     355,   355,   360,  -443,   360,   181,   390,   194,   360,   360,
     360,   360,   360,   360,   360,   360,   360,   360,   360,   360,
     360,   360,   360,   457,    77,  -443,   558,    49,   161,  -443,
     265,  -443,  -443,    49,  -443,   478,    23,   363,  -443,   355,
    -443,   559,  -443,  -443,  -443,  -443,   355,  -443,  -443,  -443,
     381,   355,   355,  -443,   433,   460,  -443,   112,  -443,   405,
     588,   454,   419,   422,   174,   425,   472,  -443,    55,  -443,
     529,   355,  -443,  -443,   427,   516,   127,   164,   -23,   355,
     355,  -443,   489,   514,   -14,  -443,  -443,  -443,   503,   547,
     635,   360,   432,   320,  -443,   517,   439,   635,   635,   635,
     635,   388,   388,   388,   388,   119,   119,   -54,   -54,   -54,
     -58,   443,  -443,  -443,   154,   610,   160,  -443,   394,  -443,
      79,  -443,   447,  -443,    35,  -443,   557,  -443,  -443,  -443,
    -443,   161,   161,  -443,   567,   472,  -443,   482,  -443,   588,
    -443,  -443,   167,  -443,   615,   620,   523,  -443,  -443,  -443,
     540,   479,   174,  -443,   472,   168,  -443,   459,  -443,   177,
    -443,   355,    27,   355,   355,  -443,   -24,   196,   462,  -443,
     360,   635,   320,   464,   200,  -443,  -443,  -443,  -443,  -443,
     467,   553,  -443,  -443,  -443,   565,   574,   577,   560,    23,
     641,  -443,  -443,  -443,   537,  -443,  -443,   -91,  -443,   211,
     136,  -443,   588,   243,   475,  -443,  -443,   647,  -443,  -443,
     247,  -443,   562,   529,   -17,   477,   161,   -19,  -443,   355,
    -443,   576,   480,   263,  -443,  -443,    35,    23,  -443,  -443,
    -443,    23,   365,   488,   355,  -443,  -443,  -443,  -443,  -443,
    -443,   649,  -443,  -443,  -443,   546,   416,  -443,  -443,  -443,
    -443,   161,  -443,  -443,  -443,  -443,   450,   472,    -5,   484,
     355,    22,   493,   355,   264,   355,  -443,  -443,   340,  -443,
    -443,  -443,   495,    38,    26,   161,  -443,  -443,   161,  -443,
     156,    39,   186,  -443,  -443,   487,   496,  -443,  -443,   570,
    -443,  -443,  -443,    39,  -443
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
     297,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    30,    30,    30,     0,
     317,     3,    21,    19,    21,    18,     8,     9,     7,    11,
      16,    17,    13,    14,    12,    15,    10,     0,   296,     0,
     271,    87,    33,     0,    46,     0,    54,    54,    54,     0,
       0,     0,     0,   270,    82,     0,     0,     0,    82,    82,
      82,     0,    44,     0,   298,   299,    29,    26,    28,    27,
       1,   297,     2,     0,     6,     5,   135,    96,    97,   127,
      79,     0,   145,     0,    54,     0,   274,     0,     0,   121,
      37,    38,     0,    91,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,     0,     0,     4,     0,
       0,   115,   109,   110,   108,     0,   112,     0,     0,   141,
     272,   250,   253,   255,   256,     0,   251,   252,   260,     0,
     144,   146,   244,   245,   246,   254,   247,   248,   249,    32,
      31,   274,     0,   273,     0,     0,    91,     0,    86,     0,
       0,     0,     0,   121,    93,    81,     0,   104,   103,    41,
      39,    41,    41,    80,    77,    78,   301,   300,     0,   134,
     114,     0,   127,   100,    99,   101,   111,   107,     0,   127,
       0,     0,   284,   259,    34,     0,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     257,     0,    53,     0,     0,   297,     0,     0,   240,     0,
       0,     0,     0,     0,     0,     0,   242,     0,   120,   149,
     156,   157,   158,   151,   153,   159,   152,   172,   160,   161,
     162,   163,   155,   150,   165,   166,     0,   318,     0,     0,
      89,     0,     0,    92,     0,    83,    84,     0,    43,   121,
      42,    24,     0,    22,   118,   116,   142,   282,   141,     0,
     126,   128,   133,   141,   137,   139,   136,     0,   105,   283,
     285,   258,   147,     0,     0,     0,    49,     0,     0,     0,
       0,    55,    57,    58,   297,   115,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   168,     0,   167,     0,     0,
       0,     0,     0,   169,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    90,     0,     0,    95,    94,
      82,    40,    36,     0,    20,     0,     0,   121,   117,     0,
     280,     0,   281,   148,    98,   102,     0,   132,   131,   130,
     284,     0,     0,   289,     0,     0,   291,   295,   286,     0,
       0,     0,    61,     0,    65,     0,     0,    48,     0,    52,
     207,     0,   241,   243,     0,     0,     0,     0,     0,     0,
       0,   191,     0,     0,     0,   164,   154,   183,   184,     0,
     179,     0,     0,     0,   170,     0,   182,   181,   197,   198,
     199,   200,   201,   202,   203,   174,   173,   176,   175,   177,
     178,     0,    35,   319,     0,     0,     0,    23,   297,   119,
     261,   263,     0,   265,   278,   264,   123,   143,   279,   129,
     106,   140,   138,   292,     0,     0,   294,     0,   287,     0,
     322,   324,     0,    47,     0,     0,     0,    71,    74,    72,
       0,    67,    64,    68,     0,     0,    56,     0,   204,     0,
     195,     0,     0,     0,     0,   189,     0,     0,     0,   237,
       0,   180,     0,     0,     0,   171,   238,    88,    85,    25,
       0,     0,   313,   305,   311,   309,   312,   307,     0,     0,
       0,   277,   269,   275,     0,   113,   290,   295,   293,     0,
     323,    50,     0,     0,     0,    70,    73,     0,    59,    69,
       0,    76,   209,   207,     0,     0,   193,     0,   192,     0,
     196,   239,     0,     0,   187,   185,   278,     0,   308,   310,
     306,     0,   262,   279,     0,   288,    51,   321,   320,   325,
      62,     0,    60,    66,    75,     0,   127,   205,   221,   222,
     190,   194,   188,   186,   266,   302,   314,     0,   125,     0,
       0,   212,     0,     0,     0,     0,   122,    63,   208,   213,
     214,   215,     0,     0,     0,   315,   303,   276,   124,   206,
       0,     0,     0,   220,   210,   240,     0,   219,   217,     0,
     218,   216,   304,     0,   211
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -443,  -443,  -443,   601,  -443,   651,  -443,   338,  -443,   351,
    -443,  -443,  -443,  -443,   429,   -89,   209,  -443,  -443,  -443,
     199,  -443,   309,  -443,   220,  -443,  -443,  -443,   231,  -443,
    -443,   -55,  -443,  -443,  -443,  -443,  -443,  -443,   538,  -443,
    -443,   446,  -196,   -86,  -443,    64,   -53,   -37,  -443,  -443,
     -78,   401,  -443,  -443,  -443,  -131,  -443,  -443,  -171,  -443,
     341,  -443,  -443,     3,  -290,  -443,  -242,   352,  -147,  -188,
    -443,  -443,  -443,  -443,  -443,  -443,   402,  -443,  -443,  -443,
     182,  -443,  -443,  -443,  -366,  -443,  -443,  -142,  -443,  -443,
    -443,  -443,  -443,   116,   -75,   -85,  -443,  -443,   -93,  -443,
    -443,  -443,  -443,  -442,   165,  -443,  -443,  -443,     8,   566,
    -443,   172,   437,  -443,   349,  -443,   431,  -443,   213,  -443,
    -443,  -443,   599,  -443,  -443,  -443,  -443,  -345,  -443,   215,
     285
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,    21,    22,    74,   252,   253,    23,    67,
      24,   140,    25,    26,    92,   159,   248,    27,    28,    29,
      86,   280,   281,   282,   364,   451,   508,   452,   453,   283,
      30,    96,    31,   245,   246,    32,    33,    34,   151,    35,
     153,   154,    36,   172,   173,   174,    78,   115,   116,   177,
      79,   171,   254,   337,   338,   148,   495,   566,   119,   260,
     261,   349,   111,   182,   255,   129,   130,   256,   257,   219,
     220,   221,   222,   223,   224,   225,   292,   226,   227,   228,
     458,   546,   572,   573,   584,   229,   230,   198,   199,   200,
     231,   232,   233,   234,   235,   132,   133,   134,   135,   136,
     137,   138,   419,   420,   421,   422,   423,    52,   424,   144,
     491,   492,   493,   343,   268,   269,   270,   357,   438,    37,
      38,    64,    65,   425,   488,   576,    72,   238,   500,   441,
     442
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     218,   258,    98,   102,   103,   104,   384,   131,   263,   276,
      41,   161,   162,    44,   160,   160,   160,   169,    53,    90,
      57,   455,   243,    40,   170,   295,    40,   297,   372,   585,
     362,   262,   179,   264,   266,   565,    76,   175,   340,   340,
     175,   271,   118,   204,   580,   580,   147,   532,   347,   379,
     436,   437,   581,   121,   122,   123,   464,    89,   277,    93,
     236,   180,   308,    39,   160,   291,   308,   380,   300,   105,
     299,   205,   300,   348,   380,    61,    56,   300,   278,   181,
      40,   459,   300,   301,   300,   414,   326,   301,   369,   556,
     497,   416,   301,    49,   328,   145,   146,   301,   240,   301,
     481,    77,   352,   474,   156,   279,   295,    42,    62,   510,
     272,   164,   165,   323,   389,   322,   390,   323,   332,    97,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   206,   482,   300,   259,   124,
     483,   386,   376,   377,   490,   341,   375,   484,   485,   381,
     242,   353,   301,   387,   388,   518,   465,   548,   469,   300,
     550,   486,    55,   339,   298,  -314,   487,   569,   570,   571,
     166,   100,   339,   345,   301,   112,   208,   121,   122,   123,
     363,   175,   523,   125,   126,   127,   582,   582,   370,   583,
     583,   373,   216,    55,    91,    43,   418,   446,   112,   262,
     113,    50,   354,   471,   431,   432,   426,   101,   209,   210,
     211,   274,   564,   537,    51,   589,   355,   208,   121,   122,
     123,    45,   480,   113,   447,   448,   435,   594,   300,   128,
     411,    46,   466,   467,   374,    58,   462,   114,   538,   308,
      54,    47,   356,   301,   558,    59,    87,    88,   212,   209,
     210,   211,   131,   436,   437,   117,  -267,    63,   131,   463,
     114,   344,   300,   124,   449,   300,   350,    48,    66,   391,
     568,    60,   443,   265,    70,   415,   160,   301,   213,   450,
     301,   359,   521,   141,   394,   319,   320,   321,   322,   212,
     323,   519,   214,   392,   290,    76,    73,   300,   300,   395,
     325,    71,   239,   326,   124,   587,   588,   125,   126,   127,
      80,   290,   301,   301,   514,   334,   516,   517,   335,   213,
     285,    81,   286,   208,   121,   122,   123,   367,   477,   468,
     368,   185,   412,   214,   479,   590,   591,   185,   215,   216,
     473,   501,   511,    83,   502,   326,   217,    84,   125,   126,
     127,   513,    82,   128,   339,   209,   210,   211,   208,   121,
     122,   123,    85,   208,   121,   122,   123,    76,    68,    69,
     249,   250,   551,    94,   525,   561,    95,   339,    99,   215,
     216,   208,   121,   122,   123,   536,   481,   217,   502,   106,
     209,   210,   211,   107,   128,   212,   210,   211,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     124,   109,   110,   293,   210,   211,   575,   540,   578,   522,
     541,   544,   482,   120,   326,   213,   483,   139,   142,   143,
     212,   303,   302,   484,   485,   212,   147,   553,   577,   214,
     339,   326,   149,   150,   152,   124,   155,   486,   163,   157,
     124,  -314,   487,   212,   125,   126,   127,   158,   121,    55,
     213,   303,   168,   123,   176,   294,   170,   178,   124,   183,
     184,   481,   185,   202,   214,   237,   203,   244,   241,   214,
     247,   251,   117,   294,   267,   215,   216,   273,   275,   125,
     126,   127,    15,   217,   125,   126,   127,   214,   284,   287,
     128,   307,   288,   289,   296,   327,   304,   482,   308,   324,
     330,   483,   125,   126,   127,   333,   336,   339,   484,   485,
     215,   216,   562,   305,   346,   215,   216,   351,   217,   360,
     306,   307,   486,   217,   365,   128,    76,   487,   308,   309,
     128,   361,  -268,   215,   216,  -317,  -317,   366,   382,  -317,
    -317,   217,   317,   318,   319,   320,   321,   322,   128,   323,
     385,   413,   428,   393,   434,   563,   186,   187,   188,   189,
     190,   191,   310,   311,   312,   313,   314,   433,   439,   315,
     316,  -316,   317,   318,   319,   320,   321,   322,     1,   323,
     303,   440,   444,     2,     1,   445,   457,   461,   454,     2,
       3,   460,   391,     4,   300,   472,     3,   475,     5,     4,
     323,     6,     7,   478,     5,   476,   494,     6,     7,   303,
     496,   503,     8,     9,   489,   498,   504,   505,     8,     9,
     506,   507,   512,   528,    10,   304,   520,    11,   524,   527,
      10,   526,   529,    11,   533,   530,   531,   534,   470,   542,
     543,   549,   383,   545,   552,   559,   560,    12,   567,    13,
     307,   557,   286,    12,   304,    13,   574,   308,   309,   579,
     592,   593,   108,   417,    14,    75,   331,   456,   303,    15,
      14,   383,   515,   509,   207,    15,   371,   429,   329,   307,
     586,   427,   555,   378,   342,   547,   308,   309,   554,   430,
     358,   310,   311,   312,   313,   314,   167,   201,   315,   316,
     535,   317,   318,   319,   320,   321,   322,   539,   323,    16,
      17,    18,     0,  -317,   499,    16,    17,    18,     0,     0,
     310,   311,   312,   313,   314,     0,     0,   315,   316,     0,
     317,   318,   319,   320,   321,   322,     0,   323,   307,     0,
       0,     0,     0,     0,     0,   308,  -317,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -317,
    -317,  -317,   313,   314,     0,     0,   315,   316,     0,   317,
     318,   319,   320,   321,   322,     0,   323
};

static const yytype_int16 yycheck[] =
{
     147,   172,    55,    58,    59,    60,   296,    82,   179,   205,
       2,   100,   101,     5,    99,   100,   101,   110,    10,     3,
      12,   366,   153,     3,    10,   213,     3,   215,     3,     3,
       3,   178,   118,   180,   181,    40,    47,   115,     3,     3,
     118,   183,    79,    81,     6,     6,    73,   489,    77,    79,
     141,   142,    14,     4,     5,     6,    79,    49,     3,    51,
     149,    44,   120,    22,   149,   212,   120,    97,   101,    61,
     217,   109,   101,   102,    97,    16,    12,   101,    23,    62,
       3,   371,   101,   116,   101,   327,   177,   116,   284,   531,
     435,   333,   116,    81,   241,    87,    88,   116,   151,   116,
      21,    37,    44,   393,    96,    50,   294,     3,    49,   454,
     185,   103,   104,   171,   302,   169,   304,   171,   249,    55,
     308,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   173,    57,   101,   175,    90,
      61,   174,   289,   290,   109,   109,   288,    68,    69,   179,
     177,    53,   116,   300,   301,   179,   179,   174,   172,   101,
     179,    82,   173,   177,   217,    86,    87,   145,   146,   147,
     106,    81,   177,   259,   116,     9,     3,     4,     5,     6,
     153,   259,   472,   134,   135,   136,   148,   148,   174,   151,
     151,   166,   166,   173,   178,     3,   173,    23,     9,   346,
      34,    81,   104,   391,   351,   352,   337,   117,    35,    36,
      37,   203,   557,    77,    85,   581,   118,     3,     4,     5,
       6,    50,   418,    34,    50,    51,   114,   593,   101,   180,
     323,    60,   379,   380,   287,    60,   109,    71,   102,   120,
      70,    70,   144,   116,   534,    70,    47,    48,    75,    35,
      36,    37,   327,   141,   142,    66,   177,     3,   333,    95,
      71,   258,   101,    90,    90,   101,   263,    96,   137,    88,
     560,    96,   361,   100,     0,   330,   361,   116,   105,   105,
     116,   273,   470,    84,    90,   166,   167,   168,   169,    75,
     171,    95,   119,   112,    97,    47,    98,   101,   101,   105,
     174,   176,    54,   177,    90,   149,   150,   134,   135,   136,
       3,    97,   116,   116,   461,   174,   463,   464,   177,   105,
     173,   175,   175,     3,     4,     5,     6,   174,   174,   382,
     177,   177,   324,   119,   174,   149,   150,   177,   165,   166,
     393,   174,   174,    81,   177,   177,   173,    60,   134,   135,
     136,   174,   173,   180,   177,    35,    36,    37,     3,     4,
       5,     6,   111,     3,     4,     5,     6,    47,    17,    18,
     161,   162,   519,   107,   174,   546,   111,   177,   117,   165,
     166,     3,     4,     5,     6,   174,    21,   173,   177,   109,
      35,    36,    37,   177,   180,    75,    36,    37,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
      90,    84,   108,    35,    36,    37,   563,   174,   565,   472,
     177,   174,    57,     3,   177,   105,    61,     4,   105,     3,
      75,    43,    14,    68,    69,    75,    73,   174,   174,   119,
     177,   177,    80,   173,     3,    90,    35,    82,     3,   174,
      90,    86,    87,    75,   134,   135,   136,   174,     4,   173,
     105,    43,   173,     6,   100,   105,    10,   110,    90,     4,
     174,    21,   177,    35,   119,     3,   115,    78,   154,   119,
      38,     3,    66,   105,   103,   165,   166,   115,     3,   134,
     135,   136,    98,   173,   134,   135,   136,   119,   109,   173,
     180,   113,   173,   173,   171,   173,    88,    57,   120,    85,
      29,    61,   134,   135,   136,   173,    81,   177,    68,    69,
     165,   166,    72,   105,   177,   165,   166,    44,   173,   173,
     112,   113,    82,   173,   104,   180,    47,    87,   120,   121,
     180,    80,   177,   165,   166,   157,   158,   173,   173,   161,
     162,   173,   164,   165,   166,   167,   168,   169,   180,   171,
     174,     3,     3,   173,   104,   115,   122,   123,   124,   125,
     126,   127,   154,   155,   156,   157,   158,   144,   173,   161,
     162,     0,   164,   165,   166,   167,   168,   169,     7,   171,
      43,     3,   173,    12,     7,   173,    67,    81,   173,    12,
      19,   174,    88,    22,   101,   173,    19,    90,    27,    22,
     171,    30,    31,     3,    27,   172,    59,    30,    31,    43,
      53,     6,    41,    42,   177,   143,     6,   104,    41,    42,
      90,   152,   173,    68,    53,    88,   174,    56,   174,    86,
      53,   174,    68,    56,     3,    68,    86,   110,   101,   174,
       3,   174,   105,    91,   174,     6,   110,    76,   174,    78,
     113,   173,   175,    76,    88,    78,   173,   120,   121,   174,
     174,   101,    71,   335,    93,    24,   247,   368,    43,    98,
      93,   105,   462,   452,   146,    98,   285,   346,   242,   113,
     574,   339,   527,   291,   257,   513,   120,   121,   526,   350,
     269,   154,   155,   156,   157,   158,   107,   141,   161,   162,
     497,   164,   165,   166,   167,   168,   169,   502,   171,   138,
     139,   140,    -1,    88,   439,   138,   139,   140,    -1,    -1,
     154,   155,   156,   157,   158,    -1,    -1,   161,   162,    -1,
     164,   165,   166,   167,   168,   169,    -1,   171,   113,    -1,
      -1,    -1,    -1,    -1,    -1,   120,   121,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,
     155,   156,   157,   158,    -1,    -1,   161,   162,    -1,   164,
     165,   166,   167,   168,   169,    -1,   171
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,     7,    12,    19,    22,    27,    30,    31,    41,    42,
      53,    56,    76,    78,    93,    98,   138,   139,   140,   182,
     183,   184,   185,   189,   191,   193,   194,   198,   199,   200,
     211,   213,   216,   217,   218,   220,   223,   300,   301,    22,
       3,   289,     3,     3,   289,    50,    60,    70,    96,    81,
      81,    85,   288,   289,    70,   173,   226,   289,    60,    70,
      96,    16,    49,     3,   302,   303,   137,   190,   190,   190,
       0,   176,   307,    98,   186,   186,    47,   226,   227,   231,
       3,   175,   173,    81,    60,   111,   201,   201,   201,   289,
       3,   178,   195,   289,   107,   111,   212,   226,   227,   117,
      81,   117,   212,   212,   212,   289,   109,   177,   184,    84,
     108,   243,     9,    34,    71,   228,   229,    66,   228,   239,
       3,     4,     5,     6,    90,   134,   135,   136,   180,   246,
     247,   275,   276,   277,   278,   279,   280,   281,   282,     4,
     192,   201,   105,     3,   290,   289,   289,    73,   236,    80,
     173,   219,     3,   221,   222,    35,   289,   174,   174,   196,
     276,   196,   196,     3,   289,   289,   226,   303,   173,   279,
      10,   232,   224,   225,   226,   231,   100,   230,   110,   224,
      44,    62,   244,     4,   174,   177,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   268,   269,
     270,   290,    35,   115,    81,   109,   173,   219,     3,    35,
      36,    37,    75,   105,   119,   165,   166,   173,   249,   250,
     251,   252,   253,   254,   255,   256,   258,   259,   260,   266,
     267,   271,   272,   273,   274,   275,   196,     3,   308,    54,
     227,   154,   177,   236,    78,   214,   215,    38,   197,   197,
     197,     3,   187,   188,   233,   245,   248,   249,   239,   228,
     240,   241,   249,   239,   249,   100,   249,   103,   295,   296,
     297,   268,   275,   115,   289,     3,   223,     3,    23,    50,
     202,   203,   204,   210,   109,   173,   175,   173,   173,   173,
      97,   249,   257,    35,   105,   250,   171,   250,   227,   249,
     101,   116,    14,    43,    88,   105,   112,   113,   120,   121,
     154,   155,   156,   157,   158,   161,   162,   164,   165,   166,
     167,   168,   169,   171,    85,   174,   177,   173,   249,   222,
      29,   195,   236,   173,   174,   177,    81,   234,   235,   177,
       3,   109,   293,   294,   244,   224,   177,    77,   102,   242,
     244,    44,    44,    53,   104,   118,   144,   298,   297,   289,
     173,    80,     3,   153,   205,   104,   173,   174,   177,   223,
     174,   232,     3,   166,   227,   268,   249,   249,   257,    79,
      97,   179,   173,   105,   245,   174,   174,   249,   249,   250,
     250,    88,   112,   173,    90,   105,   250,   250,   250,   250,
     250,   250,   250,   250,   250,   250,   250,   250,   250,   250,
     250,   279,   289,     3,   247,   212,   247,   188,   173,   283,
     284,   285,   286,   287,   289,   304,   236,   248,     3,   241,
     295,   249,   249,   144,   104,   114,   141,   142,   299,   173,
       3,   310,   311,   196,   173,   173,    23,    50,    51,    90,
     105,   206,   208,   209,   173,   308,   203,    67,   261,   245,
     174,    81,   109,    95,    79,   179,   249,   249,   227,   172,
     101,   250,   173,   227,   245,    90,   172,   174,     3,   174,
     223,    21,    57,    61,    68,    69,    82,    87,   305,   177,
     109,   291,   292,   293,    59,   237,    53,   308,   143,   311,
     309,   174,   177,     6,     6,   104,    90,   152,   207,   209,
     308,   174,   173,   174,   249,   205,   249,   249,   179,    95,
     174,   250,   227,   245,   174,   174,   174,    86,    68,    68,
      68,    86,   284,     3,   110,   299,   174,    77,   102,   310,
     174,   177,   174,     3,   174,    91,   262,   261,   174,   174,
     179,   249,   174,   174,   292,   285,   284,   173,   245,     6,
     110,   239,    72,   115,   308,    40,   238,   174,   245,   145,
     146,   147,   263,   264,   173,   249,   306,   174,   249,   174,
       6,    14,   148,   151,   265,     3,   274,   149,   150,   265,
     149,   150,   174,   101,   265
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int16 yyr1[] =
{
       0,   181,   182,   183,   183,   184,   184,   184,   184,   184,
     185,   185,   185,   185,   185,   185,   185,   185,   185,   185,
     186,   186,   187,   187,   188,   188,   189,   189,   189,   190,
     190,   191,   192,   193,   193,   194,   194,   195,   195,   196,
     197,   197,   198,   198,   199,   199,   199,   200,   200,   200,
     200,   200,   200,   201,   201,   202,   202,   203,   203,   204,
     205,   205,   205,   205,   206,   206,   207,   207,   208,   208,
     209,   209,   209,   209,   209,   210,   210,   211,   211,   211,
     211,   212,   212,   213,   214,   215,   216,   217,   218,   218,
     219,   219,   220,   221,   221,   222,   223,   223,   223,   224,
     224,   225,   225,   226,   226,   227,   227,   228,   229,   229,
     229,   230,   230,   231,   232,   232,   233,   234,   234,   235,
     236,   236,   237,   237,   238,   238,   239,   239,   240,   240,
     241,   242,   242,   242,   243,   243,   244,   244,   244,   244,
     244,   244,   245,   245,   246,   246,   247,   247,   248,   249,
     249,   249,   249,   249,   250,   250,   250,   250,   250,   250,
     250,   250,   250,   250,   250,   251,   251,   252,   252,   252,
     252,   252,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   253,   253,   254,   254,   255,   255,   255,   255,   256,
     256,   256,   256,   257,   257,   258,   258,   259,   259,   259,
     259,   259,   259,   259,   260,   260,   261,   261,   262,   262,
     263,   263,   263,   264,   264,   264,   265,   265,   265,   265,
     265,   266,   267,   268,   268,   268,   268,   268,   268,   269,
     269,   269,   269,   269,   269,   270,   270,   271,   272,   273,
     274,   274,   274,   274,   275,   275,   275,   275,   275,   275,
     276,   277,   277,   278,   278,   279,   280,   281,   281,   281,
     282,   283,   283,   284,   284,   285,   285,   286,   286,   287,
     288,   289,   289,   290,   290,   291,   291,   292,   292,   293,
     293,   294,   294,   295,   295,   296,   296,   297,   297,   298,
     298,   298,   298,   299,   299,   299,   300,   300,   301,   302,
     302,   303,   304,   304,   304,   305,   305,   305,   305,   305,
     305,   305,   305,   305,   305,   306,   307,   307,   308,   308,
     309,   309,   309,   310,   311,   311
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     3,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       5,     0,     1,     3,     1,     4,     2,     2,     2,     1,
       0,     4,     1,     2,     5,     7,     6,     1,     1,     1,
       2,     0,     5,     5,     2,     3,     2,     8,     7,     6,
       9,    10,     7,     3,     0,     1,     3,     1,     1,     4,
       4,     1,     4,     6,     1,     0,     2,     0,     1,     2,
       2,     1,     1,     2,     1,     5,     4,     4,     4,     3,
       4,     2,     0,     5,     1,     4,     4,     2,     8,     5,
       3,     0,     5,     1,     3,     3,     2,     2,     6,     1,
       1,     1,     3,     3,     3,     4,     6,     2,     1,     1,
       1,     1,     0,     7,     1,     0,     1,     1,     0,     2,
       2,     0,     4,     0,     2,     0,     3,     0,     1,     3,
       2,     1,     1,     0,     2,     0,     2,     2,     4,     2,
       4,     0,     1,     3,     1,     0,     1,     3,     2,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     2,     2,     2,
       3,     4,     1,     3,     3,     3,     3,     3,     3,     3,
       4,     3,     3,     3,     3,     5,     6,     5,     6,     4,
       6,     3,     5,     4,     5,     4,     5,     3,     3,     3,
       3,     3,     3,     3,     4,     6,     6,     0,     3,     0,
       2,     5,     0,     1,     1,     1,     2,     2,     2,     2,
       1,     6,     6,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     4,     5,
       1,     3,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     3,     2,
       1,     1,     3,     1,     1,     1,     4,     1,     3,     2,
       1,     1,     3,     1,     0,     1,     5,     1,     0,     2,
       1,     1,     0,     1,     0,     1,     2,     3,     5,     1,
       3,     1,     2,     2,     1,     0,     1,     0,     2,     1,
       3,     3,     4,     6,     8,     1,     2,     1,     2,     1,
       2,     1,     1,     1,     0,     1,     1,     0,     1,     3,
       2,     2,     0,     2,     1,     3
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
            { 
    delete (((*yyvaluep).statement)); 
}
        break;

    case YYSYMBOL_preparable_statement: /* preparable_statement  */
            { 
    delete (((*yyvaluep).statement)); 
}
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
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_transaction_statement: /* transaction_statement  */
            { 
    delete (((*yyvaluep).transaction_stmt)); 
}
        break;

    case YYSYMBOL_prepare_statement: /* prepare_statement  */
            { 
    delete (((*yyvaluep).prep_stmt)); 
}
        break;

    case YYSYMBOL_prepare_target_query: /* prepare_target_query  */
            { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_execute_statement: /* execute_statement  */
            { 
    delete (((*yyvaluep).exec_stmt)); 
}
        break;

    case YYSYMBOL_import_statement: /* import_statement  */
            { 
    delete (((*yyvaluep).import_stmt)); 
}
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
            { 
    delete (((*yyvaluep).export_stmt)); 
}
        break;

    case YYSYMBOL_show_statement: /* show_statement  */
            { 
    delete (((*yyvaluep).show_stmt)); 
}
        break;

    case YYSYMBOL_create_statement: /* create_statement  */
            { 
    delete (((*yyvaluep).create_stmt)); 
}
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
            { 
    delete (((*yyvaluep).table_element_t)); 
}
        break;

    case YYSYMBOL_column_def: /* column_def  */
            { 
    delete (((*yyvaluep).column_t)); 
}
        break;

    case YYSYMBOL_column_type_id_: /* column_type_id_  */
            { }
        break;

    case YYSYMBOL_opt_column_constraints: /* opt_column_constraints  */
            { }
        break;

    case YYSYMBOL_opt_character_set: /* opt_character_set  */
            { SAlloc::F((((*yyvaluep).sval))); }
        break;

    case YYSYMBOL_column_constraint_set: /* column_constraint_set  */
            { }
        break;

    case YYSYMBOL_column_constraint: /* column_constraint  */
            { }
        break;

    case YYSYMBOL_table_constraint: /* table_constraint  */
            { 
    delete (((*yyvaluep).table_constraint_t)); 
}
        break;

    case YYSYMBOL_drop_statement: /* drop_statement  */
            { 
    delete (((*yyvaluep).drop_stmt)); 
}
        break;

    case YYSYMBOL_opt_exists: /* opt_exists  */
            { }
        break;

    case YYSYMBOL_alter_statement: /* alter_statement  */
            { 
    delete (((*yyvaluep).alter_stmt)); 
}
        break;

    case YYSYMBOL_alter_action: /* alter_action  */
            { 
    delete (((*yyvaluep).alter_action_t)); 
}
        break;

    case YYSYMBOL_drop_action: /* drop_action  */
            { 
    delete (((*yyvaluep).drop_action_t)); 
}
        break;

    case YYSYMBOL_delete_statement: /* delete_statement  */
            { 
    delete (((*yyvaluep).delete_stmt)); 
}
        break;

    case YYSYMBOL_truncate_statement: /* truncate_statement  */
            { 
    delete (((*yyvaluep).delete_stmt)); 
}
        break;

    case YYSYMBOL_insert_statement: /* insert_statement  */
            { 
    delete (((*yyvaluep).insert_stmt)); 
}
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
            { 
    delete (((*yyvaluep).update_stmt)); 
}
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
            { 
    delete (((*yyvaluep).update_t)); 
}
        break;

    case YYSYMBOL_select_statement: /* select_statement  */
            { 
    delete (((*yyvaluep).select_stmt)); 
}
        break;

    case YYSYMBOL_select_within_set_operation: /* select_within_set_operation  */
            { 
    delete (((*yyvaluep).select_stmt)); 
}
        break;

    case YYSYMBOL_select_within_set_operation_no_parentheses: /* select_within_set_operation_no_parentheses  */
            { 
    delete (((*yyvaluep).select_stmt)); 
}
        break;

    case YYSYMBOL_select_with_paren: /* select_with_paren  */
            { 
    delete (((*yyvaluep).select_stmt)); 
}
        break;

    case YYSYMBOL_select_no_paren: /* select_no_paren  */
            { 
    delete (((*yyvaluep).select_stmt)); 
}
        break;

    case YYSYMBOL_set_operator: /* set_operator  */
            { 
    delete (((*yyvaluep).set_operator_t)); 
}
        break;

    case YYSYMBOL_set_type: /* set_type  */
            { 
    delete (((*yyvaluep).set_operator_t)); 
}
        break;

    case YYSYMBOL_opt_all: /* opt_all  */
            { }
        break;

    case YYSYMBOL_select_clause: /* select_clause  */
            { 
    delete (((*yyvaluep).select_stmt)); 
}
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
            { 
    delete (((*yyvaluep).table)); 
}
        break;

    case YYSYMBOL_from_clause: /* from_clause  */
            { 
    delete (((*yyvaluep).table)); 
}
        break;

    case YYSYMBOL_opt_where: /* opt_where  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_opt_group: /* opt_group  */
            { 
    delete (((*yyvaluep).group_t)); 
}
        break;

    case YYSYMBOL_opt_having: /* opt_having  */
            { 
    delete (((*yyvaluep).expr)); 
}
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
            { 
    delete (((*yyvaluep).order)); 
}
        break;

    case YYSYMBOL_opt_order_type: /* opt_order_type  */
            { }
        break;

    case YYSYMBOL_opt_top: /* opt_top  */
            { 
    delete (((*yyvaluep).limit)); 
}
        break;

    case YYSYMBOL_opt_limit: /* opt_limit  */
            { 
    delete (((*yyvaluep).limit)); 
}
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
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_expr: /* expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_operand: /* operand  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_scalar_expr: /* scalar_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_unary_expr: /* unary_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_binary_expr: /* binary_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_logic_expr: /* logic_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_in_expr: /* in_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_case_expr: /* case_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_case_list: /* case_list  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_exists_expr: /* exists_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_comp_expr: /* comp_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_function_expr: /* function_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_opt_window: /* opt_window  */
            { 
    delete (((*yyvaluep).window_description)); 
}
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
            { 
    delete (((*yyvaluep).frame_description)); 
}
        break;

    case YYSYMBOL_frame_type: /* frame_type  */
            { }
        break;

    case YYSYMBOL_frame_bound: /* frame_bound  */
            { 
    delete (((*yyvaluep).frame_bound)); 
}
        break;

    case YYSYMBOL_extract_expr: /* extract_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_cast_expr: /* cast_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
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
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_array_index: /* array_index  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_between_expr: /* between_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_column_name: /* column_name  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_literal: /* literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_string_literal: /* string_literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_bool_literal: /* bool_literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_num_literal: /* num_literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_int_literal: /* int_literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_null_literal: /* null_literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_interval_literal: /* interval_literal  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_param_expr: /* param_expr  */
            { 
    delete (((*yyvaluep).expr)); 
}
        break;

    case YYSYMBOL_table_ref: /* table_ref  */
            { 
    delete (((*yyvaluep).table)); 
}
        break;

    case YYSYMBOL_table_ref_atomic: /* table_ref_atomic  */
            { 
    delete (((*yyvaluep).table)); 
}
        break;

    case YYSYMBOL_nonjoin_table_ref_atomic: /* nonjoin_table_ref_atomic  */
            { 
    delete (((*yyvaluep).table)); 
}
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
            { 
    delete (((*yyvaluep).table)); 
}
        break;

    case YYSYMBOL_table_ref_name_no_alias: /* table_ref_name_no_alias  */
            { 
    delete (((*yyvaluep).table)); 
}
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
            { 
    delete (((*yyvaluep).alias_t)); 
}
        break;

    case YYSYMBOL_opt_table_alias: /* opt_table_alias  */
            { 
    delete (((*yyvaluep).alias_t)); 
}
        break;

    case YYSYMBOL_alias: /* alias  */
            { 
    delete (((*yyvaluep).alias_t)); 
}
        break;

    case YYSYMBOL_opt_alias: /* opt_alias  */
            { 
    delete (((*yyvaluep).alias_t)); 
}
        break;

    case YYSYMBOL_opt_locking_clause: /* opt_locking_clause  */
            { 
    delete (((*yyvaluep).locking_clause_vec)); 
}
        break;

    case YYSYMBOL_opt_locking_clause_list: /* opt_locking_clause_list  */
            { 
    delete (((*yyvaluep).locking_clause_vec)); 
}
        break;

    case YYSYMBOL_locking_clause: /* locking_clause  */
            { 
    delete (((*yyvaluep).locking_t)); 
}
        break;

    case YYSYMBOL_row_lock_mode: /* row_lock_mode  */
            { }
        break;

    case YYSYMBOL_opt_row_lock_policy: /* opt_row_lock_policy  */
            { }
        break;

    case YYSYMBOL_opt_with_clause: /* opt_with_clause  */
            { 
    delete (((*yyvaluep).with_description_vec)); 
}
        break;

    case YYSYMBOL_with_clause: /* with_clause  */
            { 
    delete (((*yyvaluep).with_description_vec)); 
}
        break;

    case YYSYMBOL_with_description_list: /* with_description_list  */
            { 
    delete (((*yyvaluep).with_description_vec)); 
}
        break;

    case YYSYMBOL_with_description: /* with_description  */
            { 
    delete (((*yyvaluep).with_description_t)); 
}
        break;

    case YYSYMBOL_join_clause: /* join_clause  */
            { 
    delete (((*yyvaluep).table)); 
}
        break;

    case YYSYMBOL_opt_join_type: /* opt_join_type  */
            { }
        break;

    case YYSYMBOL_join_condition: /* join_condition  */
            { 
    delete (((*yyvaluep).expr)); 
}
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

    case YYSYMBOL_index_segment_flags: /* index_segment_flags  */
            { }
        break;

    case YYSYMBOL_index_segment: /* index_segment  */
            {
    delete (((*yyvaluep).P_IdxSeg));
}
        break;

    case YYSYMBOL_index_segment_list: /* index_segment_list  */
            {
    delete (((*yyvaluep).P_IdxVal));
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
  for(SQLStatement* stmt : *(yyvsp[-1].stmt_vec)) {
    // Transfers ownership of the statement.
    result->addStatement(stmt);
  }
  unsigned param_id = 0;
  for(void* param : yyloc.param_list) {
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

	case 38: /* file_type: "BINARY"  */
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

	case 40: /* opt_file_type: WITH_FORMAT file_type  */
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
	                             { 
    (yyval.show_stmt) = new ShowStatement(kShowTables); 
}
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

	case 50: /* create_statement: CREATE INDEX opt_not_exists opt_index_name ON table_name '(' index_segment_list ')'  */
	                                                                                        { // @sobolev ident_commalist-->index_segment_list
    (yyval.create_stmt) = new CreateStatement(kCreateIndex);
    (yyval.create_stmt)->indexName = (yyvsp[-5].sval);
    (yyval.create_stmt)->ifNotExists = (yyvsp[-6].bval);
    (yyval.create_stmt)->fUnique = false;
    (yyval.create_stmt)->tableName = (yyvsp[-3].table_name).name;
    // @sobolev $$->indexColumns = $8;
    (yyval.create_stmt)->P_Idx = (yyvsp[-1].P_IdxVal);
    if((yyval.create_stmt)->P_Idx)
        (yyval.create_stmt)->P_Idx->Name = (yyval.create_stmt)->indexName;
}
		break;

	case 51: /* create_statement: CREATE UNIQUE INDEX opt_not_exists opt_index_name ON table_name '(' index_segment_list ')'  */
	                                                                                               { // @sobolev
    (yyval.create_stmt) = new CreateStatement(kCreateIndex);
    (yyval.create_stmt)->indexName = (yyvsp[-5].sval);
    (yyval.create_stmt)->ifNotExists = (yyvsp[-6].bval);
    (yyval.create_stmt)->fUnique = true;
    (yyval.create_stmt)->tableName = (yyvsp[-3].table_name).name;
    // @sobolev $$->indexColumns = $9;
    (yyval.create_stmt)->P_Idx = (yyvsp[-1].P_IdxVal);
    if((yyval.create_stmt)->P_Idx)
        (yyval.create_stmt)->P_Idx->Name = (yyval.create_stmt)->indexName;
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

	case 59: /* column_def: IDENTIFIER column_type_id_ opt_column_constraints opt_character_set  */
	                                                                                 { // @sobolev
  (yyval.column_t) = new ColumnDefinition((yyvsp[-3].sval), (yyvsp[-2].column_type_t), (yyvsp[-1].column_constraint_set));
  (yyval.column_t)->P_CharSet = (yyvsp[0].sval); // character_set
  if(!(yyval.column_t)->trySetNullableExplicit()) {
    yyerror(&yyloc, result, scanner, ("Conflicting nullability constraints for " + std::string{(yyvsp[-3].sval)}).c_str());
  }
}
		break;

	case 60: /* column_type_id_: CHARACTER_VARYING '(' INTVAL ')'  */
	{
    SString msg_buf;
    if(!ColumnType::Make("CHARACTER_VARYING", (yyvsp[-1].ival), -1, (yyval.column_type_t), msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }
}
		break;

	case 61: /* column_type_id_: IDENTIFIER  */
	{
    SString msg_buf;
    if(!ColumnType::Make((yyvsp[0].sval), -1, -1, (yyval.column_type_t), msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }
}
		break;

	case 62: /* column_type_id_: IDENTIFIER '(' INTVAL ')'  */
	{
    SString msg_buf;
    if(!ColumnType::Make((yyvsp[-3].sval), (yyvsp[-1].ival), -1, (yyval.column_type_t), msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }
}
		break;

	case 63: /* column_type_id_: IDENTIFIER '(' INTVAL ',' INTVAL ')'  */
	{
    SString msg_buf;
    if(!ColumnType::Make((yyvsp[-5].sval), (yyvsp[-3].ival), (yyvsp[-1].ival), (yyval.column_type_t), msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }    
}
		break;

	case 64: /* opt_column_constraints: column_constraint_set  */
	                                               { (yyval.column_constraint_set) = (yyvsp[0].column_constraint_set); }
		break;

	case 65: /* opt_column_constraints: %empty  */
	                                                                        { (yyval.column_constraint_set) = new std::unordered_set<ConstraintType>(); }
		break;

	case 66: /* opt_character_set: CHARACTER_SET IDENTIFIER  */
	                                             { (yyval.sval) = (yyvsp[0].sval); }
		break;

	case 67: /* opt_character_set: %empty  */
	                                                                      { (yyval.sval) = 0; }
		break;

	case 68: /* column_constraint_set: column_constraint  */
	                                          {
  (yyval.column_constraint_set) = new std::unordered_set<ConstraintType>();
  (yyval.column_constraint_set)->insert((yyvsp[0].column_constraint_t));
}
		break;

	case 69: /* column_constraint_set: column_constraint_set column_constraint  */
	                                            {
  (yyvsp[-1].column_constraint_set)->insert((yyvsp[0].column_constraint_t));
  (yyval.column_constraint_set) = (yyvsp[-1].column_constraint_set);
}
		break;

	case 70: /* column_constraint: PRIMARY KEY  */
	                                { (yyval.column_constraint_t) = ConstraintType::PrimaryKey; }
		break;

	case 71: /* column_constraint: UNIQUE  */
	         { (yyval.column_constraint_t) = ConstraintType::Unique; }
		break;

	case 72: /* column_constraint: NULL  */
	       { (yyval.column_constraint_t) = ConstraintType::Null; }
		break;

	case 73: /* column_constraint: NOT NULL  */
	           { (yyval.column_constraint_t) = ConstraintType::NotNull; }
		break;

	case 74: /* column_constraint: AUTO_INCREMENT  */
	                 { (yyval.column_constraint_t) = ConstraintType::AutoIncrement; }
		break;

	case 75: /* table_constraint: PRIMARY KEY '(' ident_commalist ')'  */
	                                                       { (yyval.table_constraint_t) = new TableConstraint(ConstraintType::PrimaryKey, (yyvsp[-1].str_vec)); }
		break;

	case 76: /* table_constraint: UNIQUE '(' ident_commalist ')'  */
	                                 { (yyval.table_constraint_t) = new TableConstraint(ConstraintType::Unique, (yyvsp[-1].str_vec)); }
		break;

	case 77: /* drop_statement: DROP TABLE opt_exists table_name  */
	                                                  {
  (yyval.drop_stmt) = new DropStatement(kDropTable);
  (yyval.drop_stmt)->ifExists = (yyvsp[-1].bval);
  (yyval.drop_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.drop_stmt)->name = (yyvsp[0].table_name).name;
}
		break;

	case 78: /* drop_statement: DROP VIEW opt_exists table_name  */
	                                    {
  (yyval.drop_stmt) = new DropStatement(kDropView);
  (yyval.drop_stmt)->ifExists = (yyvsp[-1].bval);
  (yyval.drop_stmt)->schema = (yyvsp[0].table_name).schema;
  (yyval.drop_stmt)->name = (yyvsp[0].table_name).name;
}
		break;

	case 79: /* drop_statement: DEALLOCATE PREPARE IDENTIFIER  */
	                                  {
  (yyval.drop_stmt) = new DropStatement(kDropPreparedStatement);
  (yyval.drop_stmt)->ifExists = false;
  (yyval.drop_stmt)->name = (yyvsp[0].sval);
}
		break;

	case 80: /* drop_statement: DROP INDEX opt_exists IDENTIFIER  */
	                                     {
  (yyval.drop_stmt) = new DropStatement(kDropIndex);
  (yyval.drop_stmt)->ifExists = (yyvsp[-1].bval);
  (yyval.drop_stmt)->indexName = (yyvsp[0].sval);
}
		break;

	case 81: /* opt_exists: IF EXISTS  */
	                       { (yyval.bval) = true; }
		break;

	case 82: /* opt_exists: %empty  */
	                                                    { (yyval.bval) = false; }
		break;

	case 83: /* alter_statement: ALTER TABLE opt_exists table_name alter_action  */
	                                                                 {
  (yyval.alter_stmt) = new AlterStatement((yyvsp[-1].table_name).name, (yyvsp[0].alter_action_t));
  (yyval.alter_stmt)->ifTableExists = (yyvsp[-2].bval);
  (yyval.alter_stmt)->schema = (yyvsp[-1].table_name).schema;
}
		break;

	case 84: /* alter_action: drop_action  */
	                           { (yyval.alter_action_t) = (yyvsp[0].drop_action_t); }
		break;

	case 85: /* drop_action: DROP COLUMN opt_exists IDENTIFIER  */
	                                                {
  (yyval.drop_action_t) = new DropColumnAction((yyvsp[0].sval));
  (yyval.drop_action_t)->ifExists = (yyvsp[-1].bval);
}
		break;

	case 86: /* delete_statement: DELETE FROM table_name opt_where  */
	                                                    {
    (yyval.delete_stmt) = new DeleteStatement();
    (yyval.delete_stmt)->schema = (yyvsp[-1].table_name).schema;
    (yyval.delete_stmt)->tableName = (yyvsp[-1].table_name).name;
    (yyval.delete_stmt)->expr = (yyvsp[0].expr);
}
		break;

	case 87: /* truncate_statement: TRUNCATE table_name  */
	                                         {
    (yyval.delete_stmt) = new DeleteStatement();
    (yyval.delete_stmt)->schema = (yyvsp[0].table_name).schema;
    (yyval.delete_stmt)->tableName = (yyvsp[0].table_name).name;
}
		break;

	case 88: /* insert_statement: INSERT INTO table_name opt_column_list VALUES '(' literal_list ')'  */
	                                                                                      {
    (yyval.insert_stmt) = new InsertStatement(kInsertValues);
    (yyval.insert_stmt)->schema = (yyvsp[-5].table_name).schema;
    (yyval.insert_stmt)->tableName = (yyvsp[-5].table_name).name;
    (yyval.insert_stmt)->columns = (yyvsp[-4].str_vec);
    (yyval.insert_stmt)->values = (yyvsp[-1].expr_vec);
}
		break;

	case 89: /* insert_statement: INSERT INTO table_name opt_column_list select_no_paren  */
	                                                           {
    (yyval.insert_stmt) = new InsertStatement(kInsertSelect);
    (yyval.insert_stmt)->schema = (yyvsp[-2].table_name).schema;
    (yyval.insert_stmt)->tableName = (yyvsp[-2].table_name).name;
    (yyval.insert_stmt)->columns = (yyvsp[-1].str_vec);
    (yyval.insert_stmt)->select = (yyvsp[0].select_stmt);
}
		break;

	case 90: /* opt_column_list: '(' ident_commalist ')'  */
	                                          { (yyval.str_vec) = (yyvsp[-1].str_vec); }
		break;

	case 91: /* opt_column_list: %empty  */
	                                                                     { (yyval.str_vec) = nullptr; }
		break;

	case 92: /* update_statement: UPDATE table_ref_name_no_alias SET update_clause_commalist opt_where  */
	                                                                                        {
  (yyval.update_stmt) = new UpdateStatement();
  (yyval.update_stmt)->table = (yyvsp[-3].table);
  (yyval.update_stmt)->updates = (yyvsp[-1].update_vec);
  (yyval.update_stmt)->where = (yyvsp[0].expr);
}
		break;

	case 93: /* update_clause_commalist: update_clause  */
	                                        {
  (yyval.update_vec) = new std::vector<UpdateClause*>();
  (yyval.update_vec)->push_back((yyvsp[0].update_t));
}
		break;

	case 94: /* update_clause_commalist: update_clause_commalist ',' update_clause  */
	                                              {
  (yyvsp[-2].update_vec)->push_back((yyvsp[0].update_t));
  (yyval.update_vec) = (yyvsp[-2].update_vec);
}
		break;

	case 95: /* update_clause: IDENTIFIER '=' expr  */
	                                    {
  (yyval.update_t) = new UpdateClause();
  (yyval.update_t)->column = (yyvsp[-2].sval);
  (yyval.update_t)->value = (yyvsp[0].expr);
}
		break;

	case 96: /* select_statement: opt_with_clause select_with_paren  */
	                                                     {
  (yyval.select_stmt) = (yyvsp[0].select_stmt);
  (yyval.select_stmt)->withDescriptions = (yyvsp[-1].with_description_vec);
}
		break;

	case 97: /* select_statement: opt_with_clause select_no_paren  */
	                                    {
  (yyval.select_stmt) = (yyvsp[0].select_stmt);
  (yyval.select_stmt)->withDescriptions = (yyvsp[-1].with_description_vec);
}
		break;

	case 98: /* select_statement: opt_with_clause select_with_paren set_operator select_within_set_operation opt_order opt_limit  */
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

	case 101: /* select_within_set_operation_no_parentheses: select_clause  */
	                                                           { (yyval.select_stmt) = (yyvsp[0].select_stmt); }
		break;

	case 102: /* select_within_set_operation_no_parentheses: select_clause set_operator select_within_set_operation  */
	                                                         {
  (yyval.select_stmt) = (yyvsp[-2].select_stmt);
  if((yyval.select_stmt)->setOperations == nullptr) {
    (yyval.select_stmt)->setOperations = new std::vector<SetOperation*>();
  }
  (yyval.select_stmt)->setOperations->push_back((yyvsp[-1].set_operator_t));
  (yyval.select_stmt)->setOperations->back()->nestedSelectStatement = (yyvsp[0].select_stmt);
}
		break;

	case 103: /* select_with_paren: '(' select_no_paren ')'  */
	                                            { (yyval.select_stmt) = (yyvsp[-1].select_stmt); }
		break;

	case 104: /* select_with_paren: '(' select_with_paren ')'  */
	                                                                                     { (yyval.select_stmt) = (yyvsp[-1].select_stmt); }
		break;

	case 105: /* select_no_paren: select_clause opt_order opt_limit opt_locking_clause  */
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

	case 106: /* select_no_paren: select_clause set_operator select_within_set_operation opt_order opt_limit opt_locking_clause  */
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

	case 107: /* set_operator: set_type opt_all  */
	                                {
  (yyval.set_operator_t) = (yyvsp[-1].set_operator_t);
  (yyval.set_operator_t)->isAll = (yyvsp[0].bval);
}
		break;

	case 108: /* set_type: UNION  */
	                 {
  (yyval.set_operator_t) = new SetOperation();
  (yyval.set_operator_t)->setType = SetType::kSetUnion;
}
		break;

	case 109: /* set_type: INTERSECT  */
	              {
  (yyval.set_operator_t) = new SetOperation();
  (yyval.set_operator_t)->setType = SetType::kSetIntersect;
}
		break;

	case 110: /* set_type: EXCEPT  */
	           {
  (yyval.set_operator_t) = new SetOperation();
  (yyval.set_operator_t)->setType = SetType::kSetExcept;
}
		break;

	case 111: /* opt_all: ALL  */
	              { (yyval.bval) = true; }
		break;

	case 112: /* opt_all: %empty  */
	                                           { (yyval.bval) = false; }
		break;

	case 113: /* select_clause: SELECT opt_top opt_distinct select_list opt_from_clause opt_where opt_group  */
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

	case 114: /* opt_distinct: DISTINCT  */
	                        { (yyval.bval) = true; }
		break;

	case 115: /* opt_distinct: %empty  */
	                                                     { (yyval.bval) = false; }
		break;

	case 117: /* opt_from_clause: from_clause  */
	                              { (yyval.table) = (yyvsp[0].table); }
		break;

	case 118: /* opt_from_clause: %empty  */
	                                                         { (yyval.table) = nullptr; }
		break;

	case 119: /* from_clause: FROM table_ref  */
	                             { (yyval.table) = (yyvsp[0].table); }
		break;

	case 120: /* opt_where: WHERE expr  */
	                       { (yyval.expr) = (yyvsp[0].expr); }
		break;

	case 121: /* opt_where: %empty  */
	                                                  { (yyval.expr) = nullptr; }
		break;

	case 122: /* opt_group: GROUP BY expr_list opt_having  */
	                                          {
  (yyval.group_t) = new GroupByDescription();
  (yyval.group_t)->columns = (yyvsp[-1].expr_vec);
  (yyval.group_t)->having = (yyvsp[0].expr);
}
		break;

	case 123: /* opt_group: %empty  */
	                { (yyval.group_t) = nullptr; }
		break;

	case 124: /* opt_having: HAVING expr  */
	                         { (yyval.expr) = (yyvsp[0].expr); }
		break;

	case 125: /* opt_having: %empty  */
	                                                    { (yyval.expr) = nullptr; }
		break;

	case 126: /* opt_order: ORDER BY order_list  */
	                                { (yyval.order_vec) = (yyvsp[0].order_vec); }
		break;

	case 127: /* opt_order: %empty  */
	                                                           { (yyval.order_vec) = nullptr; }
		break;

	case 128: /* order_list: order_desc  */
	                        {
  (yyval.order_vec) = new std::vector<OrderDescription*>();
  (yyval.order_vec)->push_back((yyvsp[0].order));
}
		break;

	case 129: /* order_list: order_list ',' order_desc  */
	                              {
  (yyvsp[-2].order_vec)->push_back((yyvsp[0].order));
  (yyval.order_vec) = (yyvsp[-2].order_vec);
}
		break;

	case 130: /* order_desc: expr opt_order_type  */
	                                 { (yyval.order) = new OrderDescription((yyvsp[0].order_type), (yyvsp[-1].expr)); }
		break;

	case 131: /* opt_order_type: ASC  */
	                     { (yyval.order_type) = kOrderAsc; }
		break;

	case 132: /* opt_order_type: DESC  */
	                                                { (yyval.order_type) = kOrderDesc; }
		break;

	case 133: /* opt_order_type: %empty  */
	                                                                                   { (yyval.order_type) = kOrderAsc; }
		break;

	case 134: /* opt_top: TOP int_literal  */
	                          { (yyval.limit) = new LimitDescription((yyvsp[0].expr), nullptr); }
		break;

	case 135: /* opt_top: %empty  */
	                                                                                    { (yyval.limit) = nullptr; }
		break;

	case 136: /* opt_limit: LIMIT expr  */
	                       { (yyval.limit) = new LimitDescription((yyvsp[0].expr), nullptr); }
		break;

	case 137: /* opt_limit: OFFSET expr  */
	              { (yyval.limit) = new LimitDescription(nullptr, (yyvsp[0].expr)); }
		break;

	case 138: /* opt_limit: LIMIT expr OFFSET expr  */
	                         { (yyval.limit) = new LimitDescription((yyvsp[-2].expr), (yyvsp[0].expr)); }
		break;

	case 139: /* opt_limit: LIMIT ALL  */
	            { (yyval.limit) = new LimitDescription(nullptr, nullptr); }
		break;

	case 140: /* opt_limit: LIMIT ALL OFFSET expr  */
	                        { (yyval.limit) = new LimitDescription(nullptr, (yyvsp[0].expr)); }
		break;

	case 141: /* opt_limit: %empty  */
	              { (yyval.limit) = nullptr; }
		break;

	case 142: /* expr_list: expr_alias  */
	                       {
    (yyval.expr_vec) = new std::vector<Expr*>();
    (yyval.expr_vec)->push_back((yyvsp[0].expr));
}
		break;

	case 143: /* expr_list: expr_list ',' expr_alias  */
	                             {
    (yyvsp[-2].expr_vec)->push_back((yyvsp[0].expr));
    (yyval.expr_vec) = (yyvsp[-2].expr_vec);
}
		break;

	case 144: /* opt_literal_list: literal_list  */
	                                { (yyval.expr_vec) = (yyvsp[0].expr_vec); }
		break;

	case 145: /* opt_literal_list: %empty  */
	                                                           { (yyval.expr_vec) = nullptr; }
		break;

	case 146: /* literal_list: literal  */
	                       {
    (yyval.expr_vec) = new std::vector<Expr*>();
    (yyval.expr_vec)->push_back((yyvsp[0].expr));
}
		break;

	case 147: /* literal_list: literal_list ',' literal  */
	                             {
    (yyvsp[-2].expr_vec)->push_back((yyvsp[0].expr));
    (yyval.expr_vec) = (yyvsp[-2].expr_vec);
}
		break;

	case 148: /* expr_alias: expr opt_alias  */
	                            {
    (yyval.expr) = (yyvsp[-1].expr);
    if((yyvsp[0].alias_t)) {
        (yyval.expr)->alias = _strdup((yyvsp[0].alias_t)->name);
        delete (yyvsp[0].alias_t);
    }
}
		break;

	case 154: /* operand: '(' expr ')'  */
	                       { (yyval.expr) = (yyvsp[-1].expr); }
		break;

	case 164: /* operand: '(' select_no_paren ')'  */
	                                         {
  (yyval.expr) = Expr::makeSelect((yyvsp[-1].select_stmt));
}
		break;

	case 167: /* unary_expr: '-' operand  */
	                         { (yyval.expr) = Expr::makeOpUnary(kOpUnaryMinus, (yyvsp[0].expr)); }
		break;

	case 168: /* unary_expr: NOT operand  */
	              { (yyval.expr) = Expr::makeOpUnary(kOpNot, (yyvsp[0].expr)); }
		break;

	case 169: /* unary_expr: operand ISNULL  */
	                 { (yyval.expr) = Expr::makeOpUnary(kOpIsNull, (yyvsp[-1].expr)); }
		break;

	case 170: /* unary_expr: operand IS NULL  */
	                  { (yyval.expr) = Expr::makeOpUnary(kOpIsNull, (yyvsp[-2].expr)); }
		break;

	case 171: /* unary_expr: operand IS NOT NULL  */
	                      { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeOpUnary(kOpIsNull, (yyvsp[-3].expr))); }
		break;

	case 173: /* binary_expr: operand '-' operand  */
	                                              { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpMinus, (yyvsp[0].expr)); }
		break;

	case 174: /* binary_expr: operand '+' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpPlus, (yyvsp[0].expr)); }
		break;

	case 175: /* binary_expr: operand '/' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpSlash, (yyvsp[0].expr)); }
		break;

	case 176: /* binary_expr: operand '*' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpAsterisk, (yyvsp[0].expr)); }
		break;

	case 177: /* binary_expr: operand '%' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpPercentage, (yyvsp[0].expr)); }
		break;

	case 178: /* binary_expr: operand '^' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpCaret, (yyvsp[0].expr)); }
		break;

	case 179: /* binary_expr: operand LIKE operand  */
	                       { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpLike, (yyvsp[0].expr)); }
		break;

	case 180: /* binary_expr: operand NOT LIKE operand  */
	                           { (yyval.expr) = Expr::makeOpBinary((yyvsp[-3].expr), kOpNotLike, (yyvsp[0].expr)); }
		break;

	case 181: /* binary_expr: operand ILIKE operand  */
	                        { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpILike, (yyvsp[0].expr)); }
		break;

	case 182: /* binary_expr: operand CONCAT operand  */
	                         { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpConcat, (yyvsp[0].expr)); }
		break;

	case 183: /* logic_expr: expr AND expr  */
	                           { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpAnd, (yyvsp[0].expr)); }
		break;

	case 184: /* logic_expr: expr OR expr  */
	               { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpOr, (yyvsp[0].expr)); }
		break;

	case 185: /* in_expr: operand IN '(' expr_list ')'  */
	                                       { (yyval.expr) = Expr::makeInOperator((yyvsp[-4].expr), (yyvsp[-1].expr_vec)); }
		break;

	case 186: /* in_expr: operand NOT IN '(' expr_list ')'  */
	                                   { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeInOperator((yyvsp[-5].expr), (yyvsp[-1].expr_vec))); }
		break;

	case 187: /* in_expr: operand IN '(' select_no_paren ')'  */
	                                     { (yyval.expr) = Expr::makeInOperator((yyvsp[-4].expr), (yyvsp[-1].select_stmt)); }
		break;

	case 188: /* in_expr: operand NOT IN '(' select_no_paren ')'  */
	                                         { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeInOperator((yyvsp[-5].expr), (yyvsp[-1].select_stmt))); }
		break;

	case 189: /* case_expr: CASE expr case_list "END"  */
	                                      { (yyval.expr) = Expr::makeCase((yyvsp[-2].expr), (yyvsp[-1].expr), nullptr); }
		break;

	case 190: /* case_expr: CASE expr case_list ELSE expr "END"  */
	                                      { (yyval.expr) = Expr::makeCase((yyvsp[-4].expr), (yyvsp[-3].expr), (yyvsp[-1].expr)); }
		break;

	case 191: /* case_expr: CASE case_list "END"  */
	                       { (yyval.expr) = Expr::makeCase(nullptr, (yyvsp[-1].expr), nullptr); }
		break;

	case 192: /* case_expr: CASE case_list ELSE expr "END"  */
	                                 { (yyval.expr) = Expr::makeCase(nullptr, (yyvsp[-3].expr), (yyvsp[-1].expr)); }
		break;

	case 193: /* case_list: WHEN expr THEN expr  */
	                                { (yyval.expr) = Expr::makeCaseList(Expr::makeCaseListElement((yyvsp[-2].expr), (yyvsp[0].expr))); }
		break;

	case 194: /* case_list: case_list WHEN expr THEN expr  */
	                                { (yyval.expr) = Expr::caseListAppend((yyvsp[-4].expr), Expr::makeCaseListElement((yyvsp[-2].expr), (yyvsp[0].expr))); }
		break;

	case 195: /* exists_expr: EXISTS '(' select_no_paren ')'  */
	                                             { (yyval.expr) = Expr::makeExists((yyvsp[-1].select_stmt)); }
		break;

	case 196: /* exists_expr: NOT EXISTS '(' select_no_paren ')'  */
	                                     { (yyval.expr) = Expr::makeOpUnary(kOpNot, Expr::makeExists((yyvsp[-1].select_stmt))); }
		break;

	case 197: /* comp_expr: operand '=' operand  */
	                                { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpEquals, (yyvsp[0].expr)); }
		break;

	case 198: /* comp_expr: operand EQUALS operand  */
	                         { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpEquals, (yyvsp[0].expr)); }
		break;

	case 199: /* comp_expr: operand NOTEQUALS operand  */
	                            { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpNotEquals, (yyvsp[0].expr)); }
		break;

	case 200: /* comp_expr: operand '<' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpLess, (yyvsp[0].expr)); }
		break;

	case 201: /* comp_expr: operand '>' operand  */
	                      { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpGreater, (yyvsp[0].expr)); }
		break;

	case 202: /* comp_expr: operand LESSEQ operand  */
	                         { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpLessEq, (yyvsp[0].expr)); }
		break;

	case 203: /* comp_expr: operand GREATEREQ operand  */
	                            { (yyval.expr) = Expr::makeOpBinary((yyvsp[-2].expr), kOpGreaterEq, (yyvsp[0].expr)); }
		break;

	case 204: /* function_expr: IDENTIFIER '(' ')' opt_window  */
	                                              { (yyval.expr) = Expr::makeFunctionRef((yyvsp[-3].sval), new std::vector<Expr*>(), false, (yyvsp[0].window_description)); }
		break;

	case 205: /* function_expr: IDENTIFIER '(' opt_distinct expr_list ')' opt_window  */
	                                                       { (yyval.expr) = Expr::makeFunctionRef((yyvsp[-5].sval), (yyvsp[-2].expr_vec), (yyvsp[-3].bval), (yyvsp[0].window_description)); }
		break;

	case 206: /* opt_window: OVER '(' opt_partition opt_order opt_frame_clause ')'  */
	                                                                   { (yyval.window_description) = new WindowDescription((yyvsp[-3].expr_vec), (yyvsp[-2].order_vec), (yyvsp[-1].frame_description)); }
		break;

	case 207: /* opt_window: %empty  */
	              { (yyval.window_description) = nullptr; }
		break;

	case 208: /* opt_partition: PARTITION BY expr_list  */
	                                       { (yyval.expr_vec) = (yyvsp[0].expr_vec); }
		break;

	case 209: /* opt_partition: %empty  */
	              { (yyval.expr_vec) = nullptr; }
		break;

	case 210: /* opt_frame_clause: frame_type frame_bound  */
	                                          { (yyval.frame_description) = new FrameDescription{(yyvsp[-1].frame_type), (yyvsp[0].frame_bound), new FrameBound{0, kCurrentRow, false}}; }
		break;

	case 211: /* opt_frame_clause: frame_type BETWEEN frame_bound AND frame_bound  */
	                                                 { (yyval.frame_description) = new FrameDescription{(yyvsp[-4].frame_type), (yyvsp[-2].frame_bound), (yyvsp[0].frame_bound)}; }
		break;

	case 212: /* opt_frame_clause: %empty  */
	              {
  (yyval.frame_description) = new FrameDescription{kRange, new FrameBound{0, kPreceding, true}, new FrameBound{0, kCurrentRow, false}};
}
		break;

	case 213: /* frame_type: RANGE  */
	                   { (yyval.frame_type) = kRange; }
		break;

	case 214: /* frame_type: ROWS  */
	       { (yyval.frame_type) = kRows; }
		break;

	case 215: /* frame_type: GROUPS  */
	         { (yyval.frame_type) = kGroups; }
		break;

	case 216: /* frame_bound: UNBOUNDED PRECEDING  */
	                                  { (yyval.frame_bound) = new FrameBound{0, kPreceding, true}; }
		break;

	case 217: /* frame_bound: INTVAL PRECEDING  */
	                   { (yyval.frame_bound) = new FrameBound{(yyvsp[-1].ival), kPreceding, false}; }
		break;

	case 218: /* frame_bound: UNBOUNDED FOLLOWING  */
	                      { (yyval.frame_bound) = new FrameBound{0, kFollowing, true}; }
		break;

	case 219: /* frame_bound: INTVAL FOLLOWING  */
	                   { (yyval.frame_bound) = new FrameBound{(yyvsp[-1].ival), kFollowing, false}; }
		break;

	case 220: /* frame_bound: CURRENT_ROW  */
	              { (yyval.frame_bound) = new FrameBound{0, kCurrentRow, false}; }
		break;

	case 221: /* extract_expr: EXTRACT '(' datetime_field FROM expr ')'  */
	                                                        { (yyval.expr) = Expr::makeExtract((yyvsp[-3].datetime_field), (yyvsp[-1].expr)); }
		break;

	case 222: /* cast_expr: CAST '(' expr AS column_type_id_ ')'  */
	                                                 { (yyval.expr) = Expr::makeCast((yyvsp[-3].expr), (yyvsp[-1].column_type_t)); }
		break;

	case 223: /* datetime_field: SECOND  */
	                        { (yyval.datetime_field) = kDatetimeSecond; }
		break;

	case 224: /* datetime_field: MINUTE  */
	         { (yyval.datetime_field) = kDatetimeMinute; }
		break;

	case 225: /* datetime_field: HOUR  */
	       { (yyval.datetime_field) = kDatetimeHour; }
		break;

	case 226: /* datetime_field: DAY  */
	      { (yyval.datetime_field) = kDatetimeDay; }
		break;

	case 227: /* datetime_field: MONTH  */
	        { (yyval.datetime_field) = kDatetimeMonth; }
		break;

	case 228: /* datetime_field: YEAR  */
	       { (yyval.datetime_field) = kDatetimeYear; }
		break;

	case 229: /* datetime_field_plural: SECONDS  */
	                                { (yyval.datetime_field) = kDatetimeSecond; }
		break;

	case 230: /* datetime_field_plural: MINUTES  */
	          { (yyval.datetime_field) = kDatetimeMinute; }
		break;

	case 231: /* datetime_field_plural: HOURS  */
	        { (yyval.datetime_field) = kDatetimeHour; }
		break;

	case 232: /* datetime_field_plural: DAYS  */
	       { (yyval.datetime_field) = kDatetimeDay; }
		break;

	case 233: /* datetime_field_plural: MONTHS  */
	         { (yyval.datetime_field) = kDatetimeMonth; }
		break;

	case 234: /* datetime_field_plural: YEARS  */
	        { (yyval.datetime_field) = kDatetimeYear; }
		break;

	case 237: /* array_expr: ARRAY '[' expr_list ']'  */
	                                     { (yyval.expr) = Expr::makeArray((yyvsp[-1].expr_vec)); }
		break;

	case 238: /* array_index: operand '[' int_literal ']'  */
	                                          { (yyval.expr) = Expr::makeArrayIndex((yyvsp[-3].expr), (yyvsp[-1].expr)->ival); }
		break;

	case 239: /* between_expr: operand BETWEEN operand AND operand  */
	                                                   { (yyval.expr) = Expr::makeBetween((yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr)); }
		break;

	case 240: /* column_name: IDENTIFIER  */
	                         { (yyval.expr) = Expr::makeColumnRef((yyvsp[0].sval)); }
		break;

	case 241: /* column_name: IDENTIFIER '.' IDENTIFIER  */
	                            { (yyval.expr) = Expr::makeColumnRef((yyvsp[-2].sval), (yyvsp[0].sval)); }
		break;

	case 242: /* column_name: '*'  */
	      { (yyval.expr) = Expr::makeStar(); }
		break;

	case 243: /* column_name: IDENTIFIER '.' '*'  */
	                     { (yyval.expr) = Expr::makeStar((yyvsp[-2].sval)); }
		break;

	case 250: /* string_literal: STRING  */
	                        { (yyval.expr) = Expr::makeLiteral((yyvsp[0].sval)); }
		break;

	case 251: /* bool_literal: TRUE  */
	                    { (yyval.expr) = Expr::makeLiteral(true); }
		break;

	case 252: /* bool_literal: FALSE  */
	                                                              { (yyval.expr) = Expr::makeLiteral(false); }
		break;

	case 253: /* num_literal: FLOATVAL  */
	                       { (yyval.expr) = Expr::makeLiteral((yyvsp[0].fval)); }
		break;

	case 255: /* int_literal: INTVAL  */
	                     { (yyval.expr) = Expr::makeLiteral((yyvsp[0].ival)); }
		break;

	case 256: /* null_literal: NULL  */
	                    { (yyval.expr) = Expr::makeNullLiteral(); }
		break;

	case 257: /* interval_literal: int_literal duration_field  */
	                                              {
  (yyval.expr) = Expr::makeIntervalLiteral((yyvsp[-1].expr)->ival, (yyvsp[0].datetime_field));
  delete (yyvsp[-1].expr);
}
		break;

	case 258: /* interval_literal: INTERVAL STRING datetime_field  */
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

	case 259: /* interval_literal: INTERVAL STRING  */
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

	case 260: /* param_expr: '?'  */
	                 {
  (yyval.expr) = Expr::makeParameter(yylloc.total_column);
  (yyval.expr)->ival2 = yyloc.param_list.size();
  yyloc.param_list.push_back((yyval.expr));
}
		break;

	case 262: /* table_ref: table_ref_commalist ',' table_ref_atomic  */
	                                                                        {
  (yyvsp[-2].table_vec)->push_back((yyvsp[0].table));
  auto tbl = new TableRef(kTableCrossProduct);
  tbl->list = (yyvsp[-2].table_vec);
  (yyval.table) = tbl;
}
		break;

	case 266: /* nonjoin_table_ref_atomic: '(' select_statement ')' opt_table_alias  */
	                                                                                     {
  auto tbl = new TableRef(kTableSelect);
  tbl->select = (yyvsp[-2].select_stmt);
  tbl->alias = (yyvsp[0].alias_t);
  (yyval.table) = tbl;
}
		break;

	case 267: /* table_ref_commalist: table_ref_atomic  */
	                                       {
  (yyval.table_vec) = new std::vector<TableRef*>();
  (yyval.table_vec)->push_back((yyvsp[0].table));
}
		break;

	case 268: /* table_ref_commalist: table_ref_commalist ',' table_ref_atomic  */
	                                             {
  (yyvsp[-2].table_vec)->push_back((yyvsp[0].table));
  (yyval.table_vec) = (yyvsp[-2].table_vec);
}
		break;

	case 269: /* table_ref_name: table_name opt_table_alias  */
	                                            {
  auto tbl = new TableRef(kTableName);
  tbl->schema = (yyvsp[-1].table_name).schema;
  tbl->name = (yyvsp[-1].table_name).name;
  tbl->alias = (yyvsp[0].alias_t);
  (yyval.table) = tbl;
}
		break;

	case 270: /* table_ref_name_no_alias: table_name  */
	                                     {
  (yyval.table) = new TableRef(kTableName);
  (yyval.table)->schema = (yyvsp[0].table_name).schema;
  (yyval.table)->name = (yyvsp[0].table_name).name;
}
		break;

	case 271: /* table_name: IDENTIFIER  */
	                        {
  (yyval.table_name).schema = nullptr;
  (yyval.table_name).name = (yyvsp[0].sval);
}
		break;

	case 272: /* table_name: IDENTIFIER '.' IDENTIFIER  */
	                              {
  (yyval.table_name).schema = (yyvsp[-2].sval);
  (yyval.table_name).name = (yyvsp[0].sval);
}
		break;

	case 273: /* opt_index_name: IDENTIFIER  */
	                            { (yyval.sval) = (yyvsp[0].sval); }
		break;

	case 274: /* opt_index_name: %empty  */
	                                                       { (yyval.sval) = nullptr; }
		break;

	case 276: /* table_alias: AS IDENTIFIER '(' ident_commalist ')'  */
	                                                            { (yyval.alias_t) = new Alias((yyvsp[-3].sval), (yyvsp[-1].str_vec)); }
		break;

	case 278: /* opt_table_alias: %empty  */
	                                            { (yyval.alias_t) = nullptr; }
		break;

	case 279: /* alias: AS IDENTIFIER  */
	                      { (yyval.alias_t) = new Alias((yyvsp[0].sval)); }
		break;

	case 280: /* alias: IDENTIFIER  */
	                                                           { (yyval.alias_t) = new Alias((yyvsp[0].sval)); }
		break;

	case 282: /* opt_alias: %empty  */
	                                { (yyval.alias_t) = nullptr; }
		break;

	case 283: /* opt_locking_clause: opt_locking_clause_list  */
	                                             { (yyval.locking_clause_vec) = (yyvsp[0].locking_clause_vec); }
		break;

	case 284: /* opt_locking_clause: %empty  */
	              { (yyval.locking_clause_vec) = nullptr; }
		break;

	case 285: /* opt_locking_clause_list: locking_clause  */
	                                         {
  (yyval.locking_clause_vec) = new std::vector<LockingClause*>();
  (yyval.locking_clause_vec)->push_back((yyvsp[0].locking_t));
}
		break;

	case 286: /* opt_locking_clause_list: opt_locking_clause_list locking_clause  */
	                                           {
  (yyvsp[-1].locking_clause_vec)->push_back((yyvsp[0].locking_t));
  (yyval.locking_clause_vec) = (yyvsp[-1].locking_clause_vec);
}
		break;

	case 287: /* locking_clause: FOR row_lock_mode opt_row_lock_policy  */
	                                                       {
  (yyval.locking_t) = new LockingClause();
  (yyval.locking_t)->rowLockMode = (yyvsp[-1].lock_mode_t);
  (yyval.locking_t)->rowLockWaitPolicy = (yyvsp[0].lock_wait_policy_t);
  (yyval.locking_t)->tables = nullptr;
}
		break;

	case 288: /* locking_clause: FOR row_lock_mode OF ident_commalist opt_row_lock_policy  */
	                                                             {
  (yyval.locking_t) = new LockingClause();
  (yyval.locking_t)->rowLockMode = (yyvsp[-3].lock_mode_t);
  (yyval.locking_t)->tables = (yyvsp[-1].str_vec);
  (yyval.locking_t)->rowLockWaitPolicy = (yyvsp[0].lock_wait_policy_t);
}
		break;

	case 289: /* row_lock_mode: UPDATE  */
	                       { (yyval.lock_mode_t) = RowLockMode::ForUpdate; }
		break;

	case 290: /* row_lock_mode: NO KEY UPDATE  */
	                { (yyval.lock_mode_t) = RowLockMode::ForNoKeyUpdate; }
		break;

	case 291: /* row_lock_mode: SHARE  */
	        { (yyval.lock_mode_t) = RowLockMode::ForShare; }
		break;

	case 292: /* row_lock_mode: KEY SHARE  */
	            { (yyval.lock_mode_t) = RowLockMode::ForKeyShare; }
		break;

	case 293: /* opt_row_lock_policy: SKIP LOCKED  */
	                                  { (yyval.lock_wait_policy_t) = RowLockWaitPolicy::SkipLocked; }
		break;

	case 294: /* opt_row_lock_policy: NOWAIT  */
	         { (yyval.lock_wait_policy_t) = RowLockWaitPolicy::NoWait; }
		break;

	case 295: /* opt_row_lock_policy: %empty  */
	              { (yyval.lock_wait_policy_t) = RowLockWaitPolicy::None; }
		break;

	case 297: /* opt_with_clause: %empty  */
	                                            { (yyval.with_description_vec) = nullptr; }
		break;

	case 298: /* with_clause: WITH with_description_list  */
	                                         { (yyval.with_description_vec) = (yyvsp[0].with_description_vec); }
		break;

	case 299: /* with_description_list: with_description  */
	                                         {
  (yyval.with_description_vec) = new std::vector<WithDescription*>();
  (yyval.with_description_vec)->push_back((yyvsp[0].with_description_t));
}
		break;

	case 300: /* with_description_list: with_description_list ',' with_description  */
	                                             {
  (yyvsp[-2].with_description_vec)->push_back((yyvsp[0].with_description_t));
  (yyval.with_description_vec) = (yyvsp[-2].with_description_vec);
}
		break;

	case 301: /* with_description: IDENTIFIER AS select_with_paren  */
	                                                   {
  (yyval.with_description_t) = new WithDescription();
  (yyval.with_description_t)->alias = (yyvsp[-2].sval);
  (yyval.with_description_t)->select = (yyvsp[0].select_stmt);
}
		break;

	case 302: /* join_clause: table_ref_atomic NATURAL JOIN nonjoin_table_ref_atomic  */
	                                                                     {
  (yyval.table) = new TableRef(kTableJoin);
  (yyval.table)->join = new JoinDefinition();
  (yyval.table)->join->type = kJoinNatural;
  (yyval.table)->join->left = (yyvsp[-3].table);
  (yyval.table)->join->right = (yyvsp[0].table);
}
		break;

	case 303: /* join_clause: table_ref_atomic opt_join_type JOIN table_ref_atomic ON join_condition  */
	                                                                           {
  (yyval.table) = new TableRef(kTableJoin);
  (yyval.table)->join = new JoinDefinition();
  (yyval.table)->join->type = (JoinType)(yyvsp[-4].join_type);
  (yyval.table)->join->left = (yyvsp[-5].table);
  (yyval.table)->join->right = (yyvsp[-2].table);
  (yyval.table)->join->condition = (yyvsp[0].expr);
}
		break;

	case 304: /* join_clause: table_ref_atomic opt_join_type JOIN table_ref_atomic USING '(' column_name ')'  */
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

	case 305: /* opt_join_type: INNER  */
	                      { (yyval.join_type) = kJoinInner; }
		break;

	case 306: /* opt_join_type: LEFT OUTER  */
	             { (yyval.join_type) = kJoinLeft; }
		break;

	case 307: /* opt_join_type: LEFT  */
	       { (yyval.join_type) = kJoinLeft; }
		break;

	case 308: /* opt_join_type: RIGHT OUTER  */
	              { (yyval.join_type) = kJoinRight; }
		break;

	case 309: /* opt_join_type: RIGHT  */
	        { (yyval.join_type) = kJoinRight; }
		break;

	case 310: /* opt_join_type: FULL OUTER  */
	             { (yyval.join_type) = kJoinFull; }
		break;

	case 311: /* opt_join_type: OUTER  */
	        { (yyval.join_type) = kJoinFull; }
		break;

	case 312: /* opt_join_type: FULL  */
	       { (yyval.join_type) = kJoinFull; }
		break;

	case 313: /* opt_join_type: CROSS  */
	        { (yyval.join_type) = kJoinCross; }
		break;

	case 314: /* opt_join_type: %empty  */
	                       { (yyval.join_type) = kJoinInner; }
		break;

	case 318: /* ident_commalist: IDENTIFIER  */
	                             {
  (yyval.str_vec) = new std::vector<char*>();
  (yyval.str_vec)->push_back((yyvsp[0].sval));
}
		break;

	case 319: /* ident_commalist: ident_commalist ',' IDENTIFIER  */
	                                   {
  (yyvsp[-2].str_vec)->push_back((yyvsp[0].sval));
  (yyval.str_vec) = (yyvsp[-2].str_vec);
}
		break;

	case 320: /* index_segment_flags: index_segment_flags ASC  */
	                                              {
    (yyval.ival) &= ~IndexSegment::fDesc;
}
		break;

	case 321: /* index_segment_flags: index_segment_flags DESC  */
	                             {
    (yyval.ival) |= IndexSegment::fDesc;
}
		break;

	case 322: /* index_segment_flags: %empty  */
	              {
    (yyval.ival) = 0;
}
		break;

	case 323: /* index_segment: IDENTIFIER index_segment_flags  */
	                                               {
    (yyval.P_IdxSeg) = new IndexSegment;
    (yyval.P_IdxSeg)->Field = (yyvsp[-1].sval);
    ZDELETE((yyvsp[-1].sval));
    (yyval.P_IdxSeg)->Flags = static_cast<uint>((yyvsp[0].ival));
}
		break;

	case 324: /* index_segment_list: index_segment  */
	                                   {
    (yyval.P_IdxVal) = new Index;
    (yyval.P_IdxVal)->insert((yyvsp[0].P_IdxSeg));
}
		break;

	case 325: /* index_segment_list: index_segment_list ',' index_segment  */
	                                         {
    (yyval.P_IdxVal)->insert((yyvsp[0].P_IdxSeg));
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
//
// Section 4: Additional C code
//
    /* empty */
