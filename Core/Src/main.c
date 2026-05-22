/* USER CODE BEGIN Header */
/**
  CDJ MIDI CONTROLLER - FIX LCD + MIDI + ADC PA1
*/
/* USER CODE END Header */

#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "usb_device.h"
#include "gpio.h"

#include "lcd_i2c.h"
#include "usbd_midi.h"

#include <stdio.h>

/* ================= USB ================= */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* ================= STATE ================= */
uint32_t adc_value = 0;
float pitch_filtered = 0;

/* ================= PROTOTYPE ================= */
void SystemClock_Config(void);

/* ================= MIDI ================= */
void SendPitchBend(int value)
{
    uint8_t msg[4];

    msg[0] = 0x0E;
    msg[1] = 0xE0;
    msg[2] = value & 0x7F;
    msg[3] = (value >> 7) & 0x7F;

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

/* ================= MAIN ================= */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_USB_DEVICE_Init();

    HAL_Delay(300);

    lcd_init();
    lcd_clear();

    /* HEADER FIXO */
    lcd_put_cur(0, 0);
    lcd_send_string("   DJ DEIVIDI   ");

    while (1)
    {
        /* ================= ADC PA1 ================= */
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        adc_value = HAL_ADC_GetValue(&hadc1);

        float target = ((float)adc_value / 4095.0f) * 16383.0f;

        pitch_filtered = (pitch_filtered * 0.85f) + (target * 0.15f);

        SendPitchBend((int)pitch_filtered);

        /* ================= LCD LINE 2 FIX ================= */
        int percent = (int)((pitch_filtered / 16383.0f) * 100.0f);

        char line2[17];

        /* 16 caracteres fixos + % sempre no final */
        snprintf(line2, sizeof(line2), "            %3d%%", percent);

        lcd_put_cur(1, 0);
        lcd_send_string(line2);

        HAL_Delay(50); // estabilidade LCD (IMPORTANTE)
    }
}

/* ================= CLOCK ================= */
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

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ================= ERROR ================= */
void Error_Handler(void)
{
    while (1)
    {
    }
}
