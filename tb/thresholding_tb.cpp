#include "thresholding_top.hpp"
#include "thresholding_ref.h"

#include <array>
#include <iostream>
#include <random>
#include <vector>

namespace {

	hls::stream<tensor<TI, PE>>  src("src");
	hls::stream<tensor<TO, PE>>  dst("dst");
#if THRESHOLDING_SEL
	hls::stream<thresholding_set_t<SETS>>  sel("sel");
#endif
#if THRESHOLDING_CFG
	hls::stream<cfg_t>  cfg("cfg");
	hls::stream<TT>     rsp("rsp");
#endif

	std::mt19937  gen(1);
	unsigned  errors = 0;

	void top() {
		thresholding_top(
			src,
#if THRESHOLDING_SEL
			sel,
#endif
			dst
#if THRESHOLDING_CFG
			, cfg, rsp
#endif
		);
	}

	/** Process NIN inputs against the thresholds of phase p, each of a random set. */
	void process(unsigned const  p) {
		// Stimuli: random values, range edges, and thresholds with their neighbors
		std::vector<std::array<TO, PE>>  expected(NIN);
		for(unsigned  i = 0; i < NIN; i++) {
			unsigned const  cf = i % CF;
			unsigned const  set = SETS > 1? gen() % SETS : 0;
#if THRESHOLDING_SEL
			sel.write(set);
#endif
			tensor<TI, PE>  x;
			for(unsigned  pe = 0; pe < PE; pe++) {
				unsigned const  ch = cf*PE + pe;
				TI  v;
				switch(gen() % 8) {
				case 0:
				case 1:
				case 2:
					v = rand_input(gen);
					break;
				case 3:
					v = EDGES[gen() % (sizeof(EDGES)/sizeof(EDGES[0]))];
					break;
				default:
					v = perturb(THRESHS[p][set][ch][gen() % N], int(gen() % 3) - 1);
					break;
				}
				x[pe] = v;

				int  cnt = BIAS;
				for(unsigned  t = 0; t < N; t++)  cnt += THRESHS[p][set][ch][t]*THRESH_SCALE <= to_double(v);
				expected[i][pe] = TO(cnt);
			}
			src.write(x);
		}

		// One kernel invocation per input
		for(unsigned  i = 0; i < NIN; i++)  top();

		for(unsigned  i = 0; i < NIN; i++) {
			auto const  y = dst.read();
			for(unsigned  pe = 0; pe < PE; pe++) {
				TO const  got = y[pe];
				if(got != expected[i][pe]) {
					if(errors++ < 10) {
						std::cout << "ERROR phase " << p << " input " << i << " pe " << pe
							<< " expected " << expected[i][pe] << " got " << got << std::endl;
					}
				}
			}
		}
		if(!dst.empty())  std::cout << "ERROR: surplus output" << std::endl;
	}

#if THRESHOLDING_CFG
	/** Address of threshold n of channel ch in set, see thresholding.hpp. */
	unsigned addr_of(unsigned const  set, unsigned const  ch, unsigned const  n) {
		return  ((((((set << FB) + ch/PE) << PB) + ch%PE) << NB) | n);
	}
	struct field_t {
		unsigned  set, ch, n;
		bool      ok;
	};
	field_t decode(unsigned const  a) {
		unsigned const  n   = a & ((1u << NB) - 1);
		unsigned const  pe  = (a >> NB) & ((1u << PB) - 1);
		unsigned const  cf  = (a >> (NB + PB)) & ((1u << FB) - 1);
		unsigned const  set = a >> (NB + PB + FB);
		return { set, cf*PE + pe, n, (n < N) && (pe < PE) && (cf < CF) && (set < SETS) };
	}

	/** Write the thresholds of phase p that differ from phase q (all if q == p). */
	void write(unsigned const  p, unsigned const  q) {
		unsigned  ops = 0;
		if(p == q) {
			// Out-of-scope writes, which must be ignored, before the full set
			for(unsigned  a = 0; a < (1u << AW); a++) {
				if(!decode(a).ok) {
					cfg.write(cfg_t{ THRESHS_TT[p][0][0][0], a, true });
					ops++;
				}
			}
		}
		for(unsigned  set = 0; set < SETS; set++) {
			for(unsigned  ch = 0; ch < C; ch++) {
				for(unsigned  n = 0; n < N; n++) {
					if((p == q) || (THRESHS[p][set][ch][n] != THRESHS[q][set][ch][n])) {
						cfg.write(cfg_t{ THRESHS_TT[p][set][ch][n], addr_of(set, ch, n), true });
						ops++;
					}
				}
			}
		}
		for(unsigned  i = 0; i < ops; i++)  top();
		if(!rsp.empty())  std::cout << "ERROR: response to write" << std::endl;
	}

	/** Read back all addresses, expecting the thresholds of phase p in scope and zero beyond. */
	void readback(unsigned const  p) {
		for(unsigned  a = 0; a < (1u << AW); a++)  cfg.write(cfg_t{ TT(0), a, false });
		for(unsigned  a = 0; a < (1u << AW); a++)  top();
		for(unsigned  a = 0; a < (1u << AW); a++) {
			TT const  got = rsp.read();
			field_t const  f = decode(a);
			TT const  exp = f.ok? THRESHS_TT[p][f.set][f.ch][f.n] : TT(0);
			if(got != exp) {
				if(errors++ < 10) {
					std::cout << "ERROR readback phase " << p << " address " << a
						<< " expected " << exp << " got " << got << std::endl;
				}
			}
		}
		if(!rsp.empty())  std::cout << "ERROR: surplus response" << std::endl;
	}
#endif

} // anonymous namespace

int main() {
	if(HAS_INIT)  process(0);
#if THRESHOLDING_CFG
	if(HAS_INIT)  readback(0);
	write(1, 1);
	process(1);
	write(2, 1);
	process(2);
	readback(2);
#endif

	if(errors) {
		std::cout << errors << " errors detected." << std::endl;
		return  1;
	}
	std::cout << "Test passed. " << PHASES << " phase(s) of " << NIN << " inputs verified (N=" << N << ", C=" << C << ", PE=" << PE << ", SETS=" << SETS << ")." << std::endl;
	return  0;
}
