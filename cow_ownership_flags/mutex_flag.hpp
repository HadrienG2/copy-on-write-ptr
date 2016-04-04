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

#ifndef MUTEX_FLAG_H
#define MUTEX_FLAG_H

#include <mutex>

namespace cow_ownership_flags {

   // This implementation of the copy-on-write ownership flag uses a mutex to achieve thread safety.
   class mutex_flag {
      public:
      
         // Ownership flags may be initialized to a certain value without synchronization, as at
         // construction time only one thread has access to the active ownership flag.
         mutex_flag(bool initially_owned) : m_owned{initially_owned} { }
         
         // When we move-construct from an ownership flag rvalue, we may assume that no other thread
         // has access to either that rvalue or the active flag, and avoid using synchronization.
         mutex_flag(mutex_flag && other) : m_owned{other.m_owned} { }
         
         // There's nothing special about deleting an ownership flag.
         ~mutex_flag() = default;
         
         // When we move-assign an ownership flag rvalue, no other thread has access to that rvalue,
         // so we can access it without read synchronization.
         // But the active flag may be shared with other threads, so we need write synchronization.
         mutex_flag & operator=(mutex_flag && other) {
            set_ownership(other.m_owned);
         }
         
         // Ownership flags are not copyable. Proper CoW semantics would require clearing them upon
         // copy, which is at odds with normal copy semantics. It's better to throw a compiler error
         // in this case, and let the user write more explicit code.
         mutex_flag(const mutex_flag &) = delete;
         mutex_flag & operator=(const mutex_flag &) = delete;
         
         // Authoritatively mark the active memory block as owned/not owned by the active thread
         void set_ownership(bool owned) {
            std::lock_guard<std::mutex> lock(m_ownership_mutex);
            m_owned = owned;
         }
      
         // Acquire ownership of the active memory block, using the provided resource acquisition
         // routine, if that's not done already. Other threads should block during this process.
         template<typename Callable>
         void acquire_ownership_once(Callable && acquire) {
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
   
}

#endif
