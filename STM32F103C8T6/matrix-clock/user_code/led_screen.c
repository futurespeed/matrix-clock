#include "led_screen.h"

uint16_t *gram_rgb_565;

// Ϊ�����Ч�ʣ�����G�ĵ���λ��ʵ�ʰ���RGB555�ĸ�ʽ���
void LED_screen_update_rgb_565(uint8_t brightness)
{
	uint16_t i, j, k;
	uint16_t offset = 0;
	uint16_t delay = 0;
	uint16_t mask_r = 0;
	uint16_t mask_g = 0;
	uint16_t mask_b = 0;
	uint16_t color = 0;

	for(k=0; k<5; k++)
	{
		offset = 0;
		// mask_r = 0x0001 << + k;
		// mask_g = 0x0040 << + k;
		// mask_b = 0x0800 << + k;
		mask_b = 0x0001 << k;
		mask_g = 0x0020 << k;
		mask_r = 0x0800 << k;
		for (j=0; j<MATRIX_LED_SCAN; j++)
		{
			// д���ַ
			// port_data = LL_GPIO_ReadOutputPort(LCD_SCREEN_PORT);
			// port_data &= 0x00000FFF;
			// port_data |= i << 12;
			// LL_GPIO_WriteOutputPort(LCD_SCREEN_PORT, port_data);

			(j & 0x01) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, ADDR_A_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, ADDR_A_PIN);
			(j & 0x02) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, ADDR_B_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, ADDR_B_PIN);
			(j & 0x04) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, ADDR_C_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, ADDR_C_PIN);
			(j & 0x08) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, ADDR_D_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, ADDR_D_PIN);
			(j & 0x10) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, ADDR_E_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, ADDR_E_PIN);
			
			for (i=0; i<MATRIX_LED_WIDTH; i++)
			{
				color = gram_rgb_565[offset];
				// color = (gram_rgb_565[pgram] << 8) | gram_rgb_565[pgram + 1];
				// color = (gram_rgb_565[pgram + 1] << 8) | gram_rgb_565[pgram];
				(color & mask_r) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, R1_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, R1_PIN);
				(color & mask_g) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, G1_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, G1_PIN);
				(color & mask_b) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, B1_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, B1_PIN);

				color = gram_rgb_565[offset + MATRIX_LED_WIDTH * MATRIX_LED_HEIGHT / 2];
				// color = (gram_rgb_565[pgram + 64 * 32] << 8) | gram_rgb_565[pgram + 1 + 64 * 32];
				// color = (gram_rgb_565[pgram + 1 + 64 * 32] << 8) | gram_rgb_565[pgram + 64 * 32];
				(color & mask_r) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, R2_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, R2_PIN);
				(color & mask_g) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, G2_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, G2_PIN);
				(color & mask_b) ? LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, B2_PIN) : LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, B2_PIN);

				offset += 1;

				LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, CLK_PIN);
				__NOP();
				__NOP();
				// __NOP();
				// __NOP();
				LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, CLK_PIN);
				__NOP();
				__NOP();
				// __NOP();
				// __NOP();
			}
			LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, CLK_PIN);

			//����
			LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, LAT_PIN);
			__NOP();
			__NOP();
			// __NOP();
			// __NOP();
			LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, LAT_PIN);
			__NOP();
			__NOP();
			// __NOP();
			// __NOP();
			LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, LAT_PIN);

			// �ر��жϣ�������˸
			__disable_irq();
			LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, OE_PIN);
			//������ʾ
			LL_GPIO_ResetOutputPin(LCD_SCREEN_PORT, OE_PIN);
			//����ʱ��������
			delay = brightness << k;
			while(delay--){__NOP();}
			LL_GPIO_SetOutputPin(LCD_SCREEN_PORT, OE_PIN);
			// __NOP();
			// __NOP();
			__enable_irq();
		}
	}
}

