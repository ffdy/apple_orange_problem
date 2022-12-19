// 内存数量
#define N 20

// 状态矩阵，用于渲染图形
// 内存状态，0空闲，1apple生产占用，2apple等待消费, 5apple消费占用
// 3orange生产占用，4orange等待消费，6orange消费占用
extern int mem_state[N];
// // [x][0]apple生产者，[x][1]orange生产者
// int pStatMatrix[N][2] = {0};
// // [x][0]apple消费者，[x][1]orange消费者
// int cStatMatrix[N][2] = {0};
// [x][0]apple producer [x][1]orange producer
// 0空闲，1阻塞，2生产中
// [x][2]apple consumer [x][3]orange consumer
// 0空闲，1阻塞，2消费中
extern int producer_consumer_state[N][4];

// 生产消费者工作和空闲时间
extern int work_time[N][4];
extern int free_time[N][4];

extern int mem_host[N];
extern int producer_consumer_target[N][4];