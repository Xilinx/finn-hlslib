#include <iostream>
#include <cassert>
#include "bipolar_top.hpp"

int main() {
	hls::stream<ap_uint<4>> in, out;
	int errors = 0;

	// --- Test constructor correctness via tensor packing ---
	// Bipolar(float): -1.0→0, +1.0→1
	{
		tensor<Bipolar, 4> t;
		t[0] = Bipolar(-1.0f);  // bit 0
		t[1] = Bipolar(+1.0f);  // bit 1
		t[2] = Bipolar(+1.0f);  // bit 1
		t[3] = Bipolar(-1.0f);  // bit 0
		// Expected: [3][2][1][0] = 0,1,1,0 = 0b0110 = 6
		if(t.data() != 6) {
			std::cerr << "FAIL: float constructor packing, got " << t.data() << " expected 6" << std::endl;
			errors++;
		}
	}

	// Bipolar(int): -1→0, +1→1, 0→0
	{
		tensor<Bipolar, 4> t;
		t[0] = Bipolar(0);   // bit 0
		t[1] = Bipolar(+1);  // bit 1
		t[2] = Bipolar(-1);  // bit 0
		t[3] = Bipolar(+1);  // bit 1
		// Expected: [3][2][1][0] = 1,0,1,0 = 0b1010 = 10
		if(t.data() != 10) {
			std::cerr << "FAIL: int constructor packing, got " << t.data() << " expected 10" << std::endl;
			errors++;
		}
	}

	// --- Test XNOR multiply through HLS function ---
	// Input: all +1 → bits 1,1,1,1 = 0b1111 = 15
	// XNOR: (1^~1)=1, (1^~1)=1 → popcount = 2
	// res: [3]=t[3]=1, [2]=t[0]=1, [1:0]=2 → 0b1110 = 14
	{
		in.write(ap_uint<4>(0b1111));
		bipolar_top(in, out);
		ap_uint<4> r = out.read();
		if(r != 14) {
			std::cerr << "FAIL: all-same XNOR, got " << r << " expected 14" << std::endl;
			errors++;
		}
	}

	// Input: alternating +1,-1,+1,-1 → bits 1,0,1,0 = 0b0101 = 5
	// XNOR: (1^~0)=0, (1^~0)=0 → popcount = 0
	// res: [3]=t[3]=0, [2]=t[0]=1, [1:0]=0 → 0b0100 = 4
	{
		in.write(ap_uint<4>(0b0101));
		bipolar_top(in, out);
		ap_uint<4> r = out.read();
		if(r != 4) {
			std::cerr << "FAIL: alternating XNOR, got " << r << " expected 4" << std::endl;
			errors++;
		}
	}

	// Input: -1,-1,+1,+1 → bits 0,0,1,1 = 0b1100 = 12
	// XNOR: (0^~0)=1, (1^~1)=1 → popcount = 2
	// res: [3]=t[3]=1, [2]=t[0]=0, [1:0]=2 → 0b1010 = 10
	{
		in.write(ap_uint<4>(0b1100));
		bipolar_top(in, out);
		ap_uint<4> r = out.read();
		if(r != 10) {
			std::cerr << "FAIL: half-half XNOR, got " << r << " expected 10" << std::endl;
			errors++;
		}
	}

	if(errors == 0)
		std::cout << "All Bipolar tests PASSED." << std::endl;
	else
		std::cerr << errors << " Bipolar test(s) FAILED." << std::endl;

	return  errors;
}
