/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Professional MIDI Controller v2 (Anti GPIO Ghost + Stable LCD)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "usb_device.h"
#include "adc.h"
#include "i2c.h"
#include "gpio.h"

#include "lcd_i2c.h"
#include "usbd_midi.h"

#include <stdio.h>
#include <string.h>

/* ==========================
   PROTOTYPES
   ========================== */
void SystemClock_Config(void);
void Error_Handler(void);

extern USBD_HandleTypeDef hUsbDeviceFS;

/* ==========================
   GPIO NOISE FILTER (ANTI GHOST)
   ========================== */

uint8_t GPIO_ReadStable(GPIO_TypeDef* port, uint16_t pin)
{
    uint8_t a = HAL_GPIO_ReadPin(port, pin);
    HAL_Delay(2);
    uint8_t b = HAL_GPIO_ReadPin(port, pin);
    HAL_Delay(2);
    uint8_t c = HAL_GPIO_ReadPin(port, pin);

    if (a == b && b == c)
        return a;

    return GPIO_PIN_SET; // default pull-up state
}

/* ==========================
   BUTTON SYSTEM (ROBUST DEBOUNCE)
   ========================== */

#define DEBOUNCE_TIME 40

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    uint8_t stableState;
    uint8_t lastState;
    uint32_t lastChangeTime;
} Button;

uint8_t Button_Read(Button *btn)
{
    uint8_t read = GPIO_ReadStable(btn->port, btn->pin);

    if (read != btn->lastState)
    {
        btn->lastChangeTime = HAL_GetTick();
        btn->lastState = read;
    }

    if ((HAL_GetTick() - btn->lastChangeTime) > DEBOUNCE_TIME)
    {
        if (btn->stableState != read)
        {
            btn->stableState = read;

            if (read == GPIO_PIN_RESET)
                return 1;
        }
    }

    return 0;
}

/* ==========================
   MIDI
   ========================== */

void SendMidiNote(uint8_t note, uint8_t velocity)
{
    uint8_t msg[4];

    msg[0] = 0x09;
    msg[1] = 0x90 | 0x0F;
    msg[2] = note;
    msg[3] = velocity;

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);

    HAL_Delay(2);

    msg[0] = 0x08;
    msg[1] = 0x80 | 0x0F;
    msg[2] = note;
    msg[3] = 0x00;

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

/* ==========================
   MAIN
   ========================== */

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_USB_DEVICE_Init();

    HAL_Delay(200);

    lcd_init();
    lcd_clear();

    lcd_put_cur(0,0);
    lcd_send_string("100S MIDI       ");

    lcd_put_cur(1,0);
    lcd_send_string("DJ Deividi      ");

    HAL_Delay(1500);
    lcd_clear();

    /* ================= BUTTON MAP ================= */

    Button btn_stop    = {GPIOA, GPIO_PIN_2, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_searchp = {GPIOA, GPIO_PIN_3, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_searchm = {GPIOA, GPIO_PIN_4, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_trackp  = {GPIOA, GPIO_PIN_5, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_trackm  = {GPIOA, GPIO_PIN_6, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_jet     = {GPIOA, GPIO_PIN_7, GPIO_PIN_SET, GPIO_PIN_SET, 0};

    Button btn_zip     = {GPIOB, GPIO_PIN_0, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_wah     = {GPIOB, GPIO_PIN_1, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_hold    = {GPIOB, GPIO_PIN_2, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_tempo   = {GPIOB, GPIO_PIN_3, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_eject   = {GPIOB, GPIO_PIN_4, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_mt      = {GPIOB, GPIO_PIN_5, GPIO_PIN_SET, GPIO_PIN_SET, 0};
    Button btn_play    = {GPIOB, GPIO_PIN_6, GPIO_PIN_SET, GPIO_PIN_SET, 0};

    while (1)
    {
        if (Button_Read(&btn_stop))    { lcd_clear(); lcd_send_string("STOP");    SendMidiNote(60,127); }
        if (Button_Read(&btn_searchp)) { lcd_clear(); lcd_send_string("SEARCH+"); SendMidiNote(61,127); }
        if (Button_Read(&btn_searchm)) { lcd_clear(); lcd_send_string("SEARCH-"); SendMidiNote(62,127); }
        if (Button_Read(&btn_trackp))  { lcd_clear(); lcd_send_string("TRACK+");  SendMidiNote(63,127); }
        if (Button_Read(&btn_trackm))  { lcd_clear(); lcd_send_string("TRACK-");  SendMidiNote(64,127); }
        if (Button_Read(&btn_jet))     { lcd_clear(); lcd_send_string("JET");     SendMidiNote(65,127); }
        if (Button_Read(&btn_zip))     { lcd_clear(); lcd_send_string("ZIP");     SendMidiNote(66,127); }
        if (Button_Read(&btn_wah))     { lcd_clear(); lcd_send_string("WAH");     SendMidiNote(67,127); }
        if (Button_Read(&btn_hold))    { lcd_clear(); lcd_send_string("HOLD");    SendMidiNote(68,127); }
        if (Button_Read(&btn_tempo))   { lcd_clear(); lcd_send_string("TEMPO");   SendMidiNote(69,127); }
        if (Button_Read(&btn_eject))   { lcd_clear(); lcd_send_string("EJECT");   SendMidiNote(70,127); }
        if (Button_Read(&btn_mt))      { lcd_clear(); lcd_send_string("MT");      SendMidiNote(71,127); }
        if (Button_Read(&btn_play))    { lcd_clear(); lcd_send_string("PLAY");    SendMidiNote(72,127); }

        lcd_put_cur(1,0);
        lcd_send_string("USB MIDI CH16   ");

        HAL_Delay(5);
    }
}

/* ==========================
   CLOCK
   ========================== */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ==========================
   ERROR
   ========================== */

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
