// test-sqlparser.cpp
// Copyright (c) A.Sobolev 2024
//
#include <pp.h>
#pragma hdrstop
#include "sql-parser.h"
#include "parser\bison_parser.h"

using namespace hsql;
using hsql::kExprLiteralInt;
using hsql::kExprParameter;
using hsql::kStmtDrop;
using hsql::kStmtExecute;
using hsql::kStmtInsert;
using hsql::kStmtPrepare;
using hsql::kStmtSelect;
using hsql::kDropPreparedStatement;
using hsql::DropStatement;
using hsql::ExecuteStatement;
using hsql::InsertStatement;
using hsql::PrepareStatement;
using hsql::SelectStatement;

#define TEST_PARSE_SQL_QUERY(query, result, numStatements) \
	hsql::SQLParserResult result;                            \
	hsql::SQLParser::parse(query, &result);                  \
	SLCHECK_NZ(result.isValid());                                \
	SLCHECK_EQ(result.size(), (uint)numStatements);

#define TEST_PARSE_SINGLE_SQL(query, stmtType, stmtClass, result, outputVar) \
	TEST_PARSE_SQL_QUERY(query, result, 1);                                    \
	SLCHECK_NZ(result.getStatement(0)->type() == stmtType);                       \
	const stmtClass* outputVar = (const stmtClass*)result.getStatement(0);

#define TEST_CAST_STMT(result, stmt_index, stmtType, stmtClass, outputVar) \
	SLCHECK_EQ(result.getStatement(stmt_index)->type(), stmtType);            \
	const stmtClass* outputVar = (const stmtClass*)result.getStatement(stmt_index);

static SQLParserResult parse_and_move(std::string query)
{
	hsql::SQLParserResult result;
	hsql::SQLParser::parse(query, &result);
	// Moves on return.
	return result;
}
	
static SQLParserResult move_in_and_back(SQLParserResult res) 
{
	// Moves on return.
	return res;
}

static bool test_tokens(const std::string& query, const std::vector<int16_t>& expected_tokens) 
{
	std::vector<int16_t> tokens;
	if(!SQLParser::tokenize(query, &tokens))
		return false;
	if(expected_tokens.size() != tokens.size())
		return false;
	for(uint i = 0; i < expected_tokens.size(); ++i) {
		if(!(expected_tokens[i] == tokens[i]))
			return false;
	}
	return true;
}

