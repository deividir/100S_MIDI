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

/* ================= DECK CONTROL & LED STATES ================= */
uint8_t current_deck = 0;
static uint8_t hold_pressed = 0;
static uint32_t hold_press_start = 0;
static uint8_t hold_longpress_done = 0;

// Armazena o estado de todas as 128 notas para os 2 Decks (0 = Deck A, 1 = Deck B)
uint8_t led_states[2][128] = {0};

// Wah Button (PB1)
static uint8_t last_wah_state = GPIO_PIN_SET;
static uint32_t last_wah_time = 0;

// Zip Button (PB0)
static uint8_t last_zip_state = GPIO_PIN_SET;
static uint32_t last_zip_time = 0;

// Jet Button (PA7)
static uint8_t last_jet_state = GPIO_PIN_SET;
static uint32_t last_jet_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SendCC(uint8_t cc, uint8_t value);
void SendNoteOn(uint8_t note);
void SendNoteOff(uint8_t note);
void My_LCD_Create_Char(uint8_t location, uint8_t *charmap);
void Refresh_LEDs(void);
void Update_LCD_Status(void);
void ProcessMidiRx(uint8_t *buf, uint32_t length);
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

  // Escreve o nome na segunda linha permanentemente
  lcd_put_cur(1, 0);
  lcd_send_string("   DJ Deividi   ");

  // Mostra o status inicial na primeira linha
  Update_LCD_Status();
  // Atualiza o estado dos LEDs na inicialização
  Refresh_LEDs();

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

    /* ================= HOLD (PB7) - ALTERNADOR DE DECK ================= */
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

        // Atualiza na hora o estado dos LEDs físicos para bater com o novo Deck ativo
        Refresh_LEDs();

        // Atualiza dinamicamente a linha superior do LCD com o novo Deck e o status atual dele
        Update_LCD_Status();

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
/**
  * @brief  Atualiza os pinos físicos dos LEDs baseando-se no Deck atual e na tabela armazenada.
  * @retval None
  */
void Refresh_LEDs(void)
{
    /* Abaixo estão os mapeamentos via MIDI OUT pelo Serato.
       Descomente as linhas e altere os nomes "PORT" e "Pin" para as saídas
       reais configuradas no seu CubeMX para o painel do seu CDJ-100S.
    */

    // Nota 60 = LED do PLAY
    // HAL_GPIO_WritePin(LED_PLAY_GPIO_Port, LED_PLAY_Pin, led_states[current_deck][60] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Nota 61 = LED do CUE
    // HAL_GPIO_WritePin(LED_CUE_GPIO_Port, LED_CUE_Pin, led_states[current_deck][61] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Nota 64 = LED do Search +
    // HAL_GPIO_WritePin(LED_SEARCH_PLUS_GPIO_Port, LED_SEARCH_PLUS_Pin, led_states[current_deck][64] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Nota 65 = LED do Search -
    // HAL_GPIO_WritePin(LED_SEARCH_MINUS_GPIO_Port, LED_SEARCH_MINUS_Pin, led_states[current_deck][65] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Nota 71 = LED do Zip (Mapeado como Sync)
    // HAL_GPIO_WritePin(LED_ZIP_GPIO_Port, LED_ZIP_Pin, led_states[current_deck][71] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Nota 72 = LED do Jet (Mapeado como Sync Off)
    // HAL_GPIO_WritePin(LED_JET_GPIO_Port, LED_JET_Pin, led_states[current_deck][72] ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Controle dos pinos PB12 e PB13 com base na Nota 60 (Play/Pause)
    if (led_states[current_deck][60] == 1) // Se a Nota 60 (Play) estiver ligada
    {
        HAL_GPIO_WritePin(LED_PLAY_GPIO_Port, LED_PLAY_Pin, GPIO_PIN_SET);   // Liga LED_PLAY
        HAL_GPIO_WritePin(LED_CUE_GPIO_Port, LED_CUE_Pin, GPIO_PIN_RESET); // Desliga LED_CUE
    }
    else // Se a Nota 60 (Play) estiver desligada (Pause)
    {
        HAL_GPIO_WritePin(LED_PLAY_GPIO_Port, LED_PLAY_Pin, GPIO_PIN_RESET); // Desliga LED_PLAY
        HAL_GPIO_WritePin(LED_CUE_GPIO_Port, LED_CUE_Pin, GPIO_PIN_SET);   // Liga LED_CUE
    }
}

/**
  * @brief  Atualiza a linha superior do LCD com o status dinâmico do Deck atual.
  * @retval None
  */
void Update_LCD_Status(void)
{
    lcd_put_cur(0, 0); // Move o cursor para o começo da primeira linha

    // 1. Verifica se o Serato diz que o PLAY está ativo (Nota 60 = LIGADO)
    if (led_states[current_deck][60] == 1)
    {
        if (current_deck == 0)
            lcd_send_string("DECK A - PLAY   ");
        else
            lcd_send_string("DECK B - PLAY   ");
    }
    // 2. Se o Play está desligado, verifica se o CUE está ativo (Nota 61 = LIGADO)
    else if (led_states[current_deck][61] == 1)
    {
        if (current_deck == 0)
            lcd_send_string("DECK A - CUE    ");
        else
            lcd_send_string("DECK B - CUE    ");
    }
    // 3. Se nenhum dos dois está ativo, o deck está em PAUSE
    else
    {
        if (current_deck == 0)
            lcd_send_string("DECK A - PAUSE  ");
        else
            lcd_send_string("DECK B - PAUSE  ");
    }
    // Garante que os LEDs sejam atualizados sempre que o status do LCD mudar
    Refresh_LEDs();
}

/**
  * @brief  Processa os pacotes USB MIDI recebidos (Midi Out do Host / Serato).
  * @param  buf: ponteiro para o buffer de recepção USB
  * @param  length: tamanho dos dados recebidos
  * @retval None
  */
void ProcessMidiRx(uint8_t *buf, uint32_t length)
{
    // Varre o buffer USB de 4 em 4 bytes (formato padrão USB-MIDI de mensagens)
    for (uint32_t i = 0; i < length; i += 4)
    {
        uint8_t status_byte = buf[i + 1]; // Ex: 0x90 (Note On Canal 1), 0x91 (Note On Canal 2)
        uint8_t note_number = buf[i + 2]; // O número da nota gerada (60, 61, etc.)
        uint8_t velocity    = buf[i + 3]; // Velocidade (127 = On / Liga, 0 = Off / Desliga)

        uint8_t msg_channel = status_byte & 0x0F; // Isola o canal: 0 = Deck A (Ch 1), 1 = Deck B (Ch 2)
        uint8_t msg_type    = status_byte & 0xF0; // Isola o comando: 0x90 = Note On, 0x80 = Note Off

        // Filtra se a mensagem recebida é do tipo Note On ou Note Off
        if (msg_type == 0x90 || msg_type == 0x80)
        {
            // Se o comando for Note On com velocidade maior que zero, liga (1). Caso contrário, desliga (0).
            uint8_t state = (msg_type == 0x90 && velocity > 0) ? 1 : 0;

            // Evita estouro de array garantindo os limites do barramento MIDI
            if (msg_channel < 2 && note_number < 128)
            {
                // Registra o estado recebido na memória do respectivo Deck mapeado
                led_states[msg_channel][note_number] = state;

                // Se o comando MIDI enviado pelo Serato coincidir com o Deck atual na tela, atualiza o hardware e o LCD
                if (msg_channel == current_deck)
                {
                    Refresh_LEDs();
                    Update_LCD_Status();
                }
            }
        }
    }
}
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
