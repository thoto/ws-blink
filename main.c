#include<libopencm3/stm32/rcc.h>
#include<libopencm3/stm32/flash.h>
#include<libopencm3/stm32/gpio.h>
#include<libopencm3/stm32/spi.h>
#include<libopencm3/cm3/nvic.h>
#include<libopencm3/cm3/assert.h>

#define DO_INT 0

void f (uint8_t* d, int i) {
	static int j = 0; // iteration counter

	if (i <= 27) {
		int s = (i == (j%28)); // position of wandering single red dot
		// d[0] = (s) ? 0 : (0xff - 9 * i); // green
		d[0] = (s) ? 0 : 0xff- (0xff >> (i>>2)); // green
		d[1] = (i == 0 || i == 27 || s) ? 0xff : 0; // red
		//d[2] = (s) ? 0 : (0 + 4 * i); // blue
		d[2] = (s) ? 0 : (0xff - 9 * i); // blue
	} else{
		d[0] = 0x00; d[1] = 0x00; d[2] = 0x00;
		j++;
	}
}

void prepare_next_byte(uint8_t* write, uint8_t* d, signed int i) {
	write[0] = 0b10010010;
	write[1] = 0b01001001;
	write[2] = 0b00100100;
	write[0] |= (((d[i] & (1<<7)) >> 1) | // ((d[i]>>7 & 1) << 6) |
		 ((d[i] & (1<<6)) >> 3) | // ((d[i]>>6 & 1) << 3) |
		 ((d[i] & (1<<5)) >> 5)); // ((d[i]>>5 & 1) << 0));
	write[1] |= (((d[i] & (1<<4)) << 1) | // ((d[i]>>4 & 1) << 5) |
		 ((d[i] & (1<<3)) >> 1)); // ((d[i]>>3 & 1) << 2));
	write[2] |= (((d[i] & 1<<2) << 5) | // ((d[i]>>2 & 1) << 7) |
		 ((d[i] & (1<<1)) << 3) | // ((d[i]>>1 & 1) << 4) |
		 ((d[i] & (1<<0)) << 1)); // ((d[i]>>0 & 1) << 1));

	// assert that IR did not take too long
	/*
	cm3_assert((SPI_SR(SPI1) & SPI_SR_BSY) || // still busy sending
		(h == 0 && i == 0 && j == 0));
	cm3_assert((!(SPI_SR(SPI1) & SPI_SR_TXE)) || // SR not empty (optional)
		(h == 0 && i == 0 && j == 0)); 
	*/
}


void spi1_isr(void){
	static signed int h = -1; // byte part counter
	static signed int i = -1; // byte counter
	static int j=0; // led counter

	static uint8_t d[3];

	static uint8_t write[3];

//	cm3_assert(SPI_SR(SPI1) & SPI_SR_TXE);

repeat:

	if (h >= 0) { // not at very first call
		/*
		cm3_assert((SPI_SR(SPI1) & SPI_SR_BSY) // assert still ongoing transfer
			|| (h == 0 && i == 0 && j == 0)); // .. but not when starting :-)
		*/
		*((volatile uint8_t*) &(SPI_DR(SPI1))) = write[h];

		h++;
		if (h <= 2)
			return; // i.e. send next byte part
	}

	// next byte
	h = 0; // start from first byte part

	if (i < 2) { // no new led -> just prepare next byte
		i++; // increase led counter
		prepare_next_byte(write, d, i);
		if(SPI_SR(SPI1) & SPI_SR_TXE)
			goto repeat;
		return;
	}
	// next led
	i = 0; // reset byte counter, byte part already reset

	if (j <= 28){ // not @ end of LED -> prepare byte parts
		j++; // increase led counter
		f(d, j); // fetch new LED RGB data
		prepare_next_byte(write, d, i);
		if(SPI_SR(SPI1) & SPI_SR_TXE)
			goto repeat;
		return;
	}

	// start from new: stop interrupt and wait for it to be enabled again
	j = 0;
	h = -1;
	i = -1;
	spi_disable_tx_buffer_empty_interrupt(SPI1);

} 

/*
void get_data(uint8_t* spibuf, uint8_t* d){
	for (int color = 0; color < 3; color++) {
		spibuf[color*3+2] = (0b10010010 |
			((d[color]>>7 & 1) << 6) |
			((d[color]>>6 & 1) << 3) |
			((d[color]>>5 & 1) << 0) );
		spibuf[color*3+1] = (0b01001001 |
			((d[color]>>4 & 1) << 5) |
			((d[color]>>3 & 1) << 2) );
		spibuf[color*3] = (0b00100100 |
			((d[color]>>2 & 1) << 7) |
			((d[color]>>1 & 1) << 4) |
			((d[color]>>0 & 1) << 1) );
	}
}

void spi1_isr(void){
	static int i=0;
	static int j=0; // led counter
	static uint8_t spibuf[3*3];

	uint8_t d[3];

	if (j == 0)	{
		f(d, j);
		get_data(spibuf, d);
	}

	*((volatile uint8_t*) &(SPI_DR(SPI1))) = spibuf[i++];
	if (i >= 3*3){
		i = 0;
		j++;
		f(d, j);
		get_data(spibuf, d);
		if (j >= 27){
			j = 0;
			spi_disable_tx_buffer_empty_interrupt(SPI1);
			spi_disable(SPI1);
		}
	}
}*/

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

#if ! DO_INT
	spi_enable(SPI1);

	static uint8_t d[3];
	static uint8_t write[3];
#else
	nvic_enable_irq(NVIC_SPI1_IRQ);
#endif

	while(1){
#if ! DO_INT
		for(int j = 0 ; j < 29 ; j ++) {
			f(d, j);
			for(int i = 0 ; i <= 2; i++) {
				prepare_next_byte(write, d, i);
				spi_send(SPI1, write[0]);
				spi_send(SPI1, write[1]);
				spi_send(SPI1, write[2]);
			}
		}
#else
		spi_enable(SPI1);
		spi_enable_tx_buffer_empty_interrupt(SPI1);
#endif
		for(int i = 0 ; i < 0x00fffff; i ++) __asm__("nop");
	}
	return 0;
}
