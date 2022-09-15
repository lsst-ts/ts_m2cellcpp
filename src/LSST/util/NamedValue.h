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
#include <stdint.h>
#include <string>
#include <vector>

namespace LSST {
namespace m2cellcpp {
namespace util {

/// This class stores a variable with a name so it can be set from a CSV file or similar.
/// Unit tests in test_CsvFile.cpp
template <class T>
class NamedValue {
public:
    using Ptr = std::shared_ptr<NamedValue>;
    using Map = std::map<std::string, Ptr>;

    NamedValue(std::string const& name) : _name(name) {}
    NamedValue() = delete;
    NamedValue(NamedValue const&) = default;

    ~NamedValue() = default;

    /// Return the name for this value.
    std::string getName() const { return _name; }

    /// Set `_value` to `val`.
    /// @param val
    void setValue(T const& val) { _value = val; }

    /// Return the value of the class
    T getValue() const { return _value; }

    /// Return true if `val` is equal, or in the case of floats, nearly equal to `_value`.
    virtual bool approxEqual(T const& val) const { return (_value == val); }

    /// Set `_value` to an appropriate value based on the contents of string.
    /// @throws runtime_error if `str` has an inappropriate value for type `T`.
    virtual void setFromString(std::string const& str) = 0;

    /// Return a log worthy string of this object, see `std::ostream& dump(std::ostream& os)`.
    std::string dump() const {
        std::stringstream os;
        dump(os);
        return os.str();
    }

    /// Return a log worthy string of this object. Child classes should override
    /// this function if other information is needed. This is called by the
    /// `operator<<` function below.
    virtual std::ostream& dump(std::ostream& os) const {
        os << getName() << "(" << getValue() << ")";
        return os;
    }

private:
    std::string _name;
    T _value;
};

/// `operator<<` for NamedValue and all of its derived classes.
template <class T>
std::ostream& operator<<(std::ostream& os, NamedValue<T> const& val) {
    val.dump(os);
    return os;
}

/// This class extends `NamedValue` to store a string value so it can be set from a CSV file or similar.
class NamedString : public NamedValue<std::string> {
public:
    using Ptr = std::shared_ptr<NamedString>;
    NamedString(std::string const& name) : NamedValue(name) { setValue(""); }

    /// Set `_value` from `str`, no conversion needed.
    void setFromString(std::string const& str) override { setValue(str); }
};

/// This class extends `NamedValue` to store a bool value so it can be set from a CSV file or similar.
/// Acceptable string inputs are `true` and `false` where capitalization doesn't matter.
class NamedBool : public NamedValue<bool> {
public:
    using Ptr = std::shared_ptr<NamedBool>;
    NamedBool(std::string const& name) : NamedValue(name) { setValue(false); }

    /// Set `_value` from `str`, must be 'true' or 'false' (ignores case).
    /// @throws runtime_error when `str` is not 'true' or 'false'.
    void setFromString(std::string const& str) override;
};

/// This class extends `NamedValue` to store an integer value so it can be set from a CSV file or similar.
class NamedInt : public NamedValue<int> {
public:
    using Ptr = std::shared_ptr<NamedInt>;
    NamedInt(std::string const& name) : NamedValue(name) { setValue(0); }

    /// Set the integer value from the string `str`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;
};

/// This class extends `NamedValue` to store a double value so it can be set from a CSV file or similar.
/// Unit tests in test_CsvFile.cpp
class NamedDouble : public NamedValue<double> {
public:
    using Ptr = std::shared_ptr<NamedDouble>;
    NamedDouble(std::string const& name, double allowedTolorance = 0.000000000001)
            : NamedValue(name), _allowedTolorance(allowedTolorance) {
        setValue(0.0);
    }

    /// Set the double value from the string `str`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;

    /// Return the double value of the string `str`, doing the heavy lifting for
    /// `setFromString()`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    double getValOfString(std::string const& str) const;

    /// Return true if `val` is within tolerance of the stored value.
    bool approxEqual(double const& val) const override {
        double delta = getValue() - val;
        return (delta * delta < _allowedTolorance * _allowedTolorance);
    }

    /// Return the allowed tolorance.
    double getAllowedTolorance() const { return _allowedTolorance; }

private:
    /// The acceptable variation at which this can be considered approximately equal.
    double _allowedTolorance;
};

/// This class repressents a named angle, based on `NamedDouble`. The internal units
/// are radians, but the CSV files tend to be in degrees as they are generated from
/// LabView that uses in degrees.
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
    /// @param name
    NamedAngle(std::string const& name, UnitType expectedUnits = DEGREE)
            : NamedDouble(name), _expectedUnits(expectedUnits) {}

    /// Set the double value from the string `str`, using radians or degrees as
    /// indicated by `_expectedUnits`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;

    /// Set the double value from the string `str` where `str` is in radians.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromStringRad(std::string const& str) {
        double v = getValOfString(str);
        setRad(v);
    }

    /// Set the double value from the string `str` where `str` is in degrees.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromStringDeg(std::string const& str) {
        double v = getValOfString(str);
        setDeg(v);
    }

    /// Return true if `val` is within tolerance of the stored value, using `_expectedUnits`
    bool approxEqual(double const& val) const override;

    /// Return true if `val` is within tolerance of the stored value, using `_expectedUnits`
    bool approxEqualRad(double const& val) const;

    /// Return true if `val` is within tolerance of the stored value, using `_expectedUnits`
    bool approxEqualDeg(double const& val) const;

    /// Set the value of the angle in `degrees`.
    void setDeg(double degrees) { setRad(degrees * RADPERDEG); }

    /// Set the value of the angle in `radians`.
    void setRad(double radians) { setValue(radians); }

    /// Returns the value of the angle in degrees.
    double getDeg() const { return getValue() * DEGPERRAD; }

    /// Returns the value of the angle in radians.
    double getRad() const { return getValue(); }

    /// Show both radian and degree values
    std::ostream& dump(std::ostream& os) const override {
        os << getName() << "(Rad=" << getRad() << ",Deg=" << getDeg() << ")";
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
