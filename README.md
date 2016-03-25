# The `copy_on_write_ptr` project

The idea behind `copy_on_write_ptr` is to provide users with a relatively straightforward way to use `std::shared_ptr`
with copy-on-write (CoW) semantics.

In CoW semantics, large pieces of data may be cheaply copied by reference as long as it is not written to, whereas
writing triggers a lazy deep copy of the underlying data block. Effectively, copy-on-write allows a client to have
something which offers the memory efficiency benefits of an `std::shared_ptr<const T>`, but can gracefully degrade into
a mutable `std::unique_ptr<T>` to a private data block as needed.

There is a widespread belief in the C++ community that the innovations brought forth by C++11, in particular move
semantics, have rendered copy-on-write obsolete. This belief has led, for example, a similar effort to be rejected by
the Boost community. However, this is not entirely accurate. C++11 has not rendered copy-on-write obsolete, it has
simply proposed a better solution to a *subset* of the problems which required CoW usage in the past.

Copy-on-write semantics remain appropriate in scenarios where...

   - Multiple threads need access to a large piece of data as if they owned a private copy of it.
   - It is not known in advance whether threads will need to mutate their "cheap copy" of the data.
   - The probability of data mutation is low enough for the memory efficiency gains to offset the CPU efficiency losses.


# Copy-on-write in a single-threaded world

Writing to copy-on-write data relies on an underlying notion of data ownership:

   - If the active pointer has ownership of the data block it points to, it can perform the write directly
   - If it does not have ownership, it must create a new data block (which it will own) and write there

In a single-threaded world, this may be implemented simply using a boolean dirty flag which tells whether a
`copy_on_write_ptr` owns the data it points to. A lazy copy will occur when a write is attempted as this flag is
`false`, setting, the flag to `true` along the way.


# Copy-on-write in a multithreaded world

If thread safety is desired, then copy-on-write gets more complicated, because we need to handle two data races:

   1. Two threads attempt to write new values into CoW data at the same time, potentially causing multiple lazy copies.
   2. A thread attempts to assign a new value to the pointer while another is performing a write to the contained data.

Another data race that cannot be avoided (in a library-based copy-on-write implementation) is that writing to CoW data
potentially invalidates the address of that data. Therefore, client threads should be very careful when holding
long-lived references to CoW data that may be asynchronously written to. This is a general concern when using container
libraries, though.

It should be noted that both of the data races above generally indicate an error within the client code, thus opting not
to handle them would be a reasonable option. But if they are to be handled well, thread synchronization must be used.


# Exploring the design tradeoff

To explore the design space for copy-on-write implementations, I decided to decouple the data ownership handling
mechanism from the high-level CoW interface that is provided by `copy_on_write_ptr`. In this repository, you will find
multiple implementations of this mechanism:

- A thread-unsafe implementation using a boolean flag
- An implementation using mutex synchronization to prevent concurrent ownership flag assignment and lazy copies
- An implementation using atomics-based synchronization instead of mutexes, at a cost of some design complexity
- An implementation using explicit memory ordering to try to accelerate atomics, at the cost of further complexity

I initially tried to use `std::once_flag` as a copy-on-write ownership flag implementation, however its non-readable,
non-writable, non-moveable and non-copyable semantics turned out to be too limiting for my needs.

These implementations may easily be compared from a performance and design complexity point of view: the thread-unsafe
implementation can serve as a baseline for the best performance that one may expect from copy-on-write semantics, under
disciplined single-threaded use, whereas the synchronized implementations represent different points on the design
continuum between maximal performance and minimal design complexity.
