// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

namespace OpenXLSX {
	XLRow::XLRow() : m_rowNode(nullptr), m_sharedStrings(XLSharedStringsDefaulted), m_rowDataProxy(this, m_rowNode.get())
	{
	}
	/**
	 * @details Constructs a new XLRow object from information in the underlying XML file. A pointer to the corresponding
	 * node in the underlying XML file must be provided.
	 */
	XLRow::XLRow(const XMLNode& rowNode, const XLSharedStrings& sharedStrings) : m_rowNode(std::make_unique<XMLNode>(rowNode)),
		m_sharedStrings(sharedStrings), m_rowDataProxy(this, m_rowNode.get())
	{
	}

	XLRow::XLRow(const XLRow& other) : m_rowNode(other.m_rowNode ? std::make_unique<XMLNode>(*other.m_rowNode) : nullptr),
		m_sharedStrings(other.m_sharedStrings), m_rowDataProxy(this, m_rowNode.get())
	{
	}
	/**
	 * @details Because the m_rowDataProxy variable is tied to an exact XLRow object, the move operation is
	 * not a 'pure' move, as a new XLRowDataProxy has to be constructed.
	 */
	XLRow::XLRow(XLRow&& other) noexcept : m_rowNode(std::move(other.m_rowNode)), m_sharedStrings(std::move(other.m_sharedStrings)), m_rowDataProxy(this, m_rowNode.get())
	{
	}

	XLRow::~XLRow() = default;

	XLRow& XLRow::operator=(const XLRow& other)
	{
		if(&other != this) {
			auto temp = XLRow(other);
			std::swap(*this, temp);
		}
		return *this;
	}
	/**
	 * @details Because the m_rowDataProxy variable is tied to an exact XLRow object, the move operation is
	 * not a 'pure' move, as a new XLRowDataProxy has to be constructed.
	 */
	XLRow& XLRow::operator=(XLRow&& other) noexcept
	{
		if(&other != this) {
			m_rowNode       = std::move(other.m_rowNode);
			m_sharedStrings = std::move(other.m_sharedStrings);
			m_rowDataProxy  = XLRowDataProxy(this, m_rowNode.get());
		}
		return *this;
	}

	bool XLRow::empty() const { return (!m_rowNode) || m_rowNode->empty(); }
	XLRow::operator bool() const { return m_rowNode && (not m_rowNode->empty() ); }
	/**
	 * @details Returns the m_height member by getValue.
	 */
	double XLRow::height() const { return m_rowNode->attribute("ht").as_double(15.0); }
	/**
	 * @details Set the height of the row. This is done by setting the getValue of the 'ht' attribute and setting the
	 * 'customHeight' attribute to true.
	 */
	void XLRow::setHeight(float height)
	{
		// Set the 'ht' attribute for the Cell. If it does not exist, create it.
		if(m_rowNode->attribute("ht").empty())
			m_rowNode->append_attribute("ht") = height;
		else
			m_rowNode->attribute("ht").set_value(height);
		// Set the 'customHeight' attribute. If it does not exist, create it.
		if(m_rowNode->attribute("customHeight").empty())
			m_rowNode->append_attribute("customHeight") = 1;
		else
			m_rowNode->attribute("customHeight").set_value(1);
	}
	/**
	 * @details Return the m_descent member by getValue.
	 */
	float XLRow::descent() const { return m_rowNode->attribute("x14ac:dyDescent").as_float(0.25); }
	/**
	 * @details Set the descent by setting the 'x14ac:dyDescent' attribute in the XML file
	 */
	void XLRow::setDescent(float descent)
	{
		// Set the 'x14ac:dyDescent' attribute. If it does not exist, create it.
		if(m_rowNode->attribute("x14ac:dyDescent").empty())
			m_rowNode->append_attribute("x14ac:dyDescent") = descent;
		else
			m_rowNode->attribute("x14ac:dyDescent") = descent;
	}
	/**
	 * @details Determine if the row is hidden or not.
	 */
	bool XLRow::isHidden() const { return m_rowNode->attribute("hidden").as_bool(false); }
	/**
	 * @details Set the hidden state by setting the 'hidden' attribute to true or false.
	 */
	void XLRow::setHidden(bool state)
	{
		// Set the 'hidden' attribute. If it does not exist, create it.
		if(m_rowNode->attribute("hidden").empty())
			m_rowNode->append_attribute("hidden") = static_cast<int>(state);
		else
			m_rowNode->attribute("hidden").set_value(static_cast<int>(state));
	}
	/**
	 * @details
	 * @note 2024-08-18: changed return type of rowNumber to uint32
	 *       CAUTION: there is no validity check on the underlying XML (nor was there ever one in case a value was inconsistent with OpenXLSX::MAX_ROWS)
	 */
	uint32 XLRow::rowNumber() const { return static_cast<uint32>(m_rowNode->attribute("r").as_ullong()); }
	/**
	 * @details Get the number of cells in the row, by returning the size of the m_cells vector.
	 */
	uint16 XLRow::cellCount() const
	{
		const auto node = m_rowNode->last_child_of_type(pugi::node_element);
		return node.empty() ? 0 : XLCellReference(node.attribute("r").value()).column();
	}

