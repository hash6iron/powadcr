#pragma once
#include <math.h>
const float PI = 3.14159265358979f;

template <class T>
class SineWaveGenerator {
 public:
  // the scale defines the max value which is generated
  SineWaveGenerator(float amplitude, float phase = 0.0) {
    m_amplitude = amplitude;
    m_phase = phase;
  }

  bool begin(int sample_rate, float frequency = 0.0) {
    this->m_sample_rate = sample_rate;
    this->m_frequency = frequency;
    this->m_deltaTime = 1.0 / m_sample_rate;
    return true;
  }

  /// Provides a single sample
  virtual T nextSample() {
    float angle = double_Pi * m_cycles + m_phase;
    T result = m_amplitude * sinf(angle);
    m_cycles += m_frequency * m_deltaTime;
    if (m_cycles > 1.0) {
      m_cycles -= 1.0;
    }
    return result;
  }

 protected:
  int m_sample_rate;
  volatile float m_frequency = 0;
  float m_cycles = 0.0;  // Varies between 0.0 and 1.0
  float m_amplitude = 1.0;
  float m_deltaTime = 0.0;
  float m_phase = 0.0;
  float double_Pi = PI * 2.0;
};
