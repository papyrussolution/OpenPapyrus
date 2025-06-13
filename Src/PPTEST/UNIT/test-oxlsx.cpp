// text-oxlsx.cpp
// Copyrigh (C) A.Sobolev 2025
//
#include <pp.h>
#pragma hdrstop
#include <..\osf\OpenXLSX\SRC\headers\OpenXLSX.hpp>
//
//
//
using namespace OpenXLSX;

SLTEST_R(OpenXLSX)
{
	SString temp_buf;
#if 1 // @construction {
    {
        //
        // XLCell
        //
        (temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("testXLCell.xlsx");
        std::string test_file_name(temp_buf.cptr());
        {
            // Default Constructor
            XLCell cell;
            SLCHECK_Z(cell);
            //SLCHECK_THROWS(cell.cellReference());
            SLCHECK_Z(cell.hasFormula());
            //SLCHECK_THROWS(cell.formula());
            //SLCHECK_THROWS(cell.formula().set("=1+1"));
            //SLCHECK_THROWS(cell.value());

            const auto copy = cell;
            SLCHECK_Z(copy);
            //SLCHECK_THROWS(copy.cellReference());
            SLCHECK_Z(copy.hasFormula());
            //SLCHECK_THROWS(copy.formula());
            //SLCHECK_THROWS(copy.value());
        }
        {
            // Create from worksheet
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            XLWorksheet wks = doc.workbook().sheet(1);
            auto cell = wks.cell("A1");
            cell.value() = 42;
            SLCHECK_NZ(cell);
            SLCHECK_NZ(cell.cellReference().address() == "A1");
            SLCHECK_Z(cell.hasFormula());
            SLCHECK_NZ(cell.value().get<int>() == 42);
        }
        {
            // Copy constructor
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            XLWorksheet wks = doc.workbook().sheet(1);
            auto cell = wks.cell("A1");
            cell.value() = 42;
            auto copy = cell;
            SLCHECK_NZ(copy);
            SLCHECK_NZ(copy.cellReference().address() == "A1");
            SLCHECK_Z(copy.hasFormula());
            SLCHECK_NZ(copy.value().get<int>() == 42);
        }
        {
            // Move constructor
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            XLWorksheet wks = doc.workbook().sheet(1);
            auto cell = wks.cell("A1");
            cell.value() = 42;
            XLCell copy = std::move(cell);
            SLCHECK_NZ(copy);
            SLCHECK_NZ(copy.cellReference().address() == "A1");
            SLCHECK_Z(copy.hasFormula());
            SLCHECK_NZ(copy.value().get<int>() == 42);
        }
        {
            // Copy assignment operator
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            XLWorksheet wks = doc.workbook().sheet(1);
            auto cell = wks.cell("A1");
            cell.value() = 42;
            XLCell copy;
            copy = cell;
            SLCHECK_NZ(copy);
            SLCHECK_NZ(copy.cellReference().address() == "A1");
            SLCHECK_Z(copy.hasFormula());
            SLCHECK_NZ(copy.value().get<int>() == 42);
        }
        {
            // Move assignment operator
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            XLWorksheet wks = doc.workbook().sheet(1);
            auto cell = wks.cell("A1");
            cell.value() = 42;
            XLCell copy;
            copy = std::move(cell);
            SLCHECK_NZ(copy);
            SLCHECK_NZ(copy.cellReference().address() == "A1");
            SLCHECK_Z(copy.hasFormula());
            SLCHECK_NZ(copy.value().get<int>() == 42);
        }
        {
            // Setters and Getters
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            XLWorksheet wks = doc.workbook().sheet(1);
            auto cell = wks.cell("A1");
            cell.formula().set("=1+1");
            cell.value() = 42;
            SLCHECK_NZ(cell.hasFormula());
            SLCHECK_NZ(cell.formula().get() == "=1+1");
            SLCHECK_NZ(cell.value().get<int>() == 42);
            const auto copy = cell;
            SLCHECK_NZ(copy.hasFormula());
            SLCHECK_NZ(copy.formula().get() == "=1+1");
            SLCHECK_NZ(copy.value().get<int>() == 42);
        }
        {
            // Relational operators
            auto doc = XLDocument();
            doc.create(test_file_name, XLForceOverwrite);
            auto wks = doc.workbook().worksheet("Sheet1");
            auto cell1 = wks.cell("B2");
            auto cell2 = wks.cell("B2");
            auto cell3 = wks.cell("C3");
            SLCHECK_NZ(cell1 == cell2);
            SLCHECK_Z(cell1 == cell3);
            SLCHECK_NZ(cell1 != cell3);
            SLCHECK_Z(cell1 != cell2);
        }
        {
            // Offset function
            auto doc = XLDocument();
            doc.create(test_file_name, XLForceOverwrite);
            auto wks = doc.workbook().worksheet("Sheet1");
            auto cell1 = wks.cell("B2");
            cell1.value() = "Value1";
            auto cell2 = cell1.offset(1,1);
            cell2.value() = "Value2";
            SLCHECK_NZ(cell1.value().get<std::string>() == "Value1");
            SLCHECK_NZ(cell2.value().get<std::string>() == "Value2");
            SLCHECK_NZ(cell2.cellReference().address() == "C3");
            doc.save();
        }
    }
    {
        //
        // XLCellRange
        //
        (temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("testXLCellRange.xlsx");
        std::string test_file_name(temp_buf.cptr());
        {
            XLDocument doc;
            doc.create(test_file_name, XLForceOverwrite);
            auto wks = doc.workbook().worksheet("Sheet1");
            {
                // Constructor
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto cl : rng) 
                    cl.value() = "Value";
                doc.save();
                SLCHECK_NZ(wks.cell("B2").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("C2").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("D2").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("B3").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("C3").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("D3").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("B4").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("C4").value().get<std::string>() == "Value");
                SLCHECK_NZ(wks.cell("D4").value().get<std::string>() == "Value");
            }
            {
                // Copy Constructor 
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto cl : rng) 
                    cl.value() = "Value";
                doc.save();
                XLCellRange rng2 = rng;
                for(auto cl : rng2) 
                    cl.value() = "Value2";
                doc.save();
                SLCHECK_NZ(wks.cell("B2").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("C2").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("D2").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("B3").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("C3").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("D3").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("B4").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("C4").value().get<std::string>() == "Value2");
                SLCHECK_NZ(wks.cell("D4").value().get<std::string>() == "Value2");
            }
            {
                // Move Constructor
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto cl : rng) 
                    cl.value() = "Value";
                doc.save();
                XLCellRange rng2 {std::move(rng)};
                for(auto cl : rng2) 
                    cl.value() = "Value3";
                doc.save();
                SLCHECK_NZ(wks.cell("B2").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("C2").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("D2").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("B3").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("C3").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("D3").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("B4").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("C4").value().get<std::string>() == "Value3");
                SLCHECK_NZ(wks.cell("D4").value().get<std::string>() == "Value3");
            }
            {
                // Copy Assignment
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto cl : rng) 
                    cl.value() = "Value";
                doc.save();
                XLCellRange rng2 = wks.range();
                rng2 = rng;
                for(auto cl : rng2) 
                    cl.value() = "Value4";
                doc.save();
                SLCHECK_NZ(wks.cell("B2").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("C2").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("D2").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("B3").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("C3").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("D3").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("B4").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("C4").value().get<std::string>() == "Value4");
                SLCHECK_NZ(wks.cell("D4").value().get<std::string>() == "Value4");
            }
            {
                // Move Assignment
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto cl : rng)
                    cl.value() = "Value";
                doc.save();
                XLCellRange rng2 = wks.range();
                rng2 = std::move(rng);
                for(auto cl : rng2)
                    cl.value() = "Value5";
                doc.save();

                SLCHECK_NZ(wks.cell("B2").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("C2").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("D2").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("B3").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("C3").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("D3").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("B4").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("C4").value().get<std::string>() == "Value5");
                SLCHECK_NZ(wks.cell("D4").value().get<std::string>() == "Value5");
            }
            {
                // Functions
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto iter = rng.begin(); iter != rng.end(); iter++)
                    iter->value() = "Value";
                doc.save();
                SLCHECK_NZ(rng.numColumns() == 3);
                SLCHECK_NZ(rng.numRows() == 3);
                rng.clear();
                SLCHECK_NZ(wks.cell("B2").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("C2").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("D2").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("B3").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("C3").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("D3").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("B4").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("C4").value().get<std::string>().empty());
                SLCHECK_NZ(wks.cell("D4").value().get<std::string>().empty());
            }
            {
                // XLCellIterator
                auto rng = wks.range(XLCellReference("B2"), XLCellReference("D4"));
                for(auto iter = rng.begin(); iter != rng.end(); iter++)
                    iter->value() = "Value";
                auto begin = rng.begin();
                SLCHECK_NZ(begin->cellReference().address() == "B2");
                auto iter2 = ++begin;
                SLCHECK_NZ(iter2->cellReference().address() == "C2");
                XLCellIterator iter3 = std::move(++begin) ;
                SLCHECK_NZ(iter3->cellReference().address() == "D2");
                auto iter4 = rng.begin();
                iter4 = ++iter3;
                SLCHECK_NZ(iter4->cellReference().address() == "B3");
                auto iter5 = rng.begin();
                iter5 = std::move(++iter3);
                SLCHECK_NZ(iter5->cellReference().address() == "C3");
                auto it1 = rng.begin();
                auto it2 = rng.begin();
                auto it3 = rng.end();
                SLCHECK_NZ(it1 == it2);
                SLCHECK_Z(it1 != it2);
                SLCHECK_NZ(it1 != it3);
                SLCHECK_Z(it1 == it3);
                SLCHECK_NZ(std::distance(it1, it3) == 9);
            }
        }
    }
    {
        //
        // XLCellReference
        //
        {
            // Constructors
            auto ref = XLCellReference();
            SLCHECK_NZ(ref.address() == "A1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 1);

            ref = XLCellReference("B2");
            SLCHECK_NZ(ref.address() == "B2");
            SLCHECK_NZ(ref.row() == 2);
            SLCHECK_NZ(ref.column() == 2);

            ref = XLCellReference(3,3);
            SLCHECK_NZ(ref.address() == "C3");
            SLCHECK_NZ(ref.row() == 3);
            SLCHECK_NZ(ref.column() == 3);

            ref = XLCellReference(4, "D");
            SLCHECK_NZ(ref.address() == "D4");
            SLCHECK_NZ(ref.row() == 4);
            SLCHECK_NZ(ref.column() == 4);

            ref = XLCellReference("AA1");
            SLCHECK_NZ(ref.address() == "AA1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 27);

            ref = XLCellReference(1,27);
            SLCHECK_NZ(ref.address() == "AA1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 27);

            ref = XLCellReference("XFD1");
            SLCHECK_NZ(ref.address() == "XFD1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == MAX_COLS);

            ref = XLCellReference("A1048576");
            SLCHECK_NZ(ref.address() == "A1048576");
            SLCHECK_NZ(ref.row() == MAX_ROWS);
            SLCHECK_NZ(ref.column() == 1);

            SLCHECK_THROWS(XLCellReference(0,0));
            SLCHECK_THROWS(XLCellReference("XFE1"));
            SLCHECK_THROWS(XLCellReference("A1048577"));
            SLCHECK_THROWS(XLCellReference(MAX_ROWS + 1,MAX_COLS + 1));
            SLCHECK_THROWS(XLCellReference(MAX_ROWS + 1,"XFE"));

            ref = XLCellReference("B2");
            auto c_ref = ref;
            SLCHECK_NZ(c_ref.address() == "B2");
            SLCHECK_NZ(c_ref.row() == 2);
            SLCHECK_NZ(c_ref.column() == 2);

            ref = XLCellReference("C3");
            c_ref = ref;
            SLCHECK_NZ(c_ref.address() == "C3");
            SLCHECK_NZ(c_ref.row() == 3);
            SLCHECK_NZ(c_ref.column() == 3);

            ref = XLCellReference("B2");
            auto m_ref = std::move(ref);
            SLCHECK_NZ(m_ref.address() == "B2");
            SLCHECK_NZ(m_ref.row() == 2);
            SLCHECK_NZ(m_ref.column() == 2);

            ref = XLCellReference("C3");
            m_ref = std::move(ref);
            SLCHECK_NZ(m_ref.address() == "C3");
            SLCHECK_NZ(m_ref.row() == 3);
            SLCHECK_NZ(m_ref.column() == 3);
        }
        {
            // Increment and Decrement
            auto ref = XLCellReference();
            SLCHECK_NZ(ref.address() == "A1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 1);

            ++ref;
            SLCHECK_NZ(ref.address() == "B1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 2);

            ref++;
            SLCHECK_NZ(ref.address() == "C1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 3);

            --ref;
            SLCHECK_NZ(ref.address() == "B1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 2);

            ref--;
            SLCHECK_NZ(ref.address() == "A1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 1);

            --ref;
            SLCHECK_NZ(ref.address() == "XFD1048576");
            SLCHECK_NZ(ref.row() == MAX_ROWS);
            SLCHECK_NZ(ref.column() == MAX_COLS);

            ++ref;
            SLCHECK_NZ(ref.address() == "A1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == 1);

            ref = XLCellReference(1, MAX_COLS);

            ++ref;
            SLCHECK_NZ(ref.address() == "A2");
            SLCHECK_NZ(ref.row() == 2);
            SLCHECK_NZ(ref.column() == 1);

            --ref;
            SLCHECK_NZ(ref.address() == "XFD1");
            SLCHECK_NZ(ref.row() == 1);
            SLCHECK_NZ(ref.column() == MAX_COLS);
        }
        {
            // Comparison operators
            auto ref1 = XLCellReference("B2");
            auto ref2 = XLCellReference("B2");
            auto ref3 = XLCellReference("C3");

            SLCHECK_NZ(ref1 == ref2);
            SLCHECK_Z(ref1 == ref3);
            SLCHECK_NZ(ref1 != ref3);
            SLCHECK_Z(ref1 != ref2);
            SLCHECK_NZ(ref1 < ref3);
            SLCHECK_Z(ref1 > ref3);
            SLCHECK_NZ(ref3 > ref1);
            SLCHECK_Z(ref3 < ref1);
            SLCHECK_NZ(ref1 <= ref2);
            SLCHECK_NZ(ref1 <= ref3);
            SLCHECK_Z(ref3 <= ref1);
            SLCHECK_NZ(ref2 >= ref1);
            SLCHECK_NZ(ref3 >= ref1);
            SLCHECK_Z(ref1 >= ref3);
        }
    }
    {
        {
            //
            // XLCellValue
            //
            // The purpose of this test case is to test the creation of XLDocument objects. Each section section
            // tests document creation using a different method. In addition, saving, closing and copying is tested.
            //
            // XLCellValue Tests", "[XLCellValue]
            {
                // Default Constructor
                XLCellValue value;
                SLCHECK_NZ(value.type() == XLValueType::Empty);
                SLCHECK_NZ(value.typeAsString() == "empty");
                SLCHECK_NZ(value.get<std::string>().empty());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
                SLCHECK_THROWS(value.get<int>()); // @todo Здесь вылетает 
            }
            {
                // Float Constructor
                XLCellValue value(3.14159);
                SLCHECK_NZ(value.type() == XLValueType::Float);
                SLCHECK_NZ(value.typeAsString() == "float");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_NZ(value.get<double>() == 3.14159);
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Integer Constructor
                XLCellValue value(42);

                SLCHECK_NZ(value.type() == XLValueType::Integer);
                SLCHECK_NZ(value.typeAsString() == "integer");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_NZ(value.get<int>() == 42);
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Boolean Constructor
                XLCellValue value(true);

                SLCHECK_NZ(value.type() == XLValueType::Boolean);
                SLCHECK_NZ(value.typeAsString() == "boolean");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_NZ(value.get<bool>() == true);
            }
            {
                // String Constructor
                XLCellValue value("Hello OpenXLSX!");

                SLCHECK_NZ(value.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(value.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Copy Constructor
                XLCellValue value("Hello OpenXLSX!");
                auto        copy = value;

                SLCHECK_NZ(copy.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(copy.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(copy.get<int>());
                SLCHECK_THROWS(copy.get<double>());
                SLCHECK_THROWS(copy.get<bool>());
            }
            {
                // Move Constructor
                XLCellValue value("Hello OpenXLSX!");
                auto        copy = std::move(value);

                SLCHECK_NZ(copy.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(copy.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(copy.get<int>());
                SLCHECK_THROWS(copy.get<double>());
                SLCHECK_THROWS(copy.get<bool>());
            }
            {
                // Copy Assignment
                XLCellValue value("Hello OpenXLSX!");
                XLCellValue copy(1);
                copy = value;

                SLCHECK_NZ(copy.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(copy.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(copy.get<int>());
                SLCHECK_THROWS(copy.get<double>());
                SLCHECK_THROWS(copy.get<bool>());
            }
            {
                // Move Assignment
                XLCellValue value("Hello OpenXLSX!");
                XLCellValue copy(1);
                copy = std::move(value);

                SLCHECK_NZ(copy.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(copy.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(copy.get<int>());
                SLCHECK_THROWS(copy.get<double>());
                SLCHECK_THROWS(copy.get<bool>());
            }
            {
                // Float Assignment
                XLCellValue value;
                value = 3.14159;

                SLCHECK_NZ(value.type() == XLValueType::Float);
                SLCHECK_NZ(value.typeAsString() == "float");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_NZ(value.get<double>() == 3.14159);
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Integer Assignment
                XLCellValue value;
                value = 42;

                SLCHECK_NZ(value.type() == XLValueType::Integer);
                SLCHECK_NZ(value.typeAsString() == "integer");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_NZ(value.get<int>() == 42);
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Boolean Assignment
                XLCellValue value;
                value = true;

                SLCHECK_NZ(value.type() == XLValueType::Boolean);
                SLCHECK_NZ(value.typeAsString() == "boolean");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_NZ(value.get<bool>() == true);
            }
            {
                // String Assignment
                XLCellValue value;
                value = "Hello OpenXLSX!";

                SLCHECK_NZ(value.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(value.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                (temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("testXLCellValue.xlsx");
                std::string test_file_name(temp_buf.cptr());

                // XLCellValueProxy Assignment
                XLCellValue value;
                XLDocument doc;
                doc.create(test_file_name, XLForceOverwrite);
                XLWorksheet wks = doc.workbook().sheet(1);

                wks.cell("A1").value() = "Hello OpenXLSX!";
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(value.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = 3.14159;
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Float);
                SLCHECK_NZ(value.typeAsString() == "float");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_NZ(value.get<double>() == 3.14159);
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = 42;
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Integer);
                SLCHECK_NZ(value.typeAsString() == "integer");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_NZ(value.get<int>() == 42);
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = true;
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Boolean);
                SLCHECK_NZ(value.typeAsString() == "boolean");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_NZ(value.get<bool>() == true);
            }
            {
                // Set Float
                XLCellValue value;
                value.set(3.14159);

                SLCHECK_NZ(value.type() == XLValueType::Float);
                SLCHECK_NZ(value.typeAsString() == "float");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_NZ(value.get<double>() == 3.14159);
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Set Nan Float
                XLCellValue value;
                value.set(std::numeric_limits<double>::quiet_NaN());

                SLCHECK_NZ(value.type() == XLValueType::Error);
                SLCHECK_NZ(value.typeAsString() == "error");
                SLCHECK_NZ(value.get<std::string>() == "#NUM!");
                SLCHECK_THROWS(value.get<int>());
                //        SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Set Inf Float
                XLCellValue value;
                value.set(std::numeric_limits<double>::infinity());

                SLCHECK_NZ(value.type() == XLValueType::Error);
                SLCHECK_NZ(value.typeAsString() == "error");
                SLCHECK_NZ(value.get<std::string>() == "#NUM!");
                SLCHECK_THROWS(value.get<int>());
                //        SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Set Integer
                XLCellValue value;
                value.set(42);

                SLCHECK_NZ(value.type() == XLValueType::Integer);
                SLCHECK_NZ(value.typeAsString() == "integer");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_NZ(value.get<int>() == 42);
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Set Boolean
                XLCellValue value;
                value.set(true);

                SLCHECK_NZ(value.type() == XLValueType::Boolean);
                SLCHECK_NZ(value.typeAsString() == "boolean");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_NZ(value.get<bool>() == true);
            }
            {
                // Set String
                XLCellValue value;
                value.set("Hello OpenXLSX!");

                SLCHECK_NZ(value.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(value.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Clear
                XLCellValue value;
                value.set("Hello OpenXLSX!");
                value.clear();

                SLCHECK_NZ(value.type() == XLValueType::Empty);
                SLCHECK_NZ(value.typeAsString() == "empty");
                SLCHECK_NZ(value.get<std::string>().empty());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Set Error
                XLCellValue value;
                value.set("Hello OpenXLSX!");
                value.setError("#N/A");

                SLCHECK_NZ(value.type() == XLValueType::Error);
                SLCHECK_NZ(value.typeAsString() == "error");
                SLCHECK_NZ(value.get<std::string>() == "#N/A");
                SLCHECK_THROWS(value.get<int>());
                //        SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // Implicit conversion to supported types
                XLCellValue value;

                value ="Hello OpenXLSX!";
                auto result1 = value.get<std::string>();
                SLCHECK_NZ(result1 == "Hello OpenXLSX!");
                SLCHECK_THROWS(static_cast<int>(value));
                SLCHECK_THROWS(static_cast<double>(value));
                SLCHECK_THROWS(static_cast<bool>(value));

                value = 42;
                auto result2 = static_cast<int>(value);
                SLCHECK_NZ(result2 == 42);
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(static_cast<double>(value));
                SLCHECK_THROWS(static_cast<bool>(value));

                value = 3.14159;
                auto result3 = static_cast<double>(value);
                SLCHECK_NZ(result3 == 3.14159);
                SLCHECK_THROWS(static_cast<int>(value));
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(static_cast<bool>(value));

                value = true;
                auto result4 = static_cast<bool>(value);
                SLCHECK_NZ(result4 == true);
                SLCHECK_THROWS(static_cast<int>(value));
                SLCHECK_THROWS(static_cast<double>(value));
                SLCHECK_THROWS(value.get<std::string>());
            }
        }
        {
            //
            // XLCellValue
            //
            (temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("testXLCellValueProxy.xlsx");
            std::string test_file_name(temp_buf.cptr());
            {
                // XLCellValueProxy conversion to XLCellValue (XLCellValue constructor)
                XLDocument doc;
                doc.create(test_file_name, XLForceOverwrite);
                XLWorksheet wks = doc.workbook().sheet(1);
                {
                    wks.cell("A1").value() = "Hello OpenXLSX!";
                    XLCellValue value = wks.cell("A1").value();
                    SLCHECK_NZ(value.type() == XLValueType::String);
                    SLCHECK_NZ(value.typeAsString() == "string");
                    SLCHECK_NZ(value.get<std::string>() == "Hello OpenXLSX!");
                    SLCHECK_THROWS(value.get<int>());
                    SLCHECK_THROWS(value.get<double>());
                    SLCHECK_THROWS(value.get<bool>());
                }
                {
                    wks.cell("A1").value() = 3.14159;
                    XLCellValue value = wks.cell("A1").value();
                    SLCHECK_NZ(value.type() == XLValueType::Float);
                    SLCHECK_NZ(value.typeAsString() == "float");
                    SLCHECK_THROWS(value.get<std::string>());
                    SLCHECK_THROWS(value.get<int>());
                    SLCHECK_NZ(value.get<double>() == 3.14159);
                    SLCHECK_THROWS(value.get<bool>());
                }
                {
                    wks.cell("A1").value() = 42;
                    XLCellValue value = wks.cell("A1").value();
                    SLCHECK_NZ(value.type() == XLValueType::Integer);
                    SLCHECK_NZ(value.typeAsString() == "integer");
                    SLCHECK_THROWS(value.get<std::string>());
                    SLCHECK_NZ(value.get<int>() == 42);
                    SLCHECK_THROWS(value.get<double>());
                    SLCHECK_THROWS(value.get<bool>());
                }
                {
                    wks.cell("A1").value() = true;
                    XLCellValue value = wks.cell("A1").value();
                    SLCHECK_NZ(value.type() == XLValueType::Boolean);
                    SLCHECK_NZ(value.typeAsString() == "boolean");
                    SLCHECK_THROWS(value.get<std::string>());
                    SLCHECK_THROWS(value.get<int>());
                    SLCHECK_THROWS(value.get<double>());
                    SLCHECK_NZ(value.get<bool>() == true);
                }
            }
            {
                // XLCellValueProxy conversion to XLCellValue (XLCellValue assignment operator)
                XLCellValue value;
                XLDocument doc;
                doc.create(test_file_name, XLForceOverwrite);
                XLWorksheet wks = doc.workbook().sheet(1);

                wks.cell("A1").value() = "Hello OpenXLSX!";
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::String);
                SLCHECK_NZ(value.typeAsString() == "string");
                SLCHECK_NZ(value.get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = 3.14159;
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Float);
                SLCHECK_NZ(value.typeAsString() == "float");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_NZ(value.get<double>() == 3.14159);
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = 42;
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Integer);
                SLCHECK_NZ(value.typeAsString() == "integer");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_NZ(value.get<int>() == 42);
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = true;
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Boolean);
                SLCHECK_NZ(value.typeAsString() == "boolean");
                SLCHECK_THROWS(value.get<std::string>());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_NZ(value.get<bool>() == true);

                wks.cell("A1").value().setError("#N/A");
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Error);
                SLCHECK_NZ(value.typeAsString() == "error");
                SLCHECK_NZ(value.get<std::string>() == "#N/A");
                SLCHECK_THROWS(value.get<int>());
                //        SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value().clear();
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Empty);
                SLCHECK_NZ(value.typeAsString() == "empty");
                SLCHECK_NZ(value.get<std::string>().empty());
                SLCHECK_THROWS(value.get<int>());
                SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = std::numeric_limits<double>::quiet_NaN();
                value = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Error);
                SLCHECK_NZ(value.typeAsString() == "error");
                SLCHECK_NZ(value.get<std::string>() == "#NUM!");
                SLCHECK_THROWS(value.get<int>());
                //        SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());

                wks.cell("A1").value() = std::numeric_limits<double>::infinity();
                value                  = wks.cell("A1").value();
                SLCHECK_NZ(value.type() == XLValueType::Error);
                SLCHECK_NZ(value.typeAsString() == "error");
                SLCHECK_NZ(value.get<std::string>() == "#NUM!");
                SLCHECK_THROWS(value.get<int>());
                //        SLCHECK_THROWS(value.get<double>());
                SLCHECK_THROWS(value.get<bool>());
            }
            {
                // XLCellValueProxy copy assignment
                XLCellValue value;
                XLDocument doc;
                doc.create(test_file_name, XLForceOverwrite);
                XLWorksheet wks = doc.workbook().sheet(1);

                wks.cell("A1").value() = "Hello OpenXLSX!";
                wks.cell("A2").value() = wks.cell("A1").value();
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::String);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "string");
                SLCHECK_NZ(wks.cell("A2").value().get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A1").value() = 3.14159;
                wks.cell("A2").value() = wks.cell("A1").value();
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Float);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "float");
                SLCHECK_THROWS(wks.cell("A2").value().get<std::string>());
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_NZ(wks.cell("A2").value().get<double>() == 3.14159);
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A1").value() = 42;
                wks.cell("A2").value() = wks.cell("A1").value();
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Integer);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "integer");
                SLCHECK_THROWS(wks.cell("A2").value().get<std::string>());
                SLCHECK_NZ(wks.cell("A2").value().get<int>() == 42);
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A1").value() = true;
                wks.cell("A2").value() = wks.cell("A1").value();
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Boolean);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "boolean");
                SLCHECK_THROWS(wks.cell("A2").value().get<std::string>());
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_NZ(wks.cell("A2").value().get<bool>() == true);

                wks.cell("A1").value().setError("#N/A");
                wks.cell("A2").value() = wks.cell("A1").value();
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Error);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "error");
                SLCHECK_NZ(wks.cell("A2").value().get<std::string>() == "#N/A");
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                //        SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A1").value().clear();
                wks.cell("A2").value() = wks.cell("A1").value();
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Empty);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "empty");
                SLCHECK_NZ(wks.cell("A2").value().get<std::string>().empty());
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());
            }
            {
                // XLCellValueProxy set functions
                XLCellValue value;
                XLDocument doc;
                doc.create(test_file_name, XLForceOverwrite);
                XLWorksheet wks = doc.workbook().sheet(1);

                wks.cell("A2").value().set("Hello OpenXLSX!");
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::String);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "string");
                SLCHECK_NZ(wks.cell("A2").value().get<std::string>() == "Hello OpenXLSX!");
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A2").value().set(3.14159);
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Float);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "float");
                SLCHECK_THROWS(wks.cell("A2").value().get<std::string>());
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_NZ(wks.cell("A2").value().get<double>() == 3.14159);
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A2").value().set(42);
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Integer);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "integer");
                SLCHECK_THROWS(wks.cell("A2").value().get<std::string>());
                SLCHECK_NZ(wks.cell("A2").value().get<int>() == 42);
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());

                wks.cell("A2").value().set(true);
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Boolean);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "boolean");
                SLCHECK_THROWS(wks.cell("A2").value().get<std::string>());
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_NZ(wks.cell("A2").value().get<bool>() == true);

                wks.cell("A2").value().setError("#N/A");
                SLCHECK_NZ(wks.cell("A2").value().type() == XLValueType::Error);
                SLCHECK_NZ(wks.cell("A2").value().typeAsString() == "error");
                SLCHECK_NZ(wks.cell("A2").value().get<std::string>() == "#N/A");
                SLCHECK_THROWS(wks.cell("A2").value().get<int>());
                //        SLCHECK_THROWS(wks.cell("A2").value().get<double>());
                SLCHECK_THROWS(wks.cell("A2").value().get<bool>());
            }
        }
    }
