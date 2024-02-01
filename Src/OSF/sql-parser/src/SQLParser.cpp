//
//
#include <sql-parser.h>
#pragma hdrstop
#include "parser/bison_parser.h"
#include "parser/flex_lexer.h"

namespace hsql {
	FrameBound::FrameBound(int64_t offset, FrameBoundType type, bool unbounded) : offset{offset}, type{type}, unbounded{unbounded} 
	{
	}

	FrameDescription::FrameDescription(FrameType type, FrameBound* start, FrameBound* end) : type{type}, start{start}, end{end} 
	{
	}

	FrameDescription::~FrameDescription() 
	{
		delete start;
		delete end;
	}

	WindowDescription::WindowDescription(std::vector<Expr*>* partitionList, std::vector<OrderDescription*>* orderList,
		FrameDescription* frameDescription) : partitionList{partitionList}, orderList{orderList}, frameDescription{frameDescription} 
	{
	}

	WindowDescription::~WindowDescription() 
	{
		if(partitionList) {
			for(Expr* e : *partitionList) {
				delete e;
			}
			delete partitionList;
		}
		if(orderList) {
			for(OrderDescription* orderDescription : *orderList) {
				delete orderDescription;
			}
			delete orderList;
		}
		delete frameDescription;
	}

	Expr::Expr(ExprType type) : type(type), expr(nullptr), expr2(nullptr), exprList(nullptr), select(nullptr), name(nullptr), table(nullptr),
		alias(nullptr), fval(0), ival(0), ival2(0), datetimeField(kDatetimeNone), columnType(DataType::UNKNOWN, 0), isBoolLiteral(false),
		opType(kOpNone), distinct(false), windowDescription(nullptr) 
	{
	}

	Expr::~Expr() 
	{
		delete expr;
		delete expr2;
		delete select;
		delete windowDescription;
		SAlloc::F(name);
		SAlloc::F(table);
		SAlloc::F(alias);
		if(exprList) {
			for(Expr* e : *exprList) {
				delete e;
			}
			delete exprList;
		}
	}

	Expr* Expr::make(ExprType type) 
	{
		Expr* e = new Expr(type);
		return e;
	}

	Expr* Expr::makeOpUnary(OperatorType op, Expr* expr) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = op;
		e->expr = expr;
		e->expr2 = nullptr;
		return e;
	}

	Expr* Expr::makeOpBinary(Expr* expr1, OperatorType op, Expr* expr2) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = op;
		e->expr = expr1;
		e->expr2 = expr2;
		return e;
	}

	Expr* Expr::makeBetween(Expr* expr, Expr* left, Expr* right) 
	{
		Expr* e = new Expr(kExprOperator);
		e->expr = expr;
		e->opType = kOpBetween;
		e->exprList = new std::vector<Expr*>();
		e->exprList->push_back(left);
		e->exprList->push_back(right);
		return e;
	}

	Expr* Expr::makeCaseList(Expr* caseListElement)
	{
		Expr* e = new Expr(kExprOperator);
		// Case list expressions are temporary and will be integrated into the case
		// expressions exprList - thus assign operator type kOpNone
		e->opType = kOpNone;
		e->exprList = new std::vector<Expr*>();
		e->exprList->push_back(caseListElement);
		return e;
	}

	Expr* Expr::makeCaseListElement(Expr* when, Expr* then) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = kOpCaseListElement;
		e->expr = when;
		e->expr2 = then;
		return e;
	}

	Expr* Expr::caseListAppend(Expr* caseList, Expr* caseListElement) 
	{
		caseList->exprList->push_back(caseListElement);
		return caseList;
	}

	Expr* Expr::makeCase(Expr* expr, Expr* caseList, Expr* elseExpr) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = kOpCase;
		e->expr = expr;
		e->expr2 = elseExpr;
		e->exprList = caseList->exprList;
		caseList->exprList = nullptr;
		delete caseList;
		return e;
	}

	Expr* Expr::makeLiteral(int64_t val) 
	{
		Expr* e = new Expr(kExprLiteralInt);
		e->ival = val;
		return e;
	}

	Expr* Expr::makeLiteral(double value) 
	{
		Expr* e = new Expr(kExprLiteralFloat);
		e->fval = value;
		return e;
	}

	Expr* Expr::makeLiteral(char* string) 
	{
		Expr* e = new Expr(kExprLiteralString);
		e->name = string;
		return e;
	}

	Expr* Expr::makeLiteral(bool val) 
	{
		Expr* e = new Expr(kExprLiteralInt);
		e->ival = (int)val;
		e->isBoolLiteral = true;
		return e;
	}

	Expr* Expr::makeNullLiteral()
	{
		Expr* e = new Expr(kExprLiteralNull);
		return e;
	}

	Expr* Expr::makeDateLiteral(char* string) 
	{
		Expr* e = new Expr(kExprLiteralDate);
		e->name = string;
		return e;
	}

	Expr* Expr::makeIntervalLiteral(int64_t duration, DatetimeField unit) 
	{
		Expr* e = new Expr(kExprLiteralInterval);
		e->ival = duration;
		e->datetimeField = unit;
		return e;
	}

	Expr* Expr::makeColumnRef(char* name) 
	{
		Expr* e = new Expr(kExprColumnRef);
		e->name = name;
		return e;
	}

	Expr* Expr::makeColumnRef(char* table, char* name) 
	{
		Expr* e = new Expr(kExprColumnRef);
		e->name = name;
		e->table = table;
		return e;
	}

	Expr* Expr::makeStar(void) 
	{
		Expr* e = new Expr(kExprStar);
		return e;
	}

	Expr* Expr::makeStar(char* table) 
	{
		Expr* e = new Expr(kExprStar);
		e->table = table;
		return e;
	}

	Expr* Expr::makeFunctionRef(char* func_name, std::vector<Expr*>* exprList, bool distinct, WindowDescription* window) 
	{
		Expr* e = new Expr(kExprFunctionRef);
		e->name = func_name;
		e->exprList = exprList;
		e->distinct = distinct;
		e->windowDescription = window;
		return e;
	}

	Expr* Expr::makeArray(std::vector<Expr*>* exprList) 
	{
		Expr* e = new Expr(kExprArray);
		e->exprList = exprList;
		return e;
	}

	Expr* Expr::makeArrayIndex(Expr* expr, int64_t index) 
	{
		Expr* e = new Expr(kExprArrayIndex);
		e->expr = expr;
		e->ival = index;
		return e;
	}

	Expr* Expr::makeParameter(int id) 
	{
		Expr* e = new Expr(kExprParameter);
		e->ival = id;
		return e;
	}

	Expr* Expr::makeSelect(SelectStatement* select) 
	{
		Expr* e = new Expr(kExprSelect);
		e->select = select;
		return e;
	}

	Expr* Expr::makeExists(SelectStatement* select) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = kOpExists;
		e->select = select;
		return e;
	}

	Expr* Expr::makeInOperator(Expr* expr, std::vector<Expr*>* exprList) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = kOpIn;
		e->expr = expr;
		e->exprList = exprList;
		return e;
	}

	Expr* Expr::makeInOperator(Expr* expr, SelectStatement* select) 
	{
		Expr* e = new Expr(kExprOperator);
		e->opType = kOpIn;
		e->expr = expr;
		e->select = select;
		return e;
	}

	Expr* Expr::makeExtract(DatetimeField datetimeField, Expr* expr) 
	{
		Expr* e = new Expr(kExprExtract);
		e->datetimeField = datetimeField;
		e->expr = expr;
		return e;
	}

	Expr* Expr::makeCast(Expr* expr, ColumnType columnType) 
	{
		Expr* e = new Expr(kExprCast);
		e->columnType = columnType;
		e->expr = expr;
		return e;
	}

	bool Expr::isType(ExprType exprType) const { return exprType == type; }

	bool Expr::isLiteral() const 
	{
		return isType(kExprLiteralInt) || isType(kExprLiteralFloat) || isType(kExprLiteralString) || isType(kExprParameter) ||
			   isType(kExprLiteralNull) || isType(kExprLiteralDate) || isType(kExprLiteralInterval);
	}

	bool Expr::hasAlias() const { return alias != nullptr; }
	bool Expr::hasTable() const { return table != nullptr; }
	const char* Expr::getName() const { return alias ? alias : name; }

	char* substr(const char* source, int from, int to) 
	{
		int len = to - from;
		char* copy = (char*)SAlloc::M(len + 1);
		;
		strncpy(copy, source + from, len);
		copy[len] = '\0';
		return copy;
	}
}  // namespace hsql

