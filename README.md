# VMDisp9x
Virtual Display driver for Windows 95/98/Me. Supported devices are:
- Bochs VBE Extensions (Bochs: VBE, VirtulBox: VboxVGA, QEMU: std-vga)
- VMWare SVGA-II (VMWare Workstation/Player, VirtulBox: VMSVGA, QEMU: vmware-svga)
- VBox SVGA (VirtulBox: VBoxSVGA)

Supported and tested virtualization software are:
- ~~VirtualBox 6.0 (2D, software 3D)~~
- VirtualBox 6.1 (2D, hardware OpenGL 2.1 through DX9 or OpenGL)
- VirtualBox 7.0 (2D, hardware OpenGL 2.1 through DX9 or OpenGL)
- VMWare Player 16 (2D, software 3D)
- VMWare Workstation 17 (2D, hardware OpenGL 2.1)
- QEMU 7.x, 8.0 (2D, software 3D)

2D driver is very generic and probably works with other Virtualization software as well, 3D part required my Mesa port = https://github.com/JHRobotics/mesa9x. See its documentation for more info.

## Full acceleration package
This repository contains only basic 2D driver, for 3D (hardware or software) acceleration you need sort of addition libraries and wrappers. My all-in-one package is here: https://github.com/JHRobotics/softgpu

