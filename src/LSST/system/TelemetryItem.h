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
#include "util/Log.h"

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

    /// Return false if `tiMap` is not null and item` was not successfully inserted into `tiMap`.
    /// Insert `item` into `tiMap`.
    /// @throw Telemetry
    static void insert(TelemetryItemMap* tiMap, Ptr const& item);

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
    /// @throws TelemetryException if the new object cannot be inserted into the list.
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
template <class VT>
class TItemVector : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemVector>;

    /* &&&
    /// Create a shared pointer instance of TItemDVector using `id`, number of elements `size, and `defaultVal` for
    /// all entries, then insert it into the map `itMap`.
    /// @throws util::Bug if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, size_t size, TelemetryItemMap* tiMap, VT defaultVal = 0) {
        Ptr newItem = Ptr(new TItemVector<VT>(id, size, tiMap, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }
    */
    TItemVector() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVector(std::string const& id, int size, VT defaultVal = 0) : TelemetryItem(id), _size(size) {
        for(size_t j=0; j<_size; ++j) {
            _vals.push_back(defaultVal);
        }
    }

    /// &&&
    ~TItemVector() override {}

    //// Return a copy of the values in this object.
    std::vector<VT> getVals() const {
        std::vector<VT> outVals;
        std::lock_guard<std::mutex> lg(_mtx);
        for (auto const& v : _vals) {
            outVals.push_back(v);
        }
        return outVals;
    }

    /// Set the value of the object to `vals`, which must have the same size as `_vals`.
    /// @return false if the size of `vals` is different from `_size`.
    bool setVals(std::vector<VT> const& vals) {
        if (vals.size() != _size) {
            LERROR("TItemVector::setVals wrong size vals.size()=", vals.size(), " for ", dump());
            return false;
        }
        std::lock_guard<std::mutex> lg(_mtx);
        for (size_t j = 0; j < _size; ++j) {
            _vals[j] = vals[j];
        }
        return true;
    }

    /// Set the value at `index` in `_vals` to val.
    /// @return false if index is out of range.
    bool setVal(size_t index, VT val) {
        if (index > _size) {
            return false;
        }
        std::lock_guard<std::mutex> lg(_mtx);
        _vals[index] = val;
        return true;
    }

    //// Return the value at `index` in `_vals`.
    /// @throw TelemetryException if index is out of range.
    VT getVal(size_t index) const {
    if (index > _size) {
        throw TelemetryException(ERR_LOC, "TItemVector::getVal out of range for index=" + std::to_string(index) + " for " + dump());
    }
    std::lock_guard<std::mutex> lg(_mtx);
    return _vals[index];
}

    /// Return the json representation of this object.
    nlohmann::json getJson() const override {
        nlohmann::json js;
        nlohmann::json jArray = nlohmann::json::array();

        std::lock_guard<std::mutex> lg(_mtx);
        for (size_t j = 0; j < _size; ++j) {
            jArray.push_back(_vals[j]);
        }
        js[getId()] = jArray;
        return js;
    }

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override {
        if (idExpected) {
            // This type can only have a floating point value array for `_val`.
            LERROR("TItemVector::setFromJson cannot have a json 'id' entry");
            return false;
        }
        try {
            std::vector<VT> vals = js.at(getId());
            std::string valsStr;                      // &&& delete
            for (auto const& v : vals) {         // &&& delete
                valsStr += std::to_string(v) + ", ";  // &&& delete
            }                                    // &&& delete
            LDEBUG("&&& TItemVector::setFromJson vals=", valsStr, " js.at=", js.at(getId()));
            return setVals(vals);
        } catch (nlohmann::json::out_of_range const& ex) {
            LERROR("TItemVector::setFromJson out of range for ", getId(), " js=", js);
        }
        return false;
    }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        try {
            TItemVector const& otherT = dynamic_cast<TItemVector const&>(other);
            if (getId() != other.getId() || _size != otherT._size) {
                return false;
            }
            // There's no way to reliably control which gets locked first
            // (both A.compareItem(B) or B.compareItem(A) are possible),
            // so this is needed to lock both without risk of deadlock.
            std::unique_lock lockThis(_mtx, std::defer_lock);
            std::unique_lock lockOther(otherT._mtx, std::defer_lock);
            std::lock(lockThis, lockOther);
            return (_vals == otherT._vals);
        } catch (std::bad_cast const& ex) {
            return false;
        }
    }

private:
    size_t const _size; ///< Number of elements in `_vals`.
    std::vector<VT> _vals; ///< Vector containing the double values with length _size.
    mutable std::mutex _mtx; ///< Protects `_vals`.
};


