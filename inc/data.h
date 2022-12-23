// 内存数量
#define N 20

// 状态矩阵，用于渲染图形
#define MEM_FREE            0 // 空闲
#define MEM_APPLE_PRODUCE   1 // apple生产占用
#define MEM_APPLE_WAITING   2 // apple等待消费
#define MEM_APPLE_CONSUME   5 // apple消费占用
#define MEM_ORANGE_PRODUCE  3 // orange生产占用
#define MEM_ORANGE_WAITING  4 // orange等待消费
#define MEM_ORANGE_CONSUME  6 // orange消费占用
// 内存状态
extern int mem_state[N];

#define APPLE_PRODUCER  0
#define ORANGE_PRODUCER 1
#define APPLE_CONSUMER  2
#define ORANGE_CONSUMER 3
#define MEMORY          4

#define FREE      0 // 空闲
#define WAITING   1 // 阻塞
#define PRODUCING 2 // 生产中
#define CONSUMING 2 // 消费中
// [x][APPLE_PRODUCER]apple producer [x][ORANGE_PRODUCER]orange producer
// [x][APPLE_CONSUMER]apple consumer [x][ORANGE_CONSUMER]orange consumer
extern int producer_consumer_state[N][4];

// 生产消费者工作和空闲时间
extern int work_time[N][4];
extern int free_time[N][4];

extern int mem_host[N];
extern int producer_consumer_target[N][4];