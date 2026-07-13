#pragma once

/**
 * @brief Tape animation functions for HMI display
 * Called during I2S codec operations to show visual feedback
 */

void tapeAnimationON(bool cd);
void tapeAnimationOFF(bool cd);
void resetTapeAnimation();
void downSpinMotorAnimation();
