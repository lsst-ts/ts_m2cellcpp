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
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "util/EventThread.h"
#include "util/Log.h"
#include "util/ThreadPool.h"

#include <iostream>

using namespace std;
using namespace LSST::m2cellcpp::util;

TEST_CASE("EventThread test", "[EventThread]") {
    Log::getLog().useEnvironmentLogLvl();
    LDEBUG("EventThread test");

    struct SumUnprotected {
        int total{0};
        void add(int val) { total += val; }
    };

    /// Queue a sum depending on mutex protection within the EventThread.
    {
        EventThread et{};
        SumUnprotected sum;
        int total{0};
        int cycles = 99;  // Arbitrary number of times add message to queue.
        for (int j = 1; j < cycles; j++) {
            auto cmdSum = std::make_shared<Command>([&sum, j](CmdData*) { sum.add(j); });
            total += j;
            et.queCmd(cmdSum);
        }
        et.run();
        for (int j = 1; j < cycles; j++) {
            auto cmdSum = std::make_shared<Command>([&sum, j](CmdData*) { sum.add(j); });
            total += j;
            et.queCmd(cmdSum);
        }
        et.queEnd();
        et.join();
        REQUIRE(total == sum.total);
    }

    /// Create a thread pool
    std::weak_ptr<CommandQueue> weak_que;
    std::weak_ptr<ThreadPool> weak_pool;

    /// Create a thread pool
    {
        auto cmdQueue = std::make_shared<CommandQueue>();
        weak_que = cmdQueue;
        unsigned int sz = 2;  // size of thread pool to create
        auto pool = ThreadPool::newThreadPool(sz, cmdQueue);
        weak_pool = pool;
        LDEBUG("pool size=", sz);
        REQUIRE(pool->size() == sz);

        // Shrink the pool to zero and verify that the pool is shutdown.
        pool->shutdownPool();
        LDEBUG("pool size=0 weak_pool.use_count=", weak_pool.use_count());
        pool->resize(20);  // Size should remain zero, since shutdownPool() called.
        REQUIRE(pool->size() == 0);
    }
    REQUIRE(weak_pool.use_count() == 0);
    REQUIRE(weak_que.use_count() == 0);

    {
        auto cmdQueue = std::make_shared<CommandQueue>();
        weak_que = cmdQueue;
        unsigned int sz = 10;  // size of thread pool to create
        auto pool = ThreadPool::newThreadPool(sz, cmdQueue);
        weak_pool = pool;
        LDEBUG("pool size=", sz);
        REQUIRE(pool->size() == sz);
        sz += 10;  // test increase in size of thread pool.
        pool->resize(sz);
        LDEBUG("pool size=", sz, " weak_pool.use_count=", weak_pool.use_count());
        REQUIRE(pool->size() == sz);
        sz = 5;  // test decrease in size of thread pool.
        pool->resize(sz);
        pool->waitForResize(10000);
        LDEBUG("pool size=", sz, " weak_pool.use_count=", weak_pool.use_count());
        REQUIRE(pool->size() == sz);

        /// Queue up a sum using the pool.
        struct Sum {
            std::atomic<int> total{0};
            void add(int val) { total += val; }
        };

        Sum poolSum;
        int total = 0;
        auto poolQueue = pool->getQueue();
        LDEBUG("Summing with pool");
        sz = 20;  // Want enough threads so that there are reasonable chance of collisions.
        pool->resize(sz);
        LDEBUG("pool size=", sz, " weak_pool.use_count=", weak_pool.use_count());
        REQUIRE(pool->size() == sz);

        for (int j = 1; j < 2000; j++) {
            auto cmdSum = std::make_shared<Command>([&poolSum, j](CmdData*) { poolSum.add(j); });
            total += j;
            poolQueue->queCmd(cmdSum);
        }
        LDEBUG("stopping all threads in pool");
        pool->endAll();  // These are added to end of queue, everything on queue should complete.
        pool->waitForResize(0);
        LDEBUG("pool size=", 0, " weak_pool.use_count=", weak_pool.use_count());
        REQUIRE(total == poolSum.total);

        // Test that a threads can leave the pool and complete and the pool size recovers.
        sz = 5;
        pool->resize(sz);
        pool->waitForResize(0);
        LDEBUG("pool size=", sz, " weak_pool.use_count=", weak_pool.use_count());
        Sum sum;
        std::vector<Tracker::Ptr> trackedCmds;
        bool go = false;
        std::condition_variable goCV;
        std::mutex goCVMtx;
        // Create more threads than can fit in the pool and don't let any complete.
        // Since they all leave the pool (via peThread->leavePool();), they should all run.
        int threadsRunning = sz * 2;
        for (int j = 0; j < threadsRunning; j++) {
            // The command to run.
            auto cmdDelaySum = std::make_shared<CommandForThreadPool>(
                    [&sum, &go, &goCV, &goCVMtx](CmdData* eventThread) {
                        PoolEventThread* peThread = dynamic_cast<PoolEventThread*>(eventThread);
                        peThread->leavePool();
                        sum.add(1);
                        LDEBUG("Wait for goCVTest.");
                        auto goCVTest = [&go]() { return go; };
                        std::unique_lock<std::mutex> goLock(goCVMtx);
                        goCV.wait(goLock, goCVTest);  // wait until go == true;
                        sum.add(1);
                    });
            trackedCmds.push_back(cmdDelaySum);  // Remember the command so we can check status later.
            poolQueue->queCmd(cmdDelaySum);      // Have the pool run the command when it can.
        }
        // Wait briefly (5sec) for all threads to be running.
        LDEBUG("Wait for all threads to be running.");
        for (int j = 0; sum.total < threadsRunning && j < 50; ++j) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // Verify pool size
        REQUIRE(pool->size() == sz);
        REQUIRE(sum.total == threadsRunning);
        // Shrink the pool to zero and verify the separated threads complete.
        pool->resize(0);
        pool->waitForResize(0);
        LDEBUG("pool size=0 weak_pool.use_count=", weak_pool.use_count());
        // Have the separated threads finish.
        {
            std::lock_guard<std::mutex> lock(goCVMtx);
            go = true;
        }
        goCV.notify_all();
        // Wait for all separated threads to finish
        for (auto const& ptc : trackedCmds) {
            LDEBUG("Wait for thread to finish.");
            ptc->waitComplete();
        }
        // sum.total should now be double what it was, indicating all threads completed.
        REQUIRE(sum.total == 2 * threadsRunning);
        LDEBUG("Shutting down pool.");
        pool->shutdownPool();
        pool.reset();
        LDEBUG("pool !exists weak_pool.use_count=", weak_pool.use_count());
    }
    REQUIRE(weak_pool.use_count() == 0);
    REQUIRE(weak_que.use_count() == 0);

    // Wait for a moderately long calculation to finish with CommandTracked.
    {
        SumUnprotected sum;
        auto cmdQueue = std::make_shared<CommandQueue>();
        weak_que = cmdQueue;
        unsigned int sz = 10;
        auto pool = ThreadPool::newThreadPool(sz, cmdQueue);
        weak_pool = pool;
        auto func = [&sum](CmdData*) {
            for (int j = 0; j < 900000; j++) {
                sum.add(1);
            }
        };

        auto cmdSumUnprotected = std::make_shared<CommandTracked>(func);

        class CommandData : public CommandTracked {
        public:
            void action(CmdData*) override {
                for (int j = 0; j < 900000; j++) {
                    total += 1;
                }
                setComplete();
            };
            int total{0};
        };
        auto commandData = std::make_shared<CommandData>();

        cmdQueue->queCmd(cmdSumUnprotected);
        cmdQueue->queCmd(commandData);

        cmdSumUnprotected->waitComplete();
        commandData->waitComplete();
        LDEBUG("cmdSumUnprotected=", sum.total, " commandData=", commandData->total);
        REQUIRE(sum.total == commandData->total);
        pool->shutdownPool();
    }

    // Give it some time to finish deleting everything (5 seconds)
    for (int j = 0; weak_pool.use_count() > 0 && j < 50; ++j) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    REQUIRE(weak_pool.use_count() == 0);
    REQUIRE(weak_que.use_count() == 0);
}
