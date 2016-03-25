#include <iostream>

#include "copy_on_write_ptr.hpp"
#include "cow_ownership_flags/thread_unsafe_flag.hpp"
#include "cow_ownership_flags/manually_ordered_atomics_flag.hpp"
#include "shared.hpp"

// === FORWARD DECLARATIONS ===

// Import shared definitions
using namespace Shared;

// Specialization of time_it for my comparison purposes
template<typename Callable1,
         typename Callable2>
void compare_it(Callable1 && unsafe_operation,
                Callable2 && tested_operation,
                const std::size_t amount) {
   const auto unsafe_duration = Shared::time_it(unsafe_operation, amount);
   std::cout << "With a thread-unsafe implementation, this operation takes " << unsafe_duration.count() << " s" << std::endl;
   
   const auto tested_duration = Shared::time_it(tested_operation, amount);
   std::cout << "With the tested implementation, it takes " << tested_duration.count() << " s ("
             << tested_duration.count() / unsafe_duration.count() << "x slower)" << std::endl;
}

// === PERFORMANCE TEST BODY ===

int main() {

   // === PART 0 : TEST-WIDE DEFINITIONS ===

   // Define our smart pointer types
   using UnsafePointer = copy_on_write_ptr<Data, cow_ownership_flags::thread_unsafe_flag>;
   using TestedPointer = copy_on_write_ptr<Data, cow_ownership_flags::manually_ordered_atomics_flag>;
   
   // Say hi :)
   std::cout << std::endl << "=== Microbenchmarking cow_ptr ===" << std::endl;

   // === PART 1 : CREATION FROM RAW POINTER ===
   
   const size_t creation_amount = 1000 * 1000 * 100;
   std::cout << std::endl << "Creating " << creation_amount << " pointers from raw pointers" << std::endl;
   {
      compare_it(
         [&](){
            UnsafePointer ptr{new Data{typical_value}};
         },
         [&](){
            TestedPointer ptr{new Data{typical_value}};
         },
         creation_amount
      );
   }
   
   // === PART 2 : CREATION + MOVE-CONSTRUCTION ===  (NOTE: Cannot test move construction alone easily)
   
   const size_t move_amount = 25 * creation_amount;
   std::cout << std::endl << "Creating AND move-constructing " << move_amount << " pointers" << std::endl;
   {
      compare_it(
         [&](){
            UnsafePointer source{new Data{typical_value}};
            const UnsafePointer dest{std::move(source)};
         },
         [&](){
            TestedPointer source{new Data{typical_value}};
            const TestedPointer dest{std::move(source)};
         },
         move_amount
      );
   }
   
   // === PART 3 : COPY CONSTRUCTION ===
   
   const size_t copy_amount = 1000 * 1000 * 1000;
   std::cout << std::endl << "Copy-constructing " << copy_amount << " pointers" << std::endl;
   {
      const UnsafePointer source_unsafe{new Data{typical_value}};
      const TestedPointer source_tested{new Data{typical_value}};
      
      compare_it(
         [&](){
            UnsafePointer copy{source_unsafe};
         },
         [&](){
            TestedPointer copy{source_tested};
         },
         copy_amount
      );
   }
   
   // === PART 4 : COPY CONSTRUCTION + MOVE-ASSIGNMENT ===  (NOTE: Cannot test move assignment alone easily)
   
   const size_t copy_move_amount = 5 * copy_amount;
   std::cout << std::endl << "Copy-constructing AND move-assigning " << copy_move_amount << " pointers" << std::endl;
   {
      const UnsafePointer source_unsafe{new Data{typical_value}};
      const TestedPointer source_tested{new Data{typical_value}};
      
      UnsafePointer dest_unsafe{source_unsafe};
      TestedPointer dest_tested{source_tested};
      
      compare_it(
         [&](){
            UnsafePointer copy{source_unsafe};
            dest_unsafe = std::move(copy);
         },
         [&](){
            TestedPointer copy{source_tested};
            dest_tested = std::move(copy);
         },
         copy_move_amount
      );
   }
   
   // === PART 5 : COPY ASSIGNMENT ===
   
   const size_t copy_assign_amount = 1000 * 1000 * 64;
   std::cout << std::endl << "Copy-assigning " << copy_assign_amount << " pointers" << std::endl;
   {
      const UnsafePointer source_unsafe{new Data{typical_value}};
      const TestedPointer source_tested{new Data{typical_value}};
      
      UnsafePointer dest_unsafe{source_unsafe};
      TestedPointer dest_tested{source_tested};
      
      compare_it(
         [&](){
            dest_unsafe = source_unsafe;
         },
         [&](){
            dest_tested = source_tested;
         },
         copy_assign_amount
      );
   }
   
   // === PART 6 : READ DATA ===
   
   const size_t read_amount = 1000ULL * 1000ULL * 1000ULL * 5ULL;
   std::cout << std::endl << "Reading from " << read_amount << " pointers" << std::endl;
   {
      const UnsafePointer source_unsafe{new Data{typical_value}};
      const TestedPointer source_tested{new Data{typical_value}};
      
      compare_it(
         [&](){
            const Data & read = source_unsafe.read();
         },
         [&](){
            const Data & read = source_tested.read();
         },
         read_amount
      );
   }
   
   // === PART 7 : COPY ASSIGNMENT + COLD WRITES ===  (NOTE: A pure cold write would require breaking encapsulation)
   
   const size_t cold_write_amount = 30 * copy_assign_amount;
   std::cout << std::endl << "Performing " << cold_write_amount << " pointer copies AND cold writes" << std::endl;
   {
      const UnsafePointer source_unsafe{new Data{typical_value}};
      const TestedPointer source_tested{new Data{typical_value}};
      
      UnsafePointer dest_unsafe{source_unsafe};
      TestedPointer dest_tested{source_tested};
      
      compare_it(
         [&](){
            dest_unsafe = source_unsafe;
            dest_unsafe.write(typical_value);
         },
         [&](){
            dest_tested = source_tested;
            dest_tested.write(typical_value);
         },
         cold_write_amount
      );
   }
   
   // === PART 8 : WARM WRITES ===

   const size_t warm_write_amount = cold_write_amount;
   std::cout << std::endl << "Performing " << warm_write_amount << " warm pointer writes" << std::endl;
   {
      UnsafePointer unsafe{new Data{typical_value}};
      TestedPointer tested{new Data{typical_value}};
      
      compare_it(
         [&](){
            unsafe.write(typical_value);
         },
         [&](){
            tested.write(typical_value);
         },
         warm_write_amount
      );
   }

   // === TEST FINALIZATION ===

   std::cout << std::endl;
   return 0;
   
}
