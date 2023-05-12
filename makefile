OBJS = dibthunk.obj dibcall.obj enable.obj init.obj palette.obj &
       scrsw.obj sswhook.obj modes.obj boxv.obj control.obj &
       drvlib.obj control_vxd.obj minivdd_svga.obj vmwsvxd.obj &
       scrsw_svga.obj control_svga.obj modes_svga.obj palette_svga.obj &
       pci.obj svga.obj svga3d.obj svga32.obj pci32.obj &

INCS = -I$(%WATCOM)\h\win -Iddk -Ivmware

VER_BUILD = 4

FLAGS = -DDRV_VER_BUILD=$(VER_BUILD) -DCAP_R5G6B5_ALWAYS_WRONG

# Define HWBLT if BitBlt can be accelerated.
#FLAGS += -DHWBLT

# Set DBGPRINT to add debug printf logging.
#DBGPRINT = 1

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
CFLAGS = -q -wx -s -zu -zls -6 -fp6
CFLAGS32 = -q -wx -s -zls -6s -fp6 -mf
CC = wcc
CC32 = wcc386

# Log VXD to com2
!ifdef DBGPRINT
CFLAGS32 += -DCOM2
!endif

all : boxvmini.drv vmwsmini.drv vmwsmini.vxd

# Object files
drvlib.obj : drvlib.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

boxv.obj : boxv.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

pci.obj : vmware/pci.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

pci32.obj : vmware/pci32.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

svga.obj : vmware/svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

svga32.obj : vmware/svga32.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

svga3d.obj : vmware/svga3d.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

dbgprint.obj : dbgprint.c .autodepend
	$(CC) $(CFLAGS) -zW $(FLAGS) $<

dbgprint32.obj : dbgprint32.c .autodepend
	$(CC32) $(CFLAGS32) $(FLAGS) $<

dibcall.obj : dibcall.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

dibthunk.obj : dibthunk.asm
	wasm -q $(FLAGS) $<

enable.obj : enable.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

init.obj : init.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

control.obj : control.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

control_svga.obj : control_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

control_vxd.obj : control_vxd.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

palette.obj : palette.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<
	
palette_svga.obj : palette_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

sswhook.obj : sswhook.asm
	wasm -q $(FLAGS) $<

modes.obj : modes.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<
	
modes_svga.obj : modes_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

scrsw.obj : scrsw.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

scrsw_svga.obj : scrsw_svga.c .autodepend
	$(CC) $(CFLAGS) -zW $(INCS) $(FLAGS) $<

vmwsvxd.obj : vmwsvxd.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

minivdd_svga.obj : minivdd_svga.c .autodepend
	$(CC32) $(CFLAGS32) $(INCS) $(FLAGS) $<

# Resources
boxvmini.res : res/boxvmini.rc res/colortab.bin res/config.bin res/fonts.bin res/fonts120.bin .autodepend
	wrc -q -r -ad -bt=windows -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/boxvmini.rc
	
vmwsmini.res : res/vmwsmini.rc res/colortab.bin res/config.bin res/fonts.bin res/fonts120.bin .autodepend
	wrc -q -r -ad -bt=windows -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/vmwsmini.rc

vmws_vxd.res : res/vmws_vxd.rc .autodepend
	wrc -q -r -ad -bt=nt -fo=$@ -Ires -I$(%WATCOM)/h/win $(FLAGS) res/vmws_vxd.rc

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

boxvmini.drv : $(OBJS) boxvmini.res dibeng.lib
	wlink op quiet, start=DriverInit_ disable 2055 $(DBGFILE) @<<boxvmini.lnk
system windows dll initglobal
file dibthunk.obj
file dibcall.obj
file drvlib.obj
file enable.obj
file init.obj
file palette.obj
file scrsw.obj
file sswhook.obj
file modes.obj
file boxv.obj
file control.obj
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
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
export GetDriverResourceID.450
export UserRepaintDisable.500
export ValidateMode.700
import GlobalSmartPageLock  KERNEL.230
<<
	wrc -q boxvmini.res $@

vmwsmini.drv : $(OBJS) vmwsmini.res dibeng.lib
	wlink op quiet, start=DriverInit_ disable 2055 $(DBGFILE) @<<vmwsmini.lnk
system windows dll initglobal
file dibthunk.obj
file dibcall.obj
file drvlib.obj
file enable.obj
file init.obj
file palette_svga.obj
file scrsw_svga.obj
file sswhook.obj
file modes_svga.obj
file svga.obj
file svga3d.obj
file pci.obj
file control_svga.obj
file control_vxd.obj
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
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
export GetDriverResourceID.450
export UserRepaintDisable.500
export ValidateMode.700
import GlobalSmartPageLock  KERNEL.230
<<
	wrc -q vmwsmini.res $@

vmwsmini.vxd : $(OBJS) vmws_vxd.res
	wlink op quiet $(DBGFILE32) @<<vmwsmini.lnk
system win_vxd dynamic
option map=vmwsvxd.map
option nodefaultlibs
name vmwsmini.vxd
file vmwsvxd.obj
file minivdd_svga.obj
file pci32.obj
file svga32.obj
segment '_LTEXT' PRELOAD NONDISCARDABLE
segment '_TEXT'  PRELOAD NONDISCARDABLE
segment '_DATA'  PRELOAD NONDISCARDABLE
export VMWS_DDB.1
<<

# not working now
#	wrc -q vmws_vxd.res $@


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

image : .symbolic boxv9x.img

# Create a 1.44MB distribution floppy image.
# NB: The mkimage tool is not supplied.
boxv9x.img : boxvmini.drv boxv9x.inf readme.txt
    if not exist dist mkdir dist
    copy boxvmini.drv dist
    copy boxv9x.inf   dist
    copy readme.txt   dist
    mkimage -l BOXV9X -o boxv9x.img dist
