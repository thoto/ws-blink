#include<libopencm3/stm32/rcc.h>
#include<libopencm3/stm32/flash.h>
#include<libopencm3/stm32/gpio.h>
#include<libopencm3/stm32/spi.h>
#include<libopencm3/cm3/nvic.h>

void f (uint8_t* d) {
	static int i = 0;
	
	d[0] = 0xff - 8 * i;
	d[1] = (i == 0 || i == 28) ? 0xff : 0;
	d[2] = 0 + 8 * i;

	i++;
	if(i >= 29)
		i = 0;
}


int main(void){
	//rcc_clock_setup_in_hse_8mhz_out_72mhz();
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_AFIO); // FIXME: remove

	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_FULL_SWJ,
                AFIO_MAPR_USART1_REMAP);

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_clear(GPIOA, GPIO0);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO0);

	gpio_clear(GPIOA, GPIO5);
	gpio_clear(GPIOA, GPIO7);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		GPIO5);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		GPIO7);

	rcc_periph_reset_pulse(RCC_SPI1);
	rcc_periph_clock_enable(RCC_SPI1);
	
	spi_disable(SPI1);

	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32,
		SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_1,
		SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

	spi_enable(SPI1);

//	nvic_enable_irq(NVIC_TIM1_UP_IRQ);
	
	uint8_t d[3];
	while(1){
		for(int j = 0 ; j < 29 ; j ++) {
		f(d);
		for(int i = 0 ; i <= 2; i++) {
			gpio_set(GPIOA, GPIO0);
			spi_send(SPI1, 0b10010010 |
				 ((d[i]>>7 & 1) << 6) |
				 ((d[i]>>6 & 1) << 3) |
				 ((d[i]>>6 & 1) << 0) ); 
			gpio_clear(GPIOA, GPIO0);
			spi_send(SPI1, 0b01001001 |
				 ((d[i]>>4 & 1) << 5) |
				 ((d[i]>>3 & 1) << 2) );
			spi_send(SPI1, 0b00100100 |
				 ((d[i]>>2 & 1) << 7) |
				 ((d[i]>>1 & 1) << 4) |
				 ((d[i]>>0 & 1) << 1) ); 
			}
		}
		for(int i = 0 ; i < 0x0ffffff; i ++) __asm__("nop");
	}
	return 0;
}
