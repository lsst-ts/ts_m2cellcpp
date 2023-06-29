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
#ifndef LSST_M2CELLCPP_SYSTEM_TELEMETRYITEMDEFS_H
#define LSST_M2CELLCPP_SYSTEM_TELEMETRYITEMDEFS_H

// System headers

// Third party headers
#include "nlohmann/json.hpp"

// project headers
#include "system/TelemetryItem.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

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

    /// Return reference to `_ring`.
    TItemVectorDouble& getRing() { return *_ring; }

    /// Return reference to `_intake`.
    TItemVectorDouble& getIntake() { return *_intake; }

    /// Return reference to `_exhaust`.
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

    /// Return reference to `_measured`.
    TItemDouble& getMeasured() { return *_measured; }

    /// Return reference to `_inclinometerRaw`.
    TItemDouble& getInclinometerRaw() { return *_inclinometerRaw; }

    /// Return reference to `_inclinometerProcessed`.
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

    /// Return reference to `_position`.
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

    /// Return reference to `_position`.
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

    /// Return reference to `_steps`.
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

    /// Return reference to `_steps`.
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

    /// Return reference to `_force`.
    TItemVectorDouble& getForce() { return *_force; }

    /// Return reference to `_weight`.
    TItemDouble& getWeight() { return *_weight; }

    /// Return reference to `_sum`.
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

    /// Return reference to `_inclinometer`.
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

    /// Return reference to `_inPosition`.
    TItemBoolean& getInPosition() { return *_inPosition; }


    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemM2AssemblyInPosition>(*this, other);
    }

private:
    TItemBoolean::Ptr _inPosition = TItemBoolean::create("inPosition", &_map);
};

/// &&& doc
class TItemDisplacementSensors : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemDisplacementSensors>;

    TItemDisplacementSensors() : TelemetryItem("displacementSensors") {}

    virtual ~TItemDisplacementSensors() = default;

    /// Return reference to `_thetaZ`.
    TItemVectorDouble& getThetaZ() { return *_thetaZ; }

    /// Return reference to `_deltaZ`.
    TItemVectorDouble& getDeltaZ() { return *_deltaZ; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemDisplacementSensors>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _thetaZ = TItemVectorDouble::create("thetaZ", 6, &_map);
    TItemVectorDouble::Ptr _deltaZ = TItemVectorDouble::create("deltaZ", 6, &_map);
};


/// &&& doc
class TItemIlcData : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemIlcData>;

    TItemIlcData() : TelemetryItem("ilcData") {}

    virtual ~TItemIlcData() = default;

    /// Return reference to `_status`.
    TItemVectorDouble& getStatus() { return *_status; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemIlcData>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _status = TItemVectorDouble::create("status", 78, &_map);
};

/// &&& doc
class TItemNetForcesTotal : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemNetForcesTotal>;

    TItemNetForcesTotal() : TelemetryItem("netForcesTotal") {}

    virtual ~TItemNetForcesTotal() = default;

    /// Return reference to `_fx`.
    TItemDouble& getFx() { return *_fx; }

    /// Return reference to `_fy`.
    TItemDouble& getFy() { return *_fy; }

    /// Return reference to `_fz`.
    TItemDouble& getFz() { return *_fz; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemNetForcesTotal>(*this, other);
    }

private:
    TItemDouble::Ptr _fx = TItemDouble::create("fx", &_map);
    TItemDouble::Ptr _fy = TItemDouble::create("fy", &_map);
    TItemDouble::Ptr _fz = TItemDouble::create("fz", &_map);
};

/// &&& doc
class TItemNetMomentsTotal : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemNetMomentsTotal>;

    TItemNetMomentsTotal() : TelemetryItem("netMomentsTotal") {}

    virtual ~TItemNetMomentsTotal() = default;

    /// Return reference to `_mx`.
    TItemDouble& getMx() { return *_mx; }

    /// Return reference to `_my`.
    TItemDouble& getMy() { return *_my; }

    /// Return reference to `_mz`.
    TItemDouble& getMz() { return *_mz; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemNetMomentsTotal>(*this, other);
    }

private:
    TItemDouble::Ptr _mx = TItemDouble::create("mx", &_map);
    TItemDouble::Ptr _my = TItemDouble::create("my", &_map);
    TItemDouble::Ptr _mz = TItemDouble::create("mz", &_map);
};

/// &&& doc
class TItemAxialForce : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemAxialForce>;

    TItemAxialForce() : TelemetryItem("axialForce") {}

    virtual ~TItemAxialForce() = default;

    /// Return reference to `_lutGravity`.
    TItemVectorDouble& getLutGravity() { return *_lutGravity; }

    /// Return reference to `_lutTemperature`.
    TItemVectorDouble& getLutTemperature() { return *_lutTemperature; }

    /// Return reference to `_applied`.
    TItemVectorDouble& getApplied() { return *_applied; }

    /// Return reference to `_measured`.
    TItemVectorDouble& getMeasured() { return *_measured; }

    /// Return reference to `_hardpointCorrection`.
    TItemVectorDouble& getHardpointCorrection() { return *_hardpointCorrection; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemAxialForce>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _lutGravity = TItemVectorDouble::create("lutGravity", 72, &_map);
    TItemVectorDouble::Ptr _lutTemperature = TItemVectorDouble::create("lutTemperature", 72, &_map);
    TItemVectorDouble::Ptr _applied = TItemVectorDouble::create("applied", 72, &_map);
    TItemVectorDouble::Ptr _measured = TItemVectorDouble::create("measured", 72, &_map);
    TItemVectorDouble::Ptr _hardpointCorrection = TItemVectorDouble::create("hardpointCorrection", 72, &_map);
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEMDEFS_H
