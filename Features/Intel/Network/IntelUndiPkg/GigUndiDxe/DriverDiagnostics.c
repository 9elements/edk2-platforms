/**************************************************************************

Copyright (c) 2012 - 2021, Intel Corporation. All rights reserved.

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
#include "E1000.h"
#include "DriverDiagnostics.h"

/* Protocol structures tentative definitions */
EFI_DRIVER_DIAGNOSTICS_PROTOCOL  gGigUndiDriverDiagnostics;
EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gGigUndiDriverDiagnostics2;

UINT8 mPacket[MAX_ETHERNET_SIZE];

/* Forward declaration to compile */

/** This routine is used by diagnostic software to put
   the Intel Gigabit PHY into loopback mode.

   Loopback speed is determined by the Speed value
   passed into this routine.
   Valid values are 1000, 100, and 10 Mbps

   @param[in]   Hw      Ptr to this card's HW data structure
   @param[in]   Speed   desired loopback speed

   @retval   TRUE   PHY loopback set successfully
   @retval   FALSE  Failed to set PHY loopback
**/
BOOLEAN
_SetIgpPhyLoopback (
  IN  struct e1000_hw *Hw,
  IN  UINT16           Speed
  );

/** Reads a 16bit value to the Phy. - allows user to
   get PHY Page

   @param[in]   Hw       Handle to this adapter.
   @param[in]   Page     PHY Page # or device #
   @param[in]   Offset   The register to read from the PHY. This is a
                         numeric offset value.
   @param[out]  Value    The value read to return.

   @return  E1000_SUCCESS   PHY register read successfully
   @retval  !E1000_SUCCESS  Failed to read PHY register
**/
INT32
_ReadPhyRegister16Ex (
  IN  struct e1000_hw *Hw,
  IN  UINT32           Page,
  IN  UINT32           Offset,
  OUT UINT16*          Value
  )
{
  Page = (Page & 0x07FF) << 5;
  Offset = ((Offset & 0x1F) | Page);

  return e1000_read_phy_reg (Hw, Offset, Value);
}

/** Writes a 16bit value to the Phy. - allows user to
   set PHY Page

   @param[in]   Hw       Handle to this adapter.
   @param[in]   Page     PHY Page #
   @param[in]   Offset   The register to write to the PHY. This is a
                         numeric offset value.
   @param[in]   Data     The value read to write.

   @return  E1000_SUCCESS   PHY register written successfully
   @retval  !E1000_SUCCESS  Failed to write PHY register
**/
INT32
_WritePhyRegister16Ex (
  IN  struct e1000_hw *Hw,
  IN  UINT32           Page,
  IN  UINT32           Offset,
  IN  UINT16           Data
  )
{
  Page = (Page & 0x07FF) << 5;
  Offset = ((Offset & 0x1F) | Page);

  return e1000_write_phy_reg (Hw, Offset, Data);
}

/** Build a packet to transmit in the phy loopback test.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on so that we can
                              get the MAC address

   @return   Packet is built
**/
VOID
_BuildPacket (
  IN DRIVER_DATA *AdapterInfo
  )
{
  ETHERNET_HDR *EthernetHdr;
  UINT16        Length;
  UINT16        i;

  EthernetHdr = NULL;
  Length      = 0;
  i           = 0;

  ZeroMem ((CHAR8 *) mPacket, MAX_ETHERNET_SIZE);

  // First copy the source and destination addresses
  EthernetHdr = (ETHERNET_HDR *) mPacket;
  CopyMem ((CHAR8 *) &EthernetHdr->SourceAddr, (CHAR8 *) AdapterInfo->Hw.mac.addr, ETH_ALEN);
  CopyMem ((CHAR8 *) &EthernetHdr->DestAddr, (CHAR8 *) AdapterInfo->BroadcastNodeAddress, ETH_ALEN);

  // Calculate the data segment size and store it in the header big Endian style
  Length                  = TEST_PACKET_SIZE - sizeof (ETHERNET_HDR);
  EthernetHdr->Length[0]  = (UINT8) (Length >> 8);
  EthernetHdr->Length[1]  = (UINT8) Length;

  // Generate Packet data
  for (i = 0; i < Length; i++) {
    mPacket[i + sizeof (ETHERNET_HDR)] = (UINT8) i;
  }
}

