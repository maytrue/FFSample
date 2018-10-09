#include <iostream>

#include <sys/time.h>
extern "C"
{
#include <libavformat/avformat.h>
}

int main() {

  unsigned version =  avformat_version();
  std::cout << "version:" << version << std::endl;

  struct timeval time;
  gettimeofday(&time, nullptr);

  uint64_t now = time.tv_sec + time.tv_usec / 1000  / 1000;
  std::cout << "now:" << now << std::endl;

  std::cout << "hello" << std::endl;

  return 0;
}