#ifndef COW_OWNERSHIP_FLAG_H
#define COW_OWNERSHIP_FLAG_H

#include <mutex>

// Writing to copy-on-write data relies on an underlying notion of data ownership:
//    - If the active pointer has ownership of the data it points to, perform the write
//    - If it does not have ownership, create a new data block which we own, and write there
//
// Data ownership gets a bit complicated in a multithreaded world though, as we need to avoid
// multiple writers trying to create the data block all at once. There needs to be...
//    A/ Some kind of flag telling whether we have ownership of the data block, as before
//    B/ For situations where we do not have ownership, a way to atomically...
//         - Test the flag, continue if it is false and block if it is true
//         - Set the flag to true
//         - Acquire data block ownership while other clients are waiting
//         - Release other clients to the cow_ptr
//
// I begun implementing these semantics using std::once_flag and std::call_once. However, the
// non-readable, non-writable, non-copyable and non-moveable semantics of std::once_flag are
// too limited for my needs.
//
// Hence I need to devise my own synchronization primitive for this purpose.
//
class cow_ownership_flag {
   public:
   
      // Ownership flags may be initialized to a certain value without synchronization, as at construction time only
      // one thread has access to the active ownership flag.
      cow_ownership_flag(bool initially_owned) : m_owned{initially_owned} { }
      
      // When we move-construct from an ownership flag rvalue, we may assume that no other thread has access to either
      // that rvalue or the active flag, and avoid using synchronization.
      cow_ownership_flag(cow_ownership_flag && other) : m_owned{other.m_owned} { }
      
      // There's nothing special about deleting an ownership flag.
      ~cow_ownership_flag() = default;
      
      // When we move-assign an ownership flag rvalue, no other thread has access to that rvalue, so we can access it
      // without synchronizing. But the active flag may be shared with other threads, so we need write synchronization.
      cow_ownership_flag & operator=(cow_ownership_flag && other) {
         set_ownership(other.m_owned);
      }
      
      // Ownership flags are not copyable. Proper CoW semantics would require clearing them upon copy,
      // which is at odds with normal copy semantics. It's better to throw a compiler error in this case,
      // and let the user write more explicit code.
      cow_ownership_flag(const cow_ownership_flag &) = delete;
      cow_ownership_flag & operator=(const cow_ownership_flag &) = delete;
      
      // Authoritatively mark the active memory block as owned/not owned by the active thread
      void set_ownership(bool owned) {
         std::lock_guard<std::mutex> lock(m_ownership_mutex);
         m_owned = owned;
      }
   
      // Acquire ownership of the active memory block once, using the provided resource acquisition routine,
      // if that's not done already. Have other threads block during this process.
      void acquire_ownership_once(std::function<void()> acquire) {
         std::lock_guard<std::mutex> lock(m_ownership_mutex);
         if(!m_owned) {
            acquire();
            m_owned = true;
         }
      }
      
   private:
   
      std::mutex m_ownership_mutex;
      bool m_owned;
};

#endif