/** This routine is used by diagnostic software to put
   the 82544, 82540, 82545, and 82546 MAC based network
   cards and the M88E1000 PHY into loopback mode.

   Loopback speed is determined by the Speed value
   passed into this routine.
   Valid values are 1000, 100, and 10 Mbps
   Current procedure is to:
   1) Disable auto-MDI/MDIX
   2) Perform SW phy reset (bit 15 of PHY_CONTROL)
   3) Disable autoneg and reset
   4) For the specified speed, set the loopback
      mode for that speed.  Also force the MAC
      to the correct speed and duplex for the
      specified operation.
   5) If this is an 82543, setup the TX_CLK and
      TX_CRS again.
   6) Disable the receiver so a cable disconnect
      and reconnect will not cause autoneg to
      begin.

   @param[in]   Hw      Ptr to this card's adapter data structure
   @param[in]   Speed   Desired loopback speed

   @retval   TRUE   Phy loopback set successfully
   @retval   FALSE  Failed to set PHY loopback
**/
BOOLEAN
_SetIntegratedM88PhyLoopback (
  IN struct e1000_hw *Hw,
  IN  UINT16          Speed
  )
{
  UINT32  CtrlReg         = 0;
  UINT32  StatusReg       = 0;
  UINT16  PhyReg          = 0;
  BOOLEAN LoopbackModeSet = FALSE;

  Hw->mac.autoneg = FALSE;

  // Set up desired loopback speed and duplex depending on input
  // into this function.
  switch (Speed) {
  case SPEED_1000:
    DEBUGPRINT (DIAG, ("Setting M88E1000 PHY into loopback at 1000 Mbps\n"));

    // Set up the MII control reg to the desired loopback speed.
    if (Hw->phy.type == e1000_phy_igp) {
      e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4140);  // force 1000, set loopback
    } else if (Hw->phy.type == e1000_phy_m88) {
      e1000_write_phy_reg (Hw, M88E1000_PHY_SPEC_CTRL, 0x0808);  // Auto-MDI/MDIX Off
      e1000_write_phy_reg (Hw, PHY_CONTROL, 0x9140);  // reset to update Auto-MDI/MDIX
      e1000_write_phy_reg (Hw, PHY_CONTROL, 0x8140);  // autoneg off
      e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4140);  // force 1000, set loopback
    } else if (Hw->phy.type == e1000_phy_gg82563) {
      e1000_write_phy_reg (Hw, GG82563_PHY_KMRN_MODE_CTRL, 0x1CE);  // Force Link Up
      e1000_write_phy_reg (Hw, GG82563_REG (0, 0), 0x4140);  // bit 14 = IEEE loopback, force 1000, full duplex
    }

    // Now set up the MAC to the same speed/duplex as the PHY.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg |= (E1000_CTRL_FRCSPD   |     // Set the Force Speed Bit
                E1000_CTRL_FRCDPX   |     // Set the Force Duplex Bit
                E1000_CTRL_SPD_1000 |     // Force Speed to 1000
                E1000_CTRL_FD);           // Force Duplex to FULL

    // For some SerDes we'll need to commit the writes now so that the
    // status register is updated on link.
    if (Hw->phy.media_type == e1000_media_type_internal_serdes) {
      E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
      MSEC_DELAY (100);
      CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    }

    if (Hw->phy.media_type == e1000_media_type_copper) {

      // For Marvel Phy, inverts Loss-Of-Signal
      if (Hw->phy.type == e1000_phy_m88) {
        CtrlReg |= (E1000_CTRL_ILOS);         // Invert Loss-Of-Signal
      }
    } else {

      // Set the ILOS bits on the fiber nic if half duplex link is detected.
      StatusReg = E1000_READ_REG (Hw, E1000_STATUS);
      if ((StatusReg & E1000_STATUS_FD) == 0) {
        DEBUGPRINT (DIAG, ("Link seems unstable in PHY Loopback setup\n"));
        CtrlReg |= (E1000_CTRL_ILOS | E1000_CTRL_SLU);          // Invert Loss-Of-Signal
      }
    }

    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
    LoopbackModeSet = TRUE;
    break;

  case SPEED_100:
    DEBUGPRINT (DIAG, ("Setting M88E1000 PHY into loopback at 100 Mbps\n"));

    // Set up the MII control reg to the desired loopback speed.
    e1000_write_phy_reg (Hw, M88E1000_PHY_SPEC_CTRL, 0x0808);    // Auto-MDI/MDIX Off
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x9140);    // reset to update Auto-MDI/MDIX
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x8140);    // autoneg off
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x8100);    // reset to update autoneg
    e1000_write_phy_reg (Hw, M88E1000_EXT_PHY_SPEC_CTRL, 0x0c14);    // MAC interface speed to 100Mbps
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0xe100);    // reset to update MAC interface speed
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x6100);    // force 100, set loopback

    // Now set up the MAC to the same speed/duplex as the PHY.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg |= (E1000_CTRL_ILOS   |      // Invert Loss-Of-Signal
                E1000_CTRL_SLU     |     // Set the Force Link Bit
                E1000_CTRL_FRCSPD  |     // Set the Force Speed Bit
                E1000_CTRL_FRCDPX  |     // Set the Force Duplex Bit
                E1000_CTRL_SPD_100 |     // Force Speed to 100
                E1000_CTRL_FD);          // Force Duplex to FULL

    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
    LoopbackModeSet = TRUE;
    break;

  case SPEED_10:
    DEBUGPRINT (DIAG, ("Setting M88E1000 PHY into loopback at 10 Mbps\n"));

    // Set up the MII control reg to the desired loopback speed.
    e1000_write_phy_reg (Hw, M88E1000_PHY_SPEC_CTRL, 0x0808);    // Auto-MDI/MDIX Off
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x9140);    // reset to update Auto-MDI/MDIX
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x8140);    // autoneg off
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x8100);    // reset to update autoneg
    e1000_write_phy_reg (Hw, M88E1000_EXT_PHY_SPEC_CTRL, 0x0c04);    // MAC interface speed to 10Mbps
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x8100);    // reset to update MAC interface speed
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4100);    // force 10, set loopback

    // Now set up the MAC to the same speed/duplex as the PHY.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg |= (E1000_CTRL_SLU    |       // Set the Force Link Bit
                E1000_CTRL_FRCSPD |        // Set the Force Speed Bit
                E1000_CTRL_FRCDPX |        // Set the Force Duplex Bit
                E1000_CTRL_SPD_10 |        // Force Speed to 10
                E1000_CTRL_FD);            // Force Duplex to FULL

    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
    LoopbackModeSet = TRUE;
    break;

  default:
    DEBUGPRINT (DIAG, ("Invalid speed value loopback mode \"%d\"\n", Speed));
    LoopbackModeSet = FALSE;
    break;
  }

  e1000_read_phy_reg (Hw, PHY_CONTROL, &PhyReg);
  if (Hw->phy.type == e1000_phy_m88) {

    // Disable the receiver on the PHY so when a cable is plugged
    // in, the PHY does not begin to autoneg when a cable is
    // reconnected to the NIC.
    e1000_write_phy_reg (Hw, 29, 0x001F);
    e1000_write_phy_reg (Hw, 30, 0x8FFC);
    e1000_write_phy_reg (Hw, 29, 0x001A);
    e1000_write_phy_reg (Hw, 30, 0x8FF0);

    // This delay is necessary with some nics on some machines after
    // the PHY receiver is disabled.
    USEC_DELAY (500);

    e1000_read_phy_reg (Hw, M88E1000_PHY_SPEC_CTRL, &PhyReg);
    e1000_read_phy_reg (Hw, M88E1000_EXT_PHY_SPEC_CTRL, &PhyReg);
  }

  // The following delay is necessary for the PHY loopback mode to take on ESB2 based LOMs
  MSEC_DELAY (100);

  return LoopbackModeSet;
}

#ifndef  NO_82571_SUPPORT
/** This routine is used to set fiber and serdes based 82571
   and 82575 adapters into loopback mode.

   @param[in]   Hw   Ptr to this card's adapter data structure

   @retval   TRUE   Serdes loopback set successfully
   @retval   FALSE  Failed to set SerDes loopback
**/
VOID
_SetI82571SerdesLoopback (
  struct e1000_hw *Hw
  )
{
  UINT32  CtrlReg         = 0;
  UINT32  TxctlReg        = 0;
  UINT32  StatusReg       = 0;
  BOOLEAN LinkUp          = FALSE;

  DEBUGPRINT (DIAG, ("Setting PHY loopback on I82571 fiber/serdes.\n"));

  // I82571 transceiver loopback
  CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
  CtrlReg |= E1000_CTRL_SLU;
  E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);

  // Disable autoneg by setting bit 31 of TXCW to zero
  TxctlReg = E1000_READ_REG (Hw, E1000_TXCW);
  TxctlReg &= ~(1 << 31);
  E1000_WRITE_REG (Hw, E1000_TXCW, TxctlReg);

  // Read status register link up
  StatusReg = E1000_READ_REG (Hw, E1000_STATUS);
  LinkUp = ((StatusReg & E1000_STATUS_LU) == 0) ? FALSE : TRUE;

  // Set ILOS if link is not up
  if (!LinkUp) {

    // Set bit 7 (Invert Loss) and set link up in bit 6.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg |= (E1000_CTRL_ILOS);
    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
  }

  // Write 0x410 to Serdes control register to enable SerDes analog loopback
  E1000_WRITE_REG (Hw, E1000_SCTL, 0x0410);
  MSEC_DELAY (10);
}
#endif /* NO_82571_SUPPORT */

#ifndef NO_82575_SUPPORT
/** This routine is used to set fiber and serdes based 82571
   and 82575 adapters into loopback mode.

   @param[in]   Hw   Ptr to this card's adapter data structure

   @retval   TRUE   Serdes loopback set successfully
   @retval   FALSE  Failed to set SerDes loopback
**/
VOID
_SetI82575SerdesLoopback (
  struct e1000_hw *Hw
  )
{
  UINT32 CtrlReg         = 0;
  UINT32 CtrlExtReg      = 0;
  UINT32 PcsLctl         = 0;
  UINT32 ConnSwReg       = 0;

  DEBUGPRINT (DIAG, ("Setting PHY loopback on I82575 fiber/serdes.\n"));

  // Write 0x410 to Serdes control register to enable SerDes analog loopback
  E1000_WRITE_REG (Hw, E1000_SCTL, 0x0410);
  MSEC_DELAY (10);

  CtrlExtReg = E1000_READ_REG (Hw, E1000_CTRL_EXT);
  CtrlExtReg |= E1000_CTRL_EXT_LINK_MODE_PCIE_SERDES;
  E1000_WRITE_REG (Hw, E1000_CTRL_EXT, CtrlExtReg);
  MSEC_DELAY (10);

  CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
  CtrlReg |= E1000_CTRL_SLU | E1000_CTRL_FD;
  CtrlReg &= ~(E1000_CTRL_RFCE | E1000_CTRL_TFCE | E1000_CTRL_LRST);
  E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
  MSEC_DELAY (10);

  PcsLctl = E1000_READ_REG (Hw, E1000_PCS_LCTL);
  PcsLctl |= E1000_PCS_LCTL_FORCE_LINK | E1000_PCS_LCTL_FSD |
             E1000_PCS_LCTL_FDV_FULL | E1000_PCS_LCTL_FLV_LINK_UP;
  PcsLctl &= ~E1000_PCS_LCTL_AN_ENABLE;
  E1000_WRITE_REG (Hw, E1000_PCS_LCTL, PcsLctl);
  MSEC_DELAY (10);

  // Read status register link up
  ConnSwReg = E1000_READ_REG (Hw, E1000_CONNSW);
  ConnSwReg &= ~E1000_CONNSW_ENRGSRC;
  E1000_WRITE_REG (Hw, E1000_CONNSW, ConnSwReg);
  MSEC_DELAY (10);

}
#endif /* NO_82575_SUPPORT */

