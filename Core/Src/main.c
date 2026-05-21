<<<<<<< HEAD
#include "main.h"
#include "usb_device.h"
#include "usbd_midi.h"

/* ================= USB ================= */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* ================= ADC ================= */
extern ADC_HandleTypeDef hadc1;

/* ================= PROTOTIPOS ================= */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
void Error_Handler(void);

/* ================= MIDI NOTE ================= */
void SendMidiNote(uint8_t note, uint8_t velocity)
{
    uint8_t midiPacket[4] = {0x09, 0x90, note, velocity};

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, midiPacket, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

/* ================= MIDI PITCH ================= */
void SendPitch(uint16_t value)
{
    uint8_t midi = (value * 127) / 4095;
    uint8_t msg[4] = {0x0B, 0xE0, midi, 0x00};

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

/* ================= LEITURA ADC ================= */
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

/* ================= MAIN ================= */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USB_DEVICE_Init();

    HAL_Delay(1500);

    uint8_t lastPA[7] = {1};
    uint8_t lastPB[6] = {1};
    uint8_t lastPB6 = 1;

    uint16_t lastPitch = 0;

    while (1)
    {
        /* ===== PA1–PA7 ===== */
        for (uint8_t i = 0; i < 7; i++)
        {
            GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, (GPIO_PIN_1 << i));

            if (state == GPIO_PIN_RESET && lastPA[i])
                SendMidiNote(60 + i, 127);

            lastPA[i] = state;
        }

        /* ===== PB0–PB5 ===== */
        for (uint8_t i = 0; i < 6; i++)
        {
            GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB, (GPIO_PIN_0 << i));

            if (state == GPIO_PIN_RESET && lastPB[i])
                SendMidiNote(70 + i, 127);

            lastPB[i] = state;
        }

        /* ===== PB6 ===== */
        GPIO_PinState pb6 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);

        if (pb6 == GPIO_PIN_RESET && lastPB6)
            SendMidiNote(80, 127);

        lastPB6 = pb6;

        /* ===== PITCH ===== */
        uint16_t pitch = ReadPitch();

        if (abs((int)pitch - (int)lastPitch) > 3)
        {
            SendPitch(pitch);
            lastPitch = pitch;
        }

        HAL_Delay(5);
    }
}

/* ================= SYSTEM CLOCK ================= */
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

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ================= ERROR HANDLER ================= */
void Error_Handler(void)
{
    while (1)
    {
    }
}
=======
#include "main.h"
#include "usb_device.h"
#include "usbd_midi.h"

/* ================= USB ================= */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* ================= ADC ================= */
extern ADC_HandleTypeDef hadc1;

/* ================= PROTOTIPOS ================= */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
void Error_Handler(void);

/* ================= MIDI NOTE ================= */
void SendMidiNote(uint8_t note, uint8_t velocity)
{
    uint8_t midiPacket[4] = {0x09, 0x90, note, velocity};

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, midiPacket, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

/* ================= MIDI PITCH ================= */
void SendPitch(uint16_t value)
{
    uint8_t midi = (value * 127) / 4095;
    uint8_t msg[4] = {0x0B, 0xE0, midi, 0x00};

    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

/* ================= LEITURA ADC ================= */
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

/* ================= MAIN ================= */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USB_DEVICE_Init();

    HAL_Delay(1500);

    uint8_t lastPA[7] = {1};
    uint8_t lastPB[6] = {1};
    uint8_t lastPB6 = 1;

    uint16_t lastPitch = 0;

    while (1)
    {
        /* ===== PA1–PA7 ===== */
        for (uint8_t i = 0; i < 7; i++)
        {
            GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, (GPIO_PIN_1 << i));

            if (state == GPIO_PIN_RESET && lastPA[i])
                SendMidiNote(60 + i, 127);

            lastPA[i] = state;
        }

        /* ===== PB0–PB5 ===== */
        for (uint8_t i = 0; i < 6; i++)
        {
            GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB, (GPIO_PIN_0 << i));

            if (state == GPIO_PIN_RESET && lastPB[i])
                SendMidiNote(70 + i, 127);

            lastPB[i] = state;
        }

        /* ===== PB6 ===== */
        GPIO_PinState pb6 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);

        if (pb6 == GPIO_PIN_RESET && lastPB6)
            SendMidiNote(80, 127);

        lastPB6 = pb6;

        /* ===== PITCH ===== */
        uint16_t pitch = ReadPitch();

        if (abs((int)pitch - (int)lastPitch) > 3)
        {
            SendPitch(pitch);
            lastPitch = pitch;
        }

        HAL_Delay(5);
    }
}

/* ================= SYSTEM CLOCK ================= */
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

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ================= ERROR HANDLER ================= */
void Error_Handler(void)
{
    while (1)
    {
    }
}
>>>>>>> 2bb48e3d884a1ff80e2ba85278c38ff1500f3e2c
