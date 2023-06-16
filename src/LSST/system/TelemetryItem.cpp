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

// Class header
#include "system/TelemetryItem.h"

// System headers

// Third party headers

// Project headers
#include "util/Log.h"
#include "system/TelemetryMap.h"

using namespace std;
using json = nlohmann::json;

namespace LSST {
namespace m2cellcpp {
namespace system {

bool TelemetryItem::parse(std::string const& jStr) {
    try {
        json js = json::parse(jStr);
        // Parsing the json messages is odd as they are flat.
        // This means the "id" gets mixed in with the other parmeters and
        // needs to be picked out before processing the data elements.
        return setFromJson(js, true);
    } catch (json::parse_error const& ex) {
        LERROR("json parse error msg=", ex.what());
        return false;
    }
}

bool TelemetryItem::checkIdCorrect(json const& js, bool idExpected) const {
    if (!idExpected) {
        return true;
    }
    bool correct = (js["id"] == getId());
    if (!correct) {
        LERROR("TelemetryItem::isIdCorrect Incorrect id. Expected=", getId(), " got=", js["id"]);
    }
    return correct;
}

json TelemetryItem::buildJsonFromMap(TelemetryItemMap const& tMap) const {
    json js;
    js["id"] = getId();
    for (auto const& elem : tMap) {
        auto const& item = elem.second;
        js.merge_patch(item->getJson());
    }
    return js;
}

bool TelemetryItem::setMapFromJson(TelemetryItemMap& tMap, json const& js, bool idExpected) {
    if (!checkIdCorrect(js, idExpected)) {
        return false;
    }
    bool success = true;
    for (auto&& elem : tMap) {
        TelemetryItem::Ptr item = elem.second;
        LDEBUG("&&& TelemetryItem::setMapFromJson js=", js);
        bool status = item->setFromJson(js);
        if (!status) {
            success = false;
            LERROR("TelemetryItem::setMapFromJson failed to set ", item->getId(), " from=", js);
        }
    }
    return success;
}

std::ostream& operator<<(std::ostream& os, TelemetryItem const& item) {
    os << item.dump();
    return os;
}

TItemDouble::Ptr TItemDouble::create(string const& id, TelemetryItemMap* tiMap, double defaultVal) {
    Ptr newItem = Ptr(new TItemDouble(id, defaultVal));
    if (tiMap != nullptr) {
        if (!insert(*tiMap, newItem)) {
            throw TelemetryException(ERR_LOC, "Failed to insert " + id);
        }
    }
    return newItem;
}

json TItemDouble::getJson() const {
    json js;
    double v = _val;  // conversion from atomic fails
    js[getId()] = v;
    return js;
}

bool TItemDouble::setFromJson(nlohmann::json const& js, bool idExpected) {
    if (idExpected) {
        // This type can only have a floating point value for `_val`.
        LERROR("TItemDouble::setFromJson cannot have a json 'id' entry");
        return false;
    }
    try {
        double val = js.at(getId());
        setVal(val);
        return true;
    } catch (json::out_of_range const& ex) {
        LERROR("TItemDouble::setFromJson out of range for ", getId(), " js=", js);
    }
    return false;
}

bool TItemDouble::compareItem(TelemetryItem const& other) const { //&&& looks like all compare items functions can use the same template
    try {
        TItemDouble const& otherItem = dynamic_cast<TItemDouble const&>(other);
        return (getId() == other.getId() && _val == otherItem._val);
    } catch (std::bad_cast const& ex) {
        return false;
    }
}

bool TItemPowerStatusBase::compareItem(TelemetryItem const& other) const {
    try {
        TItemPowerStatusBase const& otherItem = dynamic_cast<TItemPowerStatusBase const&>(other);
        return TelemetryMap::compareTelemetryItemMaps(_map, otherItem._map);
    } catch (std::bad_cast const& ex) {
        return false;
    }
}

TItemDoubleVectorOld::Ptr TItemDoubleVectorOld::create(string const& id, size_t size, TelemetryItemMap* tiMap, double defaultVal) {
    Ptr newItem = Ptr(new TItemDoubleVectorOld(id, size, defaultVal));
    if (tiMap != nullptr) {
        if (!insert(*tiMap, newItem)) {
            throw TelemetryException(ERR_LOC, "Failed to insert " + id);
        }
    }
    return newItem;
}


json TItemDoubleVectorOld::getJson() const {
    json js;
    json jArray = json::array();

    lock_guard<mutex> lg(_mtx);
    for (size_t j = 0; j < _size; ++j) {
        jArray.push_back(_vals[j]);
    }
    js[getId()] = jArray;
    return js;
}

bool TItemDoubleVectorOld::setVals(vector<double> const& vals) {
    if (vals.size() != _size) {
        LERROR("TItemDoubleVectorOld::setVals wrong size vals.size()=", vals.size(), " for ", dump());
        return false;
    }
    lock_guard<mutex> lg(_mtx);
    for(size_t j=0; j<_size; ++j) {
        _vals[j] = vals[j];
    }
    return true;
}

vector<double> TItemDoubleVectorOld::getVals() const {
    std::vector<double> outVals;
    lock_guard<mutex> lg(_mtx);
    for(auto const& v:_vals) {
        outVals.push_back(v);
    }
    return outVals;
}

bool TItemDoubleVectorOld::setVal(size_t index, double val) {
    if (index > _size) {
        return false;
    }
    lock_guard<mutex> lg(_mtx);
    _vals[index] = val;
    return true;
}

double TItemDoubleVectorOld::getVal(size_t index) const {
    if (index > _size) {
        throw TelemetryException(ERR_LOC, "TItemDoubleVectorOld::getVal out of range for index=" + to_string(index) + " for " + dump());
    }
    lock_guard<mutex> lg(_mtx);
    return _vals[index];
}

bool TItemDoubleVectorOld::setFromJson(nlohmann::json const& js, bool idExpected) {
    if (idExpected) {
        // This type can only have a floating point value array for `_val`.
        LERROR("TItemDoubleVectorOld::setFromJson cannot have a json 'id' entry");
        return false;
    }
    try {
        std::vector<double> vals = js.at(getId());
        string valsStr; // &&& delete
        for(auto const& v:vals) { // &&& delete
            valsStr += to_string(v) + ", "; // &&& delete
        } // &&& delete
        LDEBUG("&&& TItemDoubleVectorOld::setFromJson vals=", valsStr, " js.at=", js.at(getId()));
        return setVals(vals);
    } catch (json::out_of_range const& ex) {
        LERROR("TItemDouble::setFromJson out of range for ", getId(), " js=", js);
    }
    return false;
}

bool TItemDoubleVectorOld::compareItem(TelemetryItem const& other) const {
    try {
        TItemDoubleVectorOld const& otherT = dynamic_cast<TItemDoubleVectorOld const&>(other);
        if (getId() != other.getId() || _size != otherT._size) {
            return false;
        }
        // There's no way to reliably control which gets locked first
        // (both A.compareItem(B) or B.compareItem(A) are possible),
        // so this is needed to lock both without risk of deadlock.
        unique_lock lockThis(_mtx, std::defer_lock);
        unique_lock lockOther(otherT._mtx, std::defer_lock);
        lock(lockThis, lockOther);
        return (_vals == otherT._vals);
    } catch (std::bad_cast const& ex) {
        return false;
    }
}


bool TItemTangentForce::compareItem(TelemetryItem const& other) const {
    try {
        LDEBUG("&&& TItemTangentForce::compareItem this=", dump(), " other=", other.dump());
        TItemTangentForce const& otherItem = dynamic_cast<TItemTangentForce const&>(other);
        return TelemetryMap::compareTelemetryItemMaps(_map, otherItem._map);
    } catch (std::bad_cast const& ex) {
        return false;
    }
}


}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
