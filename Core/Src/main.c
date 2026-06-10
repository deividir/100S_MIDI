/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics. All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component. If no LICENSE file comes with
  * this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd_i2c.h"
#include "usbd_midi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PITCH_THRESHOLD 20 // Sensibilidade do Pitch
#define PITCH_SEND_INTERVAL 10 // Intervalo mínimo para enviar Pitch Bend (ms)
#define BUTTON_DEBOUNCE_TIME 50 // Reduzido para maior precisão em botões momentâneos (ms)

// VELOCIDADES DOS SCROLLS
#define SCROLL_TEXT_INTERVAL 350  // Velocidade do texto subindo/andando (ms)
#define SCROLL_PLAY_INTERVAL 200  // Velocidade do triângulo do Play correndo (ms)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* ================= USB ================= */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* ================= PITCH ================= */
uint32_t adc_value = 0;
float pitch_filtered = 0;
int last_pitch_sent = -1;

/* ================= ENCODER ================= */
int32_t last_encoder = 0;

/* ================= BUTTONS ================= */
// Play Button (PB6)
static uint8_t last_play_state = GPIO_PIN_SET;
static uint32_t last_play_time = 0;

// Cue Button (PA2)
static uint8_t last_cue_state = GPIO_PIN_SET;
static uint32_t last_cue_time = 0;

// Track+ Button (PA5)
static uint8_t last_track_plus_state = GPIO_PIN_SET;
static uint32_t last_track_plus_time = 0;

// Track- Button (PA6)
static uint8_t last_track_minus_state = GPIO_PIN_SET;
static uint32_t last_track_minus_time = 0;

// Search+ Button (PA3)
static uint8_t last_search_plus_state = GPIO_PIN_SET;
static uint32_t last_search_plus_time = 0;

// Search- Button (PA4)
static uint8_t last_search_minus_state = GPIO_PIN_SET;
static uint32_t last_search_minus_time = 0;

// MT - Master Tempo Button (PA8)
static uint8_t last_mt_state = GPIO_PIN_SET;
static uint32_t last_mt_time = 0;

// Eject Button (PA9)
static uint8_t last_eject_state = GPIO_PIN_SET;
static uint32_t last_eject_time = 0;

// Tempo Button (PB3)
static uint8_t last_tempo_state = GPIO_PIN_SET;
static uint32_t last_tempo_time = 0;

// Hold Button (PB7)
/*static uint8_t last_hold_state = GPIO_PIN_SET;
static uint32_t last_hold_time = 0; */

/* ================= DECK CONTROL ================= */
uint8_t current_deck = 0;
static uint8_t hold_pressed = 0;
static uint32_t hold_press_start = 0;
static uint8_t hold_longpress_done = 0;

// Wah Button (PB1)
static uint8_t last_wah_state = GPIO_PIN_SET;
static uint32_t last_wah_time = 0;

// Zip Button (PB0)
static uint8_t last_zip_state = GPIO_PIN_SET;
static uint32_t last_zip_time = 0;

// Jet Button (PA7)
static uint8_t last_jet_state = GPIO_PIN_SET;
static uint32_t last_jet_time = 0;

/* ================= LCD SCROLL CONTROLS ================= */
static uint32_t last_scroll_text_time = 0;
static uint32_t last_scroll_play_time = 0;
static int scroll_text_position = 0;
static int scroll_play_position = 0;

