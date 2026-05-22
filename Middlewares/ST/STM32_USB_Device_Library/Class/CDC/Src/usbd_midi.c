#include "usbd_midi.h"
#include "usbd_ctlreq.h"

#define USB_MIDI_CONFIG_DESC_SIZ 101

__ALIGN_BEGIN static uint8_t USBD_MIDI_CfgDesc[USB_MIDI_CONFIG_DESC_SIZ] __ALIGN_END =
{
    0x09,
    USB_DESC_TYPE_CONFIGURATION,
    0x65, 0x00,
    0x02,
    0x01,
    0x00,
    0x80,
    0x32,

    0x09,
    USB_DESC_TYPE_INTERFACE,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,

    0x09,
    0x24,
    0x01,
    0x00, 0x01,
    0x09, 0x00,
    0x01,
    0x01,

    0x09,
    USB_DESC_TYPE_INTERFACE,
    0x01,
    0x00,
    0x02,
    0x01,
    0x03,
    0x00,
    0x00,

    0x07,
    0x24,
    0x01,
    0x00, 0x01,
    0x41, 0x00,

    0x06,
    0x24,
    0x02,
    0x01,
    0x01,
    0x00,

    0x06,
    0x24,
    0x02,
    0x02,
    0x02,
    0x00,

    0x09,
    0x24,
    0x03,
    0x01,
    0x03,
    0x01,
    0x02,
    0x01,
    0x00,

    0x09,
    0x24,
    0x03,
    0x02,
    0x04,
    0x01,
    0x01,
    0x01,
    0x00,

    0x09,
    USB_DESC_TYPE_ENDPOINT,
    MIDI_OUT_EP,
    0x02,
    0x40, 0x00,
    0x00,
    0x00,
    0x00,

    0x05,
    0x25,
    0x01,
    0x01,
    0x01,

    0x09,
    USB_DESC_TYPE_ENDPOINT,
    MIDI_IN_EP,
    0x02,
    0x40, 0x00,
    0x00,
    0x00,
    0x00,

    0x05,
    0x25,
    0x01,
    0x01,
    0x03
};

static uint8_t *txBuffer;
static uint32_t txLength;

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_MIDI_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_MIDI_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_MIDI_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t *USBD_MIDI_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_MIDI_GetDeviceQualifierDescriptor(uint16_t *length);

USBD_ClassTypeDef USBD_MIDI =
{
    USBD_MIDI_Init,
    USBD_MIDI_DeInit,
    NULL,
    NULL,
    NULL,
    USBD_MIDI_DataIn,
    USBD_MIDI_DataOut,
    NULL,
    NULL,
    NULL,
    USBD_MIDI_GetFSCfgDesc,
    USBD_MIDI_GetFSCfgDesc,
    USBD_MIDI_GetFSCfgDesc,
    USBD_MIDI_GetDeviceQualifierDescriptor,
};

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    UNUSED(cfgidx);

    USBD_LL_OpenEP(pdev, MIDI_IN_EP, USBD_EP_TYPE_BULK, 64);
    pdev->ep_in[MIDI_IN_EP & 0xFU].is_used = 1U;

    USBD_LL_OpenEP(pdev, MIDI_OUT_EP, USBD_EP_TYPE_BULK, 64);
    pdev->ep_out[MIDI_OUT_EP & 0xFU].is_used = 1U;

    return USBD_OK;
}

static uint8_t USBD_MIDI_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    UNUSED(cfgidx);

    USBD_LL_CloseEP(pdev, MIDI_IN_EP);
    USBD_LL_CloseEP(pdev, MIDI_OUT_EP);

    return USBD_OK;
}

static uint8_t USBD_MIDI_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    UNUSED(pdev);
    UNUSED(epnum);

    return USBD_OK;
}

static uint8_t USBD_MIDI_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    UNUSED(pdev);
    UNUSED(epnum);

    return USBD_OK;
}

static uint8_t *USBD_MIDI_GetFSCfgDesc(uint16_t *length)
{
    *length = sizeof(USBD_MIDI_CfgDesc);
    return USBD_MIDI_CfgDesc;
}

__ALIGN_BEGIN static uint8_t USBD_MIDI_DeviceQualifierDesc[10] __ALIGN_END =
{
    0x0A,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

static uint8_t *USBD_MIDI_GetDeviceQualifierDescriptor(uint16_t *length)
{
    *length = sizeof(USBD_MIDI_DeviceQualifierDesc);
    return USBD_MIDI_DeviceQualifierDesc;
}

uint8_t USBD_MIDI_SetTxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff,
                              uint32_t length)
{
    UNUSED(pdev);

    txBuffer = pbuff;
    txLength = length;

    return USBD_OK;
}

uint8_t USBD_MIDI_TransmitPacket(USBD_HandleTypeDef *pdev)
{
    return USBD_LL_Transmit(pdev,
                            MIDI_IN_EP,
                            txBuffer,
                            txLength);
}
