#include "tm1637.h"

/* Задержка */

void delay_us(uint32_t us)
{
    volatile uint32_t cycles = us * 24;

    while (cycles-- > 0)
    {
        __asm__("nop");
    }
}

/* Инициализация */

void tm1637_init(void)
{
    /*
       PA6 = CLK
       PA7 = DIO
    */

    GPIOA->MODER =
        (GPIOA->MODER & ~(0xFU << (TM1637_CLK_PIN * 2))) |
        (0x5U << (TM1637_CLK_PIN * 2));

    GPIOA->OTYPER |=
        (1U << TM1637_CLK_PIN) |
        (1U << TM1637_DIO_PIN);

    GPIOA->OSPEEDR |=
        (0x3U << (TM1637_CLK_PIN * 2)) |
        (0x3U << (TM1637_DIO_PIN * 2));

    /* Высокий уровень */

    GPIOA->BSRR =
        (1U << TM1637_CLK_PIN) |
        (1U << TM1637_DIO_PIN);

    /* Инициализация */

    tm1637_start();

    tm1637_write_byte(TM1637_CMD1);

    tm1637_stop();

    tm1637_start();

    tm1637_write_byte(TM1637_CMD3);

    tm1637_stop();

    tm1637_clear();

    tm1637_display_number(0);
}

/* START */

void tm1637_start(void)
{
    GPIOA->BSRR =
        (1U << TM1637_DIO_PIN) |
        (1U << TM1637_CLK_PIN);

    delay_us(2);

    GPIOA->BRR = (1U << TM1637_DIO_PIN);

    delay_us(2);

    GPIOA->BRR = (1U << TM1637_CLK_PIN);

    delay_us(2);
}

/* STOP */

void tm1637_stop(void)
{
    GPIOA->BRR = (1U << TM1637_CLK_PIN);

    delay_us(2);

    GPIOA->BRR = (1U << TM1637_DIO_PIN);

    delay_us(2);

    GPIOA->BSRR = (1U << TM1637_CLK_PIN);

    delay_us(2);

    GPIOA->BSRR = (1U << TM1637_DIO_PIN);

    delay_us(2);
}

/* Запись байта */

void tm1637_write_byte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        GPIOA->BRR = (1U << TM1637_CLK_PIN);

        delay_us(2);

        if (byte & 0x01)
        {
            GPIOA->BSRR = (1U << TM1637_DIO_PIN);
        }
        else
        {
            GPIOA->BRR = (1U << TM1637_DIO_PIN);
        }

        delay_us(2);

        GPIOA->BSRR = (1U << TM1637_CLK_PIN);

        delay_us(2);

        byte >>= 1;
    }

    /* ACK */

    GPIOA->BRR = (1U << TM1637_CLK_PIN);

    GPIOA->BSRR = (1U << TM1637_DIO_PIN);

    delay_us(2);

    GPIOA->BSRR = (1U << TM1637_CLK_PIN);

    delay_us(2);

    GPIOA->BRR = (1U << TM1637_CLK_PIN);

    delay_us(2);
}

/* Коды сегментов */

static const uint8_t digit_codes[] = {
    0x3F,   /* 0 */
    0x06,   /* 1 */
    0x5B,   /* 2 */
    0x4F,   /* 3 */
    0x66,   /* 4 */
    0x6D,   /* 5 */
    0x7D,   /* 6 */
    0x07,   /* 7 */
    0x7F,   /* 8 */
    0x6F,   /* 9 */
    0x77,   /* A */
    0x7C,   /* b */
    0x39,   /* C */
    0x5E,   /* d */
    0x79,   /* E */
    0x71    /* F */
};

/* Минус */

#define SEG_MINUS 0x40

/* Передача четырёх цифр */

static void tm1637_display_raw(const uint8_t data[4])
{
    /* Автоинкремент */

    tm1637_start();

    tm1637_write_byte(TM1637_CMD1);

    tm1637_stop();

    /* Адрес 0 */

    tm1637_start();

    tm1637_write_byte(TM1637_CMD2);

    for (uint8_t i = 0; i < 4; i++)
    {
        tm1637_write_byte(data[i]);
    }

    tm1637_stop();

    /* Яркость */

    tm1637_start();

    tm1637_write_byte(TM1637_CMD3);

    tm1637_stop();
}

/* Вывод одной цифры */

void tm1637_display_digit(uint8_t digit, uint8_t data)
{
    if (digit > 3)
    {
        return;
    }

    uint8_t digits[4] = {
        0x00,
        0x00,
        0x00,
        0x00
    };

    digits[digit] = data;

    tm1637_display_raw(digits);
}

/* Вывод числа */

void tm1637_display_number(int number)
{
    uint8_t digits[4] = {
        0x00,
        0x00,
        0x00,
        0x00
    };

    /* Положительное число */

    if (number >= 0)
    {
        if (number > 9999)
        {
            number = 9999;
        }

        int position = 3;

        do
        {
            digits[position] = digit_codes[number % 10];

            number /= 10;

            position--;

        } while (number > 0 && position >= 0);
    }

    /* Отрицательное число */

    else
    {
        number = -number;

        if (number > 999)
        {
            number = 999;
        }

        int position = 3;

        do
        {
            digits[position] = digit_codes[number % 10];

            number /= 10;

            position--;

        } while (number > 0 && position > 0);

        digits[0] = SEG_MINUS;
    }

    tm1637_display_raw(digits);
}

/* Отображение ошибки */

void tm1637_display_error(void)
{
    uint8_t digits[4] = {
        0x79,   /* E */
        0x50,   /* r */
        0x50,   /* r */
        0x00
    };

    tm1637_display_raw(digits);
}

/* Очистка дисплея */

void tm1637_clear(void)
{
    uint8_t digits[4] = {
        0x00,
        0x00,
        0x00,
        0x00
    };

    tm1637_display_raw(digits);
}
