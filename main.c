#include "data.h"
#include "view.h"
#include "proc.h"

int main() {

  view_start();
  proc_start();

  view_done();
  proc_done();

  return 0;
}