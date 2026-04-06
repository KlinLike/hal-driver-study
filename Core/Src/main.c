/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "soft_timer.h"
#include "driver_oled.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    APP_MODE_CLOCK = 0,
    APP_MODE_KEY_COUNT
} app_mode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY_DEBOUNCE_MS  10u /* 消抖时间(ms)，可在 10~50 间微调 */
/* 运行模式：改下面宏为 APP_MODE_CLOCK 或 APP_MODE_KEY_COUNT，保存后重新编译烧录 */
#define APP_MODE_SELECT   APP_MODE_CLOCK
/* OLED：x 为列 0~15（每列 8px）；y 为页 0~7，每个字符占 y 与 y+1 两页（16px 高）。请用偶数 y 对齐行，减少跨黄/蓝区 */
#define OLED_X_TEXT       4u
#define OLED_Y_TITLE      0u
#define OLED_Y_BODY       2u
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile uint32_t clock_seconds = 0;
static volatile uint8_t clock_update_flag = 0;
static const app_mode_t s_app_mode = APP_MODE_SELECT;
static uint32_t s_key_press_count = 0;
static soft_timer_t clock_timer;
static soft_timer_t key_timer; /* 按键消抖：单次定时，EXTI 里反复 start 可重触发 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void LED_Control(int on);
void BUZZ_Control(int on);
static void App_PrintClockLine(void);
static void App_ApplyUiFullRedraw(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* SysTick 每 1s：仅时钟模式下秒计数、置 OLED 刷新标志、1Hz 翻转 LED */
static void clock_tick(void *arg)
{
    (void)arg;
    if (s_app_mode != APP_MODE_CLOCK)
        return;
    clock_seconds++;
    clock_update_flag = 1;
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_GPIO_Pin);
}

static void App_PrintClockLine(void)
{
    uint32_t s = clock_seconds;
    uint32_t hh = (s / 3600) % 24;
    uint32_t mm = (s / 60) % 60;
    uint32_t ss = s % 60;
    char buf[9];
    buf[0] = (char)('0' + hh / 10);
    buf[1] = (char)('0' + hh % 10);
    buf[2] = (ss & 1u) ? ':' : ' ';
    buf[3] = (char)('0' + mm / 10);
    buf[4] = (char)('0' + mm % 10);
    buf[5] = buf[2];
    buf[6] = (char)('0' + ss / 10);
    buf[7] = (char)('0' + ss % 10);
    buf[8] = '\0';
    OLED_PrintString(OLED_X_TEXT, OLED_Y_BODY, buf);
}

static void App_ApplyUiFullRedraw(void)
{
    OLED_Clear();
    switch (s_app_mode) {
    case APP_MODE_CLOCK:
        OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "LED Clock");
        App_PrintClockLine();
        break;
    case APP_MODE_KEY_COUNT:
        OLED_PrintString(OLED_X_TEXT, OLED_Y_TITLE, "Key Count");
        OLED_PrintSignedVal(OLED_X_TEXT, OLED_Y_BODY, (int32_t)s_key_press_count);
        break;
    default:
        break;
    }
}

/* 消抖完成：按当前编译模式处理引脚（模式由 APP_MODE_SELECT 决定，改宏后重编译） */
static void key_debounce_done(void *arg)
{
    (void)arg;

    switch (s_app_mode) {
    case APP_MODE_CLOCK:
        /* 纯时钟：消抖结束即可，不读引脚、不驱动 LED/蜂鸣器 */
        break;
    case APP_MODE_KEY_COUNT: {
        GPIO_PinState level = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_GPIO_Pin);
        if (level == GPIO_PIN_RESET) {
            s_key_press_count++;
            OLED_ClearLine(OLED_X_TEXT, OLED_Y_BODY);
            OLED_PrintSignedVal(OLED_X_TEXT, OLED_Y_BODY, (int32_t)s_key_press_count);
            LED_Control((int)(s_key_press_count & 1u));
            BUZZ_Control(0);
        }
        break;
    }
    default:
        break;
    }
}

/* 供 SysTick_Handler：每 1ms 轮询软定时器 */
void app_systick_handler(void)
{
    soft_timer_process(&clock_timer);
    soft_timer_process(&key_timer);
}

void LED_Control(int on)
{
	if(on){
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_GPIO_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_GPIO_Pin, GPIO_PIN_SET);
	}
}

void BUZZ_Control(int on)
{
	if(on){
		HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_GPIO_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_GPIO_Pin, GPIO_PIN_SET);
	}
}

int LDR_Status()
{
	return (HAL_GPIO_ReadPin(LDR_GPIO_Port, LDR_GPIO_Pin) == GPIO_PIN_RESET);
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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	LED_Control(0);
	BUZZ_Control(0);
	OLED_Init();

	soft_timer_init(&clock_timer, 1000, SOFT_TIMER_MODE_PERIODIC, clock_tick, NULL);
	soft_timer_start(&clock_timer);

	soft_timer_init(&key_timer, KEY_DEBOUNCE_MS, SOFT_TIMER_MODE_ONESHOT, key_debounce_done, NULL);
	/* 此处不 start key_timer，等有按键 EXTI 再启动，避免上电误触发 */

	App_ApplyUiFullRedraw();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (clock_update_flag) {
      clock_update_flag = 0;
      App_PrintClockLine();
    }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == KEY_GPIO_Pin) {
		/* 可重触发消抖：每次边沿重新计时，静止超过 KEY_DEBOUNCE_MS 后执行 key_debounce_done 
       soft_timer_start会刷新定时器时间 */
		soft_timer_start(&key_timer);
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
