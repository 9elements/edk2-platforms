/**************************************************************************

Copyright (c) 2012 - 2020, Intel Corporation. All rights reserved.

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


BRAND_STRUCT  mBrandingTable[] = {
    {0x8086, 0x0000, 0x0000, 0x0000, L"Intel(R) 10 Gigabit Network Connection"},
    {0x8086, 0x8086, 0x10C6, 0xA15F, L"Intel(R) 10 Gigabit XF SR Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10C6, 0xA19F, L"Intel(R) 10 Gigabit AF LRM Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10C6, 0xA09F, L"Intel(R) 10 Gigabit AF LRM Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10C6, 0xA05F, L"Intel(R) 10 Gigabit XF SR Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10C6, 0xA11F, L"Intel(R) 10 Gigabit LR AF Dual Port Server Adapter"},
    {0x8086, 0x0000, 0x10C6, 0x0000, L"Intel(R) 82598EB 10 Gigabit AF Network Connection"},
    {0x8086, 0x8086, 0x10C6, 0xA01F, L"Intel(R) 10 Gigabit AF LR Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0xA16F, L"Intel(R) 10 Gigabit XF SR Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0xA11F, L"Intel(R) 10 Gigabit AF LR Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0xA01F, L"Intel(R) 10 Gigabit AF LR Server Adapter"},
    {0x8086, 0x0000, 0x10C7, 0x0000, L"Intel(R) 82598EB 10 Gigabit AF Network Connection"},
    {0x8086, 0x8086, 0x10C7, 0xA15F, L"Intel(R) 10 Gigabit XF SR Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0xA19F, L"Intel(R) 10 Gigabit AF LRM Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0xA09F, L"Intel(R) 10 Gigabit AF LRM Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0xA05F, L"Intel(R) 10 Gigabit XF SR Server Adapter"},
    {0x8086, 0x8086, 0x10C7, 0x0A6F, L"Intel(R) 10 Gigabit AF SR Server Adapter"},
    {0x8086, 0x8086, 0x10C8, 0x0000, L"Intel(R) 10 Gigabit AT Server Adapter"},
    {0x8086, 0x8086, 0x10B6, 0x0000, L"Intel(R) 82598EB 10 Gigabit KX4 Network Connection"},
    {0x8086, 0x0000, 0x10DB, 0x0000, L"Intel(R) 82598EB 10 Gigabit Dual Port Network Connection"},
    {0x8086, 0x8086, 0x10F1, 0xA20F, L"Intel(R) 10 Gigabit AF DA Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10F1, 0xA21F, L"Intel(R) 10 Gigabit AF DA Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10EC, 0xA01F, L"Intel(R) 10 Gigabit CX4 Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x10EC, 0xA11F, L"Intel(R) 10 Gigabit CX4 Dual Port Server Adapter"},
    {0x8086, 0x8086, 0x150B, 0x0000, L"Intel(R) 10 Gigabit AT Server Adapter"},
    {0x8086, 0x8086, 0x150B, 0xA10C, L"Intel(R) 10 Gigabit AT2 Server Adapter"},
    {0x8086, 0x8086, 0x150B, 0xA12C, L"Intel(R) 10 Gigabit AT2 Server Adapter"},
    {0x8086, 0x8086, 0x150B, 0xA11C, L"Intel(R) 10 Gigabit AT2 Server Adapter"},
    {0x8086, 0x0000, 0x10FB, 0x0000, L"Intel(R) 82599 10 Gigabit Dual Port Network Connection"},
    {0x8086, 0x0000, 0x10FC, 0x0000, L"Intel(R) 82599 10 Gigabit Dual Port Network Connection"},

    {0x8086, 0x8086, 0x10FB, 0x0002, L"Intel(R) Ethernet Server Adapter X520-DA2"},
    {0x8086, 0x8086, 0x10FB, 0x0004, L"Intel(R) Ethernet Server Adapter X520-DA2"},
    {0x8086, 0x8086, 0x10FB, 0x0005, L"Intel(R) Ethernet Server Adapter X520-LR1"},
    {0x8086, 0x8086, 0x10FB, 0x0007, L"Intel(R) Ethernet Server Adapter X520-LR1"},
    {0x8086, 0x8086, 0x10FB, 0x0006, L"Intel(R) Ethernet Server Adapter X520-1"},
    {0x8086, 0x8086, 0x10FB, 0x000A, L"Intel(R) Ethernet Server Adapter X520-1"},
    {0x8086, 0x8086, 0x10FB, 0x0003, L"Intel(R) Ethernet Server Adapter X520-2"},
    {0x8086, 0x8086, 0x10FB, 0x000C, L"Intel(R) Ethernet Server Adapter X520-2"},
    {0x8086, 0x8086, 0x10FB, 0x000D, L"Intel(R) Ethernet Server Adapter X520-1OCP"},
    {0x8086, 0x8086, 0x10FB, 0x7A11, L"Intel(R) Ethernet Server Adapter X520-2"},
    {0x8086, 0x8086, 0x10FB, 0x7A12, L"Intel(R) Ethernet Server Adapter X520-2"},
    {0x8086, 0x108E, 0x10FB, 0x7B11, L"Intel(R) Ethernet Server Adapter X520-2"},
    {0x8086, 0x108E, 0x1507, 0x7B10, L"Intel(R) Ethernet ExpressModule X520-P2"},
    {0x8086, 0x8086, 0x10FB, 0x0470, L"Intel(R) Ethernet 10GSFP+ DP Embedded CNA X520-2"},
    {0x8086, 0x8086, 0x10FB, 0x0008, L"Intel(R) Ethernet OCP Server Adapter X520-2"},

    {0x8086, 0x103C, 0x10F8, 0x18D0, L"HPE Ethernet 10Gb 2-port 560FLB Adapter"},
    {0x8086, 0x103C, 0x10F8, 0x17D2, L"HPE Ethernet 10Gb 2-port 560M Adapter"},
    {0x8086, 0x103C, 0x10FB, 0x17D3, L"HPE Ethernet 10Gb 2-port 560SFP+ Adapter"},
    {0x8086, 0x103C, 0x10FB, 0x17D0, L"HPE Ethernet 10Gb 2-port 560FLR-SFP+ Adapter"},
    {0x8086, 0x103C, 0x1528, 0x211A, L"HPE Ethernet 10Gb 2-port 561T Adapter"},
    {0x8086, 0x103C, 0x10FB, 0x211B, L"HPE Ethernet 10Gb 1-port P560FLR-SFP+ Adapter"},
    {0x8086, 0x103C, 0x10FB, 0x2147, L"HP Ethernet 10Gb 1-port 561i Adapter"},
    {0x8086, 0x103C, 0x10FB, 0x2159, L"HPE Ethernet 10Gb 2-port 562i Adapter"},


    {0x8086, 0x152D, 0x10FB, 0x897E, L"Quanta Dual Port 10G SFP+ Mezzanine"},
    {0x8086, 0x152D, 0x10FB, 0x89B9, L"Quanta Dual Port 10G SFP+ Mezzanine"},
    {0x8086, 0x152D, 0x1557, 0x89B8, L"Quanta Single Port 10G SFP+ Mezzanine"},

    {0x8086, 0x17AA, 0x10FB, 0x1071, L"Lenovo ThinkServer X520-2 AnyFabric"},
    {0x8086, 0x17AA, 0x1528, 0x1073, L"Lenovo ThinkServer X540-T2 AnyFabric"},

    {0x8086, 0x8086, 0x154D, 0x7B11, L"Intel(R) Ethernet 10G 2P X520 Adapter"},


    {0x8086, 0x8086, 0x1558, 0x011A, L"Intel(R) Ethernet Converged Network Adapter X520-Q1"},
    {0x8086, 0x8086, 0x1558, 0x011B, L"Intel(R) Ethernet Converged Network Adapter X520-Q1"},

    {0x8086, 0x0000, 0x1557, 0x0000, L"Intel(R) 82599 10 Gigabit Network Connection"},
    {0x8086, 0x8086, 0x1557, 0x0001, L"Intel(R) Ethernet OCP Server Adapter X520-1"},

    {0x8086, 0x0000, 0x1529, 0x0000, L"Intel(R) 82599 10 Gigabit Dual Port Network Connection with FCoE"},

    {0x8086, 0x8086, 0x1514, 0x000B, L"Intel(R) Ethernet X520 10GbE Dual Port KX4 Mezz"},
    {0x8086, 0x8086, 0x10F9, 0x0000, L"Intel(R) 82599 10 Gigabit CX4 Dual Port Network Connection"},
    {0x8086, 0x0000, 0x10F8, 0x0000, L"Intel(R) 82599 10 Gigabit Dual Port Backplane Connection"},
    {0x8086, 0x0000, 0x152A, 0x0000, L"Intel(R) 82599 10 Gigabit Dual Port Backplane Connection with FCoE"},
    {0x8086, 0x8086, 0x10F8, 0x000C, L"Intel(R) Ethernet X520 10GbE Dual Port KX4-KR Mezz"},
    {0x8086, 0x1028, 0x10F8, 0x1F63, L"Intel(R) Ethernet 10G 2P X520-k bNDC"},
    {0x8086, 0x1028, 0x10FB, 0x1F72, L"Intel(R) Ethernet 10G 4P X520/I350 rNDC"},
    {0x8086, 0x1028, 0x10FB, 0x06EE, L"Intel(R) Ethernet 10G X520 LOM"},

    {0x8086, 0x8086, 0x151C, 0x0000, L"Intel(R) 82599 10 Gigabit TN Dual Port Network Connection"},
    {0x8086, 0x8086, 0x151C, 0xA02C, L"Intel(R) Ethernet Server Adapter X520-T2"},
    {0x8086, 0x8086, 0x151C, 0xA21C, L"Intel(R) Ethernet Server Adapter X520-T2"},
    {0x8086, 0x8086, 0x151C, 0xA03C, L"Intel(R) Ethernet Server Adapter X520-T2"},
    {0x8086, 0x108E, 0x151C, 0x7B13, L"Sun Dual 10GBASE-T LP"},
    {0x8086, 0x8086, 0x10F7, 0x000D, L"Intel(R) Ethernet Mezzanine Adapter X520-KX4-2"},
    {0x8086, 0x8086, 0x10F7, 0x0000, L"Intel(R) 10 Gigabit BR KX4 Dual Port Network Connection"},
    {0x8086, 0x0000, 0x1528, 0x0000, L"Intel(R) Ethernet Controller 10 Gigabit X540-AT2"},
    {0x8086, 0x8086, 0x1528, 0x00A2, L"Intel(R) Ethernet Converged Network Adapter X540-T1"},
    {0x8086, 0x8086, 0x1528, 0x0002, L"Intel(R) Ethernet Converged Network Adapter X540-T1"},
    {0x8086, 0x1137, 0x1528, 0x00BF, L"Intel(R) Ethernet Converged Network Adapter X540-T2"},
    {0x8086, 0x8086, 0x1528, 0x0001, L"Intel(R) Ethernet Converged Network Adapter X540-T2"},
    {0x8086, 0x8086, 0x1528, 0x001A, L"Intel(R) Ethernet Converged Network Adapter X540-T2"},
    {0x8086, 0x1028, 0x1528, 0x1F61, L"Intel(R) Ethernet 10G 4P X540/I350 rNDC"},
    {0x8086, 0x8086, 0x1528, 0x5003, L"Intel(R) Ethernet 10G 2P X540-t Adapter"},
    {0x8086, 0x8086, 0x1528, 0x5004, L"Intel(R) Ethernet 10G 2P X540-t Adapter"},
    {0x8086, 0x108E, 0x1528, 0x7B15, L"Sun Dual Port 10 GbE PCIe 2.0 Low Profile Adapter, Base-T"},
    {0x8086, 0x108E, 0x1528, 0x7B14, L"Sun Dual Port 10 GbE PCIe 2.0 ExpressModule, Base-T"},

    {0x8086, 0x8086, 0x1528, 0x0471, L"Intel(R) Ethernet 10GBT DP Embedded CNA X540-T2"},
    {0x8086, 0x103C, 0x1528, 0x192D, L"HPE Ethernet 10Gb 2-port 561FLR-T Adapter"},

    {0x8086, 0x152D, 0x1528, 0x89B7, L"Quanta Dual Port 10G BASE-T Mezzanine"},
    {0x8086, 0x152D, 0x1528, 0x89AE, L"Quanta Dual Port 10G BASE-T Mezzanine"},

    {0x8086, 0x1734, 0x1528, 0x1204, L"Intel(R) Ethernet Controller X540-AT2"},
    {0x8086, 0x8086, 0x1563, 0x001A, L"Intel(R) Ethernet Converged Network Adapter X550-T2"},
    {0x8086, 0x8086, 0x1563, 0x0001, L"Intel(R) Ethernet Converged Network Adapter X550-T2"},
    {0x8086, 0x8086, 0x1563, 0x0022, L"Intel(R) Ethernet Converged Network Adapter X550-T2"},
    {0x8086, 0x8086, 0x1563, 0x001B, L"Intel(R) Ethernet Server Adapter X550-T2 for OCP"},
    {0x8086, 0x1137, 0x1563, 0x02B2, L"Cisco X550-TX 10 Gig LOM"},
    {0x8086, 0x1137, 0x1563, 0x02B3, L"Cisco X550-TX 10 Gig LOM"},
    {0x8086, 0x8086, 0x15D1, 0x00A2, L"Intel(R) Ethernet Converged Network Adapter X550-T1"},
    {0x8086, 0x8086, 0x15D1, 0x0002, L"Intel(R) Ethernet Converged Network Adapter X550-T1"},
    {0x8086, 0x8086, 0x15D1, 0x0021, L"Intel(R) Ethernet Converged Network Adapter X550-T1"},
    {0x8086, 0x8086, 0x15D1, 0x001B, L"Intel(R) Ethernet Server Adapter X550-T1 for OCP"},
    {0x8086, 0x0000, 0x1563, 0x0000, L"Intel(R) Ethernet Controller X550"},
    {0x8086, 0x0000, 0x15AA, 0x0000, L"Intel(R) Ethernet Connection X552 10 GbE Backplane"},
    {0x8086, 0x0000, 0x15AB, 0x0000, L"Intel(R) Ethernet Connection X552 10 GbE Backplane"},
    {0x8086, 0x0000, 0x15AC, 0x0000, L"Intel(R) Ethernet Connection X552 10 GbE SFP+ "},
    {0x8086, 0x0000, 0x15AD, 0x0000, L"Intel(R) Ethernet Connection X552/X557-AT 10GBASE-T"},
    {0x8086, 0x0000, 0x15AE, 0x0000, L"Intel(R) Ethernet Connection X552 1000BASE-T"},
    {0x8086, 0x0000, 0x15B0, 0x0000, L"Intel(R) Ethernet Connection X552 Backplane"},
    {0x8086, 0x0000, 0x15C4, 0x0000, L"Intel(R) Ethernet Connection X553 10 GbE SFP+"},
    {0x8086, 0x0000, 0x15CE, 0x0000, L"Intel(R) Ethernet Connection X553 10 GbE SFP+"},
    {0x8086, 0x0000, 0x15C8, 0x0000, L"Intel(R) Ethernet Connection X553/X557-AT 10GBASE-T"},
    {0x8086, 0x0000, 0x15C6, 0x0000, L"Intel(R) Ethernet Connection X553 1GbE"},
    {0x8086, 0x0000, 0x15C7, 0x0000, L"Intel(R) Ethernet Connection X553 1GbE"},
    {0x8086, 0x0000, 0x15E4, 0x0000, L"Intel(R) Ethernet Connection X553 1GbE"},
    {0x8086, 0x0000, 0x15E5, 0x0000, L"Intel(R) Ethernet Connection X553 1GbE"},
    {0x8086, 0x0000, 0x15C2, 0x0000, L"Intel(R) Ethernet Connection X553 Backplane"},
    {0x8086, 0x0000, 0x15C3, 0x0000, L"Intel(R) Ethernet Connection X553 Backplane"},
    {0x8086, 0x8086, 0x1563, 0x001D, L"Intel(R) Ethernet 10G 2P X550-t Adapter"},
    {0x8086, 0x1028, 0x1563, 0x1FA9, L"Intel(R) Ethernet 10G 4P X550 rNDC"},
    {0x8086, 0x1028, 0x1563, 0x1FA8, L"Intel(R) Ethernet 10G 4P X550/I350 rNDC"},
    {0x8086, 0x8086, 0x10F4, 0xA06F, L"Intel(R) 10 Gigabit XF LR Server Adapter"},
    {INVALID_VENDOR_ID, INVALID_SUBVENDOR_ID, INVALID_DEVICE_ID, INVALID_SUBSYSTEM_ID, L"Invalid"},

};

UINTN mBrandingTableSize = (sizeof (mBrandingTable) / sizeof (mBrandingTable[0]));
