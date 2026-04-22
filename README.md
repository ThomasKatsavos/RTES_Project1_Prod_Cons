# RTES: Project 1st - Producer-Consumer Threads
An implementation for the first exercise for the Real Time Embedded Systems course, mainly consisted of producer and consumer type threads and a FIFO queue.
The main idea is to modify the Original C code, in order to achieve the following:
* The FIFO queue must keep items of type:
```C
struct workFunction {
  void * (*work)(void *);
  void * arg;
}
```
* Producers and Consumers must respectively add and remove 'workFunction' items from the common queue, through pointers of functions
* Remove the 'sleep()' delays and modify the number of iterations in each loop, as well as the number of Producer and Consumer threads for further performance study.

### Compilation:  ``gcc  rtes_version_prod_cons.c -o rtes1_run -lpthread -lm``
### Execution(bash): ``./rtes1_run``
