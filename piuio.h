#pragma once
#include "stdafx.h"

// ** HERE BEGINS LIBUSB DEFS

// Copied from: OpenITG
#define USB_DIR_OUT	0x00
#define USB_DIR_IN	0x80

#define USB_TYPE_STANDARD	(0x00 << 5)
#define USB_TYPE_CLASS		(0x01 << 5)
#define USB_TYPE_VENDOR		(0x02 << 5)
#define USB_TYPE_RESERVED	(0x03 << 5)

#define USB_RECIP_DEVICE	0x00
#define USB_RECIP_INTERFACE	0x01
#define USB_RECIP_ENDPOINT	0x02
#define USB_RECIP_OTHER		0x03

struct usb_dev_handle;

#define HAS_USBDRIVER_IMPL_LIBUSB

class USBDriver_Impl
{
public:
	static bool DeviceExists(uint16_t iVendorID, uint16_t iProductID);

	USBDriver_Impl();
	~USBDriver_Impl();

	bool Open(int iVendorID, int iProductID);
	void Close();

	int ControlMessage(int iType, int iRequest, int iValue, int iIndex, char *pData, int iSize, int iTimeout);

	int BulkRead(int iEndpoint, char *pData, int iSize, int iTimeout);
	int BulkWrite(int iEndpoint, char *pData, int iSize, int iTimeout);

	int InterruptRead(int iEndpoint, char *pData, int iSize, int iTimeout);
	int InterruptWrite(int iEndpoint, char *pData, int iSize, int iTimeout);

	virtual const char *GetError() const;

protected:
	bool SetConfiguration(int iConfig);

	bool ClaimInterface(int iInterface);
	bool ReleaseInterface(int iInterface);

private:
	usb_dev_handle *m_pHandle;
};

class USBDriver
{
public:
	USBDriver();
	virtual ~USBDriver();

	bool OpenInternal(uint16_t iVendorID, uint16_t iProductID);

	virtual bool Open();
	virtual void Close();

protected:
	USBDriver_Impl* m_pDriver;
};

class PIUIO : public USBDriver
{
public:
	/* returns true if the VID/PID match PIUIO. */
	static bool DeviceMatches(int iVendorID, int iProductID);

	bool Open();

	bool Read(uint32_t *pData);
	bool Write(const uint32_t iData);
	bool BulkReadWrite(uint32_t pData[8]);
};

class InputHandler_PIUIO
{
public:
	InputHandler_PIUIO();
	~InputHandler_PIUIO();

	// input field is a combination of each sensor set in m_iInputData
	uint32_t m_iInputField, m_iLastInputField, m_iChanged;
	uint32_t m_iInputData[4];

	// used for the r16 kernel hack and translates to m_iInputData
	uint32_t m_iBulkReadData[8];

	// data that will be written to PIUIO (lights, sensors)
	uint32_t m_iLightData;

	bool m_bFoundDevice;

	void InputThreadMain();

private:
	/* Allow only one handler to control the board at a time. More than one
	* handler may be loaded due to startup and Static.ini interactions, so
	* we need this to prevent obscure I/O problems. */
	static bool s_bInitialized;

	PIUIO Board;

	//void SetLightsMappings();

	/* Low-level processing changes if using the R16 kernel hack. */
	enum InputType { INPUT_NORMAL, INPUT_KERNEL } m_InputType;

	void HandleInput();
	void UpdateLights();

	bool m_bShutdown;
};

extern InputHandler_PIUIO* g_ihPIUIO;