// SQL-PARSER.H
//
#ifndef __SQL_PARSER_H
#define __SQL_PARSER_H
#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>
#include <ostream>
#include <unordered_set>
//#include <sql\ColumnType.h>
namespace hsql {
	enum class DataType {
		UNKNOWN,
		BIGINT,
		BOOLEAN,
		CHAR,
		DATE,
		DATETIME,
		DECIMAL,
		DOUBLE,
		FLOAT,
		INT,
		LONG,
		REAL,
		SMALLINT,
		TEXT,
		TIME,
		VARCHAR,
		BINARY, // @sobolev
		BLOB, // @sobolev
		MEDIUMBLOB, // @sobolev mysql
		TINYTEXT, // @sobolev mysql
		MEDIUMTEXT, // @sobolev mysql
		LONGTEXT, // @sobolev mysql
	};

	// Represents the type of a column, e.g., FLOAT or VARCHAR(10)
	struct ColumnType {
		static bool Make(const char * pIdent, int len, int prec, ColumnType & rResult, SString & rMsgBuf);
		ColumnType() = default;
		ColumnType(DataType data_type, int64 length = 0, int64 precision = 0, int64 scale = 0);
		DataType data_type;
		int64 length; // Used for, e.g., VARCHAR(10)
		int64 precision; // Used for, e.g., DECIMAL (6, 4) or TIME (5)
		int64 scale; // Used for DECIMAL (6, 4)
	};

	bool operator==(const ColumnType& lhs, const ColumnType& rhs);
	bool operator!=(const ColumnType& lhs, const ColumnType& rhs);
	std::ostream& operator<<(std::ostream&, const ColumnType&);
//#include <sql\Expr.h>
	struct SelectStatement;
	struct OrderDescription;

	// Helper function used by the lexer.
	// TODO: move to more appropriate place.
	char* substr(const char* source, int from, int to);

	enum ExprType {
		kExprLiteralFloat,
		kExprLiteralString,
		kExprLiteralInt,
		kExprLiteralNull,
		kExprLiteralDate,
		kExprLiteralInterval,
		kExprStar,
		kExprParameter,
		kExprColumnRef,
		kExprFunctionRef,
		kExprOperator,
		kExprSelect,
		kExprHint,
		kExprArray,
		kExprArrayIndex,
		kExprExtract,
		kExprCast
	};

	// Operator types. These are important for expressions of type kExprOperator.
	enum OperatorType {
		kOpNone,

		// Ternary operator
		kOpBetween,

		// n-nary special case
		kOpCase,
		kOpCaseListElement, // `WHEN expr THEN expr`

		// Binary operators.
		kOpPlus,
		kOpMinus,
		kOpAsterisk,
		kOpSlash,
		kOpPercentage,
		kOpCaret,

		kOpEquals,
		kOpNotEquals,
		kOpLess,
		kOpLessEq,
		kOpGreater,
		kOpGreaterEq,
		kOpLike,
		kOpNotLike,
		kOpILike,
		kOpAnd,
		kOpOr,
		kOpIn,
		kOpConcat,

		// Unary operators.
		kOpNot,
		kOpUnaryMinus,
		kOpIsNull,
		kOpExists
	};

	enum DatetimeField {
		kDatetimeNone,
		kDatetimeSecond,
		kDatetimeMinute,
		kDatetimeHour,
		kDatetimeDay,
		kDatetimeMonth,
		kDatetimeYear,
	};

	// Description of the frame clause within a window expression.
	enum FrameBoundType { kFollowing, kPreceding, kCurrentRow };

	struct FrameBound {
		FrameBound(int64 offset, FrameBoundType type, bool unbounded);

		int64 offset;
		FrameBoundType type;
		bool unbounded;
	};

	enum FrameType { kRange, kRows, kGroups };

	struct FrameDescription {
		FrameDescription(FrameType type, FrameBound* start, FrameBound* end);
		virtual ~FrameDescription();

		FrameType type;
		FrameBound* start;
		FrameBound* end;
	};

	typedef struct Expr Expr;

	// Description of additional fields for a window expression.
	struct WindowDescription {
		WindowDescription(std::vector<Expr*>* partitionList, std::vector<OrderDescription*>* orderList, FrameDescription* frameDescription);
		virtual ~WindowDescription();

		std::vector<Expr*> * partitionList;
		std::vector<OrderDescription*> * orderList;
		FrameDescription * frameDescription;
	};

