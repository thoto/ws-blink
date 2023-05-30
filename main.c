#include<libopencm3/stm32/rcc.h>
#include<libopencm3/stm32/flash.h>
#include<libopencm3/stm32/gpio.h>
#include<libopencm3/stm32/spi.h>
#include<libopencm3/stm32/dma.h>
#include<libopencm3/cm3/nvic.h>
#include<libopencm3/cm3/assert.h>

#define OP_MODE_DMA 2
#define OP_MODE_INT 1
#define OP_MODE_POL 0

#define OP_MODE 2

#include "neopixel.h"

#if OP_MODE == OP_MODE_INT
	#include"interrupt.h"
#endif

#define DMA_PORT DMA1
#define DMA_CHAN DMA_CHANNEL3


void dma1_channel3_isr(void) {
	if ((DMA1_ISR & DMA_ISR_TCIF3) != 0) { // transfer complete
		DMA1_IFCR |= DMA_IFCR_CTCIF3; // clear flag

		dma_disable_channel(DMA_PORT, DMA_CHAN);
		gpio_set(GPIOC, GPIO13);
	}
	if ((DMA1_ISR & DMA_ISR_TEIF3) != 0) // transfer error
		DMA1_IFCR |= DMA_IFCR_CTEIF3;
	if ((DMA1_ISR & DMA_ISR_GIF3) != 0)
		DMA1_IFCR |= DMA_IFCR_CGIF3;
}

void clear_dma_ir(){
	if ((DMA1_ISR & DMA_ISR_TCIF3) != 0)
		DMA1_IFCR |= DMA_IFCR_CTCIF3;
	if ((DMA1_ISR & DMA_ISR_TEIF3) != 0)
		DMA1_IFCR |= DMA_IFCR_CTEIF3;
	if ((DMA1_ISR & DMA_ISR_GIF3) != 0)
		DMA1_IFCR |= DMA_IFCR_CGIF3;
}


void init_dma(void* write, int write_sz) {
	dma_channel_reset(DMA_PORT, DMA_CHAN);
	dma_disable_channel(DMA_PORT, DMA_CHAN);

	// clear interrupt flags
	clear_dma_ir();

	dma_set_peripheral_address(DMA_PORT, DMA_CHAN, (uint32_t) (&SPI_DR(SPI1)));
	dma_set_memory_address(DMA_PORT, DMA_CHAN, (uint32_t) write);
	dma_set_number_of_data(DMA_PORT, DMA_CHAN, write_sz);

	// DMA1_CCR(DMA_CHANNEL3) = DMA_CCR_MSIZE_8BIT | DMA_CCR_PSIZE_8BIT |
	//	DMA_CCR_PL_LOW | DMA_CCR_MINC | DMA_CCR_DIR;
	dma_set_peripheral_size(DMA_PORT, DMA_CHAN, DMA_CCR_PSIZE_8BIT);
	dma_set_memory_size(DMA_PORT, DMA_CHAN, DMA_CCR_MSIZE_8BIT);
	dma_set_priority(DMA_PORT, DMA_CHAN, DMA_CCR_PL_LOW);
	dma_enable_memory_increment_mode(DMA_PORT, DMA_CHAN);
	dma_set_read_from_memory(DMA_PORT, DMA_CHAN);

	nvic_enable_irq(NVIC_DMA1_CHANNEL3_IRQ);
	dma_enable_transfer_complete_interrupt(DMA_PORT, DMA_CHAN);

	spi_enable_tx_dma(SPI1);
}

/**
 * initialize everything related to I/O pins, also enables GPIO clocks
 **/
void setup_gpio (void) {
	rcc_periph_reset_pulse(RCC_AFIO);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_reset_pulse(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_reset_pulse(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOC);

	// GPIO A provides SPI pins MOSI and SCLK
	gpio_clear(GPIOA, GPIO5);
	gpio_clear(GPIOA, GPIO7);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);

	// bluepill green onboard LED
	gpio_clear(GPIOC, GPIO13);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
}

/**
 * initialize SPI peripheral
 * @note spi_enable() would start transfers, so we won't do that here
 **/
void setup_spi (void) {
	rcc_periph_reset_pulse(RCC_SPI1);
	rcc_periph_clock_enable(RCC_SPI1);

	spi_disable(SPI1);

	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32,
		SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_1,
		SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
}

#define write_sz 3*3*29

int main(void){
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_reset_pulse(RCC_DMA1);
	rcc_periph_clock_enable(RCC_DMA1);

	setup_gpio();
	setup_spi();


#if OP_MODE == OP_MODE_POL
	spi_enable(SPI1);

	static uint8_t d[3];
	static uint8_t write[3];
#elif OP_MODE == OP_MODE_INT
	nvic_enable_irq(NVIC_SPI1_IRQ);
#elif OP_MODE == OP_MODE_DMA
	static uint8_t d[3]; // FIXME
	// const int write_sz = 3*3*29;
	static uint8_t write[write_sz];

	spi_enable(SPI1);

	init_dma(write, write_sz);
	spi_enable(SPI1);
#endif

	while(1){
#if OP_MODE == OP_MODE_POL
		for(int j = 0 ; j < 29 ; j ++) {
			f(d, j);
			for(int i = 0 ; i <= 2; i++) {
				prepare_next_byte(write, d, i);
				spi_send(SPI1, write[0]);
				spi_send(SPI1, write[1]);
				spi_send(SPI1, write[2]);
			}
		}
#elif OP_MODE == OP_MODE_INT
		spi_enable(SPI1);
		spi_enable_tx_buffer_empty_interrupt(SPI1);
#elif OP_MODE == OP_MODE_DMA
		// update data
		for(int j = 0 ; j < 29 ; j ++) {
			f(d, j);
			for(int i = 0 ; i <= 2; i++)
				prepare_next_byte(&write[(3*3*j)+3*i], d, i);
		}
		dma_set_number_of_data(DMA_PORT, DMA_CHAN, write_sz);

		clear_dma_ir();
		dma_enable_channel(DMA_PORT, DMA_CHAN); // start DMA request
		gpio_clear(GPIOC, GPIO13);
#endif
		for(int i = 0 ; i < 0x00fffff; i ++) __asm__("nop");
	}
	return 0;
}
