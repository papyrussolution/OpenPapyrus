// clang-format off
%{
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
%}
// clang-format on
//
// Section 2: Bison Parser Declarations
//
// Specify code that is included in the generated .h and .c files
// clang-format off
%code requires {
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
}

// Define the names of the created files (defined in Makefile)
// %output  "bison_parser.cpp"
// %defines "bison_parser.h"

%define api.pure full // Tell bison to create a reentrant parser

// Prefix the parser
%define api.prefix {hsql_}
%define api.token.prefix {SQL_}

%define parse.error verbose
%locations

%initial-action {
    // Initialize
    @$.first_column = 0;
    @$.last_column = 0;
    @$.first_line = 0;
    @$.last_line = 0;
    @$.total_column = 0;
    @$.string_length = 0;
};

// Define additional parameters for yylex (http://www.gnu.org/software/bison/manual/html_node/Pure-Calling.html)
%lex-param   { yyscan_t scanner }

// Define additional parameters for yyparse
%parse-param { hsql::SQLParserResult* result }
%parse-param { yyscan_t scanner }
//
// Define all data-types (http://www.gnu.org/software/bison/manual/html_node/Union-Decl.html)
//
%union {
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
}
//
// Destructor symbols
//
// clang-format off
%destructor { } <fval> <ival> <bval> <join_type> <order_type> <datetime_field> <column_type_t> <column_constraint_t> <import_type_t> <column_constraint_set> <lock_mode_t> <lock_wait_policy_t> <frame_type>
%destructor {
    SAlloc::F(($$.name));
    SAlloc::F(($$.schema));
} <table_name>
%destructor {
    if($$) {
        for(auto ptr : *($$)) {
            SAlloc::F(ptr);
        }
    }
    delete ($$);
} <str_vec>
%destructor { SAlloc::F(($$)); } <sval>
%destructor {
    if($$) {
        for(auto ptr : *($$)) {
            delete ptr;
        }
    }
    delete ($$);
} <table_vec> <table_element_vec> <update_vec> <expr_vec> <order_vec> <stmt_vec>
%destructor {
    delete ($$);
} <P_IdxVal> <P_IdxSeg>
%destructor { 
    delete ($$); 
} <*>
    //
    // Token Definition
    //
    %token <sval> IDENTIFIER STRING
    %token <fval> FLOATVAL
    %token <ival> INTVAL

    // SQL Keywords
    %token DEALLOCATE
    %token PARAMETERS 
    %token INTERSECT 
    // @sobolev %token TEMPORARY 
    %token DISTINCT 
    %token RESTRICT 
    %token TRUNCATE 
    %token ANALYZE 
    %token BETWEEN
    %token CASCADE 
    %token COLUMNS 
    %token CONTROL 
    %token DEFAULT 
    %token EXECUTE 
    %token EXPLAIN
    %token NATURAL 
    %token PREPARE 
    %token PRIMARY 
    %token SCHEMAS 
    %token SPATIAL 
    %token VIRTUAL 
    %token DESCRIBE 
    %token BEFORE 
    %token COLUMN 
    %token CREATE 
    %token DELETE 
    %token DIRECT
    %token ESCAPE 
    %token EXCEPT 
    %token EXISTS 
    %token EXTRACT 
    %token CAST 
    %token WITH_FORMAT 
    %token GLOBAL 
    %token HAVING 
    %token IMPORT
    %token INSERT 
    %token ISNULL 
    %token OFFSET 
    %token RENAME 
    %token SCHEMA 
    %token SELECT 
    %token SORTED
    %token TABLES 
    %token UNIQUE
    %token AUTO_INCREMENT
    %token UNLOAD 
    %token UPDATE 
    %token VALUES 
    %token AFTER 
    %token ALTER 
    %token CROSS
    %token DELTA 
    %token GROUP 
    %token INDEX 
    %token INNER 
    %token LIMIT 
    %token LOCAL 
    %token MERGE 
    %token MINUS 
    %token ORDER 
    %token OVER
    %token OUTER 
    %token RIGHT 
    %token TABLE 
    %token UNION 
    %token USING 
    %token WHERE 
    %token CALL 
    %token CASE 
    %token COPY 
    %token DESC 
    %token DROP 
    %token ELSE 
    %token FILE 
    %token FROM 
    %token FULL 
    %token HASH 
    %token HINT 
    %token INTO 
    %token JOIN
    %token LEFT 
    %token LIKE 
    %token LOAD 
    %token NULL 
    %token PARTITION 
    %token PLAN 
    %token SHOW 
    %token TEXT 
    %token THEN 
    %token VIEW 
    %token WHEN 
    %token WITH 
    %token ADD 
    %token ALL 
    %token AND 
    %token ASC 
    // @sobolev %token END 
    %token FOR 
    %token KEY
    %token NOT 
    %token OFF 
    %token SET 
    %token TOP 
    %token AS 
    %token BY 
    %token IF 
    %token IN 
    %token IS 
    %token OF 
    %token ON 
    %token OR 
    %token TO 
    %token NO
    %token ARRAY 
    %token CONCAT 
    %token ILIKE 
    %token SECOND 
    %token MINUTE 
    %token HOUR 
    %token DAY 
    %token MONTH 
    %token YEAR
    %token SECONDS 
    %token MINUTES 
    %token HOURS 
    %token DAYS 
    %token MONTHS 
    %token YEARS 
    %token INTERVAL
    %token TRUE 
    %token FALSE 
    %token TRANSACTION 
    %token BEGIN 
    %token COMMIT 
    %token ROLLBACK
    %token NOWAIT 
    %token SKIP 
    %token LOCKED 
    %token SHARE
    %token RANGE 
    %token ROWS 
    %token GROUPS 
    %token UNBOUNDED 
    %token FOLLOWING 
    %token PRECEDING 
    %token CURRENT_ROW
    %token CHARACTER_SET
    // types:
    %token CHARACTER_VARYING 
    /* @sobolev
    %token CHAR 
    %token VARCHAR 
    %token NVARCHAR 
    %token TIMESTAMP
    %token INTEGER 
    %token REAL 
    %token DECIMAL 
    %token SMALLINT 
    %token BIGINT
    %token DOUBLE 
    %token FLOAT 
    %token DATE 
    %token DATETIME
    %token LONG 
    %token TIME
    %token INT 
    %token BOOLEAN
    %token BINARY // @sobolev
    %token RAW // @sobolev
    */
    //
    // Non-Terminal types (http://www.gnu.org/software/bison/manual/html_node/Type-Decl.html)
    //
    %type <stmt_vec>               statement_list
    %type <statement>              statement preparable_statement
    %type <exec_stmt>              execute_statement
    %type <transaction_stmt>       transaction_statement
    %type <prep_stmt>              prepare_statement
    %type <select_stmt>            select_statement select_with_paren select_no_paren select_clause select_within_set_operation select_within_set_operation_no_parentheses
    %type <import_stmt>            import_statement
    %type <export_stmt>            export_statement
    %type <create_stmt>            create_statement
    %type <insert_stmt>            insert_statement
    %type <delete_stmt>            delete_statement truncate_statement
    %type <update_stmt>            update_statement
    %type <drop_stmt>              drop_statement
    %type <alter_stmt>             alter_statement
    %type <show_stmt>              show_statement
    %type <table_name>             table_name
    %type <sval>                   opt_index_name
    %type <sval>                   file_path prepare_target_query
    %type <sval>                   opt_character_set // @sobolev
    %type <frame_description>      opt_frame_clause
    %type <frame_bound>            frame_bound
    %type <frame_type>             frame_type
    %type <window_description>     opt_window
    %type <bval>                   opt_not_exists opt_exists opt_distinct opt_all
    %type <ival_pair>              opt_decimal_specification
    %type <ival>                   opt_time_precision
    %type <join_type>              opt_join_type
    %type <table>                  opt_from_clause from_clause table_ref table_ref_atomic table_ref_name nonjoin_table_ref_atomic
    %type <table>                  join_clause table_ref_name_no_alias
    %type <expr>                   expr operand scalar_expr unary_expr binary_expr logic_expr exists_expr extract_expr cast_expr
    %type <expr>                   function_expr between_expr expr_alias param_expr
    %type <expr>                   column_name literal int_literal num_literal string_literal bool_literal date_literal interval_literal
    %type <expr>                   comp_expr opt_where join_condition opt_having case_expr case_list in_expr hint
    %type <expr>                   array_expr array_index null_literal
    %type <limit>                  opt_limit opt_top
    %type <order>                  order_desc
    %type <order_type>             opt_order_type
    %type <datetime_field>         datetime_field datetime_field_plural duration_field
    %type <column_t>               column_def
    %type <table_element_t>        table_elem
    // @sobolev %type <column_type_t>          column_type
    %type <column_type_t>          column_type_id_ // @sobolev
    %type <table_constraint_t>     table_constraint
    %type <update_t>               update_clause
    %type <locking_t>              locking_clause
    %type <group_t>                opt_group
    %type <alias_t>                opt_table_alias table_alias opt_alias alias
    %type <with_description_t>     with_description
    %type <set_operator_t>         set_operator set_type
    %type <column_constraint_t>    column_constraint
    %type <column_constraint_set>  opt_column_constraints
    %type <column_constraint_set>  column_constraint_set
    %type <alter_action_t>         alter_action
    %type <drop_action_t>          drop_action
    %type <lock_wait_policy_t>     opt_row_lock_policy
    %type <lock_mode_t>            row_lock_mode

    // ImportType is used for compatibility reasons
    %type <import_type_t>          opt_file_type file_type

    %type <str_vec>                ident_commalist opt_column_list
    %type <ival>                   index_segment_flags // @sobolev
    %type <P_IdxSeg>               index_segment // @sobolev
    %type <P_IdxVal>               index_segment_list // @sobolev
    %type <expr_vec>               expr_list select_list opt_literal_list literal_list hint_list opt_hints opt_partition
    %type <table_vec>              table_ref_commalist
    %type <order_vec>              opt_order order_list
    %type <with_description_vec>   opt_with_clause with_clause with_description_list
    %type <update_vec>             update_clause_commalist
    %type <table_element_vec>      table_elem_commalist
    %type <locking_clause_vec>     opt_locking_clause_list opt_locking_clause
    //
    // Token Precedence and Associativity
    // Precedence: lowest to highest
    //
    %left     OR
    %left     AND
    %right    NOT
    %nonassoc '=' EQUALS NOTEQUALS LIKE ILIKE
    %nonassoc '<' '>' LESS GREATER LESSEQ GREATEREQ

    %nonassoc NOTNULL
    %nonassoc ISNULL
    %nonassoc IS        /* sets precedence for IS NULL, etc */
    %left     '+' '-'
    %left     '*' '/' '%'
    %left     '^'
    %left     CONCAT

    /* Unary Operators */
    %right    UMINUS
    %left     '[' ']'
    %left     '(' ')'
    %left     '.'
    %left     JOIN
