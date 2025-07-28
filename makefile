OBJS = &
  dbgprint.obj dibcall.obj &
  dibthunk.obj dddrv.obj drvlib.obj enable.obj init.obj &
  control.obj pm16_calls.obj pm16_calls_svga.obj pm16_calls_qemu.obj &
  palette.obj sswhook.obj modes.obj modes_svga.obj scrsw.obj scrsw_svga.obj &
  pm16_calls_vesa.obj modes_vesa.obj scrsw_vesa.obj

OBJS += &
  dbgprint32.obj svga.obj pci.obj vxd_fbhda.obj vxd_lib.obj vxd_main.obj &
  vxd_main_qemu.obj vxd_main_svga.obj vxd_svga.obj vxd_vdd.obj vxd_vdd_qemu.obj &
  vxd_vdd_svga.obj vxd_vbe.obj vxd_vbe_qemu.obj vxd_mouse.obj &
  vxd_mouse_svga.obj vxd_svga_mouse.obj vxd_svga_mem.obj vxd_svga_cb.obj &
  vxd_halloc.obj vxd_main_vesa.obj vxd_vesa.obj vxd_vdd_vesa.obj vxd_mtrr.obj

INCS = -I$(%WATCOM)\h\win -Iddk -Ivmware

VER_BUILD = 118

FLAGS = -DDRV_VER_BUILD=$(VER_BUILD)

# Watcom resource compiler haven't -40 option, but without it DIB engine
# displays wrong some national fonts and denies to use font smooth edge.
# This option allow to use MS 16-bit RC.EXE from DDK98 but you need
# 32 bit Windows to run this (and DDK98 installed of course).
#DDK98_PATH = C:\98DDK

# Alternative to DDK 98 (and 16bit subsystem) on HOST is fix output exe
# (new executable) flags. Also this utility is needed to fix Watcom VXD
# by wlink. If yout HOST machine isn't Windows, you've to adjust these
# 2 variables:
# executable file name
FIXLINK_EXE = fixlink.exe
# command line to produce executable
FIXLINK_CC  = wcl386 -q fixlink\fixlink.c -fe=$(FIXLINK_EXE)
# FIXLINK_CC = gcc fixlink/fixlink.c -o fixlink

# Define HWBLT if BitBlt can be accelerated.
#FLAGS += -DHWBLT

# Set DBGPRINT to add debug printf logging.
#DBGPRINT = 1

# Generate code for i486, otherwise is code generated for Pentium Pro
#I486 = 1

!ifdef DBGPRINT
FLAGS += -DDBGPRINT
OBJS  += dbgprint.obj dbgprint32.obj
# Need this to work with pre-made boxv9x.lnk
DBGFILE = file dbgprint.obj
DBGFILE32 = file dbgprint32.obj
!else
DBGFILE =
DBGFILE32 =
!endif
CFLAGS = -q -wx -s -zu -zls
CFLAGS32 = -q -wx -s -zls -mf -DVXD32 -fpi87 -ei -oeatxhn 
CC = wcc
CC32 = wcc386

!ifdef I486
CFLAGS   += -4 -fp3
CFLAGS32 += -4s -fp3
!else
CFLAGS   += -6 -fp6
CFLAGS32 += -6s -fp6
!endif

!ifdef DBGPRINT
# This allows to change mapping debug COM port (for 32bit VXD and 16bit DRV)
# -DCOM0 means mute in current module (for example: your HW has only one port
# and you want debug only VXD)
CFLAGS   += -DCOM1
CFLAGS32 += -DCOM2
!else
CFLAGS32 += -d0
!endif

# 16bit RC.EXE from DDK98
!ifdef DDK98_PATH
RC16 = $(DDK98_PATH)\bin\win98\bin16\RC.EXE
RC16_FLAGS = -40 -i $(DDK98_PATH)\inc\win98\inc16 -i res -d DRV_VER_BUILD=$(VER_BUILD)

