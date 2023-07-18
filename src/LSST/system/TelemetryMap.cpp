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

bool TelemetryMap::compareMaps(TelemetryMap const& other) {
    return TelemetryItem::compareTelemetryItemMaps(_map, other._map);
}

TelemetryItem::Ptr TelemetryMap::setItemFromJsonStr(string const& jsStr) {
    try {
        json js = json::parse(jsStr);
        return setItemFromJson(js);
    } catch (json::parse_error const& ex) {
        LERROR("TelemetryMap::setItemFromJsonStr json parse error msg=", ex.what());
        return nullptr;
    }
}

TelemetryItem::Ptr TelemetryMap::setItemFromJson(nlohmann::json const& js) {
    string id = js["id"];
    auto iter = _map.find(id);
    if (iter == _map.end()) {
        LERROR("TelemetryMap::setItemFromJson did not find ", js);
        return nullptr;
    }
    bool idExpected = true;
    LTRACE("TelemetryMap::setItemFromJson idExpected=", idExpected, " js=", js);
    // If set successfully, return a pointer to the set element, otherwise return nullptr.
    TelemetryItem::Ptr retPtr = (iter->second->setFromJson(js, idExpected)) ? (iter->second) : nullptr;
    return retPtr;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