namespace hsql {
	SQLStatement::SQLStatement(StatementType type) : hints(nullptr), type_(type) {}

	SQLStatement::~SQLStatement() 
	{
		if(hints) {
			for(Expr* hint : *hints) {
				delete hint;
			}
		}
		delete hints;
	}

	StatementType SQLStatement::type() const { return type_; }
	bool SQLStatement::isType(StatementType type) const { return (type_ == type); }
	bool SQLStatement::is(StatementType type) const { return isType(type); }
	//
	// 
	// 
	CreateStatement::CreateStatement(CreateType type) : SQLStatement(kStmtCreate), type(type), ifNotExists(false), fUnique(false), filePath(nullptr), schema(nullptr),
		tableName(nullptr), indexName(nullptr), indexColumns(nullptr), columns(nullptr), tableConstraints(nullptr), viewColumns(nullptr), select(nullptr)
	{
	}

	CreateStatement::~CreateStatement()
	{
		SAlloc::F(filePath);
		SAlloc::F(schema);
		SAlloc::F(tableName);
		SAlloc::F(indexName);
		delete select;
		if(columns) {
			for(ColumnDefinition* def : *columns) {
				delete def;
			}
			delete columns;
		}
		if(tableConstraints) {
			for(TableConstraint* def : *tableConstraints) {
				delete def;
			}
			delete tableConstraints;
		}
		if(indexColumns) {
			for(char* column : *indexColumns) {
				SAlloc::F(column);
			}
			delete indexColumns;
		}
		if(viewColumns) {
			for(char* column : *viewColumns) {
				SAlloc::F(column);
			}
			delete viewColumns;
		}
	}

	void CreateStatement::setColumnDefsAndConstraints(std::vector<TableElement*>* tableElements) 
	{
		columns = new std::vector<ColumnDefinition*>();
		tableConstraints = new std::vector<TableConstraint*>();
		for(auto tableElem : *tableElements) {
			if(auto* colDef = dynamic_cast<ColumnDefinition*>(tableElem)) {
				columns->emplace_back(colDef);
			}
			else if(auto* tableConstraint = dynamic_cast<TableConstraint*>(tableElem)) {
				tableConstraints->emplace_back(tableConstraint);
			}
		}
	}
	//
	// 
	//
	PrepareStatement::PrepareStatement() : SQLStatement(kStmtPrepare), name(nullptr), query(nullptr) {}

