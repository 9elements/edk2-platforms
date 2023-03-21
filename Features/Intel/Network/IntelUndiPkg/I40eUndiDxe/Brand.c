/**************************************************************************

Copyright (c) 2012 - 2022, Intel Corporation. All rights reserved.

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

  // Generic device Ids
#ifdef X722_SUPPORT
  {0x8086, 0x0000, 0x37D0, 0x0000, L"Intel(R) Ethernet Connection X722 for 10GbE SFP+"},
  {0x8086, 0x0000, 0x37D1, 0x0000, L"Intel(R) Ethernet Connection X722 for 1GbE"},
  {0x8086, 0x0000, 0x37D2, 0x0000, L"Intel(R) Ethernet Connection X722 for 10GBASE-T"},
  {0x8086, 0x0000, 0x37D3, 0x0000, L"Intel(R) Ethernet Connection X722 for 10GbE SFP+"},
  {0x8086, 0x0000, 0x37CF, 0x0000, L"Intel(R) Ethernet Connection X722 for 10GbE QSFP+"},
  {0x8086, 0x0000, 0x37CE, 0x0000, L"Intel(R) Ethernet Connection X722 for 10GbE backplane"},
  {0x8086, 0x1590, 0x37D2, 0x0218, L"HPE Ethernet 10Gb 2-port 568FLR-MMT Adapter"},
  {0x8086, 0x1590, 0x37D3, 0x0219, L"HPE Ethernet 10Gb 2-port 568FLR-MMSFP+ Adapter"},
  {0x8086, 0x1590, 0x37D1, 0x0217, L"HPE Ethernet 1Gb 2-port 368FLR-MMT Adapter"},
  {0x8086, 0x1590, 0x37D1, 0x0216, L"HPE Ethernet 1Gb 2-port 368i Adapter"},
  {0x8086, 0x1590, 0x37D1, 0x0247, L"HPE Ethernet 1Gb 4-port 369i Adapter"},
#endif /* NO_BRANDING_SUPPORT */
  {0x8086, 0x0000, 0x1572, 0x0000, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x0000, 0x1580, 0x0000, L"Intel(R) Ethernet Controller XL710 for 40GbE backplane"},
  {0x8086, 0x0000, 0x1581, 0x0000, L"Intel(R) Ethernet Controller X710 for 10GbE backplane"},
  {0x8086, 0x0000, 0x1583, 0x0000, L"Intel(R) Ethernet Controller XL710 for 40GbE QSFP+"},
  {0x8086, 0x0000, 0x1584, 0x0000, L"Intel(R) Ethernet Controller XL710 for 40GbE QSFP+"},
  {0x8086, 0x0000, 0x1585, 0x0000, L"Intel(R) Ethernet Controller XL710 for 10GbE QSFP+"},
  {0x8086, 0x0000, 0x1586, 0x0000, L"Intel(R) Ethernet Controller X710 for 10GBASE-T"},
  {0x8086, 0x108E, 0x1586, 0x0000, L"Intel(R) Ethernet Controller X710 for 10GBASE-T"},
  {0x8086, 0x108E, 0x1586, 0x4857, L"Intel(R) Ethernet Controller X710 for 10GBASE-T"},
  {0x8086, 0x0000, 0x158A, 0x0000, L"Intel(R) Ethernet Controller XXV710 for 25GbE backplane"},
  {0x8086, 0x0000, 0x158B, 0x0000, L"Intel(R) Ethernet Controller XXV710 for 25GbE SFP28"},
  {0x8086, 0x0000, 0x15FF, 0x0000, L"Intel(R) Ethernet Controller X710 for 10GBASE-T"},
  {0x8086, 0x0000, 0x101F, 0x0000, L"Intel(R) Ethernet Controller V710 for 5GBASE-T"},
  {0x8086, 0x0000, 0x104E, 0x0000, L"Intel(R) Ethernet Controller X710 for 10 Gigabit SFP+"},
  {0x8086, 0x0000, 0x104F, 0x0000, L"Intel(R) Ethernet Controller X710 for 10 Gigabit backplane"},

  // OEM Gen adapters
  {0x8086, 0x8086, 0x1572, 0x00A1, L"Intel(R) Ethernet Network Adapter X710-2 for OCP NIC 3.0"},
  {0x8086, 0x8086, 0x1572, 0x00A2, L"Intel(R) Ethernet Network Adapter X710-4 for OCP NIC 3.0"},
  {0x8086, 0x8086, 0x1572, 0x0008, L"Intel(R) Ethernet Converged Network Adapter X710-2"},
  {0x8086, 0x8086, 0x1572, 0x0004, L"Intel(R) Ethernet Converged Network Adapter X710-4"},
  {0x8086, 0x8086, 0x1572, 0x0002, L"Intel(R) Ethernet Converged Network Adapter X710-4"},
  {0x8086, 0x8086, 0x1584, 0x0002, L"Intel(R) Ethernet Converged Network Adapter XL710-Q1"},
  {0x8086, 0x8086, 0x1583, 0x0002, L"Intel(R) Ethernet Converged Network Adapter XL710-Q2"},
  {0x8086, 0x8086, 0x1589, 0x0002, L"Intel(R) Ethernet Converged Network Adapter X710-T4"},
  {0x8086, 0x8086, 0x1589, 0x0001, L"Intel(R) Ethernet Converged Network Adapter X710-T4"},
  {0x8086, 0x0000, 0x1589, 0x0000, L"Intel(R) Ethernet Controller X710/X557-AT 10GBASE-T"},
  {0x8086, 0x8086, 0x1589, 0x0000, L"Intel(R) Ethernet Converged Network Adapter X710-T"},
  {0x8086, 0x8086, 0x158B, 0x0002, L"Intel(R) Ethernet Network Adapter XXV710-2"},
  {0x8086, 0x8086, 0x158B, 0x0004, L"Intel(R) Ethernet Network Adapter XXV710-1"},
  {0x8086, 0x8086, 0x158B, 0x0006, L"Intel(R) Ethernet Network Adapter OCP XXV710-2"},
  {0x8086, 0x8086, 0x158B, 0x0008, L"Intel(R) Ethernet Network Adapter OCP XXV710-1"},
  {0x8086, 0x8086, 0x158B, 0x000B, L"Intel(R) Ethernet Network Adapter XXV710-DA2T"},
  {0x8086, 0x8086, 0x158B, 0x000C, L"Intel(R) Ethernet Network Adapter XXV710-DA2 for OCP 3.0"},
  {0x8086, 0x8086, 0x15FF, 0x0002, L"Intel(R) Ethernet Network Adapter X710-T4L"},
  {0x8086, 0x8086, 0x15FF, 0x0004, L"Intel(R) Ethernet Network Adapter X710-T2L"},
  {0x8086, 0x8086, 0x15FF, 0x000A, L"Intel(R) Ethernet Network Adapter X710-T4L for OCP 3.0"},
  {0x8086, 0x8086, 0x15FF, 0x000C, L"Intel(R) Ethernet Network Adapter X710-T2L for OCP 3.0"},
  {0x8086, 0x8086, 0x0DD2, 0x000D, L"Intel(R) Ethernet Network Adapter I710-T4L"},
  {0x8086, 0x8086, 0x0DD2, 0x0010, L"Intel(R) Ethernet Network Adapter I710-T4L for OCP 3.0"},