	// Represents SQL expressions (i.e. literals, operators, column_refs).
	// TODO: When destructing a placeholder expression, we might need to alter the
	// placeholder_list.
	struct Expr {
		explicit Expr(ExprType type);
		virtual ~Expr();

		ExprType type;
		// TODO: Replace expressions by list.
		Expr * expr;
		Expr * expr2;
		std::vector<Expr*> * exprList;
		SelectStatement * select;
		char * name;
		char * table;
		char * alias;
		double fval;
		int64  ival;
		int64  ival2;
		DatetimeField datetimeField;
		ColumnType columnType;
		OperatorType opType;
		bool   isBoolLiteral;
		bool   distinct;
		WindowDescription * windowDescription;
		//
		// Convenience accessor methods.
		//
		bool   isType(ExprType exprType) const;
		bool   isLiteral() const;
		bool   hasAlias() const;
		bool   hasTable() const;
		const  char * getName() const;
		//
		// Static constructors.
		//
		static Expr * make(ExprType type);
		static Expr * makeOpUnary(OperatorType op, Expr* expr);
		static Expr * makeOpBinary(Expr* expr1, OperatorType op, Expr* expr2);
		static Expr * makeBetween(Expr* expr, Expr* left, Expr* right);
		static Expr * makeCaseList(Expr* caseListElement);
		static Expr * makeCaseListElement(Expr* when, Expr* then);
		static Expr * caseListAppend(Expr* caseList, Expr* caseListElement);
		static Expr * makeCase(Expr* expr, Expr* when, Expr* elseExpr);
		static Expr * makeLiteral(int64 val);
		static Expr * makeLiteral(double val);
		static Expr * makeLiteral(char* val);
		static Expr * makeLiteral(bool val);
		static Expr * makeNullLiteral();
		static Expr * makeDateLiteral(char* val);
		static Expr * makeIntervalLiteral(int64 duration, DatetimeField unit);
		static Expr * makeColumnRef(char* name);
		static Expr * makeColumnRef(char* table, char* name);
		static Expr * makeStar(void);
		static Expr * makeStar(char* table);
		static Expr * makeFunctionRef(char* func_name, std::vector<Expr*>* exprList, bool distinct, WindowDescription* window);
		static Expr * makeArray(std::vector<Expr*>* exprList);
		static Expr * makeArrayIndex(Expr* expr, int64 index);
		static Expr * makeParameter(int id);
		static Expr * makeSelect(SelectStatement* select);
		static Expr * makeExists(SelectStatement* select);
		static Expr * makeInOperator(Expr* expr, std::vector<Expr*>* exprList);
		static Expr * makeInOperator(Expr* expr, SelectStatement* select);
		static Expr * makeExtract(DatetimeField datetimeField1, Expr* expr);
		static Expr * makeCast(Expr* expr, ColumnType columnType);
	};

	// Zero initializes an Expr object and assigns it to a space in the heap
	// For Hyrise we still had to put in the explicit NULL constructor
	// http://www.ex-parrot.com/~chris/random/initialise.html
	// Unused
	#define ALLOC_EXPR(var, type)         \
		Expr* var;                          \
		do {                                \
			Expr zero = {type};               \
			var = (Expr *)SAlloc::M(sizeof *var); \
			*var = zero;                      \
		} while(0);
	#undef ALLOC_EXPR
//#include <sql\SQLStatement.h>
	enum StatementType {
		kStmtError, // unused
		kStmtSelect,
		kStmtImport,
		kStmtInsert,
		kStmtUpdate,
		kStmtDelete,
		kStmtCreate,
		kStmtDrop,
		kStmtPrepare,
		kStmtExecute,
		kStmtExport,
		kStmtRename,
		kStmtAlter,
		kStmtShow,
		kStmtTransaction
	};

	// Base struct for every SQL statement
	struct SQLStatement {
		SQLStatement(StatementType type);
		virtual ~SQLStatement();
		StatementType type() const;
		bool isType(StatementType type) const;
		// Shorthand for isType(type).
		bool is(StatementType type) const;
		// Length of the string in the SQL query string
		size_t stringLength;
		std::vector<Expr*>* hints;
	private:
		StatementType type_;
	};
//#include <sql\Table.h>
	struct SelectStatement;
	struct JoinDefinition;
	struct TableRef;

	// Possible table reference types.
	enum TableRefType { kTableName, kTableSelect, kTableJoin, kTableCrossProduct };

	struct TableName {
		char* schema;
		char* name;
	};

