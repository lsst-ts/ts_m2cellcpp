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

class NamedValue {
public:
    using Ptr = std::shared_ptr<NamedValue>;
    using Map = std::map<std::string, Ptr>;

    NamedValue(std::string const& name) : _name(name) {}
    NamedValue() = delete;
    NamedValue(NamedValue const&) = default;

    virtual ~NamedValue() = default;

    /// &&& doc
    static void setup(Ptr const& obj, Map& nvMap) { nvMap.insert(std::make_pair(obj->getName(), obj)); }

    /// Return the name for this value.
    std::string getName() const { return _name; }

    /// Set `_valueRead` using the value of `val`.
    virtual void setValFromValueRead() = 0;

    /// &&& doc
    virtual bool check() const = 0;

    /// Set `_valueRead` to an appropriate value based on the contents of string.
    /// @throws runtime_error if `str` has an inappropriate value for type `T`.
    virtual void setFromString(std::string const& str) = 0;

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

    /// &&& doc
    NamedValueType(std::string const& name, T const& defaultVal)
            : NamedValue(name), val(defaultVal), _valueRead(defaultVal) {}

    virtual ~NamedValueType() = default;

    /// Set `_valueRead` to `val`.
    /// @param val
    void setValueRead(T const& val) { _valueRead = val; }

    /// Return the `valueRead` for this.
    T getValueRead() const { return _valueRead; }

    /// Set `val` using the value of `_valueRead`.
    void setValFromValueRead() override { val = getValueRead(); }

    /// Return true if `val` is equal, or in the case of floats, nearly equal to `_valueRead`.
    virtual bool approxEqual(T const& val) const { return (_valueRead == val); }

    /// &&& doc
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

    /// &&& doc
    NamedString(std::string const& name, std::string const& defaultVal = "")
            : NamedValueType(name, defaultVal) {}

    /// &&& doc
    static Ptr create(std::string const& name, Map& nvMap, std::string const& defaultVal = "") {
        Ptr obj = Ptr(new NamedString(name, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set `_valueRead` from `str`, no conversion needed.
    void setFromString(std::string const& str) override { setValueRead(str); }
};

/// This class extends `NamedValue` to store a bool value so it can be set from a CSV file or similar.
/// Acceptable string inputs are `true` and `false` where capitalization doesn't matter.
class NamedBool : public NamedValueType<bool> {
public:
    using Ptr = std::shared_ptr<NamedBool>;

    /// &&& doc
    NamedBool(std::string const& name, bool defaultVal = false) : NamedValueType(name, defaultVal) {}

    /// &&& doc
    static Ptr create(std::string const& name, Map& nvMap, bool defaultVal = 0) {
        Ptr obj = Ptr(new NamedBool(name, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set `_valueRead` from `str`, must be 'true' or 'false' (ignores case).
    /// @throws runtime_error when `str` is not 'true' or 'false'.
    void setFromString(std::string const& str) override;
};

/// This class extends `NamedValue` to store an integer value so it can be set from a CSV file or similar.
class NamedInt : public NamedValueType<int> {
public:
    using Ptr = std::shared_ptr<NamedInt>;

    /// &&& doc
    NamedInt(std::string const& name, int defaultVal = false) : NamedValueType(name, defaultVal) {}

    /// &&& doc
    static Ptr create(std::string const& name, Map& nvMap, int defaultVal = 0) {
        Ptr obj = Ptr(new NamedInt(name, defaultVal));
        setup(obj, nvMap);
        return obj;
    }

    /// Set the integer value from the string `str`.
    /// @throws runtime_error when the conversion is not clean (extra characters, etc.).
    void setFromString(std::string const& str) override;
};

/// This class extends `NamedValue` to store a double value so it can be set from a CSV file or similar.
/// Unit tests in test_CsvFile.cpp
class NamedDouble : public NamedValueType<double> {
public:
    using Ptr = std::shared_ptr<NamedDouble>;

    static constexpr double TOLERANCE = 0.000'001;

    /// &&& doc
    NamedDouble(std::string const& name, double tolerance = TOLERANCE, double defaultVal = 0.0)
            : NamedValueType(name, defaultVal), _allowedTolerance(tolerance) {}

    /// &&& doc
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
        std::string msg = "&&&approxEqual inV=" + std::to_string(inV);
        logWarn(msg);
        double delta = getDelta(inV);
        return ((delta * delta) <= (_allowedTolerance * _allowedTolerance));
    }

    /// &&& doc
    double getDelta(double const& inV) const { return getValueRead() - inV; }

    /// Return the allowed tolerance.
    double getAllowedTolerance() const { return _allowedTolerance; }

    // &&& doc
    bool check() const override {
        bool result = approxEqual(val);
        if (!result) {
            double delta = getDelta(val);
            double mag = 1'000'000'000;  // &&&
            std::string msg = "check failed " + dumpStr() + " delta=" + std::to_string(delta) +
                              " tol=" + std::to_string(_allowedTolerance);
            logWarn(msg);
            msg = "&&& check failed " + dumpStr() + " delta=" + std::to_string(delta * delta * mag) +
                  " tol=" + std::to_string(_allowedTolerance * _allowedTolerance * mag);
            logWarn(msg);
        }
        return result;
    }

    /// Return `_tolerance`
    double getTolerance() const { return _allowedTolerance; }

private:
    /// The acceptable variation at which this can be considered approximately equal.
    double _allowedTolerance;  // &&& change allowedTolerance to tolerance
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
    /// @param name
    NamedAngle(std::string const& name, UnitType expectedUnits = DEGREE, double tolerance = TOLERANCE,
               double defaultVal = 0.0)
            : NamedDouble(name, tolerance, defaultVal), _expectedUnits(expectedUnits) {}

    /// &&& doc
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

    // &&& doc
    bool check() const override {
        bool result = approxEqualRad(val);
        if (!result) {
            double delta = getDelta(val);
            double mag = 1'000'000'000;  // &&&
            std::string msg = "check failed " + dumpStr() + " delta=" + std::to_string(delta) +
                              " tol=" + std::to_string(getTolerance());
            logWarn(msg);
            msg = "&&& check failed " + dumpStr() + " delta=" + std::to_string(delta * delta * mag) +
                  " tol=" + std::to_string(getTolerance() * getTolerance() * mag);
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
