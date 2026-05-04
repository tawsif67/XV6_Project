#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int tickets;

  if(argc != 2){
    fprintf(2, "usage: test_scheduler tickets\n");
    exit(1);
  }

  tickets = atoi(argv[1]);
  if(tickets <= 0){
    fprintf(2, "tickets must be positive\n");
    exit(1);
  }

  if(settickets(tickets) < 0){
    fprintf(2, "settickets failed\n");
    exit(1);
  }

  for(;;)
    ;
}
