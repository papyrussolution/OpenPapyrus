/* A Bison parser, made by GNU Bison 3.7.1.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_HSQL_D_PAPYRUS_SRC_OSF_SQL_PARSER_SRC_PARSER_BISON_PARSER_H_INCLUDED
# define YY_HSQL_D_PAPYRUS_SRC_OSF_SQL_PARSER_SRC_PARSER_BISON_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef HSQL_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define HSQL_DEBUG 1
#  else
#   define HSQL_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define HSQL_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined HSQL_DEBUG */
#if HSQL_DEBUG
extern int hsql_debug;
#endif
/* "%code requires" blocks.  */

// %code requires block

#include "parser_typedef.h"

// Auto update column and line number
#define YY_USER_ACTION                        \
  yylloc->first_line = yylloc->last_line;     \
  yylloc->first_column = yylloc->last_column; \
  for(int i = 0; yytext[i] != '\0'; i++) {   \
    yylloc->total_column++;                   \
    yylloc->string_length++;                  \
    if(yytext[i] == '\n') {                  \
      yylloc->last_line++;                    \
      yylloc->last_column = 0;                \
    } else {                                  \
      yylloc->last_column++;                  \
    }                                         \
  }


/* Token kinds.  */
#ifndef HSQL_TOKENTYPE
# define HSQL_TOKENTYPE
  enum hsql_tokentype
  {
    SQL_HSQL_EMPTY = -2,
    SQL_YYEOF = 0,                 /* "end of file"  */
    SQL_HSQL_error = 256,          /* error  */
    SQL_HSQL_UNDEF = 257,          /* "invalid token"  */
    SQL_IDENTIFIER = 258,          /* IDENTIFIER  */
    SQL_STRING = 259,              /* STRING  */
    SQL_FLOATVAL = 260,            /* FLOATVAL  */
    SQL_INTVAL = 261,              /* INTVAL  */
    SQL_DEALLOCATE = 262,          /* DEALLOCATE  */
    SQL_PARAMETERS = 263,          /* PARAMETERS  */
    SQL_INTERSECT = 264,           /* INTERSECT  */
    SQL_DISTINCT = 265,            /* DISTINCT  */
    SQL_RESTRICT = 266,            /* RESTRICT  */
    SQL_TRUNCATE = 267,            /* TRUNCATE  */
    SQL_ANALYZE = 268,             /* ANALYZE  */
    SQL_BETWEEN = 269,             /* BETWEEN  */
    SQL_CASCADE = 270,             /* CASCADE  */
    SQL_COLUMNS = 271,             /* COLUMNS  */
    SQL_CONTROL = 272,             /* CONTROL  */
    SQL_DEFAULT = 273,             /* DEFAULT  */
    SQL_EXECUTE = 274,             /* EXECUTE  */
    SQL_EXPLAIN = 275,             /* EXPLAIN  */
    SQL_NATURAL = 276,             /* NATURAL  */
    SQL_PREPARE = 277,             /* PREPARE  */
    SQL_PRIMARY = 278,             /* PRIMARY  */
    SQL_SCHEMAS = 279,             /* SCHEMAS  */
    SQL_SPATIAL = 280,             /* SPATIAL  */
    SQL_VIRTUAL = 281,             /* VIRTUAL  */
    SQL_DESCRIBE = 282,            /* DESCRIBE  */
    SQL_BEFORE = 283,              /* BEFORE  */
    SQL_COLUMN = 284,              /* COLUMN  */
    SQL_CREATE = 285,              /* CREATE  */
    SQL_DELETE = 286,              /* DELETE  */
    SQL_DIRECT = 287,              /* DIRECT  */
    SQL_ESCAPE = 288,              /* ESCAPE  */
    SQL_EXCEPT = 289,              /* EXCEPT  */
    SQL_EXISTS = 290,              /* EXISTS  */
    SQL_EXTRACT = 291,             /* EXTRACT  */
    SQL_CAST = 292,                /* CAST  */
    SQL_WITH_FORMAT = 293,         /* WITH_FORMAT  */
    SQL_GLOBAL = 294,              /* GLOBAL  */
    SQL_HAVING = 295,              /* HAVING  */
    SQL_IMPORT = 296,              /* IMPORT  */
    SQL_INSERT = 297,              /* INSERT  */
    SQL_ISNULL = 298,              /* ISNULL  */
    SQL_OFFSET = 299,              /* OFFSET  */
    SQL_RENAME = 300,              /* RENAME  */
    SQL_SCHEMA = 301,              /* SCHEMA  */
    SQL_SELECT = 302,              /* SELECT  */
    SQL_SORTED = 303,              /* SORTED  */
    SQL_TABLES = 304,              /* TABLES  */
    SQL_UNIQUE = 305,              /* UNIQUE  */
    SQL_AUTO_INCREMENT = 306,      /* AUTO_INCREMENT  */
    SQL_UNLOAD = 307,              /* UNLOAD  */
    SQL_UPDATE = 308,              /* UPDATE  */
    SQL_VALUES = 309,              /* VALUES  */
    SQL_AFTER = 310,               /* AFTER  */
    SQL_ALTER = 311,               /* ALTER  */
    SQL_CROSS = 312,               /* CROSS  */
    SQL_DELTA = 313,               /* DELTA  */
    SQL_GROUP = 314,               /* GROUP  */
    SQL_INDEX = 315,               /* INDEX  */
    SQL_INNER = 316,               /* INNER  */
    SQL_LIMIT = 317,               /* LIMIT  */
    SQL_LOCAL = 318,               /* LOCAL  */
    SQL_MERGE = 319,               /* MERGE  */
    SQL_MINUS = 320,               /* MINUS  */
    SQL_ORDER = 321,               /* ORDER  */
    SQL_OVER = 322,                /* OVER  */
    SQL_OUTER = 323,               /* OUTER  */
    SQL_RIGHT = 324,               /* RIGHT  */
    SQL_TABLE = 325,               /* TABLE  */
    SQL_UNION = 326,               /* UNION  */
    SQL_USING = 327,               /* USING  */
    SQL_WHERE = 328,               /* WHERE  */
    SQL_CALL = 329,                /* CALL  */
    SQL_CASE = 330,                /* CASE  */
    SQL_COPY = 331,                /* COPY  */
    SQL_DESC = 332,                /* DESC  */
    SQL_DROP = 333,                /* DROP  */
    SQL_ELSE = 334,                /* ELSE  */
    SQL_FILE = 335,                /* FILE  */
    SQL_FROM = 336,                /* FROM  */
    SQL_FULL = 337,                /* FULL  */
    SQL_HASH = 338,                /* HASH  */
    SQL_HINT = 339,                /* HINT  */
    SQL_INTO = 340,                /* INTO  */
    SQL_JOIN = 341,                /* JOIN  */
    SQL_LEFT = 342,                /* LEFT  */
    SQL_LIKE = 343,                /* LIKE  */
    SQL_LOAD = 344,                /* LOAD  */
    SQL_NULL = 345,                /* NULL  */
    SQL_PARTITION = 346,           /* PARTITION  */
    SQL_PLAN = 347,                /* PLAN  */
    SQL_SHOW = 348,                /* SHOW  */
    SQL_TEXT = 349,                /* TEXT  */
    SQL_THEN = 350,                /* THEN  */
    SQL_VIEW = 351,                /* VIEW  */
    SQL_WHEN = 352,                /* WHEN  */
    SQL_WITH = 353,                /* WITH  */
    SQL_ADD = 354,                 /* ADD  */
    SQL_ALL = 355,                 /* ALL  */
    SQL_AND = 356,                 /* AND  */
    SQL_ASC = 357,                 /* ASC  */
    SQL_FOR = 358,                 /* FOR  */
    SQL_KEY = 359,                 /* KEY  */
    SQL_NOT = 360,                 /* NOT  */
    SQL_OFF = 361,                 /* OFF  */
    SQL_SET = 362,                 /* SET  */
    SQL_TOP = 363,                 /* TOP  */
    SQL_AS = 364,                  /* AS  */
    SQL_BY = 365,                  /* BY  */
    SQL_IF = 366,                  /* IF  */
    SQL_IN = 367,                  /* IN  */
    SQL_IS = 368,                  /* IS  */
    SQL_OF = 369,                  /* OF  */
    SQL_ON = 370,                  /* ON  */
    SQL_OR = 371,                  /* OR  */
    SQL_TO = 372,                  /* TO  */
    SQL_NO = 373,                  /* NO  */
    SQL_ARRAY = 374,               /* ARRAY  */
    SQL_CONCAT = 375,              /* CONCAT  */
    SQL_ILIKE = 376,               /* ILIKE  */
    SQL_SECOND = 377,              /* SECOND  */
    SQL_MINUTE = 378,              /* MINUTE  */
    SQL_HOUR = 379,                /* HOUR  */
    SQL_DAY = 380,                 /* DAY  */
    SQL_MONTH = 381,               /* MONTH  */
    SQL_YEAR = 382,                /* YEAR  */
    SQL_SECONDS = 383,             /* SECONDS  */
    SQL_MINUTES = 384,             /* MINUTES  */
    SQL_HOURS = 385,               /* HOURS  */
    SQL_DAYS = 386,                /* DAYS  */
    SQL_MONTHS = 387,              /* MONTHS  */
    SQL_YEARS = 388,               /* YEARS  */
    SQL_INTERVAL = 389,            /* INTERVAL  */
    SQL_TRUE = 390,                /* TRUE  */
    SQL_FALSE = 391,               /* FALSE  */
    SQL_TRANSACTION = 392,         /* TRANSACTION  */
    SQL_BEGIN = 393,               /* BEGIN  */
    SQL_COMMIT = 394,              /* COMMIT  */
    SQL_ROLLBACK = 395,            /* ROLLBACK  */
    SQL_NOWAIT = 396,              /* NOWAIT  */
    SQL_SKIP = 397,                /* SKIP  */
    SQL_LOCKED = 398,              /* LOCKED  */
    SQL_SHARE = 399,               /* SHARE  */
    SQL_RANGE = 400,               /* RANGE  */
    SQL_ROWS = 401,                /* ROWS  */
    SQL_GROUPS = 402,              /* GROUPS  */
    SQL_UNBOUNDED = 403,           /* UNBOUNDED  */
    SQL_FOLLOWING = 404,           /* FOLLOWING  */
    SQL_PRECEDING = 405,           /* PRECEDING  */
    SQL_CURRENT_ROW = 406,         /* CURRENT_ROW  */
    SQL_CHARACTER_SET = 407,       /* CHARACTER_SET  */
    SQL_CHARACTER_VARYING = 408,   /* CHARACTER_VARYING  */
    SQL_EQUALS = 409,              /* EQUALS  */
    SQL_NOTEQUALS = 410,           /* NOTEQUALS  */
    SQL_LESS = 411,                /* LESS  */
    SQL_GREATER = 412,             /* GREATER  */
    SQL_LESSEQ = 413,              /* LESSEQ  */
    SQL_GREATEREQ = 414,           /* GREATEREQ  */
    SQL_NOTNULL = 415,             /* NOTNULL  */
    SQL_UMINUS = 416               /* UMINUS  */
  };
  typedef enum hsql_tokentype hsql_token_kind_t;
