#pragma once
#include<Xinput.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <assert.h>

#define JT_NO_TRIG 1


#ifdef XINPUT_DLL
#define JT_DPAD_UP 1
#define JT_DPAD_DOWN 2
#define JT_DPAD_LEFT 4
#define JT_DPAD_RIGHT 8
#define JT_DEFAULT_DEADZONE 8192
#else
#define JT_DPAD_UP 2
#define JT_DPAD_DOWN 8
#define JT_DPAD_LEFT 4
#define JT_DPAD_RIGHT 1
#define JT_DEFAULT_DEADZONE 8192
#endif

#define JT_DPAD_XAXIS  (JT_DPAD_LEFT | JT_DPAD_RIGHT)
#define JT_DPAD_YAXIS  (JT_DPAD_UP | JT_DPAD_DOWN)
#define JT_DPAD_MASK (JT_DPAD_XAXIS | JT_DPAD_YAXIS)


#ifndef JT_NO_TRIG
#define _USE_MATH_DEFINES
#include <math.h>
#endif

//return abs(x) - signbit(x) so that -32768 wont overflow
int32_t absx(int32_t x) {
	return x ^ (x >> 31);
}
int16_t absx(int16_t x) {
	return x ^ (x >> 15);
}

inline uint8_t dpadErrorClear(uint8_t dpad) {
	if ((dpad & JT_DPAD_XAXIS) == JT_DPAD_XAXIS) dpad &= ~JT_DPAD_XAXIS;
	if ((dpad & JT_DPAD_YAXIS) == JT_DPAD_YAXIS) dpad &= ~JT_DPAD_YAXIS;
	return dpad;
}

inline uint8_t dpadDiagonalFilter(uint8_t input, uint8_t prevInput, uint8_t prevOutput) {
	if (input & prevOutput) return prevOutput;
	input = dpadErrorClear(input);
	if (input & prevInput) input = input & prevInput;
	if (input & JT_DPAD_XAXIS) return input & JT_DPAD_XAXIS;
	return input & JT_DPAD_YAXIS;
}

inline uint8_t quadrantFromCartesian(int16_t x, int16_t y) {
#ifdef JT_NO_TRIG
	constexpr int32_t s45 = 23170; // sin(45)*32768

	//rotate 45 degrees CCW because an octagon with verticies on axes is easier to solve.
	int32_t x2 = s45 *((int32_t)x - y);
	int32_t y2 = s45 *((int32_t)x + y);

	return ((y2 >> 31) & 3) ^ ((x2 >> 31) & 1);
#else
	return (uint8_t)(2.0f/ M_PI * atan2f(y, x) + 4.5f) & 3;
#endif
}

inline uint8_t octantFromCartesian(int16_t x, int16_t y) {
#ifdef JT_NO_TRIG
	constexpr int s22 = 12540; // sin(22.5)*32768
	constexpr int c22 = 30274; // cos(22.5)*32768

	//rotate 22.5 degrees CCW because an octagon with verticies on axes is easier to solve.
	int32_t x2 = x*c22 - y*s22;
	int32_t y2 = x*s22 + y*c22;
	
	//solved via kmap the 3 output bits are signbit(y2), signbit(y2)^signbit(x2), signbit(y2)^signbit(x2)^(x<y)
	return ((y2 >>31) & 7) ^ ((x2>>31) & 3) ^ ((uint32_t)(absx(x2) - absx(y2)) >> 31);
#else
	return (uint8_t)(4.0f / M_PI * atan2f(y, x) + 8.5f) & 7;
#endif
}

inline uint8_t dpadFromOctant(uint8_t octant) {
	uint8_t table[8] = {
		JT_DPAD_RIGHT,
		JT_DPAD_RIGHT | JT_DPAD_UP,
		JT_DPAD_UP,
		JT_DPAD_UP | JT_DPAD_LEFT,
		JT_DPAD_LEFT,
		JT_DPAD_LEFT | JT_DPAD_DOWN,
		JT_DPAD_DOWN,
		JT_DPAD_DOWN | JT_DPAD_RIGHT
	};
	return table[octant & 7];
}
inline uint8_t dpadFromQuadrant(uint8_t quadrant) {
	uint8_t table[8] = {
		JT_DPAD_RIGHT,
		JT_DPAD_UP,
		JT_DPAD_LEFT,
		JT_DPAD_DOWN
	};
	return table[quadrant & 3];
}

inline bool axisInDeadzone(int16_t axis, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	return absx(axis) < deadzone;
}
inline bool inSquareDeadzone(int16_t x, int16_t y, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	return max(absx(x), absx(y)) < deadzone;
}
inline bool inCircularDeadzone(int16_t x, int16_t y, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	return ((uint32_t)((int32_t)x*x) + (uint32_t)((int32_t)y*y) < (uint32_t)((int32_t)deadzone*deadzone));
}

inline WORD dpadFromAnalog(int16_t x, int16_t y) {
	//return inCircularDeadzone(x, y) ? 0: dpadFromOctant(octantFromCartesian(x, y));
	return inCircularDeadzone(x, y) ? 0 : dpadFromQuadrant(quadrantFromCartesian(x, y));
}

inline void hysteresis(int16_t *x, int16_t *y, int16_t newX, int16_t newY, uint16_t minDistance) {
	//TODO: is the abs necessary to avoid 
	uint32_t dx = (int32_t)newX - *x;
	uint32_t dy = (int32_t)newY - *y;
	if (dx*dx + dy*dy > minDistance*minDistance) {
		*x = newX;
		*y = newY;
	}
}
/* debounce2
 * waits for two consistent inputs before changing button state.
 */
inline uint16_t debounce2(uint16_t prevOut, uint16_t prevIn, uint16_t in) {
	uint16_t mask = (prevIn^in);
	return (prevOut & mask) | (in & ~mask);
}

/* debounce4
* waits for four consistent inputs before changing button state.
*/
inline uint16_t debounce4(uint16_t prevOut, uint16_t prevIn, uint16_t in) {
	uint16_t mask = (prevIn^in);
	return (prevOut & mask) | (in & ~mask);
}

class JoystickTools {
	WORD dPadDpad;
	WORD dPadUDLR;
	WORD lStickDpad;
	WORD lStickUDLR;
	WORD rStickDpad;
	WORD rStickUDLR;
	void update(XINPUT_GAMEPAD &gamepad) {
		dPadUDLR = dpadDiagonalFilter(gamepad.wButtons & JT_DPAD_MASK, dPadDpad, dPadUDLR);
		dPadDpad = dpadErrorClear(gamepad.wButtons & JT_DPAD_MASK);
	}
};