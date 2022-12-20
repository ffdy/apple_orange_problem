#include "data.h"

int mem_state[N] = {0};
int producer_consumer_state[N][4] = {0};

int work_time[N][4] = {0};
int free_time[N][4] = {0};

int mem_host[N];
int producer_consumer_target[N][4];