#endif

/* Value type.  */
#if ! defined HSQL_STYPE && ! defined HSQL_STYPE_IS_DECLARED
union HSQL_STYPE
{

    //
    // clang-format on
    //
    bool   bval;
    char * sval;
    double fval;
    int64  ival;
    uintmax_t uval;
    //
    // statements
    //
    hsql::IndexSegment * P_IdxSeg; // @sobolev
    hsql::Index * P_IdxVal; // @sobolev
    hsql::AlterStatement * alter_stmt;
    hsql::CreateStatement * create_stmt;
    hsql::DeleteStatement * delete_stmt;
    hsql::DropStatement * drop_stmt;
    hsql::ExecuteStatement * exec_stmt;
    hsql::ExportStatement * export_stmt;
    hsql::ImportStatement * import_stmt;
    hsql::InsertStatement * insert_stmt;
    hsql::PrepareStatement * prep_stmt;
    hsql::SelectStatement * select_stmt;
    hsql::ShowStatement * show_stmt;
    hsql::SQLStatement * statement;
    hsql::TransactionStatement * transaction_stmt;
    hsql::UpdateStatement * update_stmt;
    hsql::Alias* alias_t;
    hsql::AlterAction * alter_action_t;
    hsql::ColumnDefinition * column_t;
    hsql::ColumnType column_type_t;
    hsql::ConstraintType column_constraint_t;
    hsql::DatetimeField datetime_field;
    hsql::DropColumnAction * drop_action_t;
    hsql::Expr * expr;
    hsql::FrameBound * frame_bound;
    hsql::FrameDescription * frame_description;
    hsql::FrameType frame_type;
    hsql::GroupByDescription * group_t;
    hsql::ImportType import_type_t;
    hsql::JoinType join_type;
    hsql::LimitDescription * limit;
    hsql::LockingClause * locking_t;
    hsql::OrderDescription * order;
    hsql::OrderType order_type;
    hsql::SetOperation * set_operator_t;
    hsql::TableConstraint * table_constraint_t;
    hsql::TableElement * table_element_t;
    hsql::TableName table_name;
    hsql::TableRef * table;
    hsql::UpdateClause * update_t;
    hsql::WindowDescription * window_description;
    hsql::WithDescription * with_description_t;

