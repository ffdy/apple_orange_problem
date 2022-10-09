#include "data.h"
// 状态矩阵，用于渲染图形
// 内存状态，0空闲，1apple生产占用，2apple等待消费
// 3orange生产占用，4orange等待消费
int memState[N] = {0};
// // [x][0]apple生产者，[x][1]orange生产者
// int pStatMatrix[N][2] = {0};
// // [x][0]apple消费者，[x][1]orange消费者
// int cStatMatrix[N][2] = {0};
// [x][0]apple producer [x][1]orange producer
// 0空闲，1阻塞，2生产中
// [x][2]apple consumer [x][3]orange consumer
// 0空闲，1阻塞，2消费中
int pcState[N][4] = {0};
// 生产者消费者工作颜色对应
float workColor[3][3] = {
    0.5f, 0.5f, 0.5f,
    1.0f, 1.0f, 0.0f,
    0.0f, 0.5f, 0.0f
};