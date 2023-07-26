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
    /// Create the global Context object
    /// &&& doc
    static void setup();

    /// &&& doc
    static void setup(Config const& config);

    /// &&& doc
    static Globals& get();

    Globals(Globals const&) = delete;
    Globals& operator=(Globals const&) = delete;

    ~Globals() = default;

    /// &&& all of these vectors must change to be thread safe
    /// Return const vector of the hardpoints.
    std::vector<int>& getHardPointList() { return _hardPointList; }
    /// Return list of ring temperature offsets. PLACEHOLDER
    std::vector<double>& getTemperatureOffsetsRing() { return _temperatureOffsetsRing; }
    /// Return list of intake temperature offsets. PLACEHOLDER
    std::vector<double>& getTemperatureOffsetsIntake() { return _temperatureOffsetsIntake; }
    /// Return list of exhaust temperature offsets. PLACEHOLDER
    std::vector<double>& getTemperatureOffsetsExhaust() { return _temperatureOffsetsExhaust; }

    /// Increment the the number of connections when `connecting` is true, decrement when false;
    void setTcpIpConnected(bool connecting);

    /// Return true if the number of connections is greater than zero.
    bool getTcpIpConnected() const;

    /// Returns true when system is commandable by DDS. PLACEHOLDER
    bool getCommandableByDds() const { return _commandableByDds; }

    /// Returns the value of interlock. PLACEHOLDER
    bool getInterlock() const { return _interlock; }

    /// Returns telemetry source identifier.
    int getTelemetrySource() const { return _telemetrySource; }

    /// Return summary system state. PLACEHOLDER
    int getSummaryState() const { return _summaryState; }

    /// Return model digital input. PLACEHOLDER
    uint32_t getDigitalInput() const { return _digitalInput; }

    /// Return model digital output. PLACEHOLDER
    uint32_t getDigitalOutput() const { return _digitalOutput; }

    /// Return model enabled fault mask. PLACEHOLDER
    uint64_t getEnabledFaultMask() const { return _enabledFaultMask; }

    /// Return the closed loop control mode identifier. PLACEHOLDER
    int getClosedLoopControlMode() const { return _closedLoopControlMode; }

    /// Reset the global Ptr to Globals. PLACEHOLDER
    /// This should only be called at termination or unit testing.
    void reset();

private:
    Globals();

    static std::unique_ptr<Globals> _thisPtr; ///< Pointer to the global instance of Globals
    static std::mutex _thisMtx;               ///< Protects _thisPtr.

    /// List of hard points.
    /// FUTURE: Is this where this should be set/stored? PLACEHOLDER
    std::vector<int> _hardPointList{6, 16, 26, 74, 76, 78};

    /// List of ring temperature offsets. PLACEHOLDER
    std::vector<double> _temperatureOffsetsRing{21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0, 21.0};
    /// List of intake temperature offsets. PLACEHOLDER
    std::vector<double> _temperatureOffsetsIntake{0.0, 0.0};
    /// List of exhaust temperature offsets. PLACEHOLDER
    std::vector<double> _temperatureOffsetsExhaust{0.0, 0.0};


    int _tcpIpConnectedCount = 0;  ///< Number of active connections   FUTURE: Should this be in ComControlServer?
    mutable std::mutex _tcpIpConnectedMtx; ///< protects _tcpIpConnectedCount

    std::atomic<bool> _commandableByDds{true}; ///< True when system commandable by DDS. PLACEHOLDER
    std::atomic<bool> _interlock{true}; ///< Status of the interlock. PLACEHOLDER
    std::atomic<int>  _telemetrySource{1}; ///< telemetry source identifier. PLACEHOLDER
    std::atomic<int> _summaryState{5}; ///< Summary system state. PLACEHOLDER from python - salobj.State.OFFLINE=5

    std::atomic<uint32_t> _digitalInput{0x9F00FFFF}; ///< Model digital input PLACEHOLDER from python = 2667642879
    std::atomic<uint32_t> _digitalOutput{0x1C}; ///< Model digital output PLACEHOLDER from python = 28
    std::atomic<uint64_t> _enabledFaultMask{0xff800007ffffffff}; ///< Model enabled fault mask. PLACEHOLDER from python = 18410715311050326015

    std::atomic<int> _closedLoopControlMode{1}; ///< Closed loop control mode. PLACEHOLDER from python 1=ClosedLoopControlMode.Idle

};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_GLOBALS_H
