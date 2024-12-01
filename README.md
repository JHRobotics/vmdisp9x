# VMDisp9x
Virtual Display driver for Windows 95/98/Me. Supported devices are:
- Bochs VBE Extensions (Bochs: VBE, VirtulBox: VboxVGA, QEMU: std-vga)
- VMWare SVGA-II (VMWare Workstation/Player, VirtulBox: VMSVGA, QEMU: vmware-svga)
- VBox SVGA (VirtulBox: VBoxSVGA)

Supported and tested virtualization software are:
- ~~VirtualBox 6.0 (2D, software 3D)~~
- VirtualBox 6.1 (2D, hardware OpenGL 2.1 through DX9/OpenGL)
- VirtualBox 7.x (2D, hardware OpenGL 2.1 through DX9/OpenGL or 4.1 through DX11/Vulkan)
- VMWare Player 16 (2D, hardware OpenGL 2.1/3.3)
- VMWare Workstation 17 (2D, hardware OpenGL 2.1/4.3)
- QEMU 7.x, 8.0 (2D, software 3D)

2D driver is very generic and probably works with other Virtualization software as well, 3D part required my Mesa port = https://github.com/JHRobotics/mesa9x. See its documentation for more info.

## Easy installation
This repository only contains the display driver, if you want user friendly installation, please use [SoftGPU](https://github.com/JHRobotics/softgpu) instead (this driver is part of SoftGPU project). There is also small [tutorial here](https://github.com/JHRobotics/vmdisp9x/issues/9#issuecomment-2452598883).

## Origin
Driver is based on [Michal Necasek's VirtualBox driver](http://www.os2museum.com/wp/windows-9x-video-minidriver-hd/). With my modifications:
- added VMWare SVGA and VirtualBox SVGA support
- added OpenGL ICD support (simple command that only returns the name of OpenGL implementation library)
- most calls converted to 32bit mini-VDD driver (faster and not limited by 64k segmentation)
- added access VMWare/VBox SVGA API to support real 3D acceleration from user space driver
- added API to access VRAM/FB directly
- added DirectDraw support
- added DirectX support (in development)

QEMU support (`-vga std`) is from [Philip Kelley driver modification](https://github.com/phkelley/boxv9x)


## OpenGL support
OpenGL is supported by OpenGL ICD driver loading (you can use software only driver but also exists HW accelerated implementations). Currently supported ICD drivers are [Mesa9x](https://github.com/JHRobotics/mesa9x/) or my [qemu-3dfx fork](https://github.com/JHRobotics/qemu-3dfx).

## DirectDraw support
DirectDraw is supported, Ring-3 driver is in separated project [VMHal9x](https://github.com/JHRobotics/vmhal9x).

## Direct3D support
Microsoft DirectX is complex and relatively complicated API handled by DDI driver. For Windows 9x there are 4 versions of this API DirectX: DirectX 2.0 - 6.1 through DDRAW.dll, DirectX 7 still through DDRAW.dll, DirectX 8 through D3D8.dll and DirectX 9 through D3D9.dll. DDI driver has feature level (basically equals API version request with some forward compatibility) and every later API after DX7 required minimum level, for example DirectX 8 required DDI version 6 as minimum, DirectX 9 required 7. Behaviour changes with DirectX 10 which required DDI version 10. If you wish to implement DDI 9 you must also implement all older. This change with DirectX 11 where it is needed only actual feature set and all older DDI can be emulated - but this is out of reach by Windows 9x and we're still stuck at DirectX 9.

Direct3D is now in development, most of D3D code is in VMHal9x, rasterization is done by Mesa9x. **D3D is not part of releases yet!**

## Glide support
Glide support has nothing to do with display driver. But when OpenGL is supported is possible to use [OpenGlide9X](ttps://github.com/JHRobotics/openglide9x) wrapper to translate Glide (2 and 3) calls to OpenGL.


## VirtualBox

VirtualBox is supported from version 6.1 (but 5.0 and 6.0 with some limitation works). More on [SoftGPU readme](https://github.com/JHRobotics/softgpu/#virtualbox-vm-setup-with-hw-acceleration).


## VMware Workstation and Player
VMware workstation is supported in current version (17.5.x). In theory, this driver can work from version 9.x, but I don't have enough resources to do complete testing of old closed non-free software. More information also on [SoftGPU readme](https://github.com/JHRobotics/softgpu/?tab=readme-ov-file#vmware-workstation-setup-with-hw-acceleration).


## QEMU
QEMU is supported since **v1.2023.0.10**. Supported adapters are `-vga std` which using modified VBE driver (`qemumini.drv`) and `-vga vmware` where VMware driver now works but is limited to 32bpp colours only. I plan to support *VirGL* in future, but currently no 3D acceleration isn't available in vanilla QEMU (but HW acceleration is possible with [QEMU-3DFX](https://github.com/kjliew/qemu-3dfx).


## Technical
`*.drv` = 16bit driver runs in 16-bit protected mode in RING 3 (!) but with access to I/O instructions 

`*.vxd` = 32bit driver runs in 32-bit protected mode in RING 0

`*.dll` = 32bit user library runs in 32-bit protected mode in RING 3

### Adapters

Default `*.inf` file is supporting these 4 adapters:

`[VBox]` Default adapter to VirtualBox (VBoxVGA) (until version 6.0 only one adapter) - using 16-bit `boxvmini.drv` driver. Device identification is `PCI\VEN_80EE&DEV_BEEF&SUBSYS_00000000`.

`[Qemu]` QEMUs `-vga std` (or `-device VGA`) device - using 16-bit `qemumini.drv` driver. Device identification is `PCI\VEN_1234&DEV_1111`.

`[VMSvga]` VMware adapter, VirtualBox VMSVGA and QEMU `-vga vmware`. Using 16-bit `vmwsmini.drv` driver and 32-bit `vmwsmini.vxd` driver. Device identification is `PCI\VEN_15AD&DEV_0405&SUBSYS_040515AD`.

`[VBoxSvga]` VirtualBox VBoxSVGA, using 16-bit `vmwsmini.drv` driver and 32-bit `vmwsmini.vxd` driver. Device identification is `PCI\VEN_80EE&DEV_BEEF&SUBSYS_040515AD`.

## Resolutions support
With default `*.inf` file, maximum resolution is 1920 x 1200. Maximum wired resolution is 5120 x 4096. For compatibility reasons maximum of VRAM is limited to 128 MB (If you set to adapter more, it'll report only first 128 MB).

However, it is possible increase the limit to 256 MB (Windows 9x maximum) by set this registry key: `HKLM\Software\VMWSVGA\VRAMLimit` to 256. (You can also decrease VRAM size by same way and have more free space is system area.)

### QXGA, WQHD, 4K and 5K
Resolutions sets larger than FullHD, are present in inf file, but needs to be *enabled* if you wish  would use them. They're split to 4 individual sections:

- `[VM.QXGA]` - QXGA, QWXGA and some others bit larger then FullHD
- `[VM.WQHD]` - 1440p resolutions set
- `[VM.UHD]` - 4K resolutions set
- `[VM.R5K]` - 5K resolutions set

To enable one of them just append section name to `AddReg=` parameter to corresponding adapter. For example, to add 4K to **VMware adapter** change:

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

For example, adding 1366x768 for all colour modes can look like:

```
HKR,"MODES\8\1366,768"
HKR,"MODES\16\1366,768"
HKR,"MODES\24\1366,768"
HKR,"MODES\32\1366,768"
```

### VRAM size

Due limitation of virtual display card is usually required have enough memory for 2 display buffers, when 1st one is always in 32bpp. So, for 1024x768 16bpp you need about 4.5 MB VRAM (1024 x 768 x 4 + 1024 x 768 x 2). When HW acceleration is used, VRAM is not well utilized - textures and frame buffers must be in system RAM.


## Security
In 2D mode any application could read and write guest frame buffer and rest of video ram. If 3D is enabled and works (on hypervisor side) is possible by any application to write virtual GPU FIFO which could leads to read memory of different process (in same guest) or crash the guest. These risks are noted but needs to be mentioned that these old systems haven’t any or has only minimal security management. For example, Microsoft Windows 9x systems haven't file system rights, all process has mapped system memory (in last 1 GB of 32-bit memory space) and any user could run 16-bit application where have access to everything including I/O because of compatibility.


## Compilation from source
Install [Open Watcom 1.9](http://openwatcom.org/ftp/install/), then type
```
wmake
```
Edit `makefile` to enable addition logging and you can read original [readdev.txt](readdev.txt).

## Todo
- Complete recomended mini-VDD function in [minivdd.c](minivdd.c), stubs here and cites from original MSDN are in comments.
- ~Complete GPU10 functions (with synchronization with Mesa)~
- VirGL
- DDI
- VESA support

## External links
http://www.os2museum.com/wp/windows-9x-video-minidriver-hd/

https://wiki.osdev.org/Bochs_VBE_Extensions

https://wiki.osdev.org/VMWare_SVGA-II
