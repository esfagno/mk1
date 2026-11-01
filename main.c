#include "main.h"
#include "tm1637.h"
#include "keyboard.h"

volatile uint32_t tickCount;

char lastKey;
uint32_t lastScanTime;

/* Переменные калькулятора */

static int8_t operand1 = -1;
static int8_t operand2 = -1;

static char operation = '\0';

static uint8_t operationSelected = 0;
static uint8_t resultShown = 0;

/*
    A -> +
    B -> -
    C -> *
    D -> /
*/

/* Преобразование клавиши в операцию */

char getOperation(char key)
{
    switch (key)
    {
        case 'A':
            return '+';

        case 'B':
            return '-';

        case 'C':
            return '*';

        case 'D':
            return '/';

        default:
            return '\0';
    }
}

/* Сброс калькулятора */

void calculatorReset(void)
{
    operand1 = -1;
    operand2 = -1;

    operation = '\0';

    operationSelected = 0;
    resultShown = 0;

    tm1637_clear();
}

/* Обработка нажатий клавиатуры */

void calculatorProcessKey(char key)
{
    /*
       Если результат уже показан
       и нажата цифра, начинаем новый расчёт.
    */

    if (resultShown &&
        key >= '0' &&
        key <= '9')
    {
        calculatorReset();
    }

    /* Ввод первого и второго операнда */

    if (key >= '0' && key <= '9')
    {
        if (operand1 == -1)
        {
            operand1 = key - '0';

            tm1637_display_number(operand1);

            printf("Operand 1: %d\r\n", operand1);
        }
        else if (operationSelected && operand2 == -1)
        {
            operand2 = key - '0';

            tm1637_display_number(operand2);

            printf("Operand 2: %d\r\n", operand2);
        }

        return;
    }

    /* Выбор операции A, B, C или D */

    char op = getOperation(key);

    if (op != '\0')
    {
        if (operand1 != -1 && operand2 == -1)
        {
            operation = op;
            operationSelected = 1;

            printf("Operation: %c\r\n", operation);
        }

        return;
    }

    /* Выполнение вычисления по клавише # */

    if (key == '#')
    {
        if (operand1 == -1 ||
            operand2 == -1 ||
            !operationSelected)
        {
            return;
        }

        int result = 0;

        switch (operation)
        {
            case '+':

                result = operand1 + operand2;

                break;

            case '-':

                result = operand1 - operand2;

                break;

            case '*':

                result = operand1 * operand2;

                break;

            case '/':

                /* Проверка деления на ноль */

                if (operand2 == 0)
                {
                    printf("Error: division by zero\r\n");

                    tm1637_display_error();

                    resultShown = 1;

                    return;
                }

                /* Целочисленное деление */

                result = operand1 / operand2;

                break;

            default:

                return;
        }

        printf(
            "%d %c %d = %d\r\n",
            operand1,
            operation,
            operand2,
            result
        );

        tm1637_display_number(result);

        resultShown = 1;
    }
}

/* SysTick */

void osSystickHandler(void)
{
    tickCount++;
}

/* Инициализация GPIO */

void initGPIO(void)
{
    /* Включение тактирования GPIOA и GPIOB */

    RCC->AHBENR |=
        RCC_AHBENR_GPIOAEN |
        RCC_AHBENR_GPIOBEN;

    /* PA5 — выход для светодиода */

    GPIOA->MODER =
        (GPIOA->MODER & ~(3U << 10)) |
        (1U << 10);

    GPIOA->OTYPER &= ~(1U << 5);

    GPIOA->OSPEEDR |= (1U << 10);
}

/* Инициализация USART2 */

void initUSART2(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 и PA3 — альтернативный режим */

    GPIOA->MODER =
        (GPIOA->MODER & ~(0xFU << 4)) |
        (0xAU << 4);

    GPIOA->AFR[0] =
        (GPIOA->AFR[0] & ~(0xFFU << 8)) |
        (1U << 8) |
        (1U << 12);

    /* 48 MHz / 115200 */

    USART2->BRR = 417;

    USART2->CR1 =
        USART_CR1_TE |
        USART_CR1_UE;
}

/* Инициализация SysTick */

void initSysTick(void)
{
    /* 1 мс при частоте 48 МГц */

    SysTick->LOAD = 47999;

    SysTick->VAL = 0;

    SysTick->CTRL =
        (1U << 2) |
        (1U << 1) |
        (1U << 0);
}

/* Перенаправление printf в USART */

int _write(int file, uint8_t *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        while (!(USART2->ISR & USART_ISR_TXE));

        USART2->TDR = ptr[i];
    }

    return len;
}

/* Главная функция */

int main(void)
{
    initGPIO();

    initUSART2();

    initSysTick();

    initKeyboard();

    tm1637_init();

    printf("Calculator started\r\n");

    printf("Digits: 0-9\r\n");

    printf("A: +, B: -, C: *, D: /\r\n");

    printf("#: calculate\r\n");

    /* Включение светодиода */

    GPIOA->ODR |= (1U << 5);

    while (1)
    {
        char key = scanKeyboard();

        if (key != '\0')
        {
            calculatorProcessKey(key);
        }
    }

    return 0;
}
