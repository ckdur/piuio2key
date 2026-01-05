#include "stdafx.h"
#include "lxio.h"

InputHandler_LXIO* g_ihLXIO;

#define PIULXIO_VENDOR_ID 0x0d2f
#define PIULXIO_PRODUCT_ID 0x1020
#define PIULXIO_PRODUCT_ID_2 0x1040

#define PIULXIO_ENDPOINT_OUT 0x02
#define PIULXIO_ENDPOINT_IN 0x01

bool LXIO::DeviceMatches(int iVID, int iPID)
{
  return iVID == PIULXIO_VENDOR_ID && (iPID == PIULXIO_PRODUCT_ID || iPID == PIULXIO_PRODUCT_ID_2);
}

bool LXIO::Open()
{
  // Open either
  bool ret1 = OpenInternal(PIULXIO_VENDOR_ID, PIULXIO_PRODUCT_ID);
  if(!ret1) ret1 = OpenInternal(PIULXIO_VENDOR_ID, PIULXIO_PRODUCT_ID_2);
  
  return ret1;
}

bool LXIO::Read(uint8_t *pData)
{
  int iExpected = 16;

  int iResult = m_pDriver->InterruptRead(
    0x80 | PIULXIO_ENDPOINT_IN, (char*)pData, iExpected, 1000);

  return iResult == iExpected;
}

bool LXIO::Write(uint8_t* pData)
{
  int iExpected = 16;

  int iResult = m_pDriver->InterruptWrite(
    PIULXIO_ENDPOINT_OUT, (char*)pData, iExpected, 1000);

  return iResult == iExpected;
}

bool InputHandler_LXIO::s_bInitialized = false;

// simple helper function to automatically reopen LXIO if a USB error occurs
static void Reconnect(LXIO &board)
{
  printf("LXIO connection lost! Retrying...");

  while (!board.Open())
  {
    board.Close();
  }

  printf("LXIO reconnected.");
}

InputHandler_LXIO::InputHandler_LXIO()
{
  InputHandler::InputHandler();

  if (s_bInitialized)
  {
    printf("InputHandler_LXIO: Redundant driver loaded. Disabling...");
    return;
  }

  // attempt to connect to the I/O board
  if (!Board.Open())
  {
    printf("InputHandler_LXIO: Could not establish a connection with the I/O device.");
    return;
  }

  // set the relevant global flags (static flag, input type)
  m_bFoundDevice = true;
  s_bInitialized = true;
}

InputHandler_LXIO::~InputHandler_LXIO()
{
  // reset all lights and unclaim the device
  if (m_bFoundDevice)
  {
    Board.Close();

    s_bInitialized = false;
  }
}

void InputHandler_LXIO::InputThreadMain()
{
  UpdateLights();
  HandleInput();
}

void InputHandler_LXIO::HandleInput()
{
  if (!m_bFoundDevice) return;
  // reset our reading data
  m_iInputField = 0x0;
  memset(&m_iInputData, 0, sizeof(m_iInputData) / sizeof(uint32_t));

  uint8_t LXInputData[16];
  uint8_t LXLampData[16];
  memset(LXLampData, 0, 16);
  memcpy(LXLampData, &m_iLightData, 4); // Just copy it

  // Send the lamp data
  while (!Board.Write(LXLampData))
    Reconnect(Board);

  // Read everything
  while (!Board.Read(LXInputData))
    Reconnect(Board);

  // Convert to sensors
  for (int i = 0; i < 4; ++i) {
    uint8_t* inputDataBytes = (uint8_t*)&m_iInputData[i];
    inputDataBytes[0] = ~LXInputData[i];
    inputDataBytes[2] = ~LXInputData[i+4];
    inputDataBytes[1] = ~LXInputData[8];
    inputDataBytes[3] = ~LXInputData[9];
  }

  // combine the read data into a single field
  for (int i = 0; i < 4; ++i)
    m_iInputField |= m_iInputData[i];

  // generate our input events bit field (1 = change, 0 = no change)
  m_iChanged = m_iInputField ^ m_iLastInputField;
  m_iLastInputField = m_iInputField;

  // For the button, lets propagate
  m_iButtonField = 0;
  memcpy(&m_iButtonField, LXInputData + 10, 2);
  m_iButtonField = ~m_iButtonField & 0xFFFF;
  m_iButtonChanged = m_iButtonField ^ m_iLastButtonField;
  m_iLastButtonField = m_iButtonField;
}

void InputHandler_LXIO::UpdateLights()
{
}

