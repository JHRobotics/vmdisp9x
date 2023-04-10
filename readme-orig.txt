This is a display minidriver for the Windows 9x family of operating systems,
designed to run on the virtual hardware provided by VirtualBox.

The driver can be installed through the Display Settings dialog or through
the Device Manager.

Supported color depths are 8/16/24/32 bits per pixel. The driver supports
many common resolutions, limited by the available video memory (VRAM).

The driver does not have a built in list of modes. It will set more or less
any resolution between 640x480 and 5120x3840. The INF file determines the
set of predefined modes. By editing the INF file or the resolution information
copied into the Registry, users can add or remove available modes.
