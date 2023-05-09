#include<libopencm3/stm32/rcc.h>
#include<libopencm3/stm32/flash.h>
#include<libopencm3/stm32/gpio.h>
#include<libopencm3/stm32/spi.h>
#include<libopencm3/cm3/nvic.h>

#define DO_INT 0

void f (uint8_t* d, int i) {
	// static int i = 0; // current byte
	static int j = 0; // iteration counter

	if (i <= 27) {
		int s = (i == (j%28)); // position of wandering single red dot
		d[0] = (s) ? 0 : (0xff - 9 * i); // green
		d[1] = (i == 0 || i == 27 || s) ? 0xff : 0; // red
		d[2] = (s) ? 0 : (0 + 4 * i); // blue
		i++;
	} else{
		d[0] = 0x00; d[1] = 0x00; d[2] = 0x00;
		i = 0;
		j++;
	}
}

void spi1_isr(void){
	static int h=0; // byte part counter
	static int i=0; // byte counter
	static int j=0; // led counter

	static uint8_t d[3];

	if (! (SPI_SR(SPI1) & SPI_SR_TXE))
		return; // wrong interrupt

	if (h == 0 && i == 0) // new byte
		f(d, j);

	switch(h){ // which part of byte to write
		case 0:
			SPI_DR(SPI1) = 0b10010010 |
				 ((d[i]>>7 & 1) << 6) |
				 ((d[i]>>6 & 1) << 3) |
				 ((d[i]>>5 & 1) << 0) ;
			break;
		case 1:
			SPI_DR(SPI1) = 0b01001001 |
				 ((d[i]>>4 & 1) << 5) |
				 ((d[i]>>3 & 1) << 2) ;
			break;
		case 2:
			SPI_DR(SPI1) = 0b00100100 |
				 ((d[i]>>2 & 1) << 7) |
				 ((d[i]>>1 & 1) << 4) |
				 ((d[i]>>0 & 1) << 1) ;
			break;
	}
	h++;
	if (h <= 2)
		return;

	// next byte
	h = 0;
	i++;
	if (i <= 2)
		return;

	// next led
	i = 0;
	j++;
	if (j <= 28)
		return;

	// start from new: stop interrupt and wait for it to be enabled again
	j = 0;
	spi_disable_tx_buffer_empty_interrupt(SPI1);
}

int main(void){
	//rcc_clock_setup_in_hse_8mhz_out_72mhz();
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_clear(GPIOA, GPIO5);
	gpio_clear(GPIOA, GPIO7);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);

	rcc_periph_reset_pulse(RCC_SPI1);
	rcc_periph_clock_enable(RCC_SPI1);

	spi_disable(SPI1);

	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32,
		SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_1,
		SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

	spi_enable(SPI1);

#if ! DO_INT
	static uint8_t d[3];
#else
	nvic_enable_irq(NVIC_SPI1_IRQ);
#endif

	while(1){
#if ! DO_INT
		for(int j = 0 ; j < 29 ; j ++) {
		f(d, j);
		for(int i = 0 ; i <= 2; i++) {
			spi_send(SPI1, 0b10010010 |
				 ((d[i]>>7 & 1) << 6) |
				 ((d[i]>>6 & 1) << 3) |
				 ((d[i]>>5 & 1) << 0) );
			spi_send(SPI1, 0b01001001 |
				 ((d[i]>>4 & 1) << 5) |
				 ((d[i]>>3 & 1) << 2) );
			spi_send(SPI1, 0b00100100 |
				 ((d[i]>>2 & 1) << 7) |
				 ((d[i]>>1 & 1) << 4) |
				 ((d[i]>>0 & 1) << 1) );
			}
		}
#else
		spi_enable_tx_buffer_empty_interrupt(SPI1);
#endif
		for(int i = 0 ; i < 0x00fffff; i ++) __asm__("nop");
	}
	return 0;
}
