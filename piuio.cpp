#include "stdafx.h"
#include "PIUIO.h"

InputHandler_PIUIO* g_ihPIUIO;

/* static struct to ensure the USB subsystem is initialized on start */
struct USBInit
{
	USBInit() { usb_init(); usb_find_busses(); usb_find_devices(); }
};

static struct USBInit g_USBInit;

static struct usb_device *FindDevice(int iVendorID, int iProductID)
{
	for (usb_bus *bus = usb_get_busses(); bus; bus = bus->next)
		for (struct usb_device *dev = bus->devices; dev; dev = dev->next)
			if (iVendorID == dev->descriptor.idVendor && iProductID == dev->descriptor.idProduct)
				return dev;

	printf("FindDevice(): no match for VID 0x%04x, PID 0x%04x.", iVendorID, iProductID);
	return NULL;
}

bool USBDriver_Impl::DeviceExists(uint16_t iVendorID, uint16_t iProductID)
{
	return FindDevice(iVendorID, iProductID) != NULL;
}

USBDriver_Impl::USBDriver_Impl()
{
	m_pHandle = NULL;
}

USBDriver_Impl::~USBDriver_Impl()
{
	Close();
}

bool USBDriver_Impl::Open(int iVendorID, int iProductID)
{
	Close();

	if (usb_find_busses() < 0)
	{
		printf("Libusb: usb_find_busses: %s", usb_strerror());
		return false;
	}

	if (usb_find_devices() < 0)
	{
		printf("Libusb: usb_find_devices: %s", usb_strerror());
		return false;
	}

	struct usb_device *dev = FindDevice(iVendorID, iProductID);

	if (dev == NULL)
	{
		printf("Libusb: no match for %04x, %04x.", iVendorID, iProductID);
		return false;
	}

	m_pHandle = usb_open(dev);

	if (m_pHandle == NULL)
	{
		printf("Libusb: usb_open: %s", usb_strerror());
		return false;
	}

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	// The device may be claimed by a kernel driver. Attempt to reclaim it.

	for (unsigned iface = 0; iface < dev->config->bNumInterfaces; iface++)
	{
		int iResult = usb_detach_kernel_driver_np(m_pHandle, iface);

		// no attached driver, no error -- ignore these
		if (iResult == -ENODATA || iResult == 0)
			continue;

		printf("usb_detach_kernel_driver_np: %s\n", usb_strerror());


#ifdef LIBUSB_HAS_GET_DRIVER_NP
		// on EPERM, a driver exists and we can't detach - report which one
		if (iResult == -EPERM)
		{
			char szDriverName[16];
			strcpy(szDriverName, "(unknown)");
			usb_get_driver_np(m_pHandle, iface, szDriverName, 16);

			printf("(cannot detach kernel driver \"%s\")", szDriverName);
		}
#endif	// LIBUSB_HAS_GET_DRIVER_NP

		Close();
		return false;
	}
#endif	// LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP

	if (!SetConfiguration(dev->config->bConfigurationValue))
	{
		printf("Libusb: usb_set_configuration: %s", usb_strerror());
		Close();
		return false;
	}

	// attempt to claim all interfaces for this device
	for (unsigned i = 0; i < dev->config->bNumInterfaces; i++)
	{
		if (!ClaimInterface(i))
		{
			printf("Libusb: usb_claim_interface(%i): %s", i, usb_strerror());
			Close();
			return false;
		}
	}

	return true;
}

void USBDriver_Impl::Close()
{
	// never opened
	if (m_pHandle == NULL)
		return;

	usb_set_altinterface(m_pHandle, 0);
	usb_reset(m_pHandle);
	usb_close(m_pHandle);
	m_pHandle = NULL;
}

int USBDriver_Impl::ControlMessage(int iType, int iRequest, int iValue, int iIndex, char *pData, int iSize, int iTimeout)
{
	return usb_control_msg(m_pHandle, iType, iRequest, iValue, iIndex, pData, iSize, iTimeout);
}

int USBDriver_Impl::BulkRead(int iEndpoint, char *pData, int iSize, int iTimeout)
{
	return usb_bulk_read(m_pHandle, iEndpoint, pData, iSize, iTimeout);
}

int USBDriver_Impl::BulkWrite(int iEndpoint, char *pData, int iSize, int iTimeout)
{
	return usb_bulk_write(m_pHandle, iEndpoint, pData, iSize, iTimeout);
}

int USBDriver_Impl::InterruptRead(int iEndpoint, char *pData, int iSize, int iTimeout)
{
	return usb_interrupt_read(m_pHandle, iEndpoint, pData, iSize, iTimeout);
}

int USBDriver_Impl::InterruptWrite(int iEndpoint, char *pData, int iSize, int iTimeout)
{
	return usb_interrupt_write(m_pHandle, iEndpoint, pData, iSize, iTimeout);
}

bool USBDriver_Impl::SetConfiguration(int iConfig)
{
	return usb_set_configuration(m_pHandle, iConfig) == 0;
}

bool USBDriver_Impl::ClaimInterface(int iInterface)
{
	return usb_claim_interface(m_pHandle, iInterface) == 0;
}

bool USBDriver_Impl::ReleaseInterface(int iInterface)
{
	return usb_release_interface(m_pHandle, iInterface) == 0;
}

const char* USBDriver_Impl::GetError() const
{
	return usb_strerror();
}

USBDriver::USBDriver()
{
	m_pDriver = NULL;
}

USBDriver::~USBDriver()
{
	delete m_pDriver;
}

