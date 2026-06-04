#ifndef TOF_SENSOR_H
#define TOF_SENSOR_H

// Define the number of measurements that will be taken (must match .py file)
#define XMEASUREMENTS 3

void Sensor_Init(void);
void Measure(void);
void Stop_Ranging(void);
void Measurements_Enable(void);
int Is_Enabled(void);

#endif