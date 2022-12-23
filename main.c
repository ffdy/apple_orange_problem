#include "data.h"
#include "view.h"
#include "proc.h"
#include <unistd.h>

int main() {

  view_start();
  sleep(3);
  proc_start();

  view_done();
  proc_done();

  return 0;
}