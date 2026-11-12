#pragma once

#include <cstddef>  // for size_t
#include <stdint.h>
#include <string.h>  // for memcpy

/**
 * @brief Fast clamp/constrain function optimized for ESP32.
 *
 * Replaces Arduino's constrain() with faster inline implementation.
 * Uses simple if-statements which are faster than the macro version.
 *
 * @tparam T The type of value to clamp
 * @param value The value to clamp
 * @param minVal The minimum value
 * @param maxVal The maximum value
 * @return Clamped value
 */
template <typename T>
inline T fastClamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

/**
 * @brief Fast integer map function optimized for ESP32.
 *
 * Replaces Arduino's map() with optimized inline computation.
 * Uses 32-bit math to prevent overflow and clamps result to output range.
 *
 * @param x Input value
 * @param inMin Input range minimum
 * @param inMax Input range maximum
 * @param outMin Output range minimum
 * @param outMax Output range maximum
 * @return Mapped and clamped value
 */
template <typename T>
inline T fastMap(T x, T inMin, T inMax, T outMin, T outMax) {
    // Pre-calculate ranges to avoid redundant subtraction
    T inRange = inMax - inMin;
    T outRange = outMax - outMin;

    // Compute mapped value: (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin
    T result = ((x - inMin) * outRange) / inRange + outMin;

    // Clamp to output range
    return fastClamp(result, outMin, outMax);
}

/**
 * @brief Generic median filter using insertion sort.
 *
 * Applies median filter to an array of values for noise rejection.
 * Uses insertion sort (O(n²) worst case but O(n) best case for nearly-sorted data).
 * For small arrays (< 10 elements), this is faster than quicksort and doesn't require ESP-DSP.
 *
 * Median filter is more effective than averaging for removing outliers/spikes.
 * Uses memcpy for DMA-optimized copying on ESP32.
 *
 * @tparam T The type of array elements (must support comparison operators)
 * @tparam N The size of the array
 * @param buffer The input buffer array
 * @param count The number of valid elements to process (can be less than N)
 * @return The median value
 */
template <typename T, size_t N>
T medianFilter(const T* buffer, size_t count) {
    if (count == 0) return T(0);
    if (count > N) count = N;

    // Create a copy of buffer for sorting (don't modify original)
    T sorted[N];
    memcpy(sorted, buffer, count * sizeof(T));

    // Insertion sort - optimized for small, nearly-sorted arrays
    for (size_t i = 1; i < count; i++) {
        T key = sorted[i];
        int j = i - 1;

        // Shift elements greater than key to the right
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }

    // Return median value (middle element)
    return sorted[count / 2];
}

/**
 * @brief Returns the length of a fixed-size array.
 *
 * This function template returns the number of elements in a
 * fixed-size array. It works for any type and size of array.
 *
 * @tparam T The type of the array elements.
 * @tparam N The number of elements in the array.
 * @param array The array whose length is to be determined.
 * @return The number of elements in the array.
 */
template <typename T, size_t N>
constexpr size_t arrayLength(T (&)[N]) {
    return N;
}