	PrepareStatement::~PrepareStatement() 
	{
		SAlloc::F(name);
		SAlloc::F(query);
	}
	//
	//
	//
	// KeyConstraints
	TableConstraint::TableConstraint(ConstraintType type, std::vector<char*>* columnNames) : type(type), columnNames(columnNames) 
	{
	}

	TableConstraint::~TableConstraint() 
	{
		for(char* def : *columnNames) {
			SAlloc::F(def);
		}
		delete columnNames;
	}

	// ColumnDefinition
	ColumnDefinition::ColumnDefinition(char* name, ColumnType type, std::unordered_set<ConstraintType>* column_constraints)
		: column_constraints(column_constraints), name(name), type(type), nullable(true) {}

	ColumnDefinition::~ColumnDefinition() 
	{
		SAlloc::F(name);
		delete column_constraints;
	}

	ColumnType::ColumnType(DataType data_type, int64_t length, int64_t precision, int64_t scale) : data_type(data_type), length(length), precision(precision), scale(scale) 
	{
	}

	bool operator==(const ColumnType& lhs, const ColumnType& rhs) 
	{
		if(lhs.data_type != rhs.data_type)  
			return false;
		return lhs.length == rhs.length && lhs.precision == rhs.precision && lhs.scale == rhs.scale;
	}

	bool operator!=(const ColumnType& lhs, const ColumnType& rhs) { return !(lhs == rhs); }

	std::ostream& operator<<(std::ostream& stream, const ColumnType& column_type) 
	{
		switch(column_type.data_type) {
			case DataType::UNKNOWN: stream << "UNKNOWN"; break;
			case DataType::INT: stream << "INT"; break;
			case DataType::BIGINT: stream << "BIGINT"; break;
			case DataType::LONG: stream << "LONG"; break;
			case DataType::FLOAT: stream << "FLOAT"; break;
			case DataType::DOUBLE: stream << "DOUBLE"; break;
			case DataType::REAL: stream << "REAL"; break;
			case DataType::CHAR: stream << "CHAR(" << column_type.length << ")"; break;
			case DataType::VARCHAR: stream << "VARCHAR(" << column_type.length << ")"; break;
			case DataType::BINARY: stream << "BINARY(" << column_type.length << ")"; break; // @sobolev
			case DataType::DECIMAL: stream << "DECIMAL"; break;
			case DataType::TEXT: stream << "TEXT"; break;
			case DataType::DATETIME: stream << "DATETIME"; break;
			case DataType::DATE: stream << "DATE"; break;
			case DataType::TIME: stream << "TIME"; break;
			case DataType::SMALLINT: stream << "SMALLINT"; break;
			case DataType::BOOLEAN: stream << "BOOLEAN"; break;
		}
		return stream;
	}

	// DeleteStatement
	DeleteStatement::DeleteStatement() : SQLStatement(kStmtDelete), schema(nullptr), tableName(nullptr), expr(nullptr) {}

	DeleteStatement::~DeleteStatement() 
	{
		SAlloc::F(schema);
		SAlloc::F(tableName);
		delete expr;
	}

	// DropStatement
	DropStatement::DropStatement(DropType type) : SQLStatement(kStmtDrop), type(type), schema(nullptr), name(nullptr), indexName(nullptr) 
	{
	}

	DropStatement::~DropStatement() 
	{
		SAlloc::F(schema);
		SAlloc::F(name);
		SAlloc::F(indexName);
	}

	// AlterStatement and supportive classes

	AlterAction::AlterAction(ActionType type) : type(type) {}

	AlterAction::~AlterAction() = default;

	DropColumnAction::DropColumnAction(char* column_name) : AlterAction(ActionType::DropColumn), columnName(column_name), ifExists(false) 
	{
	}

	DropColumnAction::~DropColumnAction() 
	{ 
		SAlloc::F(columnName); 
	}

	AlterStatement::AlterStatement(char* name, AlterAction* action) : SQLStatement(kStmtAlter), schema(nullptr), ifTableExists(false), name(name), action(action) 
	{
	}

	AlterStatement::~AlterStatement() 
	{
		SAlloc::F(schema);
		SAlloc::F(name);
		delete action;
	}

	// TransactionStatement
	TransactionStatement::TransactionStatement(TransactionCommand command)
		: SQLStatement(kStmtTransaction), command(command) {}

	TransactionStatement::~TransactionStatement() {}

	// ExecuteStatement
	ExecuteStatement::ExecuteStatement() : SQLStatement(kStmtExecute), name(nullptr), parameters(nullptr) {}

	ExecuteStatement::~ExecuteStatement() 
	{
		SAlloc::F(name);
		if(parameters) {
			for(Expr* param : *parameters) {
				delete param;
			}
			delete parameters;
		}
	}

	// ExportStatement
	ExportStatement::ExportStatement(ImportType type) : SQLStatement(kStmtExport), type(type), filePath(nullptr), schema(nullptr), tableName(nullptr), select(nullptr) 
	{
	}

	ExportStatement::~ExportStatement() 
	{
		SAlloc::F(filePath);
		SAlloc::F(schema);
		SAlloc::F(tableName);
		delete select;
	}

	// ImportStatement
	ImportStatement::ImportStatement(ImportType type) : SQLStatement(kStmtImport), type(type), filePath(nullptr),
		schema(nullptr), tableName(nullptr), whereClause(nullptr) 
	{
	}

	ImportStatement::~ImportStatement() 
	{
		SAlloc::F(filePath);
		SAlloc::F(schema);
		SAlloc::F(tableName);
		delete whereClause;
	}