/// &&& doc
class TItemVectorDouble : public TItemVector<double> {
public:
    using Ptr = std::shared_ptr<TItemVectorDouble>;

    /// Create a shared pointer instance of TItemVectorDouble using `id`, number of elements `size, and `defaultVal` for
    /// all entries, then insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, size_t size, TelemetryItemMap* tiMap, double defaultVal = 0.0) {
        Ptr newItem = Ptr(new TItemVectorDouble(id, size, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }

    TItemVectorDouble() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVectorDouble(std::string const& id, int size, double defaultVal = 0.0) : TItemVector(id, size, defaultVal) {}

    /// &&&
    ~TItemVectorDouble() override {}
};

/// &&& doc
class TItemVectorInt : public TItemVector<int> {
public:
    using Ptr = std::shared_ptr<TItemVectorInt>;

    /// Create a shared pointer instance of TItemVectorInt using `id`, number of elements `size, and `defaultVal` for
    /// all entries, then insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, size_t size, TelemetryItemMap* tiMap, int defaultVal = 0) { // &&& this can probably be done with a template function.
        Ptr newItem = Ptr(new TItemVectorInt(id, size, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }
    TItemVectorInt() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVectorInt(std::string const& id, int size, int defaultVal = 0) : TItemVector(id, size, defaultVal) {}

    /// &&&
    ~TItemVectorInt() override {}
};


/// common elements between powerStatus and powerStatusRaw
/// Unit tests in tests/test_TelemetryCom
class TItemPowerStatusBase : public TelemetryItem {
public:
    TItemPowerStatusBase(std::string const& id) : TelemetryItem(id) {}

    virtual ~TItemPowerStatusBase() = default;

    /* &&&
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
    */

    /// Return reference to `_motorVoltage`.
    TItemDouble& getMotorVoltage() { return *_motorVoltage; }

    /// Return reference to `_motorCurrent`.
    TItemDouble& getMotorCurrent() { return *_motorCurrent; }

    /// Return reference to `_commVoltage`.
    TItemDouble& getCommVoltage() { return *_commVoltage; }

    /// Return reference to `_commCurrent`.
    TItemDouble& getCommCurrent() { return *_commCurrent; }


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
    TItemVectorDouble& getLutGravity() const { return *_lutGravity; }

    /// &&& doc
    TItemVectorDouble& getLutTemperature() const { return *_lutTemperature; }

    /// &&& doc
    TItemVectorDouble& getApplied() const { return *_applied; }

    /// &&& doc
    TItemVectorDouble& getMeasured() const { return *_measured; }

    /// &&& doc
    TItemVectorDouble& getHardpointCorrection() const { return *_hardpointCorrection; }

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
    TItemVectorDouble::Ptr _lutGravity = TItemVectorDouble::create("lutGravity", 6, &_map);
    TItemVectorDouble::Ptr _lutTemperature = TItemVectorDouble::create("lutTemperature", 0, &_map);
    TItemVectorDouble::Ptr _applied = TItemVectorDouble::create("applied", 6, &_map);
    TItemVectorDouble::Ptr _measured = TItemVectorDouble::create("measured", 6, &_map);
    TItemVectorDouble::Ptr _hardpointCorrection = TItemVectorDouble::create("hardpointCorrection", 6, &_map);
};

/// &&& doc
class TItemForceBalance : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemForceBalance>;

    TItemForceBalance() : TelemetryItem("forceBalance") {}

    virtual ~TItemForceBalance() = default;

    /// Return pointer to `_fx`.
    TItemDouble& getFx() { return *_fx; }

    /// Return pointer to `_fy`.
    TItemDouble& getFy() { return *_fy; }

    /// Return pointer to `_fz`.
    TItemDouble& getFz() { return *_fz; }

    /// Return pointer to `_mx`.
    TItemDouble& getMx() { return *_mx; }

    /// Return pointer to `_my`.
    TItemDouble& getMy() { return *_my; }

    /// Return pointer to `_`.
    TItemDouble& getMz() { return *_mz; }

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
    TItemDouble::Ptr _fx = TItemDouble::create("fx", &_map);
    TItemDouble::Ptr _fy = TItemDouble::create("fy", &_map);
    TItemDouble::Ptr _fz = TItemDouble::create("fz", &_map);
    TItemDouble::Ptr _mx = TItemDouble::create("mx", &_map);
    TItemDouble::Ptr _my = TItemDouble::create("my", &_map);
    TItemDouble::Ptr _mz = TItemDouble::create("mz", &_map);
};


}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
