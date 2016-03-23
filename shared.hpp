#ifndef SHARED_H
#define SHARED_H

#include <chrono>

// These shared facilities are used by all my copy-on-write benchmarking programs
namespace Shared {

   // Some forward declarations required to use std::chrono's timing functions
   using Clock = std::chrono::system_clock;
   using Duration = std::chrono::duration<float>;

   // Generic timer for performance measurement purposes
   template <typename Callable,
             typename DurationType = Duration>
   DurationType time_it(Callable && operation,
                        const std::size_t amount) {
      std::chrono::time_point<Clock> start_time, end_time;
      start_time = Clock::now();
      for(size_t i = 0; i < amount; ++i) {
         operation();
      }
      end_time = Clock::now();
      return end_time - start_time;
   }
   
   // Define the data type used by the test, and a typical value of it
   using Data = int;
   const Data typical_value = 42;
   
}

#endif
