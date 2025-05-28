// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

constexpr uint8_t alphabetSize = 26;
constexpr uint8_t asciiOffset = 64;

namespace {
	bool addressIsValid(uint32 row, uint16 column)
	{
		return !(row < 1 || row > OpenXLSX::MAX_ROWS || column < 1 || column > OpenXLSX::MAX_COLS);
	}
}
/**
 * @details The constructor creates a new XLCellReference from a string, e.g. 'A1'. If there's no input,
 * the default reference will be cell A1.
 */
XLCellReference::XLCellReference(const std::string& cellAddress)
{
	if(not cellAddress.empty())  setAddress(cellAddress);
	if(cellAddress.empty() || not addressIsValid(m_row, m_column)) { // 2024-04-25: throw exception on empty string
		throw XLCellAddressError("Cell reference is invalid");
		// ===== 2024-05-27: below code is obsolete due to exception on invalid cellAddress
		// m_row         = 1;
		// m_column      = 1;
		// m_cellAddress = "A1";
	}
}

/**
 * @details This constructor creates a new XLCellReference from a given row and column number, e.g. 1,1 (=A1)
 * @todo consider swapping the arguments.
 */
XLCellReference::XLCellReference(uint32 row, uint16 column)
{
	if(!addressIsValid(row, column))  
		throw XLCellAddressError("Cell reference is invalid");
	setRowAndColumn(row, column);
}

/**
 * @details This constructor creates a new XLCellReference from a row number and the column name (e.g. 1, A)
 * @todo consider swapping the arguments.
 */
XLCellReference::XLCellReference(uint32 row, const std::string& column)
{
	if(!addressIsValid(row, columnAsNumber(column)))  
		throw XLCellAddressError("Cell reference is invalid");
	setRowAndColumn(row, columnAsNumber(column));
}

XLCellReference::XLCellReference(const XLCellReference& other) = default;
XLCellReference::XLCellReference(XLCellReference&& other) noexcept = default;
XLCellReference::~XLCellReference() = default;
XLCellReference& XLCellReference::operator=(const XLCellReference& other) = default;
XLCellReference& XLCellReference::operator=(XLCellReference&& other) noexcept = default;

XLCellReference& XLCellReference::operator++()
{
	if(m_column < MAX_COLS) {
		setColumn(m_column + 1);
	}
	else if(m_column == MAX_COLS && m_row < MAX_ROWS) {
		m_column = 1;
		setRow(m_row + 1);
	}
	else if(m_column == MAX_COLS && m_row == MAX_ROWS) {
		m_column      = 1;
		m_row         = 1;
		m_cellAddress = "A1";
	}
	return *this;
}

XLCellReference XLCellReference::operator++(int)
{
	auto oldRef(*this);
	++(*this);
	return oldRef;
}

XLCellReference& XLCellReference::operator--()
{
	if(m_column > 1) {
		setColumn(m_column - 1);
	}
	else if(m_column == 1 && m_row > 1) {
		m_column = MAX_COLS;
		setRow(m_row - 1);
	}
	else if(m_column == 1 && m_row == 1) {
		m_column      = MAX_COLS;
		m_row         = MAX_ROWS;
		m_cellAddress = "XFD1048576"; // this address represents the very last cell that an excel spreadsheet can reference / support
	}
	return *this;
}

XLCellReference XLCellReference::operator--(int)
{
	auto oldRef(*this);
	--(*this);
	return oldRef;
}
/**
 * @details Returns the m_row property.
 */
uint32 XLCellReference::row() const { return m_row; }
/**
 * @details Sets the row of the XLCellReference objects. If the number is larger than 16384 (the maximum),
 * the row is set to 16384.
 */
void XLCellReference::setRow(uint32 row)
{
	if(!addressIsValid(row, m_column))  
		throw XLCellAddressError("Cell reference is invalid");
	m_row = row;
	m_cellAddress = columnAsString(m_column) + rowAsString(m_row);
}
/**
 * @details Returns the m_column property.
 */
uint16 XLCellReference::column() const { return m_column; }
/**
 * @details Sets the column of the XLCellReference object. If the number is larger than 1048576 (the maximum),
 * the column is set to 1048576.
 */
void XLCellReference::setColumn(uint16 column)
{
	if(!addressIsValid(m_row, column))  
		throw XLCellAddressError("Cell reference is invalid");
	m_column      = column;
	m_cellAddress = columnAsString(m_column) + rowAsString(m_row);
}

/**
 * @details Sets row and column of the XLCellReference object. Checks that row and column is less than
 * or equal to the maximum row and column numbers allowed by Excel.
 */
void XLCellReference::setRowAndColumn(uint32 row, uint16 column)
{
	if(!addressIsValid(row, column))  
		throw XLCellAddressError("Cell reference is invalid");
	m_row         = row;
	m_column      = column;
	m_cellAddress = columnAsString(m_column) + rowAsString(m_row);
}
/**
 * @details Returns the m_cellAddress property.
 */
std::string XLCellReference::address() const { return m_cellAddress; }
/**
 * @details Sets the address of the XLCellReference object, e.g. 'B2'. Checks that row and column is less than
 * or equal to the maximum row and column numbers allowed by Excel.
 */
