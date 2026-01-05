#pragma once
#include "stdafx.h"
#include "usb_driver.h"
#include "input_handler.h"

// ** HERE BEGINS LIBUSB DEFS

// Copied from: OpenITG
#define USB_DIR_OUT   0x00
#define USB_DIR_IN    0x80

#define USB_TYPE_STANDARD   (0x00 << 5)
#define USB_TYPE_CLASS      (0x01 << 5)
#define USB_TYPE_VENDOR     (0x02 << 5)
#define USB_TYPE_RESERVED   (0x03 << 5)

#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02
#define USB_RECIP_OTHER     0x03

class PIUIOBUTTON : public USBDriver
{
public:
  /* returns true if the VID/PID match PIUIOBUTTON. */
  static bool DeviceMatches(int iVendorID, int iProductID);

  bool Open();

  bool Xfer(uint8_t *pData);
};

class InputHandler_PIUIOBUTTON : public InputHandler
{
public:
  InputHandler_PIUIOBUTTON();
  ~InputHandler_PIUIOBUTTON();

  void InputThreadMain();

private:
  static bool s_bInitialized;

  PIUIOBUTTON Board;

  void HandleInput();
  void UpdateLights();
};

extern InputHandler_PIUIOBUTTON* g_ihPIUIOBUTTON;