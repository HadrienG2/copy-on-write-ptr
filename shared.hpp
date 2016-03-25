/*  This file is part of copy_on_write_ptr.

    copy_on_write_ptr is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    copy_on_write_ptr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with copy_on_write_ptr.  If not, see <http://www.gnu.org/licenses/>. */

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
