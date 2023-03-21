/**************************************************************************

Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/
#include "DeviceSupport.h"

BRAND_STRUCT mBrandingTable[] = {
#ifndef NO_BRANDING_SUPPORT

#ifndef NO_82571_SUPPORT
    {0x8086, 0x0000, 0x105E, 0x0000, L"Intel(R) PRO/1000 PT Dual Port Network Connection"},
    {0x8086, 0x8086, 0x105E, 0x005E, L"Intel(R) PRO/1000 PT Dual Port Server Connection"},
    {0x8086, 0x8086, 0x105E, 0x115E, L"Intel(R) PRO/1000 PT Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x105E, 0x125E, L"Intel(R) PRO/1000 PT Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x105E, 0x135E, L"Intel(R) PRO/1000 PT Dual Port Server Adapter"},
    {0x8086, 0x103C, 0x105E, 0x704E, L"Intel(R) PRO/1000 PT Dual Port Server Adapter"},
    {0x8086, 0x103C, 0x105E, 0x7044, L"HP NC360T PCIe DP Gigabit Server Adapter"},
    {0x8086, 0x0000, 0x105F, 0x0000, L"Intel(R) PRO/1000 PF Dual Port Server Adapter"},
    {0x8086, 0x103C, 0x105F, 0x704F, L"Intel(R) PRO/1000 PF Dual Port Server Adapter"},
    {0x8086, 0x0000, 0x1060, 0x0000, L"Intel(R) PRO/1000 PB Dual Port Server Connection"},
    {0x8086, 0x0000, 0x10A4, 0x0000, L"Intel(R) PRO/1000 PT Quad Port Server Adapter"},
    {0x8086, 0x108E, 0x10D5, 0xF1BC, L"Intel(R) Gigabit PT Quad Port Server ExpressModule"},
    {0x8086, 0x0000, 0x10A5, 0x0000, L"Intel(R) PRO/1000 PF Quad Port Server Adapter"},
    {0x8086, 0x0000, 0x10BC, 0x0000, L"Intel(R) PRO/1000 PT Quad Port LP server Adapter"},
    {0x8086, 0x103C, 0x10BC, 0x704B, L"HP NC364T PCIe Quad Port Gigabit Server Adapter"},
    {0x8086, 0x8086, 0x107D, 0x1082, L"Intel(R) PRO/1000 PT Server Adapter"},
    {0x8086, 0x8086, 0x107D, 0x1092, L"Intel(R) PRO/1000 PT Server Adapter"},
    {0x8086, 0x8086, 0x107D, 0x1084, L"Intel(R) PRO/1000 PT Server Adapter"},
    {0x8086, 0x0000, 0x107D, 0x0000, L"Intel(R) PRO/1000 PT Network Connection"},
    {0x8086, 0x0000, 0x107E, 0x0000, L"Intel(R) PRO/1000 PF Network Connection"},
    {0x8086, 0x8086, 0x107E, 0x1084, L"Intel(R) PRO/1000 PF Server Adapter"},
    {0x8086, 0x8086, 0x107E, 0x1094, L"Intel(R) PRO/1000 PF Server Adapter"},
    {0x8086, 0x8086, 0x107E, 0x1085, L"Intel(R) PRO/1000 PF Server Adapter"},
    {0x8086, 0x0000, 0x107F, 0x0000, L"Intel(R) PRO/1000 PB Server Connection"},
    {0x8086, 0x0000, 0x10B9, 0x0000, L"Intel(R) PRO/1000 PT Desktop Adapter"},
    {0x8086, 0x103C, 0x10B9, 0x704A, L"HP NC110T PCIe Gigabit Server Adapter"},
    {0x8086, 0x0000, 0x108B, 0x0000, L"Intel(R) PRO/1000 PM Network Connection"},
    {0x8086, 0x0000, 0x108C, 0x0000, L"Intel(R) PRO/1000 PM Network Connection"},
    {0x8086, 0x0000, 0x109A, 0x0000, L"Intel(R) PRO/1000 PL Network Connection"},
#ifndef NO_82574_SUPPORT
    {0x8086, 0x0000, 0x10D3, 0x0000, L"Intel(R) 82574L Gigabit Network Connection"},
    {0x8086, 0x8086, 0x10D3, 0xA01F, L"Intel(R) Gigabit CT Desktop Adapter"},
    {0x8086, 0x8086, 0x10D3, 0x0001, L"Intel(R) Gigabit CT2 Desktop Adapter"},
    {0x8086, 0x103C, 0x10D3, 0x1785, L"HP NC112i 1-port Ethernet Server Adapter"},
    {0x8086, 0x0000, 0x10F6, 0x0000, L"Intel(R) 82574L Gigabit Network Connection"},
    {0x8086, 0x0000, 0x150C, 0x0000, L"Intel(R) 82583V Gigabit Network Connection"},
#endif /* NO_82574_SUPPORT */
#endif /* NO_82571_SUPPORT */