%%
//
// Section 3: Grammar Definition
//
// Defines our general input.
input : statement_list opt_semicolon {
  for(SQLStatement* stmt : *$1) {
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
    delete $1;
  };

// clang-format on
statement_list : statement {
  $1->stringLength = yylloc.string_length;
  yylloc.string_length = 0;
  $$ = new std::vector<SQLStatement*>();
  $$->push_back($1);
} | statement_list ';' statement {
  $3->stringLength = yylloc.string_length;
  yylloc.string_length = 0;
  $1->push_back($3);
  $$ = $1;
};

statement : prepare_statement opt_hints {
  $$ = $1;
  $$->hints = $2;
} | preparable_statement opt_hints {
  $$ = $1;
  $$->hints = $2;
} | show_statement { $$ = $1; } | import_statement { $$ = $1; } | export_statement { $$ = $1; };

preparable_statement : select_statement { $$ = $1; }
| create_statement { $$ = $1; }
| insert_statement { $$ = $1; }
| delete_statement { $$ = $1; }
| truncate_statement { $$ = $1; }
| update_statement { $$ = $1; }
| drop_statement { $$ = $1; }
| alter_statement { $$ = $1; }
| execute_statement { $$ = $1; }
| transaction_statement { $$ = $1; };
//
// Hints
//
opt_hints : WITH HINT '(' hint_list ')' { $$ = $4; } | /* empty */ { $$ = nullptr; };

hint_list : hint {
    $$ = new std::vector<Expr*>();
    $$->push_back($1);
} | hint_list ',' hint {
    $1->push_back($3);
    $$ = $1;
};

hint : IDENTIFIER {
    $$ = Expr::make(kExprHint);
    $$->name = $1;
} | IDENTIFIER '(' literal_list ')' {
    $$ = Expr::make(kExprHint);
    $$->name = $1;
    $$->exprList = $3;
};
//
// Transaction Statement
//
transaction_statement : BEGIN opt_transaction_keyword { $$ = new TransactionStatement(kBeginTransaction); }
| ROLLBACK opt_transaction_keyword { $$ = new TransactionStatement(kRollbackTransaction); }
| COMMIT opt_transaction_keyword { $$ = new TransactionStatement(kCommitTransaction); };

opt_transaction_keyword : TRANSACTION | /* empty */;
//
// Prepared Statement
//
prepare_statement : PREPARE IDENTIFIER FROM prepare_target_query {
    $$ = new PrepareStatement();
    $$->name = $2;
    $$->query = $4;
};

prepare_target_query : STRING execute_statement : EXECUTE IDENTIFIER {
    $$ = new ExecuteStatement();
    $$->name = $2;
} | EXECUTE IDENTIFIER '(' opt_literal_list ')' {
    $$ = new ExecuteStatement();
    $$->name = $2;
    $$->parameters = $4;
};
//
// Import Statement
// IMPORT FROM TBL FILE 'test/students.tbl' INTO students
// COPY students FROM 'test/students.tbl' [WITH FORMAT TBL]
//
import_statement : IMPORT FROM file_type FILE file_path INTO table_name {
    $$ = new ImportStatement($3);
    $$->filePath = $5;
    $$->schema = $7.schema;
    $$->tableName = $7.name;
} | COPY table_name FROM file_path opt_file_type opt_where {
    $$ = new ImportStatement($5);
    $$->filePath = $4;
    $$->schema = $2.schema;
    $$->tableName = $2.name;
    $$->whereClause = $6;
};

file_type : IDENTIFIER {
  if(sstreqi_ascii($1, "csv")) {
    $$ = kImportCSV;
  } 
  else if(sstreqi_ascii($1, "tbl")) {
    $$ = kImportTbl;
  } 
  else if(sstreqi_ascii($1, "binary") || sstreqi_ascii($1, "bin")) {
    $$ = kImportBinary;
  } 
  else {
    SAlloc::F($1);
    yyerror(&yyloc, result, scanner, "File type is unknown.");
    YYERROR;
  }
  SAlloc::F($1);
} | "BINARY" { // @sobolev
    $$ = kImportBinary;
};

file_path : string_literal {
    $$ = _strdup($1->name);
    delete $1;
};

opt_file_type : WITH_FORMAT file_type { $$ = $2; } | /* empty */ { $$ = kImportAuto; };
//
// Export Statement
// COPY students TO 'test/students.tbl' (WITH FORMAT TBL)
//
export_statement : COPY table_name TO file_path opt_file_type {
    $$ = new ExportStatement($5);
    $$->filePath = $4;
    $$->schema = $2.schema;
    $$->tableName = $2.name;
} | COPY select_with_paren TO file_path opt_file_type {
    $$ = new ExportStatement($5);
    $$->filePath = $4;
    $$->select = $2;
};
//
// Show Statement
// SHOW TABLES;
//
show_statement : SHOW TABLES { 
    $$ = new ShowStatement(kShowTables); 
} | SHOW COLUMNS table_name {
  $$ = new ShowStatement(kShowColumns);
  $$->schema = $3.schema;
  $$->name = $3.name;
} | DESCRIBE table_name {
  $$ = new ShowStatement(kShowColumns);
  $$->schema = $2.schema;
  $$->name = $2.name;
};
//
// Create Statement
// CREATE TABLE students (name TEXT, student_number INTEGER, city TEXT, grade DOUBLE)
// CREATE TABLE students FROM TBL FILE 'test/students.tbl'
//
create_statement : CREATE TABLE opt_not_exists table_name FROM IDENTIFIER FILE file_path {
  $$ = new CreateStatement(kCreateTableFromTbl);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  if(!sstreqi_ascii($6, "tbl")) {
    SAlloc::F($6);
    yyerror(&yyloc, result, scanner, "File type is unknown.");
    YYERROR;
  }
  SAlloc::F($6);
  $$->filePath = $8;
} | CREATE TABLE opt_not_exists table_name '(' table_elem_commalist ')' {
  $$ = new CreateStatement(kCreateTable);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  $$->setColumnDefsAndConstraints($6);
  delete $6;
  if(result->errorMsg()) {
    delete $$;
    YYERROR;
  }
} | CREATE TABLE opt_not_exists table_name AS select_statement {
  $$ = new CreateStatement(kCreateTable);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  $$->select = $6;
} | CREATE INDEX opt_not_exists opt_index_name ON table_name '(' index_segment_list ')' { // @sobolev ident_commalist-->index_segment_list
    $$ = new CreateStatement(kCreateIndex);
    $$->indexName = $4;
    $$->ifNotExists = $3;
    $$->fUnique = false;
    $$->tableName = $6.name;
    // @sobolev $$->indexColumns = $8;
    $$->P_Idx = $8;
    if($$->P_Idx)
        $$->P_Idx->Name = $$->indexName;
} | CREATE UNIQUE INDEX opt_not_exists opt_index_name ON table_name '(' index_segment_list ')' { // @sobolev
    $$ = new CreateStatement(kCreateIndex);
    $$->indexName = $5;
    $$->ifNotExists = $4;
    $$->fUnique = true;
    $$->tableName = $7.name;
    // @sobolev $$->indexColumns = $9;
    $$->P_Idx = $9;
    if($$->P_Idx)
        $$->P_Idx->Name = $$->indexName;
} | CREATE VIEW opt_not_exists table_name opt_column_list AS select_statement {
  $$ = new CreateStatement(kCreateView);
  $$->ifNotExists = $3;
  $$->schema = $4.schema;
  $$->tableName = $4.name;
  $$->viewColumns = $5;
  $$->select = $7;
};

opt_not_exists : IF NOT EXISTS { $$ = true; } | /* empty */ { $$ = false; };

table_elem_commalist : table_elem {
  $$ = new std::vector<TableElement*>();
  $$->push_back($1);
} | table_elem_commalist ',' table_elem {
  $1->push_back($3);
  $$ = $1;
};

table_elem : column_def { $$ = $1; } | table_constraint { $$ = $1; };

// @sobolev column_def : IDENTIFIER column_type opt_column_constraints {
column_def : IDENTIFIER column_type_id_ opt_column_constraints opt_character_set { // @sobolev
  $$ = new ColumnDefinition($1, $2, $3);
  $$->P_CharSet = $4; // character_set
  if(!$$->trySetNullableExplicit()) {
    yyerror(&yyloc, result, scanner, ("Conflicting nullability constraints for " + std::string{$1}).c_str());
  }
};

// @sobolev
column_type_id_ : CHARACTER_VARYING '(' INTVAL ')' 
{
    SString msg_buf;
    if(!ColumnType::Make("CHARACTER_VARYING", $3, -1, $$, msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }
} | IDENTIFIER 
{
    SString msg_buf;
    if(!ColumnType::Make($1, -1, -1, $$, msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }
} | IDENTIFIER '(' INTVAL ')' 
{
    SString msg_buf;
    if(!ColumnType::Make($1, $3, -1, $$, msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }
} | IDENTIFIER '(' INTVAL ',' INTVAL ')'
{
    SString msg_buf;
    if(!ColumnType::Make($1, $3, $5, $$, msg_buf)) {
        yyerror(&yyloc, result, scanner, msg_buf);
        YYERROR;
    }    
}

/* @sobolev column_type : BIGINT { $$ = ColumnType{hsql::DataType::BIGINT}; }
| BOOLEAN { $$ = ColumnType{hsql::DataType::BOOLEAN}; }
| CHAR '(' INTVAL ')' { $$ = ColumnType{hsql::DataType::CHAR, $3}; }
| CHARACTER_VARYING '(' INTVAL ')' { $$ = ColumnType{hsql::DataType::VARCHAR, $3}; }
| DATE { $$ = ColumnType{hsql::DataType::DATE}; };
| DATETIME { $$ = ColumnType{hsql::DataType::DATETIME}; }
| DECIMAL opt_decimal_specification {
  $$ = ColumnType{hsql::DataType::DECIMAL, 0, $2->first, $2->second};
  delete $2;
} | DOUBLE { $$ = ColumnType{hsql::DataType::DOUBLE}; }
| FLOAT { $$ = ColumnType{hsql::DataType::FLOAT}; }
| INT { $$ = ColumnType{hsql::DataType::INT}; }
| INTEGER { $$ = ColumnType{hsql::DataType::INT}; }
| LONG { $$ = ColumnType{hsql::DataType::LONG}; }
| REAL { $$ = ColumnType{hsql::DataType::REAL}; }
| SMALLINT { $$ = ColumnType{hsql::DataType::SMALLINT}; }
| TEXT { $$ = ColumnType{hsql::DataType::TEXT}; }
| TIME opt_time_precision { $$ = ColumnType{hsql::DataType::TIME, 0, $2}; }
| TIMESTAMP { $$ = ColumnType{hsql::DataType::DATETIME}; }
| VARCHAR '(' INTVAL ')' { $$ = ColumnType{hsql::DataType::VARCHAR, $3}; }
| BINARY '(' INTVAL ')' { $$ = ColumnType{hsql::DataType::BINARY, $3}; } // @sobolev
| RAW '(' INTVAL ')' { $$ = ColumnType{hsql::DataType::BINARY, $3}; } // @sobolev
*/

opt_time_precision : '(' INTVAL ')' { $$ = $2; } | /* empty */ { $$ = 0; };

opt_decimal_specification : '(' INTVAL ',' INTVAL ')' { $$ = new std::pair<int64, int64>{$2, $4}; }
| '(' INTVAL ')' { $$ = new std::pair<int64, int64>{$2, 0}; }
| /* empty */ { $$ = new std::pair<int64, int64>{0, 0}; };

opt_column_constraints : column_constraint_set { $$ = $1; } | /*empty*/ { $$ = new std::unordered_set<ConstraintType>(); };
opt_character_set : CHARACTER_SET IDENTIFIER { $$ = $2; } | /*empty*/ { $$ = 0; }; // @sobolev

column_constraint_set : column_constraint {
  $$ = new std::unordered_set<ConstraintType>();
  $$->insert($1);
} | column_constraint_set column_constraint {
  $1->insert($2);
  $$ = $1;
}

column_constraint : PRIMARY KEY { $$ = ConstraintType::PrimaryKey; }
| UNIQUE { $$ = ConstraintType::Unique; }
| NULL { $$ = ConstraintType::Null; }
| NOT NULL { $$ = ConstraintType::NotNull; }
| AUTO_INCREMENT { $$ = ConstraintType::AutoIncrement; } // @sobolev
;

table_constraint : PRIMARY KEY '(' ident_commalist ')' { $$ = new TableConstraint(ConstraintType::PrimaryKey, $4); }
| UNIQUE '(' ident_commalist ')' { $$ = new TableConstraint(ConstraintType::Unique, $3); };

//
// Drop Statement
// DROP TABLE students;
// DEALLOCATE PREPARE stmt;
//
drop_statement : DROP TABLE opt_exists table_name {
  $$ = new DropStatement(kDropTable);
  $$->ifExists = $3;
  $$->schema = $4.schema;
  $$->name = $4.name;
} | DROP VIEW opt_exists table_name {
  $$ = new DropStatement(kDropView);
  $$->ifExists = $3;
  $$->schema = $4.schema;
  $$->name = $4.name;
} | DEALLOCATE PREPARE IDENTIFIER {
  $$ = new DropStatement(kDropPreparedStatement);
  $$->ifExists = false;
  $$->name = $3;
} | DROP INDEX opt_exists IDENTIFIER {
  $$ = new DropStatement(kDropIndex);
  $$->ifExists = $3;
  $$->indexName = $4;
};

opt_exists : IF EXISTS { $$ = true; } | /* empty */ { $$ = false; };
//
// ALTER Statement
// ALTER TABLE students DROP COLUMN name;
//
alter_statement : ALTER TABLE opt_exists table_name alter_action {
  $$ = new AlterStatement($4.name, $5);
  $$->ifTableExists = $3;
  $$->schema = $4.schema;
};

alter_action : drop_action { $$ = $1; }

drop_action : DROP COLUMN opt_exists IDENTIFIER {
  $$ = new DropColumnAction($4);
  $$->ifExists = $3;
};
//
// Delete Statement / Truncate statement
// DELETE FROM students WHERE grade > 3.0
// DELETE FROM students <=> TRUNCATE students
//
delete_statement : DELETE FROM table_name opt_where {
    $$ = new DeleteStatement();
    $$->schema = $3.schema;
    $$->tableName = $3.name;
    $$->expr = $4;
};

truncate_statement : TRUNCATE table_name {
    $$ = new DeleteStatement();
    $$->schema = $2.schema;
    $$->tableName = $2.name;
};
//
// Insert Statement
// INSERT INTO students VALUES ('Max', 1112233, 'Musterhausen', 2.3)
// INSERT INTO employees SELECT * FROM stundents
//
insert_statement : INSERT INTO table_name opt_column_list VALUES '(' literal_list ')' {
    $$ = new InsertStatement(kInsertValues);
    $$->schema = $3.schema;
    $$->tableName = $3.name;
    $$->columns = $4;
    $$->values = $7;
} | INSERT INTO table_name opt_column_list select_no_paren {
    $$ = new InsertStatement(kInsertSelect);
    $$->schema = $3.schema;
    $$->tableName = $3.name;
    $$->columns = $4;
    $$->select = $5;
};

opt_column_list : '(' ident_commalist ')' { $$ = $2; } | /* empty */ { $$ = nullptr; };

//
// Update Statement
// UPDATE students SET grade = 1.3, name='Felix Fürstenberg' WHERE name = 'Max Mustermann';
//
update_statement : UPDATE table_ref_name_no_alias SET update_clause_commalist opt_where {
  $$ = new UpdateStatement();
  $$->table = $2;
  $$->updates = $4;
  $$->where = $5;
};

update_clause_commalist : update_clause {
  $$ = new std::vector<UpdateClause*>();
  $$->push_back($1);
} | update_clause_commalist ',' update_clause {
  $1->push_back($3);
  $$ = $1;
};

update_clause : IDENTIFIER '=' expr {
  $$ = new UpdateClause();
  $$->column = $1;
  $$->value = $3;
};
//
// Select Statement
//
select_statement : opt_with_clause select_with_paren {
  $$ = $2;
  $$->withDescriptions = $1;
} | opt_with_clause select_no_paren {
  $$ = $2;
  $$->withDescriptions = $1;
} | opt_with_clause select_with_paren set_operator select_within_set_operation opt_order opt_limit {
  $$ = $2;
  if($$->setOperations == nullptr) {
    $$->setOperations = new std::vector<SetOperation*>();
  }
  $$->setOperations->push_back($3);
  $$->setOperations->back()->nestedSelectStatement = $4;
  $$->setOperations->back()->resultOrder = $5;
  $$->setOperations->back()->resultLimit = $6;
  $$->withDescriptions = $1;
};

select_within_set_operation : select_with_paren | select_within_set_operation_no_parentheses;

select_within_set_operation_no_parentheses : select_clause { $$ = $1; }
| select_clause set_operator select_within_set_operation {
  $$ = $1;
  if($$->setOperations == nullptr) {
    $$->setOperations = new std::vector<SetOperation*>();
  }
  $$->setOperations->push_back($2);
  $$->setOperations->back()->nestedSelectStatement = $3;
};

select_with_paren : '(' select_no_paren ')' { $$ = $2; } | '(' select_with_paren ')' { $$ = $2; };

select_no_paren : select_clause opt_order opt_limit opt_locking_clause {
  $$ = $1;
  $$->order = $2;
  // Limit could have been set by TOP.
  if($3) {
    delete $$->limit;
    $$->limit = $3;
  }
  if($4) {
    $$->lockings = $4;
  }
} | select_clause set_operator select_within_set_operation opt_order opt_limit opt_locking_clause {
  $$ = $1;
  if($$->setOperations == nullptr) {
    $$->setOperations = new std::vector<SetOperation*>();
  }
  $$->setOperations->push_back($2);
  $$->setOperations->back()->nestedSelectStatement = $3;
  $$->setOperations->back()->resultOrder = $4;
  $$->setOperations->back()->resultLimit = $5;
  $$->lockings = $6;
};

set_operator : set_type opt_all {
  $$ = $1;
  $$->isAll = $2;
};

set_type : UNION {
  $$ = new SetOperation();
  $$->setType = SetType::kSetUnion;
} | INTERSECT {
  $$ = new SetOperation();
  $$->setType = SetType::kSetIntersect;
} | EXCEPT {
  $$ = new SetOperation();
  $$->setType = SetType::kSetExcept;
};

opt_all : ALL { $$ = true; } | /* empty */ { $$ = false; };

select_clause : SELECT opt_top opt_distinct select_list opt_from_clause opt_where opt_group {
  $$ = new SelectStatement();
  $$->limit = $2;
  $$->selectDistinct = $3;
  $$->selectList = $4;
  $$->fromTable = $5;
  $$->whereClause = $6;
  $$->groupBy = $7;
};

opt_distinct : DISTINCT { $$ = true; } | /* empty */ { $$ = false; };
select_list : expr_list;
opt_from_clause : from_clause { $$ = $1; } | /* empty */ { $$ = nullptr; };

from_clause : FROM table_ref { $$ = $2; };
opt_where : WHERE expr { $$ = $2; } | /* empty */ { $$ = nullptr; };

opt_group : GROUP BY expr_list opt_having {
  $$ = new GroupByDescription();
  $$->columns = $3;
  $$->having = $4;
} | /* empty */ { $$ = nullptr; };

opt_having : HAVING expr { $$ = $2; } | /* empty */ { $$ = nullptr; };
opt_order : ORDER BY order_list { $$ = $3; } | /* empty */ { $$ = nullptr; };

order_list : order_desc {
  $$ = new std::vector<OrderDescription*>();
  $$->push_back($1);
} | order_list ',' order_desc {
  $1->push_back($3);
  $$ = $1;
};

order_desc : expr opt_order_type { $$ = new OrderDescription($2, $1); };
opt_order_type : ASC { $$ = kOrderAsc; } | DESC { $$ = kOrderDesc; } | /* empty */ { $$ = kOrderAsc; };

// TODO: TOP and LIMIT can take more than just int literals.

opt_top : TOP int_literal { $$ = new LimitDescription($2, nullptr); } | /* empty */ { $$ = nullptr; };

opt_limit : LIMIT expr { $$ = new LimitDescription($2, nullptr); }
| OFFSET expr { $$ = new LimitDescription(nullptr, $2); }
| LIMIT expr OFFSET expr { $$ = new LimitDescription($2, $4); }
| LIMIT ALL { $$ = new LimitDescription(nullptr, nullptr); }
| LIMIT ALL OFFSET expr { $$ = new LimitDescription(nullptr, $4); }
| /* empty */ { $$ = nullptr; };
//
// Expressions
//
expr_list : expr_alias {
    $$ = new std::vector<Expr*>();
    $$->push_back($1);
} | expr_list ',' expr_alias {
    $1->push_back($3);
    $$ = $1;
};

opt_literal_list : literal_list { $$ = $1; } | /* empty */ { $$ = nullptr; };

literal_list : literal {
    $$ = new std::vector<Expr*>();
    $$->push_back($1);
} | literal_list ',' literal {
    $1->push_back($3);
    $$ = $1;
};

expr_alias : expr opt_alias {
    $$ = $1;
    if($2) {
        $$->alias = _strdup($2->name);
        delete $2;
    }
};

expr : operand | between_expr | logic_expr | exists_expr | in_expr;

operand : '(' expr ')' { $$ = $2; }
| array_index | scalar_expr | unary_expr | binary_expr | case_expr | function_expr | extract_expr | cast_expr |
    array_expr | '(' select_no_paren ')' {
  $$ = Expr::makeSelect($2);
};

scalar_expr : column_name | literal;

unary_expr : '-' operand { $$ = Expr::makeOpUnary(kOpUnaryMinus, $2); }
| NOT operand { $$ = Expr::makeOpUnary(kOpNot, $2); }
| operand ISNULL { $$ = Expr::makeOpUnary(kOpIsNull, $1); }
| operand IS NULL { $$ = Expr::makeOpUnary(kOpIsNull, $1); }
| operand IS NOT NULL { $$ = Expr::makeOpUnary(kOpNot, Expr::makeOpUnary(kOpIsNull, $1)); };

binary_expr : comp_expr | operand '-' operand { $$ = Expr::makeOpBinary($1, kOpMinus, $3); }
| operand '+' operand { $$ = Expr::makeOpBinary($1, kOpPlus, $3); }
| operand '/' operand { $$ = Expr::makeOpBinary($1, kOpSlash, $3); }
| operand '*' operand { $$ = Expr::makeOpBinary($1, kOpAsterisk, $3); }
| operand '%' operand { $$ = Expr::makeOpBinary($1, kOpPercentage, $3); }
| operand '^' operand { $$ = Expr::makeOpBinary($1, kOpCaret, $3); }
| operand LIKE operand { $$ = Expr::makeOpBinary($1, kOpLike, $3); }
| operand NOT LIKE operand { $$ = Expr::makeOpBinary($1, kOpNotLike, $4); }
| operand ILIKE operand { $$ = Expr::makeOpBinary($1, kOpILike, $3); }
| operand CONCAT operand { $$ = Expr::makeOpBinary($1, kOpConcat, $3); };

logic_expr : expr AND expr { $$ = Expr::makeOpBinary($1, kOpAnd, $3); }
| expr OR expr { $$ = Expr::makeOpBinary($1, kOpOr, $3); };

in_expr : operand IN '(' expr_list ')' { $$ = Expr::makeInOperator($1, $4); }
| operand NOT IN '(' expr_list ')' { $$ = Expr::makeOpUnary(kOpNot, Expr::makeInOperator($1, $5)); }
| operand IN '(' select_no_paren ')' { $$ = Expr::makeInOperator($1, $4); }
| operand NOT IN '(' select_no_paren ')' { $$ = Expr::makeOpUnary(kOpNot, Expr::makeInOperator($1, $5)); };

// CASE grammar based on: flex & bison by John Levine
// https://www.safaribooksonline.com/library/view/flex-bison/9780596805418/ch04.html#id352665
case_expr : CASE expr case_list "END" { $$ = Expr::makeCase($2, $3, nullptr); }
| CASE expr case_list ELSE expr "END" { $$ = Expr::makeCase($2, $3, $5); }
| CASE case_list "END" { $$ = Expr::makeCase(nullptr, $2, nullptr); }
| CASE case_list ELSE expr "END" { $$ = Expr::makeCase(nullptr, $2, $4); };

case_list : WHEN expr THEN expr { $$ = Expr::makeCaseList(Expr::makeCaseListElement($2, $4)); }
| case_list WHEN expr THEN expr { $$ = Expr::caseListAppend($1, Expr::makeCaseListElement($3, $5)); };

exists_expr : EXISTS '(' select_no_paren ')' { $$ = Expr::makeExists($3); }
| NOT EXISTS '(' select_no_paren ')' { $$ = Expr::makeOpUnary(kOpNot, Expr::makeExists($4)); };

comp_expr : operand '=' operand { $$ = Expr::makeOpBinary($1, kOpEquals, $3); }
| operand EQUALS operand { $$ = Expr::makeOpBinary($1, kOpEquals, $3); }
| operand NOTEQUALS operand { $$ = Expr::makeOpBinary($1, kOpNotEquals, $3); }
| operand '<' operand { $$ = Expr::makeOpBinary($1, kOpLess, $3); }
| operand '>' operand { $$ = Expr::makeOpBinary($1, kOpGreater, $3); }
| operand LESSEQ operand { $$ = Expr::makeOpBinary($1, kOpLessEq, $3); }
| operand GREATEREQ operand { $$ = Expr::makeOpBinary($1, kOpGreaterEq, $3); };

// `function_expr is used for window functions, aggregate expressions, and functions calls because we run into shift/
// reduce conflicts when splitting them.
function_expr : IDENTIFIER '(' ')' opt_window { $$ = Expr::makeFunctionRef($1, new std::vector<Expr*>(), false, $4); }
| IDENTIFIER '(' opt_distinct expr_list ')' opt_window { $$ = Expr::makeFunctionRef($1, $4, $3, $6); };

// Window function expressions, based on https://www.postgresql.org/docs/15/sql-expressions.html#SYNTAX-WINDOW-FUNCTIONS
// We do not support named windows, collations and exclusions (for simplicity) and filters (not part of the SQL standard).
opt_window : OVER '(' opt_partition opt_order opt_frame_clause ')' { $$ = new WindowDescription($3, $4, $5); }
| /* empty */ { $$ = nullptr; };

opt_partition : PARTITION BY expr_list { $$ = $3; }
| /* empty */ { $$ = nullptr; };

// We use the Postgres default if the frame end or the whole frame clause is omitted. "If `frame_end` is omitted, the
// end defaults to `CURRENT ROW`. [...] The default framing option is `RANGE UNBOUNDED PRECEDING`, which is the same as
// `RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW`."
opt_frame_clause : frame_type frame_bound { $$ = new FrameDescription{$1, $2, new FrameBound{0, kCurrentRow, false}}; }
| frame_type BETWEEN frame_bound AND frame_bound { $$ = new FrameDescription{$1, $3, $5}; }
| /* empty */ {
  $$ = new FrameDescription{kRange, new FrameBound{0, kPreceding, true}, new FrameBound{0, kCurrentRow, false}};
};

frame_type : RANGE { $$ = kRange; }
| ROWS { $$ = kRows; }
| GROUPS { $$ = kGroups; };

frame_bound : UNBOUNDED PRECEDING { $$ = new FrameBound{0, kPreceding, true}; }
| INTVAL PRECEDING { $$ = new FrameBound{$1, kPreceding, false}; }
| UNBOUNDED FOLLOWING { $$ = new FrameBound{0, kFollowing, true}; }
| INTVAL FOLLOWING { $$ = new FrameBound{$1, kFollowing, false}; }
| CURRENT_ROW { $$ = new FrameBound{0, kCurrentRow, false}; };

extract_expr : EXTRACT '(' datetime_field FROM expr ')' { $$ = Expr::makeExtract($3, $5); };
// @sobolev cast_expr : CAST '(' expr AS column_type ')' { $$ = Expr::makeCast($3, $5); };
cast_expr : CAST '(' expr AS column_type_id_ ')' { $$ = Expr::makeCast($3, $5); }; // @sobolev

datetime_field : SECOND { $$ = kDatetimeSecond; }
| MINUTE { $$ = kDatetimeMinute; }
| HOUR { $$ = kDatetimeHour; }
| DAY { $$ = kDatetimeDay; }
| MONTH { $$ = kDatetimeMonth; }
| YEAR { $$ = kDatetimeYear; };

datetime_field_plural : SECONDS { $$ = kDatetimeSecond; }
| MINUTES { $$ = kDatetimeMinute; }
| HOURS { $$ = kDatetimeHour; }
| DAYS { $$ = kDatetimeDay; }
| MONTHS { $$ = kDatetimeMonth; }
| YEARS { $$ = kDatetimeYear; };

duration_field : datetime_field | datetime_field_plural;
array_expr : ARRAY '[' expr_list ']' { $$ = Expr::makeArray($3); };
array_index : operand '[' int_literal ']' { $$ = Expr::makeArrayIndex($1, $3->ival); };
between_expr : operand BETWEEN operand AND operand { $$ = Expr::makeBetween($1, $3, $5); };

column_name : IDENTIFIER { $$ = Expr::makeColumnRef($1); }
| IDENTIFIER '.' IDENTIFIER { $$ = Expr::makeColumnRef($1, $3); }
| '*' { $$ = Expr::makeStar(); }
| IDENTIFIER '.' '*' { $$ = Expr::makeStar($1); };

literal : string_literal | bool_literal | num_literal | null_literal | /* @sobolev date_literal |*/ interval_literal | param_expr;
string_literal : STRING { $$ = Expr::makeLiteral($1); };
bool_literal : TRUE { $$ = Expr::makeLiteral(true); } | FALSE { $$ = Expr::makeLiteral(false); };
num_literal : FLOATVAL { $$ = Expr::makeLiteral($1); } | int_literal;
int_literal : INTVAL { $$ = Expr::makeLiteral($1); };
null_literal : NULL { $$ = Expr::makeNullLiteral(); };

/* @sobolev
date_literal : "DATE" STRING { // @sobolev DATE-->"DATE"
  int day{0}, month{0}, year{0}, chars_parsed{0};
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if(sscanf($2, "%4d-%2d-%2d%n", &day, &month, &year, &chars_parsed) != 3 || $2[chars_parsed] != 0) {
    SAlloc::F($2);
    yyerror(&yyloc, result, scanner, "Found incorrect date format. Expected format: YYYY-MM-DD");
    YYERROR;
  }
  $$ = Expr::makeDateLiteral($2);
};*/

interval_literal : int_literal duration_field {
  $$ = Expr::makeIntervalLiteral($1->ival, $2);
  delete $1;
} | INTERVAL STRING datetime_field {
  int duration{0}, chars_parsed{0};
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if(sscanf($2, "%d%n", &duration, &chars_parsed) != 1 || $2[chars_parsed] != 0) {
    SAlloc::F($2);
    yyerror(&yyloc, result, scanner, "Found incorrect interval format. Expected format: INTEGER");
    YYERROR;
  }
  SAlloc::F($2);
  $$ = Expr::makeIntervalLiteral(duration, $3);
} | INTERVAL STRING {
  int duration{0}, chars_parsed{0};
  // 'seconds' and 'minutes' are the longest accepted interval qualifiers (7 chars) + null byte
  char unit_string[8];
  // If the whole string is parsed, chars_parsed points to the terminating null byte after the last character
  if(sscanf($2, "%d %7s%n", &duration, unit_string, &chars_parsed) != 2 || $2[chars_parsed] != 0) {
    SAlloc::F($2);
    yyerror(&yyloc, result, scanner, "Found incorrect interval format. Expected format: INTEGER INTERVAL_QUALIIFIER");
    YYERROR;
  }
  SAlloc::F($2);
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
  $$ = Expr::makeIntervalLiteral(duration, unit);
};

param_expr : '?' {
  $$ = Expr::makeParameter(yylloc.total_column);
  $$->ival2 = yyloc.param_list.size();
  yyloc.param_list.push_back($$);
};
//
// Table
//
table_ref : table_ref_atomic | table_ref_commalist ',' table_ref_atomic {
  $1->push_back($3);
  auto tbl = new TableRef(kTableCrossProduct);
  tbl->list = $1;
  $$ = tbl;
};

table_ref_atomic : nonjoin_table_ref_atomic | join_clause;

nonjoin_table_ref_atomic : table_ref_name | '(' select_statement ')' opt_table_alias {
  auto tbl = new TableRef(kTableSelect);
  tbl->select = $2;
  tbl->alias = $4;
  $$ = tbl;
};

table_ref_commalist : table_ref_atomic {
  $$ = new std::vector<TableRef*>();
  $$->push_back($1);
} | table_ref_commalist ',' table_ref_atomic {
  $1->push_back($3);
  $$ = $1;
};

table_ref_name : table_name opt_table_alias {
  auto tbl = new TableRef(kTableName);
  tbl->schema = $1.schema;
  tbl->name = $1.name;
  tbl->alias = $2;
  $$ = tbl;
};

table_ref_name_no_alias : table_name {
  $$ = new TableRef(kTableName);
  $$->schema = $1.schema;
  $$->name = $1.name;
};

table_name : IDENTIFIER {
  $$.schema = nullptr;
  $$.name = $1;
} | IDENTIFIER '.' IDENTIFIER {
  $$.schema = $1;
  $$.name = $3;
};

opt_index_name : IDENTIFIER { $$ = $1; } | /* empty */ { $$ = nullptr; };
table_alias : alias | AS IDENTIFIER '(' ident_commalist ')' { $$ = new Alias($2, $4); };
opt_table_alias : table_alias | /* empty */ { $$ = nullptr; };
alias : AS IDENTIFIER { $$ = new Alias($2); } | IDENTIFIER { $$ = new Alias($1); };
opt_alias : alias | /* empty */ { $$ = nullptr; };
//
// Row Locking Descriptions
//
opt_locking_clause : opt_locking_clause_list { $$ = $1; }
| /* empty */ { $$ = nullptr; };

opt_locking_clause_list : locking_clause {
  $$ = new std::vector<LockingClause*>();
  $$->push_back($1);
} | opt_locking_clause_list locking_clause {
  $1->push_back($2);
  $$ = $1;
};

locking_clause : FOR row_lock_mode opt_row_lock_policy {
  $$ = new LockingClause();
  $$->rowLockMode = $2;
  $$->rowLockWaitPolicy = $3;
  $$->tables = nullptr;
} | FOR row_lock_mode OF ident_commalist opt_row_lock_policy {
  $$ = new LockingClause();
  $$->rowLockMode = $2;
  $$->tables = $4;
  $$->rowLockWaitPolicy = $5;
};

row_lock_mode : UPDATE { $$ = RowLockMode::ForUpdate; }
| NO KEY UPDATE { $$ = RowLockMode::ForNoKeyUpdate; }
| SHARE { $$ = RowLockMode::ForShare; }
| KEY SHARE { $$ = RowLockMode::ForKeyShare; };

opt_row_lock_policy : SKIP LOCKED { $$ = RowLockWaitPolicy::SkipLocked; }
| NOWAIT { $$ = RowLockWaitPolicy::NoWait; }
| /* empty */ { $$ = RowLockWaitPolicy::None; };
//
// With Descriptions
//
opt_with_clause : with_clause | /* empty */ { $$ = nullptr; };

with_clause : WITH with_description_list { $$ = $2; };

with_description_list : with_description {
  $$ = new std::vector<WithDescription*>();
  $$->push_back($1);
}
| with_description_list ',' with_description {
  $1->push_back($3);
  $$ = $1;
};

with_description : IDENTIFIER AS select_with_paren {
  $$ = new WithDescription();
  $$->alias = $1;
  $$->select = $3;
};
//
// Join Statements
//
join_clause : table_ref_atomic NATURAL JOIN nonjoin_table_ref_atomic {
  $$ = new TableRef(kTableJoin);
  $$->join = new JoinDefinition();
  $$->join->type = kJoinNatural;
  $$->join->left = $1;
  $$->join->right = $4;
} | table_ref_atomic opt_join_type JOIN table_ref_atomic ON join_condition {
  $$ = new TableRef(kTableJoin);
  $$->join = new JoinDefinition();
  $$->join->type = (JoinType)$2;
  $$->join->left = $1;
  $$->join->right = $4;
  $$->join->condition = $6;
} | table_ref_atomic opt_join_type JOIN table_ref_atomic USING '(' column_name ')' {
  $$ = new TableRef(kTableJoin);
  $$->join = new JoinDefinition();
  $$->join->type = (JoinType)$2;
  $$->join->left = $1;
  $$->join->right = $4;
  auto left_col = Expr::makeColumnRef(_strdup($7->name));
  if($7->alias) {
    left_col->alias = _strdup($7->alias);
  }
  if($1->getName()) {
    left_col->table = _strdup($1->getName());
  }
  auto right_col = Expr::makeColumnRef(_strdup($7->name));
  if($7->alias) {
    right_col->alias = _strdup($7->alias);
  }
  if($4->getName()) {
    right_col->table = _strdup($4->getName());
  }
  $$->join->condition = Expr::makeOpBinary(left_col, kOpEquals, right_col);
  delete $7;
};

opt_join_type : INNER { $$ = kJoinInner; }
| LEFT OUTER { $$ = kJoinLeft; }
| LEFT { $$ = kJoinLeft; }
| RIGHT OUTER { $$ = kJoinRight; }
| RIGHT { $$ = kJoinRight; }
| FULL OUTER { $$ = kJoinFull; }
| OUTER { $$ = kJoinFull; }
| FULL { $$ = kJoinFull; }
| CROSS { $$ = kJoinCross; }
| /* empty, default */ { $$ = kJoinInner; };

join_condition : expr;
//
// Misc
//
opt_semicolon : ';' | /* empty */;

ident_commalist : IDENTIFIER {
  $$ = new std::vector<char*>();
  $$->push_back($1);
} | ident_commalist ',' IDENTIFIER {
  $1->push_back($3);
  $$ = $1;
};

index_segment_flags : index_segment_flags ASC {
    $$ &= ~IndexSegment::fDesc;
} | index_segment_flags DESC {
    $$ |= IndexSegment::fDesc;
} | /*empty*/ {
    $$ = 0;
}

index_segment : IDENTIFIER index_segment_flags {
    $$ = new IndexSegment;
    $$->Field = $1;
    ZDELETE($1);
    $$->Flags = static_cast<uint>($2);
}

index_segment_list : index_segment {
    $$ = new Index;
    $$->insert($1);
} | index_segment_list ',' index_segment {
    $$->insert($3);
};

// clang-format off
%%
// clang-format on
//
// Section 4: Additional C code
//
    /* empty */
