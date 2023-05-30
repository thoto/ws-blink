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