	struct Alias {
		Alias(char* name, std::vector<char*>* columns = nullptr);
		~Alias();

		char* name;
		std::vector<char*>* columns;
	};

	// Holds reference to tables. Can be either table names or a select statement.
	struct TableRef {
		TableRef(TableRefType type);
		virtual ~TableRef();
		// Returns true if a schema is set.
		bool hasSchema() const;
		// Returns the alias, if it is set. Otherwise the name.
		const char* getName() const;

		TableRefType type;
		char* schema;
		char* name;
		Alias* alias;
		SelectStatement* select;
		std::vector<TableRef*>* list;
		JoinDefinition* join;
	};

	// Possible types of joins.
	enum JoinType { kJoinInner, kJoinFull, kJoinLeft, kJoinRight, kJoinCross, kJoinNatural };

	// Definition of a join construct.
	struct JoinDefinition {
		JoinDefinition();
		virtual ~JoinDefinition();
		TableRef* left;
		TableRef* right;
		Expr* condition;
		JoinType type;
	};
//#include <sql\AlterStatement.h>
	// Note: Implementations of constructors and destructors can be found in statements.cpp.
	enum ActionType {
		DropColumn,
	};

	struct AlterAction {
		AlterAction(ActionType type);
		virtual ~AlterAction();
		ActionType type;
	};

	struct DropColumnAction : AlterAction {
		DropColumnAction(char* column_name);
		~DropColumnAction() override;
		char* columnName;
		bool ifExists;
	};

	// Represents SQL Alter Table statements.
	// Example "ALTER TABLE students DROP COLUMN name;"
	struct AlterStatement : SQLStatement {
		AlterStatement(char* name, AlterAction* action);
		~AlterStatement() override;

		char* schema;
		bool ifTableExists;
		char* name;
		AlterAction* action;
	};
//#include <sql\CreateStatement.h>
	// Note: Implementations of constructors and destructors can be found in statements.cpp.
	struct SelectStatement;

	enum struct ConstraintType { 
		None, 
		NotNull, 
		Null, 
		PrimaryKey, 
		Unique,
		AutoIncrement // @sobolev
	};

	// Superclass for both TableConstraint and Column Definition
	struct TableElement {
		virtual ~TableElement() 
		{
		}
	};

	// Represents definition of a table constraint
	struct TableConstraint : TableElement {
		TableConstraint(ConstraintType keyType, std::vector<char*>* columnNames);
		~TableConstraint() override;

		ConstraintType type;
		std::vector<char*> * columnNames;
	};

	// Represents definition of a table column
	struct ColumnDefinition : TableElement {
		ColumnDefinition(char* name, ColumnType type, std::unordered_set<ConstraintType> * pConstraints);
		~ColumnDefinition() override;
		// By default, columns are nullable. However, we track if a column is explicitly requested to be nullable to
		// notice conflicts with PRIMARY KEY table constraints.
		bool trySetNullableExplicit() 
		{
			if(P_Constraints->count(ConstraintType::NotNull) || P_Constraints->count(ConstraintType::PrimaryKey)) {
				if(P_Constraints->count(ConstraintType::Null)) {
					return false;
				}
				nullable = false;
			}
			return true;
		}
		std::unordered_set<ConstraintType> * P_Constraints;
		char * name;
		char * P_CharSet; // @sobolev
		ColumnType type;
		bool nullable;
	};

	enum CreateType {
		kCreateTable,
		kCreateTableFromTbl, // Hyrise file format
		kCreateView,
		kCreateIndex
	};

	struct IndexSegment {
		IndexSegment() : Flags(0)
		{
		}
		enum {
			fDesc = 0x0001
		};
		uint   Flags;
		SString Field;
	};

	class Index : public TSCollection <IndexSegment> {
	public:
		Index() : TSCollection <IndexSegment>(), Flags(0)
		{
		}
		enum {
			fUnique = 0x0001
		};
		uint   Flags;
		SString Name;
	};

	// Represents SQL Create statements.
	// Example: "CREATE TABLE students (name TEXT, student_number INTEGER, city TEXT, grade DOUBLE)"
	struct CreateStatement : SQLStatement {
		explicit CreateStatement(CreateType type);
		~CreateStatement() override;
		void setColumnDefsAndConstraints(std::vector<TableElement*>* tableElements);

