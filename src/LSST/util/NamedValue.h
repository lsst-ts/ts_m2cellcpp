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

#ifndef LSST_M2CELLCPP_UTIL_NAMEDVALUE_H
#define LSST_M2CELLCPP_UTIL_NAMEDVALUE_H

// System headers
#include <cmath>
#include <map>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

namespace LSST {
namespace m2cellcpp {
namespace util {

class CsvFile;

/// This is the base class for storing the name key and a value that can be set elsewhere.
/// Currently, the value is set from a `util::CsvFile`, but this is not required. It is
/// done for testing purposes.
/// UML diagram doc/LabViewVIUML.txt
/// Unit tests in test_CsvFile.cpp
class NamedValue {
public:
    using Ptr = std::shared_ptr<NamedValue>;
    using Map = std::map<std::string, Ptr>;

    NamedValue(std::string const& name) : _name(name) {}
    NamedValue() = delete;
    NamedValue(NamedValue const&) = default;

    virtual ~NamedValue() = default;

    /// Add NamedValue::Ptr `obj` to `nvMap`.
    /// @throw runtime_error if `obj` was already in the map.
    static void setup(Ptr const& obj, Map& nvMap);

    /// Set the values in `outputMap` so that they don't equal the values read from the CSV file.
    static void voidValForTest(Map& outputMap);

    /// Return the name for this value.
    std::string getName() const { return _name; }

    /// Set `_valueRead` using the value of `val`.
    virtual void setValFromValueRead() = 0;

    /// Set the `_valueRead` and `val` for everything in `nvMap` from the `row` in `csvFile`.
    /// The first row of `csvFile` must contain the column names. Duplicate
    /// column names are not allowed.
    /// @throw runtime_error if a `NamedValue` in `nvMap` cannot be found in `csvFile`, or the row doesn't
    /// exist.
    static void setMapValuesFromFile(Map& nvMap, CsvFile& csvFile, int row);

    /// Return true if the value (`val`) is within tolerance of its expected value (`_valueRead`).
    virtual bool check() const = 0;

    /// Set `_valueRead` to an appropriate value based on the contents of string.
    /// @throws runtime_error if `str` has an inappropriate value for type `T`.
    virtual void setFromString(std::string const& str) = 0;

    /// Set member `val` to a value different from the `_valueRead`.
    /// This helps determine that the value was during the test and isn't just
    /// leftover from reading the file.
    virtual void voidValForTest() = 0;

    /// Insert the individual map elements of `src` into `dest`.
    /// @throw runtime_error if there's a duplicate element key in the list.
    static void insertMapElements(Map& src, Map& dest);

    /// Write the contents of `nvMap` into the `os` stream.
    static std::ostream& mapDump(std::ostream& os, Map const& nvMap);

    /// Return a string, appropriate for the log, with the contents of `nvMap`.
    static std::string mapDumpStr(Map const& nvMap);

    /// Return a log worthy string of this object, see `std::ostream& dump(std::ostream& os)`.
    std::string dumpStr() const {
        std::stringstream os;
        dump(os);
        return os.str();
    }

    /// Return a log worthy string of this object. Child classes should override
    /// this function. This is called by the `operator<<` function below.
    virtual std::ostream& dump(std::ostream& os) const = 0;

    /// Log `msg` at the warning level. It's undesirable to have
    /// Log.h included in the header, and the template can't be defined in a cpp
    /// file, so there's this.
    void logWarn(std::string const& msg) const;

private:
    std::string const _name;  ///< The name of this variable as found in the file.
};

/// `operator<<` for NamedValue and all of its derived classes.
inline std::ostream& operator<<(std::ostream& os, NamedValue const& nVal) {
    nVal.dump(os);
    return os;
}

/// This class stores a variable with a name so it can be set from a CSV file or similar.
/// Unit tests in test_CsvFile.cpp
template <class T>
class NamedValueType : public NamedValue {
public:
    using Ptr = std::shared_ptr<NamedValueType>;

    /// Constructor
    /// @param name - Name of the key for this value.
    /// @param defaultVal - Default value this item (`val`).
    NamedValueType(std::string const& name, T const& defaultVal)
            : NamedValue(name), val(defaultVal), _valueRead(defaultVal) {}

    virtual ~NamedValueType() = default;

    /// Set `_valueRead` and `val` to `value`.
    void setValueRead(T const& value) {
        val = value;
        _valueRead = val;
    }

    /// Return the `valueRead` for this.
    T getValueRead() const { return _valueRead; }

    /// Set `val` using the value of `_valueRead`.
    void setValFromValueRead() override { val = getValueRead(); }

    /// Return true if `val` is equal, or in the case of floats, nearly equal to `_valueRead`.
    virtual bool approxEqual(T const& val) const { return (_valueRead == val); }

    /// Return true if `val` is within `_tolerance` of `_valueRead`.
    bool check() const override {
        bool result = approxEqual(val);
        if (!result) {
            std::string msg = "check failed " + dumpStr();
            logWarn(msg);
        }
        return result;
    }

