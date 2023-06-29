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

    /// Return true if all elements of `mapA` have a match in `mapB`, where `note` is
    /// used to help identify what is being compared in the log.
    static bool compareTelemetryItemMaps(TelemetryItemMap const& mapA, TelemetryItemMap const& mapB, std::string const& note ="");

    /// Create a `TelemetryItem` with immutable id of `id`.
    TelemetryItem(std::string const& id) : _id(id) {}

    virtual ~TelemetryItem() = default;

    TelemetryItem() = delete;
    TelemetryItem(TelemetryItem const&) = delete;
    TelemetryItem& operator=(TelemetryItem const&) = delete;

    /// Return the id string
    std::string getId() const { return _id; };

    /// Return a json object containing the id.
    virtual nlohmann::json getJson() const {
        return buildJsonFromMap(_map);
    }

    /// Return true if this item and `other` have the same id and values.
    virtual bool compareItem(TelemetryItem const& other) const = 0;

    /// Parse the json string `jStr` and load the appropriate values into this object.
    /// @return true if item parsed correctly and had appropriate data.
    bool parse(std::string const& jStr); // &&& use TelemetryException ???

    /// Set the value of this `TelemetryItem` from `js`.
    /// @param idExpected - If set to true, `js` must contain a valid entry for `id`.
    /// @return true if the value of all relate items could be set from `js`
    virtual bool setFromJson(nlohmann::json const& js, bool idExpected = false) {
        return setMapFromJson(_map, js, idExpected);
    }

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

    /// &&& doc
    template <class TIC>
    static bool compareItemsTemplate(TelemetryItem const& a, TelemetryItem const& b) {
        try {
            TIC const& aItem = dynamic_cast<TIC const&>(a);
            TIC const& bItem = dynamic_cast<TIC const&>(b);
            return compareTelemetryItemMaps(aItem._map, bItem._map);
        } catch (std::bad_cast const& ex) {
            return false;
        }
    }

    /// Map of items for this TelemetryItem. Does not change after constructor.
    TelemetryItemMap _map; // &&& name change needed, maybe make pointer so classes that don't use it can leave it nullptr and the then check the pointer???

private:
    std::string const _id;
};

/// &&& create a TItemSingle template base class for single element items like TItemDouble and TItemBool

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

    virtual ~TItemDouble() {}

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

/// This class is used to store a specific telemetry item with a boolean value.
/// Unit tests in tests/test_TelemetryCom
class TItemBoolean : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemBoolean>;

    /// Create a shared pointer instance of TItemBoolean using `id`, and `defaultVal`, and
    /// insert it into the map `itMap`.
    /// @see `TItemBoolean`
    /// @throws TelemetryException if the new object cannot be inserted into the list.
    static Ptr create(std::string const& id, TelemetryItemMap* itMap, bool defaultVal = false);
    TItemBoolean() = delete;

    virtual ~TItemBoolean() {}

    /// Create a `TItemBoolean` object with identifier `id` and option value `defaultVal`.
    TItemBoolean(std::string const& id, bool defaultVal = false) : TelemetryItem(id), _val(defaultVal) {}

    //// Set the value of the object to `val`.
    void setVal(bool val) { _val = val; }

    //// Return the value of this object.
    bool getVal() const { return _val; }

    /// Return the json representation of this object.
    nlohmann::json getJson() const override;

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override;

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override;

private:
    std::atomic<bool> _val;
};

/// &&& doc
template <class VT>
class TItemVector : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemVector>;

    TItemVector() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVector(std::string const& id, int size, VT defaultVal = 0) : TelemetryItem(id), _size(size) {
        for(size_t j=0; j<_size; ++j) {
            _vals.push_back(defaultVal);
        }
    }

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

    ~TItemVectorInt() override {}
};


/// common elements between powerStatus and powerStatusRaw
/// Unit tests in tests/test_TelemetryCom
class TItemPowerStatusBase : public TelemetryItem {
public:
    TItemPowerStatusBase(std::string const& id) : TelemetryItem(id) {}

    virtual ~TItemPowerStatusBase() = default;

    /// Return reference to `_motorVoltage`.
    TItemDouble& getMotorVoltage() { return *_motorVoltage; }

    /// Return reference to `_motorCurrent`.
    TItemDouble& getMotorCurrent() { return *_motorCurrent; }

    /// Return reference to `_commVoltage`.
    TItemDouble& getCommVoltage() { return *_commVoltage; }

    /// Return reference to `_commCurrent`.
    TItemDouble& getCommCurrent() { return *_commCurrent; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemPowerStatusBase>(*this, other);
    }

private:
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

    /// Return reference to `_lutGravity`.
    TItemVectorDouble& getLutGravity() const { return *_lutGravity; }

