/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_midi.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
extern ADC_HandleTypeDef hadc1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SendMidiNote(uint8_t note, uint8_t velocity);
void SendPitch(uint16_t value);
uint16_t ReadPitch(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void SendMidiNote(uint8_t note, uint8_t velocity)
{
    uint8_t midiPacket[4] = {0x09, 0x90, note, velocity};
    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, midiPacket, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

void SendPitch(uint16_t value)
{
    uint8_t midi = (value * 127) / 4095;
    uint8_t msg[4] = {0x0B, 0xE0, midi, 0x00};
    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

uint16_t ReadPitch(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    return sum / 8;
}



/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  HAL_Delay(1500);
  uint8_t lastPA[7] = {1,1,1,1,1,1,1};
  uint8_t lastPB[6] = {1,1,1,1,1,1};
  uint8_t lastPB6 = 1;
  uint16_t lastPitch = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    /* USER CODE BEGIN 3 */
    for (uint8_t i = 0; i < 7; i++)
    {
        GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, (GPIO_PIN_1 << i));
        if (state == GPIO_PIN_RESET && lastPA[i])
            SendMidiNote(60 + i, 127);
        lastPA[i] = state;
    }

    for (uint8_t i = 0; i < 6; i++)
    {
        GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB, (GPIO_PIN_0 << i));
        if (state == GPIO_PIN_RESET && lastPB[i])
            SendMidiNote(70 + i, 127);
        lastPB[i] = state;
    }

    GPIO_PinState pb6 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
    if (pb6 == GPIO_PIN_RESET && lastPB6)
        SendMidiNote(80, 127);
    lastPB6 = pb6;

    uint16_t pitch = ReadPitch();
    if (abs((int)pitch - (int)lastPitch) > 3)
    {
        SendPitch(pitch);
        lastPitch = pitch;
    }
    HAL_Delay(5);
    /* USER CODE END 3 */
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
