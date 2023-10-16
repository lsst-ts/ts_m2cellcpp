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

#ifndef LSST_M2CELLCPP_FAULTMGR_FAULTSTATUSBITS_H
#define LSST_M2CELLCPP_FAULTMGR_FAULTSTATUSBITS_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "control/control_defs.h"

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

/// Class representation of the "Fault Status", which is a bitmap
/// representing the status of several system items. In the LabView
/// code, this class comes from "Faults-Warnings-Info_BitStatus.lvclass"
/// and has an icon with a box and TelemBit in the label.
/// Aside from the static mask functions, this class is not thread safe.
/// "Telemetry Fault Manager", "Fault Manager", and "PowerSubsystems use the
/// same bitmap bits but have different initialization code.
/// Unit tests in test_FpgaIo.cpp.
class FaultStatusBits {
public:
    using Ptr = std::shared_ptr<FaultStatusBits>;

    /// Create a FaultStatusMap with a blank _bitmap
    FaultStatusBits() = default;

    /// Create a FaultStatusMap with a `_bitmap` set to `bitmap`.
    FaultStatusBits(uint64_t bitmap) : _bitmap(bitmap) {}

    FaultStatusBits(FaultStatusBits const&) = default;
    FaultStatusBits& operator=(FaultStatusBits const&) = default;
    virtual ~FaultStatusBits() = default;

    /// These enums should match those in SystemControlDefs.lvlib:Faults-WarningsEnum.ctl
    /// Changing the values of these enums will likely cause hardware problems.
    enum {
        STALE_DATA_WARN=0,                       ///< “stale data warning” = 0
        STALE_DATA_FAULT=1,                      ///< “stale data fault” = 1
        BROADCAST_ERR=2,                         ///< “broadcast error” = 2
        ACTUATOR_FAULT=3,                        ///< “actuator fault” = 3
        EXCESSIVE_FORCE=4,                       ///< “excessive force” = 4
        ACTUATOR_LIMIT_OL=5,                     ///< “actuator limit OL” = 5
        ACTUATOR_LIMIT_CL=6,                     ///< “actuator limit CL” = 6
        INCLINOMETER_W_LUT=7,                    ///< “inclinometer w/ lut” = 7
        INCLINOMETER_WO_LUT=8,                   ///< “inclinometer w/o lut” = 8
        MOTOR_VOLTAGE_FAULT=9,                   ///< “motor voltage error fault” = 9
        MOTOR_VOLTAGE_WARN=10,                   ///< “motor voltage error warning” = 10
        COMM_VOLTAGE_FAULT=11,                   ///< “comm voltage error fault” = 11
        COMM_VOLTAGE_WARN=12,                    ///< “comm voltage error warning” = 12
        MOTOR_OVER_CURRENT=13,                   ///< “motor over current” = 13
        COMM_OVER_CURRENT=14,                    ///< “comm over current” = 14
        POWER_RELAY_OPEN_FAULT=15,               ///< “power relay open fault” = 15
        POWER_HEALTH_FAULT=16,                   ///< “power supply health fault” = 16
        COMM_MULTI_BREAKER_FAULT=17,             ///< “comm multi-breaker fault” = 17
        MOTOR_MULTI_BREAKER_FAULT=18,            ///< “motor multi-breaker fault” = 18
        SINGLE_BREAKER_TRIP=19,                  ///< “single breaker trip” = 19
        POWER_SUPPLY_LOAD_SHARE_ERR=20,          ///< “power supply load share error” = 20
        DISPLACEMENT_SENSOR_RANGE_ERR=21,        ///< “displacement sensor range error” = 21
        INCLINOMETER_RANGE_ERR=22,               ///< “inclinometer range error” = 22
        MIRROR_TEMP_SENSOR_FAULT=23,             ///< “mirror temp sensor fault” = 23
        MIRROR_TEMP_SENSOR_WARN=24,              ///< “mirror temp sensor warning” = 24
        CELL_TEMP_WARN=25,                       ///< “cell temp warning” = 25
        AXIAL_ACTUATOR_ENCODER_RANGE_FAULT=26,   ///< “axial actuator encoder range fault” = 26
        TANGENT_ACTUATOR_ENCODER_RANGE_FAULT=27, ///< “tangent actuator encoder range fault” = 27
        MOTOR_RELAY=28,                          ///< “motor relay” = 28
        COMM_RELAY=29,                           ///< “comm relay” = 29
        HARDWARE_FAULT=30,                       ///< “hardware fault” = 30
        INTERLOCK_FAULT=31,                      ///< “interlock fault” = 31
        TANGENT_LOAD_CELL_FAULT=32,              ///< “tangent load cell fault” = 32
        ELEVATION_ANGLE_DIFF_FAULT=33,           ///< “elevation angle difference error fault” = 33
        MONITOR_ILC_READ_WARN=34,                ///< “monitoring ILC read error warning” = 34
        POWER_SYSTEM_TIMEOUT=35,                 ///< power system timeout, not from in LabView
        /// “SPARE_36” = 36
        /// … all SPARE …
        /// “SPARE_54” = 54
        PARAMETER_FILE_READ_FAULT=55,            ///< “configurable parameter file read error fault” = 55
        ILC_STATE_TRANSITION_FAULT=56,           ///< “ILC state transition error fault” = 56
        CRIO_COMM_FAULT=57,                      ///< “cRIO COMM error fault” = 57
        LOSS_OF_TMA_WARN=58,                     ///< “loss of TMA comm warning” = 58
        LOSS_OF_TMA_COMM_ON_ENABLE_FAULT=59,     ///< “loss of TMA comm on ENABLE fault” = 59
        TEMP_DIFF_WARN=60,                       ///< “excessive temperature differential warning” = 60
        CRIO_TIMING_FAULT=61,                    ///< “cRIO timing fault” = 61
        CRIO_TIMING_WARN=62,                     ///< “cRIO timing warning” = 62
        USER_GENERATED_FAULT=63                  ///< “user generated fault” = 63
    };

