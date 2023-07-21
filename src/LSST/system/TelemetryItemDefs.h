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

    /// Return reference to `_motorVoltage`, unit: volt.
    TItemDouble& getMotorVoltage() { return *_motorVoltage; }

    /// Return reference to `_motorCurrent`, unit: ampere.
    TItemDouble& getMotorCurrent() { return *_motorCurrent; }

    /// Return reference to `_commVoltage`, unit: volt.
    TItemDouble& getCommVoltage() { return *_commVoltage; }

    /// Return reference to `_commCurrent`, unit: ampere.
    TItemDouble& getCommCurrent() { return *_commCurrent; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemPowerStatusBase>(*this, other);
    }

private:
    TItemDouble::Ptr _motorVoltage = TItemDouble::create("motorVoltage", &_tiMap);
    TItemDouble::Ptr _motorCurrent = TItemDouble::create("motorCurrent", &_tiMap);
    TItemDouble::Ptr _commVoltage = TItemDouble::create("commVoltage", &_tiMap);
    TItemDouble::Ptr _commCurrent = TItemDouble::create("commCurrent", &_tiMap);
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



/// TelemetryItem child class for storing the "tangentForce" values.
/// Unit tests in tests/test_TelemetryCom
class TItemTangentForce : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTangentForce>;

    TItemTangentForce() : TelemetryItem("tangentForce") {}

    ~TItemTangentForce() override {};

    /// Return reference to `_lutGravity`, unit: newton.
    TItemVectorDouble& getLutGravity() const { return *_lutGravity; }

    /// Return reference to `_lutTemperature`, units: newton.
    TItemVectorDouble& getLutTemperature() const { return *_lutTemperature; }

    /// Return reference to `_applied`, unit: newton.
    TItemVectorDouble& getApplied() const { return *_applied; }

    /// Return reference to `_measured`, unit: newton.
    TItemVectorDouble& getMeasured() const { return *_measured; }

    /// Return reference to `_hardpointCorrection`, unit: newton.
    TItemVectorDouble& getHardpointCorrection() const { return *_hardpointCorrection; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTangentForce>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _lutGravity = TItemVectorDouble::create("lutGravity", 6, &_tiMap);
    TItemVectorDouble::Ptr _lutTemperature = TItemVectorDouble::create("lutTemperature", 6, &_tiMap);
    TItemVectorDouble::Ptr _applied = TItemVectorDouble::create("applied", 6, &_tiMap);
    TItemVectorDouble::Ptr _measured = TItemVectorDouble::create("measured", 6, &_tiMap);
    TItemVectorDouble::Ptr _hardpointCorrection = TItemVectorDouble::create("hardpointCorrection", 6, &_tiMap);
};

/// TelemetryItem child class for storing the "forceBalance" values.
/// Unit tests in tests/test_TelemetryCom
class TItemForceBalance : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemForceBalance>;

    TItemForceBalance() : TelemetryItem("forceBalance") {}

    virtual ~TItemForceBalance() = default;

    /// Return reference to `_fx`, unit: newton.
    TItemDouble& getFx() { return *_fx; }

    /// Return reference to `_fy`, unit: newton.
    TItemDouble& getFy() { return *_fy; }

    /// Return reference to `_fz`, unit: newton.
    TItemDouble& getFz() { return *_fz; }

    /// Return reference to `_mx`, unit: newton*meter.
    TItemDouble& getMx() { return *_mx; }

    /// Return reference to `_my`, unit: newton*meter.
    TItemDouble& getMy() { return *_my; }

    /// Return reference to `_mz`, unit: newton*meter.
    TItemDouble& getMz() { return *_mz; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemForceBalance>(*this, other);
    }

private:
    TItemDouble::Ptr _fx = TItemDouble::create("fx", &_tiMap);
    TItemDouble::Ptr _fy = TItemDouble::create("fy", &_tiMap);
    TItemDouble::Ptr _fz = TItemDouble::create("fz", &_tiMap);
    TItemDouble::Ptr _mx = TItemDouble::create("mx", &_tiMap);
    TItemDouble::Ptr _my = TItemDouble::create("my", &_tiMap);
    TItemDouble::Ptr _mz = TItemDouble::create("mz", &_tiMap);
};


/// Common elements between position message ids.
/// Unit tests in tests/test_TelemetryCom
class TItemPositionBase : public TelemetryItem {
public:
    TItemPositionBase(std::string const& id) : TelemetryItem(id) {}