#ifdef X722_SUPPORT
  // Apollo Fountain OEM Gen
  {0x8086, 0x8086, 0x0DDA, 0x0000, L"Intel(R) Ethernet Connection X722 for 10GbE SFP+"},

  // Champagne Fountain OEM Gen
  {0x8086, 0x8086, 0x37D0, 0x0002, L"Intel(R) Ethernet Network Adapter X722-2"},
  {0x8086, 0x8086, 0x37D0, 0x0004, L"Intel(R) Ethernet Network Adapter X722-4"},
#endif /* X722_SUPPORT */

  // Retail adapters
  {0x8086, 0x8086, 0x1572, 0x0011, L"Intel(R) Ethernet Network Adapter X710-2 for OCP NIC 3.0"},
  {0x8086, 0x8086, 0x1572, 0x0012, L"Intel(R) Ethernet Network Adapter X710-4 for OCP NIC 3.0"},
  {0x8086, 0x8086, 0x1572, 0x0007, L"Intel(R) Ethernet Converged Network Adapter X710-2"},
  {0x8086, 0x8086, 0x1572, 0x0001, L"Intel(R) Ethernet Converged Network Adapter X710-4"},
  {0x8086, 0x8086, 0x1572, 0x000E, L"Intel(R) Ethernet Server Adapter OCP X710-2"},
  {0x8086, 0x8086, 0x1572, 0x000F, L"Intel(R) Ethernet Server Adapter OCP X710-2"},
  {0x8086, 0x8086, 0x1584, 0x0001, L"Intel(R) Ethernet Converged Network Adapter XL710-Q1"},
  {0x8086, 0x8086, 0x1583, 0x0001, L"Intel(R) Ethernet Converged Network Adapter XL710-Q2"},
  {0x8086, 0x8086, 0x1584, 0x0004, L"Intel(R) Ethernet Server Adapter XL710-Q1OCP"},
  {0x8086, 0x8086, 0x1583, 0x0004, L"Intel(R) Ethernet Server Adapter XL710-Q2OCP"},
  {0x8086, 0x8086, 0x158B, 0x0001, L"Intel(R) Ethernet Network Adapter XXV710-2"},
  {0x8086, 0x8086, 0x158B, 0x0003, L"Intel(R) Ethernet Network Adapter XXV710-1"},
  {0x8086, 0x8086, 0x158B, 0x0005, L"Intel(R) Ethernet Network Adapter OCP XXV710-2"},
  {0x8086, 0x8086, 0x158B, 0x0007, L"Intel(R) Ethernet Network Adapter OCP XXV710-1"},
  {0x8086, 0x8086, 0x15FF, 0x0001, L"Intel(R) Ethernet Network Adapter X710-T4L"},
  {0x8086, 0x8086, 0x15FF, 0x0003, L"Intel(R) Ethernet Network Adapter X710-T2L"},
  {0x8086, 0x8086, 0x15FF, 0x0009, L"Intel(R) Ethernet Network Adapter X710-T4L for OCP 3.0"},
  {0x8086, 0x8086, 0x15FF, 0x000B, L"Intel(R) Ethernet Network Adapter X710-T2L for OCP 3.0"},
