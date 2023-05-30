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