    virtual ~TItemPositionBase() = default;

    /// Return reference to `_x`, unit: micron.
    TItemDouble& getX() { return *_x; }

    /// Return reference to `_y`, unit: micron.
    TItemDouble& getY() { return *_y; }

    /// Return reference to `_z`, unit: micron.
    TItemDouble& getZ() { return *_z; }

    /// Return reference to `_xRot`, unit: arcsec.
    TItemDouble& getXRot() { return *_xRot; }

    /// Return reference to `_y`, unit: arcsec.
    TItemDouble& getYRot() { return *_yRot; }

    /// Return reference to `_z`, unit: arcsec.
    TItemDouble& getZRot() { return *_zRot; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemPositionBase>(*this, other);
    }

private:
    TItemDouble::Ptr _x = TItemDouble::create("x", &_tiMap);
    TItemDouble::Ptr _y = TItemDouble::create("y", &_tiMap);
    TItemDouble::Ptr _z = TItemDouble::create("z", &_tiMap);
    TItemDouble::Ptr _xRot = TItemDouble::create("xRot", &_tiMap);
    TItemDouble::Ptr _yRot = TItemDouble::create("yRot", &_tiMap);
    TItemDouble::Ptr _zRot = TItemDouble::create("zRot", &_tiMap);
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

/// TelemetryItem child class for storing the "temperature" values.
/// Unit tests in tests/test_TelemetryCom
class TItemTemperature : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTemperature>;

    TItemTemperature() : TelemetryItem("temperature") {}

    virtual ~TItemTemperature() = default;

    /// Return reference to `_ring`, unit: deg_C.
    TItemVectorDouble& getRing() { return *_ring; }

    /// Return reference to `_intake`, unit: deg_C.
    TItemVectorDouble& getIntake() { return *_intake; }

    /// Return reference to `_exhaust`, unit: deg_C.
    TItemVectorDouble& getExhaust() { return *_exhaust; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTemperature>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _ring = TItemVectorDouble::create("ring", 12, &_tiMap);
    TItemVectorDouble::Ptr _intake = TItemVectorDouble::create("intake", 2, &_tiMap);
    TItemVectorDouble::Ptr _exhaust = TItemVectorDouble::create("exhaust", 2, &_tiMap);
};

/// TelemetryItem child class for storing the "zenithAngle" values.
/// Unit tests in tests/test_TelemetryCom
class TItemZenithAngle : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemZenithAngle>;

    TItemZenithAngle() : TelemetryItem("zenithAngle") {}

    virtual ~TItemZenithAngle() = default;

    /// Return reference to `_measured`, unit: degree.
    TItemDouble& getMeasured() { return *_measured; }

    /// Return reference to `_inclinometerRaw`, unit: degree.
    TItemDouble& getInclinometerRaw() { return *_inclinometerRaw; }

    /// Return reference to `_inclinometerProcessed`, unit: degree.
    TItemDouble& getInclinometerProcessed() { return *_inclinometerProcessed; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemZenithAngle>(*this, other);
    }

private:
    TItemDouble::Ptr _measured = TItemDouble::create("measured", &_tiMap);
    TItemDouble::Ptr _inclinometerRaw = TItemDouble::create("inclinometerRaw", &_tiMap);
    TItemDouble::Ptr _inclinometerProcessed = TItemDouble::create("inclinometerProcessed", &_tiMap);
};

/// TelemetryItem child class for storing the "axialEncoderPositions" values.
/// Unit tests in tests/test_TelemetryCom
class TItemAxialEncoderPositions : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemAxialEncoderPositions>;

    TItemAxialEncoderPositions() : TelemetryItem("axialEncoderPositions") {}

    virtual ~TItemAxialEncoderPositions() = default;

    /// Return reference to `_position`, unit: micron.
    TItemVectorDouble& getPosition() { return *_position; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemAxialEncoderPositions>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _position = TItemVectorDouble::create("position", 72, &_tiMap);
};

/// TelemetryItem child class for storing the "tangentEncoderPositions" values.
/// Unit tests in tests/test_TelemetryCom
class TItemTangentEncoderPositions : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTangentEncoderPositions>;

    TItemTangentEncoderPositions() : TelemetryItem("tangentEncoderPositions") {}

    virtual ~TItemTangentEncoderPositions() = default;

    /// Return reference to `_position`, unit: micron.
    TItemVectorDouble& getPosition() { return *_position; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTangentEncoderPositions>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _position = TItemVectorDouble::create("position", 6, &_tiMap);
};