#ifndef NO_82580_SUPPORT
/** This routine is used to set fiber and serdes based 82571
   and 82575 adapters into loopback mode.

   @param[in]   Hw   Ptr to this card's adapter data structure

   @retval   TRUE   Serdes loopback set successfully
   @retval   FALSE  Failed to set SerDes loopback
**/
VOID
_SetI82580SerdesLoopback (
  struct e1000_hw *Hw
  )
{
  UINT32 RctlReg         = 0;
  UINT32 CtrlReg         = 0;
  UINT32 PcsLctl         = 0;
  UINT32 ConnSwReg       = 0;

  DEBUGPRINT (DIAG, ("Setting PHY loopback on 82580 fiber/serdes.\n"));

  if ((Hw->device_id == E1000_DEV_ID_DH89XXCC_SGMII) ||
    (Hw->device_id == E1000_DEV_ID_DH89XXCC_SERDES) ||
    (Hw->device_id == E1000_DEV_ID_DH89XXCC_BACKPLANE) ||
    (Hw->device_id == E1000_DEV_ID_DH89XXCC_SFP))
  {
    UINT32 Reg;

    // Enable DH89xxCC MPHY for near end loopback
    Reg = E1000_READ_REG (Hw, E1000_MPHY_ADDR_CTL);
    Reg = (Reg & E1000_MPHY_ADDR_CTL_OFFSET_MASK) |
          E1000_MPHY_PCS_CLK_REG_OFFSET;
    E1000_WRITE_REG (Hw, E1000_MPHY_ADDR_CTL, Reg);

    Reg = E1000_READ_REG (Hw, E1000_MPHY_DATA);
    Reg |= E1000_MPHY_PCS_CLK_REG_DIGINELBEN;
    E1000_WRITE_REG (Hw, E1000_MPHY_DATA, Reg);
  }

  // Write 0x410 to Serdes control register to enable SerDes analog loopback
  E1000_WRITE_REG (Hw, E1000_SCTL, E1000_ENABLE_SERDES_LOOPBACK);

  // Configure SerDes to loopback
  RctlReg = E1000_READ_REG (Hw, E1000_RCTL);
  RctlReg |= E1000_RCTL_LBM_TCVR;
  E1000_WRITE_REG (Hw, E1000_RCTL, RctlReg);

  // Move to Force mode
  CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
  CtrlReg |= E1000_CTRL_SLU | E1000_CTRL_FD;
  CtrlReg &= ~(E1000_CTRL_RFCE | E1000_CTRL_TFCE);
  E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);

  // Force MAC speed to 1000 Mbps
  CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
  CtrlReg &= ~E1000_CTRL_SPD_SEL;
  CtrlReg |= (E1000_CTRL_FRCSPD  |
              E1000_CTRL_FRCDPX  |
              E1000_CTRL_SPD_1000 |
              E1000_CTRL_FD);
  E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);

  PcsLctl = E1000_READ_REG (Hw, E1000_PCS_LCTL);
  PcsLctl |= E1000_PCS_LCTL_FORCE_LINK | E1000_PCS_LCTL_FSD |
             E1000_PCS_LCTL_FDV_FULL | E1000_PCS_LCTL_FLV_LINK_UP;
  PcsLctl &= ~E1000_PCS_LCTL_AN_ENABLE;
  E1000_WRITE_REG (Hw, E1000_PCS_LCTL, PcsLctl);

  ConnSwReg = E1000_READ_REG (Hw, E1000_CONNSW);
  ConnSwReg &= ~E1000_CONNSW_ENRGSRC;
  E1000_WRITE_REG (Hw, E1000_CONNSW, ConnSwReg);

  MSEC_DELAY (500);   // Need this delay or SerDes loopback will fail.
}
#endif /* NO_82580_SUPPORT */

/** This routine is used by diagnostic software to put
   the Intel Gigabit PHY into loopback mode.

   Loopback speed is determined by the Speed value
   passed into this routine.
   Valid values are 1000, 100, and 10 Mbps

   @param[in]   Hw      Ptr to this card's adapter data structure
   @param[in]   Speed   Desired loopback speed

   @retval   TRUE   PHY loopback set successfully
   @retval   FALSE  Failed to set PHY loopback
**/
BOOLEAN
_SetBoazmanPhyLoopback (
  IN  struct e1000_hw*Hw,
  IN  UINT16          Speed
  )
{
  UINT16  PhyValue        = 0;
  BOOLEAN LoopbackModeSet = FALSE;

#ifndef NO_82571_SUPPORT
#ifndef NO_82574_SUPPORT
  UINT32 Reg             = 0;

  // 82574 requires ILOS set
  if (Hw->mac.type == e1000_82574
    || Hw->mac.type == e1000_82583)
  {
    DEBUGPRINT (DIAG, ( "Setting ILOS on 82574.\n"));
    Reg = E1000_READ_REG (Hw, E1000_CTRL);
    E1000_WRITE_REG (Hw, E1000_CTRL, Reg | E1000_CTRL_ILOS);
  }
#endif /* NO_82574_SUPPORT */
#endif /* NO_82571_SUPPORT */

  if (Speed == SPEED_1000) {
    DEBUGPRINT (DIAG, ( "Setting Boazman PHY into loopback at 1000 Mbps\n"));

    // set 21_2.2:0 to the relevant speed (1G ?3b110, 100Mb ?3b101, 10Mb ? 3b100)
    _ReadPhyRegister16Ex (Hw, 2, 21, &PhyValue);
    PhyValue = (PhyValue & (~(7))) | 6;
  } else if (Speed == SPEED_100) {
    DEBUGPRINT (DIAG, ( "Setting Boazman PHY into loopback at 100 Mbps\n"));

    // set 21_2.2:0 to the relevant speed (1G ?3b110, 100Mb ?3b101, 10Mb ? 3b100)
    _ReadPhyRegister16Ex (Hw, 2, 21, &PhyValue);
    PhyValue = (PhyValue & (~(7))) | 5;
  } else {
    DEBUGPRINT (DIAG, ( "Setting Boazman PHY into loopback at 10 Mbps\n"));

    // set 21_2.2:0 to the relevant speed (1G ?3b110, 100Mb ?3b101, 10Mb ? 3b100)
    _ReadPhyRegister16Ex (Hw, 2, 21, &PhyValue);
    PhyValue = (PhyValue & (~(7))) | 4;
  }

  _WritePhyRegister16Ex (Hw, 2, 21, PhyValue);

  // assert sw reset (so settings will take effect).
  e1000_read_phy_reg (Hw, PHY_CONTROL, &PhyValue);
  e1000_write_phy_reg (Hw, PHY_CONTROL, PhyValue | (1 << 15));
  MSEC_DELAY (1);

#ifndef NO_82571_SUPPORT
#ifndef NO_82574_SUPPORT

  // ICH9 and ICH10 version requires all these undocumented writes
  if (Hw->mac.type != e1000_82574
    || Hw->mac.type != e1000_82583)
  {
    // force duplex to FD: 16_769.3:2=3.
    _ReadPhyRegister16Ex (Hw, 769, 16, &PhyValue);
    PhyValue |= (3 << 2);
    _WritePhyRegister16Ex (Hw, 769, 16, PhyValue);

    // set 16_776.6= state (link up when in force link)
    _ReadPhyRegister16Ex (Hw, 776, 16, &PhyValue);
    PhyValue |= (1 << 6);
    _WritePhyRegister16Ex (Hw, 776, 16, PhyValue);

    // set 16_769.6= state (force link)
    _ReadPhyRegister16Ex (Hw, 769, 16, &PhyValue);
    PhyValue |= (1 << 6);
    _WritePhyRegister16Ex (Hw, 769, 16, PhyValue);

    // Set Early Link Enable - 20_769.10 = 1
    _ReadPhyRegister16Ex (Hw, 769, 20, &PhyValue);
    PhyValue |= (1 << 10);
    _WritePhyRegister16Ex (Hw, 769, 20, PhyValue);
  }
#endif /* NO_82574_SUPPORT */
#endif /* NO_82571_SUPPORT */

  LoopbackModeSet = _SetIgpPhyLoopback (Hw, Speed);

  return LoopbackModeSet;
}