		CreateType type;
		bool ifNotExists;                         // default: false
		bool fUnique;                             // default: false @sobolev (create unique index)
		char * filePath;                          // default: nullptr
		char * schema;                            // default: nullptr
		char * tableName;                         // default: nullptr
		char * indexName;                         // default: nullptr
		// @sobolev std::vector<char*> * indexColumns;        // default: nullptr
		Index * P_Idx; // @sobolev
		std::vector<ColumnDefinition*> * columns; // default: nullptr
		std::vector<TableConstraint*>  * tableConstraints; // default: nullptr
		std::vector<char*> * viewColumns;
		SelectStatement * select;
	};
//#include <sql\DeleteStatement.h>
	// Note: Implementations of constructors and destructors can be found in statements.cpp.
	// 
	// Represents SQL Delete statements.
	// Example: "DELETE FROM students WHERE grade > 3.0"
	// Note: if (expr == nullptr) => delete all rows (truncate)
	struct DeleteStatement : SQLStatement {
		DeleteStatement();
		~DeleteStatement() override;

		char* schema;
		char* tableName;
		Expr* expr;
	};
//#include <sql\DropStatement.h>
	// Note: Implementations of constructors and destructors can be found in statements.cpp.
	enum DropType { 
		kDropTable, 
		kDropSchema, 
		kDropIndex, 
		kDropView, 
		kDropPreparedStatement 
	};

	// Represents SQL Delete statements.
	// Example "DROP TABLE students;"
	struct DropStatement : SQLStatement {
		DropStatement(DropType type);
		~DropStatement() override;

		DropType type;
		bool ifExists;
		char* schema;
		char* name;
		char* indexName;
	};
//#include <sql\ExecuteStatement.h>
	// Represents SQL Execute statements.
	// Example: "EXECUTE ins_prep(100, "test", 2.3);"
	struct ExecuteStatement : SQLStatement {
		ExecuteStatement();
		~ExecuteStatement() override;

		char* name;
		std::vector<Expr*>* parameters;
	};
//#include <sql\SelectStatement.h>
	enum OrderType { 
		kOrderAsc, 
		kOrderDesc 
	};

	enum SetType { 
		kSetUnion, 
		kSetIntersect, 
		kSetExcept 
	};

	enum RowLockMode { 
		ForUpdate, 
		ForNoKeyUpdate, 
		ForShare, 
		ForKeyShare 
	};

	enum RowLockWaitPolicy { 
		NoWait, 
		SkipLocked, 
		None 
	};

	// Description of the order by clause within a select statement.
	struct OrderDescription {
		OrderDescription(OrderType type, Expr* expr);
		virtual ~OrderDescription();

		OrderType type;
		Expr* expr;
	};

	// Description of the limit clause within a select statement.
	struct LimitDescription {
		LimitDescription(Expr* limit, Expr* offset);
		virtual ~LimitDescription();

		Expr* limit;
		Expr* offset;
	};

	// Description of the group-by clause within a select statement.
	struct GroupByDescription {
		GroupByDescription();
		virtual ~GroupByDescription();

		std::vector<Expr*>* columns;
		Expr* having;
	};

	struct WithDescription {
		~WithDescription();

		char* alias;
		SelectStatement* select;
	};

	struct SetOperation {
		SetOperation();
		virtual ~SetOperation();

		SetType setType;
		bool isAll;
		SelectStatement* nestedSelectStatement;
		std::vector<OrderDescription*>* resultOrder;
		LimitDescription* resultLimit;
	};

	struct LockingClause {
		RowLockMode rowLockMode;
		RowLockWaitPolicy rowLockWaitPolicy;
		std::vector<char*>* tables;
	};

	// Representation of a full SQL select statement.
	struct SelectStatement : SQLStatement {
		SelectStatement();
		~SelectStatement() override;

		TableRef* fromTable;
		bool selectDistinct;
		std::vector<Expr*>* selectList;
		Expr* whereClause;
		GroupByDescription* groupBy;

