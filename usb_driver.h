#pragma once
#include "stdafx.h"

// ** HERE BEGINS LIBUSB DEFS
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
