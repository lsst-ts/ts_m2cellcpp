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
#ifndef LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
#define LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H

// System headers
#include <arpa/inet.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Third party headers
#include "nlohmann/json.hpp"

// project headers
#include "util/Issue.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// Exception specific to Telemetry.
///
/// unit test: test_TelemetryCom.cpp
class TelemetryException : public util::Issue {
public:
    TelemetryException(Context const& ctx, std::string const& msg) : util::Issue(ctx, msg) {}
};


class TelemetryItem;

/// Type the `TelemetryItemMap`. Objects of this will be copied, so the
/// map must store pointers.
typedef std::map<std::string, std::shared_ptr<TelemetryItem>> TelemetryItemMap;
typedef std::shared_ptr<TelemetryItemMap> TelemetryItemMapPtr;

/// One item that can be sent by the server in `TelemetryCom`. This class
/// can be used to build items with several elements.
/// @see `TItemPowerStatusBase` and `TItemDouble` as examples.
/// as an example.
/// Unit tests in tests/test_TelemetryCom
class TelemetryItem {
public:
    using Ptr = std::shared_ptr<TelemetryItem>;

    /// Return true if `item` was successfully inserted into `tiMap`.
    static bool insert(TelemetryItemMap& tiMap, Ptr const& item) {
        auto ret = tiMap.insert(std::make_pair(item->getId(), item));
        bool insertSuccess = ret.second;
        return insertSuccess;
    }

    /// Create a `TelemetryItem` with immutable id of `id`.
    TelemetryItem(std::string const& id) : _id(id) {}
    virtual ~TelemetryItem() = default;

    TelemetryItem() = delete;
    TelemetryItem(TelemetryItem const&) = delete;
    TelemetryItem& operator=(TelemetryItem const&) = delete;

    /// Return the id string
    std::string getId() const { return _id; };

    /// Return a json object containing the id.
    virtual nlohmann::json getJson() const = 0;

    /// Return true if this item and `other` have the same id and values.
    virtual bool compareItem(TelemetryItem const& other) const = 0;

    /// Parse the json string `jStr` and load the appropriate values into this object.
    /// @return true if item parsed correctly and had appropriate data.
    bool parse(std::string const& jStr); // &&& use TelemetryException ???

    /// Set the value of this `TelemetryItem` from `js`.
    /// @param idExpected - If set to true, `js` must contain a valid entry for `id`.
    /// @return true if the value of all relate items could be set from `js`
    virtual bool setFromJson(nlohmann::json const& js, bool idExpected = false) = 0;

    /// Return a string reasonable for logging.
    std::string dump() const {
        std::stringstream os;
        os << getJson();
        return os.str();
    }

    /// Add a reasonable string representation of `item` for logging to `os`
    friend std::ostream& operator<<(std::ostream& os, TelemetryItem const& item);

protected:
    /// Build a json object using this object and the provided `tMap`.
    nlohmann::json buildJsonFromMap(TelemetryItemMap const& tMap) const;

    /// Return false if values in `tMap` cannot be set from `js` and `idExpected`.
    bool setMapFromJson(TelemetryItemMap& tMap, nlohmann::json const& js, bool idExpected);

    /// Return false if `idExpected` and the `js` "id" entry is wrong.
    bool checkIdCorrect(nlohmann::json const& js, bool idExpected) const;

private:
    std::string const _id;
};

/// This class is used to store a specific telemetry item with a double value.
/// Unit tests in tests/test_TelemetryCom
class TItemDouble : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemDouble>;

    /// Create a shared pointer instance of TItemDouble using `id`, and `defaultVal`, and
    /// insert it into the map `itMap`.
    /// @see `TItemDouble`
    /// @throws util::Bug if the new object cannot be inserted into the list  // &&& use TelemetryException ???
    static Ptr create(std::string const& id, TelemetryItemMap* itMap, double defaultVal = 0.0);
    TItemDouble() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal`.
    TItemDouble(std::string const& id, double defaultVal = 0.0) : TelemetryItem(id), _val(defaultVal) {}

    //// Set the value of the object to `val`.
    void setVal(double val) { _val = val; }

    //// Return the value of this object.
    double getVal() const { return _val; }

    /// Return the json representation of this object.
    nlohmann::json getJson() const override;

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override;

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override;

private:
    std::atomic<double> _val;
};

/// &&& doc
class TItemDoubleVector : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemDoubleVector>;

    /// Create a shared pointer instance of TItemDoubleVector using `id`, number of elements `size, and `defaultVal` for
    /// all entries, then insert it into the map `itMap`.
    /// @throws util::Bug if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, size_t size, TelemetryItemMap* itMap, double defaultVal = 0.0);
    TItemDoubleVector() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemDoubleVector(std::string const& id, int size, double defaultVal = 0.0) : TelemetryItem(id), _size(size) {
        for(size_t j=0; j<_size; ++j) {
            _vals.push_back(defaultVal);
        }
    }

    /// &&&
    ~TItemDoubleVector() override {}

    //// Return a copy of the values in this object.
    std::vector<double> getVals() const;

    /// Set the value of the object to `vals`, which must have the same size as `_vals`.
    /// @return false if the size of `vals` is different from `_size`.
    bool setVals(std::vector<double> const& vals);

    /// Set the value at `index` in `_vals` to val.
    /// @return false if index is out of range.
    bool setVal(size_t index, double val);

    //// Return the value at `index` in `_vals`.
    /// @throw TelemetryException if index is out of range.
    double getVal(size_t index) const;

    /// Return the json representation of this object.
    nlohmann::json getJson() const override;

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override;

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override;

