#ifndef PARALLELIZE_H__
#define PARALLELIZE_H__

#ifndef USE_SIMD
#	error turn on SSE support with USE_SIMD macro
#endif

#include <xmmintrin.h>


namespace clunk {

template<int N, typename T>
struct sse_danielson_lanczos {
	typedef __m128 sse_type;
	enum { SSE_DIV = sizeof(sse_type) / sizeof(float) };

	typedef sse_danielson_lanczos<N / 2, float> next_type;

	static void apply(sse_type * data_re, sse_type * data_im, bool inversion) {
		next_type::apply(data_re, data_im, inversion);
		next_type::apply(data_re + N / 2, data_im + N / 2, inversion);
			
		int sign = inversion? -1: 1;
		T a = (T)(-2 * M_PI / N * sign);
		T wtemp = sin(a / 2);
		
		std::complex<T> wp (-2 * wtemp * wtemp, sin(a)), w(1, 0);

		for (unsigned i = 0; i < N / 2 ; ++i) {
			int j = i + N / 2;
			
			float w_re_buf[SSE_DIV], w_im_buf[SSE_DIV];
			for (unsigned k = 0; k < SSE_DIV; ++k) {
				w_re_buf[k] = w.real();
				w_im_buf[k] = w.imag();
				w += w * wp;
			}
			sse_type w_re, w_im;
			w_re = _mm_loadu_ps(w_re_buf);
			w_im = _mm_loadu_ps(w_im_buf);
			
			sse_type temp_re = _mm_sub_ps(_mm_mul_ps(data_re[j], w_re), _mm_mul_ps(data_im[j], w_im)), 
					 temp_im = _mm_add_ps(_mm_mul_ps(data_im[j], w_re), _mm_mul_ps(data_re[j], w_im));

			data_re[j] = _mm_sub_ps(data_re[i], temp_re);
			data_im[j] = _mm_sub_ps(data_im[i], temp_im);
			data_re[i] = _mm_add_ps(data_re[i], temp_re);
			data_im[i] = _mm_add_ps(data_im[i], temp_im);
		}
	};
};

template<typename T>
struct sse_danielson_lanczos<1, T> {
	typedef __m128 sse_type;
	enum { SSE_DIV = sizeof(sse_type) / sizeof(T) };

	typedef danielson_lanczos<SSE_DIV, T> next_type;

	static void apply(sse_type * data_re, sse_type * data_im, bool inversion) {
		float re[SSE_DIV], im[SSE_DIV]; 
		_mm_storeu_ps(re, *data_re);
		_mm_storeu_ps(im, *data_re);

		std::complex<T> data[SSE_DIV];
		for(unsigned i = 0; i < SSE_DIV; ++i) {
			data[i] = std::complex<T>(re[i], im[i]);
		}
		
		next_type::apply(data, inversion);

		for(unsigned i = 0; i < SSE_DIV; ++i) {
			re[i] = data[i].real();
			im[i] = data[i].imag();
		}
		
		*data_re = _mm_loadu_ps(re);
		*data_im = _mm_loadu_ps(im);
	}
};


template<int BITS>
class fft_context<BITS, float> {
public: 
	typedef __m128 sse_type;
	
	enum { N = 1 << BITS };
	enum { SSE_DIV = sizeof(sse_type) / sizeof(float) };
	//static assert
	enum { SSE_N = (N - 1) / SSE_DIV + 1 };

	typedef std::complex<float> value_type;
	value_type data[N];
	
	inline void fft(bool inversion) {
		scramble();
		load();
		next.apply(data_re, data_im, inversion);
		if (inversion) {
			sse_type n = _mm_set_ps1(N);
			for(unsigned i = 0; i < SSE_N; ++i) {
				data_re[i] = _mm_div_ps(data_re[i], n);
				data_im[i] = _mm_div_ps(data_im[i], n);
			}
		}
		save();
	}

private:
	sse_type data_re[SSE_N], data_im[SSE_N];
	sse_danielson_lanczos<SSE_N, float> next;

	void load() {
		for(int i = 0; i < SSE_N; ++i) {
			float buf_re[SSE_DIV], buf_im[SSE_DIV];
			for(int j = 0; j < SSE_DIV; ++j) {
				int idx = i * SSE_DIV + j; 
				buf_re[j] = (idx < N)? data[idx].real(): 0;
				buf_im[j] = (idx < N)? data[idx].imag(): 0;
			}
			data_re[i] = _mm_loadu_ps(buf_re);
			data_im[i] = _mm_loadu_ps(buf_im);
		}
	}

	void save() {
		for(int i = 0; i < SSE_N; ++i) {
			float buf_re[SSE_DIV], buf_im[SSE_DIV];
			_mm_storeu_ps(buf_re, data_re[i]);
			_mm_storeu_ps(buf_im, data_im[i]);
			
			for(int j = 0; j < SSE_DIV; ++j) {
				int idx = i * SSE_DIV + j;
				if (idx >= N)
					break;
				data[idx] = value_type(buf_re[j], buf_im[j]);
			}
		}
	}

	template<typename V>
	static inline void swap(V &a, V &b) {
		V t = a;
		a = b; 
		b = t;
	}

	void scramble() {
		int j = 0;
		for(int i = 0; i < N; ++i) {
			if (i > j) {
				swap(data[i], data[j]);
			}
			int m = N / 2;
			while(j >= m && m >= 2) {
				j -= m;
				m >>= 1;
			}
			j += m;
		}
	}
};

}

#endif