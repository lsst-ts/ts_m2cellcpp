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
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Third party headers
#include "nlohmann/json.hpp"

// project headers

namespace LSST {
namespace m2cellcpp {
namespace system {

class TelemetryItem;

typedef std::map<std::string, std::shared_ptr<TelemetryItem>> TelemetryItemMap;

/// &&& doc
class TelemetryItem {
public:
    using Ptr = std::shared_ptr<TelemetryItem>;

    /// Return true if `item` was successfully inserted into `tiMap`.
    static bool insert(TelemetryItemMap& tiMap, Ptr const& item) {
        auto ret = tiMap.insert(std::make_pair(item->getId(), item));
        bool insertSuccess = ret.second;
        return insertSuccess;
    }

    TelemetryItem() = delete;
    TelemetryItem(std::string const& id) : _id(id) {}
    virtual ~TelemetryItem() = default;

    TelemetryItem(TelemetryItem const&) = delete;
    TelemetryItem& operator=(TelemetryItem const&) = delete;

    /// Return the id string
    std::string getId() const { return _id; };

    /// Return a json object containing the id.
    virtual nlohmann::json getJson() const = 0;

    /// Parse the json string `jStr` and load the appropriate values into this object.
    /// @return true if item parsed correctly and had appropriate data.
    bool parse(std::string const& jStr);

    /// doc&&&
    virtual bool setFromJson(nlohmann::json& js, bool idExpected = false) = 0;

protected:
    /// Build a json object using this object and the provided `tMap`.
    nlohmann::json buildJsonFromMap(TelemetryItemMap const& tMap) const;

    /// Return false if values in `tMap` cannot be set from `js` and `idExpected`.
    bool setMapFromJson(TelemetryItemMap& tMap, nlohmann::json& js, bool idExpected);

    /// Return false if `idExpected` and the `js` "id" entry is wrong.
    bool checkIdCorrect(nlohmann::json& js, bool idExpected) const;

private:
    std::string const _id;
};

/// &&& doc
class TItemDouble : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemDouble>;

    /// &&& doc
    /// @throws util::Bug if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, TelemetryItemMap* itMap, double defaultVal = 0.0);
    TItemDouble() = delete;

    /// &&& doc
    TItemDouble(std::string const& id, double defaultVal = 0.0) : TelemetryItem(id), _val(defaultVal) {}

    //// &&& doc
    void setVal(double val) { _val = val; }

    //// &&& doc
    double getVal() const { return _val; }

    /// Local override of getJson
    nlohmann::json getJson() const override;

    /// doc&&&
    bool setFromJson(nlohmann::json& js, bool idExpected) override;

private:
    std::atomic<double> _val;
};

/// common elements between powerStatus and powerStatusRaw
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

    /// doc&&&
    bool setFromJson(nlohmann::json& js, bool idExpected) override {
        return setMapFromJson(_map, js, idExpected);
    }

private:
    /// Map of items for this TelemetryItem. Does not change after constructor.
    TelemetryItemMap _map;
    TItemDouble::Ptr _motorVoltage = TItemDouble::create("motorVoltage", &_map);
    TItemDouble::Ptr _motorCurrent = TItemDouble::create("motorCurrent", &_map);
    TItemDouble::Ptr _commVoltage = TItemDouble::create("commVoltage", &_map);
    TItemDouble::Ptr _commCurrent = TItemDouble::create("commCurrent", &_map);
};

/// &&& doc
class TItemPowerStatus : public TItemPowerStatusBase {
public:
    using Ptr = std::shared_ptr<TItemPowerStatus>;

    TItemPowerStatus() : TItemPowerStatusBase("powerStatus") {}
    virtual ~TItemPowerStatus() = default;
};

/// &&& doc
class TItemPowerStatusRaw : public TItemPowerStatusBase {
public:
    using Ptr = std::shared_ptr<TItemPowerStatusRaw>;

    TItemPowerStatusRaw() : TItemPowerStatusBase("powerStatusRaw") {}
    virtual ~TItemPowerStatusRaw() = default;
};

/// &&& doc
class TelemetryMap {  ///&&& not in use yet
public:
    using Ptr = std::shared_ptr<TelemetryMap>;
    TelemetryMap() = default;
    TelemetryMap(TelemetryMap const&) = delete;
    TelemetryMap& operator=(TelemetryMap const&) = delete;
    ~TelemetryMap() = default;

private:
    TelemetryItemMap _map;  ///< &&& doc

    /// &&& doc
    template <typename T>
    std::shared_ptr<T> _addItem() {
        std::shared_ptr<T> item = std::shared_ptr<T>(new T());
        _map.insert(make_pair(item->getId(), item));
        return item;
    }

    TItemPowerStatus::Ptr _powerStatus = TelemetryMap::_addItem<TItemPowerStatus>();
    TItemPowerStatusRaw::Ptr _powerStatusRaw = TelemetryMap::_addItem<TItemPowerStatusRaw>();
    //&&& here -> in TelemetryCom have the server write these, and the client read them. Then verify
    //&&& the values match.
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
