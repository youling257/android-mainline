# SPDX-License-Identifier: GPL-2.0

EXTRA_CFLAGS += -Wno-unused-variable
EXTRA_CFLAGS += -Wno-unused-value
EXTRA_CFLAGS += -Wno-unused-label
EXTRA_CFLAGS += -Wno-unused-parameter
EXTRA_CFLAGS += -Wno-unused-function
EXTRA_CFLAGS += -Wno-unused
EXTRA_CFLAGS += -DCONFIG_CONCURRENT_MODE
EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(src)/hal
EXTRA_CFLAGS += -I$(src)/platform
EXTRA_CFLAGS += -DCONFIG_POWER_SAVING
EXTRA_CFLAGS += -DCONFIG_BT_COEXIST
EXTRA_CFLAGS += -DCONFIG_TRAFFIC_PROTECT
EXTRA_CFLAGS += -DCONFIG_LOAD_PHY_PARA_FROM_FILE
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211
EXTRA_CFLAGS += -DRTW_USE_CFG80211_STA_EVENT
EXTRA_CFLAGS += -DCONFIG_P2P_IPS

r8723bu-y = \
		core/rtw_cmd.o \
		core/rtw_security.o \
		core/rtw_debug.o \
		core/rtw_io.o \
		core/rtw_ioctl_query.o \
		core/rtw_ioctl_set.o \
		core/rtw_ieee80211.o \
		core/rtw_mlme.o \
		core/rtw_mlme_ext.o \
		core/rtw_wlan_util.o \
		core/rtw_vht.o \
		core/rtw_pwrctrl.o \
		core/rtw_rf.o \
		core/rtw_recv.o \
		core/rtw_sta_mgt.o \
		core/rtw_ap.o \
		core/rtw_xmit.o	\
		core/rtw_p2p.o \
		core/rtw_tdls.o \
		core/rtw_br_ext.o \
		core/rtw_iol.o \
		core/rtw_sreset.o \
		core/rtw_btcoex.o \
		core/rtw_beamforming.o \
		core/rtw_odm.o \
		core/rtw_efuse.o \
		hal/hal_intf.o \
		hal/hal_com.o \
		hal/hal_com_phycfg.o \
		hal/hal_phy.o \
		hal/hal_btcoex.o \
		hal/hal_usb.o \
		hal/hal_usb_led.o \
		hal/odm_debug.o \
		hal/odm_AntDiv.o\
		hal/odm_interface.o\
		hal/odm_HWConfig.o\
		hal/odm.o\
		hal/HalPhyRf.o\
		hal/odm_EdcaTurboCheck.o\
		hal/odm_DIG.o\
		hal/odm_PathDiv.o\
		hal/odm_RaInfo.o\
		hal/odm_DynamicBBPowerSaving.o\
		hal/odm_DynamicTxPower.o\
		hal/odm_CfoTracking.o\
		hal/odm_NoiseMonitor.o \
		hal/HalBtc8723b1Ant.o \
		hal/HalBtc8723b2Ant.o \
		hal/HalPwrSeqCmd.o \
		hal/Hal8723BPwrSeq.o\
		hal/rtl8723b_sreset.o \
		hal/rtl8723b_hal_init.o \
		hal/rtl8723b_phycfg.o \
		hal/rtl8723b_rf6052.o \
		hal/rtl8723b_dm.o \
		hal/rtl8723b_rxdesc.o \
		hal/rtl8723b_cmd.o \
		hal/usb_halinit.o \
		hal/rtl8723bu_led.o \
		hal/rtl8723bu_xmit.o \
		hal/rtl8723bu_recv.o \
		hal/usb_ops.o \
		hal/HalHWImg8723B_BB.o \
		hal/HalHWImg8723B_MAC.o \
		hal/HalHWImg8723B_RF.o \
		hal/HalHWImg8723B_FW.o \
		hal/odm_RegConfig8723B.o \
		hal/HalPhyRf_8723B.o \
		hal/odm_RTL8723B.o \
		os_dep/osdep_service.o \
		os_dep/os_intfs.o \
		os_dep/usb_intf.o \
		os_dep/usb_ops_linux.o \
		os_dep/ioctl_linux.o \
		os_dep/xmit_linux.o \
		os_dep/mlme_linux.o \
		os_dep/recv_linux.o \
		os_dep/ioctl_cfg80211.o \
		os_dep/wifi_regd.o \
		os_dep/rtw_android.o \
		os_dep/rtw_proc.o \
		platform/platform_ops.o

obj-$(CONFIG_RTL8723BU) := r8723bu.o

ccflags-y += -I$(srctree)/$(src)/include -I$(srctree)/$(src)/hal -I$(srctree)$(src)/platform
