#include "data.h"
#include "view.h"
#include "proc.h"

int main() {

  view_start();
  // 段溢出？？局部变量销毁
  // proc_start();

  view_done();
  // proc_done();

  return 0;
}