    /// Return a log worthy string of this object. Child classes should override
    /// this function if other information is needed. This is called by the
    /// `operator<<` function below.
    std::ostream& dump(std::ostream& os) const override {
        os << getName() << "(" << val << ", read=" << getValueRead() << ")";
        return os;
    }

    /// The value for this instance.
    /// For inputs and constants, this will be set from `_valueRead`.
    /// For outputs, this will be compared to `_valueRead` using approxEqual.
    T val;

private:
    T _valueRead;  ///< The value of this variable as read from the file.
};

/// This class extends `NamedValue` to store a string value so it can be set from a CSV file or similar.
class NamedString : public NamedValueType<std::string> {
public:
    using Ptr = std::shared_ptr<NamedString>;

    /// Constructor
    /// @param name - Name of the key for this value.
    /// @param defaultVal - Default string value this item (`val`).
    NamedString(std::string const& name, std::string const& defaultVal = "")
            : NamedValueType(name, defaultVal) {}

    /// Create a pointer to a new instance of `NamedString` and add it to `nvMap`.
    static Ptr create(std::string const& name, Map& nvMap, std::string const& defaultVal = "") {
        Ptr obj = Ptr(new NamedString(name, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set `_valueRead` from `str`, no conversion needed.
    void setFromString(std::string const& str) override { setValueRead(str); }

    /// Set member `val` to a value different from the `_valueRead`.
    void voidValForTest() override { val = ""; }
};

/// This class extends `NamedValue` to store a bool value so it can be set from a CSV file or similar.
/// Acceptable string inputs are `true` and `false` where capitalization doesn't matter.
class NamedBool : public NamedValueType<bool> {
public:
    using Ptr = std::shared_ptr<NamedBool>;

    /// Constructor
    /// @param name - Name of the key for this value.
    /// @param defaultVal - Default bool value this item (`val`).
    NamedBool(std::string const& name, bool defaultVal = false) : NamedValueType(name, defaultVal) {}

    /// Create a pointer to a new instance of `NamedBool` and add it to `nvMap`.
    static Ptr create(std::string const& name, Map& nvMap, bool defaultVal = 0) {
        Ptr obj = Ptr(new NamedBool(name, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set `_valueRead` from `str`, must be 'true' or 'false' (ignores case).
    /// @throws runtime_error when `str` is not 'true' or 'false'.
    void setFromString(std::string const& str) override;

    /// Set member `val` to a value different from the `_valueRead`.
    void voidValForTest() override { val = !getValueRead(); }
};

/// This class extends `NamedValue` to store an integer value so it can be set from a CSV file or similar.
class NamedInt : public NamedValueType<int> {
public:
    using Ptr = std::shared_ptr<NamedInt>;

    /// Constructor
    /// @param name - Name of the key for this value.
    /// @param defaultVal - Default int value this item (`val`).
    NamedInt(std::string const& name, int defaultVal = false) : NamedValueType(name, defaultVal) {}

    /// Create a pointer to a new instance of `NamedInt` and add it to `nvMap`.
    static Ptr create(std::string const& name, Map& nvMap, int defaultVal = 0) {
        Ptr obj = Ptr(new NamedInt(name, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set the integer value from the string `str`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;

    /// Set member `val` to a value different from the `_valueRead`.
    void voidValForTest() override { val = -987654; }
};

/// This class extends `NamedValue` to store a double value so it can be set from a CSV file or similar.
/// Unit tests in test_CsvFile.cpp
class NamedDouble : public NamedValueType<double> {
public:
    using Ptr = std::shared_ptr<NamedDouble>;

    static constexpr double TOLERANCE = 0.000'001;

    /// Constructor
    /// @param name - Name of the key for this value.
    /// @param defaultVal - Default double value this item (`val`).
    NamedDouble(std::string const& name, double tolerance = TOLERANCE, double defaultVal = 0.0)
            : NamedValueType(name, defaultVal), _tolerance(tolerance) {}

    /// Create a pointer to a new instance of `NamedDouble` and add it to `nvMap`.
    static Ptr create(std::string const& name, Map& nvMap, double tolerance = TOLERANCE,
                      double defaultVal = 0.0) {
        Ptr obj = Ptr(new NamedDouble(name, tolerance, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set the double value from the string `str`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;

    /// Return the double value of the string `str`, doing the heavy lifting for
    /// `setFromString()`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    double getValOfString(std::string const& str) const;

    /// Return true if `inV` is within tolerance of the stored value.
    bool approxEqual(double const& inV) const override {
        double delta = getDelta(inV);
        return ((delta * delta) <= (_tolerance * _tolerance));
    }

    /// Return the difference between `_valueRead` and `inV`.
    double getDelta(double const& inV) const { return getValueRead() - inV; }

    /// Return the allowed tolerance.
    double getAllowedTolerance() const { return _tolerance; }

    /// Return true if `val` is within `_tolerance` of `_valueRead`.
    bool check() const override {
        bool result = approxEqual(val);
        if (!result) {
            double delta = getDelta(val);
            std::string msg = "check failed " + dumpStr() + " delta=" + std::to_string(delta) +
                              " tol=" + std::to_string(_tolerance);
            logWarn(msg);
        }
        return result;
    }

    /// Return `_tolerance`
    double getTolerance() const { return _tolerance; }

    /// Set member `val` to a value different from the `_valueRead`.
    void voidValForTest() override { val = -9876543210.0; }

private:
    /// The acceptable variation at which this can be considered approximately equal.
    double _tolerance;
};

/// This class repressents a named angle, based on `NamedDouble`, the internal units
/// are radians. The CSV files tend to be in degrees as they are generated from
/// LabView that uses degrees.
/// `val` from the parent is in radians.
/// Unit tests in test_CsvFile.cpp
class NamedAngle : public NamedDouble {
public:
    using Ptr = std::shared_ptr<NamedAngle>;

    enum UnitType { RADIAN, DEGREE };

    /// FUTURE: constants and constrain functions should probably live somewhere else.
    /// The value of PI and related constants.
    static constexpr double PI = M_PI;
    static constexpr double PI2 = PI * 2.0;
    static constexpr double RADPERDEG = (PI / 180.0);
    static constexpr double DEGPERRAD = (180.0 / PI);

    /// Return the value, in radians, constrained to -PI to +PI.
    static double constrain(double radians) {
        radians = std::fmod(radians, PI2);
        while (radians < -PI) radians += PI2;
        while (radians >= PI) radians -= PI2;
        return radians;
    }

    /// Return the value, in radians, constrained from 0 to +2PI.
    static double constrain0to2Pi(double radians) {
        radians = std::fmod(radians, PI2);
        while (radians < 0.0) radians += PI2;
        while (radians >= PI2) radians -= PI2;
        return radians;
    }

    /// Constructor, `expectedUnits` are RADIAN or DEGREE, which is used in `setFromString()`
    /// @param name - Name of the key for this item.
    /// @param expectedUnits - Units used with `setFromString()` `RADIAN` or `DEGREE`
    /// @param tolerance - Always radians, how much `val` can differ from `_valueRead` and still be
    ///                    `approxEqual()`.
    /// @param defaultVal - Always radians, default value of `val`.
    NamedAngle(std::string const& name, UnitType expectedUnits = DEGREE, double tolerance = TOLERANCE,
               double defaultVal = 0.0)
            : NamedDouble(name, tolerance, defaultVal), _expectedUnits(expectedUnits) {}

    /// Create a pointer to a new instance of `NamedAngle` and add it to `nvMap`.
    static Ptr create(std::string const& name, Map& nvMap, UnitType expectedUnits = DEGREE,
                      double tolerance = TOLERANCE, double defaultVal = 0) {
        Ptr obj = Ptr(new NamedAngle(name, expectedUnits, tolerance, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set the double value from the string `str`, using radians or degrees as
    /// indicated by `_expectedUnits`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;

    /// Set the double value from the string `str` where `str` is in radians.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromStringRad(std::string const& str) {
        double v = getValOfString(str);
        setRadRead(v);
    }

    /// Set the double value from the string `str` where `str` is in degrees.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromStringDeg(std::string const& str) {
        double v = getValOfString(str);
        setDegRead(v);
    }

    /// Return true if `val` is within `_tolerance` of `_valueRead`.
    bool check() const override {
        bool result = approxEqualRad(val);
        if (!result) {
            double delta = getDelta(val);
            std::string msg = "check failed " + dumpStr() + " delta=" + std::to_string(delta) +
                              " tol=" + std::to_string(getTolerance());
            logWarn(msg);
        }
        return result;
    }

    /// Return true if `inV` is within tolerance of the stored value, using `_expectedUnits`
    bool approxEqual(double const& inV) const override;

    /// Return true if `inV` is within tolerance of the stored value, using `_expectedUnits`
    bool approxEqualRad(double const& inV) const;

    /// Return true if `inV` is within tolerance of the stored value, using `_expectedUnits`
    bool approxEqualDeg(double const& inV) const;

    /// Set the value of the angle in `degrees`.
    void setDegRead(double degrees) { setRadRead(degrees * RADPERDEG); }

    /// Set the value of the angle in `radians`.
    void setRadRead(double radians) { setValueRead(radians); }

    /// Returns the value of the angle in degrees.
    double getDegRead() const { return getValueRead() * DEGPERRAD; }

    /// Returns the value from the file of the angle in radians.
    double getRadRead() const { return getValueRead(); }

    /// Show both radian and degree values
    std::ostream& dump(std::ostream& os) const override {
        os << getName() << "(Rad=" << val << " read(Rad=" << getRadRead() << ",Deg=" << getDegRead() << "))";
        return os;
    }

private:
    /// Indicates if string input is expected to be radians or degrees.
    UnitType _expectedUnits;
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_NAMEDVALUE_H
