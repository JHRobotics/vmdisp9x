/*
 * list for generating function prototypes, assembly entry generating,
 * and fill dispatch table.
 * comment out functions without usage
 */

VDDFUNC(REGISTER_DISPLAY_DRIVER, register_display_driver)
VDDFUNC(GET_VDD_BANK, get_vdd_bank)
VDDFUNC(SET_VDD_BANK, set_vdd_bank)
VDDFUNC(RESET_BANK, reset_bank)
VDDFUNC(PRE_HIRES_TO_VGA, pre_hires_to_vga)
VDDFUNC(POST_HIRES_TO_VGA, post_hires_to_vga)
VDDFUNC(PRE_VGA_TO_HIRES, pre_vga_to_hires)
VDDFUNC(POST_VGA_TO_HIRES, post_vga_to_hires)
VDDFUNC(SAVE_REGISTERS, save_registers)
VDDFUNC(RESTORE_REGISTERS, restore_registers)
//VDDFUNC(MODIFY_REGISTER_STATE, modify_register_state)
//VDDFUNC(ACCESS_VGA_MEMORY_MODE, access_vga_memory_mode)
//VDDFUNC(ACCESS_LINEAR_MEMORY_MODE, access_linear_memory_mode)
#if defined(QEMU)
VDDFUNC(ENABLE_TRAPS, enable_traps)
#endif
//VDDFUNC(DISABLE_TRAPS, disable_traps)
//VDDFUNC(MAKE_HARDWARE_NOT_BUSY, make_hardware_not_busy)
VDDFUNC(VIRTUALIZE_CRTC_IN, virtualize_crtc_in)
VDDFUNC(VIRTUALIZE_CRTC_OUT, virtualize_crtc_out)
//VDDFUNC(VIRTUALIZE_SEQUENCER_IN, virtualize_sequencer_in)
//VDDFUNC(VIRTUALIZE_SEQUENCER_OUT, virtualize_sequencer_out)
//VDDFUNC(VIRTUALIZE_GCR_IN, virtualize_gcr_in)
//VDDFUNC(VIRTUALIZE_GCR_OUT, virtualize_gcr_out)
//VDDFUNC(SET_LATCH_BANK, set_latch_bank)
//VDDFUNC(RESET_LATCH_BANK, reset_latch_bank)
//VDDFUNC(SAVE_LATCHES, save_latches)
//VDDFUNC(RESTORE_LATCHES, restore_latches)
#if defined(QEMU)
VDDFUNC(DISPLAY_DRIVER_DISABLING, display_driver_disabling)
#endif
//VDDFUNC(SELECT_PLANE, select_plane)
//VDDFUNC(PRE_CRTC_MODE_CHANGE, pre_crtc_mode_change)
//VDDFUNC(POST_CRTC_MODE_CHANGE, post_crtc_mode_change)
//VDDFUNC(VIRTUALIZE_DAC_OUT, virtualize_dac_out)
//VDDFUNC(VIRTUALIZE_DAC_IN, virtualize_dac_in)
///! VDDFUNC(GET_CURRENT_BANK_WRITE, get_current_bank_write)
//VDDFUNC(GET_CURRENT_BANK_READ, get_current_bank_read)
///! VDDFUNC(SET_BANK, set_bank)0
// VDDFUNCRET(CHECK_HIRES_MODE, check_hires_mode)
VDDFUNC(GET_TOTAL_VRAM_SIZE, get_total_vram_size)
///! VDDFUNC(GET_BANK_SIZE, get_bank_size)
#ifdef VESA
//VDDFUNCRET(SET_HIRES_MODE, set_hires_mode)
#endif
///! VDDFUNC(PRE_HIRES_SAVE_RESTORE, pre_hires_save_restore)
///! VDDFUNC(POST_HIRES_SAVE_RESTORE, post_hires_save_restore)
//VDDFUNCRET(VESA_SUPPORT, vesa_support)
VDDFUNC(GET_CHIP_ID, get_chip_id)
#ifdef VESA
VDDFUNCRET(CHECK_SCREEN_SWITCH_OK, check_screen_switch_ok)
#endif
//VDDFUNC(VIRTUALIZE_BLTER_IO, virtualize_blter_io)
//VDDFUNC(SAVE_MESSAGE_MODE_STATE, save_message_mode_state)
//VDDFUNC(SAVE_FORCED_PLANAR_STATE, save_forced_planar_state)
///! VDDFUNC(VESA_CALL_POST_PROCESSING, vesa_call_post_processing)
//VDDFUNC(PRE_INT_10_MODE_SET, pre_int_10_mode_set)
//VDDFUNC(NBR_MINI_VDD_FUNCTIONS_40, nbr_mini_vdd_functions_40)
//VDDFUNC(GET_NUM_UNITS, get_num_units)
//VDDFUNC(TURN_VGA_OFF, turn_vga_off)
//VDDFUNC(TURN_VGA_ON, turn_vga_on)
//VDDFUNC(SET_ADAPTER_POWER_STATE, set_adapter_power_state)
//VDDFUNC(GET_ADAPTER_POWER_STATE_CAPS, get_adapter_power_state_caps)
//VDDFUNC(SET_MONITOR_POWER_STATE, set_monitor_power_state)
//VDDFUNC(GET_MONITOR_POWER_STATE_CAPS, get_monitor_power_state_caps)
//VDDFUNC(GET_MONITOR_INFO, get_monitor_info)
//VDDFUNC(I2C_OPEN, i2c_open)
//VDDFUNC(I2C_ACCESS, i2c_access)
//VDDFUNC(GPIO_OPEN, gpio_open)
//VDDFUNC(GPIO_ACCESS, gpio_access)
//VDDFUNC(COPYPROTECTION_ACCESS, copyprotection_access)
//VDDFUNC(NBR_MINI_VDD_FUNCTIONS_41, nbr_mini_vdd_functions_41)