/** This routine is used by diagnostic software to put
   the Intel Gigabit PHY into loopback mode.

   Loopback speed is determined by the Speed value
   passed into this routine.
   Valid values are 1000, 100, and 10 Mbps

   @param[in]   Hw      Ptr to this card's HW data structure
   @param[in]   Speed   desired loopback speed

   @retval   TRUE   PHY loopback set successfully
   @retval   FALSE  Failed to set PHY loopback
**/
BOOLEAN
_SetNinevehPhyLoopback (
  struct e1000_hw *Hw,
  IN  UINT16       Speed
  )
{
  UINT32  StatusReg       = 0;

  // A magic delay, originally was set to 10 and then the loopback diagnoastics fails.
  UINT32  DelayValue      = 100;
  UINT32  DelayMax        = 5000;
  UINT32  i               = 0;
  BOOLEAN LoopbackModeSet = FALSE;

  if (Speed == SPEED_1000) {
    DEBUGPRINT (DIAG, ("Setting Nineveh PHY into loopback at 1000 Mbps\n"));
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4140);    // force 1000, set loopback
    LoopbackModeSet = TRUE;
  } else if (Speed == SPEED_100) {
    DEBUGPRINT (DIAG, ( "Setting Nineveh PHY into loopback at 100 Mbps\n"));
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x6100);    // force 100, set loopback
  } else {
    LoopbackModeSet = _SetIgpPhyLoopback (Hw, Speed);
  }

  // Poll for link to be stable
  for (i = 0; i < DelayMax; i += DelayValue) {
    MSEC_DELAY (DelayValue);
    StatusReg = E1000_READ_REG (Hw, E1000_STATUS);
    if (StatusReg & (E1000_STATUS_LU | E1000_STATUS_FD)) {
      DEBUGPRINT (DIAG, ("Nineveh link up indication after %d iterations\n", i));
      if (Speed == SPEED_1000) {
        if (StatusReg & E1000_STATUS_SPEED_1000) {
          DEBUGPRINT (DIAG, ("Nineveh 1gb loopback link detected after %d iterations\n", i));
          break;
        }
      } else if (Speed == SPEED_100) {
        if (StatusReg & E1000_STATUS_SPEED_100) {
          DEBUGPRINT (DIAG, ("Nineveh 100mbit loopback link detected after %d iterations\n", i));
          break;
        }
      } else {

        // Don't bother reading the status register data for 10mbit. We force this up in
        // _SetIgpPhyLoopback
        DEBUGPRINT (DIAG, ("Nineveh 10mbit loopback link detected after %d iterations\n", i));
        break;
      }
    }
  }

  return LoopbackModeSet;
}

/** This routine is used by diagnostic software to put
   the Intel Gigabit PHY into loopback mode.

   Loopback speed is determined by the Speed value
   passed into this routine.
   Valid values are 1000, 100, and 10 Mbps

   @param[in]   Hw      Ptr to this card's HW data structure
   @param[in]   Speed   desired loopback speed

   @retval   TRUE   PHY loopback set successfully
   @retval   FALSE  Failed to set PHY loopback
**/
BOOLEAN
_SetIgpPhyLoopback (
  IN  struct e1000_hw *Hw,
  IN  UINT16           Speed
  )
{
  UINT32  CtrlReg         = 0;
  UINT32  StatusReg       = 0;
  UINT16  PhyReg          = 0;
  BOOLEAN LoopbackModeSet = FALSE;

  Hw->mac.autoneg = FALSE;

  // Set up desired loopback speed and duplex depending on input
  // into this function.
  switch (Speed) {
  case SPEED_1000:
    DEBUGPRINT (DIAG, ("Setting IGP01E1000 PHY into loopback at 1000 Mbps\n"));

    // Set up the MII control reg to the desired loopback speed.
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4140);    // force 1000, set loopback
    MSEC_DELAY (250);

    // Now set up the MAC to the same speed/duplex as the PHY.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg |= (E1000_CTRL_FRCSPD   |     // Set the Force Speed Bit
                E1000_CTRL_FRCDPX   |     // Set the Force Duplex Bit
                E1000_CTRL_SPD_1000 |     // Force Speed to 1000
                E1000_CTRL_FD);           // Force Duplex to FULL

    if (Hw->phy.media_type != e1000_media_type_copper) {

      // Set the ILOS bits on the fiber nic if half duplex link is detected.
      StatusReg = E1000_READ_REG (Hw, E1000_STATUS);
      if ((StatusReg & E1000_STATUS_FD) == 0) {
        CtrlReg |= (E1000_CTRL_ILOS | E1000_CTRL_SLU);  // Invert Loss-Of-Signal
      }
    }

    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
    LoopbackModeSet = TRUE;
    break;

  case SPEED_100:
    DEBUGPRINT (DIAG, ("Setting IGP01E1000 PHY into loopback at 100 Mbps\n"));

    // Set up the MII control reg to the desired loopback speed.
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x6100);    // force 100, set loopback

    // Now set up the MAC to the same speed/duplex as the PHY.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg |= (E1000_CTRL_FRCSPD  |      // Set the Force Speed Bit
                E1000_CTRL_FRCDPX  |      // Set the Force Duplex Bit
                E1000_CTRL_SPD_100 |      // Force Speed to 100
                E1000_CTRL_FD);           // Force Duplex to FULL

    if (Hw->phy.media_type != e1000_media_type_copper) {

      // Set the ILOS bits on the fiber nic if half duplex link is
      // detected.
      StatusReg = E1000_READ_REG (Hw, E1000_STATUS);
      if ((StatusReg & E1000_STATUS_FD) == 0) {
        CtrlReg |= (E1000_CTRL_ILOS | E1000_CTRL_SLU);  // Invert Loss-Of-Signal
      }
    }

    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
    LoopbackModeSet = TRUE;
    break;

  case SPEED_10:
    DEBUGPRINT (DIAG, ("Setting IGP01E1000 PHY into loopback at 10 Mbps\n"));

    // Set up the MII control reg to the desired loopback speed.
    e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4100);    // force 10, set loopback

    // For 10mbps loopback we need to assert the "Force link pass" bit in
    // the Port Configuration register
    e1000_read_phy_reg (Hw, IGP01E1000_PHY_PORT_CONFIG, &PhyReg);
    PhyReg |= 0x4000;
    e1000_write_phy_reg (Hw, IGP01E1000_PHY_PORT_CONFIG, PhyReg);

    // Now set up the MAC to the same speed/duplex as the PHY.
    CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
    CtrlReg &= ~E1000_CTRL_SPD_SEL;       // Clear the Speed selection bits
    CtrlReg |= (E1000_CTRL_FRCSPD |       // Set the Force Speed Bit
                E1000_CTRL_FRCDPX |       // Set the Force Duplex Bit
                E1000_CTRL_SPD_10 |       // Force Speed to 10
                E1000_CTRL_FD);           // Force Duplex to FULL

    if (Hw->phy.media_type != e1000_media_type_copper) {

      // Set the ILOS bits on the fiber nic if half duplex link is
      // detected.
      StatusReg = E1000_READ_REG (Hw, E1000_STATUS);
      if ((StatusReg & E1000_STATUS_FD) == 0) {
        CtrlReg |= (E1000_CTRL_ILOS | E1000_CTRL_SLU);  // Invert Loss-Of-Signal
      }
    }

    E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);
    LoopbackModeSet = TRUE;
    break;

  default:
    DEBUGPRINT (DIAG, ("Invalid speed value loopback mode \"%d\"\n", Speed));
    LoopbackModeSet = FALSE;
    break;
  }

  USEC_DELAY (500);
  return LoopbackModeSet;
}