	// InsertStatement
	InsertStatement::InsertStatement(InsertType type) : SQLStatement(kStmtInsert),
		type(type), schema(nullptr), tableName(nullptr), columns(nullptr), values(nullptr), select(nullptr) 
	{
	}

	InsertStatement::~InsertStatement() 
	{
		SAlloc::F(schema);
		SAlloc::F(tableName);
		delete select;
		if(columns) {
			for(char* column : *columns) {
				SAlloc::F(column);
			}
			delete columns;
		}
		if(values) {
			for(Expr* expr : *values) {
				delete expr;
			}
			delete values;
		}
	}

	// ShowStatament
	ShowStatement::ShowStatement(ShowType type) : SQLStatement(kStmtShow), type(type), schema(nullptr), name(nullptr) {}

	ShowStatement::~ShowStatement() 
	{
		SAlloc::F(schema);
		SAlloc::F(name);
	}

	// SelectStatement.h

	// OrderDescription
	OrderDescription::OrderDescription(OrderType type, Expr* expr) : type(type), expr(expr) {}
	OrderDescription::~OrderDescription() { delete expr; }

	// LimitDescription
	LimitDescription::LimitDescription(Expr* limit, Expr* offset) : limit(limit), offset(offset) {}

	LimitDescription::~LimitDescription() 
	{
		delete limit;
		delete offset;
	}

	// GroypByDescription
	GroupByDescription::GroupByDescription() : columns(nullptr), having(nullptr) {}

	GroupByDescription::~GroupByDescription() 
	{
		delete having;
		if(columns) {
			for(Expr* expr : *columns) {
				delete expr;
			}
			delete columns;
		}
	}

	WithDescription::~WithDescription() 
	{
		SAlloc::F(alias);
		delete select;
	}

	// SelectStatement
	SelectStatement::SelectStatement() : SQLStatement(kStmtSelect), fromTable(nullptr), selectDistinct(false), selectList(nullptr),
		whereClause(nullptr), groupBy(nullptr), setOperations(nullptr), order(nullptr), withDescriptions(nullptr), limit(nullptr), lockings(nullptr) 
	{
	}

	SelectStatement::~SelectStatement() 
	{
		delete fromTable;
		delete whereClause;
		delete groupBy;
		delete limit;
		// Delete each element in the select list.
		if(selectList) {
			for(Expr* expr : *selectList) {
				delete expr;
			}
			delete selectList;
		}
		if(order) {
			for(OrderDescription* desc : *order) {
				delete desc;
			}
			delete order;
		}
		if(withDescriptions) {
			for(WithDescription* desc : *withDescriptions) {
				delete desc;
			}
			delete withDescriptions;
		}
		if(setOperations) {
			for(SetOperation* setOperation : *setOperations) {
				delete setOperation;
			}
			delete setOperations;
		}
		if(lockings) {
			for(LockingClause* lockingClause : *lockings) {
				if(lockingClause->tables) {
					for(char* dtable : *lockingClause->tables) {
						SAlloc::F(dtable);
					}
					delete lockingClause->tables;
				}
				delete lockingClause;
			}
			delete lockings;
		}
	}

	// UpdateStatement
	UpdateStatement::UpdateStatement() : SQLStatement(kStmtUpdate), table(nullptr), updates(nullptr), where(nullptr) {}

	UpdateStatement::~UpdateStatement() 
	{
		delete table;
		delete where;
		if(updates) {
			for(UpdateClause* update : *updates) {
				SAlloc::F(update->column);
				delete update->value;
				delete update;
			}
			delete updates;
		}
	}

	// Alias
	Alias::Alias(char* name, std::vector<char*>* columns) : name(name), columns(columns) {}

	Alias::~Alias() 
	{
		SAlloc::F(name);
		if(columns) {
			for(char* column : *columns) {
				SAlloc::F(column);
			}
			delete columns;
		}
	}

	// TableRef
	TableRef::TableRef(TableRefType type) : type(type), schema(nullptr), name(nullptr), alias(nullptr), select(nullptr), list(nullptr), join(nullptr) 
	{
	}

	TableRef::~TableRef() 
	{
		SAlloc::F(schema);
		SAlloc::F(name);
		delete select;
		delete join;
		delete alias;
		if(list) {
			for(TableRef* table : *list) {
				delete table;
			}
			delete list;
		}
	}

	bool TableRef::hasSchema() const { return schema != nullptr; }
	const char* TableRef::getName() const { return alias ? alias->name : name; }

	// JoinDefinition
	JoinDefinition::JoinDefinition() : left(nullptr), right(nullptr), condition(nullptr), type(kJoinInner) {}

	JoinDefinition::~JoinDefinition() 
	{
		delete left;
		delete right;
		delete condition;
	}

	SetOperation::SetOperation() : nestedSelectStatement(nullptr), resultOrder(nullptr), resultLimit(nullptr) {}

	SetOperation::~SetOperation() 
	{
		delete nestedSelectStatement;
		delete resultLimit;
		if(resultOrder) {
			for(OrderDescription* desc : *resultOrder) {
				delete desc;
			}
			delete resultOrder;
		}
	}
	//
	//
	//
	SQLParserResult::SQLParserResult() : isValid_(false), errorMsg_(nullptr) {}

	SQLParserResult::SQLParserResult(SQLStatement* stmt) : isValid_(false), errorMsg_(nullptr) { addStatement(stmt); }

	// Move constructor.
	SQLParserResult::SQLParserResult(SQLParserResult&& moved) { *this = std::forward<SQLParserResult>(moved); }

