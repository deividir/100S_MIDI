#ifndef __USBD_MIDI_H
#define __USBD_MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#define MIDI_IN_EP                                     0x81U
#define MIDI_OUT_EP                                    0x01U

#define MIDI_DATA_FS_MAX_PACKET_SIZE                   64U

#define USB_MIDI_CONFIG_DESC_SIZ                       101U

#define MIDI_DATA_FS_IN_PACKET_SIZE                    MIDI_DATA_FS_MAX_PACKET_SIZE
#define MIDI_DATA_FS_OUT_PACKET_SIZE                   MIDI_DATA_FS_MAX_PACKET_SIZE

typedef struct
{
  uint32_t data[MIDI_DATA_FS_MAX_PACKET_SIZE / 4U];

  uint8_t  *RxBuffer;
  uint8_t  *TxBuffer;

  uint32_t RxLength;
  uint32_t TxLength;

  __IO uint32_t TxState;
  __IO uint32_t RxState;

} USBD_MIDI_HandleTypeDef;

typedef struct _USBD_MIDI_Itf
{
  int8_t (* Init)(void);
  int8_t (* DeInit)(void);
  int8_t (* Receive)(uint8_t *Buf, uint32_t *Len);

} USBD_MIDI_ItfTypeDef;

extern USBD_ClassTypeDef USBD_MIDI;

#define USBD_MIDI_CLASS &USBD_MIDI

uint8_t USBD_MIDI_RegisterInterface(USBD_HandleTypeDef *pdev,
                                    USBD_MIDI_ItfTypeDef *fops);

uint8_t USBD_MIDI_SetTxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff,
                              uint32_t length);

uint8_t USBD_MIDI_TransmitPacket(USBD_HandleTypeDef *pdev);

uint8_t USBD_MIDI_SetRxBuffer(USBD_HandleTypeDef *pdev,
                              uint8_t *pbuff);

uint8_t USBD_MIDI_ReceivePacket(USBD_HandleTypeDef *pdev);

#ifdef __cplusplus
}
#endif

#endif
