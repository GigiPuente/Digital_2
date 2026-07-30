#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

void adc_init(void);
uint8_t adc_read(uint8_t channel);

#endif /* ADC_H_ */