/// TelemetryItem child class for storing the "axialActuatorSteps" values.
/// Unit tests in tests/test_TelemetryCom
class TItemAxialActuatorSteps : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemAxialActuatorSteps>;

    TItemAxialActuatorSteps() : TelemetryItem("axialActuatorSteps") {}

    virtual ~TItemAxialActuatorSteps() = default;

    /// Return reference to `_steps`, unit: unitless.
    TItemVectorInt& getSteps() { return *_steps; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemAxialActuatorSteps>(*this, other);
    }

private:
    TItemVectorInt::Ptr _steps = TItemVectorInt::create("steps", 72, &_tiMap);
};

/// TelemetryItem child class for storing the "tangentActuatorSteps" values.
/// Unit tests in tests/test_TelemetryCom
class TItemTangentActuatorSteps : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTangentActuatorSteps>;

    TItemTangentActuatorSteps() : TelemetryItem("tangentActuatorSteps") {}

    virtual ~TItemTangentActuatorSteps() = default;

    /// Return reference to `_steps`, unit: unitless.
    TItemVectorInt& getSteps() { return *_steps; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTangentActuatorSteps>(*this, other);
    }

private:
    TItemVectorInt::Ptr _steps = TItemVectorInt::create("steps", 6, &_tiMap);
};

/// TelemetryItem child class for storing the "forceErrorTangent" values.
/// Unit tests in tests/test_TelemetryCom
class TItemForceErrorTangent : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemForceErrorTangent>;

    TItemForceErrorTangent() : TelemetryItem("forceErrorTangent") {}

    virtual ~TItemForceErrorTangent() = default;

    /// Return reference to `_force`, unit: newton.
    TItemVectorDouble& getForce() { return *_force; }

    /// Return reference to `_weight`, unit: kilo.
    TItemDouble& getWeight() { return *_weight; }

    /// Return reference to `_sum`, unit: newton.
    TItemDouble& getSum() { return *_sum; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemForceErrorTangent>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _force = TItemVectorDouble::create("force", 6, &_tiMap);
    TItemDouble::Ptr _weight = TItemDouble::create("weight", &_tiMap);
    TItemDouble::Ptr _sum = TItemDouble::create("sum", &_tiMap);
};

/// TelemetryItem child class for storing the "inclinometerAngleTma" values.
/// Unit tests in tests/test_TelemetryCom
class TItemInclinometerAngleTma : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemInclinometerAngleTma>;

    TItemInclinometerAngleTma() : TelemetryItem("inclinometerAngleTma") {}

    virtual ~TItemInclinometerAngleTma() = default;

    /// Return reference to `_inclinometer`, units degree.
    TItemDouble& getInclinometer() { return *_inclinometer; }


    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemInclinometerAngleTma>(*this, other);
    }

private:
    TItemDouble::Ptr _inclinometer = TItemDouble::create("inclinometer", &_tiMap);
};


/// TelemetryItem child class for storing the "displacementSensors" values.
/// Unit tests in tests/test_TelemetryCom
class TItemDisplacementSensors : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemDisplacementSensors>;

    TItemDisplacementSensors() : TelemetryItem("displacementSensors") {}

    virtual ~TItemDisplacementSensors() = default;

    /// Return reference to `_thetaZ`, unit: micron.
    TItemVectorDouble& getThetaZ() { return *_thetaZ; }

    /// Return reference to `_deltaZ`, unit: micron.
    TItemVectorDouble& getDeltaZ() { return *_deltaZ; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemDisplacementSensors>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _thetaZ = TItemVectorDouble::create("thetaZ", 6, &_tiMap);
    TItemVectorDouble::Ptr _deltaZ = TItemVectorDouble::create("deltaZ", 6, &_tiMap);
};


/// TelemetryItem child class for storing the "ilcData" values.
/// Unit tests in tests/test_TelemetryCom
class TItemIlcData : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemIlcData>;

    TItemIlcData() : TelemetryItem("ilcData") {}

    virtual ~TItemIlcData() = default;

    /// Return reference to `_status`, unit: unitless.
    TItemVectorDouble& getStatus() { return *_status; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemIlcData>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _status = TItemVectorDouble::create("status", 78, &_tiMap);
};

