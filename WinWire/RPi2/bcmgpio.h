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

//
// BCM GPIO Controller
//

#include "common.h"
#include "RPi2\bcm.h"

#define BCM_GPIO_BASE_OFFSET		0x00200000
#define BCM_GPIO_CPU_BASE			(BCM_CPU_PERIPH_BASE + BCM_GPIO_BASE_OFFSET)
#define BCM_GPIO_BUS_BASE			(BCM_BUS_PERIPH_BASE + BCM_GPIO_BASE_OFFSET)
#define BCM_GPIO_REG_LEN			0x100

#define BCM_GPIO_FSEL_LENGTH		6

#define BCM_GPIO_NUM_BANKS			2
#define BCM_GPIO_BANK0_NUM_PINS		32

namespace WinWire {
    namespace RPi2 {

#include <pshpack4.h>
        typedef struct _BCM_GPIO_REGISTERS
        {
            ULONG FuncSelect[BCM_GPIO_FSEL_LENGTH];					// GPFSEL0-5
            ULONG Reserved0;
            ULONG Set[BCM_GPIO_NUM_BANKS];							// GPSET0-1
            ULONG Reserved1;
            ULONG Clear[BCM_GPIO_NUM_BANKS];						// GPCLR0-1
            ULONG Reserved2;
            ULONG Level[BCM_GPIO_NUM_BANKS];						// GPLEV0-1
            ULONG Reserved3;
            ULONG EventDetectStatus[BCM_GPIO_NUM_BANKS];			// GPEDS0-1
            ULONG Reserved4;
            ULONG RisingEdgeDetectEnable[BCM_GPIO_NUM_BANKS];		// GPREN0-1
            ULONG Reserved5;
            ULONG FallingEdgeDetectEnable[BCM_GPIO_NUM_BANKS];		// GPFEN0-1
            ULONG Reserved6;
            ULONG HighDetectEnable[BCM_GPIO_NUM_BANKS];				// GPHEN0-1
            ULONG Reserved7;
            ULONG LowDetectEnable[BCM_GPIO_NUM_BANKS];				// GPLEN0-1
            ULONG Reserved8;
            ULONG AsyncRisingEdgeDetectEnable[BCM_GPIO_NUM_BANKS];	// GPAREN0-1
            ULONG Reserved9;
            ULONG AsyncFallingEdgeDetectEnable[BCM_GPIO_NUM_BANKS];	// GPAFEN0-1
            ULONG Reserved10;
            ULONG PullupPulldownEnable;								// GPPUD
            ULONG PullupPulldownEnableClock[BCM_GPIO_NUM_BANKS];	// GPPUDCLK0-1
            ULONG Reserved12;
            ULONG Reserved13;

        } BCM_GPIO_REGISTERS, *PBCM_GPIO_REGISTERS;
#include <poppack.h>

        enum BCM_AVAIL_GPIO_PIN
        {
            BCM_GPIO4 = 4,		// P1-7
            BCM_GPIO5 = 5,		// P1-29
            BCM_GPIO6 = 6,		// P1-31
            BCM_GPIO12 = 12,	// P1-32
            BCM_GPIO13 = 13,	// P1-33
            BCM_GPIO16 = 16,	// P1-36
            BCM_GPIO17 = 17,	// P1-11
            BCM_GPIO18 = 18,	// P1-12
            BCM_GPIO21 = 21,	// P1-13
            BCM_GPIO22 = 22,	// P1-15
            BCM_GPIO23 = 23,	// P1-16
            BCM_GPIO24 = 24,	// P1-18
            BCM_GPIO25 = 25,	// P1-22
            BCM_GPIO26 = 26,	// P1-37
        };

        enum BCM_GPIO_FSEL
        {
            BCM_GPIO_FSEL_Input = 0x0,
            BCM_GPIO_FSEL_Output,
            BCM_GPIO_FSEL_Alt0 = 0x4,
            BCM_GPIO_FSEL_Alt1,
            BCM_GPIO_FSEL_Alt2,
            BCM_GPIO_FSEL_Alt3,
            BCM_GPIO_FSEL_Alt4 = 0x3,
            BCM_GPIO_FSEL_Alt5 = 0x2,
        };

