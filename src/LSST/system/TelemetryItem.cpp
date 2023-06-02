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
#include "util/Bug.h"
#include "util/Log.h"

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
    // &&& this can be done in the same manner as buildJsonFromMap
    // &&& _map can probably be made a member of TelemetryItem
    if (!checkIdCorrect(js, idExpected)) {
        return false;
    }
    bool success = true;
    for (auto&& elem : tMap) {
        TelemetryItem::Ptr item = elem.second;
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

bool compareTelemetryItemMaps(TelemetryItemMap const& mapA, TelemetryItemMap const& mapB,
                              string const& note = "") {
    if (mapA.size() != mapB.size()) {
        LWARN(note, "::compare sizes different mapA=", mapA.size(), " mapB=", mapB.size());
        return false;
    }
    for (auto const& elem : mapA) {
        TelemetryItem::Ptr ptrA = elem.second;
        string itemId = ptrA->getId();
        LDEBUG("&&& comparing ", itemId);

        auto iterB = mapB.find(itemId);
        if (iterB == mapB.end()) {
            LWARN(note, "::compare mapB did not contain key=", itemId);
            return false;
        }
        TelemetryItem::Ptr ptrB = iterB->second;
        bool match = ptrA->compareItem(*ptrB);
        if (!match) {
            LWARN(note, "::compare no match for ptrA=", ptrA->dump(), " ptrB=", ptrB->dump());
            return false;
        }
    }
    return true;
}

bool TelemetryMap::compareMaps(TelemetryMap const& other) {
    lock_guard<std::mutex> lgThis(_mapMtx);
    lock_guard<std::mutex> lgOther(other._mapMtx);

    return compareTelemetryItemMaps(_map, other._map);
}

TItemDouble::Ptr TItemDouble::create(string const& id, TelemetryItemMap* tiMap, double defaultVal) {
    Ptr newItem = Ptr(new TItemDouble(id, defaultVal));
    if (tiMap != nullptr) {
        if (!insert(*tiMap, newItem)) {
            throw util::Bug(ERR_LOC, "Failed to insert " + id);
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

bool TItemDouble::compareItem(TelemetryItem const& other) const {
    TItemDouble const& otherTItemDouble = dynamic_cast<TItemDouble const&>(other);
    return (getId() == other.getId() && _val == otherTItemDouble._val);
}

bool TItemPowerStatusBase::compareItem(TelemetryItem const& other) const {
    TItemPowerStatusBase const& otherIpsb = dynamic_cast<TItemPowerStatusBase const&>(other);
    return compareTelemetryItemMaps(_map, otherIpsb._map);
    ;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