// Texto com espaços antes e depois para dar o efeito de entrar e sair perfeitamente da tela
const char texto_nome[] = "                DJ Deividi                ";
const int tamanho_texto = sizeof(texto_nome) - 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SendCC(uint8_t cc, uint8_t value);
void SendNoteOn(uint8_t note);
void SendNoteOff(uint8_t note);
void Update_LCD_Scrolls(uint32_t now);
void My_LCD_Create_Char(uint8_t location, uint8_t *charmap);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SendCC(uint8_t cc, uint8_t value)
{
    uint8_t msg[4] = {0x0B, current_deck ? 0xB1 : 0xB0, cc, value & 0x7F};
    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

void SendNoteOn(uint8_t note)
{
    uint8_t msg[4] = {0x09, current_deck ? 0x91 : 0x90, note, 127};
    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

void SendNoteOff(uint8_t note)
{
    uint8_t msg[4] = {0x08, current_deck ? 0x81 : 0x80, note, 0};
    USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
    USBD_MIDI_TransmitPacket(&hUsbDeviceFS);
}

// FUNÇÃO PARA CRIAR CARACTERE PERSONALIZADO CASO A BIBLIOTECA NÃO TENHA
void My_LCD_Create_Char(uint8_t location, uint8_t *charmap)
{
    location &= 0x07; // O LCD só aceita posições de 0 a 7
    lcd_send_cmd(0x40 + (location << 3)); // Aponta para a memória CGRAM do LCD
    for (int i = 0; i < 8; i++) {
        lcd_send_data(charmap[i]); // Escreve as 8 linhas do desenho
    }
}

// FUNÇÃO DE SCROLL DUPLO E INDEPENDENTE
void Update_LCD_Scrolls(uint32_t now)
{
    /* ----- SCROLL DA LINHA 1: TEXTO (DJ Deividi) ----- */
    if (now - last_scroll_text_time >= SCROLL_TEXT_INTERVAL)
    {
        last_scroll_text_time = now;
        char janela_texto[17];

        for (int i = 0; i < 16; i++) {
            janela_texto[i] = texto_nome[(scroll_text_position + i) % tamanho_texto];
        }
        janela_texto[16] = '\0';

        lcd_put_cur(0, 0);
        lcd_send_string(janela_texto);

        scroll_text_position++;
        if (scroll_text_position >= tamanho_texto) {
            scroll_text_position = 0;
        }
    }

    /* ----- SCROLL DA LINHA 2: TRIÂNGULO DO PLAY CORRENDO ----- */
    if (now - last_scroll_play_time >= SCROLL_PLAY_INTERVAL)
    {
        last_scroll_play_time = now;

        lcd_put_cur(1, 0);
        lcd_send_string("                ");

        lcd_put_cur(1, scroll_play_position);
        lcd_send_data(0); // Manda desenhar o caractere customizado 0

        scroll_play_position++;
        if (scroll_play_position >= 16)
        {
            scroll_play_position = 0;
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  HAL_Delay(500);

  /* START ENCODER */
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

  last_encoder = (int32_t)(int16_t)__HAL_TIM_GET_COUNTER(&htim3);

  // INICIALIZA O LCD
  lcd_init();
  lcd_clear();
  lcd_put_cur(0,0);
  lcd_send_string("DECK A");
  lcd_put_cur(1,0);
  lcd_send_string("DJ Deividi");

  // Definição binária do triângulo clássico do PLAY
  uint8_t simbolo_play[8] = {
      0x10,  // ■ □ □ □ □
      0x18,  // ■ ■ □ □ □
      0x1C,  // ■ ■ ■ □ □
      0x1E,  // ■ ■ ■ ■ □
      0x1C,  // ■ ■ ■ □ □
      0x18,  // ■ ■ □ □ □
      0x10,  // ■ □ □ □ □
      0x00   // □ □ □ □ □
  };

  // Usando a nossa função segura que funciona em qualquer biblioteca LCD I2C
  My_LCD_Create_Char(0, simbolo_play);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();


    /* ================= PITCH (ADC + FILTER) ================= */
    uint32_t soma = 0;
    for(int i = 0; i < 16; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        soma += HAL_ADC_GetValue(&hadc1);
    }
    adc_value = soma / 16;

    float target = 16383.0f - (((float)adc_value / 4095.0f) * 16383.0f);
    pitch_filtered = (pitch_filtered * 0.95f) + (target * 0.05f);
    int pitch_value = (int)pitch_filtered;

    static uint32_t last_pitch_time = 0;
    if((last_pitch_sent < 0 || abs(pitch_value - last_pitch_sent) > PITCH_THRESHOLD)
        && (now - last_pitch_time > PITCH_SEND_INTERVAL))
    {
        uint8_t msg[4];
        msg[0] = 0x0E;
        msg[1] = current_deck ? 0xE1 : 0xE0;
        msg[2] = pitch_value & 0x7F;
        msg[3] = (pitch_value >> 7) & 0x7F;

        USBD_MIDI_SetTxBuffer(&hUsbDeviceFS, msg, 4);
        USBD_MIDI_TransmitPacket(&hUsbDeviceFS);

        last_pitch_sent = pitch_value;
        last_pitch_time = now;
    }

    /* ================= JOG (TIM3 ENCODER) ================= */
    int32_t encoder = (int32_t)(int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    int32_t diff = encoder - last_encoder;

    if(diff != 0)
    {
        int direction = (diff > 0) ? 1 : -1;
        int speed = abs(diff);
        if(speed > 10) speed = 10;

        int value = 64 + (direction * speed);
        if(value < 1)   value = 1;
        if(value > 127) value = 127;

        SendCC(20, value);
        last_encoder = encoder;
    }

    /* ================= PLAY ================= */
    uint8_t play_state = HAL_GPIO_ReadPin(PLAY_GPIO_Port, PLAY_Pin);
    if((now - last_play_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(play_state != last_play_state)
        {
            if(play_state == GPIO_PIN_RESET) {
                SendNoteOn(60);
            } else {
                SendNoteOff(60);
            }

            last_play_time = now;
            last_play_state = play_state;
        }
    }

    /* ================= CUE ================= */
    uint8_t cue_state = HAL_GPIO_ReadPin(STOP_GPIO_Port, STOP_Pin);
    if((now - last_cue_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(cue_state != last_cue_state)
        {
            if(cue_state == GPIO_PIN_RESET) {
                SendNoteOn(61);
            } else {
                SendNoteOff(61);
            }

            last_cue_time = now;
            last_cue_state = cue_state;
        }
    }

    /* ================= TRACK+ (PA5) ================= */
    uint8_t track_plus_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
    if((now - last_track_plus_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(track_plus_state != last_track_plus_state)
        {
            if(track_plus_state == GPIO_PIN_RESET) {
                SendNoteOn(62);
            } else {
                SendNoteOff(62);
            }
            last_track_plus_time = now;
            last_track_plus_state = track_plus_state;
        }
    }

    /* ================= TRACK- (PA6) ================= */
    uint8_t track_minus_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
    if((now - last_track_minus_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(track_minus_state != last_track_minus_state)
        {
            if(track_minus_state == GPIO_PIN_RESET) {
                SendNoteOn(63);
            } else {
                SendNoteOff(63);
            }
            last_track_minus_time = now;
            last_track_minus_state = track_minus_state;
        }
    }

    /* ================= SEARCH+ (PA3) ================= */
    uint8_t search_plus_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);
    if((now - last_search_plus_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(search_plus_state != last_search_plus_state)
        {
            if(search_plus_state == GPIO_PIN_RESET) {
                SendNoteOn(64);
            } else {
                SendNoteOff(64);
            }
            last_search_plus_time = now;
            last_search_plus_state = search_plus_state;
        }
    }

    /* ================= SEARCH- (PA4) ================= */
    uint8_t search_minus_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
    if((now - last_search_minus_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(search_minus_state != last_search_minus_state)
        {
            if(search_minus_state == GPIO_PIN_RESET) {
                SendNoteOn(65);
            } else {
                SendNoteOff(65);
            }
            last_search_minus_time = now;
            last_search_minus_state = search_minus_state;
        }
    }

    /* ================= MT - MASTER TEMPO (PA8) ================= */
    uint8_t mt_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);
    if((now - last_mt_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(mt_state != last_mt_state)
        {
            if(mt_state == GPIO_PIN_RESET) {
                SendNoteOn(66);
            } else {
                SendNoteOff(66);
            }
            last_mt_time = now;
            last_mt_state = mt_state;
        }
    }

    /* ================= EJECT (PA9) ================= */
    uint8_t eject_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9);
    if((now - last_eject_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(eject_state != last_eject_state)
        {
            if(eject_state == GPIO_PIN_RESET) {
                SendNoteOn(67);
            } else {
                SendNoteOff(67);
            }
            last_eject_time = now;
            last_eject_state = eject_state;
        }
    }

    /* ================= TEMPO (PB3) ================= */
    uint8_t tempo_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
    if((now - last_tempo_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(tempo_state != last_tempo_state)
        {
            if(tempo_state == GPIO_PIN_RESET) {
                SendNoteOn(68);
            } else {
                SendNoteOff(68);
            }
            last_tempo_time = now;
            last_tempo_state = tempo_state;
        }
    }

    /* ================= HOLD (PB7) ================= */
    uint8_t hold_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7);

    if(hold_state == GPIO_PIN_RESET && !hold_pressed)
    {
        hold_pressed = 1;
        hold_press_start = now;
        hold_longpress_done = 0;
    }

    if(hold_pressed && !hold_longpress_done && ((now - hold_press_start) >= 2000))
    {
        current_deck ^= 1;

        lcd_clear();
        lcd_put_cur(0,0);
        if(current_deck == 0)
            lcd_send_string("DECK A");
        else
            lcd_send_string("DECK B");

        lcd_put_cur(1,0);
        lcd_send_string("DJ Deividi");

        hold_longpress_done = 1;
    }

    if(hold_state == GPIO_PIN_SET)
    {
        hold_pressed = 0;
    }

    /* ================= WAH (PB1) ================= */
    uint8_t wah_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    if((now - last_wah_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(wah_state != last_wah_state)
        {
            if(wah_state == GPIO_PIN_RESET) {
                SendNoteOn(70);
            } else {
                SendNoteOff(70);
            }
            last_wah_time = now;
            last_wah_state = wah_state;
        }
    }

    /* ================= ZIP (PB0) ================= */
    uint8_t zip_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    if((now - last_zip_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(zip_state != last_zip_state)
        {
            if(zip_state == GPIO_PIN_RESET) {
                SendNoteOn(71);
            } else {
                SendNoteOff(71);
            }
            last_zip_time = now;
            last_zip_state = zip_state;
        }
    }

    /* ================= JET (PA7) ================= */
    uint8_t jet_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
    if((now - last_jet_time) > BUTTON_DEBOUNCE_TIME)
    {
        if(jet_state != last_jet_state)
        {
            if(jet_state == GPIO_PIN_RESET) {
                SendNoteOn(72);
            } else {
                SendNoteOff(72);
            }
            last_jet_time = now;
            last_jet_state = jet_state;
        }
    }

    HAL_Delay(1);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USER CODE END */