## Origin
Driver is based on [Michal Necasek's VirtualBox driver](http://www.os2museum.com/wp/windows-9x-video-minidriver-hd/). With my modifications:
- added VMWare SVGA and VirtualBox SVGA support
- added OpenGL ICD support (simple command that only returns the name of OpenGL implementation library)
- added 32bit mini-VDD driver stub (required for future development to better support for 16-bit applications)
- added access to VMWare/VBox SVGA registers and FIFO to support real 3D acceleration from user space driver

QEMU support (`-vga std`) is from [Philip Kelley driver modification](https://github.com/phkelley/boxv9x)


## OpenGL ICD support
OpenGL is much more driver independent than DirectX, if fact, there is only 1 (ONE by words) function that needs to be served by driver, 0x1101. More on https://github.com/JHRobotics/mesa9x

## Glide support
3dfx proprietary API (later open sourced) for their Voodoo Graphics 3D accelerator cards. This has nothing to do with the display driver - keep in mind, that original Voodoo and Voodoo 2 cards were only 3D accretors and you still need some other video card to draw 2D part (= all system things). Calling from user space was through DLL (Glide2x.dll, Glide3x.dll) in Windows or driver (Glide.vxd) in DOS and they made the rest.

If you have some OpenGL driver, you can use OpenGlide to translate Glide call to OpenGL. My port of Open Glide for Windows 9x is here: https://github.com/JHRobotics/openglide9x

## DirectX support
Microsoft DirectDraw/DirectX is complex and relatively complicated API handled by DDI driver. For Windows 9x there are 4 version of this API DirectX: DirectX 2.0 - 6.1 through DDRAW.dll, DirectX 7 still through DDRAW.dll, DirectX through D3D8.dll and DirectX 9 through D3D9.dll. DDI driver has feature level (basically equals API version request with some forward compatibility) and every API required minimum level, for example DirectX 8 required DDI version 6 as minimum, DirectX 9 required 7. Behaviour changes with DirectX 10 which required DDI version 10. If you wish to implement DDI 9 you must also implement all older. This change with DirectX 11 where is needed only actual feature set and all older DDI can be emulated - but is out of reach by Windows 9x and we still stuck DirectX 9.

There no DDI support in this driver, but here is Wine project (and with set called WineD3D) which can translate DirectX (and DirectDraw too) calls to OpenGL. Minimum is OpenGL with version 2.1 or better. My port WineD3D for Windows 9x is here: https://github.com/JHRobotics/wine9x

Of course, there is alternative for DX8 and DX9 called Swiftshader 2.0

## QEMU
QEMU is supported since **v1.2023.0.10**. Supported adapters are `-vga std` which using modified VBE driver (`qemumini.drv`) and `-vga vmware` where VMware driver now works but is limited to 32 bpp colors only. I plan to support *VirGL* in future, but currently no 3D acceleration isn't available in QEMU. 

## Technical
`*.drv` = 16bit driver runs in 16-bit protected mode in RING 3 (!) but with access to I/O instructions 

`*.vxd` = 32bit driver runs in 32-bit protected mode in RING 0

`*.dll` = 32bit user library runs in 32-bit protected mode in RING 3

### Adapters

Default inf file is supporting these 4 adapter:

`[VBox]` Default adapter to VirtualBox (VBoxVGA) (until version 6.0 only one adapter) - using 16 bit `boxvmini.drv` driver. Device identification is `PCI\VEN_80EE&DEV_BEEF&SUBSYS_00000000`.

`[Qemu]` QEMUs `-vga std` (or `-device VGA`) device - using 16 bit `qemumini.drv` driver. Device identification is `PCI\VEN_1234&DEV_1111`.

`[VMSvga]` VMware adapter, VirtualBox VMSVGA and QEMU `-vga vmware`. Using 16 bit `vmwsmini.drv` driver and 32 bit `vmwsmini.vxd` driver. Device identification is `PCI\VEN_15AD&DEV_0405&SUBSYS_040515AD`.

`[VBoxSvga]` VirtualBox VBoxSVGA, using 16 bit `vmwsmini.drv` driver and 32 bit `vmwsmini.vxd` driver. Device identification is `PCI\VEN_80EE&DEV_BEEF&SUBSYS_040515AD`.

## Resolutions support
With default `*.inf` file, default maximum resolution is 1920 x 1200. Maximum wired resolution is 5120 x 4096. For compatibility reasons maximum of VRAM is limited to 128 MB (If you set to adapter more, it'll report only first 128 MB).

However it is possible increase the limit to 256 MB (Windows 9x maximum), if code is compiled with defined `VRAM256MB` (edit `makefile` to set this and recompile project) maximum resolution for this settings is 8192 x 5120.

### QXGA, WQHD, 4K and 5K
Resolutions sets larger than FullHD, are present in inf file, but needs to be *enabled* if you wish use them. They're split to 4 individual section:

- `[VM.QXGA]` - QXGA, QWXGA and some others bit larger then FullHD
- `[VM.WQHD]` - 1440p resolutions set
- `[VM.UHD]` - 4K resolutions set
- `[VM.R5K]` - 5K resolutions set

To enable one of them just append section name to `AddReg=` parameter to corresponding adapter. For example to add 4K to **VMware adapter** change:

```
[VMSvga]
CopyFiles=VMSvga.Copy,Dx.Copy,DX.CopyBackup,Voodoo.Copy
DelReg=VM.DelReg
AddReg=VMSvga.AddReg,VM.AddReg,DX.addReg
```

to:

```
[VMSvga]
CopyFiles=VMSvga.Copy,Dx.Copy,DX.CopyBackup,Voodoo.Copy
DelReg=VM.DelReg
AddReg=VMSvga.AddReg,VM.AddReg,DX.addReg,VM.UHD
```

### Custom resolutions

To add custom resolution just append line to `[VM.AddReg]` section with following format:

```
HKR,"MODES\{BPP}\{WIDTH},{HEIGHT}"
```

For example adding 1366x768 for all color modes can look like:

```
HKR,"MODES\8\1366,768"
HKR,"MODES\16\1366,768"
HKR,"MODES\24\1366,768"
HKR,"MODES\32\1366,768"
```

### VRAM size

In current state the driver using VRAM as framebuffer only. For example if you're using 1920 x 1080 x 32 bpp, driver consume only 8 MB of VRAM and rest of is unutilized. Even if you using HW accelerated graphics with this driver all textures, surface and even backbuffers (implemented as surfaces) are stored in virtual machine's RAM.


## Security
In 2D mode any application could read and write guest frame buffer and rest of video ram. If 3D is enabled and works (on hypervisor site) is possible by any application to write virtual GPU FIFO which could leads to read memory of different process (in same guest) or crash the guest. These risks are noted but needs to be mentioned that these old systems haven't or has only minimal security management. For example, Microsoft Windows 9x system hasn't no file system rights, all process has mapped system memory (in last 1 GB of 32bit memory space) and any user could run 16bit application where have access to everything including I/O because of compatibility.

## Compilation from source
Install [Open Watcom 1.9](http://openwatcom.org/ftp/install/), then type
```
wmake
```
Edit `makefile` to enable addition logging and you can read original [readdev.txt](readdev.txt).

## Todo
- Complete recomended mini-VDD function in [minivdd.c](minivdd.c), stubs here and cites from original MSDN are in comments.
- Complete GPU10 functions (with synchronization with Mesa)
- VirGL
- DDI

## External links
http://www.os2museum.com/wp/windows-9x-video-minidriver-hd/

https://wiki.osdev.org/Bochs_VBE_Extensions

https://wiki.osdev.org/VMWare_SVGA-II