bool USBDriver::OpenInternal(uint16_t iVendorID, uint16_t iProductID)
{
	Close();

	/* see if this device actually exists before trying to open it */
	if (!USBDriver_Impl::DeviceExists(iVendorID, iProductID))
	{
		printf("USBDriver::OpenInternal(0x%04x, 0x%04x): device does not exist\n", iVendorID, iProductID);
		return false;
	}

	m_pDriver = new USBDriver_Impl();

	/* if !m_pDriver, this build cannot support USB drivers. */
	if (m_pDriver == NULL)
	{
		printf("USBDriver::OpenInternal(): Create failed. (No driver impl?");
		return false;
	}

	return m_pDriver->Open(iVendorID, iProductID);
}

bool USBDriver::Open()
{
	return false;
}

void USBDriver::Close()
{
	if (m_pDriver)
		m_pDriver->Close();
}

const short PIUIO_VENDOR_ID = 0x0547;
const short PIUIO_PRODUCT_ID = 0x1002;

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

bool PIUIO::BulkReadWrite(uint32_t pData[8])
{
	/* XXX: magic number left over from the ITG disassembly */
	int iExpected = 32;

	// this is caught by the r16 kernel hack, using '10011' as
	// a sentinel. the rest of the USB parameters aren't used.
	int iResult = m_pDriver->ControlMessage(0, 0, 0, 0,
		(char*)pData, iExpected, 10011);

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
	m_iLightData = 0x0;
	m_iChanged = 0;
	m_bFoundDevice = false;

	/* if a handler has already been created (e.g. by ScreenArcadeStart)
	* and it has claimed the board, don't try to claim it again. */
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
	m_bShutdown = false;

	m_iInputField = 0;
	m_iLastInputField = 0;

	bool bIsKernel = false;
#ifdef LINUX
	FILE *file;
	file = fopen("/rootfs/stats/patch/modules/usbcore.ko", "rb");
	if (file != NULL)
	{
		fclose(file);
		bIsKernel = true;
	}
#endif

	m_InputType = bIsKernel ? INPUT_KERNEL : INPUT_NORMAL;

	//SetLightsMappings();
}

InputHandler_PIUIO::~InputHandler_PIUIO()
{
	// reset all lights and unclaim the device
	if (m_bFoundDevice)
	{
		Board.Write(0);	// it's okay if this fails
		Board.Close();

		s_bInitialized = false;
	}
}

//void InputHandler_PIUIO::SetLightsMappings()
//{
/*uint32_t iCabinetLights[NUM_CABINET_LIGHTS] =
{
// UL, UR, LL, LR marquee lights
(1 << 23), (1 << 26), (1 << 25), (1 << 24),

// selection buttons (not used), bass lights
0, 0, (1 << 10), (1 << 10)
};

uint32_t iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS] =
{
// Left, Right, Up, Down
{ (1 << 20), (1 << 21), (1 << 18), (1 << 19) },	// Player 1
{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	// Player 2
};*/

/* The coin counter moves halfway if we send bit 4, then the rest of
* the way when we send bit 5. If bit 5 is sent without bit 4 prior,
* the coin counter doesn't do anything, so we just keep it on and
* use bit 4 to pulse. */
//uint32_t iCoinTriggers[2] = { (1 << 27), (1 << 28) };
//}

void InputHandler_PIUIO::InputThreadMain()
{
	UpdateLights();
	HandleInput();

	/* export the I/O values to the helper, for LUA binding */
	//MK6Helper::Import( m_iInputData, m_iLightData );
}
void InputHandler_PIUIO::HandleInput()
{
	if (!m_bFoundDevice) return;
	// reset our reading data
	m_iInputField = 0x0;
	memset(&m_iInputData, 0, sizeof(m_iInputData) / sizeof(uint32_t));

	/* PIU02 only reports one set of sensors (with four total sets) on each
	* I/O request. In order to report all sensors for all arrows, we must:
	*
	* 1. Specify the requested set at bits 15-16 and 31-32 of the output
	* 2. Write lights data and sensor selection to PIUIO
	* 3. Read from PIUIO and save that set of sensor data
	* 4. Repeat 2-3 until all four sets of sensor data are obtained
	* 5. Bitwise OR the sensor data together to produce the final input
	*
	* The R16 kernel hack simply does all of this in kernel space, instead
	* of alternating between kernel space and user space on each call.
	* We pass it an 8-member uint32_t array with the lights data and get
	* input in its place. (Why 8? I have no clue. We just RE'd it...)
	*/

	switch (m_InputType)
	{
	case INPUT_NORMAL:
		/* Normal input: write light data (with requested sensor set);
		* perform one Write/Read cycle to get input; invert. */
	{
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
	}
	break;
	case INPUT_KERNEL:
		/* Kernel input: write light data (with desired sensor set)
		* in array members 0, 2, 4, 6; call BulkReadWrite; invert
		* input and copy it to our central data array. */
	{
		memset(&m_iBulkReadData, 0, sizeof(m_iBulkReadData) / sizeof(uint32_t));

		m_iLightData &= 0xFFFCFFFC;

		for (uint32_t i = 0; i < 4; ++i)
			m_iBulkReadData[i * 2] = m_iLightData | (i | (i << 16));

		Board.BulkReadWrite(m_iBulkReadData);

		/* PIUIO opens high, so invert the input data. */
		for (uint32_t i = 0; i < 4; ++i)
			m_iInputData[i] = ~m_iBulkReadData[i * 2];
	}
	break;
	}

	// combine the read data into a single field
	for (int i = 0; i < 4; ++i)
		m_iInputField |= m_iInputData[i];

	// generate our input events bit field (1 = change, 0 = no change)
	m_iChanged = m_iInputField ^ m_iLastInputField;
	m_iLastInputField = m_iInputField;
}

void InputHandler_PIUIO::UpdateLights()
{
	// NO LIGHTS UPDATING
}
// HERE ENDS LIBUSB