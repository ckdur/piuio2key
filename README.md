# PIUIO Input for windows (Using LIBUSB)

This project contains the source code for the piuio2key (io2key). The program does:

- Translate the PIUIO input to keyboard presses
- Has a diagnostic tool to test your pads
- Contains medium support for activating lights on-presses

# Build

- First, you need to create several folders in `D:\usr`, namely `lib` and `include`.
- Download the latest release of the windows version of libusb, which can be found in
 [https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.6.0/]. 
 Download the libusb-win32-bin-1.2.6.0.zip file.
- Extract it, then copy the following files
    `libusb-win32-bin-1.2.6.0\lib\msvc\libusb.lib` to `D:\usr\lib`
    `libusb-win32-bin-1.2.6.0\include\lusb0_usb.h` to `D:\usr\include`
- Open VisualStudio, and open `piuio2key.vcxproj`
- Compile

# Make the driver

- Download the latest release of the windows version of libusb, which can be found in
 [https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.6.0/]. 
 Download the libusb-win32-bin-1.2.6.0.zip file.
- In `libusb-win32-bin-1.2.6.0\lib`, open the `inf-wizard.exe`
- Follow the steps (with the IO connected) to create the driver.
- More easily, you can just download the main libusb-filter from [https://sourceforge.net/projects/libusb-win32]