#endif // } 0 @construction
    return CurrentStatus;
}

#if 0 // {
//
// XLColor
//
TEST_CASE("XLColor", "[XLColor]")
{
    SECTION("Constructors") {

        XLColor color;
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.red() == 0);
        REQUIRE(color.green() == 0);
        REQUIRE(color.blue() == 0);
        REQUIRE(color.hex() == "ff000000");

        color = XLColor(128, 128, 128);
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.red() == 128);
        REQUIRE(color.green() == 128);
        REQUIRE(color.blue() == 128);
        REQUIRE(color.hex() == "ff808080");

        color = XLColor(128, 128, 128, 128);
        REQUIRE(color.alpha() == 128);
        REQUIRE(color.red() == 128);
        REQUIRE(color.green() == 128);
        REQUIRE(color.blue() == 128);
        REQUIRE(color.hex() == "80808080");

        color = XLColor("80808080");
        REQUIRE(color.alpha() == 128);
        REQUIRE(color.red() == 128);
        REQUIRE(color.green() == 128);
        REQUIRE(color.blue() == 128);
        REQUIRE(color.hex() == "80808080");

        color = XLColor("ffffff");
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.red() == 255);
        REQUIRE(color.green() == 255);
        REQUIRE(color.blue() == 255);
        REQUIRE(color.hex() == "ffffffff");

        color.set("808080");
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.red() == 128);
        REQUIRE(color.green() == 128);
        REQUIRE(color.blue() == 128);
        REQUIRE(color.hex() == "ff808080");

        color.set(255, 255, 255);
        REQUIRE(color.alpha() == 255);
        REQUIRE(color.red() == 255);
        REQUIRE(color.green() == 255);
        REQUIRE(color.blue() == 255);
        REQUIRE(color.hex() == "ffffffff");

        color.set(0, 0, 0, 0);
        REQUIRE(color.alpha() == 0);
        REQUIRE(color.red() == 0);
        REQUIRE(color.green() == 0);
        REQUIRE(color.blue() == 0);
        REQUIRE(color.hex() == "00000000");

        REQUIRE_THROWS(XLColor("ffff"));
        REQUIRE_THROWS(XLColor("ffffffffff"));

    }
}
//
// XLDateTime
//
TEST_CASE("XLDateTime Tests", "[XLFormula]")
{

    SECTION("Default construction")
    {
        XLDateTime dt;

        REQUIRE(dt.serial() == Approx(1.0));

        auto tm = dt.tm();
        REQUIRE(tm.tm_year == 0);
        REQUIRE(tm.tm_mon == 0);
        REQUIRE(tm.tm_mday == 1);
        REQUIRE(tm.tm_yday == 0);
        REQUIRE(tm.tm_wday == 0);
        REQUIRE(tm.tm_hour == 0);
        REQUIRE(tm.tm_min == 0);
        REQUIRE(tm.tm_sec == 0);
    }

    SECTION("Constructor (serial number)")
    {
        REQUIRE_THROWS(XLDateTime(0.0));

        XLDateTime dt (6069.86742);

        REQUIRE(dt.serial() == Approx(6069.86742));

        auto tm = dt.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
//        REQUIRE(tm.tm_yday == 0);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 20);
        REQUIRE(tm.tm_min == 49);
        REQUIRE(tm.tm_sec == 5);
    }

    SECTION("Constructor (serial number, seconds rounding)")
    {
        REQUIRE_THROWS(XLDateTime(0.0));

        const double serial = 6069.000008;
        XLDateTime dt (serial);

        REQUIRE(dt.serial() == Approx(serial));

        auto tm = dt.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 0);
        REQUIRE(tm.tm_min == 0);
        REQUIRE(tm.tm_sec == 1);
    }

    SECTION("Constructor (std::tm object)")
    {
        std::tm tmo;

        tmo.tm_year = -1;
        tmo.tm_mon = 0;
        tmo.tm_mday = 0;
        tmo.tm_yday = 0;
        tmo.tm_wday = 0;
        tmo.tm_hour = 0;
        tmo.tm_min = 0;
        tmo.tm_sec = 0;
        REQUIRE_THROWS(XLDateTime(tmo));

        tmo.tm_year = 89;
        tmo.tm_mon = 13;
        REQUIRE_THROWS(XLDateTime(tmo));

        tmo.tm_mon = -1;
        REQUIRE_THROWS(XLDateTime(tmo));

        tmo.tm_mon = 11;
        tmo.tm_mday = 0;
        REQUIRE_THROWS(XLDateTime(tmo));

        tmo.tm_mday = 32;
        REQUIRE_THROWS(XLDateTime(tmo));

        tmo.tm_mday = 30;
        tmo.tm_hour = 17;
        tmo.tm_min = 6;
        tmo.tm_sec = 28;
        tmo.tm_wday = 6;

        XLDateTime dt(tmo);

        REQUIRE(dt.serial() == Approx(32872.7128));
    }

    SECTION("Copy Constructor")
    {
        XLDateTime dt (6069.86742);
        XLDateTime dt2 = dt;

        auto tm = dt2.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 20);
        REQUIRE(tm.tm_min == 49);
        REQUIRE(tm.tm_sec == 5);
    }

    SECTION("Move Constructor")
    {
        XLDateTime dt (6069.86742);
        XLDateTime dt2 = std::move(dt);

        auto tm = dt2.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 20);
        REQUIRE(tm.tm_min == 49);
        REQUIRE(tm.tm_sec == 5);
    }

    SECTION("Copy Assignment")
    {
        XLDateTime dt (6069.86742);
        XLDateTime dt2;
        dt2 = dt;

        auto tm = dt2.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 20);
        REQUIRE(tm.tm_min == 49);
        REQUIRE(tm.tm_sec == 5);
    }

    SECTION("Move Assignment")
    {
        XLDateTime dt (6069.86742);
        XLDateTime dt2;
        dt2 = std::move(dt);

        auto tm = dt2.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 20);
        REQUIRE(tm.tm_min == 49);
        REQUIRE(tm.tm_sec == 5);
    }

    SECTION("Serial Assignment")
    {
        XLDateTime dt (1.0);
        dt = 6069.86742;

        auto tm = dt.tm();
        REQUIRE(tm.tm_year == 16);
        REQUIRE(tm.tm_mon == 7);
        REQUIRE(tm.tm_mday == 12);
        REQUIRE(tm.tm_wday == 6);
        REQUIRE(tm.tm_hour == 20);
        REQUIRE(tm.tm_min == 49);
        REQUIRE(tm.tm_sec == 5);
    }

    SECTION("std::tm Assignment")
    {
        std::tm tmo;
        tmo.tm_year = 16;
        tmo.tm_mon = 7;
        tmo.tm_mday = 12;
        tmo.tm_wday = 6;
        tmo.tm_hour = 20;
        tmo.tm_min = 49;
        tmo.tm_sec = 5;

        XLDateTime dt (1.0);
        dt = tmo;

        REQUIRE(dt.serial() == Approx(6069.86742));
    }

    SECTION("Implicit conversion")
    {
        std::tm tmo;
        tmo.tm_year = 16;
        tmo.tm_mon = 7;
        tmo.tm_mday = 12;
        tmo.tm_wday = 6;
        tmo.tm_hour = 20;
        tmo.tm_min = 49;
        tmo.tm_sec = 5;

        XLDateTime dt (1.0);
        dt = tmo;

        double serial = dt;
        std::tm result = dt;

        REQUIRE(serial == Approx(6069.86742));
        REQUIRE(result.tm_year == 16);
        REQUIRE(result.tm_mon == 7);
        REQUIRE(result.tm_mday == 12);
        REQUIRE(result.tm_wday == 6);
        REQUIRE(result.tm_hour == 20);
        REQUIRE(result.tm_min == 49);
        REQUIRE(result.tm_sec == 5);
    }
}
//
// XLDocument
//
/**
 * @brief The purpose of this test case is to test the creation of XLDocument objects. Each section section
 * tests document creation using a different method. In addition, saving, closing and copying is tested.
 */
