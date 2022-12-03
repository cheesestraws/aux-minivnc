#ifndef TIMER_H
#define TIMER_H

long start_us();
long us_since(long us);
void print_time_since(char* label, long us);

#endif