SLTEST_R(sql_parser)
{
	{ //TEST(DeleteStatementTest) 
		SQLParserResult result;
		SQLParser::parse("DELETE FROM students WHERE grade > 2.0;", &result);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		SLCHECK_NZ(result.getStatement(0)->type() == kStmtDelete);
		const DeleteStatement* stmt = (const DeleteStatement*)result.getStatement(0);
		SLCHECK_NZ(sstreq(stmt->tableName, "students"));
		SLCHECK_NZ(stmt->expr);
		SLCHECK_NZ(stmt->expr->isType(kExprOperator));
		SLCHECK_NZ(sstreq(stmt->expr->expr->name, "grade"));
		SLCHECK_EQ(stmt->expr->expr2->fval, 2.0);
	}
	{ // TEST(CreateStatementTest) 
		SQLParserResult result;
		SQLParser::parse(
			"CREATE TABLE dummy_table ("
			"  c_bigint BIGINT, \n"
			"\n/* some \n comment */\n"
			"  c_boolean BOOLEAN, "
			"  c_char CHAR(42), "
			"  c_date DATE, "
			"  c_datetime DATETIME, "
			"  c_decimal DECIMAL, "
			"  c_decimal_precision DECIMAL(13), "
			"  c_decimal_precision_scale DECIMAL(13,37), "
			"  c_double_not_null DOUBLE NOT NULL, "
			"  c_float FLOAT, "
			"  c_int INT, "
			"  PRIMARY KEY(c_char, c_int), "
			"  c_integer_null INTEGER NULL, "
			"  c_long LONG, "
			"  c_real REAL, "
			"  c_smallint SMALLINT, "
			"  c_text TEXT UNIQUE PRIMARY KEY NOT NULL, "
			"  c_time TIME, "
			"  c_time_precision TIME(17), "
			"  c_timestamp TIMESTAMP, "
			"  c_varchar VARCHAR(50), "
			"  c_char_varying CHARACTER VARYING(60)"
			")",
			&result);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		SLCHECK_EQ(result.getStatement(0)->type(), kStmtCreate);

		const CreateStatement* stmt = (const CreateStatement*)result.getStatement(0);
		SLCHECK_EQ(stmt->type, kCreateTable);
		SLCHECK_NZ(sstreq(stmt->tableName, "dummy_table"));
		SLCHECK_NZ(stmt->columns);
		SLCHECK_EQ(stmt->columns->size(), 21U);
		// c_bigint BIGINT
		SLCHECK_NZ(sstreq(stmt->columns->at(0)->name, "c_bigint"));
		SLCHECK_NZ(stmt->columns->at(0)->type == (ColumnType{hsql::DataType::BIGINT}));
		SLCHECK_EQ(stmt->columns->at(0)->nullable, true);
		// c_boolean BOOLEAN
		SLCHECK_NZ(sstreq(stmt->columns->at(1)->name, "c_boolean"));
		SLCHECK_NZ(stmt->columns->at(1)->type == (ColumnType{hsql::DataType::BOOLEAN}));
		SLCHECK_EQ(stmt->columns->at(1)->nullable, true);
		// c_char CHAR(42)
		SLCHECK_NZ(sstreq(stmt->columns->at(2)->name, "c_char"));
		SLCHECK_NZ(stmt->columns->at(2)->type == (ColumnType{hsql::DataType::CHAR, 42}));
		SLCHECK_NZ(stmt->columns->at(2)->type != (ColumnType{hsql::DataType::CHAR, 43}));
		SLCHECK_EQ(stmt->columns->at(2)->nullable, true);
		// c_date DATE
		SLCHECK_NZ(sstreq(stmt->columns->at(3)->name, "c_date"));
		SLCHECK_NZ(stmt->columns->at(3)->type == (ColumnType{hsql::DataType::DATE}));
		SLCHECK_EQ(stmt->columns->at(3)->nullable, true);
		// c_datetime DATETIME
		SLCHECK_NZ(sstreq(stmt->columns->at(4)->name, "c_datetime"));
		SLCHECK_NZ(stmt->columns->at(4)->type == (ColumnType{hsql::DataType::DATETIME}));
		SLCHECK_EQ(stmt->columns->at(4)->nullable, true);
		// c_decimal DECIMAL
		SLCHECK_NZ(sstreq(stmt->columns->at(5)->name, "c_decimal"));
		SLCHECK_NZ(stmt->columns->at(5)->type == (ColumnType{hsql::DataType::DECIMAL}));
		SLCHECK_EQ(stmt->columns->at(5)->nullable, true);
		// c_decimal_precision DECIMAL(13)
		SLCHECK_NZ(sstreq(stmt->columns->at(6)->name, "c_decimal_precision"));
		SLCHECK_NZ(stmt->columns->at(6)->type == (ColumnType{hsql::DataType::DECIMAL, 0, 13}));
		SLCHECK_NZ(stmt->columns->at(6)->type != (ColumnType{hsql::DataType::DECIMAL, 0, 14}));
		SLCHECK_NZ(stmt->columns->at(6)->type != (ColumnType{hsql::DataType::DECIMAL, 1, 13}));
		SLCHECK_EQ(stmt->columns->at(6)->nullable, true);
		// c_decimal_precision_scale DECIMAL(13,37)
		SLCHECK_NZ(sstreq(stmt->columns->at(7)->name, "c_decimal_precision_scale"));
		SLCHECK_NZ(stmt->columns->at(7)->type == (ColumnType{hsql::DataType::DECIMAL, 0, 13, 37}));
		SLCHECK_NZ(stmt->columns->at(7)->type != (ColumnType{hsql::DataType::DECIMAL, 0, 14, 37}));
		SLCHECK_NZ(stmt->columns->at(7)->type != (ColumnType{hsql::DataType::DECIMAL, 0, 13, 38}));
		SLCHECK_NZ(stmt->columns->at(7)->type != (ColumnType{hsql::DataType::DECIMAL, 1, 13, 37}));
		SLCHECK_EQ(stmt->columns->at(7)->nullable, true);
		// c_double_not_null DOUBLE NOT NULL
		SLCHECK_NZ(sstreq(stmt->columns->at(8)->name, "c_double_not_null"));
		SLCHECK_NZ(stmt->columns->at(8)->type == (ColumnType{hsql::DataType::DOUBLE}));
		SLCHECK_EQ(stmt->columns->at(8)->nullable, false);
		SLCHECK_EQ(stmt->columns->at(8)->P_Constraints->size(), 1U);
		SLCHECK_NZ(stmt->columns->at(8)->P_Constraints->count(ConstraintType::NotNull));
		// c_float FLOAT
		SLCHECK_NZ(sstreq(stmt->columns->at(9)->name, "c_float"));
		SLCHECK_NZ(stmt->columns->at(9)->type == (ColumnType{hsql::DataType::FLOAT}));
		SLCHECK_EQ(stmt->columns->at(9)->nullable, true);
		// c_int INT
		SLCHECK_NZ(sstreq(stmt->columns->at(10)->name, "c_int"));
		SLCHECK_NZ(stmt->columns->at(10)->type == (ColumnType{hsql::DataType::INT}));
		SLCHECK_EQ(stmt->columns->at(10)->nullable, true);
		// c_integer INTEGER NULL
		SLCHECK_NZ(sstreq(stmt->columns->at(11)->name, "c_integer_null"));
		SLCHECK_NZ(stmt->columns->at(11)->type == (ColumnType{hsql::DataType::INT}));
		SLCHECK_EQ(stmt->columns->at(11)->nullable, true);
		SLCHECK_EQ(stmt->columns->at(11)->P_Constraints->size(), 1U);
		SLCHECK_NZ(stmt->columns->at(11)->P_Constraints->count(ConstraintType::Null));
		// c_long LONG
		SLCHECK_NZ(sstreq(stmt->columns->at(12)->name, "c_long"));
		SLCHECK_NZ(stmt->columns->at(12)->type == (ColumnType{hsql::DataType::LONG}));
		SLCHECK_EQ(stmt->columns->at(12)->nullable, true);
		// c_real REAL
		SLCHECK_NZ(sstreq(stmt->columns->at(13)->name, "c_real"));
		SLCHECK_NZ(stmt->columns->at(13)->type == (ColumnType{hsql::DataType::REAL}));
		SLCHECK_EQ(stmt->columns->at(13)->nullable, true);
		// c_smallint SMALLINT
		SLCHECK_NZ(sstreq(stmt->columns->at(14)->name, "c_smallint"));
		SLCHECK_NZ(stmt->columns->at(14)->type == (ColumnType{hsql::DataType::SMALLINT}));
		SLCHECK_EQ(stmt->columns->at(14)->nullable, true);
		// c_text TEXT UNIQUE PRIMARY KEY NOT NULL
		SLCHECK_NZ(sstreq(stmt->columns->at(15)->name, "c_text"));
		SLCHECK_NZ(stmt->columns->at(15)->type == (ColumnType{hsql::DataType::TEXT}));
		SLCHECK_EQ(stmt->columns->at(15)->nullable, false);
		// Expecting two elements in P_Constraints since information about NULL constraints is separately stored in
		// ColumnDefinition::nullable
		SLCHECK_EQ(stmt->columns->at(15)->P_Constraints->size(), 3U);
		SLCHECK_NZ(stmt->columns->at(15)->P_Constraints->count(ConstraintType::Unique));
		SLCHECK_NZ(stmt->columns->at(15)->P_Constraints->count(ConstraintType::PrimaryKey));
		SLCHECK_NZ(stmt->columns->at(15)->P_Constraints->count(ConstraintType::NotNull));
		// c_time TIME
		SLCHECK_NZ(sstreq(stmt->columns->at(16)->name, "c_time"));
		SLCHECK_NZ(stmt->columns->at(16)->type == (ColumnType{hsql::DataType::TIME}));
		SLCHECK_EQ(stmt->columns->at(16)->nullable, true);
		// c_time_precision TIME(17)
		SLCHECK_NZ(sstreq(stmt->columns->at(17)->name, "c_time_precision"));
		SLCHECK_NZ(stmt->columns->at(17)->type == (ColumnType{hsql::DataType::TIME, 0, 17}));
		SLCHECK_NZ(stmt->columns->at(17)->type != (ColumnType{hsql::DataType::TIME, 0, 18}));
		SLCHECK_NZ(stmt->columns->at(17)->type != (ColumnType{hsql::DataType::TIME, 1, 17}));
		SLCHECK_EQ(stmt->columns->at(17)->nullable, true);
		// c_timestamp TIMESTAMP
		SLCHECK_NZ(sstreq(stmt->columns->at(18)->name, "c_timestamp"));
		SLCHECK_NZ(stmt->columns->at(18)->type == (ColumnType{hsql::DataType::DATETIME}));
		SLCHECK_EQ(stmt->columns->at(18)->nullable, true);
		// c_varchar VARCHAR(50)
		SLCHECK_NZ(sstreq(stmt->columns->at(19)->name, "c_varchar"));
		SLCHECK_NZ(stmt->columns->at(19)->type == (ColumnType{hsql::DataType::VARCHAR, 50}));
		SLCHECK_NZ(stmt->columns->at(19)->type != (ColumnType{hsql::DataType::VARCHAR, 51}));
		SLCHECK_EQ(stmt->columns->at(19)->nullable, true);
		// c_char_varying CHARACTER VARYING(60)
		SLCHECK_NZ(sstreq(stmt->columns->at(20)->name, "c_char_varying"));
		SLCHECK_NZ(stmt->columns->at(20)->type == (ColumnType{hsql::DataType::VARCHAR, 60}));
		SLCHECK_NZ(stmt->columns->at(20)->type != (ColumnType{hsql::DataType::VARCHAR, 61}));
		// Table constraints are identified and separated during the parsing of the SQL string
		// Table constraints:
		// - PRIMARY KEY(c_char, c_int)
		SLCHECK_EQ(stmt->tableConstraints->size(), 1U);
		SLCHECK_NZ(stmt->tableConstraints->at(0)->type == ConstraintType::PrimaryKey);
		SLCHECK_NZ(sstreq(stmt->tableConstraints->at(0)->columnNames->at(0), "c_char"));
		SLCHECK_NZ(sstreq(stmt->tableConstraints->at(0)->columnNames->at(1), "c_int"));
	}
	{ // TEST(CreateAsSelectStatementTest)
		SQLParserResult result;
		SQLParser::parse("CREATE TABLE students_2 AS SELECT student_number, grade FROM students", &result);

		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		SLCHECK_EQ(result.getStatement(0)->type(), kStmtCreate);

		const CreateStatement* stmt = (const CreateStatement*)result.getStatement(0);
		SLCHECK_EQ(stmt->type, kCreateTable);
		SLCHECK_NZ(sstreq(stmt->tableName, "students_2"));
		SLCHECK_Z(stmt->columns);
		SLCHECK_NZ(stmt->select);
		SLCHECK_NZ(stmt->select->selectList->at(0)->isType(kExprColumnRef));
		SLCHECK_NZ(sstreq(stmt->select->selectList->at(0)->getName(), "student_number"));
		SLCHECK_NZ(stmt->select->selectList->at(1)->isType(kExprColumnRef));
		SLCHECK_NZ(sstreq(stmt->select->selectList->at(1)->getName(), "grade"));
	}
	{ // TEST(UpdateStatementTest)
		SQLParserResult result;
		SQLParser::parse("UPDATE students SET grade = 5.0, name = 'test' WHERE name = 'Max O''Mustermann';", &result);

		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		SLCHECK_EQ(result.getStatement(0)->type(), kStmtUpdate);

		const UpdateStatement* stmt = (const UpdateStatement*)result.getStatement(0);
		SLCHECK_NZ(stmt->table);
		SLCHECK_NZ(sstreq(stmt->table->name, "students"));

		SLCHECK_NZ(stmt->updates);
		SLCHECK_EQ(stmt->updates->size(), 2U);
		SLCHECK_NZ(sstreq(stmt->updates->at(0)->column, "grade"));
		SLCHECK_NZ(sstreq(stmt->updates->at(1)->column, "name"));
		SLCHECK_NZ(stmt->updates->at(0)->value->isType(kExprLiteralFloat));
		SLCHECK_NZ(stmt->updates->at(1)->value->isType(kExprLiteralString));
		SLCHECK_EQ(stmt->updates->at(0)->value->fval, 5.0);
		SLCHECK_NZ(sstreq(stmt->updates->at(1)->value->name, "test"));

		SLCHECK_NZ(stmt->where);
		SLCHECK_NZ(stmt->where->isType(kExprOperator));
		SLCHECK_EQ(stmt->where->opType, kOpEquals);
		SLCHECK_NZ(sstreq(stmt->where->expr->name, "name"));
		SLCHECK_NZ(sstreq(stmt->where->expr2->name, "Max O'Mustermann"));
	}
	{
		const char * p_stmt = "INSERT INTO `tourist` (`id`, `name`, `passport`, `birthday`, `phone`, `icq`, `email`, `address`, `web`, `trafficSource`, `tracking`, `dataReg`, `dateUpdate`, `status`, `category`, `covid`, `idAmo`, `gender`, `nationality`) VALUES"
		" (1,	'любовь',	'',	NULL,	79214245933,	NULL,	NULL,	'',	NULL,	NULL,	NULL,	'2021-06-28 17:53:55',	'2021-08-27 09:01:24',	0,	0,	NULL,	12694183,	0,	NULL);";
		TEST_PARSE_SINGLE_SQL(p_stmt, kStmtInsert, InsertStatement, result, stmt);
	}
	{ // TEST(InsertStatementTest)
		TEST_PARSE_SINGLE_SQL("INSERT INTO `students` (`dateUpdate`, `int_value`, `string-value`, `real-vaLUE`) VALUES ('Max Mustermann', 12345, 'Musterhausen', 2.0)", kStmtInsert, InsertStatement, result, stmt);
		SLCHECK_EQ(stmt->values->size(), 4U);
		// TODO
	}
	{ // TEST(AlterStatementDropActionTest)
		SQLParserResult result;
		SQLParser::parse("ALTER TABLE mytable DROP COLUMN IF EXISTS mycolumn", &result);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		const AlterStatement* stmt = (const AlterStatement*)result.getStatement(0);
		SLCHECK_NZ(sstreq(stmt->name, "mytable"));
		SLCHECK_EQ(stmt->ifTableExists, false);
		auto dropAction = (const DropColumnAction*)stmt->action;
		SLCHECK_EQ(dropAction->type, hsql::ActionType::DropColumn);
		SLCHECK_NZ(sstreq(dropAction->columnName, "mycolumn"));
	}
	{ // TEST(CreateIndexStatementTest)
		SQLParserResult result;
		SQLParser::parse("CREATE INDEX myindex ON myTable (col1);", &result);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);

		const CreateStatement* stmt = (const CreateStatement*)result.getStatement(0);
		SLCHECK_NZ(sstreq(stmt->indexName, "myindex"));
		SLCHECK_NZ(sstreq(stmt->tableName, "myTable"));
		SLCHECK_EQ(stmt->type, kCreateIndex);
		SLCHECK_EQ(stmt->ifNotExists, false);
		//SLCHECK_EQ(stmt->indexColumns->size(), 1U);
		SLCHECK_EQ(stmt->P_Idx->getCount(), 1U);
	}
	{ // TEST(CreateIndexStatementIfNotExistsTest) 
		SQLParserResult result;
		SQLParser::parse("CREATE INDEX IF NOT EXISTS myindex ON myTable (col1 DESC, col2 asc);", &result);
		SLCHECK_NZ(result.isValid());
		if(result.isValid()) {
			SLCHECK_EQ(result.size(), 1U);
			const CreateStatement* stmt = (const CreateStatement*)result.getStatement(0);
			SLCHECK_NZ(sstreq(stmt->indexName, "myindex"));
			SLCHECK_NZ(sstreq(stmt->tableName, "myTable"));
			SLCHECK_EQ(stmt->type, kCreateIndex);
			SLCHECK_EQ(stmt->ifNotExists, true);
			//SLCHECK_EQ(stmt->indexColumns->size(), 2U);
			SLCHECK_EQ(stmt->P_Idx->getCount(), 2U);
			if(stmt->P_Idx->getCount() == 2) {
				SLCHECK_NZ(stmt->P_Idx->at(0)->Flags & hsql::IndexSegment::fDesc);
				SLCHECK_Z(stmt->P_Idx->at(1)->Flags & hsql::IndexSegment::fDesc);
				SLCHECK_NZ(stmt->P_Idx->at(1)->Field.IsEqiAscii("col2"));
			}
		}
	}
	{ // TEST(DropIndexTest) 
		SQLParserResult result;
		SQLParser::parse("DROP INDEX myindex", &result);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		const DropStatement* stmt = (const DropStatement*)result.getStatement(0);
		SLCHECK_NZ(sstreq(stmt->indexName, "myindex"));
		SLCHECK_EQ(stmt->ifExists, false);
	}
	{ // TEST(DropIndexIfExistsTest)
		SQLParserResult result;
		SQLParser::parse("DROP INDEX IF EXISTS myindex", &result);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(result.size(), 1U);
		const DropStatement* stmt = (const DropStatement*)result.getStatement(0);
		SLCHECK_NZ(sstreq(stmt->indexName, "myindex"));
		SLCHECK_EQ(stmt->ifExists, true);
	}
	{ // TEST(DropTableStatementTest)
		TEST_PARSE_SINGLE_SQL("DROP TABLE students", kStmtDrop, DropStatement, result, stmt);
		SLCHECK_Z(stmt->ifExists);
		SLCHECK_EQ(stmt->type, kDropTable);
		SLCHECK_NZ(stmt->name);
		SLCHECK_NZ(sstreq(stmt->name, "students"));
	}
	{ // TEST(DropTableIfExistsStatementTest)
		TEST_PARSE_SINGLE_SQL("DROP TABLE IF EXISTS students", kStmtDrop, DropStatement, result, stmt);
		SLCHECK_NZ(stmt->ifExists);
		SLCHECK_EQ(stmt->type, kDropTable);
		SLCHECK_NZ(stmt->name);
		SLCHECK_NZ(sstreq(stmt->name, "students"));
	}
	{ // TEST(ReleaseStatementTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students;", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_EQ(1U, result.size());
		SLCHECK_Z(stmt->whereClause);
		std::vector<SQLStatement*> statements = result.releaseStatements();
		SLCHECK_EQ(0U, result.size());
		for(SQLStatement* stmt : statements) {
			delete stmt;
		}
	}
	{ // TEST(ShowTableStatementTest)
		TEST_PARSE_SINGLE_SQL("SHOW TABLES;", kStmtShow, ShowStatement, result, stmt);
		SLCHECK_EQ(stmt->type, kShowTables);
		SLCHECK_Z(stmt->name);
	}
	{ // TEST(ShowColumnsStatementTest)
		TEST_PARSE_SINGLE_SQL("SHOW COLUMNS students;", kStmtShow, ShowStatement, result, stmt);
		SLCHECK_EQ(stmt->type, kShowColumns);
		SLCHECK_NZ(stmt->name);
		SLCHECK_NZ(sstreq(stmt->name, "students"));
	}
	{ // TEST(DescribeStatementTest)
		TEST_PARSE_SINGLE_SQL("DESCRIBE students;", kStmtShow, ShowStatement, result, stmt);
		SLCHECK_EQ(stmt->type, kShowColumns);
		SLCHECK_NZ(stmt->name);
		SLCHECK_NZ(sstreq(stmt->name, "students"));
	}
	{ // TEST(ImportStatementTest)
		TEST_PARSE_SINGLE_SQL("IMPORT FROM TBL FILE 'students_file' INTO students;", kStmtImport, ImportStatement, result, stmt);
		SLCHECK_EQ(stmt->type, kImportTbl);
		SLCHECK_NZ(stmt->tableName);
		SLCHECK_NZ(sstreq(stmt->tableName, "students"));
		SLCHECK_NZ(sstreq(stmt->filePath, "students_file"));
	}
	{ // TEST(CopyStatementTest)
		TEST_PARSE_SINGLE_SQL("COPY students FROM 'students_file' WITH FORMAT BINARY;", kStmtImport, ImportStatement, import_result, import_stmt);
		SLCHECK_EQ(import_stmt->type, kImportBinary);
		SLCHECK_NZ(import_stmt->tableName);
		SLCHECK_NZ(sstreq(import_stmt->tableName, "students"));
		SLCHECK_NZ(import_stmt->filePath);
		SLCHECK_NZ(sstreq(import_stmt->filePath, "students_file"));
		SLCHECK_Z(import_stmt->whereClause);
		TEST_PARSE_SINGLE_SQL("COPY students FROM 'students_file' WHERE lastname = 'Potter';", kStmtImport, ImportStatement,
			import_filter_result, import_filter_stmt);
		SLCHECK_EQ(import_filter_stmt->type, kImportAuto);
		SLCHECK_NZ(import_filter_stmt->tableName);
		SLCHECK_NZ(sstreq(import_filter_stmt->tableName, "students"));
		SLCHECK_NZ(import_filter_stmt->filePath);
		SLCHECK_NZ(sstreq(import_filter_stmt->filePath, "students_file"));
		SLCHECK_NZ(import_filter_stmt->whereClause);
		SLCHECK_EQ(import_filter_stmt->whereClause->opType, kOpEquals);
		SLCHECK_EQ(import_filter_stmt->whereClause->expr->type, kExprColumnRef);
		SLCHECK_NZ(sstreq(import_filter_stmt->whereClause->expr->name, "lastname"));
		SLCHECK_EQ(import_filter_stmt->whereClause->expr2->type, kExprLiteralString);
		SLCHECK_NZ(sstreq(import_filter_stmt->whereClause->expr2->name, "Potter"));
		TEST_PARSE_SINGLE_SQL("COPY students TO 'students_file' WITH FORMAT CSV;", kStmtExport, ExportStatement,
			export_table_result, export_table_stmt);

		SLCHECK_EQ(export_table_stmt->type, kImportCSV);
		SLCHECK_NZ(export_table_stmt->tableName);
		SLCHECK_NZ(sstreq(export_table_stmt->tableName, "students"));
		SLCHECK_NZ(export_table_stmt->filePath);
		SLCHECK_NZ(sstreq(export_table_stmt->filePath, "students_file"));
		SLCHECK_Z(export_table_stmt->select);

		TEST_PARSE_SINGLE_SQL("COPY (SELECT firstname, lastname FROM students) TO 'students_file';", kStmtExport,
			ExportStatement, export_select_result, export_select_stmt);

		SLCHECK_EQ(export_select_stmt->type, kImportAuto);
		SLCHECK_Z(export_select_stmt->tableName);
		SLCHECK_NZ(export_select_stmt->filePath);
		SLCHECK_NZ(sstreq(export_select_stmt->filePath, "students_file"));

		SLCHECK_NZ(export_select_stmt->select);
		const auto& select_stmt = export_select_stmt->select;
		SLCHECK_Z(select_stmt->whereClause);
		SLCHECK_Z(select_stmt->groupBy);
		SLCHECK_EQ(select_stmt->selectList->size(), 2U);
		SLCHECK_NZ(select_stmt->selectList->at(0)->isType(kExprColumnRef));
		SLCHECK_NZ(sstreq(select_stmt->selectList->at(0)->getName(), "firstname"));
		SLCHECK_NZ(select_stmt->selectList->at(1)->isType(kExprColumnRef));
		SLCHECK_NZ(sstreq(select_stmt->selectList->at(1)->getName(), "lastname"));
		SLCHECK_NZ(select_stmt->fromTable);
		SLCHECK_NZ(sstreq(select_stmt->fromTable->name, "students"));
	}
	{ // TEST(MoveSQLResultTest)
		SQLParserResult res = parse_and_move("SELECT * FROM test;");
		SLCHECK_NZ(res.isValid());
		SLCHECK_EQ(1U, res.size());

		// Moved around.
		SQLParserResult new_res = move_in_and_back(std::move(res));

		// Original object should be invalid.
		SLCHECK_Z(res.isValid());
		SLCHECK_EQ(0U, res.size());

		SLCHECK_NZ(new_res.isValid());
		SLCHECK_EQ(1U, new_res.size());
	}
	{ // TEST(HintTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students WITH HINT(NO_CACHE, SAMPLE_RATE(10));", kStmtSelect, SelectStatement,
			result, stmt);
		SLCHECK_NZ(stmt->hints);
		SLCHECK_EQ(2U, stmt->hints->size());
		SLCHECK_NZ(sstreq("NO_CACHE", stmt->hints->at(0)->name));
		SLCHECK_NZ(sstreq("SAMPLE_RATE", stmt->hints->at(1)->name));
		SLCHECK_EQ(1U, stmt->hints->at(1)->exprList->size());
		SLCHECK_EQ(10LL, stmt->hints->at(1)->exprList->at(0)->ival);
	}
	{ // TEST(StringLengthTest)
		TEST_PARSE_SQL_QUERY("SELECT * FROM bar; INSERT INTO foo VALUES (4);\t\n SELECT * FROM foo;", result, 3);
		SLCHECK_EQ(result.getStatement(0)->stringLength, 18U);
		SLCHECK_EQ(result.getStatement(1)->stringLength, 28U);
		SLCHECK_EQ(result.getStatement(2)->stringLength, 21U);
	}
	{ // TEST(ExceptOperatorTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students EXCEPT SELECT * FROM students_2;", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "students_2"));
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "students"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetExcept);
	}
	{ // TEST(IntersectOperatorTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students INTERSECT SELECT * FROM students_2;", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "students_2"));
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "students"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetIntersect);
	}
	{ // TEST(UnionOperatorTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students UNION SELECT * FROM students_2;", kStmtSelect, SelectStatement, result,stmt);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "students_2"));
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "students"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetUnion);
		SLCHECK_Z(stmt->setOperations->back()->isAll);
	}
	{ // TEST(UnionAllOperatorTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students UNION ALL SELECT * FROM students_2;", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "students_2"));
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "students"));
		SLCHECK_NZ(stmt->setOperations->back()->isAll);
	}
	{ // TEST(NestedSetOperationTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students INTERSECT SELECT grade FROM students_2 UNION SELECT * FROM employees;",
			kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->setOperations->back()->nestedSelectStatement->fromTable->name, "employees"));
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "students_2"));
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "students"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetIntersect);
		SLCHECK_EQ(stmt->setOperations->back()->nestedSelectStatement->setOperations->back()->setType, kSetUnion);
		SLCHECK_Z(stmt->setOperations->back()->isAll);
	}
	{ // TEST(OrderByFullStatementTest)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students INTERSECT SELECT grade FROM students_2 UNION SELECT * FROM employees ORDER BY grade ASC;",
			kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_EQ(stmt->setOperations->back()->resultOrder->at(0)->type, kOrderAsc);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->resultOrder->at(0)->expr->name, "grade"));
		SLCHECK_Z(stmt->setOperations->back()->isAll);
	}
	{ // TEST(SetOperationSubQueryOrder)
		TEST_PARSE_SINGLE_SQL("(SELECT * FROM students ORDER BY name DESC) INTERSECT SELECT grade FROM students_2 UNION SELECT * FROM "
			"employees ORDER BY grade ASC;", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_EQ(stmt->order->at(0)->type, kOrderDesc);
		SLCHECK_NZ(sstreq(stmt->order->at(0)->expr->name, "name"));

		SLCHECK_EQ(stmt->setOperations->back()->resultOrder->at(0)->type, kOrderAsc);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->resultOrder->at(0)->expr->name, "grade"));
		SLCHECK_Z(stmt->setOperations->back()->isAll);
	}
	{ // TEST(SetOperationLastSubQueryOrder) 
		TEST_PARSE_SINGLE_SQL("SELECT * FROM students INTERSECT SELECT grade FROM students_2 UNION (SELECT * FROM employees ORDER BY name "
			"DESC) ORDER BY grade ASC;", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_EQ(stmt->setOperations->back()->nestedSelectStatement->setOperations->back()->nestedSelectStatement->order->at(0)->type, kOrderDesc);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->setOperations->back()->nestedSelectStatement->order->at(0)->expr->name, "name"));
		SLCHECK_EQ(stmt->setOperations->back()->resultOrder->at(0)->type, kOrderAsc);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->resultOrder->at(0)->expr->name, "grade"));
		SLCHECK_Z(stmt->setOperations->back()->isAll);
	}
	{ // TEST(NestedDifferentSetOperationsWithWithClause) 
		TEST_PARSE_SINGLE_SQL("WITH UNION_FIRST AS (SELECT * FROM A UNION SELECT * FROM B) SELECT * FROM UNION_FIRST EXCEPT SELECT * FROM C",
			kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->alias, "UNION_FIRST"));
		SLCHECK_EQ(stmt->withDescriptions->back()->select->setOperations->back()->setType, kSetUnion);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->select->fromTable->name, "A"));
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->select->setOperations->back()->nestedSelectStatement->fromTable->name, "B"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetExcept);
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "UNION_FIRST"));
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "C"));
	}
	{ // TEST(NestedAllSetOperationsWithWithClause) 
		TEST_PARSE_SINGLE_SQL("WITH UNION_FIRST AS (SELECT * FROM A UNION SELECT * FROM B) SELECT * FROM UNION_FIRST EXCEPT SELECT * FROM "
			"(SELECT * FROM C INTERSECT SELECT * FROM D)", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->alias, "UNION_FIRST"));
		SLCHECK_EQ(stmt->withDescriptions->back()->select->setOperations->back()->setType, kSetUnion);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->select->fromTable->name, "A"));
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->select->setOperations->back()->nestedSelectStatement->fromTable->name, "B"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetExcept);
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "UNION_FIRST"));
		SLCHECK_EQ(stmt->setOperations->back()->nestedSelectStatement->fromTable->select->setOperations->back()->setType, kSetIntersect);
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->select->fromTable->name, "C"));
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->select->setOperations->back()->nestedSelectStatement->fromTable->name, "D"));
	}
	{ // TEST(NestedSetOperationsWithMultipleWithClauses) 
		TEST_PARSE_SINGLE_SQL(
			"WITH UNION_FIRST AS (SELECT * FROM A UNION SELECT * FROM B),INTERSECT_SECOND AS (SELECT * FROM UNION_FIRST "
			"INTERSECT SELECT * FROM C) SELECT * FROM UNION_FIRST EXCEPT SELECT * FROM INTERSECT_SECOND",
			kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->at(0)->alias, "UNION_FIRST"));
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->alias, "INTERSECT_SECOND"));
		SLCHECK_EQ(stmt->withDescriptions->at(0)->select->setOperations->back()->setType, kSetUnion);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->at(0)->select->fromTable->name, "A"));
		SLCHECK_NZ(sstreq(stmt->withDescriptions->at(0)->select->setOperations->back()->nestedSelectStatement->fromTable->name, "B"));
		SLCHECK_EQ(stmt->withDescriptions->back()->select->setOperations->back()->setType, kSetIntersect);
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->select->fromTable->name, "UNION_FIRST"));
		SLCHECK_NZ(sstreq(stmt->withDescriptions->back()->select->setOperations->back()->nestedSelectStatement->fromTable->name, "C"));
		SLCHECK_EQ(stmt->setOperations->back()->setType, kSetExcept);
		SLCHECK_NZ(sstreq(stmt->fromTable->name, "UNION_FIRST"));
		SLCHECK_NZ(sstreq(stmt->setOperations->back()->nestedSelectStatement->fromTable->name, "INTERSECT_SECOND"));
	}
	{ // TEST(WrongOrderByStatementTest) 
		SQLParserResult res = parse_and_move("SELECT * FROM students ORDER BY name INTERSECT SELECT grade FROM students_2;");
		SLCHECK_Z(res.isValid());
	}
	{ // TEST(BeginTransactionTest) 
		{
			TEST_PARSE_SINGLE_SQL("BEGIN TRANSACTION;", kStmtTransaction, TransactionStatement, transaction_result, transaction_stmt);
			SLCHECK_EQ(transaction_stmt->command, kBeginTransaction);
		}
		{
			TEST_PARSE_SINGLE_SQL("BEGIN;", kStmtTransaction, TransactionStatement, transaction_result, transaction_stmt);
			SLCHECK_EQ(transaction_stmt->command, kBeginTransaction);
		}
	}
	{ // TEST(RollbackTransactionTest) 
		TEST_PARSE_SINGLE_SQL("ROLLBACK TRANSACTION;", kStmtTransaction, TransactionStatement, transaction_result, transaction_stmt);
		SLCHECK_EQ(transaction_stmt->command, kRollbackTransaction);
	}
	{ // TEST(CommitTransactionTest) 
		TEST_PARSE_SINGLE_SQL("COMMIT TRANSACTION;", kStmtTransaction, TransactionStatement, transaction_result, transaction_stmt);
		SLCHECK_EQ(transaction_stmt->command, kCommitTransaction);
	}
	{ // TEST(CastAsType) 
		TEST_PARSE_SINGLE_SQL("SELECT CAST(ID AS VARCHAR(8)) FROM TEST", kStmtSelect, SelectStatement, result, stmt);
		SLCHECK_NZ(result.isValid());
		SLCHECK_EQ(stmt->selectList->size(), 1U);
		SLCHECK_NZ(stmt->selectList->front()->columnType.data_type == hsql::DataType::VARCHAR);
		SLCHECK_EQ(stmt->selectList->front()->columnType.length, 8LL);
	}
	{ // TEST(SQLParserTokenizeTest)
		SLCHECK_NZ(test_tokens("SELECT * FROM test;", {SQL_SELECT, '*', SQL_FROM, SQL_IDENTIFIER, ';'}));
		SLCHECK_NZ(test_tokens("SELECT a, 'b' FROM test WITH HINT;", {SQL_SELECT, SQL_IDENTIFIER, ',', SQL_STRING, SQL_FROM, SQL_IDENTIFIER, SQL_WITH, SQL_HINT, ';'}));
	}
	{ // TEST(SQLParserTokenizeStringifyTest)
		const std::string query = "SELECT * FROM test;";
		std::vector<int16_t> tokens;
		SLCHECK_NZ(SQLParser::tokenize(query, &tokens));
		// Make u16string.
		std::u16string token_string(tokens.cbegin(), tokens.cend());
		// Check if u16 string is cacheable.
		std::map<std::u16string, std::string> cache;
		cache[token_string] = query;
		SLCHECK_NZ(query == cache[token_string]);
		SLCHECK_NZ(&query != &cache[token_string]);
	}
	//
	{ // TEST(PrepareSingleStatementTest)
		TEST_PARSE_SINGLE_SQL("PREPARE test FROM 'SELECT * FROM students WHERE grade = ?';", kStmtPrepare, PrepareStatement, result, prepare);
		SLCHECK_NZ(sstreq(prepare->name, "test"));
		SLCHECK_NZ(sstreq(prepare->query, "SELECT * FROM students WHERE grade = ?"));
		TEST_PARSE_SINGLE_SQL(prepare->query, kStmtSelect, SelectStatement, result2, select);
		SLCHECK_EQ(result2.parameters().size(), 1U);
		SLCHECK_NZ(select->whereClause->expr2->isType(kExprParameter));
		SLCHECK_EQ(select->whereClause->expr2->ival, 0LL);
	}
	{ // TEST(DeallocatePrepareStatementTest)
		TEST_PARSE_SINGLE_SQL("DEALLOCATE PREPARE test;", kStmtDrop, DropStatement, result, drop);
		SLCHECK_EQ(drop->type, kDropPreparedStatement);
		SLCHECK_NZ(sstreq(drop->name, "test"));
	}
	{ // TEST(StatementWithParameters)
		TEST_PARSE_SINGLE_SQL("SELECT * FROM test WHERE a = ? AND b = ?", kStmtSelect, SelectStatement, result, stmt);
		const hsql::Expr* eq1 = stmt->whereClause->expr;
		const hsql::Expr* eq2 = stmt->whereClause->expr2;
		SLCHECK_EQ(result.parameters().size(), 2U);
		SLCHECK_EQ(eq1->opType, hsql::kOpEquals);
		SLCHECK_NZ(eq1->expr->isType(hsql::kExprColumnRef));
		SLCHECK_NZ(eq1->expr2->isType(kExprParameter));
		SLCHECK_EQ(eq1->expr2->ival, 0LL);
		SLCHECK_EQ(result.parameters()[0], eq1->expr2);
		SLCHECK_EQ(eq2->opType, hsql::kOpEquals);
		SLCHECK_NZ(eq2->expr->isType(hsql::kExprColumnRef));
		SLCHECK_NZ(eq2->expr2->isType(kExprParameter));
		SLCHECK_EQ(eq2->expr2->ival, 1LL);
		SLCHECK_EQ(result.parameters()[1], eq2->expr2);
	}
	{ // TEST(ExecuteStatementTest)
		TEST_PARSE_SINGLE_SQL("EXECUTE test(1, 2);", kStmtExecute, ExecuteStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->name, "test"));
		SLCHECK_EQ(stmt->parameters->size(), 2U);
	}
	{ // TEST(ExecuteStatementTestNoParam)
		TEST_PARSE_SINGLE_SQL("EXECUTE test();", kStmtExecute, ExecuteStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->name, "test"));
		SLCHECK_EQ(stmt->parameters, 0);
	}
	{ // TEST(ExecuteStatementTestNoParamList)
		TEST_PARSE_SINGLE_SQL("EXECUTE test;", kStmtExecute, ExecuteStatement, result, stmt);
		SLCHECK_NZ(sstreq(stmt->name, "test"));
		SLCHECK_Z(stmt->parameters);
	}
	{
		//
		// Здесь я хочу проверить парсинг файлов создания базы данных Papyrus, генерируемых при сборке системы
		//
		SString path;
		SLS.QueryPath("srcroot", path);
		if(path.NotEmpty()) {
			const char * pp_files[] = { "ppdbs-mysql.sql", "ppdbs-sqlite.sql"};
			STempBuffer in_buf(SMEGABYTE(1));
			path.SetLastSlash().Cat("rsrc").SetLastSlash().Cat("dl600");
			SString src_file_name;
			for(uint i = 0; i < SIZEOFARRAY(pp_files); i++) {
				(src_file_name = path).SetLastSlash().Cat(pp_files[i]);
				SFile f_in(src_file_name, SFile::mRead);
				if(f_in.IsValid()) {
					size_t actual_size = 0;
					if(f_in.ReadAll(in_buf, 0, &actual_size)) {
						hsql::SQLParserResult result;
						std::string query;
						query.insert(0, static_cast<const char *>(in_buf.vcptr()), actual_size);
						if(hsql::SQLParser::parse(query, &result)) {
							SLCHECK_NZ(result.isValid());
						}
					}
				}
			}
		}
	}
	if(1) {
		// Произвольный sql-файл D:\\DEV\\Resource\\Data\\ETC\\(DB timeup.es).sql 
		// D:/DEV/Resource/Data/ETC/scantour_ru\tourist.sql
		STempBuffer in_buf(SMEGABYTE(1));
		//SString src_file_name("C:\\DEV\\RESOURCE\\DATA\\ETC\\scantour.ru\\pay_zap_new.sql");
		SString src_file_name("C:\\DEV\\RESOURCE\\DATA\\ETC\\scantour.ru\\tourist.sql"/*"D:/DEV/Resource/Data/ETC/scantour_ru/tourist.sql"*/);
		SFile f_in(src_file_name, SFile::mRead|SFile::mBinary);
		if(f_in.IsValid()) {
			size_t actual_size = 0;
			if(f_in.ReadAll(in_buf, 0, &actual_size)) {
				hsql::SQLParserResult result;
				SString query;
				query.CatN(in_buf, actual_size);
				if(hsql::SQLParser::parse(query, &result)) {
					SLCHECK_NZ(result.isValid());
				}
			}
		}
	}
	return CurrentStatus;
}
