/*
 * This file is part of LSST ts_m2cellcpp test suite.
 *
 * Developed for the Vera C. Rubin Observatory Telescope & Site Software Systems.
 * This product includes software developed by the Vera C.Rubin Observatory Project
 * (https://www.lsst.org). See the COPYRIGHT file at the top-level directory of
 * this distribution for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define CATCH_CONFIG_MAIN

// System headers
#include <exception>

// 3rd party headers
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

// Project headers
#include "faultmgr/FaultStatusBits.h"
#include "control/FpgaIoOld.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;
using namespace LSST::m2cellcpp::faultmgr;

TEST_CASE("Test FpgaIo", "[FpgaIo]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");
    LSST::m2cellcpp::system::Config::setup(cfgPath + "unitTestCfg.yaml");
    {
        LDEBUG("Testing Ilc");
        Ilc ilc("test_1", 1);

        ilc.setStatus(0xe1);
        REQUIRE(ilc.getBroadcastCommCount() == 0x0e);
        REQUIRE(ilc.getFault() == true);
        REQUIRE(ilc.getCWLimit() == false);
        REQUIRE(ilc.getCCWLimit() == false);

        ilc.setStatus(0x14);
        REQUIRE(ilc.getBroadcastCommCount() == 0x01);
        REQUIRE(ilc.getFault() == false);
        REQUIRE(ilc.getCWLimit() == true);
        REQUIRE(ilc.getCCWLimit() == false);

        ilc.setStatus(0x38);
        REQUIRE(ilc.getBroadcastCommCount() == 0x03);
        REQUIRE(ilc.getFault() == false);
        REQUIRE(ilc.getCWLimit() == false);
        REQUIRE(ilc.getCCWLimit() == true);
    }

    {
        LDEBUG("Testing AllIlcs");
        AllIlcs ilcs(true);
        auto a = ilcs.getIlc(1);
        REQUIRE(a->getName() == "Axial_1");
        REQUIRE(a->getIdNum() == 1);
        a = ilcs.getIlc(72);
        REQUIRE(a->getName() == "Axial_72");
        REQUIRE(a->getIdNum() == 72);
        a = ilcs.getIlc(73);
        REQUIRE(a->getName() == "Tangent_73");
        REQUIRE(a->getIdNum() == 73);
        a = ilcs.getIlc(78);
        REQUIRE(a->getName() == "Tangent_78");
        REQUIRE(a->getIdNum() == 78);

        REQUIRE_THROWS(ilcs.getIlc(0));
        REQUIRE_THROWS(ilcs.getIlc(79));
    }

    {
        LDEBUG("Test DaqInMock DaqOutMock");
        FpgaIoOld fpga(true);

        auto ilcMotorCurrent = fpga.getIlcMotorCurrent();
        auto ilcCommCurrent = fpga.getIlcCommCurrent();
        auto ilcMotorVoltage = fpga.getIlcMotorVoltage();
        auto ilcCommVoltage = fpga.getIlcCommVoltage();

        auto ilcMotorPowerOnOut = fpga.getIlcMotorPowerOnOut();
        auto ilcCommPowerOnOut = fpga.getIlcCommPowerOnOut();
        auto crioInterlockEnableOut = fpga.getCrioInterlockEnableOut();

        auto ilcMotorPowerOnIn = fpga.getIlcMotorPowerOnIn();
        auto ilcCommPowerOnIn = fpga.getIlcCommPowerOnIn();
        auto crioInterlockEnableIn = fpga.getCrioInterlockEnableIn();

        auto ilcs = fpga.getAllIlcs();

        auto testIlcMotorCurrent = fpga.getTestIlcMotorCurrent();
        auto testIlcCommCurrent = fpga.getTestIlcCommCurrent();
        auto testIlcMotorVoltage = fpga.getTestIlcMotorVoltage();
        auto testIlcCommVoltage = fpga.getTestIlcCommVoltage();

        LDEBUG("Test links");
        testIlcMotorCurrent->setSource(10.0);
        DaqOutMock::Data daqOutData = testIlcMotorCurrent->getData();
        REQUIRE(daqOutData.source == 10.0);
        DaqInMock::Data daqInData0 = ilcMotorCurrent->getData();

        testIlcMotorCurrent->write();
        daqOutData = testIlcMotorCurrent->getData();
        REQUIRE(daqOutData.source == 10.0);
        REQUIRE(daqOutData.outVal == 5.0);

        DaqInMock::Data daqInData1 = ilcMotorCurrent->getData();
        REQUIRE(daqInData1.raw == 5.0);
        REQUIRE(daqInData0.lastRead != daqInData1.lastRead);

        DaqInMock::Data daqInData3 = ilcMotorCurrent->getData();
        REQUIRE(daqInData3.adjusted == 10.0);

        LDEBUG("Test DaqBoolInMock DaqBoolOutMock: Motor=true, Comm=false, interlock=false");
        FpgaTimePoint lastWrite = ilcMotorPowerOnOut->getLastWrite();
        FpgaTimePoint lastRead = ilcMotorPowerOnIn->getLastRead();
        ilcMotorPowerOnOut->setVal(true);
        REQUIRE(ilcMotorPowerOnOut->getVal() == true);
        ilcCommPowerOnOut->setVal(false);
        REQUIRE(ilcCommPowerOnOut->getVal() == false);
        fpga.writeAllOutputs();
        REQUIRE(lastWrite != ilcMotorPowerOnOut->getLastWrite());
        REQUIRE(ilcMotorPowerOnIn->getVal() == true);
        REQUIRE(lastRead != ilcMotorPowerOnIn->getLastRead());
        REQUIRE(ilcCommPowerOnIn->getVal() == false);
        REQUIRE(crioInterlockEnableOut->getVal() == false);
        REQUIRE(crioInterlockEnableIn->getVal() == false);

        auto mCurrent = ilcMotorCurrent->getData();
        auto mVoltage = ilcMotorVoltage->getData();
        LDEBUG("mCurrent adjusted=", mCurrent.adjusted, " raw=", mCurrent.raw);
        REQUIRE(mCurrent.raw == 0.25);
        REQUIRE(mCurrent.adjusted == 0.5);
        LDEBUG("mVoltage adjusted=", mVoltage.adjusted, " raw=", mVoltage.raw);
        REQUIRE(mVoltage.raw == 8.0);
        REQUIRE(mVoltage.adjusted == 8.0);

        auto cCurrent = ilcCommCurrent->getData();
        auto cVoltage = ilcCommVoltage->getData();
        LDEBUG("cCurrent adjusted=", cCurrent.adjusted, " raw=", cCurrent.raw);
        REQUIRE(cCurrent.raw == 0.0);
        REQUIRE(cCurrent.adjusted == 0.0);
        LDEBUG("cVoltage adjusted=", cVoltage.adjusted, " raw=", cVoltage.raw);
        REQUIRE(cVoltage.raw == 0.0);
        REQUIRE(cVoltage.adjusted == 0.0);

        LDEBUG("Test DaqBoolInMock DaqBoolOutMock: Motor=false, Comm=true, interlock=false");
        ilcMotorPowerOnOut->setVal(false);
        ilcCommPowerOnOut->setVal(true);
        fpga.writeAllOutputs();
        REQUIRE(ilcMotorPowerOnIn->getVal() == false);
        REQUIRE(ilcCommPowerOnIn->getVal() == true);
        REQUIRE(crioInterlockEnableOut->getVal() == false);
        REQUIRE(crioInterlockEnableIn->getVal() == false);

        mCurrent = ilcMotorCurrent->getData();
        mVoltage = ilcMotorVoltage->getData();
        LDEBUG("mCurrent adjusted=", mCurrent.adjusted, " raw=", mCurrent.raw);
        REQUIRE(mCurrent.raw == 0.0);
        REQUIRE(mCurrent.adjusted == 0.0);
        LDEBUG("mVoltage adjusted=", mVoltage.adjusted, " raw=", mVoltage.raw);
        REQUIRE(mVoltage.raw == 0.0);
        REQUIRE(mVoltage.adjusted == 0.0);

        cCurrent = ilcCommCurrent->getData();
        cVoltage = ilcCommVoltage->getData();
        LDEBUG("cCurrent adjusted=", cCurrent.adjusted, " raw=", cCurrent.raw);
        REQUIRE(cCurrent.raw == 0.1);
        REQUIRE(cCurrent.adjusted == 0.1);
        LDEBUG("cVoltage adjusted=", cVoltage.adjusted, " raw=", cVoltage.raw);
        REQUIRE(cVoltage.raw == 12.0);
        REQUIRE(cVoltage.adjusted == 12.0);

        LDEBUG("Test DaqBoolInMock DaqBoolOutMock: interlock=true");
        crioInterlockEnableOut->setVal(true);
        fpga.writeAllOutputs();
        REQUIRE(crioInterlockEnableOut->getVal() == true);
        REQUIRE(crioInterlockEnableIn->getVal() == true);

        LCRITICAL(fpga.dump());
    }
}

TEST_CASE("Test FaultStatusMap", "[FaultStatusMap]") {
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");

    {
        FaultStatusBits fsm;
        REQUIRE(fsm.getBitmap() == 0);

        uint64_t closedLoop = FaultStatusBits::getMaskClosedLoopControl();
        uint64_t openLoop = FaultStatusBits::getMaskOpenLoopControl();
        uint64_t telemetry = FaultStatusBits::getMaskTelemetryOnlyControl();
        uint64_t faults = FaultStatusBits::getMaskFaults();
        uint64_t faultsOther = FaultStatusBits::getMaskFaults();
        uint64_t warn = FaultStatusBits::getMaskWarn();
        uint64_t warnOther = FaultStatusBits::getMaskWarn();
        uint64_t info = FaultStatusBits::getMaskInfo();
        uint64_t infoOther = FaultStatusBits::getMaskInfo();

        REQUIRE(closedLoop == 0);
        REQUIRE(faults == faultsOther);
        REQUIRE(faults != 0);
        REQUIRE(warn == warnOther);
        REQUIRE(warn != 0);
        REQUIRE(info == infoOther);
        REQUIRE(info != 0);

        fsm.setBitAt(FaultStatusBits::CRIO_TIMING_FAULT);
        REQUIRE(fsm.getBit(FaultStatusBits::CRIO_TIMING_FAULT) != 0);

        fsm.setBitAt(FaultStatusBits::BROADCAST_ERR);  // = 2
        REQUIRE(fsm.getBit(FaultStatusBits::CRIO_TIMING_FAULT) != 0);
        REQUIRE(fsm.getBit(FaultStatusBits::BROADCAST_ERR) == 4);

        fsm.unsetBitAt(FaultStatusBits::CRIO_TIMING_FAULT);
        REQUIRE(fsm.getBit(FaultStatusBits::CRIO_TIMING_FAULT) == 0);
        REQUIRE(fsm.getBit(FaultStatusBits::BROADCAST_ERR) == 4);

        fsm.setBitmap(openLoop);
        LDEBUG("telemetry=", FaultStatusBits::getBinaryStr(telemetry));
        LDEBUG("openLoop=", FaultStatusBits::getBinaryStr(openLoop));
        FaultStatusBits diff(fsm.getBitsSetOutOfMask(telemetry));
        LDEBUG("diff a=", FaultStatusBits::getBinaryStr(diff.getBitmap()), " ", diff.getAllSetBitEnums());
        REQUIRE(diff.getBitmap() == 0);
        REQUIRE(fsm.getBitsSetInMask(openLoop) != 0);

        fsm.setBitmap(telemetry);
        diff.setBitmap(fsm.getBitsSetOutOfMask(telemetry));
        LDEBUG("diff b=", FaultStatusBits::getBinaryStr(diff.getBitmap()), " ", diff.getAllSetBitEnums());

        REQUIRE_THROWS(fsm.setBitAt(64));
        REQUIRE_THROWS(fsm.setBitAt(-1));
    }
}