    std::vector<char*> * str_vec;
    std::unordered_set<hsql::ConstraintType> * column_constraint_set;
    std::vector<hsql::Expr *> * expr_vec;
    std::vector<hsql::OrderDescription*> * order_vec;
    std::vector<hsql::SQLStatement*> * stmt_vec;
    std::vector<hsql::TableElement*> * table_element_vec;
    std::vector<hsql::TableRef*> * table_vec;
    std::vector<hsql::UpdateClause*> * update_vec;
    std::vector<hsql::WithDescription*> * with_description_vec;
    std::vector<hsql::LockingClause*> * locking_clause_vec;
    std::pair<int64, int64> * ival_pair;
    hsql::RowLockMode lock_mode_t;
    hsql::RowLockWaitPolicy lock_wait_policy_t;


};
typedef union HSQL_STYPE HSQL_STYPE;
# define HSQL_STYPE_IS_TRIVIAL 1
# define HSQL_STYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined HSQL_LTYPE && ! defined HSQL_LTYPE_IS_DECLARED
typedef struct HSQL_LTYPE HSQL_LTYPE;
struct HSQL_LTYPE {
	int first_line;
	int first_column;
	int last_line;
	int last_column;
};
	#define HSQL_LTYPE_IS_DECLARED 1
	#define HSQL_LTYPE_IS_TRIVIAL 1
#endif



int hsql_parse (hsql::SQLParserResult* result, yyscan_t scanner);

#endif /* !YY_HSQL_D_PAPYRUS_SRC_OSF_SQL_PARSER_SRC_PARSER_BISON_PARSER_H_INCLUDED  */
