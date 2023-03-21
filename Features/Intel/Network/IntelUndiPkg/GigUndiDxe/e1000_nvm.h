/**************************************************************************

Copyright (c) 2001 - 2021, Intel Corporation

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
#ifndef _E1000_NVM_H_
#define _E1000_NVM_H_

#if !defined(NO_READ_PBA_RAW) || !defined(NO_WRITE_PBA_RAW)
struct e1000_pba {
	u16 word[2];
	u16 *pba_block;
};
#endif


void e1000_init_nvm_ops_generic(struct e1000_hw *hw);
#ifndef NO_NULL_OPS_SUPPORT
s32  e1000_null_read_nvm(struct e1000_hw *hw, u16 a, u16 b, u16 *c);
void e1000_null_nvm_generic(struct e1000_hw *hw);
s32  e1000_null_led_default(struct e1000_hw *hw, u16 *data);
s32  e1000_null_write_nvm(struct e1000_hw *hw, u16 a, u16 b, u16 *c);
#endif /* NO_NULL_OPS_SUPPORT */
s32  e1000_acquire_nvm_generic(struct e1000_hw *hw);

s32  e1000_poll_eerd_eewr_done(struct e1000_hw *hw, int ee_reg);
s32  e1000_read_mac_addr_generic(struct e1000_hw *hw);
#ifndef NO_PBA_NUM_ONLY_SUPPORT
s32  e1000_read_pba_num_generic(struct e1000_hw *hw, u32 *pba_num);
#endif /* NO_PBA_NUM_ONLY_SUPPORT */
s32  e1000_read_pba_string_generic(struct e1000_hw *hw, u8 *pba_num,
				   u32 pba_num_size);
#ifndef NO_READ_PBA_LENGTH
s32  e1000_read_pba_length_generic(struct e1000_hw *hw, u32 *pba_num_size);
#endif /* NO_READ_PBA_LENGTH */
#ifndef NO_READ_PBA_RAW
s32 e1000_read_pba_raw(struct e1000_hw *hw, u16 *eeprom_buf,
		       u32 eeprom_buf_size, u16 max_pba_block_size,
		       struct e1000_pba *pba);
#endif /* NO_READ_PBA_RAW */
#ifndef NO_WRITE_PBA_RAW
s32 e1000_write_pba_raw(struct e1000_hw *hw, u16 *eeprom_buf,
			u32 eeprom_buf_size, struct e1000_pba *pba);
#endif /* NO_WRITE_PBA_RAW */
#ifndef NO_GET_PBA_BLOCK_SIZE
s32 e1000_get_pba_block_size(struct e1000_hw *hw, u16 *eeprom_buf,
			     u32 eeprom_buf_size, u16 *pba_block_size);
#endif /* NO_GET_PBA_BLOCK_SIZE */
#if !defined(NO_82575_SUPPORT)
s32  e1000_read_nvm_spi(struct e1000_hw *hw, u16 offset, u16 words, u16 *data);
#endif
#ifndef NO_MICROWIRE_SUPPORT
s32  e1000_read_nvm_microwire(struct e1000_hw *hw, u16 offset,
			      u16 words, u16 *data);
#endif
s32  e1000_read_nvm_eerd(struct e1000_hw *hw, u16 offset, u16 words,
			 u16 *data);
s32  e1000_valid_led_default_generic(struct e1000_hw *hw, u16 *data);
s32  e1000_validate_nvm_checksum_generic(struct e1000_hw *hw);
#ifndef NO_MICROWIRE_SUPPORT
s32  e1000_write_nvm_microwire(struct e1000_hw *hw, u16 offset,
			       u16 words, u16 *data);
#endif
s32  e1000_write_nvm_spi(struct e1000_hw *hw, u16 offset, u16 words,
			 u16 *data);
s32  e1000_update_nvm_checksum_generic(struct e1000_hw *hw);
void e1000_release_nvm_generic(struct e1000_hw *hw);

#define E1000_STM_OPCODE	0xDB00

#endif
