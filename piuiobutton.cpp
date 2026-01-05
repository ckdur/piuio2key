#include "stdafx.h"
#include "piuiobutton.h"

InputHandler_PIUIOBUTTON* g_ihPIUIOBUTTON;

#define PIUIOBUTTON_VENDOR_ID 0x0D2F
#define PIUIOBUTTON_PRODUCT_ID 0x1010

#define PIUIOBUTTON_ENDPOINT_OUT 0x02
#define PIUIOBUTTON_ENDPOINT_IN 0x01

/* proprietary (arbitrary?) request PIUIOBUTTON requires to handle I/O */
const short PIUIOBUTTON_CTL_REQ = 0xAE;

/* timeout value for read/writes, in microseconds (so, 10 ms) */
const int REQ_TIMEOUT = 10000;

bool PIUIOBUTTON::DeviceMatches(int iVID, int iPID)
{
  return iVID == PIUIOBUTTON_VENDOR_ID && iPID == PIUIOBUTTON_PRODUCT_ID;
}

bool PIUIOBUTTON::Open()
{
  return OpenInternal(PIUIOBUTTON_VENDOR_ID, PIUIOBUTTON_PRODUCT_ID);
}

bool PIUIOBUTTON::Xfer(uint8_t *pData)
{
  int iExpected = 16;

  int iResult = m_pDriver->InterruptRead(
    PIUIOBUTTON_ENDPOINT_OUT, (char*)pData, iExpected, 1000);

  return iResult == iExpected;
}


bool InputHandler_PIUIOBUTTON::s_bInitialized = false;

// simple helper function to automatically reopen PIUIOBUTTON if a USB error occurs
static void Reconnect(PIUIOBUTTON &board)
{
  printf("PIUIOBUTTON connection lost! Retrying...");

  while (!board.Open())
  {
    board.Close();
  }

  printf("PIUIOBUTTON reconnected.");
}

InputHandler_PIUIOBUTTON::InputHandler_PIUIOBUTTON()
{
  InputHandler::InputHandler();

  if (s_bInitialized)
  {
    printf("InputHandler_PIUIOBUTTON: Redundant driver loaded. Disabling...");
    return;
  }

  // attempt to connect to the I/O board
  if (!Board.Open())
  {
    printf("InputHandler_PIUIOBUTTON: Could not establish a connection with the I/O device.");
    return;
  }

  // set the relevant global flags (static flag, input type)
  m_bFoundDevice = true;
  s_bInitialized = true;
}

InputHandler_PIUIOBUTTON::~InputHandler_PIUIOBUTTON()
{
  // reset all lights and unclaim the device
  if (m_bFoundDevice)
  {
    Board.Close();

    s_bInitialized = false;
  }
}

void InputHandler_PIUIOBUTTON::InputThreadMain()
{
  UpdateLights();
  HandleInput();
}

void InputHandler_PIUIOBUTTON::HandleInput()
{
  if (!m_bFoundDevice) return;
  // reset our reading data
  m_iInputField = 0x0;
  memset(&m_iInputData, 0, sizeof(m_iInputData) / sizeof(uint32_t));

  uint8_t bytes_fb[16];
  memset(bytes_fb, 0, 16);

  Board.Xfer(bytes_fb);

  // generate our input events bit field (1 = change, 0 = no change)
  m_iChanged = 0;
  m_iInputField = 0;
  m_iLastInputField = 0;

  // For the button, convert it to lxio entries
  uint8_t* bytes = (uint8_t*)&m_iButtonField;
  bytes[0] = ((bytes_fb[0] & 0x01)? 0xFF:0xFC) & // UL/UR both on red button
             ((bytes_fb[0] & 0x02)? 0xFF:0xF7) & // DL on left button
             ((bytes_fb[0] & 0x04)? 0xFF:0xEF) & // DR on right button
             ((bytes_fb[0] & 0x08)? 0xFF:0xFB) ; // Center on green button
  bytes[1] = ((bytes_fb[0] & 0x10)? 0xFF:0xFC) & // UL/UR both on red button
             ((bytes_fb[0] & 0x20)? 0xFF:0xF7) & // DL on left button
             ((bytes_fb[0] & 0x40)? 0xFF:0xEF) & // DR on right button
             ((bytes_fb[0] & 0x80)? 0xFF:0xFB) ; // Center on green button
  m_iButtonChanged = m_iButtonField ^ m_iLastButtonField;
  m_iLastButtonField = m_iButtonField;
}

void InputHandler_PIUIOBUTTON::UpdateLights()
{
}