#ifndef NO_82580_SUPPORT
/** Sets PHY Loopback on 82580 adapter

   @param[in]   Hw   Ptr to this card's HW data structure

   @retval   TRUE   PHY loopback set successfully
   @retval   FALSE  Failed to set PHY loopback
**/
BOOLEAN
_SetPhyLoopback82580 (
  struct e1000_hw *Hw
  )
{
  UINT32  Reg;
  UINT16  PhyReg = 0;
  BOOLEAN LoopbackModeSet = FALSE;

  DEBUGPRINT (DIAG, ("_SetPhyLoopback82580\n"));

  // Set Link Mode to Internal
  Reg = E1000_READ_REG (Hw, E1000_CTRL_EXT);
  DEBUGPRINT (DIAG, ("_SetPhyLoopback82580: E1000_CTRL_EXT = 0x%x\n", Reg));
  Reg = (~E1000_CTRL_EXT_LINK_MODE_MASK) & Reg;
  E1000_WRITE_REG (Hw, E1000_CTRL_EXT, Reg);

  // Disable PHY power management in case the cable is unplugged and the PHY is asleep
  DEBUGPRINT (DIAG, ("PHPM = %08x\n", E1000_READ_REG (Hw, 0x0E14)));
  Reg = E1000_READ_REG (Hw, 0x0E14);
  Reg &= ~0x0005;     // SPD_EN and LPLU
  E1000_WRITE_REG (Hw, 0x0E14, Reg);

  // Set 1000 Mbps loopback mode in PHY
  e1000_write_phy_reg (Hw, PHY_CONTROL, 0x4140);

  // Set 1000 Mbps mode in MAC
  Reg = E1000_READ_REG (Hw, E1000_CTRL);
  Reg &= ~E1000_CTRL_SPD_SEL;           // Clear the Speed selection bits
  Reg |= (E1000_CTRL_FRCSPD  |          // Set the Force Speed Bit
          E1000_CTRL_FRCDPX  |          // Set the Force Duplex Bit
          E1000_CTRL_SPD_1000 |         // Force Speed to 1000
          E1000_CTRL_FD);               // Force Duplex to FULL
  E1000_WRITE_REG (Hw, E1000_CTRL, Reg);

  // Enable PHY loopback mode
  e1000_read_phy_reg (Hw, PHY_PHLBKC, &PhyReg);
  DEBUGPRINT (DIAG, ("PHY_PHLBKC = %04x\n", PhyReg));
  PhyReg = 0x8001;    // MII and Force Link Status
  e1000_write_phy_reg (Hw, PHY_PHLBKC, PhyReg);

  e1000_read_phy_reg (Hw, PHY_PHCTRL1, &PhyReg);
  DEBUGPRINT (DIAG, ("PHY_PHCTRL1 = %04x\n", PhyReg));
  PhyReg |= 0x2000;   // LNK_EN
  e1000_write_phy_reg (Hw, PHY_PHCTRL1, PhyReg);

  MSEC_DELAY (500);

  LoopbackModeSet = TRUE;
  return LoopbackModeSet;
}
#endif /* NO_82580_SUPPORT */

/** This routine is used to set i354 into MAC loopback mode.

   @param[in]   Hw   Ptr to this card's HW data structure

   @return    Device put into loopback mode
**/
VOID
_SetI354MacLoopback (
  struct e1000_hw *Hw
  )
{
  UINT32 CtrlReg = 0;
  UINT32 RctlReg = 0;
  UINT16 MiiReg  = 0;

  CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);

  e1000_read_phy_reg (Hw, PHY_STATUS, &MiiReg);
  e1000_read_phy_reg (Hw, PHY_STATUS, &MiiReg);

  if (!(MiiReg & MII_SR_LINK_STATUS)) {
    CtrlReg |= E1000_CTRL_ILOS;
  }

  CtrlReg |= (E1000_CTRL_SLU      |   // Set Link up status
              E1000_CTRL_FRCSPD   |   // Set the Force Speed Bit
              E1000_CTRL_FRCDPX   |   // Set the Force Duplex Bit
              E1000_CTRL_SPD_1000 |   // Force Speed to 1000
              E1000_CTRL_FD);         // Force Duplex to FULL

  E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);

  RctlReg = E1000_READ_REG (Hw, E1000_RCTL);
  RctlReg |= E1000_RCTL_LBM_MAC;
  E1000_WRITE_REG (Hw, E1000_RCTL, RctlReg);
}

/** This routine is used to set i350 into MAC loopback mode.

   @param[in]   Hw   Ptr to this card's HW data structure

   @retrun   Device put into loopack mode
**/
VOID
_SetI350MacLoopback (
  struct e1000_hw *Hw
  )
{
  UINT32 RctlReg         = 0;
  UINT32 CtrlReg         = 0;
  UINT32 EeerReg         = 0;

  DEBUGPRINT (DIAG, ("Setting MAC loopback on i350.\n"));

  // Configure SerDes to loopback
  RctlReg = E1000_READ_REG (Hw, E1000_RCTL);
  RctlReg &= ~E1000_RCTL_LBM_MASK;
  RctlReg |= E1000_RCTL_LBM_MAC;
  E1000_WRITE_REG (Hw, E1000_RCTL, RctlReg);

  // Move to Force mode
  CtrlReg = E1000_READ_REG (Hw, E1000_CTRL);
  CtrlReg |= E1000_CTRL_SLU | E1000_CTRL_FRCSPD | E1000_CTRL_FD;
  CtrlReg &= ~(E1000_CTRL_SPD_SEL);
  CtrlReg |= E1000_CTRL_SPD_1000; // Force 1G
  E1000_WRITE_REG (Hw, E1000_CTRL, CtrlReg);

  // Enable checking EEE operation in MAC loopback mode
  EeerReg = E1000_READ_REG (Hw, E1000_EEER);
  EeerReg |= E1000_EEER_EEE_FRC_AN;
  E1000_WRITE_REG (Hw, E1000_EEER, EeerReg);

  MSEC_DELAY (200);   // Add required delay
}


