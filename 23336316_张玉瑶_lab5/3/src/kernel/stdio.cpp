#include "stdio.h"
#include "os_type.h"
#include "asm_utils.h"
#include "os_modules.h"
#include "stdarg.h"
#include "stdlib.h"
#include <cctype>
#include <cstdlib>
STDIO::STDIO()
{
    initialize();
}

void STDIO::initialize()
{
    screen = (uint8 *)0xb8000;
}

void STDIO::print(uint x, uint y, uint8 c, uint8 color)
{

    if (x >= 25 || y >= 80)
    {
        return;
    }

    uint pos = x * 80 + y;
    screen[2 * pos] = c;
    screen[2 * pos + 1] = color;
}

void STDIO::print(uint8 c, uint8 color)
{
    uint cursor = getCursor();
    screen[2 * cursor] = c;
    screen[2 * cursor + 1] = color;
    cursor++;
    if (cursor == 25 * 80)
    {
        rollUp();
        cursor = 24 * 80;
    }
    moveCursor(cursor);
}

void STDIO::print(uint8 c)
{
    print(c, 0x07);
}

void STDIO::moveCursor(uint position)
{
    if (position >= 80 * 25)
    {
        return;
    }

    uint8 temp;

    // 处理高8位
    temp = (position >> 8) & 0xff;
    asm_out_port(0x3d4, 0x0e);
    asm_out_port(0x3d5, temp);

    // 处理低8位
    temp = position & 0xff;
    asm_out_port(0x3d4, 0x0f);
    asm_out_port(0x3d5, temp);
}

uint STDIO::getCursor()
{
    uint pos;
    uint8 temp;

    pos = 0;
    temp = 0;
    // 处理高8位
    asm_out_port(0x3d4, 0x0e);
    asm_in_port(0x3d5, &temp);
    pos = ((uint)temp) << 8;

    // 处理低8位
    asm_out_port(0x3d4, 0x0f);
    asm_in_port(0x3d5, &temp);
    pos = pos | ((uint)temp);

    return pos;
}

void STDIO::moveCursor(uint x, uint y)
{
    if (x >= 25 || y >= 80)
    {
        return;
    }

    moveCursor(x * 80 + y);
}

void STDIO::rollUp()
{
    uint length;
    length = 25 * 80;
    for (uint i = 80; i < length; ++i)
    {
        screen[2 * (i - 80)] = screen[2 * i];
        screen[2 * (i - 80) + 1] = screen[2 * i + 1];
    }

    for (uint i = 24 * 80; i < length; ++i)
    {
        screen[2 * i] = ' ';
        screen[2 * i + 1] = 0x07;
    }
}

int STDIO::print(const char *const str)
{
    int i = 0;

    for (i = 0; str[i]; ++i)
    {
        switch (str[i])
        {
        case '\n':
            uint row;
            row = getCursor() / 80;
            if (row == 24)
            {
                rollUp();
            }
            else
            {
                va_before_add(row);
            }
            moveCursor(row * 80);
            break;

        default:
            print(str[i]);
            break;
        }
    }

    return i;
}

int printf_add_to_buffer(char *buffer, char c, int &idx, const int BUF_LEN)
{
    int counter = 0;

    buffer[idx] = c;
    va_before_add(idx);

    if (idx == BUF_LEN)
    {
        buffer[idx] = '\0';
        counter = stdio.print(buffer);
        idx = 0;
    }

    return counter;
}

int printf(const char *const fmt, ...)
{
    const int BUF_LEN = 32;
    int temp;
    char buffer[BUF_LEN + 1];
    char number[33];
    int precision;
     int digit;
     int int_part;
    int idx, counter;
    double fractional_part;
    double tempp;
    va_list ap;

    va_start(ap, fmt);
    idx = 0;
    counter = 0;

    for (int i = 0; fmt[i]; va_before_add(i))
    {
        if (fmt[i] != '%')
        {
            counter += printf_add_to_buffer(buffer, fmt[i], idx, BUF_LEN);
        }
        else
        {
            va_after_add(i);
            if (fmt[i] == '\0')
            {
                break;
            }

            switch (fmt[i])
            {
            case '%':
                counter += printf_add_to_buffer(buffer, fmt[i], idx, BUF_LEN);
                break;

            case 'c':
                counter += printf_add_to_buffer(buffer, va_arg(ap, char), idx, BUF_LEN);
                break;

            case 's':
                buffer[idx] = '\0';
                idx = 0;
                counter += stdio.print(buffer);
                counter += stdio.print(va_arg(ap, const char *));
                break;

            case 'd':
            case 'x':
                temp = va_arg(ap, int);

                if (temp < 0 && fmt[i] == 'd')
                {
                    counter += printf_add_to_buffer(buffer, '-', idx, BUF_LEN);
                    temp = -temp;
                }

                itos(number, temp, (fmt[i] == 'd' ? 10 : 16));

                for (int j = 0; number[j]; va_before_add(j))
                {
                    counter += printf_add_to_buffer(buffer, number[j], idx, BUF_LEN);
                }
                break;
                
            case 'f':                  
                tempp = va_arg(ap, double);  
                precision = 6;  
    

                 if (tempp < 0)
                 {
                       counter += printf_add_to_buffer(buffer, '-', idx, BUF_LEN);
                       tempp = -tempp;
                   }


                  int_part = (int)tempp;
                 itos(number, int_part, 10); 


                  for (int j = 0; number[j]; va_before_add(j))
                  {
                      counter += printf_add_to_buffer(buffer, number[j], idx, BUF_LEN);
                  }


                  counter += printf_add_to_buffer(buffer, '.', idx, BUF_LEN);


                  fractional_part = tempp - int_part;
                  for (int j = 0; j < precision; ++j)
                  {
                      fractional_part *= 10; 
                      digit = (int)fractional_part;  
                      counter += printf_add_to_buffer(buffer, '0' + digit, idx, BUF_LEN);
                      fractional_part -= digit;  
                  }
                  break;
              
             
            case '.'://处理点nf     
                int np = fmt[va_before_add(i)]-'0';
                
                if(fmt[va_before_add(i)]!='f') break;
                             
                tempp = va_arg(ap, double);  
                precision = np;  
    

                 if (tempp < 0)
                 {
                       counter += printf_add_to_buffer(buffer, '-', idx, BUF_LEN);
                       tempp = -tempp;
                   }


                 int_part = (int)tempp;
                 itos(number, int_part, 10); 


                  for (int j = 0; number[j]; va_before_add(j))
                  {
                      counter += printf_add_to_buffer(buffer, number[j], idx, BUF_LEN);
                  }


                  counter += printf_add_to_buffer(buffer, '.', idx, BUF_LEN);


                 fractional_part = tempp - int_part;
                  for (int j = 0; j < precision; ++j)
                  {
                      fractional_part *= 10; 
                      digit = (int)fractional_part;  
                      counter += printf_add_to_buffer(buffer, '0' + digit, idx, BUF_LEN);
                      fractional_part -= digit;  
                  }
                  break;
              
              
             
              
              
            }
        }
    }

    buffer[idx] = '\0';
    counter += stdio.print(buffer);

    return counter;
}
