#include "stdafx.h"
#include "input_handler.h"

InputHandler::InputHandler()
{
  m_iLightData = 0x0;
  m_iChanged = 0;
  m_bFoundDevice = false;

  m_iInputField = 0;
  m_iButtonField = 0;
  m_iChanged = 0;
  m_iLastInputField = 0;
  m_iLastButtonField = 0;
  m_iButtonChanged = 0;
}

InputHandler::~InputHandler()
{
}

void InputHandler::InputThreadMain()
{
    // Nothing
}
