/*
	Copyright 2015 Muhamad Lotfy

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#pragma once

#define BCM_CPU_PERIPH_BASE			0x3F000000
#define BCM_BUS_PERIPH_BASE			0x7E000000
#define BCM_CPU_DRAM_BASE			0
#define BCM_BUS_DRAM_BASE			0xC0000000

#define BCM_CPU_TO_BUS_PERIPH_ADDR(x)	(BCM_BUS_PERIPH_BASE + ((ULONG)(x) - BCM_CPU_PERIPH_BASE))
#define BCM_CPU_TO_BUS_DRAM_ADDR(x)		(BCM_BUS_DRAM_BASE + ((ULONG)(x) - BCM_CPU_DRAM_BASE))