/** Set the PHY into loopback mode.  This routine integrates any errata workarounds that might exist.

   @param[in]   Hw   Pointer to the shared code adapter structure
   @param[in]   Speed   Select speed to perform loopback test

   @retval   TRUE  PHY has been configured for loopback mode
   @retval   FALSE PHY has not been configured for loopback mode
**/
BOOLEAN
E1000SetPhyLoopback (
  struct e1000_hw *Hw,
  UINT32           Speed
  )
{
  BOOLEAN Status;

  DEBUGPRINT (DIAG, ("E1000SetPhyLoopback\n"));

  switch (Hw->mac.type) {
#ifndef NO_82571_SUPPORT
  case e1000_82573:
    DEBUGPRINT (DIAG, ("Enabling M88E1000 loopback mode.\n"));
    Status = _SetIntegratedM88PhyLoopback (Hw, Speed);
    break;
#endif /* NO_82571_SUPPORT */
#ifndef NO_80003ES2LAN_SUPPORT
  case e1000_80003es2lan:
    DEBUGPRINT (DIAG, ("Enabling M88E1000 loopback mode.\n"));
    Status = _SetIntegratedM88PhyLoopback (Hw, Speed);
    break;
#endif /* NO_80003ES2LAN_SUPPORT */
#ifndef NO_82571_SUPPORT
  case e1000_82571:
  case e1000_82572:

    // I82571 sets a special loopback mode through the SERDES register. This is only for Fiber
    // adapters and is used because MAC and PHY loopback are broken on these adapters
    if (Hw->phy.media_type != e1000_media_type_copper) {
      _SetI82571SerdesLoopback (Hw);
      Status = TRUE;
    } else {
      DEBUGPRINT (DIAG, ("I82571: Enabling IGP01E100 loopback mode.\n"));
      Status = _SetIgpPhyLoopback (Hw, Speed);
    }
    break;
#endif /* NO_82571_SUPPORT */

#ifndef NO_82571_SUPPORT
#ifndef NO_82574_SUPPORT
  case e1000_82574:
  case e1000_82583:
    DEBUGPRINT (DIAG, ("Enabling Boazman for 82574, 82583 loopback mode.\n"));
    Status = _SetBoazmanPhyLoopback (Hw, Speed);
    break;
#endif /* NO_82574_SUPPORT */
#endif /* NO_82571_SUPPORT */

#ifndef NO_82575_SUPPORT
  case e1000_82575:
    if (Hw->phy.media_type != e1000_media_type_copper) {
      _SetI82575SerdesLoopback (Hw);
      Status = TRUE;
    } else {
      DEBUGPRINT (DIAG, ("Enabling 82575, 82576 loopback\n"));
      Status = _SetNinevehPhyLoopback (Hw, Speed);
    }
    break;
#endif /* NO_82575_SUPPORT */
#ifndef NO_82576_SUPPORT
  case e1000_82576:
#endif /* NO_82576_SUPPORT */
  case e1000_82580:
  case e1000_i350:
  case e1000_i210:
  case e1000_i211:

    if (Hw->phy.media_type == e1000_media_type_copper
      && !Hw->dev_spec._82575.sgmii_active)
    {
      DEBUGPRINT (DIAG, ("Enabling 82580 loopback for copper\n"));
      Status = _SetPhyLoopback82580 (Hw);
    } else {
      DEBUGPRINT (DIAG, ("Enabling 82580 loopback for SerDes/SGMII/1000BASE-KX\n"));
      if (((Hw->phy.media_type == e1000_media_type_internal_serdes) ||
        (Hw->phy.media_type == e1000_media_type_fiber)) &&
        ((Hw->mac.type == e1000_i350) ||
        (Hw->mac.type == e1000_i210)))
      {
        DEBUGPRINT (DIAG, ("Enabling i350 MAC loopback\n"));
        _SetI350MacLoopback (Hw);
      } else {
        _SetI82580SerdesLoopback (Hw);
      }
      Status = TRUE;
    }
    break;
  case e1000_i354:
    _SetI354MacLoopback (Hw);
    Status = TRUE;
    break;
  default:
    DEBUGPRINT (DIAG, ("Unknown MAC type.\n"));
    DEBUGWAIT (DIAG);
    Status = FALSE;
    break;
  }

  return Status;
}

/** Run the PHY loopback test for N iterations.  This routine transmits a packet, waits a bit, and then
   checks to see if it was received.  If any of the packets are not received then it will be interpreted as
   a failure.

   @param[in]   AdapterInfo      Pointer to the NIC data structure the PHY loopback test will be run on.
   @param[in]   PxeCpbTransmit   Pointer to the packet to transmit.

   @retval   EFI_SUCCESS            All packets were received successfully
   @retval   EFI_OUT_OF_RESOURCES   Not enough memory to allocate packet buffers for testing.
   @retval   EFI_DEVICE_ERROR       Received packet data has been corrupted
   @retval   EFI_DEVICE_ERROR       No data received
**/
EFI_STATUS
GigUndiRunPhyLoopback (
  IN DRIVER_DATA      *AdapterInfo,
  IN PXE_CPB_TRANSMIT  PxeCpbTransmit
  )
{
  PXE_CPB_RECEIVE  CpbReceive;
  PXE_DB_RECEIVE   DbReceive;
  EFI_STATUS       Status;
  UINT64           FreeTxBuffer[DEFAULT_TX_DESCRIPTORS];
  UINT32           i = 0;
  UINT32           j = 0;

  while (j < PHY_LOOPBACK_ITERATIONS) {
    Status = E1000Transmit (
               AdapterInfo,
               (UINT64) (UINTN) &PxeCpbTransmit,
               PXE_OPFLAGS_TRANSMIT_WHOLE
             );

    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("E1000Transmit error Status %X. Iteration=%d\n", Status, j));
      DEBUGWAIT (CRITICAL);
      break;
    }

    // Wait a little, then check to see if the packet has arrived
    CpbReceive.BufferAddr = (PXE_UINT64) (UINTN) AllocateZeroPool (RX_BUFFER_SIZE);
    if (CpbReceive.BufferAddr == (PXE_UINT64) (UINTN) NULL) {
      DEBUGPRINT (CRITICAL, ("Failed to alloc CpbReceive.BufferAddr on iteration %d\n", j));
      Status = EFI_OUT_OF_RESOURCES;
      DEBUGWAIT (CRITICAL);
      break;
    }

    CpbReceive.BufferLen = RX_BUFFER_SIZE;

    for (i = 0; i <= 100000; i++) {
      Status = E1000Receive (
                 AdapterInfo,
                 (PXE_CPB_RECEIVE *) (UINTN) &CpbReceive,
                 (PXE_DB_RECEIVE *) (UINTN) &DbReceive
                 );
      gBS->Stall (10);

      if (Status == PXE_STATCODE_NO_DATA) {
        continue;
      } else if (Status != PXE_STATCODE_SUCCESS) {
        break;
      }

      //
      // Packets from NCSI may be received even though internal PHY loopback
      // is set.
      // Test for packet we have just sent. If received something else, ignore
      // and continue polling for packets.
      //
      if (CompareMem ((VOID *) (UINTN) CpbReceive.BufferAddr, (VOID *) (UINTN) mPacket, TEST_PACKET_SIZE) == 0) {
        //
        // Coming out with PXE_STATCODE_SUCCESS
        //
        break;
      }
    }

    if (i > 100000) {
      DEBUGPRINT (CRITICAL, ("ERROR: Receive timeout on iteration %d\n", j));
      Status = EFI_DEVICE_ERROR;
      break;
    } else if (Status != PXE_STATCODE_SUCCESS) {
      DEBUGPRINT (CRITICAL, ("ERROR: Receive failed with status %X\n", Status));
      Status = EFI_DEVICE_ERROR;
      break;
    }

    E1000FreeTxBuffers (
      AdapterInfo,
      DEFAULT_TX_DESCRIPTORS,
      FreeTxBuffer
    );

    j++;
    gBS->FreePool ((VOID *) ((UINTN) CpbReceive.BufferAddr));
  }

  return Status;
}

