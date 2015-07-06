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

namespace WinWire {
    template<class TGpioProvider>
    class HBridge
    {
    public:
        HBridge(int in1Pin, int in2Pin) :
            In1Pin(in1Pin),
            In2Pin(in2Pin)
        {}

        bool Init()
        {
            TGpioProvider::Inst().GpioPinSetDir(In1Pin, TGpioProvider::DIR_Output);
            TGpioProvider::Inst().GpioPinSetDir(In2Pin, TGpioProvider::DIR_Output);

            return true;
        }

        void Forward()
        {
            TGpioProvider::Inst().GpioPinWrite(In2Pin, false);
            TGpioProvider::Inst().GpioPinWrite(In1Pin, true);
        }

        void Backward()
        {
            TGpioProvider::Inst().GpioPinWrite(In1Pin, false);
            TGpioProvider::Inst().GpioPinWrite(In2Pin, true);
        }

        void Stop()
        {
            TGpioProvider::Inst().GpioPinWrite(In1Pin, false);
            TGpioProvider::Inst().GpioPinWrite(In2Pin, false);
        }

    private:
        int In1Pin;
        int In2Pin;
    };
}