/// TelemetryItem child class for storing the "netForcesTotal" values.
/// Unit tests in tests/test_TelemetryCom
class TItemNetForcesTotal : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemNetForcesTotal>;

    TItemNetForcesTotal() : TelemetryItem("netForcesTotal") {}

    virtual ~TItemNetForcesTotal() = default;

    /// Return reference to `_fx`, unit: newton.
    TItemDouble& getFx() { return *_fx; }

    /// Return reference to `_fy`, unit: newton.
    TItemDouble& getFy() { return *_fy; }

    /// Return reference to `_fz`, unit: newton.
    TItemDouble& getFz() { return *_fz; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemNetForcesTotal>(*this, other);
    }

private:
    TItemDouble::Ptr _fx = TItemDouble::create("fx", &_tiMap);
    TItemDouble::Ptr _fy = TItemDouble::create("fy", &_tiMap);
    TItemDouble::Ptr _fz = TItemDouble::create("fz", &_tiMap);
};

/// TelemetryItem child class for storing the "netMomentsTotal" values.
/// Unit tests in tests/test_TelemetryCom
class TItemNetMomentsTotal : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemNetMomentsTotal>;

    TItemNetMomentsTotal() : TelemetryItem("netMomentsTotal") {}

    virtual ~TItemNetMomentsTotal() = default;

    /// Return reference to `_mx`, unit: newton*meter.
    TItemDouble& getMx() { return *_mx; }

    /// Return reference to `_my`, unit: newton*meter.
    TItemDouble& getMy() { return *_my; }

    /// Return reference to `_mz`, unit: newton*meter.
    TItemDouble& getMz() { return *_mz; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemNetMomentsTotal>(*this, other);
    }

private:
    TItemDouble::Ptr _mx = TItemDouble::create("mx", &_tiMap);
    TItemDouble::Ptr _my = TItemDouble::create("my", &_tiMap);
    TItemDouble::Ptr _mz = TItemDouble::create("mz", &_tiMap);
};

/// TelemetryItem child class for storing the "axialForce" values.
/// Unit tests in tests/test_TelemetryCom
class TItemAxialForce : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemAxialForce>;

    TItemAxialForce() : TelemetryItem("axialForce") {}

    virtual ~TItemAxialForce() = default;

    /// Return reference to `_lutGravity`, unit: newton.
    TItemVectorDouble& getLutGravity() { return *_lutGravity; }

    /// Return reference to `_lutTemperature`, unit: newton.
    TItemVectorDouble& getLutTemperature() { return *_lutTemperature; }

    /// Return reference to `_applied`, unit: newton.
    TItemVectorDouble& getApplied() { return *_applied; }

    /// Return reference to `_measured`, unit: newton.
    TItemVectorDouble& getMeasured() { return *_measured; }

    /// Return reference to `_hardpointCorrection`, unit: newton.
    TItemVectorDouble& getHardpointCorrection() { return *_hardpointCorrection; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemAxialForce>(*this, other);
    }

private:
    TItemVectorDouble::Ptr _lutGravity = TItemVectorDouble::create("lutGravity", 72, &_tiMap);
    TItemVectorDouble::Ptr _lutTemperature = TItemVectorDouble::create("lutTemperature", 72, &_tiMap);
    TItemVectorDouble::Ptr _applied = TItemVectorDouble::create("applied", 72, &_tiMap);
    TItemVectorDouble::Ptr _measured = TItemVectorDouble::create("measured", 72, &_tiMap);
    TItemVectorDouble::Ptr _hardpointCorrection = TItemVectorDouble::create("hardpointCorrection", 72, &_tiMap);
};

/// This class is used to accept the telescope elevation angle from a client.
class TItemTelElevation : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemTelElevation>;

    TItemTelElevation() : TelemetryItem("tel_elevation") {}

    virtual ~TItemTelElevation() = default;

    /// Return reference to `actualPosition`, unit: degree.
    TItemDouble& getActualPosition() { return *_actualPosition; }

    /// Return reference to `compName`, unit: unitless.
    /// Expected values: "MTMount"
    TItemString& getCompName() { return *_compName; }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        return compareItemsTemplate<TItemTelElevation>(*this, other);
    }

private:
    TItemDouble::Ptr _actualPosition = TItemDouble::create("actualPosition", &_tiMap);
    TItemString::Ptr _compName = TItemString::create("compName", &_tiMap);
};
}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEMDEFS_H
