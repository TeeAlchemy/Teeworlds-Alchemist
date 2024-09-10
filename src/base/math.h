/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_MATH_H
#define BASE_MATH_H

#include <stdlib.h>
#include <random>

template <typename T>
inline T clamp(T val, T min, T max)
{
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

inline float sign(float f)
{
	return f < 0.0f ? -1.0f : 1.0f;
}

inline int round_truncate(float f)
{
	return (int)f;
}

inline int round_to_int(float f)
{
	if (f > 0)
		return (int)(f + 0.5f);
	return (int)(f - 0.5f);
}

template <typename T, typename TB>
inline T mix(const T a, const T b, TB amount)
{
	return a + (b - a) * amount;
}

float random_float();
bool random_prob(float f);
int random_int(int Min, int Max);
inline float frandom() { return rand() / (float)(RAND_MAX); }

// float to fixed
inline int f2fx(float v) { return (int)(v * (float)(1 << 10)); }
inline float fx2f(int v) { return v * (1.0f / (1 << 10)); }

inline int gcd(int a, int b)
{
	while (b != 0)
	{
		int c = a % b;
		a = b;
		b = c;
	}
	return a;
}

class fxp
{
	int value;

public:
	void set(int v) { value = v; }
	int get() const { return value; }
	fxp &operator=(int v)
	{
		value = v << 10;
		return *this;
	}
	fxp &operator=(float v)
	{
		value = (int)(v * (float)(1 << 10));
		return *this;
	}
	operator float() const { return value / (float)(1 << 10); }
};

const float pi = 3.1415926535897932384626433f;

template <typename T>
inline T min(T a, T b) { return a < b ? a : b; }
template <typename T>
inline T max(T a, T b) { return a > b ? a : b; }
template <typename T>
inline T mt_absolute(T a) { return a < T(0) ? -a : a; }

template <typename T>
constexpr inline T minimum(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
constexpr inline T minimum(T a, T b, T c)
{
	return minimum(minimum(a, b), c);
}

template <typename T>
constexpr inline T maximum(T a, T b)
{
	return a > b ? a : b;
}

template <typename T>
constexpr inline T maximum(T a, T b, T c)
{
	return maximum(maximum(a, b), c);
}

template <typename T>
constexpr inline T absolute(T a)
{
	return a < T(0) ? -a : a;
}

// percents
template < typename T> // char is arithmetic type we must exclude it 'a' / 'd' etc
using PercentArithmetic = typename std::enable_if < std::is_arithmetic  < T >::value && !std::is_same < T, char >::value, T >::type;

template <typename T> // derive from the number of percent e.g. ((100, 10%) = 10)
PercentArithmetic<T> translate_to_percent_rest(T value, float percent) { return (T)(((double)value / 100.0f) * (T)percent); }

template <typename T> // add to the number a percentage e.g. ((100, 10%) = 110)
PercentArithmetic<T> add_percent_to_source(T* pvalue, float percent)
{
	*pvalue = ((T)((double)*pvalue) * (1.0f + ((T)percent / 100.0f)));
	return (T)(*pvalue);
}

template <typename T> // translate from the first to the second in percent e.g. ((10, 5) = 50%)
PercentArithmetic<T> translate_to_percent(T from, T value) { return (T)(((double)value / (double)from) * 100.0f); }

template <typename T> // translate from the first to the second in percent e.g. ((10, 5, 50) = 25%)
PercentArithmetic<T> translate_to_percent(T from, T value, float maximum_percent) { return (T)(((double)value / (double)from) * maximum_percent); }

// translate to commas
// example: 201010 - 201,010
template<typename type, const char separator = ',', const unsigned num = 3>
static std::string get_commas(type Number)
{
	std::string NumberString;
	if constexpr(std::is_same_v<std::string, type>)
		NumberString = Number;
	else if constexpr(std::is_arithmetic_v<type>)
		NumberString = std::to_string(Number);
	else 
		NumberString(Number);

	if(NumberString.length() > (num + 1))
	{
		for(auto it = NumberString.rbegin(); (num + 1) <= std::distance(it, NumberString.rend());)
		{
			std::advance(it, num);
			NumberString.insert(it.base(), separator);
		}
	}

	return NumberString;
}


// translate to label
// example: 1021321 - 1,021 million
template<typename type, const char separator = ','>
static std::string get_label(type Number)
{
	constexpr unsigned num = 3;
	std::string NumberString;
	if constexpr(std::is_same_v<std::string, type>)
		NumberString = Number;
	else if constexpr(std::is_arithmetic_v<type>)
		NumberString = std::to_string(Number);
	else
		NumberString(Number);

	const char* pLabel[24] =
	{
		"--ignored--", // 1000
		"million", // 1000000
		"billion", // 1000000000
		"trillion", // 1000000000000
		"quadrillion",  // 1000000000000000
		"quintillion",  // 1000000000000000000
		"sextillion",  // 1000000000000000000000
		"septillion",  // 1000000000000000000000000
		"octillion",  // 1000000000000000000000000
		"nonillion",  // 1000000000000000000000000000
		"decillion",  // 1000000000000000000000000000000
		"undecillion",  // 1000000000000000000000000000000000
		"duodecillion",  // 1000000000000000000000000000000000000
		"tredecillion",  // 1000000000000000000000000000000000000000
		"quattuordecillion",  // 1000000000000000000000000000000000000000000
		"quindecillion",  // 1000000000000000000000000000000000000000000000
		"sexdecillion",  // 1000000000000000000000000000000000000000000000000
		"septendecillion",  // 1000000000000000000000000000000000000000000000000000
		"octodecillion",  // 1000000000000000000000000000000000000000000000000000000
		"novemdecillion",  // 1000000000000000000000000000000000000000000000000000000000
		"vigintillion",  // 1000000000000000000000000000000000000000000000000000000000
		"unvigintillion",  // 1000000000000000000000000000000000000000000000000000000000000
		"duovigintillion",  // 1000000000000000000000000000000000000000000000000000000000000000
		"trevigintillion"  // 1000000000000000000000000000000000000000000000000000000000000000000
	};

	if(NumberString.length() > (num + 1))
	{
		int Position = -1;
		auto iter = NumberString.end();
		for(auto it = NumberString.rbegin(); (num + 1) <= std::distance(it, NumberString.rend());)
		{
			if(iter != NumberString.end())
			{
				NumberString.erase(iter, NumberString.end());
			}

			std::advance(it, num);
			iter = NumberString.insert(it.base(), separator);
			Position++;
		}

		if(Position > 0 && Position < 24)
			NumberString.append(std::string(pLabel[Position]));
	}


	return NumberString;
}

#ifndef FCOM
// FCOM - format number / string (from 1321923 to 1,321,923)
#define FCOM(value) get_commas(value).c_str()
#endif

#ifndef FLAB
// FLAB - format number / string (from 1321923 to 1,321 million)
#define FLAB(value) get_label(value).c_str()
#endif

#endif // BASE_MATH_H