TEST_CASE("XLDocument Tests", "[XLDocument]")
{
    std::string file    = "./testXLDocument.xlsx";
    std::string newfile = "./TestDocumentCreationNew.xlsx";

    /**
     * @test
     *
     * @details
     */
    SECTION("Create empty XLDocument, using default constructor")
    {
        XLDocument doc;
        REQUIRE_FALSE(doc);
    }

    //    /**
    //     * @test Create new document using the CreateDocument method.
    //     *
    //     * @details Creates an empty document and creates the excel file using the CreateDocument() member function.
    //     * Success is tested by checking if the file have been created on disk and that the DocumentName member function
    //     * returns the correct file name.
    //     */
    //    SECTION("Section 01A: Create new using CreateDocument()")
    //    {
    //        XLDocument doc;
    //        doc.create(file);
    //        std::ifstream f(file);
    //        REQUIRE(f.good());
    //        REQUIRE(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief Open an existing document using the constructor.
    //     *
    //     * @details Opens an existing document by passing the file name to the constructor.
    //     * Success is tested by checking that the DocumentName member function returns the correct file name.
    //     */
    //    SECTION("Section 01B: Open existing using Constructor")
    //    {
    //        XLDocument doc(file);
    //        REQUIRE(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01C: Open existing using openDocument()")
    //    {
    //        XLDocument doc;
    //        doc.open(file);
    //        REQUIRE(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01D: Save document using Save()")
    //    {
    //        XLDocument doc(file);
    //
    //        doc.save();
    //        std::ifstream n(file);
    //        REQUIRE(n.good());
    //        REQUIRE(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01E: Save document using SaveDocumentAs()")
    //    {
    //        XLDocument doc(file);
    //
    //        doc.saveAs(newfile);
    //        std::ifstream n(newfile);
    //        REQUIRE(n.good());
    //        REQUIRE(doc.name() == newfile);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01F: Copy construction")
    //    {
    //        XLDocument doc(file);
    //        XLDocument copy = doc;
    //
    //        REQUIRE(copy.name() == doc.name());
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01G: Copy assignment")
    //    {
    //        XLDocument doc(file);
    //        XLDocument copy;
    //        copy = doc;
    //
    //        REQUIRE(copy.name() == doc.name());
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01H: Move construction")
    //    {
    //        XLDocument doc(file);
    //        XLDocument copy = std::move(doc);
    //
    //        REQUIRE(copy.name() == file);
    //        REQUIRE_THROWS(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01I: Move assignment")
    //    {
    //        XLDocument doc(file);
    //        XLDocument copy;
    //        copy = std::move(doc);
    //
    //        REQUIRE(copy.name() == file);
    //        REQUIRE_THROWS(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01J: Close and Reopen")
    //    {
    //        XLDocument doc;
    //        doc.create(file);
    //        doc.close();
    //        REQUIRE_THROWS(doc.name() == file);
    //
    //        doc.open(file);
    //        REQUIRE(doc.name() == file);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01K: Reopen without closing")
    //    {
    //        XLDocument doc;
    //        doc.create(file);
    //
    //        doc.open(newfile);
    //        REQUIRE(doc.name() == newfile);
    //    }
    //
    //    /**
    //     * @brief
    //     *
    //     * @details
    //     */
    //    SECTION("Section 01L: Open document as const")
    //    {
    //        const XLDocument doc(file);
    //        REQUIRE(doc.name() == file);
    //    }
}
//
// XLFormula
//
TEST_CASE("XLFormula Tests", "[XLFormula]")
{
    SECTION("Default Constructor")
    {
        XLFormula formula;

        REQUIRE(formula.get().empty());
    }

    SECTION("String Constructor")
    {
        XLFormula formula1("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        std::string s = "BLAH2";
        XLFormula formula2(s);
        REQUIRE(formula2.get() == "BLAH2");
    }

    SECTION("Copy Constructor")
    {
        XLFormula formula1("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        XLFormula formula2 = formula1;
        REQUIRE(formula2.get() == "BLAH1");

    }

    SECTION("Move Constructor")
    {
        XLFormula formula1("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        XLFormula formula2 = std::move(formula1);
        REQUIRE(formula2.get() == "BLAH1");

    }

    SECTION("Copy assignment")
    {
        XLFormula formula1("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        XLFormula formula2;
        formula2 = formula1;
        REQUIRE(formula2.get() == "BLAH1");
    }

    SECTION("Move assignment")
    {
        XLFormula formula1("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        XLFormula formula2;
        formula2 = std::move(formula1);
        REQUIRE(formula2.get() == "BLAH1");

    }

    SECTION("Clear")
    {
        XLFormula formula1("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        formula1.clear();
        REQUIRE(formula1.get().empty());
    }

    SECTION("String assignment")
    {
        XLFormula formula1;
        formula1 = "BLAH1";
        REQUIRE(formula1.get() == "BLAH1");

        XLFormula formula2;
        formula2 = std::string("BLAH2");
        REQUIRE(formula2.get() == "BLAH2");
    }

    SECTION("String setter")
    {
        XLFormula formula1;
        formula1.set("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        XLFormula formula2;
        formula2.set(std::string("BLAH2"));
        REQUIRE(formula2.get() == "BLAH2");
    }

    SECTION("Implicit conversion")
    {
        XLFormula formula1;
        formula1.set("BLAH1");
        REQUIRE(formula1.get() == "BLAH1");

        auto result = std::string(formula1);
        REQUIRE(result == "BLAH1");
    }

    SECTION("FormulaProxy")
    {
        XLDocument doc;
        doc.create("./testXLFormula.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        wks.cell("A1").formula() = "=1+1";
        wks.cell("B2").formula() = wks.cell("A1").formula();
        REQUIRE(wks.cell("B2").formula() == XLFormula("=1+1"));

        XLFormula form = wks.cell("B2").formula();
        REQUIRE(form == XLFormula("=1+1"));

        REQUIRE(wks.cell("A1").hasFormula());
        wks.cell("A1").formula().clear();
        REQUIRE_FALSE(wks.cell("A1").hasFormula());
        REQUIRE(wks.cell("B2").formula() == XLFormula("=1+1"));

    }
}
//
// XLRow
//
TEST_CASE("XLRow Tests", "[XLRow]")
{
    SECTION("XLRow")
    {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto row_3 = wks.row(3);
        REQUIRE(row_3.rowNumber() == 3);

        XLRow copy1  = row_3;
        auto  height = copy1.height();
        REQUIRE(copy1.height() == height);

        XLRow copy2   = std::move(copy1);
        auto  descent = copy2.descent();
        REQUIRE(copy2.descent() == descent);

        XLRow copy3;
        copy3 = copy2;
        copy3.setHeight(height * 3);
        REQUIRE(copy3.height() == height * 3);
        copy3.setHeight(height * 4);
        REQUIRE(copy3.height() == height * 4);

        XLRow copy4;
        copy4 = std::move(copy3);
        copy4.setDescent(descent * 2);
        REQUIRE(copy4.descent() == descent * 2);
        copy4.setDescent(descent * 3);
        REQUIRE(copy4.descent() == descent * 3);

        REQUIRE(copy4.isHidden() == false);
        copy4.setHidden(true);
        REQUIRE(copy4.isHidden() == true);
        copy4.setHidden(false);
        REQUIRE(copy4.isHidden() == false);


        auto row1 = wks.row(11);
        auto row2 = wks.row(12);
        auto row3 = wks.row(13);
        auto row4 = wks.row(13);
        REQUIRE(row3 == row4);
        REQUIRE(row4 >= row3);
        REQUIRE(row3 <= row4);
        REQUIRE(row3 >= row2);
        REQUIRE(row2 <= row3);
        REQUIRE(row1 != row2);
        REQUIRE(row2 > row1);
        REQUIRE(row1 < row2);

        doc.save();
    }
}

TEST_CASE("XLRowData Tests", "[XLRowData]")
{

    SECTION("XLRowData (vector<int>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::vector<int> {1, 2, 3, 4, 5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::vector<int>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 15);

        val1sum = 0;
        const auto val1results2 = static_cast<std::vector<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<int>();
        REQUIRE(val1sum == 15);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (vector<double>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::vector<double> {1.1, 2.2, 3.3, 4.4, 5.5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0.0;
        const auto val1results1 = static_cast<std::vector<double>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 16.5);

        val1sum = 0;
        const auto val1results2 = static_cast<std::vector<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<double>();
        REQUIRE(val1sum == 16.5);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (vector<bool>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::vector<bool> {true, false, true, false, true};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::vector<bool>>(row1c.values());
        for (const auto v : val1results1)
            if (v) val1sum += 1;
        REQUIRE(val1sum == 3);

        val1sum = 0;
        const auto val1results2 = static_cast<std::vector<XLCellValue>>(row1c.values());
        for (const auto v : val1results2)
            if(v.get<bool>()) val1sum += 1;
        REQUIRE(val1sum == 3);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (vector<std::string>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::vector<std::string> {"This", "is", "a", "test."};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::vector<std::string>>(row1c.values());
        for (const auto v : val1results1) val1sum += v.size();
        REQUIRE(val1sum == 12);

        val1sum = 0;
        const auto val1results2 = static_cast<std::vector<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<std::string>().size();
        REQUIRE(val1sum == 12);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (vector<XLCellValue>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::vector<XLCellValue> {1, 2, 3, 4, 5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::vector<int>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 15);

        val1sum = 0;
        const auto val1results2 = static_cast<std::vector<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<int>();
        REQUIRE(val1sum == 15);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (list<int>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::list<int> {1, 2, 3, 4, 5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::list<int>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 15);

        val1sum = 0;
        const auto val1results2 = static_cast<std::list<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<int>();
        REQUIRE(val1sum == 15);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (list<bool>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::list<bool> {true, false, true, false, true};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::list<bool>>(row1c.values());
        for (const auto v : val1results1)
            if (v) val1sum += 1;
        REQUIRE(val1sum == 3);

        val1sum = 0;
        const auto val1results2 = static_cast<std::list<XLCellValue>>(row1c.values());
        for (const auto v : val1results2)
            if (v.get<bool>()) val1sum += 1;
        REQUIRE(val1sum == 3);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (list<double>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::list<double> {1.1, 2.2, 3.3, 4.4, 5.5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0.0;
        const auto val1results1 = static_cast<std::list<double>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 16.5);

        val1sum = 0;
        const auto val1results2 = static_cast<std::list<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<double>();
        REQUIRE(val1sum == 16.5);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (list<std::string>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::list<std::string> {"This", "is", "a", "test."};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::list<std::string>>(row1c.values());
        for (const auto v : val1results1) val1sum += v.size();
        REQUIRE(val1sum == 12);

        val1sum = 0;
        const auto val1results2 = static_cast<std::list<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<std::string>().size();
        REQUIRE(val1sum == 12);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (list<XLCellValue>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::list<XLCellValue> {1, 2, 3, 4, 5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::list<int>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 15);

        val1sum = 0;
        const auto val1results2 = static_cast<std::list<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<int>();
        REQUIRE(val1sum == 15);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (deque<int>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::deque<int> {1, 2, 3, 4, 5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::deque<int>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 15);

        val1sum = 0;
        const auto val1results2 = static_cast<std::deque<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<int>();
        REQUIRE(val1sum == 15);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (deque<bool>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::deque<bool> {true, false, true, false, true};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::deque<bool>>(row1c.values());
        for (const auto v : val1results1)
            if (v) val1sum += 1;
        REQUIRE(val1sum == 3);

        val1sum = 0;
        const auto val1results2 = static_cast<std::deque<XLCellValue>>(row1c.values());
        for (const auto v : val1results2)
            if (v.get<bool>()) val1sum += 1;
        REQUIRE(val1sum == 3);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (deque<double>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::deque<double> {1.1, 2.2, 3.3, 4.4, 5.5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0.0;
        const auto val1results1 = static_cast<std::deque<double>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 16.5);

        val1sum = 0;
        const auto val1results2 = static_cast<std::deque<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<double>();
        REQUIRE(val1sum == 16.5);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (deque<std::string>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::deque<std::string> {"This", "is", "a", "test."};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::deque<std::string>>(row1c.values());
        for (const auto v : val1results1) val1sum += v.size();
        REQUIRE(val1sum == 12);

        val1sum = 0;
        const auto val1results2 = static_cast<std::deque<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<std::string>().size();
        REQUIRE(val1sum == 12);

        row1.values().clear();

        doc.save();
    }

    SECTION("XLRowData (deque<XLCellValue>)") {
        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto val1 = std::deque<XLCellValue> {1, 2, 3, 4, 5};
        auto row1 = wks.row(1);
        row1.values() = val1;

        const auto row1c = row1;

        auto val1sum = 0;
        const auto val1results1 = static_cast<std::deque<int>>(row1c.values());
        for (const auto v : val1results1) val1sum += v;
        REQUIRE(val1sum == 15);

        val1sum = 0;
        const auto val1results2 = static_cast<std::deque<XLCellValue>>(row1c.values());
        for (const auto v : val1results2) val1sum += v.get<int>();
        REQUIRE(val1sum == 15);

        row1.values().clear();

        doc.save();
    }
}

TEST_CASE("XLRowDataRange Tests", "[XLRowDataRange]")
{
    SECTION("XLRowDataRange") {

        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto row = wks.row(1);
        auto range = row.cells();
        for (auto& cell : range) cell.value() = 1;

        auto sum = 0;
        for (const auto& cell : range) sum += cell.value().get<int>();
        REQUIRE(sum == 1);
        REQUIRE(range.size() == 1);

        auto range_copy = range;
        sum = 0;
        for (const auto& cell : range_copy) sum += cell.value().get<int>();
        REQUIRE(sum == 1);
        REQUIRE(range_copy.size() == 1);

        auto range_move = std::move(range_copy);
        sum = 0;
        for (const auto& cell : range_move) sum += cell.value().get<int>();
        REQUIRE(sum == 1);
        REQUIRE(range_move.size() == 1);

        auto range_copy2 = range_move;
        range_copy2 = range;
        sum = 0;
        for (const auto& cell : range_copy2) sum += cell.value().get<int>();
        REQUIRE(sum == 1);
        REQUIRE(range_copy2.size() == 1);

        auto range_move2 = range_move;
        range_move2 = std::move(range_copy2);
        sum = 0;
        for (const auto& cell : range_move2) sum += cell.value().get<int>();
        REQUIRE(sum == 1);
        REQUIRE(range_move2.size() == 1);

        auto row2 = wks.row(2);
        auto range2 = row2.cells(8);
        for (auto& cell : range2) cell.value() = 1;

        sum = 0;
        for (const auto& cell : row2.cells()) sum += cell.value().get<int>();
        REQUIRE(sum == 8);
        REQUIRE(range2.size() == 8);

        auto row3 = wks.row(3);
        auto range3 = row3.cells(3, 8);
        for (auto& cell : range3) cell.value() = 1;

        sum = 0;
        for (const auto& cell : row3.cells())
            if (cell.value().type() == XLValueType::Integer) sum += cell.value().get<int>();
        REQUIRE(sum == 6);
        REQUIRE(row3.cells().size() == 8);

        doc.save();
    }

    SECTION("XLRowDataIterator") {

        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto row = wks.row(1);
        auto range = row.cells(2);
        for (auto& cell : range) cell.value() = 1;
        auto begin = range.begin();
        auto end = range.end();
        auto other = begin;

        REQUIRE(begin == other);
        REQUIRE_FALSE(begin == end);
        REQUIRE(begin != end);
        REQUIRE_FALSE(begin != other);

        ++other;
        REQUIRE_FALSE(begin == other);

        ++other;
        REQUIRE(other == end);

        other = begin;
        REQUIRE(begin == other);

        other++;
        other = std::move(begin);
        begin = range.begin();
        REQUIRE(begin == other);

    }

}

TEST_CASE("XLRowIterator Tests", "[XLRowDataRange]")
{
    SECTION("XLRowIterator") {

        XLDocument doc;
        doc.create("./testXLRow.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");

        auto range = wks.rows(1,3);
        auto first = range.begin();
        auto second = first;
        second++;
        auto third = range.end();
        auto fourth = std::move(third);
        third = second;
        second = std::move(first);
        first = range.begin();

        REQUIRE(second == range.begin());
        REQUIRE_FALSE(second == third);
        REQUIRE_FALSE(second == fourth);
        REQUIRE(range.rowCount() == 3);
        REQUIRE(first->rowNumber() == 1);

    }
}
//
// XLSheet
//
TEST_CASE("XLSheet Tests", "[XLSheet]")
{
    SECTION("XLSheet Visibility") {

        XLDocument doc;
        doc.create("./testXLSheet1.xlsx");

        auto wks1 = doc.workbook().sheet(1);
        wks1.setName("VeryHidden");
        REQUIRE(wks1.name() == "VeryHidden");

        doc.workbook().addWorksheet("Hidden");
        auto wks2 = doc.workbook().sheet("Hidden");
        REQUIRE(wks2.name() == "Hidden");

        doc.workbook().addWorksheet("Visible");
        auto wks3 = doc.workbook().sheet("Visible");
        REQUIRE(wks3.name() == "Visible");


        REQUIRE(wks1.visibility() == XLSheetState::Visible);
        REQUIRE(wks2.visibility() == XLSheetState::Visible);
        REQUIRE(wks3.visibility() == XLSheetState::Visible);

        wks1.setVisibility(XLSheetState::VeryHidden);
        REQUIRE(wks1.visibility() == XLSheetState::VeryHidden);

        wks2.setVisibility(XLSheetState::Hidden);
        REQUIRE(wks2.visibility() == XLSheetState::Hidden);

        REQUIRE_THROWS(wks3.setVisibility(XLSheetState::Hidden));
        wks3.setVisibility(XLSheetState::Visible);

        doc.save();
    }

    SECTION("XLSheet Tab Color") {

        XLDocument doc;
        doc.create("./testXLSheet2.xlsx");

        auto wks1 = doc.workbook().sheet(1);
        wks1.setName("Sheet1");
        REQUIRE(wks1.name() == "Sheet1");

        doc.workbook().addWorksheet("Sheet2");
        auto wks2 = doc.workbook().sheet("Sheet2");
        REQUIRE(wks2.name() == "Sheet2");

        doc.workbook().addWorksheet("Sheet3");
        auto wks3 = doc.workbook().sheet("Sheet3");
        REQUIRE(wks3.name() == "Sheet3");

        wks1.setColor(XLColor(255, 0, 0));
        REQUIRE(wks1.color() == XLColor(255, 0, 0));
        REQUIRE_FALSE(wks1.color() == XLColor(0, 0, 0));

        wks2.setColor(XLColor(0, 255, 0));
        REQUIRE(wks2.color() == XLColor(0, 255, 0));
        REQUIRE_FALSE(wks2.color() == XLColor(0, 0, 0));

        wks3.setColor(XLColor(0, 0, 255));
        REQUIRE(wks3.color() == XLColor(0, 0, 255));
        REQUIRE_FALSE(wks3.color() == XLColor(0, 0, 0));

        doc.save();
    }
}
#endif // } 0 