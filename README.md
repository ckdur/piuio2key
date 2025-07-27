# PIUIO Input for windows (Using LIBUSB)

This project contains the source code for the piuio2key (io2key). The program does:

- Translate the PIUIO input to keyboard presses
- Has a diagnostic tool to test your pads
- Contains medium support for activating lights on-presses

# Build

- Download the latest release of the windows version of [libusb](https://github.com/mcuee/libusb-win32/releases). 
 Download the libusb-win32-bin-1.4.0.0.zip file.
- Extract it, then copy the following files
    `libusb-win32-bin-1.4.0.0\lib\msvc\libusb.lib` to `.\lib`
    `libusb-win32-bin-1.4.0.0\include\lusb0_usb.h` to `.\include`
- Open VisualStudio, and open `piuio2key.vcxproj`
- Compile

# Make the driver

- Download [zadig](https://github.com/pbatard/libwdi/releases) and extract it.
- Run zadig, click the long dropdown menu and select `EZ-USB FX2` device.

- Click arrow down button on `WinUSB` text box to select `libusb-win32`.
- Click `Install WCID Driver` button.
