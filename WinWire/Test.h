//
// Copyright 2015 Muhamad Lotfy
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include "common.h"

namespace WinWire {
    template<class TGpioProvider>
    static void BenchmarkGpio(int pinNum, const int numSamples)
    {
        HpcTimer timer;

        TGpioProvider::Inst().GpioPinSetDir(pinNum, TGpioProvider::DIR_Output);

        timer.Start();
        for (int i = 0; i < numSamples; ++i)
        {
            TGpioProvider::Inst().GpioPinWrite(pinNum, (i & 1) > 0);
        }
        timer.Stop();

        double writeOps = timer.OperationsPerSecond(numSamples);
        LogInfo("GPIO Writes/Second = %d, Write=%fus", (int)writeOps, 1000000.0 / writeOps);

        TGpioProvider::Inst().GpioPinSetDir(pinNum, TGpioProvider::DIR_Input);

        timer.Start();
        for (int i = 0; i < numSamples; ++i)
        {
            bool state = TGpioProvider::Inst().GpioPinRead(pinNum);
        }
        timer.Stop();

        double readOps = timer.OperationsPerSecond(numSamples);
        LogInfo("GPIO Reads/Second = %d, Read=%fus", (int)readOps, 1000000.0 / readOps);
    }
};