	SQLParserResult& SQLParserResult::operator=(SQLParserResult&& moved) 
	{
		isValid_ = moved.isValid_;
		errorMsg_ = moved.errorMsg_;
		statements_ = std::move(moved.statements_);
		moved.errorMsg_ = nullptr;
		moved.reset();
		return *this;
	}

	SQLParserResult::~SQLParserResult() { reset(); }
	void SQLParserResult::addStatement(SQLStatement* stmt) { statements_.push_back(stmt); }
	const SQLStatement* SQLParserResult::getStatement(size_t index) const { return statements_[index]; }
	SQLStatement* SQLParserResult::getMutableStatement(size_t index) { return statements_[index]; }
	size_t SQLParserResult::size() const { return statements_.size(); }
	bool SQLParserResult::isValid() const { return isValid_; }
	const char* SQLParserResult::errorMsg() const { return errorMsg_; }
	int SQLParserResult::errorLine() const { return errorLine_; }
	int SQLParserResult::errorColumn() const { return errorColumn_; }
	void SQLParserResult::setIsValid(bool isValid) { isValid_ = isValid; }

	void SQLParserResult::setErrorDetails(char* errorMsg, int errorLine, int errorColumn) 
	{
		errorMsg_ = errorMsg;
		errorLine_ = errorLine;
		errorColumn_ = errorColumn;
	}

	const std::vector<SQLStatement*>& SQLParserResult::getStatements() const { return statements_; }

	std::vector<SQLStatement*> SQLParserResult::releaseStatements() 
	{
		std::vector<SQLStatement*> copy = statements_;
		statements_.clear();
		return copy;
	}

	void SQLParserResult::reset() 
	{
		for(SQLStatement* statement : statements_) {
			delete statement;
		}
		statements_.clear();
		isValid_ = false;
		SAlloc::F(errorMsg_);
		errorMsg_ = nullptr;
		errorLine_ = -1;
		errorColumn_ = -1;
	}

	// Does NOT take ownership.
	void SQLParserResult::addParameter(Expr* parameter) 
	{
		parameters_.push_back(parameter);
		std::sort(parameters_.begin(), parameters_.end(), [](const Expr* a, const Expr* b) { return a->ival < b->ival; });
	}

	const std::vector<Expr*>& SQLParserResult::parameters() { return parameters_; }
	//
	//
	//
	SQLParser::SQLParser() { slfprintf_stderr("SQLParser only has static methods atm! Do not initialize!\n"); }

	// static
	bool SQLParser::parse(const std::string & sql, SQLParserResult * result) 
	{
		yyscan_t scanner;
		if(hsql_lex_init(&scanner)) {
			// Couldn't initialize the lexer.
			slfprintf_stderr("SQLParser: Error when initializing lexer!\n");
			return false;
		}
		else {
			const char* text = sql.c_str();
			YY_BUFFER_STATE state = hsql__scan_string(text, scanner);
			// Parse the tokens.
			// If parsing fails, the result will contain an error object.
			int ret = hsql_parse(result, scanner);
			bool success = (ret == 0);
			result->setIsValid(success);
			hsql__delete_buffer(state, scanner);
			hsql_lex_destroy(scanner);
			return true;
		}
	}

	/*static*/bool SQLParser::parseSQLString(const char* sql, SQLParserResult* result) { return parse(sql, result); }
	/*static*/bool SQLParser::parseSQLString(const std::string& sql, SQLParserResult* result) { return parse(sql, result); }

