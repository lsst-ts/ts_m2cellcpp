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

#ifndef LSST_M2CELLCPP_SYSTEM_GLOBALS_H
#define LSST_M2CELLCPP_SYSTEM_GLOBALS_H

// System headers
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

// Third party headers
#include "nlohmann/json.hpp"

// Project headers

namespace LSST {
namespace m2cellcpp {
namespace system {

class Config;

/// This class is used to contain global instances. It is meant to be
/// setup once using the system::Config. Setting and values in this class
/// should be threadsafe and there should only be one instance of this class.
///
///For unit testing and other simple tests, the class default values may be used.
/// FUTURE: Many of the values here are likely to be place holders with values
///         to help it play nicely with the existing GUI. Eventually, all of these
///         need to work properly or be removed. This may actually be a reasonable
///         place to keep these values.
///         Part of the purpose of this class is to keep the "PLACEHOLDERs" in one
///         location so they aren't lurking in the depths of the code.
class Globals {
public:
    /// Create the global Context object, aquiring some values from the `config`.
    static void setup(Config const& config);

    /// Return a reference to the Global instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static Globals& get();

    Globals() = delete;
    Globals(Globals const&) = delete;
    Globals& operator=(Globals const&) = delete;

    ~Globals() = default;

    /// Return const vector of the hardpoints.
    std::vector<int>& getHardPointList() {
        std::lock_guard<std::mutex> lock(_hardPointListMtx);
        return _hardPointList;
    }

    /// Return list of ring temperature offsets. PLACEHOLDER
    std::vector<double>& getTemperatureOffsetsRing() {
        std::lock_guard<std::mutex> lock(_temperatureOffsetsRingMtx);
        return _temperatureOffsetsRing;
    }

    /// Return list of intake temperature offsets. PLACEHOLDER
    std::vector<double>& getTemperatureOffsetsIntake() {
        std::lock_guard<std::mutex> lock(_temperatureOffsetsIntakeMtx);
        return _temperatureOffsetsIntake;
    }

    /// Return list of exhaust temperature offsets. PLACEHOLDER
    std::vector<double>& getTemperatureOffsetsExhaust() {
        std::lock_guard<std::mutex> lock(_temperatureOffsetsExhaustMtx);
        return _temperatureOffsetsExhaust; }

    /// Increment the number of connections when `connecting` is true, decrement when false;
    void setTcpIpConnected(bool connecting);

    /// Return true if the number of connections is greater than zero.
    bool getTcpIpConnected() const;

    /// Returns true when system is commandable by DDS. PLACEHOLDER
    bool getCommandableByDds() const { return _commandableByDds; }

    /// Returns a json msg string suitable to be read by clients.
    nlohmann::json getCommandableByDdsJson() const;

    /// Returns the value of interlock. PLACEHOLDER
    bool getInterlock() const { return _interlock; }

    /// Returns telemetry source identifier.
    int getTelemetrySource() const { return _telemetrySource; }

    /// Return summary system state. PLACEHOLDER
    int getSummaryState() const { return _summaryState; }

    /// Set summary system state to `val`. PLACEHOLDER
    void setSummaryState(int val) { _summaryState = val; }

    /// Return model digital input. PLACEHOLDER
    uint32_t getDigitalInput() const { return _digitalInput; }

    /// Return model digital output. PLACEHOLDER
    uint32_t getDigitalOutput() const { return _digitalOutput; }

    /// Return the closed loop control mode identifier. PLACEHOLDER
    int getClosedLoopControlMode() const { return _closedLoopControlMode; }

    /// Set command source to remote when `isRemote` is true, otherwise local.
    /// @return true if the command source was set successfully.
    bool setCommandSourceIsRemote(bool isRemote) {
        _commandSourceIsRemote = isRemote;
        _commandableByDds = isRemote;
        return true;
    }

    /// Reset the global Ptr to Globals. PLACEHOLDER
    /// This should only be called at termination or unit testing.
    void reset();

private:
    /// Private constructor to force call to `setup(Config const& config)`.
    Globals(Config const& config);

    static std::unique_ptr<Globals> _thisPtr; ///< Pointer to the global instance of Globals
    static std::mutex _thisMtx;               ///< Protects _thisPtr.

    /// List of hard points.
    /// The hardpoint list is the 1-based index {6, 16, 26, 74, 76, 78}
    /// some calculations are done with the 0-based index {5, 15,25,73, 75, 77}
    /// FUTURE: Is this where this should be set/stored? PLACEHOLDER
    std::vector<int> _hardPointList{6, 16, 26, 74, 76, 78};
    std::mutex _hardPointListMtx; ///< Protects `_hardPointList`

    /// List of ring temperature offsets in degrees C. PLACEHOLDER
    std::vector<double> _temperatureOffsetsRing{21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0};
    std::mutex _temperatureOffsetsRingMtx; ///< Protects `_temperatureOffsetsRing`

    /// List of intake temperature offsets. PLACEHOLDER
    std::vector<double> _temperatureOffsetsIntake{0.0, 0.0};
    std::mutex _temperatureOffsetsIntakeMtx; ///< Protects `_temperatureOffsetsIntake`

    /// List of exhaust temperature offsets. PLACEHOLDER
    std::vector<double> _temperatureOffsetsExhaust{0.0, 0.0};
    std::mutex _temperatureOffsetsExhaustMtx; ///< Protects `_temperatureOffsetsExhaust`

    int _tcpIpConnectedCount = 0;  ///< Number of active connections   FUTURE: Should this be in ComControlServer?
    mutable std::mutex _tcpIpConnectedMtx; ///< protects _tcpIpConnectedCount

    std::atomic<bool> _commandableByDds{true}; ///< True when system commandable by DDS. PLACEHOLDER
    std::atomic<bool> _interlock{true}; ///< Status of the interlock. PLACEHOLDER
    std::atomic<int>  _telemetrySource{1}; ///< telemetry source identifier. PLACEHOLDER
    std::atomic<int> _summaryState{5}; ///< Summary system state. PLACEHOLDER from python - salobj.State.OFFLINE=5

    std::atomic<uint32_t> _digitalInput{0x9F00FFFF}; ///< Model digital input PLACEHOLDER from python = 2667642879
    std::atomic<uint32_t> _digitalOutput{0x1C}; ///< Model digital output PLACEHOLDER from python = 28

    std::atomic<int> _closedLoopControlMode{1}; ///< Closed loop control mode. PLACEHOLDER from python 1=ClosedLoopControlMode.Idle

    std::atomic<bool> _commandSourceIsRemote{false}; ///< Command source is remote when true, otherwise false.

};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_GLOBALS_H
