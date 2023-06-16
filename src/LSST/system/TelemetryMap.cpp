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
#include "system/TelemetryMap.h"

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

bool TelemetryMap::compareTelemetryItemMaps(TelemetryItemMap const& mapA, TelemetryItemMap const& mapB,
                                            string const& note) {
    if (mapA.size() != mapB.size()) {
        LWARN(note, "::compare sizes different mapA=", mapA.size(), " mapB=", mapB.size());
        return false;
    }
    for (auto const& elem : mapA) {
        TelemetryItem::Ptr ptrA = elem.second;
        string itemId = ptrA->getId();

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
    return compareTelemetryItemMaps(_map, other._map);
}

bool TelemetryMap::setItemFromJsonStr(string const& jsStr) {
    try {
        LDEBUG("&&& TelemetryMap::setItemFromJsonStr parse str=", jsStr);
        json js = json::parse(jsStr);
        LDEBUG("&&& TelemetryMap::setItemFromJsonStr parse js=", js);
        return setItemFromJson(js);
    } catch (json::parse_error const& ex) {
        LERROR("TelemetryMap::setItemFromJsonStr json parse error msg=", ex.what());
        return false;
    }
}

bool TelemetryMap::setItemFromJson(nlohmann::json const& js) {
    string id = js["id"];
    auto iter = _map.find(id);
    if (iter == _map.end()) {
        LERROR("TelemetryMap::setItemFromJson did not find ", js);
        return false;
    }
    bool idExpected = true;
    LDEBUG("&&& TelemetryMap::setItemFromJson idExpected=", idExpected, " js=", js);
    return iter->second->setFromJson(js, idExpected);
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
