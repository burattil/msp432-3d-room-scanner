#ifndef TOF_SENSOR_H
#define TOF_SENSOR_H

// Define the number of measurements that will be taken (must match .py file)
#define XMEASUREMENTS 3

void Sensor_Init(void);
void Get_Measurements(void);
void Send_Measurements(void);
void Stop_Ranging(void);
int Measurements_Taken(void);

#endif