BOXVMINI_DRV_RC = $(RC16) $(RC16_FLAGS) res\boxvmini.rc $@
VMWSMINI_DRV_RC = $(RC16) $(RC16_FLAGS) res\vmwsmini.rc $@
QEMUMINI_DRV_RC = $(RC16) $(RC16_FLAGS) res\qemumini.rc $@
VESAMINI_DRV_RC = $(RC16) $(RC16_FLAGS) res\vesamini.rc $@
!else
BOXVMINI_DRV_RC = wrc -q boxvmini.res $@ && $(FIXLINK_EXE) -40 $@
VMWSMINI_DRV_RC = wrc -q vmwsmini.res $@ && $(FIXLINK_EXE) -40 $@
QEMUMINI_DRV_RC = wrc -q qemumini.res $@ && $(FIXLINK_EXE) -40 $@
VESAMINI_DRV_RC = wrc -q vesamini.res $@ && $(FIXLINK_EXE) -40 $@
!endif

all : vmwsmini.drv vmwsmini.vxd qemumini.drv qemumini.vxd boxvmini.drv boxvmini.vxd vesamini.drv vesamini.vxd

# Object files: PM16 RING-3
dbgprint.obj : dbgprint.c .autodepend
	$(CC) $(CFLAGS) -zW $(FLAGS) $<

dibcall.obj : dibcall.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

dibthunk.obj : dibthunk.asm
	wasm -q $(FLAGS) $<

dddrv.obj : dddrv.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

drvlib.obj: drvlib.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

enable.obj : enable.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

init.obj : init.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

control.obj : control.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

pm16_calls.obj : pm16_calls.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

pm16_calls_svga.obj : pm16_calls_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

pm16_calls_qemu.obj : pm16_calls_qemu.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

pm16_calls_vesa.obj : pm16_calls_vesa.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

palette.obj : palette.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<
	
sswhook.obj : sswhook.asm
	wasm -q $(FLAGS) $<

modes.obj : modes.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<
	
modes_svga.obj : modes_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

modes_vesa.obj : modes_vesa.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

scrsw.obj : scrsw.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

scrsw_svga.obj : scrsw_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

scrsw_vesa.obj : scrsw_vesa.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

# Object files: PM32 RING-0

dbgprint32.obj : dbgprint32.c .autodepend
	$(CC32) $(CFLAGS32) $(FLAGS) $<
	
svga.obj : vmware/svga.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

pci.obj : vmware/pci.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_fbhda.obj : vxd_fbhda.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_lib.obj : vxd_lib.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_main.obj : vxd_main.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_main_qemu.obj : vxd_main_qemu.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_main_svga.obj : vxd_main_svga.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_main_vesa.obj : vxd_main_vesa.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_mouse.obj : vxd_mouse.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_mouse_svga.obj : vxd_mouse_svga.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_svga.obj : vxd_svga.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_svga_mouse.obj : vxd_svga_mouse.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_svga_mem.obj : vxd_svga_mem.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_svga_cb.obj : vxd_svga_cb.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_halloc.obj : vxd_halloc.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vbe.obj : vxd_vbe.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vesa.obj : vxd_vesa.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vbe_qemu.obj : vxd_vbe_qemu.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vdd.obj : vxd_vdd.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vdd_qemu.obj : vxd_vdd_qemu.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vdd_svga.obj : vxd_vdd_svga.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_vdd_vesa.obj : vxd_vdd_vesa.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

vxd_mtrr.obj : vxd_mtrr.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

# Resources
boxvmini.res : res/boxvmini.rc res/colortab.bin res/config.bin res/fonts.bin res/fonts120.bin .autodepend
	wrc -q -r -ad -bt=windows -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/boxvmini.rc
	
vmwsmini.res : res/vmwsmini.rc res/colortab.bin res/config.bin res/fonts.bin res/fonts120.bin .autodepend
	wrc -q -r -ad -bt=windows -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/vmwsmini.rc

qemumini.res : res/qemumini.rc res/colortab.bin res/config.bin res/fonts.bin res/fonts120.bin .autodepend
	wrc -q -r -ad -bt=windows -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/qemumini.rc

vesamini.res : res/vesamini.rc res/colortab.bin res/config.bin res/fonts.bin res/fonts120.bin .autodepend
	wrc -q -r -ad -bt=windows -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/vesamini.rc

res/colortab.bin : res/colortab.c
	wcc -q $(INCS) $<
	wlink op quiet disable 1014, 1023 name $@ sys dos output raw file colortab.obj