	XLRowDataProxy & XLRow::values() { return m_rowDataProxy; }
	const XLRowDataProxy & XLRow::values() const { return m_rowDataProxy; }

	XLRowDataRange XLRow::cells() const
	{
		const XMLNode node = m_rowNode->last_child_of_type(pugi::node_element);
		if(node.empty())
			return XLRowDataRange()/*empty range*/;
		return XLRowDataRange(*m_rowNode, 1, XLCellReference(node.attribute("r").value()).column(), m_sharedStrings.get());
	}

	XLRowDataRange XLRow::cells(uint16 cellCount) const { return XLRowDataRange(*m_rowNode, 1, cellCount, m_sharedStrings.get()); }

	XLRowDataRange XLRow::cells(uint16 firstCell, uint16 lastCell) const
	{
		return XLRowDataRange(*m_rowNode, firstCell, lastCell, m_sharedStrings.get());
	}
	/**
	 * @details Find & return a cell in indicated column
	 */
	XLCell XLRow::findCell(uint16 columnNumber)
	{
		if(m_rowNode->empty())  
			return XLCell{};
		XMLNode cellNode = m_rowNode->last_child_of_type(pugi::node_element);
		// ===== If there are no cells in the current row, or the requested cell is beyond the last cell in the row...
		if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber))
			return XLCell{}; // fail

		// ===== If the requested node is closest to the end, start from the end and search backwards...
		if(XLCellReference(cellNode.attribute("r").value()).column() - columnNumber < columnNumber) {
			while(not cellNode.empty() && (XLCellReference(cellNode.attribute("r").value()).column() > columnNumber))
				cellNode = cellNode.previous_sibling_of_type(pugi::node_element);
			// ===== If the backwards search failed to locate the requested cell
			if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber))
				return XLCell{}; // fail
		}
		// ===== Otherwise, start from the beginning
		else {
			// ===== At this point, it is guaranteed that there is at least one node_element in the row that is not empty.
			cellNode = m_rowNode->first_child_of_type(pugi::node_element);
			// ===== It has been verified above that the requested columnNumber is <= the column number of the last node_element, therefore this loop will halt:
			while(XLCellReference(cellNode.attribute("r").value()).column() < columnNumber)
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
			// ===== If the forwards search failed to locate the requested cell
			if(XLCellReference(cellNode.attribute("r").value()).column() > columnNumber)
				return XLCell{}; // fail
		}
		return XLCell(cellNode, m_sharedStrings.get());
	}
	/**
	 * @details Determine the value of the style attribute "s" - if attribute does not exist, return default value
	 */
	XLStyleIndex XLRow::format() const { return m_rowNode->attribute("s").as_uint(XLDefaultCellFormat); }
	/**
	 * @brief Set the row style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
	 *        If the style attribute "s" does not exist, create it
	 */
	bool XLRow::setFormat(XLStyleIndex cellFormatIndex)
	{
		XMLAttribute customFormatAtt = m_rowNode->attribute("customFormat");
		if(cellFormatIndex != XLDefaultCellFormat) {
			if(customFormatAtt.empty()) {
				customFormatAtt = m_rowNode->append_attribute("customFormat");
				if(customFormatAtt.empty())  return false;// fail if missing customFormat attribute could not be created
			}
			customFormatAtt.set_value("true");
		}
		else { // cellFormatIndex is XLDefaultCellFormat
			if(not customFormatAtt.empty())  m_rowNode->remove_attribute(customFormatAtt);// an existing customFormat attribute should be deleted
		}
		XMLAttribute styleAtt = m_rowNode->attribute("s");
		if(styleAtt.empty()) {
			styleAtt = m_rowNode->append_attribute("s");
			if(styleAtt.empty())  return false;
		}
		styleAtt.set_value(cellFormatIndex);
		return true;
	}

	bool XLRow::isEqual(const XLRow& lhs, const XLRow& rhs)
	{
		// 2024-05-28 BUGFIX: (!lhs.m_rowNode && rhs.m_rowNode) was not evaluated, triggering a segmentation fault on dereferencing
		if(static_cast<bool>(lhs.m_rowNode) != static_cast<bool>(rhs.m_rowNode))  
			return false;
		// ===== If execution gets here, row nodes are BOTH valid or BOTH invalid / empty
		if(not lhs.m_rowNode)  
			return true;    // checking one for being empty is enough to know both are empty
		return *lhs.m_rowNode == *rhs.m_rowNode;
	}

	bool XLRow::isLessThan(const XLRow& lhs, const XLRow& rhs) { return *lhs.m_rowNode < *rhs.m_rowNode; }
	//
	//
	//
	XLRowIterator::XLRowIterator(const XLRowRange& rowRange, XLIteratorLocation loc) : m_dataNode(std::make_unique<XMLNode>(*rowRange.m_dataNode)),
		m_firstRow(rowRange.m_firstRow), m_lastRow(rowRange.m_lastRow), m_currentRow(), m_sharedStrings(rowRange.m_sharedStrings), m_endReached(false),
		m_hintRow(), m_hintRowNumber(0), m_currentRowStatus(XLNotLoaded), m_currentRowNumber(0)
	{
		if(loc == XLIteratorLocation::End)
			m_endReached = true;
		else {
			m_currentRowNumber = m_firstRow;
		}
	}

	XLRowIterator::~XLRowIterator() = default;

	XLRowIterator::XLRowIterator(const XLRowIterator& other) : m_dataNode(std::make_unique<XMLNode>(*other.m_dataNode)), m_firstRow(other.m_firstRow),
		m_lastRow(other.m_lastRow), m_currentRow(other.m_currentRow), m_sharedStrings(other.m_sharedStrings), m_endReached(other.m_endReached),
		m_hintRow(other.m_hintRow), m_hintRowNumber(other.m_hintRowNumber), m_currentRowStatus(other.m_currentRowStatus), m_currentRowNumber(other.m_currentRowNumber)
	{
	}

	XLRowIterator::XLRowIterator(XLRowIterator&& other) noexcept = default;

	XLRowIterator& XLRowIterator::operator=(const XLRowIterator& other)
	{
		if(&other != this) {
			auto temp = XLRowIterator(other);
			std::swap(*this, temp);
		}
		return *this;
	}

	XLRowIterator& XLRowIterator::operator=(XLRowIterator&& other) noexcept = default;
	/**
	 * @brief update m_currentRow by fetching (or inserting) a row at m_currentRowNumber
	 */
	void XLRowIterator::updateCurrentRow(bool createIfMissing)
	{
		// ===== Quick exit checks - can't be true when m_endReached
		if(m_currentRowStatus == XLLoaded)  
			return;                           // nothing to do, row is already loaded
		if(!createIfMissing && m_currentRowStatus == XLNoSuchRow)  
			return;    // nothing to do, row has already been determined as missing
		// ===== At this stage, m_currentRowStatus is XLUnloaded or XLNoSuchRow and createIfMissing == true
		if(m_endReached)
			throw XLInputError("XLRowIterator updateCurrentRow: iterator should not be dereferenced when endReached() == true");
		// ===== Row needs to be updated
		if(m_hintRow.empty()) {   // no hint has been established: fetch first row node the "tedious" way
			if(createIfMissing)  // getRowNode creates missing rows
				m_currentRow = XLRow(getRowNode(*m_dataNode, m_currentRowNumber), m_sharedStrings.get());
			else                // findRowNode returns an empty row for missing rows
				m_currentRow = XLRow(findRowNode(*m_dataNode, m_currentRowNumber), m_sharedStrings.get());
		}
		else {
			// ===== Find or create, and fetch an XLRow at m_currentRowNumber
			if(m_currentRowNumber > m_hintRowNumber) {
				// ===== Start from m_hintRow and search forwards...
				XMLNode rowNode = m_hintRow.next_sibling_of_type(pugi::node_element);
				uint32 rowNo = 0;
				while(not rowNode.empty()) {
					rowNo = static_cast<uint32>(rowNode.attribute("r").as_ullong());
					if(rowNo >= m_currentRowNumber)
						break;// if desired row was reached / passed, break before incrementing rowNode
					rowNode = rowNode.next_sibling_of_type(pugi::node_element);
				}
				if(rowNo != m_currentRowNumber)  
					rowNode = XMLNode{};// if a higher row number was found, set empty node (means: "missing")
				// ===== Create missing row node if createIfMissing == true
				if(createIfMissing && rowNode.empty()) {
					rowNode = m_dataNode->insert_child_after("row", m_hintRow);
					rowNode.append_attribute("r").set_value(m_currentRowNumber);
				}
				if(rowNode.empty()) // if row could not be found / created
					m_currentRow = XLRow{}; // make sure m_currentRow is set to an empty cell
				else
					m_currentRow = XLRow(rowNode, m_sharedStrings.get());
			}
			else
				throw XLInternalError("XLRowIterator::updateCurrentRow: an internal error occured (m_currentRowNumber <= m_hintRowNumber)");
		}

		if(m_currentRow.empty())    // if row is confirmed missing
			m_currentRowStatus = XLNoSuchRow; // mark this status for further calls to updateCurrentRow()
		else {
			// ===== If the current row exists, update the hints
			m_hintRow          = *m_currentRow.m_rowNode;// don't store a full XLRow, just the XMLNode, for better performance
			m_hintRowNumber    = m_currentRowNumber;
			m_currentRowStatus = XLLoaded;              // mark row status for further calls to updateCurrentRow()
		}
	}

	XLRowIterator& XLRowIterator::operator++()        // 2024-04-29: patched for whitespace
	{
		if(m_endReached)
			throw XLInputError("XLRowIterator: tried to increment beyond end operator");
		if(m_currentRowNumber < m_lastRow)
			++m_currentRowNumber;
		else
			m_endReached = true;
		m_currentRowStatus = XLNotLoaded; // trigger a new attempt to locate / create the row via updateRowCell
		return *this;
	}

	XLRowIterator XLRowIterator::operator++(int)
	{
		auto oldIter(*this);
		++(*this);
		return oldIter;
	}

	XLRow& XLRowIterator::operator*()
	{
		updateCurrentRow(XLCreateIfMissing);
		return m_currentRow;
	}

	XLRowIterator::pointer XLRowIterator::operator->()
	{
		updateCurrentRow(XLCreateIfMissing);
		return &m_currentRow;
	}

	bool XLRowIterator::operator==(const XLRowIterator& rhs) const
	{
		if(m_endReached && rhs.m_endReached)  return true;    // If both iterators are end iterators

		if(m_currentRowNumber != rhs.m_currentRowNumber)      // If iterators point to a different row
			return false;                                     // that means no match

		// CAUTION: for performance reasons, disabled all checks whether this and rhs are iterators on the same worksheet & row range
		return true;

		// if (*m_dataNode != *rhs.m_dataNode) return false;     // TBD: iterators over different worksheets may never match
		// TBD if iterators shall be considered not equal if they were created on different XLRowRanges
		// this would require checking the m_firstRow and m_lastRow, potentially costing CPU time

		// return m_currentRow == rhs.m_currentRow;   // match only if row nodes are equal
		// CAUTION: in the current code, that means iterators that point to the same row in different worksheets,
		// and rows that do not exist in both sheets, will be considered equal
	}

	bool XLRowIterator::operator!=(const XLRowIterator& rhs) const { return !(*this == rhs); }

	XLRowIterator::operator bool() const { return false; }

	bool XLRowIterator::rowExists()
	{
		// ===== Update m_currentRow once so that rowExists will always test the correct cell (an empty row if current row doesn't exist)
		updateCurrentRow(XLDoNotCreateIfMissing);
		return not m_currentRow.empty();
	}
	//
	//
	//
	XLRowRange::XLRowRange(const XMLNode& dataNode, uint32 first, uint32 last, const XLSharedStrings& sharedStrings) : 
		m_dataNode(std::make_unique<XMLNode>(dataNode)), m_firstRow(first), m_lastRow(last), m_sharedStrings(sharedStrings)
	{
	}

	XLRowRange::XLRowRange(const XLRowRange& other) : m_dataNode(std::make_unique<XMLNode>(*other.m_dataNode)), m_firstRow(other.m_firstRow),
		m_lastRow(other.m_lastRow), m_sharedStrings(other.m_sharedStrings)
	{
	}

	XLRowRange::XLRowRange(XLRowRange&& other) noexcept = default;
	XLRowRange::~XLRowRange() = default;

	XLRowRange& XLRowRange::operator=(const XLRowRange& other)
	{
		if(&other != this) {
			auto temp = XLRowRange(other);
			std::swap(*this, temp);
		}

		return *this;
	}

	XLRowRange& XLRowRange::operator=(XLRowRange&& other) noexcept = default;
	uint32 XLRowRange::rowCount() const { return m_lastRow - m_firstRow + 1; }
	XLRowIterator XLRowRange::begin() { return XLRowIterator(*this, XLIteratorLocation::Begin); }
	XLRowIterator XLRowRange::end() { return XLRowIterator(*this, XLIteratorLocation::End); }
}