private:
    size_t const _size; ///< Number of elements in `_vals`.
    std::vector<double> _vals; ///< Vector containing the double values with length _size.
    mutable std::mutex _mtx; ///< Protects `_vals`.
};



/// common elements between powerStatus and powerStatusRaw
/// Unit tests in tests/test_TelemetryCom
class TItemPowerStatusBase : public TelemetryItem {
public:
    TItemPowerStatusBase(std::string const& id) : TelemetryItem(id) {}

    virtual ~TItemPowerStatusBase() = default;

    /// Set `_motorVoltage` to `val`.
    void setMotorVoltage(double val) { _motorVoltage->setVal(val); }

    /// Set `_motorCurrent` to `val`.
    void setMotorCurrent(double val) { _motorCurrent->setVal(val); }

    /// Set `_commVoltage` to `val`.
    void setCommVoltage(double val) { _commVoltage->setVal(val); }

    /// Set `_commCurrent` to `val`.
    void setCommCurrent(double val) { _commCurrent->setVal(val); }

    /// Return the value of `_motorVoltage`.
    double getMotorVoltage() { return _motorVoltage->getVal(); }

    /// Return the value of `_motorCurrent`.
    double getMotorCurrent() { return _motorCurrent->getVal(); }

    /// Return the value of `_commVoltage`.
    double getCommVoltage() { return _commVoltage->getVal(); }

    /// Return the value of `_commCurrent`.
    double getCommCurrent() { return _commCurrent->getVal(); }

    /// Local override of getJson
    nlohmann::json getJson() const override { return buildJsonFromMap(_map); }

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override {
        return setMapFromJson(_map, js, idExpected);
    }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override;

private:
    /// Map of items for this TelemetryItem. Does not change after constructor.
    TelemetryItemMap _map;
    TItemDouble::Ptr _motorVoltage = TItemDouble::create("motorVoltage", &_map);
    TItemDouble::Ptr _motorCurrent = TItemDouble::create("motorCurrent", &_map);
    TItemDouble::Ptr _commVoltage = TItemDouble::create("commVoltage", &_map);
    TItemDouble::Ptr _commCurrent = TItemDouble::create("commCurrent", &_map);
};

/// This class is used to store data for the "powerStatus" entry.
/// Unit tests in tests/test_TelemetryCom
class TItemPowerStatus : public TItemPowerStatusBase {
public:
    using Ptr = std::shared_ptr<TItemPowerStatus>;

    TItemPowerStatus() : TItemPowerStatusBase("powerStatus") {}
    virtual ~TItemPowerStatus() = default;
};

/// This class is used to store data for the "powerStatusRaw" entry.
/// Unit tests in tests/test_TelemetryCom
class TItemPowerStatusRaw : public TItemPowerStatusBase {
public:
    using Ptr = std::shared_ptr<TItemPowerStatusRaw>;

    TItemPowerStatusRaw() : TItemPowerStatusBase("powerStatusRaw") {}
    virtual ~TItemPowerStatusRaw() = default;
};



/// &&& doc
class TItemTangentForce : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTangentForce>;

    TItemTangentForce() : TelemetryItem("tangentForce") {}

    ~TItemTangentForce() override {};

    //&&&/// Set `_motorVoltage` to `val`.
    //&&&void setMotorVoltage(double val) { _motorVoltage->setVal(val); }

    /// &&& doc
    TItemDoubleVector& getLutGravity() const { return *_lutGravity; }

    /// &&& doc
    TItemDoubleVector& getLutTemperature() const { return *_lutTemperature; }

    /// &&& doc
    TItemDoubleVector& getApplied() const { return *_applied; }

    /// &&& doc
    TItemDoubleVector& getMeasured() const { return *_measured; }

    /// &&& doc
    TItemDoubleVector& getHardpointCorrection() const { return *_hardpointCorrection; }

    /// Local override of getJson
    nlohmann::json getJson() const override { return buildJsonFromMap(_map); }

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override {
        return setMapFromJson(_map, js, idExpected);
    }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override;

private:
    /// Map of items for this TelemetryItem. Does not change after constructor.
    TelemetryItemMap _map;
    TItemDoubleVector::Ptr _lutGravity = TItemDoubleVector::create("lutGravity", 6, &_map);
    TItemDoubleVector::Ptr _lutTemperature = TItemDoubleVector::create("lutTemperature", 0, &_map);
    TItemDoubleVector::Ptr _applied = TItemDoubleVector::create("applied", 6, &_map);
    TItemDoubleVector::Ptr _measured = TItemDoubleVector::create("measured", 6, &_map);
    TItemDoubleVector::Ptr _hardpointCorrection = TItemDoubleVector::create("hardpointCorrection", 6, &_map);
};


}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