/** Sets up the adapter to run the Phy loopback test and then calls
   the loop which will iterate through the test.

   @param[in]   UndiPrivateData   Driver private data structure

   @retval   EFI_SUCCESS                 The PHY loopback test passed.
   @retval   EFI_DEVICE_ERROR            Phy loopback test failed
   @retval   EFI_INVALID_PARAMETER       Some other error occured.
   @retval   EFI_UNSUPPORTED             Failed to set PHY loopback mode
**/
EFI_STATUS
GigUndiPhyLoopback (
  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  PXE_CPB_TRANSMIT                      PxeCpbTransmit;
  EFI_STATUS                            Status;
  EFI_STATUS                            LoopbackStatus;
  DRIVER_DATA                           *AdapterInfo;
  struct e1000_hw                       *Hw;

  AdapterInfo = &UndiPrivateData->NicInfo;
  Hw          = &AdapterInfo->Hw;

  // Uninstall NII protocol.
  // This should make network stack drivers to stop UNDI (including Tx buffer
  // retrieval).
  Status = gBS->UninstallProtocolInterface (
                  UndiPrivateData->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  &UndiPrivateData->NiiProtocol31
                  );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to uninstall NII: %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINTWAIT (DIAG, ("NII uninstalled\n"));

  DEBUGPRINT (DIAG, ("UndiPrivateData->NicInfo.Block %X\n", (UINTN) AdapterInfo->Block));
  DEBUGPRINT (DIAG, ("UndiPrivateData->NicInfo.MapMem %X\n", (UINTN) AdapterInfo->MapMem));
  DEBUGPRINT (DIAG, ("UndiPrivateData->NicInfo.Delay %X\n", (UINTN) AdapterInfo->Delay));
  DEBUGPRINT (DIAG, ("UndiPrivateData->NicInfo.MemIo %X\n", (UINTN) AdapterInfo->MemIo));
  DEBUGWAIT (DIAG);

  // Adapter is stopped at this point. Need to reinitialize it to enable Tx/Rx.
  e1000_reset_hw (Hw);
  AdapterInfo->HwInitialized = FALSE;
  if (E1000Inititialize (AdapterInfo) != PXE_STATCODE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Error initializing adapter!\n"));
    LoopbackStatus = EFI_DEVICE_ERROR;
    goto ExitShutdown;
  }

  DEBUGPRINTWAIT (DIAG, ("Adapter initialized\n"));

  gBS->Stall (200000);

  // Put the PHY into loopback mode.
  if (E1000SetPhyLoopback (Hw, SPEED_1000)) {
    DEBUGPRINTWAIT (DIAG, ("PHY loopback mode set successful\n"));
  } else {
    DEBUGPRINTWAIT (CRITICAL, ("ERROR: PHY loopback not set!\n"));
    LoopbackStatus = EFI_UNSUPPORTED;
    goto ExitShutdown;
  }

  DEBUGPRINTWAIT (DIAG, ("PHY loopback was set up.\n"));

  // Enable the receive unit.
  E1000ReceiveStart (AdapterInfo);

  if (Hw->mac.type == e1000_i210 ||
    Hw->mac.type == e1000_i211)
  {
    gBS->Stall (1000000);
  }

  // Build our packet, and send it out the door.
  DEBUGPRINT (DIAG, ("Building Packet\n"));
  _BuildPacket (AdapterInfo);

  PxeCpbTransmit.MediaheaderLen = sizeof (ETHERNET_HDR);
  PxeCpbTransmit.DataLen        = TEST_PACKET_SIZE - sizeof (ETHERNET_HDR);
  PxeCpbTransmit.FrameAddr      = (UINTN) mPacket;
  PxeCpbTransmit.reserved       = 0;

#if (DBG_LVL & DIAG)
  DEBUGPRINT (DIAG, ("Packet length = %d\n", PxeCpbTransmit.DataLen));
  DEBUGPRINT (DIAG, ("Packet = %X FrameAddr = %X\n", (UINTN) mPacket, PxeCpbTransmit.FrameAddr));
  DEBUGPRINT (DIAG, ("Packet data:\n"));

  for (UINTN i = 0; i < PxeCpbTransmit.DataLen; i++) {
    DEBUGPRINT (DIAG, ("%d: %x ", i, ((UINT8 *) ((UINTN) PxeCpbTransmit.FrameAddr))[i]));
  }
  DEBUGWAIT (DIAG);
#endif /* (DBG_LVL & DIAG) */

  LoopbackStatus = GigUndiRunPhyLoopback (AdapterInfo, PxeCpbTransmit);
  DEBUGPRINTWAIT (DIAG, ("PHY Loopback test returns %r\n", LoopbackStatus));

  // Stop receive unit.
  E1000ReceiveStop (AdapterInfo);

ExitShutdown:
  // After PHY loopback test completes we need to perform a full reset of the adapter.
  // If the adapter was initialized on entry then force a full reset of the adapter.
  // Also reenable the receive unit if it was enabled before we started the PHY loopback test.

  if ((Hw->device_id == E1000_DEV_ID_DH89XXCC_SGMII) ||
    (Hw->device_id == E1000_DEV_ID_DH89XXCC_SERDES) ||
    (Hw->device_id == E1000_DEV_ID_DH89XXCC_BACKPLANE) ||
    (Hw->device_id == E1000_DEV_ID_DH89XXCC_SFP))
  {
    UINT32    Reg;

    // Disable near end loopback on DH89xxCC MPHY
    Reg= E1000_READ_REG (Hw, E1000_MPHY_ADDR_CTL);
    Reg = (Reg & E1000_MPHY_ADDR_CTL_OFFSET_MASK ) |
      E1000_MPHY_PCS_CLK_REG_OFFSET;
    E1000_WRITE_REG (Hw, E1000_MPHY_ADDR_CTL, Reg);

    Reg = E1000_READ_REG (Hw, E1000_MPHY_DATA);
    Reg &= ~E1000_MPHY_PCS_CLK_REG_DIGINELBEN;
    E1000_WRITE_REG (Hw, E1000_MPHY_DATA, Reg);
  }

  // Shutdown the adapter. Make it ready to be reinitialized via UNDI interface.
  E1000Shutdown (AdapterInfo);
  e1000_phy_hw_reset (Hw);
  AdapterInfo->HwInitialized = FALSE;

  DEBUGPRINTWAIT (DIAG, ("Adapter has been reset.\n"));

  // Reinstall NII protocol.
  Status = gBS->InstallProtocolInterface (
                  &UndiPrivateData->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  EFI_NATIVE_INTERFACE,
                  &UndiPrivateData->NiiProtocol31
                  );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("NII installation failed: %r\n", Status));
    goto Exit;
  }

  DEBUGPRINTWAIT (DIAG, ("NII has been reinstalled.\n"));

  // Reattach network stack drivers to the child handle on which NII sits.
  Status = gBS->ConnectController (
                  UndiPrivateData->DeviceHandle,
                  NULL,
                  UndiPrivateData->Undi32DevPath,
                  FALSE
                  );

  if (EFI_ERROR (Status)) {
    DEBUGPRINTWAIT (CRITICAL, ("Failed to connect controller: %r\n", Status));
  }

  DEBUGPRINTWAIT (DIAG, ("Device handle has been reconnected.\n"));

Exit:
  // Report error if loopback or NII (un)install flow fails
  // (with the exception of EFI_NOT_FOUND, expected in case of SNP not loaded beforehand).
  if (EFI_ERROR (LoopbackStatus)) {
    Status = LoopbackStatus;
  } else if (Status == EFI_NOT_FOUND) {
    Status = EFI_SUCCESS;
  }

  DEBUGPRINTWAIT (DIAG, ("Returning: %r\n", Status));

  return Status;
}

