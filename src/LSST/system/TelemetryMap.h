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
#ifndef LSST_M2CELLCPP_SYSTEM_TELEMETRYMAP_H
#define LSST_M2CELLCPP_SYSTEM_TELEMETRYMAP_H

// System headers

// Third party headers

// project headers
#include "system/TelemetryItemDefs.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// This class stores all the items that need to be sent through `TelemetryCom`.
/// Unit tests in tests/test_TelemetryCom
class TelemetryMap {
public:
    using Ptr = std::shared_ptr<TelemetryMap>;
    TelemetryMap() = default;
    TelemetryMap(TelemetryMap const& other) { _map = other.copyMap(); }
    TelemetryMap& operator=(TelemetryMap const&) = delete;
    ~TelemetryMap() = default;

    /// Return a copy of the internal `_map` object.
    /// It's a map of pointers, so both maps represent the same items.
    TelemetryItemMap copyMap() const {
        TelemetryItemMap newMap = _map;
        return newMap;
    }

    /// Locate and set the single item represented by `jsStr`.
    /// @return true if the item was found and set.
    bool setItemFromJsonStr(std::string const& jsStr);

    /// Locate and set the single item represented by `js`.
    /// @return true if the item was found and set.
    bool setItemFromJson(nlohmann::json const& js);

    /// Return true if all items in this map match and equall all items in `other`.
    bool compareMaps(TelemetryMap const& other);

    /// Return a pointer to `_powerStatus`.
    TItemPowerStatus::Ptr getPowerStatus() const { return _powerStatus; }

    /// Return a pointer to `_powerStatusRaw`.
    TItemPowerStatusRaw::Ptr getPowerStatusRaw() const { return _powerStatusRaw; }

    /// Return a pointer to `_tangentForce`.
    TItemTangentForce::Ptr getTangentForce() const { return _tangentForce; }

    /// Return a pointer to `_forceBalance`.
    TItemForceBalance::Ptr getForceBalance() const {return _forceBalance; }

    /// Return a pointer to `_position`.
    TItemPosition::Ptr getPosition() const {return _position; }

    /// Return a pointer to `_positionIMS`.
    TItemPositionIMS::Ptr getPositionIMS() const {return _positionIMS; }

    /// Return a pointer to `_temperature`.
    TItemTemperature::Ptr getTemperature() const {return _temperature; }

    /// Return a pointer to `_zenithAngle`.
    TItemZenithAngle::Ptr getZenithAngle() const {return _zenithAngle; }

    /// Return a pointer to `_axialEncoderPositions`.
    TItemAxialEncoderPositions::Ptr getAxialEncoderPositions() const {return _axialEncoderPositions; }

    /// Return a pointer to `_tangentEncoderPositions`.
    TItemTangentEncoderPositions::Ptr getTangentEncoderPositions() const {return _tangentEncoderPositions; }

    /// Return a pointer to `_axialActuatorSteps`.
    TItemAxialActuatorSteps::Ptr getAxialActuatorSteps() const {return _axialActuatorSteps; }

    /// Return a pointer to `_tangentActuatorSteps`.
    TItemTangentActuatorSteps::Ptr getTangentActuatorSteps() const {return _tangentActuatorSteps; }

    /// Return a pointer to `_forceErrorTangent`.
    TItemForceErrorTangent::Ptr getForceErrorTangent() const {return _forceErrorTangent; }

    /// Return a pointer to `_inclinometerAngleTma`.
    TItemInclinometerAngleTma::Ptr getInclinometerAngleTma() const {return _inclinometerAngleTma; }

    /// Return a pointer to `_displacementSensors`.
    TItemDisplacementSensors::Ptr getDisplacementSensors() const {return _displacementSensors; }

    /// Return a pointer to `_ilcData`.
    TItemIlcData::Ptr getIlcData() const {return _ilcData; }

    /// Return a pointer to `_netForcesTotal`.
    TItemNetForcesTotal::Ptr getNetForcesTotal() const {return _netForcesTotal; }

    /// Return a pointer to `_netMomentsTotal`.
    TItemNetMomentsTotal::Ptr getNetMomentsTotal() const {return _netMomentsTotal; }

    /// Return a pointer to `_axialForce`.
    TItemAxialForce::Ptr getAxialForce() const {return _axialForce; }

private:
    /// Map of all telemetry items to be sent to clients.
    /// Once created, the items contained in the map will not change, but their values will.
    /// This allows the map to be accessed without a mutex, but the item value setting
    /// and reading do need to be threadsafe.
    TelemetryItemMap _map;

    /// Create an object ot type `T` derived from `TelemetryItem` and add it to `_map`.
    template <typename T>
    std::shared_ptr<T> _addItem() {
        std::shared_ptr<T> item = std::shared_ptr<T>(new T());
        _map.insert(make_pair(item->getId(), item));
        return item;
    }

    TItemPowerStatus::Ptr _powerStatus = TelemetryMap::_addItem<TItemPowerStatus>();  ///< "powerStatus"
    TItemPowerStatusRaw::Ptr _powerStatusRaw =
            TelemetryMap::_addItem<TItemPowerStatusRaw>();  ///< "powerStatusRaw"
    TItemTangentForce::Ptr _tangentForce = _addItem<TItemTangentForce>(); ///< "tangentForce"
    TItemForceBalance::Ptr _forceBalance = _addItem<TItemForceBalance>(); ///< "forceBalance"
    TItemPosition::Ptr _position = _addItem<TItemPosition>(); ///< "position"
    TItemPositionIMS::Ptr _positionIMS = _addItem<TItemPositionIMS>(); ///< "positionIMS"
    TItemTemperature::Ptr _temperature = _addItem<TItemTemperature>(); ///< "temperature"
    TItemZenithAngle::Ptr _zenithAngle = _addItem<TItemZenithAngle>(); ///< "zenithAngle"
    /// "axialEncoderPositions"
    TItemAxialEncoderPositions::Ptr _axialEncoderPositions = _addItem<TItemAxialEncoderPositions>();
     /// "tangentEncoderPositions"
    TItemTangentEncoderPositions::Ptr _tangentEncoderPositions = _addItem<TItemTangentEncoderPositions>();
    TItemAxialActuatorSteps::Ptr _axialActuatorSteps = _addItem<TItemAxialActuatorSteps>(); ///< "axialActuatorSteps"
    /// "tangentActuatorSteps"
    TItemTangentActuatorSteps::Ptr _tangentActuatorSteps = _addItem<TItemTangentActuatorSteps>();
    TItemForceErrorTangent::Ptr _forceErrorTangent = _addItem<TItemForceErrorTangent>(); ///< "forceErrorTangent"
    TItemInclinometerAngleTma::Ptr _inclinometerAngleTma = _addItem<TItemInclinometerAngleTma>(); ///< "inclinometerAngleTma"
    TItemDisplacementSensors::Ptr _displacementSensors = _addItem<TItemDisplacementSensors>(); ///< "displacementSensors"
    TItemIlcData::Ptr _ilcData = _addItem<TItemIlcData>(); ///< "ilcData"
    TItemNetForcesTotal::Ptr _netForcesTotal = _addItem<TItemNetForcesTotal>(); ///< "netForcesTotal"
    TItemNetMomentsTotal::Ptr _netMomentsTotal = _addItem<TItemNetMomentsTotal>(); ///< "netMomentsTotal"
    TItemAxialForce::Ptr _axialForce = _addItem<TItemAxialForce>(); ///< "axialForce"
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYMAP_H
