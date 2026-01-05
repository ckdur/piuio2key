#pragma once
#include "stdafx.h"
#include "usb_driver.h"

class InputHandler {
public:
  InputHandler();
  ~InputHandler();

  // input field is a combination of each sensor set in m_iInputData
  uint32_t m_iInputField, m_iLastInputField, m_iChanged;
  uint32_t m_iInputData[4];
  uint32_t m_iButtonField, m_iLastButtonField, m_iButtonChanged;

  // data that will be written to PIUIO (lights, sensors)
  uint32_t m_iLightData;

  bool m_bFoundDevice;

  virtual void InputThreadMain();
};