/** Runs diagnostics on a controller.

   @param[in]   This   A pointer to the EFI_DRIVER_DIAGNOSTICS_PROTOCOL instance.
   @param[in]   ControllerHandle   The handle of the controller to run diagnostics on.
   @param[in]   ChildHandle     The handle of the child controller to run diagnostics on
                                This is an optional parameter that may be NULL.  It will
                                be NULL for device drivers.  It will also be NULL for a
                                bus drivers that wish to run diagnostics on the bus
                                controller.  It will not be NULL for a bus driver that
                                wishes to run diagnostics on one of its child controllers.
   @param[in]   DiagnosticType   Indicates type of diagnostics to perform on the controller
                                 specified by ControllerHandle and ChildHandle.   See
                                 "Related Definitions" for the list of supported types.
   @param[in]   Language   A pointer to a three character ISO 639-2 language
                           identifier.  This is the language in which the optional
                           error message should be returned in Buffer, and it must
                           match one of the languages specified in SupportedLanguages.
                           The number of languages supported by a driver is up to
                           the driver writer.
   @param[out]   ErrorType   A GUID that defines the format of the data returned in
                             Buffer.
   @param[out]   BufferSize   The size, in bytes, of the data returned in Buffer.
   @param[out]   Buffer   A buffer that contains a Null-terminated Unicode string
                          plus some additional data whose format is defined by
                          ErrorType.  Buffer is allocated by this function with
                          AllocatePool(), and it is the caller's responsibility
                          to free it with a call to FreePool().

   @retval   EFI_SUCCESS            The controller specified by ControllerHandle and
                                    ChildHandle passed the diagnostic.
   @retval   EFI_INVALID_PARAMETER  ControllerHandle is not a valid EFI_HANDLE.
   @retval   EFI_INVALID_PARAMETER  ChildHandle is not NULL and it is not a valid
                                    EFI_HANDLE.
   @retval   EFI_INVALID_PARAMETER  Language is NULL.
   @retval   EFI_INVALID_PARAMETER  ErrorType is NULL.
   @retval   EFI_INVALID_PARAMETER  BufferType is NULL.
   @retval   EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval   EFI_UNSUPPORTED        The driver specified by This does not support
                                    running diagnostics for the controller specified
                                    by ControllerHandle and ChildHandle.
   @retval   EFI_UNSUPPORTED        The driver specified by This does not support the
                                    type of diagnostic specified by DiagnosticType.
   @retval   EFI_UNSUPPORTED        The driver specified by This does not support the
                                    language specified by Language.
   @retval   EFI_OUT_OF_RESOURCES   There are not enough resources available to complete
                                    the diagnostics.
   @retval   EFI_OUT_OF_RESOURCES   There are not enough resources available to return
                                    the status information in ErrorType, BufferSize,
                                    and Buffer.
   @retval   EFI_DEVICE_ERROR       The controller specified by ControllerHandle and
                                    ChildHandle did not pass the diagnostic.
**/
EFI_STATUS
EFIAPI
GigUndiDriverDiagnosticsRunDiagnostics (
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL *           This,
  IN EFI_HANDLE                                  ControllerHandle,
  IN EFI_HANDLE                                  ChildHandle, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTIC_TYPE                  DiagnosticType,
  IN CHAR8 *                                     Language,
  OUT EFI_GUID **                                ErrorType,
  OUT UINTN *                                    BufferSize,
  OUT CHAR16 **                                  Buffer
  )
{
  EFI_DEVICE_PATH_PROTOCOL *UndiDevicePath;
  UNDI_PRIVATE_DATA *       UndiPrivateData;
  EFI_NII_POINTER_PROTOCOL *NiiPointerProtocol;
  EFI_STATUS                Status;

  Status           = EFI_SUCCESS;
  UndiPrivateData  = NULL;

  // Validate input parameters

  // Check against invalid NULL parameters
  if (NULL == Language
    || NULL == ErrorType
    || NULL == BufferSize
    || NULL == Buffer
    || NULL == ControllerHandle)
  {
    return EFI_INVALID_PARAMETER;
  }

  // Check against unsupported languages
  if (((&gGigUndiDriverDiagnostics == This) &&
    (CompareMem ("eng", Language, 4) != 0))
    || (((EFI_DRIVER_DIAGNOSTICS_PROTOCOL *) &gGigUndiDriverDiagnostics2 == This) &&
    (CompareMem ("en-US", Language, 6) != 0)))
  {
    DEBUGPRINT (CRITICAL, ("Driver Diagnostics: Unsupported Language\n"));
    return EFI_UNSUPPORTED;
  }

  // Make sure this driver is currently managing ControllerHandle
  // This satisfies the ControllerHandle validation requirement in scope of detecion of invalid EFI handle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID * *) &UndiDevicePath,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (DIAG, (" OpenProtocol Status = %8X\n", Status));
    return Status;
  }

  //  Open an instance for the gEfiPro1000Comp protocol so we can check
  //  if the child handle interface is actually supported and calculate the pointer to UndiPrivateData.
  DEBUGPRINT (DIAG, ("Open an instance for the gEfiPro1000Com Protocol\n"));
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNiiPointerGuid,
                  (VOID * *) &NiiPointerProtocol,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("OpenProtocol error Status %X\n", Status));
    DEBUGWAIT (CRITICAL);
    return Status;
  }

  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

  // ChildHandle input parameter can be NULL. If it is not NULL we have to validate it.
  if (NULL != ChildHandle) {

    // Make sure this ChildHandle is a valid EFI handle with NII protocol support
    // This satisfies the ChildHandle validation requirement in scope of detecion of invalid EFI handle
    Status = gBS->OpenProtocol (
                    ChildHandle,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    NULL,
                    gUndiDriverBinding.DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

    if (EFI_ERROR (Status)) {
      DEBUGPRINT (DIAG, (" OpenProtocol Status = %8X\n", Status));
      return Status;
    }

    // Now we know the ChildHandle is a valid EFI handle.
    // Let's check if current ControllerHandle supports ChildHandle
    if (ChildHandle != UndiPrivateData->DeviceHandle) {
      DEBUGPRINT (CRITICAL, ("Driver Diagnostics: Unsupported Child handle: %x\n", ChildHandle));
      DEBUGPRINT (CRITICAL, ("UndiPrivateData->DeviceHandle: %x\n", UndiPrivateData->DeviceHandle));
      return EFI_UNSUPPORTED;
    }
  }

  switch (DiagnosticType) {
  case EfiDriverDiagnosticTypeStandard:
    if  (e1000_validate_nvm_checksum (&UndiPrivateData->NicInfo.Hw) == 0) {
      Status = EFI_SUCCESS;
    } else {
      DEBUGPRINT (CRITICAL, ("Driver Diagnostics: e1000_validate_nvm_checksum error!\n"));
      DEBUGWAIT (CRITICAL);
      Status = EFI_DEVICE_ERROR;
    }
    break;
  case EfiDriverDiagnosticTypeExtended:
    if (UndiPrivateData->NicInfo.UndiEnabled
      && UndiPrivateData->IsChildInitialized)
    {
      Status = GigUndiPhyLoopback (UndiPrivateData);
      if (EFI_ERROR (Status)) {
        DEBUGPRINT (CRITICAL, ("Driver Diagnostics: GigUndiPhyLoopback error Status %X\n", Status));
        DEBUGWAIT (CRITICAL);
      }
    } else {
      Status = EFI_UNSUPPORTED;
    }
    break;
  case EfiDriverDiagnosticTypeManufacturing:
    DEBUGPRINT (CRITICAL, ("Driver Diagnostics: EfiDriverDiagnosticTypeManufacturing not supported\n"));
    DEBUGWAIT (CRITICAL);
    Status = EFI_UNSUPPORTED;
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Driver Diagnostics: DiagnosticType unsupported!\n"));
    DEBUGWAIT (CRITICAL);
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}

/* Protocol structures definitions and initialization */

EFI_DRIVER_DIAGNOSTICS_PROTOCOL gGigUndiDriverDiagnostics = {
  GigUndiDriverDiagnosticsRunDiagnostics,
  "eng"
};

EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gGigUndiDriverDiagnostics2 = {
  (EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS) GigUndiDriverDiagnosticsRunDiagnostics,
  "en-US"
};