    /// Returns a mask that covers the faults allowed for closed-loop control, which is none.
    /// Should match “closed-loop control mask” found in Faults-Warnings-Info_BitStatus.lvclass:masks.vi
    static uint64_t getMaskClosedLoopControl() { return 0; }

    /// Returns a mask that covers the faults allowed for open-loop control, which is fairly small.
    /// Should match “open-loop control mask” found in Faults-Warnings-Info_BitStatus.lvclass:masks.vi
    static uint64_t getMaskOpenLoopControl();

    /// Returns a mask that covers the faults allowed for telemetry only control.
    /// Should match “telemetry-only control mask” found in Faults-Warnings-Info_BitStatus.lvclass:masks.vi
    static uint64_t getMaskTelemetryOnlyControl();

    /// Returns a mask of the faults.
    /// Should match "Fault Mask" found in Faults-Warnings-Info_BitStatus.lvclass:masks.vi
    static uint64_t getMaskFaults();

    /// Returns a mask of the warnings.
    /// Should match "Warnings Mask" found in Faults-Warnings-Info_BitStatus.lvclass:masks.vi
    static uint64_t getMaskWarn();

    /// Returns a mask of the warnings.
    /// Should match "Info Mask" found in Faults-Warnings-Info_BitStatus.lvclass:masks.vi
    static uint64_t getMaskInfo();

    /// Returns "Affected Fault Mask" used by "TelemetryFaultManager" set in
    /// "TelemetryFaultManager.lvclass:init_affected_masks.vi"
    static uint64_t getTelemetryFaultManagerAffectedFaultMask();

    /// Returns "Affected Warning Mask" used by "TelemetryFaultManager" set in
    /// "TelemetryFaultManager.lvclass:init_affected_masks.vi"
    static uint64_t getTelemetryFaultManagerAffectedWarningMask();

    /// Returns "Affected Faults Bit Mask" used by "PowerSubsystem" set in
    /// "PowerSubsystem.lvclass:create_affected_masks.vi"
    static uint64_t getPowerSubsystemFaultManagerAffectedFaultMask();