    /// Return reference to `_lutTemperature`.
    TItemVectorDouble& getLutTemperature() const { return *_lutTemperature; }

    /// Return reference to `_applied`.
    TItemVectorDouble& getApplied() const { return *_applied; }

    /// Return reference to `_measured`.
    TItemVectorDouble& getMeasured() const { return *_measured; }

    /// Return reference to `_hardpointCorrection`.
    TItemVectorDouble& getHardpointCorrection() const { return *_hardpointCorrection; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTangentForce>(*this, other);
    }

private:
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

    /// Return reference to `_fx`.
    TItemDouble& getFx() { return *_fx; }

    /// Return reference to `_fy`.
    TItemDouble& getFy() { return *_fy; }

    /// Return reference to `_fz`.
    TItemDouble& getFz() { return *_fz; }

    /// Return reference to `_mx`.
    TItemDouble& getMx() { return *_mx; }

    /// Return reference to `_my`.
    TItemDouble& getMy() { return *_my; }

    /// Return reference to `_mz`.
    TItemDouble& getMz() { return *_mz; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemForceBalance>(*this, other);
    }

private:
    TItemDouble::Ptr _fx = TItemDouble::create("fx", &_map);
    TItemDouble::Ptr _fy = TItemDouble::create("fy", &_map);
    TItemDouble::Ptr _fz = TItemDouble::create("fz", &_map);
    TItemDouble::Ptr _mx = TItemDouble::create("mx", &_map);
    TItemDouble::Ptr _my = TItemDouble::create("my", &_map);
    TItemDouble::Ptr _mz = TItemDouble::create("mz", &_map);
};


/// Common elements between position message ids.
/// Unit tests in tests/test_TelemetryCom
class TItemPositionBase : public TelemetryItem {
public:
    TItemPositionBase(std::string const& id) : TelemetryItem(id) {}

    virtual ~TItemPositionBase() = default;

    /// Return reference to `_x`.
    TItemDouble& getX() { return *_x; }

    /// Return reference to `_y`.
    TItemDouble& getY() { return *_y; }

    /// Return reference to `_z`.
    TItemDouble& getZ() { return *_z; }

    /// Return reference to `_xRot`.
    TItemDouble& getXRot() { return *_xRot; }

    /// Return reference to `_y`.
    TItemDouble& getYRot() { return *_yRot; }

    /// Return reference to `_z`.
    TItemDouble& getZRot() { return *_zRot; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemPositionBase>(*this, other);
    }

private:
    TItemDouble::Ptr _x = TItemDouble::create("x", &_map);
    TItemDouble::Ptr _y = TItemDouble::create("y", &_map);
    TItemDouble::Ptr _z = TItemDouble::create("z", &_map);
    TItemDouble::Ptr _xRot = TItemDouble::create("xRot", &_map);
    TItemDouble::Ptr _yRot = TItemDouble::create("yRot", &_map);
    TItemDouble::Ptr _zRot = TItemDouble::create("zRot", &_map);
};

/// This class is used to store data for the "position" entry.
/// Unit tests in tests/test_TelemetryCom
class TItemPosition : public TItemPositionBase {
public:
    using Ptr = std::shared_ptr<TItemPosition>;

    TItemPosition() : TItemPositionBase("position") {}
    virtual ~TItemPosition() = default;
};

/// This class is used to store data for the "positionIMS" entry.
/// Unit tests in tests/test_TelemetryCom
class TItemPositionIMS : public TItemPositionBase {
public:
    using Ptr = std::shared_ptr<TItemPositionIMS>;

    TItemPositionIMS() : TItemPositionBase("positionIMS") {}
    virtual ~TItemPositionIMS() = default;
};

/// &&& doc
class TItemTemperature : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTemperature>;

    TItemTemperature() : TelemetryItem("Temperature") {}

    virtual ~TItemTemperature() = default;

    /// Return pointer to `_ring`.
    TItemVectorDouble& getRing() { return *_ring; }

    /// Return pointer to `_intake`.
    TItemVectorDouble& getIntake() { return *_intake; }

    /// Return pointer to `_exhaust`.
    TItemVectorDouble& getExhaust() { return *_exhaust; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTemperature>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _ring = TItemVectorDouble::create("ring", 12, &_map);
    TItemVectorDouble::Ptr _intake = TItemVectorDouble::create("intake", 2, &_map);
    TItemVectorDouble::Ptr _exhaust = TItemVectorDouble::create("exhaust", 2, &_map);
};

