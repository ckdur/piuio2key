#pragma once
#include "stdafx.h"
#include "usb_driver.h"
#include "input_handler.h"

class LXIO : public USBDriver
{
public:
  /* returns true if the VID/PID match LXIO. */
  static bool DeviceMatches(int iVendorID, int iProductID);

  bool Open();

  bool Read(uint8_t *pData);
  bool Write(uint8_t *pData);
};

class InputHandler_LXIO : public InputHandler
{
public:
  InputHandler_LXIO();
  ~InputHandler_LXIO();

  void InputThreadMain();

private:
  static bool s_bInitialized;

  LXIO Board;

  void HandleInput();
  void UpdateLights();
};

extern InputHandler_LXIO* g_ihLXIO;