void XLCellReference::setAddress(const std::string& address)
{
	const auto [fst, snd] = coordinatesFromAddress(address);
	m_row            = fst;
	m_column         = snd;
	m_cellAddress    = address;
}

std::string XLCellReference::rowAsString(uint32 row)
{
#ifdef CHARCONV_ENABLED
	std::array<char, 7> str {};
	const auto*         p = std::to_chars(str.data(), str.data() + str.size(), row).ptr;
	return std::string { str.data(), static_cast<uint16>(p - str.data()) };
#else
	std::string result;
	while(row != 0) {
		int rem = row % 10;
		result += (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		row = row / 10;
	}
	for(unsigned int i = 0; i < result.length() / 2; i++)  
		std::swap(result[i], result[result.length() - i - 1]);
	return result;
#endif
}

uint32 XLCellReference::rowAsNumber(const std::string& row)
{
#ifdef CHARCONV_ENABLED
	uint32 value = 0;
	std::from_chars(row.data(), row.data() + row.size(), value);
	return value;
#else
	return stoul(row);
#endif
}
/**
 * @details Helper method to calculate the column letter from column number.
 */
std::string XLCellReference::columnAsString(uint16 column)
{
	std::string result;
	// ===== If there is one letter in the Column Name:
	if(column <= alphabetSize)  
		result += static_cast<char>(column + asciiOffset);
	// ===== If there are two letters in the Column Name:
	else if(column > alphabetSize && column <= alphabetSize * (alphabetSize + 1)) {
		result += static_cast<char>((column - (alphabetSize + 1)) / alphabetSize + asciiOffset + 1);
		result += static_cast<char>((column - (alphabetSize + 1)) % alphabetSize + asciiOffset + 1);
	}
	// ===== If there are three letters in the Column Name:
	else {
		result += static_cast<char>((column - 703) / (alphabetSize * alphabetSize) + asciiOffset + 1);
		result += static_cast<char>(((column - 703) / alphabetSize) % alphabetSize + asciiOffset + 1);
		result += static_cast<char>((column - 703) % alphabetSize + asciiOffset + 1);
	}
	return result;
}
/**
 * @details Helper method to calculate the column number from column letter.
 * @throws XLInputError
 * @note 2024-06-03: added check for valid address
 */
uint16 XLCellReference::columnAsNumber(const std::string& column)
{
	uint64_t letterCount = 0;
	uint32 colNo = 0;
	for(const auto letter : column) {
		if(letter >= 'A' && letter <= 'Z') { // allow only uppercase letters
			++letterCount;
			colNo = colNo * 26 + (letter - 'A' + 1);
		}
		else
			break;
	}
	// ===== If the full string was decoded and colNo is within allowed range [1;MAX_COLS]
	if(letterCount == column.length() && colNo > 0 && colNo <= MAX_COLS)
		return static_cast<uint16>(colNo);
	throw XLInputError("XLCellReference::columnAsNumber - column \"" + column + "\" is invalid");
	/* 2024-06-19 OBSOLETE CODE:
	   // uint16 result = 0;
	   // uint16 factor = 1;
	   //
	   // for (int16 i = static_cast<int16>(column.size() - 1); i >= 0; --i) {
	   //     result += static_cast<uint16>((column[static_cast<uint64_t>(i)] - asciiOffset) * factor);
	   //     factor *= alphabetSize;
	   // }
	   //
	   // return result;
	 */
}

/**
 * @details Helper method for calculating the coordinates from the cell address.
 * @throws XLInputError
 * @note 2024-06-03: added check for valid address
 */
XLCoordinates XLCellReference::coordinatesFromAddress(const std::string& address)
{
	uint64_t letterCount = 0;
	uint32 colNo = 0;
	for(const auto letter : address) {
		if(letter >= 'A' && letter <= 'Z') { // allow only uppercase letters
			++letterCount;
			colNo = colNo * 26 + (letter - 'A' + 1);
		}
		else
			break;
	}
	// ===== If address contains between 1 and 3 letters and has at least 1 more character for the row
	if(colNo > 0 && colNo <= MAX_COLS && address.length() > letterCount) {
		size_t pos = static_cast<size_t>(letterCount);
		uint64_t rowNo = 0;
		for(; pos < address.length() && std::isdigit(address[pos]); ++pos) // check digits
			rowNo = rowNo * 10 + (address[pos] - '0');
		if(pos == address.length() && rowNo <= MAX_ROWS) // full address was < 4 letters + only digits
			return std::make_pair(rowNo, colNo);
	}
	throw XLInputError("XLCellReference::coordinatesFromAddress - address \"" + address + "\" is invalid");

	/* 2024-06-19 OBSOLETE CODE
	   // auto it = std::find_if(address.begin(), address.end(), ::isdigit);
	   // auto columnPart = std::string(address.begin(), it);
	   // auto rowPart = std::string(it, address.end());
	   //
	   // return std::make_pair(rowAsNumber(rowPart), columnAsNumber(columnPart));
	 */
}
