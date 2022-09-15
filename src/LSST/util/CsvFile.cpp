// -*- LSST-C++ -*-
/*
 *  This file is part of LSST M2 support system package.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

/// System headers
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

// Class header
#include "util/CsvFile.h"

// LSST headers
#include "util/Log.h"

using namespace std;

namespace {

/// Append `row` to stringstream `os`.
void appendRow(stringstream& os, LSST::m2cellcpp::util::CsvFile::Row const& row) {
    bool first = true;
    for (string const& val : row) {
        if (first) {
            first = false;
        } else {
            os << ",";
        }
        os << val;
    }
    os << "\n";
}
}  // namespace

namespace LSST {
namespace m2cellcpp {
namespace util {

void CsvFile::_readRow(std::stringstream& lineStrm, Row& row) {
    string colVal;
    while (getline(lineStrm, colVal, ',')) {
        row.push_back(colVal);
    }
}

void CsvFile::read() {
    // Open the file.
    fstream fstrm;
    fstrm.open(_fileName, ios::in);
    if (!fstrm.is_open() || fstrm.bad()) {
        string emsg = "CsvFile::read() could not open file " + _fileName + " " + to_string(errno);
        LERROR(emsg);
        throw runtime_error(emsg);
    }

    // Get the column names from the header line
    string line;
    getline(fstrm, line);
    {
        stringstream lineStrm(line);
        _readRow(lineStrm, _columnNames);
    }
    _columnCount = _columnNames.size();

    // Read in the data rows of the file.
    for (int rowCount = 0; getline(fstrm, line); ++rowCount) {
        vector<std::string> row;
        stringstream lineStrm(line);
        _readRow(lineStrm, row);
        if (row.size() == 0) return;
        if (row.size() < _columnNames.size()) {
            string eMsg = "CsvFile::read() " + _fileName + " incomplete row " + to_string(rowCount);
            LERROR(eMsg);
            throw runtime_error(eMsg);
        }
        if (row.size() > _columnNames.size()) {
            LWARN("CsvFile::read() ", _fileName, " extra columns in row=", rowCount);
        }
        _rowStrings.push_back(row);
    }
    _rowCount = _rowStrings.size();
    _organize();
}

void CsvFile::_organize() {
    for (int j = 0; j < _columnCount; ++j) {
        auto result = _colIndex.insert(make_pair(_columnNames[j], j));
        if (result.second == false) {
            string eMsg = "CsvFile::_organize duplicate column name " + _columnNames[j] + " in " + _fileName;
            LERROR(eMsg);
            throw runtime_error(eMsg);
        }
    }
}

string CsvFile::dumpStr() const {
    stringstream os;
    // Add headers
    appendRow(os, _columnNames);

    for (Row const& row : _rowStrings) {
        appendRow(os, row);
    }
    return os.str();
}

string CsvFile::getValue(string const& col, int row) {
    if (row < 0 || row >= _rowCount) {
        string eMsg =
                "CsvFile::getValue row " + to_string(row) + " is out of range for " + _fileName + ":" + col;
        LWARN(eMsg);
        throw runtime_error(eMsg);
    }
    auto iter = _colIndex.find(col);
    if (iter == _colIndex.end()) {
        string eMsg = "CsvFile::getValue col " + col + " is out of range for " + _fileName;
        LWARN(eMsg);
        throw runtime_error(eMsg);
    }
    string val = _rowStrings[row][iter->second];
    return val;
}

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
