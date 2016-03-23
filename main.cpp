#include <chrono>
#include <functional>
#include <iostream>
#include <memory>

#include "copy_on_write_ptr.hpp"

// === FORWARD DECLARATIONS ===

// Some forward declarations required to use std::chrono's timing functions
using Clock = std::chrono::system_clock;
using Duration = std::chrono::duration<float>;

// Generic timer for performance measurement purposes
template <typename DurationType = Duration>
DurationType time_it(const std::function<void()> & operation,
                     const std::size_t amount) {
   std::chrono::time_point<Clock> start_time, end_time;
   start_time = Clock::now();
   for(size_t i = 0; i < amount; ++i) {
      operation();
   }
   end_time = Clock::now();
   return end_time - start_time;
}

// Specialization for my comparison purposes
void compare_it(const std::function<void()> & shptr_operation,
                const std::function<void()> & cowptr_operation,
                const std::size_t amount) {
   const auto shptr_duration = time_it(shptr_operation, amount);
   std::cout << "With a raw shared_ptr, this operation takes " << shptr_duration.count() << " s" << std::endl;
   
   const auto cowptr_duration = time_it(cowptr_operation, amount);
   std::cout << "With cow_ptr, it takes " << cowptr_duration.count() << " s ("
             << cowptr_duration.count() / shptr_duration.count() << "x slower)" << std::endl;
}

// === PERFORMANCE TEST BODY ===

int main() {

   // === PART 0 : TEST-WIDE DEFINITIONS ===

   // Define the expected contents of our cow_ptr
   using Data = int;
   const Data typical_value = 42;
   using SharedPointer = std::shared_ptr<Data>;
   using COWPointer = copy_on_write_ptr<Data>;
   
   // Say hi :)
   std::cout << std::endl << "=== Microbenchmarking cow_ptr ===" << std::endl;

   // === PART 1 : CREATION FROM RAW POINTER ===
   
   const size_t creation_amount = 1000 * 1000 * 20;
   std::cout << std::endl << "Creating " << creation_amount << " pointers from raw pointers" << std::endl;
   {
      compare_it(
         [&](){
            const SharedPointer ptr{new Data{typical_value}};
         },
         [&](){
            const COWPointer ptr{new Data{typical_value}};
         },
         creation_amount
      );
   }
   
   // === PART 2 : CREATION + MOVE ===  (NOTE: Cannot test move alone yet, as that requires assignment operators)
   
   const size_t move_amount = 5 * creation_amount;
   std::cout << std::endl << "Creating AND moving " << move_amount << " pointers" << std::endl;
   {
      compare_it(
         [&](){
            SharedPointer source{new Data{typical_value}};
            const SharedPointer dest{std::move(source)};
         },
         [&](){
            COWPointer source{new Data{typical_value}};
            const COWPointer dest{std::move(source)};
         },
         move_amount
      );
   }
   
   // === PART 3 : COPY CONSTRUCTION ===
   
   const size_t copy_amount = 1000 * 1000 * 40;
   std::cout << std::endl << "Copying " << copy_amount << " pointers" << std::endl;
   {
      const SharedPointer source_shptr{std::make_shared<Data>(typical_value)};
      const COWPointer source_cowptr{new Data{typical_value}};
      
      compare_it(
         [&](){
            SharedPointer copy{source_shptr};
         },
         [&](){
            COWPointer copy{source_cowptr};
         },
         copy_amount
      );
   }
   
   // === PART 4 : READ DATA ===
   
   const size_t read_amount = 1000 * 1000 * 128;
   std::cout << std::endl << "Reading from " << read_amount << " pointers" << std::endl;
   {
      const SharedPointer source_shptr{std::make_shared<Data>(typical_value)};
      const COWPointer source_cowptr{new Data{typical_value}};
      
      compare_it(
         [&](){
            const Data & read = *source_shptr;
         },
         [&](){
            const Data & read = source_cowptr.read();
         },
         read_amount
      );
   }
   
   // === PART 5 : COPY + COLD WRITES ===  (NOTE: A pure cold write would require breaking encapsulation)
   
   const size_t cold_write_amount = 5 * copy_amount;
   std::cout << std::endl << "Performing " << cold_write_amount << " pointer copies AND cold writes" << std::endl;
   {
      const SharedPointer source_shptr{std::make_shared<Data>(typical_value)};
      const COWPointer source_cowptr{new Data{typical_value}};
      
      compare_it(
         [&](){
            SharedPointer dest_shptr{source_shptr};
            *dest_shptr = typical_value;
         },
         [&](){
            COWPointer dest_cowptr{source_cowptr};
            dest_cowptr.write(typical_value);
         },
         cold_write_amount
      );
   }
   
   // === PART 6 : WARM WRITES ===

   const size_t warm_write_amount = 4 * cold_write_amount;
   std::cout << std::endl << "Performing " << warm_write_amount << " warm pointer writes" << std::endl;
   {
      SharedPointer shptr{std::make_shared<Data>(typical_value)};
      COWPointer cowptr{new Data{typical_value}};
      
      compare_it(
         [&](){
            *shptr = typical_value;
         },
         [&](){
            cowptr.write(typical_value);
         },
         warm_write_amount
      );
   }

   // === TEST FINALIZATION ===

   std::cout << std::endl;
   return 0;
   
}
