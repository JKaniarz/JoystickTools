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

/* Pick your default deadzone function.
 * inCircularDeadzone is most accurate
 * inOctagonDeadzone is pretty good and faster on old hardware.
 * inSquareDeadzone is fastest/least accurate
 */
#define preferredDeadzone  inCircularDeadzone

#ifndef JT_NO_TRIG
#define _USE_MATH_DEFINES
#include <math.h>
#endif

//return abs(x) - signbit(x) so that -32768 wont overflow
inline constexpr int32_t absx(int32_t x) {
	return x ^ (x >> 31);
}
inline constexpr int16_t absx(int16_t x) {
	return x ^ (x >> 15);
}
inline constexpr int8_t absx(int8_t x) {
	return x ^ (x >> 7);
}

/* dpadErrorClear
 * removes bad inputs from really sloppy dpads.
 * 3 directions pressed -> middle of 3 only.
 * 4 directions pressed -> no directions pressed.
 */
inline uint8_t dpadErrorClear(uint8_t dpad) {
	if ((dpad & JT_DPAD_XAXIS) == JT_DPAD_XAXIS) dpad &= ~JT_DPAD_XAXIS;
	if ((dpad & JT_DPAD_YAXIS) == JT_DPAD_YAXIS) dpad &= ~JT_DPAD_YAXIS;
	return dpad;
}

/* dpadDiagonalFilter
 * Maps diagonal inputs to cardinal inputs by making the previous input
 * "sticky" this helps on Xbox360 pads which tend to rock side to side 
 * when pressing down.
 */
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

	//rotate 45 degrees CCW because a square with verticies on XY axes is easier to solve.
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
	uint8_t table[4] = {
		JT_DPAD_RIGHT,
		JT_DPAD_UP,
		JT_DPAD_LEFT,
		JT_DPAD_DOWN
	};
	return table[quadrant & 3];
}

/* axisInDeadzone
 * Is the axis/trigger inside the deadzone.
 */
inline bool axisInDeadzone(int16_t axis, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	return absx(axis) < deadzone;
}

/* inSquareDeadzone
 * Is the joystick inside a square of width 'deadzone'
 * The square is oriented flat-side up.
 */
inline bool inSquareDeadzone(int16_t x, int16_t y, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	return max(absx(x), absx(y)) < deadzone;
}

/* inOctagonDeadzone
 * Is the joystick inside an octagon circumscribed by a cicle of radius 'deadzone'
 * Points are UDLR and at 45s (ie. pointy, not flat, on top)
 * On old hardware this is faster than a circle because it's multiply free.
 */
inline bool inOctagonDeadzone(int16_t x, int16_t y, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	x = absx(x);
	y = absx(y);
	if (x > y) {
		return x < JT_DEFAULT_DEADZONE - y / 2;
	} else {
		return y < JT_DEFAULT_DEADZONE - x / 2;
	}
}

/* inCircularDeadzone
 * Is the stick inside a circle of radius 'deadzone'?
 */
inline bool inCircularDeadzone(int16_t x, int16_t y, int16_t deadzone = JT_DEFAULT_DEADZONE) {
	return ((uint32_t)((int32_t)x*x) + (uint32_t)((int32_t)y*y) < (uint32_t)((int32_t)deadzone*deadzone));
}

/* dpadFromAnalog
 * Convert analog stick into a dpad.
 */
inline uint8_t dpadFromAnalog(int16_t x, int16_t y) {
	return preferredDeadzone(x, y) ? 0: dpadFromOctant(octantFromCartesian(x, y));
}

/* hysteresis
 * Waits for analog stick to move a certain amount before updating position.
 * Probably useless because modern joystics do this internally. But if you have
 * a really noisy joistick this will still the position.
 */
inline void hysteresis(int16_t *x, int16_t *y, int16_t newX, int16_t newY, uint16_t minDistance) {
	if(preferredDeadzone(newX-*x, newY-*y)) {
		*x = newX;
		*y = newY;
	}
}

/* Debounce2
 * Waits for three consistent inputs before changing button state.
 * Probably useless because modern joystics come debounced.
 */
struct Debounce2 {
	uint16_t prevOut = 0;
	uint16_t prevIn = 0;
	uint16_t operator()(uint16_t in) {
		uint16_t mask = (prevIn ^ in);
		prevOut = (prevOut & mask) | (in & ~mask);
		prevIn = in;
		return prevOut;
	}
};

/* Debounce3
 * Waits for three consistent inputs before changing button state.
 * Probably useless because modern joystics come debounced.
 */
struct Debounce3 {
	uint16_t prevOut = 0;
	uint16_t prevIn2 = 0;
	uint16_t prevIn1 = 0;
	uint16_t operator()(uint16_t in) {
		uint16_t mask = (prevIn2 & prevIn1 & in) ^ (prevIn2 | prevIn1 | in);
		prevOut = (prevOut & mask) | (in & ~mask);
		prevIn2 = prevIn1;
		prevIn1 = in;
		return prevOut;
	}
};

/* Debounce4
 * Waits for four consistent inputs before changing button state.
 * Probably useless because modern joystics come debounced.
 */
struct Debounce4 {
	uint16_t prevOut = 0;
	uint16_t prevIn3 = 0;
	uint16_t prevIn2 = 0;
	uint16_t prevIn1 = 0;
	uint16_t operator()(uint16_t in) {
		uint16_t mask = (prevIn3 & prevIn2 & prevIn1 & in) ^ (prevIn3 | prevIn2 | prevIn1 | in);
		prevOut =  (prevOut & mask) | (in & ~mask);
		prevIn3 = prevIn2;
		prevIn2 = prevIn1;
		prevIn1 = in;
		return prevOut;
	}
};

struct JoystickTools {
	uint8_t dPadDpad = 0;
	uint8_t dPadUDLR = 0;
	uint8_t lStickDpad = 0;
	uint8_t lStickUDLR = 0;
	uint8_t rStickDpad = 0;
	uint8_t rStickUDLR = 0;

	void update(XINPUT_GAMEPAD &gamepad) {
		dPadUDLR = dpadDiagonalFilter(gamepad.wButtons & JT_DPAD_MASK, dPadDpad, dPadUDLR);
		dPadDpad = dpadErrorClear(gamepad.wButtons & JT_DPAD_MASK);
		uint8_t newLStickDpad = dpadFromAnalog(gamepad.sThumbLX, gamepad.sThumbLY);
		lStickUDLR = dpadDiagonalFilter(newLStickDpad, lStickDpad, lStickUDLR);
		lStickDpad = newLStickDpad;
		uint8_t newRStickDpad = dpadFromAnalog(gamepad.sThumbRX, gamepad.sThumbRY);
		rStickUDLR = dpadDiagonalFilter(newRStickDpad, rStickDpad, rStickUDLR);
		rStickDpad = newRStickDpad;
	}
};