#ifdef X722_SUPPORT

  // Champagne Fountain Retail
  {0x8086, 0x8086, 0x37D0, 0x0001, L"Intel(R) Ethernet Network Adapter X722-2"},
  {0x8086, 0x8086, 0x37D0, 0x0003, L"Intel(R) Ethernet Network Adapter X722-4"},
#endif /* X722_SUPPORT */

  // Generic branding strings applicable for functions with zeroed out Sub Device ID
  {0x8086, 0x8086, 0x1572, 0x0000, L"Intel(R) Ethernet Converged Network Adapter X710"},
  {0x8086, 0x8086, 0x1584, 0x0000, L"Intel(R) Ethernet Converged Network Adapter XL710-Q1"},
  {0x8086, 0x8086, 0x1583, 0x0000, L"Intel(R) Ethernet Converged Network Adapter XL710-Q2"},
  {0x8086, 0x8086, 0x158A, 0x0000, L"Intel(R) Ethernet Controller XXV710 for 25GbE backplane"},
  {0x8086, 0x8086, 0x158B, 0x0000, L"Intel(R) Ethernet Network Adapter XXV710"},
  {0x8086, 0x8086, 0x15FF, 0x0000, L"Intel(R) Ethernet Network Adapter X710-TL"},
  {0x8086, 0x8086, 0x0DD2, 0x0000, L"Intel(R) Ethernet Network Adapter I710-T4L"},

  // PCSD devices
  {0x8086, 0x8086, 0x1584, 0x0003, L"Intel(R) Ethernet I/O Module XL710-Q1"},
  {0x8086, 0x8086, 0x1583, 0x0003, L"Intel(R) Ethernet I/O Module XL710-Q2"},

  // Dell adapters
  {0x8086, 0x8086, 0x1589, 0x0003, L"Intel(R) Ethernet Converged Network Adapter X710-T"},
  {0x8086, 0x8086, 0x1572, 0x0005, L"Intel(R) Ethernet Converged Network Adapter X710"},
  {0x8086, 0x8086, 0x1572, 0x0006, L"Intel(R) Ethernet Converged Network Adapter X710"},
  {0x8086, 0x1028, 0x1581, 0x1F98, L"Intel(R) Ethernet 10G 4P X710-k bNDC"},
  {0x8086, 0x1028, 0x1572, 0x1F99, L"Intel(R) Ethernet 10G 4P X710/I350 rNDC"},
  {0x8086, 0x1028, 0x1581, 0x1F9E, L"Intel(R) Ethernet 10G 2P X710-k bNDC"},
  {0x8086, 0x1028, 0x1581, 0x0000, L"Intel(R) Ethernet 10G X710-k bNDC"},
  {0x8086, 0x1028, 0x1572, 0x1F9C, L"Intel(R) Ethernet 10G 4P X710 SFP+ rNDC"},
  {0x8086, 0x1028, 0x1572, 0x0000, L"Intel(R) Ethernet 10G X710 rNDC"},

  {0x8086, 0x8086, 0x1583, 0x0006, L"Intel(R) Ethernet Converged Network Adapter XL710-Q2"},
  {0x8086, 0x1028, 0x1583, 0x0000, L"Intel(R) Ethernet 40G 2P XL710 QSFP+ rNDC"},
  {0x8086, 0x1028, 0x1583, 0x1F9F, L"Intel(R) Ethernet 40G 2P XL710 QSFP+ rNDC"},
  {0x8086, 0x1028, 0x1586, 0x1FB4, L"Dell Storage SC6100 10Gb BaseT"},
  {0x8086, 0x1028, 0x1572, 0x1FB6, L"Dell Storage SC6100 10Gb SFP+"},
  {0x8086, 0x8086, 0x1572, 0x000B, L"Intel(R) Ethernet Server Adapter X710-DA2 for OCP"},
  {0x8086, 0x8086, 0x158A, 0x000A, L"Intel(R) Ethernet 25G 2P XXV710 Mezz"},
  {0x8086, 0x8086, 0x158B, 0x0009, L"Intel(R) Ethernet 25G 2P XXV710 Adapter"},
  {0x8086, 0x8086, 0x158B, 0x000A, L"Intel(R) Ethernet 25G 2P XXV710 OCP"},
  {0x8086, 0x8086, 0x158B, 0x000D, L"Intel(R) Ethernet 25G 2P XXV710 OCP"},
  {0x8086, 0x8086, 0x1572, 0x0013, L"Intel(R) Ethernet 10G 2P X710 OCP"},
  {0x8086, 0x8086, 0x1572, 0x0014, L"Intel(R) Ethernet 10G 4P X710 OCP"},
  {0x8086, 0x8086, 0x1572, 0x0015, L"Intel(R) Ethernet Server Adapter X710-DA2 for OCP"},
  {0x8086, 0x8086, 0x15FF, 0x0005, L"Intel(R) Ethernet 10G 2P X710-T2L-t Adapter"},
  {0x8086, 0x8086, 0x15FF, 0x0006, L"Intel(R) Ethernet 10G 4P X710-T4L-t Adapter"},
  {0x8086, 0x8086, 0x15FF, 0x0007, L"Intel(R) Ethernet 10G 2P X710-T2L-t OCP"},
  {0x8086, 0x8086, 0x15FF, 0x0008, L"Intel(R) Ethernet 10G 4P X710-T4L-t OCP"},
  {0x8086, 0x8086, 0x15FF, 0x000D, L"Intel(R) Ethernet 10G 2P X710-T2L-t OCP"},

  // IBM adapters
  {0x8086, 0x8086, 0x1572, 0x0009, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x8086, 0x1572, 0x000A, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},

  // Lenovo adapters
  {0x8086, 0x8086, 0x1589, 0x00A0, L"Intel(R) Ethernet Converged Network Adapter X710-T4"},
  {0x8086, 0x8086, 0x1572, 0x4005, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x8086, 0x1572, 0x4006, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x8086, 0x1583, 0x4007, L"Intel(R) Ethernet Controller XL710 for 40GbE QSFP+"},
  {0x8086, 0x8086, 0x1584, 0x4008, L"Intel(R) Ethernet Controller XL710 for 40GbE QSFP+"},
  {0x8086, 0x17AA, 0x1572, 0x0000, L"Lenovo ThinkServer X710 AnyFabric for 10Gbe SFP+"},
  {0x8086, 0x17AA, 0x1572, 0x4001, L"Lenovo ThinkServer X710-4 AnyFabric for 10GbE SFP+"},
  {0x8086, 0x17AA, 0x1572, 0x4002, L"Lenovo ThinkServer X710-2 AnyFabric for 10GbE SFP+"},
  {0x8086, 0x17AA, 0x1583, 0x4003, L"Lenovo ThinkServer XL710-Q2 AnyFabric for 40GbE QSFP+"},
  {0x8086, 0x17AA, 0x1584, 0x4004, L"Lenovo ThinkServer XL710-Q1 AnyFabric for 40GbE QSFP+"},
  {0x8086, 0x17AA, 0x1572, 0x000B, L"Lenovo ThinkServer XL710-4 AnyFabric"},
  {0x8086, 0x8086, 0x1572, 0x000D, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x8086, 0x1581, 0x000E, L"Intel(R) Ethernet Controller X710 for 10GbE backplane"},
  {0x8086, 0x8086, 0x158B, 0x4001, L"Intel(R) Ethernet Network Adapter XXV710-2"},
  {0x8086, 0x8086, 0x1572, 0x4007, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x8086, 0x15FF, 0x000F, L"Intel(R) Ethernet Network Adapter X710-T2L for OCP 3.0"},
  {0x8086, 0x8086, 0x15FF, 0x4009, L"Intel(R) Ethernet Network Adapter X710-T2L"},
  {0x8086, 0x8086, 0x15FF, 0x4019, L"Intel(R) Ethernet Network Adapter X710-T4L"},
  {0x8086, 0x8086, 0x15FF, 0x4012, L"Intel(R) Ethernet Network Adapter X710-T4L for OCP 3.0"},
  {0x8086, 0x8086, 0x15FF, 0x4018, L"Intel(R) Ethernet Network Adapter X710-T2L for OCP 3.0"},
  {0x8086, 0x8086, 0x0DD2, 0x401A, L"Intel(R) Ethernet Network Adapter I710-T4L"},
  {0x8086, 0x8086, 0x0DD2, 0x401B, L"Intel(R) Ethernet Network Adapter I710-T4L for OCP 3.0"},
