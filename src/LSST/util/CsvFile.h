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

#ifndef LSST_M2CELLCPP_UTIL_CSVFILE_H
#define LSST_M2CELLCPP_UTIL_CSVFILE_H

// System headers
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace LSST {
namespace m2cellcpp {
namespace util {

/// This class is used to read in a CSV file and provide the contents
/// by column name and row number.
/// Unit tests in test_CsvFile.cpp
class CsvFile {
public:
    using Row = std::vector<std::string>;

    CsvFile(std::string const& fileName) : _fileName(fileName) {}

    CsvFile() = delete;
    CsvFile(CsvFile const&) = delete;
    CsvFile& operator=(CsvFile const&) = delete;

    ~CsvFile() = default;

    /// Read in the CSV file.
    /// @throws runtime_error if the file could not be read.
    void read();

    /// Return a log worthy string of this class's contents.
    std::string dumpStr() const;

    /// Return the number of columns.
    int getColumnCount() const { return _columnCount; }

    /// Return the number of data rows read from the file.
    int getRowCount() const { return _rowCount; }

    /// Return the valaue for column with name `col` from `row`.
    /// @throws runtime_error if there is a problem.
    std::string getValue(std::string const& col, int row);

private:
    /// Read a single row from the CSV file.
    /// @throws runtime_error if there is a problem.
    void _readRow(std::stringstream& lineStrm, Row& row);
    void _organize();  ///< Make the index for headers.

    std::string _fileName;         ///< Name of the CSV file.
    Row _columnNames;              ///< Names of the columns.
    int _columnCount;              ///< Number of columns read from the file.
    std::vector<Row> _rowStrings;  ///< values in the rows.
    int _rowCount = 0;             ///< Number of data rows read from the file.

    std::map<std::string, int> _colIndex;  ///< index of columns
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_CSVFILE_H