    /// Returns "Affected Warnings/Info Bit Mask" used by "PowerSubsystem" set in
    /// "PowerSubsystem.lvclass:create_affected_masks.vi"
    static uint64_t getPowerSubsystemFaultManagerAffectedWarningMask();

    /// Returns a mask of the faults for the COMM or MOTOR power subsystem, depending on
    /// the value of `sysType`.
    /// Should match "subsystem fault mask" maps found in BasePowerSubsystem.lvclass:set_fault_masks.vi
    static uint64_t getMaskPowerSubsystemFaults(control::PowerSystemType sysType);

    /// Returns a mask of the health faults for the powre system.
    static uint64_t getMaskHealthFaults();

    /// Set (when `set` == true) or unset (when `set` == false) the bit at `pos` in `bitmap`
    static void setBit64(uint64_t& bitmap, int pos, bool set);

    /// Set (when `set` == true) or unset (when `set` == false) the bit at `pos` in `bitmap`
    static void setBit32(uint32_t& bitmap, int pos, bool set);

    /// Set (when `set` == true) or unset (when `set` == false) the bit at `pos` in `bitmap`
    static void setBit8(uint8_t& bitmap, int pos, bool set);

    /// Set the bit in `_bitmap` at `pos` to '1'.
    /// @throws range_error.
    void setBitAt(int pos);

    /// Unset the bit in `_bitmap` at `pos`.
    /// @throws range_error.
    void unsetBitAt(int pos);

    /// Return a copy of `_bitmap`.
    uint64_t getBitmap() const { return _bitmap; }

    /// Set `_bitmap` to `bitmap`.
    void setBitmap(uint64_t bitmap) { _bitmap = bitmap; }

    /// Return the string associated with `enumVal`.
    static std::string getEnumString(int enumVal);

    /// Return a string containing the string version of all set bit enums
    std::string getAllSetBitEnums();

    /// Return the bit in `_bitmap` at `pos`, `pos` out of range returns 0.
    uint64_t getBit(int pos);

    /// Return all bits that are set in `_bitmap` and in `mask`.
    uint64_t getBitsSetInMask(uint64_t mask) {
        return _bitmap & mask;
    }

    /// Return all bits that are set in `_bitmap` and not in `mask`.
    uint64_t getBitsSetOutOfMask(uint64_t mask) {
        return _bitmap & ~mask;
    }

    /// Return a binary string representation of `_bitmap`.
    static std::string getBinaryStr(uint64_t);

private:
    uint64_t _bitmap = 0; ///< Bitmap of status.

    static Ptr _maskOpenLoopControl; ///< Stored “open-loop control mask”.
    static Ptr _maskTelemetryOnlyControl; ///< Stored “telemetry-only control mask”.
    static Ptr _maskFaults; ///< Stored "Fault Mask".
    static Ptr _maskWarn; ///< Stored "Warnings Mask".
    static Ptr _maskInfo; ///< Stored "Info Mask".

    static Ptr _maskSubsystemCommFault; ///< Stored "subsystem fault mask" for COMM.
    static Ptr _maskSubsystemMotorFault; ///< Stored "subsystem fault mask" for MOTOR.

    /// Stored "Telemetry Fault Manager Affected Fault Mask" for TelemetryFaultManager
    static Ptr _telemetryFaultManagerAffectedFaultMask;

    /// Stored "Telemetry Fault Manager Affected Warning Mask" for TelemetryFaultManager
    static Ptr _telemetryFaultManagerAffectedWarningMask;

    /// Stored "PowerSubsystem Affected Fault Mask"
    static Ptr _powerSubsystemFaultManagerAffectedFaultMask;

    /// Stored "PowerSubsystem Affected Fault Mask"
    static Ptr _powerSubsystemFaultManagerAffectedWarningMask;

    /// Stored health fault mask
    static Ptr _healthFaultMask;

};


}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_FAULTMGR_FAULTSTATUSBITS_H