#ifdef X722_SUPPORT
  {0x8086, 0x17AA, 0x37D2, 0x4020, L"Intel(R) Ethernet Connection X722 for 10GBASE-T"},
  {0x8086, 0x17AA, 0x37D2, 0x4021, L"Intel(R) Ethernet Connection X722 for 10GBASE-T"},
  {0x8086, 0x17AA, 0x37D2, 0x4024, L"Intel(R) Ethernet Connection X722 for 10GBASE-T"},
  {0x8086, 0x17AA, 0x37CE, 0x4023, L"Intel(R) Ethernet Connection X722 for 10GbE backplane"},
  {0x8086, 0x17AA, 0x37D3, 0x4020, L"Intel(R) Ethernet Connection X722 for 10GbE SFP+"},
  {0x8086, 0x17AA, 0x37D3, 0x4021, L"Intel(R) Ethernet Connection X722 for 10GbE SFP+"},
  {0x8086, 0x17AA, 0x37D2, 0x4025, L"Intel(R) Ethernet Connection X722 for 10GBASE-T"},
  {0x8086, 0x17AA, 0x37D3, 0x4025, L"Intel(R) Ethernet Connection X722 for 10G SFP+"},
  {0x8086, 0x17AA, 0x37CE, 0x4025, L"Intel(R) Ethernet Connection X722 for 10GbE backplane"},
  {0x8086, 0x17AA, 0x37D1, 0x4020, L"Intel(R) Ethernet Connection X722 for 1GbE"},
  {0x8086, 0x17AA, 0x37D1, 0x4021, L"Intel(R) Ethernet Connection X722 for 1GbE"},
  {0x8086, 0x17AA, 0x37D1, 0x4022, L"Intel(R) Ethernet Connection X722 for 1GbE"},
  {0x8086, 0x17AA, 0x37D1, 0x4024, L"Intel(R) Ethernet Connection X722 for 1GbE"},
