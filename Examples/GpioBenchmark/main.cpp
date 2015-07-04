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

#define DEBUG_HPC
#include "RPi2\RPi2Fx.h"

using namespace WinWire::RPi2;

#define BENCHMARK_GPIO_PIN 22
#define BENCHMARK_ITERATIONS 1000000

int __cdecl wmain()
{
    if (!RPi2Fx::Inst().Init())
    {
        LogInfo("Failed to init WinWire lib for RPi2");
        return -1;
    }

    BenchmarkGpio(BENCHMARK_GPIO_PIN, BENCHMARK_ITERATIONS);

    return 0;
}