res/config.bin : res/config.c
	wcc -q $(INCS) $<
	wlink op quiet disable 1014, 1023 name $@ sys dos output raw file config.obj

res/fonts.bin : res/fonts.c .autodepend
	wcc -q $(INCS) $<
	wlink op quiet disable 1014, 1023 name $@ sys dos output raw file fonts.obj

res/fonts120.bin : res/fonts120.c .autodepend
	wcc -q $(INCS) $<
	wlink op quiet disable 1014, 1023 name $@ sys dos output raw file fonts120.obj

# Libraries
dibeng.lib : ddk/dibeng.lbc
	wlib -b -q -n -fo -ii @$< $@

# Fixer
$(FIXLINK_EXE): drvfix.c
	$(FIXLINK_CC)

boxvmini.drv : $(OBJS) boxvmini.res dibeng.lib $(FIXLINK_EXE)
	wlink op quiet, start=DriverInit_ disable 2055 $(DBGFILE) @<<boxvmini.lnk
system windows dll initglobal
file dibcall.obj
file dibthunk.obj
file dddrv.obj 
file drvlib.obj
file enable.obj 
file init.obj
file control.obj
file pm16_calls.obj
file palette.obj
file sswhook.obj
file modes.obj
file scrsw.obj
name boxvmini.drv
option map=boxvmini.map
library dibeng.lib
library clibs.lib
option modname=DISPLAY
option description 'DISPLAY : 100, 96, 96 : DIB Engine based Mini display driver.'
option oneautodata
segment type data preload fixed
segment '_TEXT'  preload shared
segment '_INIT'  preload moveable
export BitBlt.1
export ColorInfo.2
export Control.3
export Disable.4
export Enable.5
export EnumDFonts.6
export EnumObj.7
export Output.8
export Pixel.9
export RealizeObject.10
export StrBlt.11
export ScanLR.12
export DeviceMode.13
export ExtTextOut.14
export GetCharWidth.15
export DeviceBitmap.16
export FastBorder.17
export SetAttribute.18
export DibBlt.19
export CreateDIBitmap.20
export DibToDevice.21
export SetPalette.22
export GetPalette.23
export SetPaletteTranslate.24
export GetPaletteTranslate.25
export UpdateColors.26
export StretchBlt.27
export StretchDIBits.28
export SelectBitmap.29
export BitmapBits.30
export ReEnable.31
export DDIGammaRamp.32
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
export GetDriverResourceID.450
export UserRepaintDisable.500
export ValidateMode.700
import GlobalSmartPageLock  KERNEL.230
<<
	$(BOXVMINI_DRV_RC)

vmwsmini.drv : $(OBJS) vmwsmini.res dibeng.lib $(FIXLINK_EXE)
	wlink op quiet, start=DriverInit_ disable 2055 $(DBGFILE) @<<vmwsmini.lnk
system windows dll initglobal
file dibcall.obj
file dibthunk.obj
file dddrv.obj 
file drvlib.obj
file enable.obj 
file init.obj
file control.obj
file pm16_calls_svga.obj
file palette.obj
file sswhook.obj
file modes_svga.obj
file scrsw_svga.obj
name vmwsmini.drv
option map=vmwsmini.map
library dibeng.lib
library clibs.lib
option modname=DISPLAY
option description 'DISPLAY : 100, 96, 96 : DIB Engine based Mini display driver.'
option oneautodata
segment type data preload fixed
segment '_TEXT'   preload shared
segment '_INIT'   preload moveable
export BitBlt.1
export ColorInfo.2
export Control.3
export Disable.4
export Enable.5
export EnumDFonts.6
export EnumObj.7
export Output.8
export Pixel.9
export RealizeObject.10
export StrBlt.11
export ScanLR.12
export DeviceMode.13
export ExtTextOut.14
export GetCharWidth.15
export DeviceBitmap.16
export FastBorder.17
export SetAttribute.18
export DibBlt.19
export CreateDIBitmap.20
export DibToDevice.21
export SetPalette.22
export GetPalette.23
export SetPaletteTranslate.24
export GetPaletteTranslate.25
export UpdateColors.26
export StretchBlt.27
export StretchDIBits.28
export SelectBitmap.29
export BitmapBits.30
export ReEnable.31
export DDIGammaRamp.32
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
export GetDriverResourceID.450
export UserRepaintDisable.500
export ValidateMode.700
import GlobalSmartPageLock  KERNEL.230
<<
	$(VMWSMINI_DRV_RC)