#ifndef NO_80003ES2LAN_SUPPORT
    {0x8086, 0x0000, 0x1096, 0x0000, L"Intel(R) PRO/1000 EB Network Connection "},
    {0x8086, 0x10F1, 0x1096, 0x2692, L"Intel(R) PRO/1000 EB1 Network Connection "},
    {0x8086, 0x1734, 0x1096, 0x10A8, L"Intel(R) PRO/1000 EB1 Network Connection "},
    {0x8086, 0x0000, 0x1098, 0x0000, L"Intel(R) PRO/1000 EB Backplane Connection "},
    {0x8086, 0x0000, 0x10BA, 0x0000, L"Intel(R) PRO/1000 EB1 Network Connection "},
    {0x8086, 0x0000, 0x10BB, 0x0000, L"Intel(R) PRO/1000 EB1 Backplane Connection "},
#endif /* NO_80003ES2LAN_SUPPORT */

#ifndef NO_82575_SUPPORT
#ifndef NO_82576_SUPPORT
    {0x8086, 0x0000, 0x10C9, 0x0000, L"Intel(R) 82576 Gigabit Dual Port Network Connection"},
    {0x8086, 0x8086, 0x10C9, 0x0000, L"Intel(R) Gigabit ET Dual Port Server Adapter"},
    {0x8086, 0x0000, 0x10E6, 0x0000, L"Intel(R) 82576 Gigabit Dual Port Network Connection"},
    {0x8086, 0x8086, 0x10E6, 0x0000, L"Intel(R) Gigabit EF Dual Port Server Adapter"},
    {0x8086, 0x0000, 0x10E7, 0x0000, L"Intel(R) 82576 Gigabit Dual Port Server Network Connection"},
    {0x8086, 0x8086, 0x10E8, 0xA02B, L"Intel(R) Gigabit ET Quad Port Server Adapter"},
    {0x8086, 0x8086, 0x10E8, 0xA02C, L"Intel(R) Gigabit ET Quad Port Server Adapter"},
    {0x8086, 0x8086, 0x1526, 0xA05C, L"Intel(R) Gigabit ET2 Quad Port Server Adapter"},
    {0x8086, 0x8086, 0x1526, 0xA06C, L"Intel(R) Gigabit ET2 Quad Port Server Adapter"},
    {0x8086, 0x0000, 0x150A, 0x0000, L"Intel(R) 82576NS Gigabit Ethernet Controller"},
    {0x8086, 0x0000, 0x1518, 0x0000, L"Intel(R) 82576NS SerDes Gigabit Ethernet Controller"},
    {0x8086, 0x0000, 0x150D, 0xA10C, L"Intel(R) Gigabit ET Quad Port Mezzanine Card"},
#endif /* NO_82576_SUPPORT */
    {0x8086, 0x0000, 0x10A7, 0x0000, L"Intel(R) 82575EB Gigabit Network Connection"},
    {0x8086, 0x0000, 0x10A9, 0x0000, L"Intel(R) 82575EB Gigabit Backplane Connection"},
    {0x8086, 0x0000, 0x10D6, 0x0000, L"Intel(R) Gigabit VT Quad Port Server Adapter"},
#endif /* NO_82575_SUPPORT */

