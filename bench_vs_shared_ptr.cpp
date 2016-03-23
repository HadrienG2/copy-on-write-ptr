#include <iostream>
#include <memory>

#include "copy_on_write_ptr.hpp"
#include "shared.hpp"

// === FORWARD DECLARATIONS ===

// Import shared definitions
using namespace Shared;

// Specialization of time_it for my comparison purposes
template<typename Callable1,
         typename Callable2>
void compare_it(Callable1 && shptr_operation,
                Callable2 && cowptr_operation,
                const std::size_t amount) {
   const auto shptr_duration = Shared::time_it(shptr_operation, amount);
   std::cout << "With a raw shared_ptr, this operation takes " << shptr_duration.count() << " s" << std::endl;
   
   const auto cowptr_duration = Shared::time_it(cowptr_operation, amount);
   std::cout << "With cow_ptr, it takes " << cowptr_duration.count() << " s ("
             << cowptr_duration.count() / shptr_duration.count() << "x slower)" << std::endl;
}

// === PERFORMANCE TEST BODY ===

int main() {

   // === PART 0 : TEST-WIDE DEFINITIONS ===

   // Define our smart pointer types
   using SharedPointer = std::shared_ptr<Data>;
   using COWPointer = copy_on_write_ptr<Data, cow_ownership_flags::thread_unsafe_flag>;
   
   // Say hi :)
   std::cout << std::endl << "=== Microbenchmarking cow_ptr ===" << std::endl;

   // === PART 1 : CREATION FROM RAW POINTER ===
   
   const size_t creation_amount = 1000 * 1000 * 100;
   std::cout << std::endl << "Creating " << creation_amount << " pointers from raw pointers" << std::endl;
   {
      compare_it(
         [&](){
            SharedPointer ptr{new Data{typical_value}};
         },
         [&](){
            COWPointer ptr{new Data{typical_value}};
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
   
   const size_t copy_amount = 1000 * 1000 * 1000;
   std::cout << std::endl << "Copy-constructing " << copy_amount << " pointers" << std::endl;
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
   
   // === PART 4 : COPY CONSTRUCTION + MOVE-ASSIGNMENT ===  (NOTE: Cannot test move assignment alone easily)
   
   const size_t copy_move_amount = 5 * copy_amount;
   std::cout << std::endl << "Copy-constructing AND move-assigning " << copy_move_amount << " pointers" << std::endl;
   {
      const SharedPointer source_shptr{std::make_shared<Data>(typical_value)};
      const COWPointer source_cowptr{new Data{typical_value}};
      
      SharedPointer dest_shptr{source_shptr};
      COWPointer dest_cowptr{source_cowptr};
      
      compare_it(
         [&](){
            SharedPointer copy{source_shptr};
            dest_shptr = std::move(copy);
         },
         [&](){
            COWPointer copy{source_cowptr};
            dest_cowptr = std::move(copy);
         },
         copy_move_amount
      );
   }
   
   // === PART 5 : COPY ASSIGNMENT ===
   
   const size_t copy_assign_amount = 1000 * 1000 * 64;
   std::cout << std::endl << "Copy-assigning " << copy_assign_amount << " pointers" << std::endl;
   {
      const SharedPointer source_shptr{std::make_shared<Data>(typical_value)};
      const COWPointer source_cowptr{new Data{typical_value}};
      
      SharedPointer dest_shptr{source_shptr};
      COWPointer dest_cowptr{source_cowptr};
      
      compare_it(
         [&](){
            dest_shptr = source_shptr;
         },
         [&](){
            dest_cowptr = source_cowptr;
         },
         copy_assign_amount
      );
   }
   
   // === PART 6 : READ DATA ===
   
   const size_t read_amount = 1000 * 1000 * 1024;
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
   
   // === PART 7 : COPY ASSIGNMENT + COLD WRITES ===  (NOTE: A pure cold write would require breaking encapsulation)
   
   const size_t cold_write_amount = 30 * copy_assign_amount;
   std::cout << std::endl << "Performing " << cold_write_amount << " pointer copies AND cold writes" << std::endl;
   {
      const SharedPointer source_shptr{std::make_shared<Data>(typical_value)};
      const COWPointer source_cowptr{new Data{typical_value}};
      
      SharedPointer dest_shptr{source_shptr};
      COWPointer dest_cowptr{source_cowptr};
      
      compare_it(
         [&](){
            dest_shptr = source_shptr;
            *dest_shptr = typical_value;
         },
         [&](){
            dest_cowptr = source_cowptr;
            dest_cowptr.write(typical_value);
         },
         cold_write_amount
      );
   }
   
   // === PART 8 : WARM WRITES ===

   const size_t warm_write_amount = cold_write_amount;
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