qemumini.drv : $(OBJS) qemumini.res dibeng.lib $(FIXLINK_EXE)
	wlink op quiet, start=DriverInit_ disable 2055 $(DBGFILE) @<<qemumini.lnk
system windows dll initglobal
file dibcall.obj
file dibthunk.obj
file dddrv.obj 
file drvlib.obj
file enable.obj 
file init.obj
file control.obj
file pm16_calls_qemu.obj
file palette.obj
file sswhook.obj
file modes.obj
file scrsw.obj
name qemumini.drv
option map=qemumini.map
library dibeng.lib
library clibs.lib
option modname=DISPLAY
option description 'DISPLAY : 100, 96, 96 : DIB Engine based Mini display driver.'
option oneautodata
segment type data preload fixed
segment '_TEXT'  preload shared
segment '_INIT'  preload moveable
export BitBlt.1
export ColorInfo.2
export Control.3
export Disable.4
export Enable.5
export EnumDFonts.6
export EnumObj.7
export Output.8
export Pixel.9
export RealizeObject.10
export StrBlt.11
export ScanLR.12
export DeviceMode.13
export ExtTextOut.14
export GetCharWidth.15
export DeviceBitmap.16
export FastBorder.17
export SetAttribute.18
export DibBlt.19
export CreateDIBitmap.20
export DibToDevice.21
export SetPalette.22
export GetPalette.23
export SetPaletteTranslate.24
export GetPaletteTranslate.25
export UpdateColors.26
export StretchBlt.27
export StretchDIBits.28
export SelectBitmap.29
export BitmapBits.30
export ReEnable.31
export DDIGammaRamp.32
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
export GetDriverResourceID.450
export UserRepaintDisable.500
export ValidateMode.700
import GlobalSmartPageLock  KERNEL.230
<<
	$(QEMUMINI_DRV_RC)

vesamini.drv : $(OBJS) vesamini.res dibeng.lib $(FIXLINK_EXE)
	wlink op quiet, start=DriverInit_ disable 2055 $(DBGFILE) @<<vesamini.lnk
system windows dll initglobal
file dibcall.obj
file dibthunk.obj
file dddrv.obj 
file drvlib.obj
file enable.obj 
file init.obj
file control.obj
file palette.obj
file sswhook.obj
file scrsw_vesa.obj
file pm16_calls_vesa.obj
file modes_vesa.obj
name vesamini.drv
option map=vesamini.map
library dibeng.lib
library clibs.lib
option modname=DISPLAY
option description 'DISPLAY : 100, 96, 96 : DIB Engine based Mini display driver.'
option oneautodata
segment type data preload fixed
segment '_TEXT'  preload shared
segment '_INIT'  preload moveable
export BitBlt.1
export ColorInfo.2
export Control.3
export Disable.4
export Enable.5
export EnumDFonts.6
export EnumObj.7
export Output.8
export Pixel.9
export RealizeObject.10
export StrBlt.11
export ScanLR.12
export DeviceMode.13
export ExtTextOut.14
export GetCharWidth.15
export DeviceBitmap.16
export FastBorder.17
export SetAttribute.18
export DibBlt.19
export CreateDIBitmap.20
export DibToDevice.21
export SetPalette.22
export GetPalette.23
export SetPaletteTranslate.24
export GetPaletteTranslate.25
export UpdateColors.26
export StretchBlt.27
export StretchDIBits.28
export SelectBitmap.29
export BitmapBits.30
export ReEnable.31
export DDIGammaRamp.32
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
export GetDriverResourceID.450
export UserRepaintDisable.500
export ValidateMode.700
import GlobalSmartPageLock  KERNEL.230
<<
	$(VESAMINI_DRV_RC)

vmwsmini.vxd : $(OBJS) $(FIXLINK_EXE)
	wlink op quiet $(DBGFILE32) @<<vmwsmini.lnk