        static PBCM_GPIO_REGISTERS GpioReg;

        class RPi2Gpio
        {
        public:

            enum
            {
                DIR_Input = 0x0,
                DIR_Output = 0x1
            };

            enum
            {
                PULL_None = 0x0,
                PULL_Down = 0x1,
                PULL_Up = 0x2
            };

            __inline static void GpioPinSetDir(int pinNum, int dir)
            {
                /*
                A 3-in-1 Raspbian like function
                #define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
                #define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
                #define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
                */

                // Each reg control 10 GPIO pins
                int nFsel = pinNum / 10;
                ULONG fsel = GpioReg->FuncSelect[nFsel];
                // Each FSEL register controls 10 GPIO pins, and each pin
                // has 3-bits for its function
                int numShifts = (pinNum % 10) * 3;

                // Unset the pin FSEL 3-bits
                fsel &= ~(0x7 << numShifts);

                // Set desired ALT function
                fsel |= dir << numShifts;

                WRITE_REGISTER_NOFENCE_ULONG(GpioReg->FuncSelect + nFsel, fsel);
            }

            __inline static void GpioPinSetPull(int pinNum, int pull)
            {
                // Each reg controls 32 GPIO pin
                _ASSERTE(pinNum < 32 && pinNum > 0 && "Pin number should be in the range [0,31]");

                // At least wait 150 cycles per data-sheet
                WRITE_REGISTER_NOFENCE_ULONG(&GpioReg->PullupPulldownEnable, pull);
                MicroDelay(1);

                WRITE_REGISTER_NOFENCE_ULONG(GpioReg->PullupPulldownEnableClock, 1 << pinNum);
                MicroDelay(1);

                WRITE_REGISTER_NOFENCE_ULONG(&GpioReg->PullupPulldownEnable, 0);
                WRITE_REGISTER_NOFENCE_ULONG(GpioReg->PullupPulldownEnableClock, 0);
            }

            __inline static void GpioPinWrite(int pinNum, bool state)
            {
                // Each reg controls 32 GPIO pin
                _ASSERT(pinNum < 32 && pinNum > 0 && "Pin number should be in the range [0,31]");

                // true -> HIGH
                // false -> LOW
                WRITE_REGISTER_NOFENCE_ULONG(state ? GpioReg->Set : GpioReg->Clear, 1 << pinNum);
            }

            __inline static bool GpioPinRead(int pinNum)
            {
                // Each reg controls 32 GPIO pin
                _ASSERTE(pinNum < 32 && pinNum > 0 && "Pin number should be in the range [0,31]");
                return (READ_REGISTER_NOFENCE_ULONG(GpioReg->Level) & (1 << pinNum)) > 0;
            }

            __inline static ULONG GpioBankRead(int bank)
            {
                _ASSERTE(bank == 0 && "Only bank 0 is supported");
                return READ_REGISTER_NOFENCE_ULONG(GpioReg->Level);
            }

            __inline static void BenchmarkGpio(int pinNum, const int numSamples)
            {
                HpcTimer timer;

                GpioPinSetDir(pinNum, DIR_Output);

                timer.Start();
                for (int i = 0; i < numSamples; ++i)
                {
                    GpioPinWrite(pinNum, (i & 1) > 0);
                }
                timer.Stop();

                LogInfo("GPIO Writes/Second = %f", timer.OperationsPerSecond(numSamples));

                GpioPinSetDir(pinNum, DIR_Input);

                timer.Start();
                for (int i = 0; i < numSamples; ++i)
                {
                    bool state = GpioPinRead(pinNum);
                }
                timer.Stop();

                LogInfo("GPIO Reads/Second = %f", timer.OperationsPerSecond(numSamples));
            }
        };
    }
}