		// Note that a SetOperation is always connected to a
		// different SelectStatement. This statement can itself
		// have SetOperation connections to other SelectStatements.
		// To evaluate the operations in the correct order:
		//    Iterate over the setOperations vector:
		//      1. Fully evaluate the nestedSelectStatement within the SetOperation
		//      2. Connect the original statement with the
		//         evaluated nestedSelectStatement
		//      3. Apply the resultOrder and the resultLimit
		//      4. The result now functions as the the original statement
		//         for the next iteration
		//
		// Example:
		//
		//   (SELECT * FROM students INTERSECT SELECT * FROM students_2) UNION SELECT * FROM students_3 ORDER BY grade ASC;
		//
		//   1. We evaluate `Select * FROM students`
		//   2. Then we iterate over the setOperations vector
		//   3. We evalute the nestedSelectStatement of the first entry, which is: `SELECT * FROM students_2`
		//   4. We connect the result of 1. with the results of 3. using the setType, which is INTERSECT
		//   5. We continue the iteration of the setOperations vector
		//   6. We evaluate the new nestedSelectStatement which is: `SELECT * FROM students_3`
		//   7. We apply a Union-Operation to connect the results of 4. and 6.
		//   8. Finally, we apply the resultOrder of the last SetOperation (ORDER BY grade ASC)
		std::vector<SetOperation*>* setOperations;

		std::vector<OrderDescription*>* order;
		std::vector<WithDescription*>* withDescriptions;
		LimitDescription* limit;
		std::vector<LockingClause*>* lockings;
	};
//#include <sql\ImportStatement.h>
	enum ImportType {
		kImportCSV,
		kImportTbl, // Hyrise file format
		kImportBinary,
		kImportAuto
	};

	// Represents SQL Import statements.
	struct ImportStatement : SQLStatement {
		ImportStatement(ImportType type);
		~ImportStatement() override;

		ImportType type;
		char* filePath;
		char* schema;
		char* tableName;
		Expr* whereClause;
	};
//#include <sql\ExportStatement.h>
	// Represents SQL Export statements.
	struct ExportStatement : SQLStatement {
		ExportStatement(ImportType type);
		~ExportStatement() override;

		// ImportType is used for compatibility reasons
		ImportType type;
		char* filePath;
		char* schema;
		char* tableName;
		SelectStatement* select;
	};
//#include <sql\InsertStatement.h>
	enum InsertType { 
		kInsertValues, 
		kInsertSelect 
	};

	// Represents SQL Insert statements.
	// Example: "INSERT INTO students VALUES ('Max', 1112233, 'Musterhausen', 2.3)"
	struct InsertStatement : SQLStatement {
		InsertStatement(InsertType type);
		~InsertStatement() override;

		InsertType type;
		char* schema;
		char* tableName;
		std::vector<char*>* columns;
		std::vector<Expr*>* values;
		SelectStatement* select;
	};
//#include <sql\PrepareStatement.h>
	// Represents SQL Prepare statements.
	// Example: PREPARE test FROM 'SELECT * FROM test WHERE a = ?;'
	struct PrepareStatement : SQLStatement {
		PrepareStatement();
		~PrepareStatement() override;
		char* name;
		// The query that is supposed to be prepared.
		char* query;
	};
//#include <sql\ShowStatement.h>
	// Note: Implementations of constructors and destructors can be found in statements.cpp.
	enum ShowType { 
		kShowColumns, 
		kShowTables 
	};

	// Represents SQL SHOW statements.
	// Example "SHOW TABLES;"
	struct ShowStatement : SQLStatement {
		ShowStatement(ShowType type);
		~ShowStatement() override;

		ShowType type;
		char* schema;
		char* name;
	};
//#include <sql\TransactionStatement.h>
	// Represents SQL Transaction statements.
	// Example: BEGIN TRANSACTION;
	enum TransactionCommand { kBeginTransaction, kCommitTransaction, kRollbackTransaction };

	struct TransactionStatement : SQLStatement {
		TransactionStatement(TransactionCommand command);
		~TransactionStatement() override;

		TransactionCommand command;
	};
//#include <sql\UpdateStatement.h>
	// Represents "column = value" expressions.
	struct UpdateClause {
		char* column;
		Expr* value;
	};