	/*static*/bool SQLParser::tokenize(const std::string& sql, std::vector<int16_t>* tokens) 
	{
		// Initialize the scanner.
		yyscan_t scanner;
		if(hsql_lex_init(&scanner)) {
			slfprintf_stderr("SQLParser: Error when initializing lexer!\n");
			return false;
		}
		else {
			YY_BUFFER_STATE state = hsql__scan_string(sql.c_str(), scanner);
			YYSTYPE yylval;
			YYLTYPE yylloc;
			// Step through the string until EOF is read.
			// Note: hsql_lex returns int, but we know that its range is within 16 bit.
			for(int16_t token = hsql_lex(&yylval, &yylloc, scanner); token;) {
				tokens->push_back(token);
				token = hsql_lex(&yylval, &yylloc, scanner);
				if(oneof2(token, SQL_IDENTIFIER, SQL_STRING))
					SAlloc::F(yylval.sval);
			}
			hsql__delete_buffer(state, scanner);
			hsql_lex_destroy(scanner);
			return true;
		}
	}
	//
	//
	//
	void printOperatorExpression(Expr* expr, uintmax_t num_indent);
	void printAlias(Alias* alias, uintmax_t num_indent);
	std::ostream& operator<<(std::ostream& os, const OperatorType& op);
	std::ostream& operator<<(std::ostream& os, const DatetimeField& datetime);
	std::ostream& operator<<(std::ostream& os, const FrameBound& frame_bound);
	std::string indent(uintmax_t num_indent) { return std::string(static_cast<uint>(num_indent), '\t'); }
	void inprint(int64_t val, uintmax_t num_indent) { std::cout << indent(num_indent).c_str() << val << "  " << std::endl; }
	void inprint(double val, uintmax_t num_indent) { std::cout << indent(num_indent).c_str() << val << std::endl; }
	void inprint(const char* val, uintmax_t num_indent) { std::cout << indent(num_indent).c_str() << val << std::endl; }
	void inprint(const char* val, const char* val2, uintmax_t num_indent) { std::cout << indent(num_indent).c_str() << val << "->" << val2 << std::endl; }
	void inprintC(char val, uintmax_t num_indent) { std::cout << indent(num_indent).c_str() << val << std::endl; }
	void inprint(const OperatorType& op, uintmax_t num_indent) { std::cout << indent(num_indent) << op << std::endl; }
	void inprint(const ColumnType& colType, uintmax_t num_indent) { std::cout << indent(num_indent) << colType << std::endl; }
	void inprint(const DatetimeField& colType, uintmax_t num_indent) { std::cout << indent(num_indent) << colType << std::endl; }
	void printTableRefInfo(TableRef* table, uintmax_t num_indent) 
	{
		switch(table->type) {
			case kTableName:
				inprint(table->name, num_indent);
				if(table->schema) {
					inprint("Schema", num_indent + 1);
					inprint(table->schema, num_indent + 2);
				}
				break;
			case kTableSelect:
				printSelectStatementInfo(table->select, num_indent);
				break;
			case kTableJoin:
				inprint("Join Table", num_indent);
				inprint("Left", num_indent + 1);
				printTableRefInfo(table->join->left, num_indent + 2);
				inprint("Right", num_indent + 1);
				printTableRefInfo(table->join->right, num_indent + 2);
				inprint("Join Condition", num_indent + 1);
				printExpression(table->join->condition, num_indent + 2);
				break;
			case kTableCrossProduct:
				for(TableRef* tbl : *table->list)  printTableRefInfo(tbl, num_indent);
				break;
		}
		if(table->alias) {
			printAlias(table->alias, num_indent);
		}
	}
	void printAlias(Alias* alias, uintmax_t num_indent) 
	{
		inprint("Alias", num_indent + 1);
		inprint(alias->name, num_indent + 2);
		if(alias->columns) {
			for(char* column : *(alias->columns)) {
				inprint(column, num_indent + 3);
			}
		}
	}
	void printOperatorExpression(Expr* expr, uintmax_t num_indent) 
	{
		if(expr == nullptr) {
			inprint("null", num_indent);
			return;
		}
		inprint(expr->opType, num_indent);
		printExpression(expr->expr, num_indent + 1);
		if(expr->expr2) {
			printExpression(expr->expr2, num_indent + 1);
		}
		else if(expr->exprList) {
			for(Expr* e : *expr->exprList)  printExpression(e, num_indent + 1);
		}
	}

	void printExpression(Expr* expr, uintmax_t num_indent) 
	{
		if(expr) {
			switch(expr->type) {
				case kExprStar:
					inprint("*", num_indent);
					break;
				case kExprColumnRef:
					inprint(expr->name, num_indent);
					if(expr->table) {
						inprint("Table:", num_indent + 1);
						inprint(expr->table, num_indent + 2);
					}
					break;
				// case kExprTableColumnRef: inprint(expr->table, expr->name, num_indent); break;
				case kExprLiteralFloat:
					inprint(expr->fval, num_indent);
					break;
				case kExprLiteralInt:
					inprint(expr->ival, num_indent);
					break;
				case kExprLiteralString:
					inprint(expr->name, num_indent);
					break;
				case kExprLiteralDate:
					inprint(expr->name, num_indent);
					break;
				case kExprLiteralNull:
					inprint("NULL", num_indent);
					break;
				case kExprLiteralInterval:
					inprint("INTERVAL", num_indent);
					inprint(expr->ival, num_indent + 1);
					inprint(expr->datetimeField, num_indent + 1);
					break;
				case kExprFunctionRef:
					inprint(expr->name, num_indent);
					for(Expr* e : *expr->exprList) {
						printExpression(e, num_indent + 1);
					}
					if(expr->windowDescription) {
						printWindowDescription(expr->windowDescription, num_indent + 1);
					}
					break;
				case kExprExtract:
					inprint("EXTRACT", num_indent);
					inprint(expr->datetimeField, num_indent + 1);
					printExpression(expr->expr, num_indent + 1);
					break;
				case kExprCast:
					inprint("CAST", num_indent);
					inprint(expr->columnType, num_indent + 1);
					printExpression(expr->expr, num_indent + 1);
					break;
				case kExprOperator:
					printOperatorExpression(expr, num_indent);
					break;
				case kExprSelect:
					printSelectStatementInfo(expr->select, num_indent);
					break;
				case kExprParameter:
					inprint(expr->ival, num_indent);
					break;
				case kExprArray:
					for(Expr* e : *expr->exprList) {
						printExpression(e, num_indent + 1);
					}
					break;
				case kExprArrayIndex:
					printExpression(expr->expr, num_indent + 1);
					inprint(expr->ival, num_indent);
					break;
				default:
					std::cerr << "Unrecognized expression type " << expr->type << std::endl;
					return;
			}
			if(expr->alias) {
				inprint("Alias", num_indent + 1);
				inprint(expr->alias, num_indent + 2);
			}
		}
	}

	void printOrderBy(const std::vector<OrderDescription*>* expr, uintmax_t num_indent) {
		if(!expr)  return;
		for(const auto& order_description : *expr) {
			printExpression(order_description->expr, num_indent);
			if(order_description->type == kOrderAsc) {
				inprint("ascending", num_indent);
			}
			else {
				inprint("descending", num_indent);
			}
		}
	}

