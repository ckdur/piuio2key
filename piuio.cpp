#include "stdafx.h"
#include "piuio.h"

InputHandler_PIUIO* g_ihPIUIO;

#define PIUIO_VENDOR_ID 0x0547
#define PIUIO_PRODUCT_ID 0x1002
#define PIUIOBUTTON_VENDOR_ID 0x0D2F
#define PIUIOBUTTON_PRODUCT_ID 0x1010

/* proprietary (arbitrary?) request PIUIO requires to handle I/O */
const short PIUIO_CTL_REQ = 0xAE;

/* timeout value for read/writes, in microseconds (so, 10 ms) */
const int REQ_TIMEOUT = 10000;

bool PIUIO::DeviceMatches(int iVID, int iPID)
{
  return iVID == PIUIO_VENDOR_ID && iPID == PIUIO_PRODUCT_ID;
}

bool PIUIO::Open()
{
  return OpenInternal(PIUIO_VENDOR_ID, PIUIO_PRODUCT_ID);
}

bool PIUIO::Read(uint32_t *pData)
{
  /* XXX: magic number left over from the ITG disassembly */
  int iExpected = 8;

  int iResult = m_pDriver->ControlMessage(
    USB_DIR_IN | USB_TYPE_VENDOR, PIUIO_CTL_REQ,
    0, 0, (char*)pData, iExpected, REQ_TIMEOUT);

  return iResult == iExpected;
}

bool PIUIO::Write(const uint32_t iData)
{
  /* XXX: magic number left over from the ITG disassembly */
  int iExpected = 8;

  int iResult = m_pDriver->ControlMessage(
    USB_DIR_OUT | USB_TYPE_VENDOR, PIUIO_CTL_REQ,
    0, 0, (char*)&iData, iExpected, REQ_TIMEOUT);

  return iResult == iExpected;
}

bool InputHandler_PIUIO::s_bInitialized = false;

// simple helper function to automatically reopen PIUIO if a USB error occurs
static void Reconnect(PIUIO &board)
{
  printf("PIUIO connection lost! Retrying...");

  while (!board.Open())
  {
    board.Close();
  }

  printf("PIUIO reconnected.");
}

InputHandler_PIUIO::InputHandler_PIUIO()
{
  InputHandler::InputHandler();

  if (s_bInitialized)
  {
    printf("InputHandler_PIUIO: Redundant driver loaded. Disabling...");
    return;
  }

  // attempt to connect to the I/O board
  if (!Board.Open())
  {
    printf("InputHandler_PIUIO: Could not establish a connection with the I/O device.");
    return;
  }

  // set the relevant global flags (static flag, input type)
  m_bFoundDevice = true;
  s_bInitialized = true;
}

InputHandler_PIUIO::~InputHandler_PIUIO()
{
  // reset all lights and unclaim the device
  if (m_bFoundDevice)
  {
    Board.Write(0);  // it's okay if this fails
    Board.Close();

    s_bInitialized = false;
  }
}

void InputHandler_PIUIO::InputThreadMain()
{
  UpdateLights();
  HandleInput();
}

void InputHandler_PIUIO::HandleInput()
{
  if (!m_bFoundDevice) return;
  // reset our reading data
  m_iInputField = 0x0;
  memset(&m_iInputData, 0, sizeof(m_iInputData) / sizeof(uint32_t));

  for (uint32_t i = 0; i < 4; ++i)
  {
    // write a set of sensors to request
    m_iLightData &= 0xFFFCFFFC;
    m_iLightData |= (i | (i << 16));

    // request this set of sensors
    while (!Board.Write(m_iLightData))
      Reconnect(Board);

    m_iInputData[i] = 0x0;
    // read from this set of sensors
    while (!Board.Read(&m_iInputData[i]))
      Reconnect(Board);

    m_iInputData[i] = ~m_iInputData[i];
  }

  // combine the read data into a single field
  for (int i = 0; i < 4; ++i)
    m_iInputField |= m_iInputData[i];

  // generate our input events bit field (1 = change, 0 = no change)
  m_iChanged = m_iInputField ^ m_iLastInputField;
  m_iLastInputField = m_iInputField;

  // For the button, there is nothing
  m_iButtonChanged = 0;
  m_iButtonField = 0;
  m_iLastButtonField = 0;
}

void InputHandler_PIUIO::UpdateLights()
{
}
