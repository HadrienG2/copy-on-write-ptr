#ifndef COW_PTR_H
#define COW_PTR_H

#include <memory>
#include <mutex>

// The cow_ptr class implements copy-on-write semantics on top of std::shared_ptr
template<typename T>
class cow_ptr {
   public:
      // === BASIC CLASS LIFECYCLE ===
   
      // Construct a cow_ptr from a raw pointer, acquire ownership
      cow_ptr(T * ptr) :
         m_payload{ptr},
         m_ownership_flag{}
      {
         acquire_ownership();
      }
      
      // Move-construct from a cow_ptr, acquire ownership
      cow_ptr(cow_ptr<T> && cptr) :
         m_payload{cptr.m_payload},
         m_ownership_flag{}
      {
         acquire_ownership();
      }
      
      // Copy-construct from a cow_ptr, DO NOT acquire ownership
      cow_ptr(const cow_ptr<T> & cptr) :
         m_payload{cptr.m_payload},
         m_ownership_flag{}
      { }
      
      // All our data members can take care of themselves on their own
      ~cow_ptr() = default;
      
      // Copy and move assignment must be deleted for now, because std::once_flag's non-moveability make it difficult
      cow_ptr & operator=(const cow_ptr & cptr) = delete;
      cow_ptr & operator=(cow_ptr && cptr) = delete;
      
      // === DATA ACCESS ===
      
      // Reading from a cow_ptr does not require ownership
      const T & read() const { return *m_payload; }
      
      // Writing to a cow_ptr requires ownership, which must be acquired as needed
      void write(const T & value) {
         copy_if_not_owner();
         *m_payload = value;
      }
      
      void write(T && value) {
         copy_if_not_owner();
         *m_payload = value;
      }

   private:
      std::shared_ptr<T> m_payload;
      std::once_flag m_ownership_flag;
      
      // Mark ourserlves as the owner of the payload object
      void acquire_ownership() {
         std::call_once(m_ownership_flag, [](){});
      }
      
      // If we are not the owner of the payload object, make a private copy of it
      void copy_if_not_owner() {
         std::call_once(m_ownership_flag, [this](){
            m_payload = std::make_shared<T>(*m_payload);
         });
      }
};

#endif