	void printWindowDescription(WindowDescription* window_description, uintmax_t num_indent) {
		inprint("OVER", num_indent);
		if(window_description->partitionList) {
			inprint("PARTITION BY", num_indent + 1);
			for(const auto e : *window_description->partitionList) {
				printExpression(e, num_indent + 2);
			}
		}

		if(window_description->orderList) {
			inprint("ORDER BY", num_indent + 1);
			printOrderBy(window_description->orderList, num_indent + 2);
		}
		std::stringstream stream;
		switch(window_description->frameDescription->type) {
			case kRows: stream << "ROWS"; break;
			case kRange: stream << "RANGE"; break;
			case kGroups: stream << "GROUPS"; break;
		}
		stream << " BETWEEN " << *window_description->frameDescription->start << " AND " << *window_description->frameDescription->end;
		inprint(stream.str().c_str(), num_indent + 1);
	}
	void printSelectStatementInfo(const SelectStatement* stmt, uintmax_t num_indent) 
	{
		inprint("SelectStatement", num_indent);
		inprint("Fields:", num_indent + 1);
		for(Expr* expr : *stmt->selectList)  printExpression(expr, num_indent + 2);

		if(stmt->fromTable) {
			inprint("Sources:", num_indent + 1);
			printTableRefInfo(stmt->fromTable, num_indent + 2);
		}

		if(stmt->whereClause) {
			inprint("Search Conditions:", num_indent + 1);
			printExpression(stmt->whereClause, num_indent + 2);
		}

		if(stmt->groupBy) {
			inprint("GroupBy:", num_indent + 1);
			for(Expr* expr : *stmt->groupBy->columns)  printExpression(expr, num_indent + 2);
			if(stmt->groupBy->having) {
				inprint("Having:", num_indent + 1);
				printExpression(stmt->groupBy->having, num_indent + 2);
			}
		}
		if(stmt->lockings) {
			inprint("Lock Info:", num_indent + 1);
			for(LockingClause* lockingClause : *stmt->lockings) {
				inprint("Type", num_indent + 2);
				if(lockingClause->rowLockMode == RowLockMode::ForUpdate) {
					inprint("FOR UPDATE", num_indent + 3);
				}
				else if(lockingClause->rowLockMode == RowLockMode::ForNoKeyUpdate) {
					inprint("FOR NO KEY UPDATE", num_indent + 3);
				}
				else if(lockingClause->rowLockMode == RowLockMode::ForShare) {
					inprint("FOR SHARE", num_indent + 3);
				}
				else if(lockingClause->rowLockMode == RowLockMode::ForKeyShare) {
					inprint("FOR KEY SHARE", num_indent + 3);
				}
				if(lockingClause->tables) {
					inprint("Target tables:", num_indent + 2);
					for(char* dtable : *lockingClause->tables) {
						inprint(dtable, num_indent + 3);
					}
				}
				if(lockingClause->rowLockWaitPolicy != RowLockWaitPolicy::None) {
					inprint("Waiting policy: ", num_indent + 2);
					if(lockingClause->rowLockWaitPolicy == RowLockWaitPolicy::NoWait)
						inprint("NOWAIT", num_indent + 3);
					else
						inprint("SKIP LOCKED", num_indent + 3);
				}
			}
		}

		if(stmt->setOperations) {
			for(SetOperation* setOperation : *stmt->setOperations) {
				switch(setOperation->setType) {
					case SetType::kSetIntersect: inprint("Intersect:", num_indent + 1); break;
					case SetType::kSetUnion: inprint("Union:", num_indent + 1); break;
					case SetType::kSetExcept: inprint("Except:", num_indent + 1); break;
				}
				printSelectStatementInfo(setOperation->nestedSelectStatement, num_indent + 2);
				if(setOperation->resultOrder) {
					inprint("SetResultOrderBy:", num_indent + 1);
					printOrderBy(setOperation->resultOrder, num_indent + 2);
				}
				if(setOperation->resultLimit) {
					if(setOperation->resultLimit->limit) {
						inprint("SetResultLimit:", num_indent + 1);
						printExpression(setOperation->resultLimit->limit, num_indent + 2);
					}
					if(setOperation->resultLimit->offset) {
						inprint("SetResultOffset:", num_indent + 1);
						printExpression(setOperation->resultLimit->offset, num_indent + 2);
					}
				}
			}
		}
		if(stmt->order) {
			inprint("OrderBy:", num_indent + 1);
			printOrderBy(stmt->order, num_indent + 2);
		}

		if(stmt->limit && stmt->limit->limit) {
			inprint("Limit:", num_indent + 1);
			printExpression(stmt->limit->limit, num_indent + 2);
		}

		if(stmt->limit && stmt->limit->offset) {
			inprint("Offset:", num_indent + 1);
			printExpression(stmt->limit->offset, num_indent + 2);
		}
	}

	void printImportStatementInfo(const ImportStatement* stmt, uintmax_t num_indent) 
	{
		inprint("ImportStatement", num_indent);
		inprint(stmt->filePath, num_indent + 1);
		switch(stmt->type) {
			case ImportType::kImportCSV: inprint("CSV", num_indent + 1); break;
			case ImportType::kImportTbl: inprint("TBL", num_indent + 1); break;
			case ImportType::kImportBinary: inprint("BINARY", num_indent + 1); break;
			case ImportType::kImportAuto: inprint("AUTO", num_indent + 1); break;
		}
		inprint(stmt->tableName, num_indent + 1);
		if(stmt->whereClause) {
			inprint("WHERE:", num_indent + 1);
			printExpression(stmt->whereClause, num_indent + 2);
		}
	}