/// &&& doc
class TItemZenithAngle : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemZenithAngle>;

    TItemZenithAngle() : TelemetryItem("zenithAngle") {}

    virtual ~TItemZenithAngle() = default;

    /// Return pointer to `_measured`.
    TItemDouble& getMeasured() { return *_measured; }

    /// Return pointer to `_inclinometerRaw`.
    TItemDouble& getInclinometerRaw() { return *_inclinometerRaw; }

    /// Return pointer to `_inclinometerProcessed`.
    TItemDouble& getInclinometerProcessed() { return *_inclinometerProcessed; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemZenithAngle>(*this, other);
    }

private:
    TItemDouble::Ptr _measured = TItemDouble::create("measured", &_map);
    TItemDouble::Ptr _inclinometerRaw = TItemDouble::create("inclinometerRaw", &_map);
    TItemDouble::Ptr _inclinometerProcessed = TItemDouble::create("inclinometerProcessed", &_map);
};

/// &&& doc
class TItemAxialEncoderPositions : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemAxialEncoderPositions>;

    TItemAxialEncoderPositions() : TelemetryItem("axialEncoderPositions") {}

    virtual ~TItemAxialEncoderPositions() = default;

    /// Return pointer to `_position`.
    TItemVectorDouble& getPosition() { return *_position; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemAxialEncoderPositions>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _position = TItemVectorDouble::create("position", 72, &_map);
};

/// &&& doc
class TItemTangentEncoderPositions : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTangentEncoderPositions>;

    TItemTangentEncoderPositions() : TelemetryItem("tangentEncoderPositions") {}

    virtual ~TItemTangentEncoderPositions() = default;

    /// Return pointer to `_position`.
    TItemVectorDouble& getPosition() { return *_position; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTangentEncoderPositions>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _position = TItemVectorDouble::create("position", 6, &_map);
};

/// &&& doc
class TItemAxialActuatorSteps : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemAxialActuatorSteps>;

    TItemAxialActuatorSteps() : TelemetryItem("axialActuatorSteps") {}

    virtual ~TItemAxialActuatorSteps() = default;

    /// Return pointer to `_steps`.
    TItemVectorInt& getSteps() { return *_steps; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemAxialActuatorSteps>(*this, other);
    }

private:
    TItemVectorInt::Ptr _steps = TItemVectorInt::create("steps", 72, &_map);
};

/// &&& doc
class TItemTangentActuatorSteps : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTangentActuatorSteps>;

    TItemTangentActuatorSteps() : TelemetryItem("tangentActuatorSteps") {}

    virtual ~TItemTangentActuatorSteps() = default;

    /// Return pointer to `_steps`.
    TItemVectorInt& getSteps() { return *_steps; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTangentActuatorSteps>(*this, other);
    }

private:
    TItemVectorInt::Ptr _steps = TItemVectorInt::create("steps", 6, &_map);
};

/// &&& doc
class TItemForceErrorTangent : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemForceErrorTangent>;

    TItemForceErrorTangent() : TelemetryItem("forceErrorTangent") {}

    virtual ~TItemForceErrorTangent() = default;

    /// Return pointer to `_force`.
    TItemVectorDouble& getForce() { return *_force; }

    /// Return pointer to `_weight`.
    TItemDouble& getWeight() { return *_weight; }

    /// Return pointer to `_sum`.
    TItemDouble& getSum() { return *_sum; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemForceErrorTangent>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _force = TItemVectorDouble::create("force", 6, &_map);
    TItemDouble::Ptr _weight = TItemDouble::create("weight", &_map);
    TItemDouble::Ptr _sum = TItemDouble::create("sum", &_map);
};

/// &&& doc
class TItemInclinometerAngleTma : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemInclinometerAngleTma>;

    TItemInclinometerAngleTma() : TelemetryItem("inclinometerAngleTma") {}

    virtual ~TItemInclinometerAngleTma() = default;

    /// Return pointer to `_inclinometer`.
    TItemDouble& getInclinometer() { return *_inclinometer; }


    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemInclinometerAngleTma>(*this, other);
    }

private:
    TItemDouble::Ptr _inclinometer = TItemDouble::create("inclinometer", &_map);
};

/// &&& doc
class TItemM2AssemblyInPosition : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemM2AssemblyInPosition>;

    TItemM2AssemblyInPosition() : TelemetryItem("m2AssemblyInPosition") {}

    virtual ~TItemM2AssemblyInPosition() = default;

    /// Return pointer to `_inPosition`.
    TItemBoolean& getInPosition() { return *_inPosition; }


    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemM2AssemblyInPosition>(*this, other);
    }

private:
    TItemBoolean::Ptr _inPosition = TItemBoolean::create("inPosition", &_map);
};


/// &&& still need displacementSensors ilcData netForcesTotal netMomentsTotal axialForce

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