	// Represents SQL Update statements.
	struct UpdateStatement : SQLStatement {
		UpdateStatement();
		~UpdateStatement() override;
		// TODO: switch to char* instead of TableRef
		TableRef* table;
		std::vector<UpdateClause*>* updates;
		Expr* where;
	};
//#include <util\sqlhelper.h>
	// Prints a summary of the given SQLStatement.
	void printStatementInfo(const SQLStatement* stmt);
	// Prints a summary of the given SelectStatement with the given indentation.
	void printSelectStatementInfo(const SelectStatement* stmt, uintmax_t num_indent);
	// Prints a summary of the given ImportStatement with the given indentation.
	void printImportStatementInfo(const ImportStatement* stmt, uintmax_t num_indent);
	// Prints a summary of the given CopyStatement with the given indentation.
	void printExportStatementInfo(const ExportStatement* stmt, uintmax_t num_indent);
	// Prints a summary of the given InsertStatement with the given indentation.
	void printInsertStatementInfo(const InsertStatement* stmt, uintmax_t num_indent);
	// Prints a summary of the given CreateStatement with the given indentation.
	void printCreateStatementInfo(const CreateStatement* stmt, uintmax_t num_indent);
	// Prints a summary of the given TransactionStatement with the given indentation.
	void printTransactionStatementInfo(const TransactionStatement* stmt, uintmax_t num_indent);
	// Prints a summary of the given Expression with the given indentation.
	void printExpression(const Expr * expr, uintmax_t num_indent);
	// Prints an ORDER BY clause
	void printOrderBy(const std::vector<OrderDescription*>* expr, uintmax_t num_indent);
	// Prints WindowDescription.
	void printWindowDescription(WindowDescription* window_description, uintmax_t num_indent);
//#include <SQLParserResult.h>
	// 
	// Represents the result of the SQLParser.
	// If parsing was successful it contains a list of SQLStatement.
	//
	class SQLParserResult {
	public:
		// Initialize with empty statement list.
		SQLParserResult();
		// Initialize with a single statement.
		// Takes ownership of the statement.
		SQLParserResult(SQLStatement* stmt);
		// Move constructor.
		SQLParserResult(SQLParserResult&& moved);
		SQLParserResult& operator=(SQLParserResult&& moved);
		// Deletes all statements in the result.
		virtual ~SQLParserResult();
		// Set whether parsing was successful.
		void setIsValid(bool isValid);
		// Returns true if parsing was successful.
		bool isValid() const;
		// Returns the number of statements in the result.
		size_t size() const;
		// Set the details of the error, if available.
		// Takes ownership of errorMsg.
		void setErrorDetails(char* errorMsg, int errorLine, int errorColumn);
		// Returns the error message, if an error occurred.
		const char* errorMsg() const;
		// Returns the line number of the occurrance of the error in the query.
		int errorLine() const;
		// Returns the column number of the occurrance of the error in the query.
		int errorColumn() const;
		// Adds a statement to the result list of statements.
		// SQLParserResult takes ownership of the statement.
		void addStatement(SQLStatement* stmt);
		// Gets the SQL statement with the given index.
		const SQLStatement* getStatement(size_t index) const;
		// Gets the non const SQL statement with the given index.
		SQLStatement* getMutableStatement(size_t index);
		// Get the list of all statements.
		const std::vector<SQLStatement*>& getStatements() const;
		// Returns a copy of the list of all statements in this result.
		// Removes them from this result.
		std::vector<SQLStatement*> releaseStatements();
		// Deletes all statements and other data within the result.
		void reset();
		// Does NOT take ownership.
		void addParameter(Expr* parameter);
		const std::vector<Expr*>& parameters();
	private:
		// List of statements within the result.
		std::vector<SQLStatement*> statements_;
		// Flag indicating the parsing was successful.
		bool isValid_;
		// Error message, if an error occurred.
		char* errorMsg_;
		// Line number of the occurrance of the error in the query.
		int errorLine_;
		// Column number of the occurrance of the error in the query.
		int errorColumn_;
		// Does NOT have ownership.
		std::vector<Expr*> parameters_;
	};
//#include <SQLParser.h>
	//
	// Static methods used to parse SQL strings.
	//
	class SQLParser {
	public:
		// Parses a given constant character SQL string into the result object.
		// Returns true if the lexer and parser could run without internal errors.
		// This does NOT mean that the SQL string was valid SQL. To check that
		// you need to check result->isValid();
		static bool parse(const std::string & sql, SQLParserResult* result);
		static bool parse(const char * pSqlPgm, SQLParserResult * result);
		// Run tokenization on the given string and store the tokens in the output vector.
		static bool tokenize(const std::string & sql, std::vector<int16_t>* tokens);
		// Deprecated.
		// Old method to parse SQL strings. Replaced by parse().
		static bool parseSQLString(const char * sql, SQLParserResult* result);
		// Deprecated.
		// Old method to parse SQL strings. Replaced by parse().
		static bool parseSQLString(const std::string & sql, SQLParserResult* result);
	private:
		SQLParser();
	};
}  // namespace hsql
// 
//#include <parser\bison_parser.h>
//#include <parser\flex_lexer.h>
#endif // __SQL_PARSER_H