	void printExportStatementInfo(const ExportStatement* stmt, uintmax_t num_indent) 
	{
		inprint("ExportStatement", num_indent);
		inprint(stmt->filePath, num_indent + 1);
		switch(stmt->type) {
			case ImportType::kImportCSV: inprint("CSV", num_indent + 1); break;
			case ImportType::kImportTbl: inprint("TBL", num_indent + 1); break;
			case ImportType::kImportBinary: inprint("BINARY", num_indent + 1); break;
			case ImportType::kImportAuto: inprint("AUTO", num_indent + 1); break;
		}
		if(stmt->tableName) {
			inprint(stmt->tableName, num_indent + 1);
		}
		else {
			printSelectStatementInfo(stmt->select, num_indent + 1);
		}
	}

	void printCreateStatementInfo(const CreateStatement* stmt, uintmax_t num_indent) {
		inprint("CreateStatement", num_indent);
		inprint(stmt->tableName, num_indent + 1);
		if(stmt->filePath)  inprint(stmt->filePath, num_indent + 1);
	}

	void printInsertStatementInfo(const InsertStatement* stmt, uintmax_t num_indent) {
		inprint("InsertStatement", num_indent);
		inprint(stmt->tableName, num_indent + 1);
		if(stmt->columns) {
			inprint("Columns", num_indent + 1);
			for(char* col_name : *stmt->columns) {
				inprint(col_name, num_indent + 2);
			}
		}
		switch(stmt->type) {
			case kInsertValues:
				inprint("Values", num_indent + 1);
				for(Expr* expr : *stmt->values) {
					printExpression(expr, num_indent + 2);
				}
				break;
			case kInsertSelect:
				printSelectStatementInfo(stmt->select, num_indent + 1);
				break;
		}
	}

	void printTransactionStatementInfo(const TransactionStatement* stmt, uintmax_t num_indent) 
	{
		inprint("TransactionStatement", num_indent);
		switch(stmt->command) {
			case kBeginTransaction: inprint("BEGIN", num_indent + 1); break;
			case kCommitTransaction: inprint("COMMIT", num_indent + 1); break;
			case kRollbackTransaction: inprint("ROLLBACK", num_indent + 1); break;
		}
	}

	void printStatementInfo(const SQLStatement* stmt) 
	{
		switch(stmt->type()) {
			case kStmtSelect: printSelectStatementInfo((const SelectStatement*)stmt, 0); break;
			case kStmtInsert: printInsertStatementInfo((const InsertStatement*)stmt, 0); break;
			case kStmtCreate: printCreateStatementInfo((const CreateStatement*)stmt, 0); break;
			case kStmtImport: printImportStatementInfo((const ImportStatement*)stmt, 0); break;
			case kStmtExport: printExportStatementInfo((const ExportStatement*)stmt, 0); break;
			case kStmtTransaction: printTransactionStatementInfo((const TransactionStatement*)stmt, 0); break;
			default: break;
		}
	}

	std::ostream & operator<<(std::ostream& os, const OperatorType& op) 
	{
		static const std::map<const OperatorType, const std::string> operatorToToken = {
			{kOpNone, "None"},     {kOpBetween, "BETWEEN"},
			{kOpCase, "CASE"},     {kOpCaseListElement, "CASE LIST ELEMENT"},
			{kOpPlus, "+"},        {kOpMinus, "-"},
			{kOpAsterisk, "*"},    {kOpSlash, "/"},
			{kOpPercentage, "%"},  {kOpCaret, "^"},
			{kOpEquals, "="},      {kOpNotEquals, "!="},
			{kOpLess, "<"},        {kOpLessEq, "<="},
			{kOpGreater, ">"},     {kOpGreaterEq, ">="},
			{kOpLike, "LIKE"},     {kOpNotLike, "NOT LIKE"},
			{kOpILike, "ILIKE"},   {kOpAnd, "AND"},
			{kOpOr, "OR"},         {kOpIn, "IN"},
			{kOpConcat, "CONCAT"}, {kOpNot, "NOT"},
			{kOpUnaryMinus, "-"},  {kOpIsNull, "IS NULL"},
			{kOpExists, "EXISTS"}};

		const auto found = operatorToToken.find(op);
		if(found == operatorToToken.cend()) {
			return os << static_cast<uint64_t>(op);
		}
		else {
			return os << (*found).second;
		}
	}

	std::ostream& operator<<(std::ostream& os, const DatetimeField& datetime) 
	{
		static const std::map<const DatetimeField, const std::string> operatorToToken = {
			{kDatetimeNone, "None"}, 
			{kDatetimeSecond, "SECOND"}, 
			{kDatetimeMinute, "MINUTE"}, 
			{kDatetimeHour, "HOUR"},
			{kDatetimeDay, "DAY"},   
			{kDatetimeMonth, "MONTH"},   
			{kDatetimeYear, "YEAR"}
		};
		const auto found = operatorToToken.find(datetime);
		if(found == operatorToToken.cend()) {
			return os << static_cast<uint64_t>(datetime);
		}
		else {
			return os << (*found).second;
		}
	}

	std::ostream& operator<<(std::ostream& os, const FrameBound& frame_bound) 
	{
		if(frame_bound.type == kCurrentRow) {
			os << "CURRENT ROW";
			return os;
		}
		if(frame_bound.unbounded) {
			os << "UNBOUNDED";
		}
		else {
			os << frame_bound.offset;
		}
		os << " ";
		if(frame_bound.type == kPreceding) {
			os << "PRECEDING";
		}
		else {
			os << "FOLLOWING";
		}
		return os;
	}
}  // namespace hsql