#ifndef NO_82580_SUPPORT
    {0x8086, 0x0000, 0x150E, 0x0000, L"Intel(R) 82580 Gigabit Network Connection"},
    {0x8086, 0x103C, 0x150E, 0x1780, L"HPE NC365T PCIe Quad Port Gigabit Server Adapter"},
    {0x8086, 0x0000, 0x150F, 0x0000, L"Intel(R) 82580 Gigabit Fiber Network Connection"},
    {0x8086, 0x0000, 0x1510, 0x0000, L"Intel(R) 82580 Gigabit Backplane Connection"},
    {0x8086, 0x0000, 0x1511, 0x0000, L"Intel(R) 82580 Gigabit SFP Connection"},
    {0x8086, 0x8086, 0x150E, 0x12A1, L"Intel(R) Ethernet Server Adapter I340-T4"},
    {0x8086, 0x8086, 0x150E, 0x12A2, L"Intel(R) Ethernet Server Adapter I340-T4"},
    {0x8086, 0x8086, 0x1516, 0x12B1, L"Intel(R) Ethernet Server Adapter I340-T2"},
    {0x8086, 0x8086, 0x1516, 0x12B2, L"Intel(R) Ethernet Server Adapter I340-T2"},
    {0x8086, 0x8086, 0x1527, 0x0001, L"Intel(R) Ethernet Server Adapter I340-F4"},
    {0x8086, 0x8086, 0x1527, 0x0002, L"Intel(R) Ethernet Server Adapter I340-F4"},
    {0x8086, 0x0000, 0x0438, 0x0000, L"Intel(R) DH8900CC Series Gigabit Network Connection"},
    {0x8086, 0x0000, 0x043a, 0x0000, L"Intel(R) DH8900CC Series Gigabit Fiber Network Connection"},
    {0x8086, 0x0000, 0x043c, 0x0000, L"Intel(R) DH8900CC Series Gigabit Backplane Network Connection"},
    {0x8086, 0x0000, 0x0440, 0x0000, L"Intel(R) DH8900CC Series Gigabit SFP Network Connection"},
