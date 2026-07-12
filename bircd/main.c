
#include "bircd.h"

int	main(int ac, char **av)
{
  t_env	e;


  for (int i = 0; i < 10; i++)
  {
    print();
  }
  print();
  print();
  print();
  print();
  print();
  print();
  print();
  print();
  print();
  print();
  print();

  init_env(&e);
  get_opt(&e, ac, av);
  srv_create(&e, e.port);
  main_loop(&e);
  return (0);
}
