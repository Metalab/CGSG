#ifndef __EXTRAMATH_H
#define __EXTRAMATH_H

#ifdef _MSC_VER
inline float round(float n) { return floor(n+0.5f); }
inline double round(double n) { return floor(n+0.5); }
inline float log2(float n) {
	return log(n) / log(2.0f);
}

inline double log2(double n) {
	return log(n) / log(2.0);
}
#endif

#endif
