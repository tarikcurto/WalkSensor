#ifndef MPU6050_I2C_H
#define MPU6050_I2C_H

void mpu6050_init();
void mpu6050_read(float accel[3], float gyro[3], float *temp);
#endif