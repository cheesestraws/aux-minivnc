package main

import (
	"fmt"
)

func main() {
	fmt.Printf("#include \"luts.h\"\n\n");

	// 1-bit lut:	

	fmt.Printf("unsigned int lut1bit[16] = {\n");
	for i := 0; i < 16; i++ {
		var number string
		
		for j := 0; j < 4; j++ {
			bit := i & (1 << j)
			if (bit > 0) {
				number = "01" + number
			} else {
				number = "00" + number
			}
		}
		number = "0x" + number
		
		fmt.Printf("\t%s,\n", number)
	}
	fmt.Printf("};\n\n");
	
	// 2-bit lut
	fmt.Printf("unsigned int lut2bit[256] = {\n");

	for i := 0; i < 256; i++ {
		var number string
		for j := 0; j < 4; j++ {
			blob := (i >> (j * 2)) & 3
			number = fmt.Sprintf("0%x", blob) + number
		}
		number = "0x" + number
		fmt.Printf("\t%s,\n", number)
	}
	fmt.Printf("};\n\n");

	// 4-bit lut
	fmt.Printf("unsigned short lut4bit[256] = {\n");

	for i := 0; i < 256; i++ {
		var number string
		for j := 0; j < 2; j++ {
			blob := (i >> (j * 4)) & 15
			number = fmt.Sprintf("%02x", blob) + number
		}
		number = "0x" + number
		fmt.Printf("\t%s,\n", number)
	}
	fmt.Printf("};\n\n");


}