#endif

  // Cisco adapters
  {0x8086, 0x1137, 0x1583, 0x0000, L"Cisco(R) Ethernet Converged NIC XL710-QDA2"},
  {0x8086, 0x1137, 0x1583, 0x013C, L"Cisco(R) Ethernet Converged NIC XL710-QDA2"},
  {0x8086, 0x1137, 0x1572, 0x013B, L"Cisco(R) Ethernet Converged NIC X710-DA4"},
  {0x8086, 0x1137, 0x1572, 0x0000, L"Cisco(R) Ethernet Converged NIC X710-DA"},
  {0x8086, 0x1137, 0x1572, 0x020A, L"Cisco(R) Ethernet Converged NIC X710-DA2"},
  {0x8086, 0x1137, 0x1589, 0x0000, L"Cisco(R) Ethernet Converged NIC X710-T4"},
  {0x8086, 0x1137, 0x1589, 0x020B, L"Cisco(R) Ethernet Converged NIC X710-T4"},
  {0x8086, 0x1137, 0x158B, 0x0000, L"Cisco(R) Ethernet Network Adapter XXV710"},
  {0x8086, 0x1137, 0x158B, 0x02B4, L"Cisco(R) Ethernet Network Adapter XXV710 OCP 2.0"},
  {0x8086, 0x1137, 0x15FF, 0x02C1, L"Cisco(R) X710T2LG 2x10 GbE RJ45 PCIe NIC"},
  {0x8086, 0x1137, 0x15FF, 0x02C2, L"Cisco(R) X710T4LG 4x10 GbE RJ45 PCIe NIC"},
  {0x8086, 0x1137, 0x15FF, 0x0000, L"Cisco(R) X710TLG GbE RJ45 PCIe NIC"},
  {0x8086, 0x1137, 0x15FF, 0x02D9, L"Cisco(R) Ethernet Network Adapter X710-T2L OCP 3.0"},
  {0x8086, 0x1137, 0x15FF, 0x02DA, L"Cisco(R) Ethernet Network Adapter X710-T4L OCP 3.0"},
  {0x8086, 0x1137, 0x0DD2, 0x02E3, L"Cisco(R) I710T4LG 4x1 GbE RJ45 PCIe NIC"},
  {0x8086, 0x1137, 0x0DD2, 0x0000, L"Cisco(R) I710T4LG 4x1 GbE RJ45 PCIe NIC"},

  // Oracle adapters
  {0x8086, 0x108E, 0x1583, 0x0000, L"Oracle 10 Gb/40 Gb Ethernet Adapter"},
  {0x8086, 0x108E, 0x1583, 0x7B1D, L"Oracle 10 Gb/40 Gb Ethernet Adapter"},
  {0x8086, 0x108E, 0x1583, 0x7B1B, L"Oracle Quad 10Gb Ethernet Adapter"},
  {0x8086, 0x108E, 0x1589, 0x0000, L"Oracle Quad Port 10GBase-T Adapter"},
  {0x8086, 0x108E, 0x1589, 0x7B1C, L"Oracle Quad Port 10GBase-T Adapter"},

  // HP adapters
  {0x8086, 0x103C, 0x1572, 0x0000, L"HPE Ethernet 10Gb 562SFP+ Adapter"},
  {0x8086, 0x103C, 0x1572, 0x22FD, L"HPE Ethernet 10Gb 2-port 562SFP+ Adapter"},
  {0x8086, 0x103C, 0x1572, 0x22FC, L"HPE Ethernet 10Gb 2-port 562FLR-SFP+ Adapter"},
  {0x8086, 0x103C, 0x1587, 0x0000, L"HPE Eth 10/20Gb 2p 660FLB Adptr"},
  {0x8086, 0x103C, 0x1587, 0x22FE, L"HPE Eth 10/20Gb 2p 660FLB Adptr"},
  {0x8086, 0x103C, 0x1588, 0x0000, L"HPE Eth 10/20Gb 2p 660M Adptr"},
  {0x8086, 0x103C, 0x1588, 0x22FF, L"HPE Eth 10/20Gb 2p 660M Adptr"},
  {0x8086, 0x1590, 0x1581, 0x0000, L"HPE Ethernet 10Gb 2-port 563i Adapter"},
  {0x8086, 0x1590, 0x1581, 0x00F8, L"HPE Ethernet 10Gb 2-port 563i Adapter"},
  {0x8086, 0x1590, 0x1572, 0x0000, L"HPE Eth 10Gb 4p 563SFP+ Adptr"},
  {0x8086, 0x1590, 0x1572, 0x0225, L"HPE Eth 10Gb 4p 563SFP+ Adptr"},
  {0x8086, 0x1590, 0x1572, 0x0000, L"Intel(R) Ethernet Controller X710 for 10GbE SFP+"},
  {0x8086, 0x1590, 0x1572, 0x022F, L"HPE Ethernet 10Gb 2-port 564i Communication Board "},
  {0x8086, 0x1590, 0x158B, 0x0000, L"Intel(R) Ethernet Network Adapter XXV710-2"},
  {0x8086, 0x1590, 0x158B, 0x0253, L"HPE Ethernet 10/25Gb 2-port 661SFP28 Adapter"},
  {0x8086, 0x1590, 0x158A, 0x0286, L"HPE Synergy 4610C 10/25Gb Ethernet Adapter"},
  {0x8086, 0x1590, 0x158A, 0x0000, L"HPE 10/25Gb Ethernet Adapter"},

  // Inspur adapters
  {0x8086, 0x1BD4, 0x0DDA, 0x0076, L"Inspur Ethernet Connection F102IX722 for 10GbE SFP"},

#ifdef X722_SUPPORT
  {0x8086, 0x1590, 0x37CE, 0x0215, L"HPE Ethernet 10Gb 2-port 568i Adapter"},
#endif
#else /* NOT N0_BRANDING_SUPPORT */
  {0x8086, 0x8086, 0x0000, 0x0000, L"Intel(R) Network Connection"},
#endif /* N0_BRANDING_SUPPORT */
  {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, L" "},

};

UINTN mBrandingTableSize = (sizeof (mBrandingTable) / sizeof (mBrandingTable[0]));