system win_vxd dynamic
option map=vmwsvxd.map
option nodefaultlibs
name vmwsmini.vxd
file vxd_main_svga.obj
file svga.obj
file pci.obj
file vxd_fbhda.obj
file vxd_lib.obj
file vxd_svga.obj
file vxd_svga_mouse.obj
file vxd_svga_mem.obj
file vxd_svga_cb.obj
file vxd_vdd_svga.obj
file vxd_mouse_svga.obj
file vxd_halloc.obj
segment '_TEXT'  PRELOAD NONDISCARDABLE
segment '_DATA'  PRELOAD NONDISCARDABLE
segment 'CONST'  PRELOAD NONDISCARDABLE
segment 'CONST2' PRELOAD NONDISCARDABLE
segment '_BSS'   PRELOAD NONDISCARDABLE
export VXD_DDB.1
<<
	$(FIXLINK_EXE) -vxd32 $@

# not working now
#	wrc -q vmws_vxd.res $@

qemumini.vxd : $(OBJS) $(FIXLINK_EXE)
	wlink op quiet $(DBGFILE32) @<<qemumini.lnk
system win_vxd dynamic
option map=qemumini.map
option nodefaultlibs
name qemumini.vxd
file vxd_main_qemu.obj
file pci.obj
file vxd_fbhda.obj
file vxd_lib.obj
file vxd_vbe_qemu.obj
file vxd_vdd_qemu.obj
file vxd_mouse.obj
segment '_TEXT'  PRELOAD NONDISCARDABLE
segment '_DATA'  PRELOAD NONDISCARDABLE
segment 'CONST'  PRELOAD NONDISCARDABLE
segment 'CONST2' PRELOAD NONDISCARDABLE
segment '_BSS'   PRELOAD NONDISCARDABLE
export VXD_DDB.1
<<
	$(FIXLINK_EXE) -vxd32 $@

boxvmini.vxd : $(OBJS) $(FIXLINK_EXE)
	wlink op quiet $(DBGFILE32) @<<boxvmini.lnk
system win_vxd dynamic
option map=boxvmini.map
option nodefaultlibs
name boxvmini.vxd
file vxd_main.obj
file pci.obj
file vxd_fbhda.obj
file vxd_vbe.obj
file vxd_lib.obj
file vxd_vdd.obj
file vxd_mouse.obj
segment '_TEXT'  PRELOAD NONDISCARDABLE
segment '_DATA'  PRELOAD NONDISCARDABLE
segment 'CONST'  PRELOAD NONDISCARDABLE
segment 'CONST2' PRELOAD NONDISCARDABLE
segment '_BSS'   PRELOAD NONDISCARDABLE
export VXD_DDB.1
<<
	$(FIXLINK_EXE) -vxd32 $@

vesamini.vxd : $(OBJS) $(FIXLINK_EXE)
	wlink op quiet $(DBGFILE32) @<<vesamini.lnk
system win_vxd dynamic
option map=vesamini.map
option nodefaultlibs
name vesamini.vxd
file vxd_main_vesa.obj
file pci.obj
file vxd_fbhda.obj
file vxd_lib.obj
file vxd_vesa.obj
file vxd_vdd_vesa.obj
file vxd_mouse.obj
file vxd_mtrr.obj
segment '_TEXT'  PRELOAD NONDISCARDABLE
segment '_DATA'  PRELOAD NONDISCARDABLE
segment 'CONST'  PRELOAD NONDISCARDABLE
segment 'CONST2' PRELOAD NONDISCARDABLE
segment '_BSS'   PRELOAD NONDISCARDABLE
export VXD_DDB.1
<<
	$(FIXLINK_EXE) -vxd32 $@

# Cleanup
clean : .symbolic
    rm *.obj
    rm *.err
    rm *.lib
    rm *.drv
    rm *.vxd
    rm *.map
    rm *.res
    rm *.img
    rm res/*.obj
    rm res/*.bin
    rm vmware/*.obj
    -rm -f $(FIXLINK_EXE)

image : .symbolic boxv9x.img

# Create a 1.44MB distribution floppy image.
# NB: The mkimage tool is not supplied.
boxv9x.img : boxvmini.drv boxv9x.inf readme.txt
    if not exist dist mkdir dist
    copy boxvmini.drv dist
    copy boxv9x.inf   dist
    copy readme.txt   dist
    mkimage -l BOXV9X -o boxv9x.img dist
