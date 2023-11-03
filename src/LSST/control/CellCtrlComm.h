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

#ifndef LSST_M2CELLCPP_CELLCTRLCOMM_H
#define LSST_M2CELLCPP_CELLCTRLCOMM_H

// System headers
#include <memory>

// Project headers

namespace LSST {
namespace m2cellcpp {
namespace control {

/// &&& This class represents Controller->startupCellCommunications and CellCommunications.vi
/// &&& doc.  (is this class poorly named???)
//
// &&& This section has to with configuring the MotionCtrl(motion control loop objects) FpgaCtrl(FPGA setup, read/write, simu)
// &&& Controller->InitializeFunctionalGlobals.vi - The individual items should be done in a separate class/function
// &&&    getCellGeomFromFile.vi - Opens the cellGeom.json file and uses it to set configuration file values.
// &&&     - critical read values:
// &&&        - locAct_axial (2d array 64 bit double array in m)
// &&&        - locAct_tangent (64 bit double angle, file in deg, internal in rad)
// &&&        - radiusActTangent (64 bit double in m, (demo value of 1.780189734 which can probably be ignored)
// &&&     - CellConfiguration->Write Axial Actuator Locations(X, Y[m]).vi  - c++ probably just store in cell configuration data structure
// &&&     - CellConfiguration->Write Tangent Angular Locations.vi  - c++ probably just store in cell configuration data structure
// &&&     - CellConfiguration->Write Tangent Actuator Radius.vi  - c++ probably just store in cell configuration data structure
// &&&     - CellConfiguration->Write numOfAxialAct.vi  - c++ probably just store in cell configuration data structure
// &&&     - CellConfiguration->Write numOfTangentAct.vi  - c++ probably just store in cell configuration data structure
// &&&     - CellConfiguration->InitializeWithDefaultData.vi  - take the data from CellConfiguration and send to CellActuatorData (which stores it, but where is it accessed ??? )
// &&&   - Control Loop Force Limits FG: - called with init parameter
// &&&     - Sets the open and closed loop force limits (ask Te-Wei or Patricio about correct values)
// &&&        - Max Closed-Loop Axial Force - default 100 lbf
// &&&        - Max Closed-Loop Tangent Force - default 1100 lbf
// &&&        - Max Open-Loop Axial Force (default) - 110 lbf
// &&&        - Max Open-Loop Tangent Force (default) - 1350 lbf   *** front pane of the DefaultMaxTangentOpen-LoopForceLimit.vi is mislabled as closed-loop
// &&&        - Max Open Loop Force Limits
// &&&           - Axial - 140 lbf
// &&&           - Tangent - 1400 lbf
// &&&   - Step Speed Limit FG - init   - StepVelocityLimitsFG.vi
// &&&     - Axial - 40  (units ???)
// &&&     - Tangent - 127  (units ???)
// &&&   - CL Control Params FG - init - ControlLoopControlParametersFG.vi ** several config values
// &&&      * Stale data max count taken from json file item - ilcStaleDataLimit
// &&&      - Telemetry Control Parameters.Actuator Encoder Range
// &&&         - Axial ILC Min Encoder Value = -16000000
// &&&         - Axial ILC Max Encoder Value =  16000000
// &&&         - Tangent ILC Min Encoder Value = -16000000
// &&&         - Tangent ILC Max Encoder Value =  16000000
// &&&      - Telemetry Control Parameters.Inclinometer Sensor Range
// &&&         - Min Inclinometer Value = -0.001745329251994 rad
// &&&         - Max Inclinometer Value =  6.284930636432 rad
// &&&      - Telemetry Control Parameters.Temperature Sensor Range
// &&&         - Min Cell Temperature = 233.15 Cdeg
// &&&         - Max Cell Temperature = 318.15 Cdeg
// &&&         - Min Mirror Temperature = 233.15 Cdeg
// &&&         - Max Mirror Temperature = 313.15 Cdeg
// &&&      - Telemetry Control Parameters.Displacement Sensor Range
// &&&         - min = -0.0001  m
// &&&         - max =  0.04 m
// &&&   - Calib Data FG:CalibrationDataFG.vi
// &&&     - Motor Voltage, Comm Voltage, Com Current - all have the same values
// &&&        - Gain = 1.0
// &&&        - Offset = 0.0
// &&&     - Motor Current has
// &&&        - Gain = 5.0
// &&&        - offset = 0.0
// &&&PowerSystem   - System Config FG:SystemConfigurationFG.vi  ****** power up configuration values ********
// &&&PowerSystem     There are a lot of configuration values in here having to do with power supply very specific,
// &&&PowerSystem     probably best to open up the vi and look at the set values. Elements set listed below
// &&&PowerSystem     - "Power Subsystem Configuration Parameters.Power Subsystem Common Configuration Parameters"
// &&&PowerSystem     - "Power Subsystem Configuration Parameters.Comm Power Bus Configuration Parameters"
// &&&PowerSystem     - "Power Subsystem Configuration Parameters.Motor Power Bus Configuration Parameters"
// &&&PowerSystem     - "Power Subsystem Configuration Parameters.Boost Current Fault Enabled" - this is set to false.
// &&& - Ctrlr.U/D F Limit:updateForceLimits.vi - Pulls limitForce values for several items from a json file (??? which file)
// &&&   - limitForce_closedLoopAxial
// &&&   - limitForce_closedLoopTangent
// &&&   - limitForce_openLoopAxial
// &&&   - limitForce_openLoopTangent
// &&&   - limitForceMax_openLoopAxial
// &&&   - limitForceMax_openLoopTangent
//
class CellCtrlComm {
public:
    using Ptr = std::shared_ptr<CellCtrlComm>;

    static void setup();
private:
    CellCtrlComm();


};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CELLCTRLCOMM_H