#endif /* NO_82580_SUPPORT */

    {0x8086, 0x0000, 0x1521, 0x0000, L"Intel(R) I350 Gigabit Network Connection"},
    {0x8086, 0x0000, 0x1522, 0x0000, L"Intel(R) I350 Gigabit Fiber Network Connection"},
    {0x8086, 0x0000, 0x1523, 0x0000, L"Intel(R) I350 Gigabit Backplane Connection"},
    {0x8086, 0x0000, 0x1524, 0x0000, L"Intel(R) I350 Gigabit Connection"},
    {0x8086, 0x8086, 0x1521, 0x0001, L"Intel(R) Ethernet Server Adapter I350-T4"},
    {0x8086, 0x8086, 0x1521, 0x0002, L"Intel(R) Ethernet Server Adapter I350-T2"},
    {0x8086, 0x8086, 0x1521, 0x00A1, L"Intel(R) Ethernet Server Adapter I350-T4"},
    {0x8086, 0x8086, 0x1521, 0x00A2, L"Intel(R) Ethernet Server Adapter I350-T2"},
    {0x8086, 0x8086, 0x1521, 0x0003, L"Intel(R) Ethernet Network Adapter I350-T4 for OCP NIC 3.0"},
    {0x8086, 0x8086, 0x1521, 0x00A3, L"Intel(R) Ethernet Network Adapter I350-T4 for OCP NIC 3.0"},
    {0x8086, 0x8086, 0x1522, 0x0003, L"Intel(R) Ethernet Server Adapter I350-F4"},
    {0x8086, 0x8086, 0x1522, 0x00A3, L"Intel(R) Ethernet Server Adapter I350-F4"},
    {0x8086, 0x8086, 0x1522, 0x0004, L"Intel(R) Ethernet Server Adapter I350-F2"},
    {0x8086, 0x8086, 0x1522, 0x00A4, L"Intel(R) Ethernet Server Adapter I350-F2"},

    {0x8086, 0x8086, 0x1521, 0x5002, L"Intel(R) Gigabit 2P I350-t Adapter"},
    {0x8086, 0x8086, 0x1521, 0x5001, L"Intel(R) Gigabit 4P I350-t Adapter"},
    {0x8086, 0x8086, 0x1521, 0x5003, L"Intel(R) Ethernet 1G 4P I350-t OCP"},

    {0x8086, 0x8086, 0x1523, 0x1F52, L"Intel(R) Gigabit 4P I350-t Mezz"},

    {0x8086, 0x8086, 0x1522, 0x0005, L"Intel(R) Ethernet Server Adapter I350-F1"},
    {0x8086, 0x8086, 0x1521, 0x00A1, L"Intel(R) Ethernet Server Adapter I350-T4"},
    {0x8086, 0x8086, 0x1521, 0x00A2, L"Intel(R) Ethernet Server Adapter I350-T2"},
    {0x8086, 0x8086, 0x1521, 0x00AA, L"Intel(R) Ethernet Network Adapter I350-T4 for OCP NIC 3.0"},
    {0x8086, 0x8086, 0x1521, 0x1521, L"Intel(R) I350 Gigabit Network Connection"},

    // Dell
    {0x8086, 0x1028, 0x1521, 0x1F60, L"Intel(R) Gigabit 4P I350-t rNDC"},
    {0x8086, 0x1028, 0x1521, 0x1F62, L"Intel(R) Gigabit 4P X540/I350 rNDC"},
    {0x8086, 0x1028, 0x1521, 0x1F73, L"Intel(R) Gigabit 4P X520/I350 rNDC"},
    {0x8086, 0x1028, 0x1521, 0x04cf, L"Intel(R) Gigabit 2P I350-t LOM"},
    {0x8086, 0x1028, 0x1521, 0x1F9A, L"Intel(R) Gigabit 4P X710/I350 rNDC"},
    {0x8086, 0x1028, 0x1521, 0x0602, L"Intel(R) Gigabit 2P I350-t LOM"},
    {0x8086, 0x1028, 0x1521, 0x075A, L"Intel(R) Gigabit I350-t LOM"},
    {0x8086, 0x1028, 0x1523, 0x0660, L"Intel(R) Gigabit 2P I350 LOM"},
    {0x8086, 0x1028, 0x1521, 0x0693, L"Intel(R) Gigabit 2P I350-t LOM"},
    {0x8086, 0x1028, 0x1521, 0x06E2, L"Intel(R) Gigabit 2P I350-t LOM"},
    {0x8086, 0x1028, 0x1523, 0x1F9B, L"Intel(R) Gigabit 4P I350 bNDC"},
    {0x8086, 0x1028, 0x1521, 0x1FAA, L"Intel(R) Gigabit 4P X550/I350 rNDC"},
    {0x8086, 0x1028, 0x1521, 0x0757, L"Intel(R) Gigabit I350-t LOM"},

    // Cisco
    {0x8086, 0x1137, 0x1521, 0x023E, L"Cisco 1GigE I350 LOM"},

    // HP branded devices
    {0x8086, 0x103C, 0x1521, 0x2226, L"HPE Ethernet 1Gb 1-port 364i Adapter"},
    {0x8086, 0x103C, 0x1521, 0x337F, L"HPE Ethernet 1Gb 2-port 361i Adapter"},
    {0x8086, 0x103C, 0x1521, 0x3380, L"HPE Ethernet 1Gb 4-port 366i Adapter"},
    {0x8086, 0x103C, 0x1521, 0x339E, L"HPE Ethernet 1Gb 2-port 361T Adapter"},
    {0x8086, 0x103C, 0x1521, 0x17D1, L"HPE Ethernet 1Gb 4-port 366FLR Adapter"},
    {0x8086, 0x103C, 0x1521, 0x2003, L"HPE Ethernet 1Gb 2-port 367i Adapter"},
    {0x8086, 0x103C, 0x1521, 0x8157, L"HPE Ethernet 1Gb 4-port 366T Adapter"},
    {0x8086, 0x103C, 0x1523, 0x339F, L"HPE Ethernet 1Gb 4-port 366M Adapter"},
    {0x8086, 0x103C, 0x1523, 0x18D1, L"HPE Ethernet 1Gb 2-port 361FLB Adapter"},
    {0x8086, 0x103C, 0x1523, 0x1989, L"HPE Ethernet 1Gb 2-port 363i Adapter"},
    {0x8086, 0x1590, 0x1521, 0x00FF, L"HPE Ethernet 1Gb 4-port 366i Communication Board"},

    // Sun/Oracle adapters&modules
    {0x8086, 0x108E, 0x1522, 0x7B19, L"Sun Dual Port GbE PCIe 2.0 Low Profile Adapter, MMF"},
    {0x8086, 0x108E, 0x1522, 0x7B17, L"Sun Quad Port GbE PCIe 2.0 ExpressModule, MMF"},
    {0x8086, 0x108E, 0x1521, 0x7B16, L"Sun Quad Port GbE PCIe 2.0 ExpressModule, UTP"},
    {0x8086, 0x108E, 0x1521, 0x7B18, L"Sun Quad Port GbE PCIe 2.0 Low Profile Adapter, UTP"},

    // Quanta
    {0x8086, 0x152D, 0x1521, 0x899C, L"Quanta Dual Port 1G BASE-T Mezzanine"},
    {0x8086, 0x152D, 0x1521, 0x89AE, L"Quanta Dual Port 1G BASE-T Mezzanine"},
    {0x8086, 0x152D, 0x1521, 0x89B6, L"Quanta Dual Port 1G BASE-T Mezzanine"},

    // Lenovo
    {0x8086, 0x17AA, 0x1521, 0x1074, L"Lenovo ThinkServer I350-T4 AnyFabric"},
    {0x8086, 0x17AA, 0x1521, 0x4016, L"Intel(R) I350 Gigabit Network Connection"},

    {0x8086, 0x8086, 0x1521, 0x4017, L"Intel(R) Ethernet Network Adapter I350-T4 for OCP NIC 3.0"},

    {0x8086, 0x0000, 0x1533, 0x0000, L"Intel(R) I210 Gigabit  Network Connection"},
    {0x8086, 0x0000, 0x1536, 0x0000, L"Intel(R) I210 Gigabit  Fiber Network Connection"},
    {0x8086, 0x0000, 0x1537, 0x0000, L"Intel(R) I210 Gigabit  Backplane Connection"},
    {0x8086, 0x0000, 0x1538, 0x0000, L"Intel(R) I210 Gigabit  Network Connection"},
    {0x8086, 0x0000, 0x157B, 0x0000, L"Intel(R) I210 Gigabit Network Connection"},
    {0x8086, 0x0000, 0x157C, 0x0000, L"Intel(R) I210 Gigabit Backplane Connection"},

    {0x8086, 0x8086, 0x1533, 0x0001, L"Intel(R) Ethernet Server Adapter I210-T1"},
    {0x8086, 0x8086, 0x1533, 0x0002, L"Intel(R) Ethernet Server Adapter I210-T1"},
    {0x8086, 0x103C, 0x1533, 0x0003, L"Intel(R) Ethernet I210-T1 GbE NIC"},
    {0x8086, 0x1028, 0x1533, 0x0B35, L"Intel(R) I210 Gigabit Network Connection"},
    {0x8086, 0x17AA, 0x1533, 0x1059, L"Intel(R) I210 Gigabit Network Connection"},
    {0x8086, 0x17AA, 0x1533, 0x404D, L"Intel(R) I210 PCIe 1Gb 1-Port RJ45 LOM"},

    {0x8086, 0x0000, 0x1539, 0x0000, L"Intel(R) I211 Gigabit  Network Connection"},

    // Intel(R) Ethernet Connection I219-LM
    {0x8086, 0x0000, 0x0DC5, 0x0000, L"Intel(R) Ethernet Connection (23) I219-LM"},
    {0x8086, 0x0000, 0x0DC6, 0x0000, L"Intel(R) Ethernet Connection (23) I219-V"},
    {0x8086, 0x0000, 0x0DC7, 0x0000, L"Intel(R) Ethernet Connection (22) I219-LM"},
    {0x8086, 0x0000, 0x0DC8, 0x0000, L"Intel(R) Ethernet Connection (22) I219-V"},
    {0x8086, 0x0000, 0x1A1C, 0x0000, L"Intel(R) Ethernet Connection (17) I219-LM"},
    {0x8086, 0x0000, 0x1A1D, 0x0000, L"Intel(R) Ethernet Connection (17) I219-V"},
    {0x8086, 0x0000, 0x1A1E, 0x0000, L"Intel(R) Ethernet Connection (16) I219-LM"},
    {0x8086, 0x0000, 0x1A1F, 0x0000, L"Intel(R) Ethernet Connection (16) I219-V"},
    {0x8086, 0x0000, 0x15F4, 0x0000, L"Intel(R) Ethernet Connection (15) I219-LM"},
    {0x8086, 0x0000, 0x15F5, 0x0000, L"Intel(R) Ethernet Connection (15) I219-V"},
    {0x8086, 0x0000, 0x15F9, 0x0000, L"Intel(R) Ethernet Connection (14) I219-LM"},
    {0x8086, 0x0000, 0x15FA, 0x0000, L"Intel(R) Ethernet Connection (14) I219-V"},
    {0x8086, 0x0000, 0x15FB, 0x0000, L"Intel(R) Ethernet Connection (13) I219-LM"},
    {0x8086, 0x0000, 0x15FC, 0x0000, L"Intel(R) Ethernet Connection (13) I219-V"},
    {0x8086, 0x0000, 0x0D53, 0x0000, L"Intel(R) Ethernet Connection (12) I219-LM"},
    {0x8086, 0x0000, 0x0D55, 0x0000, L"Intel(R) Ethernet Connection (12) I219-V"},
    {0x8086, 0x0000, 0x0D4C, 0x0000, L"Intel(R) Ethernet Connection (11) I219-LM"},
    {0x8086, 0x0000, 0x0D4D, 0x0000, L"Intel(R) Ethernet Connection (11) I219-V"},
    {0x8086, 0x0000, 0x0D4C, 0x0000, L"Intel(R) Ethernet Connection (10) I219-LM"},
    {0x8086, 0x0000, 0x0D4F, 0x0000, L"Intel(R) Ethernet Connection (10) I219-V"},
    {0x8086, 0x0000, 0x15E1, 0x0000, L"Intel(R) Ethernet Connection (9) I219-LM"},
    {0x8086, 0x0000, 0x15E2, 0x0000, L"Intel(R) Ethernet Connection (9) I219-V"},
    {0x8086, 0x0000, 0x15DF, 0x0000, L"Intel(R) Ethernet Connection (8) I219-LM"},
    {0x8086, 0x0000, 0x15E0, 0x0000, L"Intel(R) Ethernet Connection (8) I219-V"},
    {0x8086, 0x0000, 0x15BB, 0x0000, L"Intel(R) Ethernet Connection (7) I219-LM"},
    {0x8086, 0x0000, 0x15BC, 0x0000, L"Intel(R) Ethernet Connection (7) I219-V"},
    {0x8086, 0x0000, 0x15BD, 0x0000, L"Intel(R) Ethernet Connection (6) I219-LM"},
    {0x8086, 0x0000, 0x15BE, 0x0000, L"Intel(R) Ethernet Connection (6) I219-V"},
    {0x8086, 0x0000, 0x15E3, 0x0000, L"Intel(R) Ethernet Connection (5) I219-LM"},
    {0x8086, 0x0000, 0x15D6, 0x0000, L"Intel(R) Ethernet Connection (5) I219-V"},
    {0x8086, 0x0000, 0x15D7, 0x0000, L"Intel(R) Ethernet Connection (4) I219-LM"},
    {0x8086, 0x0000, 0x15D8, 0x0000, L"Intel(R) Ethernet Connection (4) I219-V"},
    {0x8086, 0x0000, 0x15B9, 0x0000, L"Intel(R) Ethernet Connection (3) I219-LM"},
    {0x8086, 0x0000, 0x15B7, 0x0000, L"Intel(R) Ethernet Connection (2) I219-LM"},
    {0x8086, 0x0000, 0x15B8, 0x0000, L"Intel(R) Ethernet Connection (2) I219-V"},
    {0x8086, 0x0000, 0x156F, 0x0000, L"Intel(R) Ethernet Connection I219-LM"},
    {0x8086, 0x0000, 0x1570, 0x0000, L"Intel(R) Ethernet Connection I219-V"},

    {0x8086, 0x0000, 0x1F40, 0x0000, L"Intel(R) Ethernet Connection I354 1.0 GbE Backplane"},
    {0x8086, 0x0000, 0x1F41, 0x0000, L"Intel(R) Ethernet Connection I354"},
    {0x8086, 0x1028, 0x1F40, 0x05F1, L"Intel(R) Ethernet Connection I354 1.0 GbE Backplane"},

    {0x8086, 0x0000, 0x1F45, 0x0000, L"Intel(R) Ethernet Connection I354 2.5 GbE Backplane"},

#else /* N0_BRANDING_SUPPORT */
    {0x8086, 0x8086, 0x0000, 0x0000, L"Intel(R) PRO/1000 Network Connection"},
#endif /* NO_BRANDING_SUPPORT */
    {INVALID_VENDOR_ID, INVALID_SUBVENDOR_ID, INVALID_DEVICE_ID, INVALID_SUBSYSTEM_ID, L""},
};

UINTN mBrandingTableSize = (sizeof (mBrandingTable) / sizeof (mBrandingTable[0]));
