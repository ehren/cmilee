#ifndef CETO_MAKE_SHARED_HPP_INCLUDED
#define CETO_MAKE_SHARED_HPP_INCLUDED

//  make_local_shared_object.hpp
//
//  Copyright (c) 2007, 2008, 2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/smart_ptr/make_shared.html
//  for documentation.

#include <kit_local_shared_ptr/detail/sp_forward.hpp>
#include <kit_local_shared_ptr/local_shared_ptr.hpp>
#include <cstddef>
#include <new>

namespace ceto {

  namespace detail {

    template <class T>
    class sp_ms_deleter {
    private:
      typedef
          typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type storage_type;

      bool initialized_;
      storage_type storage_;

    private:
      void destroy() {
        if (initialized_) {
          // fixes incorrect aliasing warning
          T *p = reinterpret_cast<T *>(&storage_);
          p->~T();

          initialized_ = false;
        }
      }

    public:
      sp_ms_deleter() noexcept : initialized_(false) {
      }

      template <class A>
      explicit sp_ms_deleter(A const &) noexcept : initialized_(false) {
      }

      // optimization: do not copy storage_
      sp_ms_deleter(sp_ms_deleter const &) noexcept : initialized_(false) {
      }

      ~sp_ms_deleter() {
        destroy();
      }

      void operator()(T *) {
        destroy();
      }

      static void operator_fn(T *) // operator() can't be static
      {
      }

      void *address() noexcept {
        return &storage_;
      }

      void set_initialized() noexcept {
        initialized_ = true;
      }
    };

    template <class T, class A>
    class sp_as_deleter {
    private:
      typedef
          typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type storage_type;

      storage_type storage_;
      A a_;
      bool initialized_;

    private:
      void destroy() {
        if (initialized_) {
          T *p = reinterpret_cast<T *>(&storage_);

          std::allocator_traits<A>::destroy(a_, p);

          initialized_ = false;
        }
      }

    public:
      sp_as_deleter(A const &a) noexcept : a_(a), initialized_(false) {
      }

      // optimization: do not copy storage_
      sp_as_deleter(sp_as_deleter const &r) noexcept : a_(r.a_), initialized_(false) {
      }

      ~sp_as_deleter() {
        destroy();
      }

      void operator()(T *) {
        destroy();
      }

      static void operator_fn(T *) // operator() can't be static
      {
      }

      void *address() noexcept {
        return &storage_;
      }

      void set_initialized() noexcept {
        initialized_ = true;
      }
    };

    template <class T>
    struct sp_if_not_array {
      typedef ceto::local_shared_ptr<T> type;
    };

    template <class T>
    struct sp_if_not_array<T[]> {};

    template <class T, std::size_t N>
    struct sp_if_not_array<T[N]> {};

  } // namespace detail

#define CETO_SP_MSD(T) ceto::detail::sp_inplace_tag<ceto::detail::sp_ms_deleter<T>>()

  // _noinit versions

  template <class T>
  typename ceto::detail::sp_if_not_array<T>::type make_local_shared_noinit() {
    ceto::local_shared_ptr<T> pt(static_cast<T *>(0), CETO_SP_MSD(T));

    ceto::detail::sp_ms_deleter<T> *pd
        = static_cast<ceto::detail::sp_ms_deleter<T> *>(pt._internal_get_untyped_deleter());

    void *pv = pd->address();

    ::new (pv) T;
    pd->set_initialized();

    T *pt2 = static_cast<T *>(pv);

    ceto::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
    return ceto::local_shared_ptr<T>(pt, pt2);
  }

  template <class T, class A>
  typename ceto::detail::sp_if_not_array<T>::type allocate_shared_noinit(A const &a) {
    ceto::local_shared_ptr<T> pt(static_cast<T *>(0), CETO_SP_MSD(T), a);

    ceto::detail::sp_ms_deleter<T> *pd
        = static_cast<ceto::detail::sp_ms_deleter<T> *>(pt._internal_get_untyped_deleter());

    void *pv = pd->address();

    ::new (pv) T;
    pd->set_initialized();

    T *pt2 = static_cast<T *>(pv);

    ceto::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
    return ceto::local_shared_ptr<T>(pt, pt2);
  }

  // Variadic templates, rvalue reference

  template <class T, class... Args>
  typename ceto::detail::sp_if_not_array<T>::type make_local_shared(Args &&... args) {
    ceto::local_shared_ptr<T> pt(static_cast<T *>(0), CETO_SP_MSD(T));

    ceto::detail::sp_ms_deleter<T> *pd
        = static_cast<ceto::detail::sp_ms_deleter<T> *>(pt._internal_get_untyped_deleter());

    void *pv = pd->address();

    ::new (pv) T(ceto::detail::sp_forward<Args>(args)...);
    pd->set_initialized();

    T *pt2 = static_cast<T *>(pv);

    ceto::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
    return ceto::local_shared_ptr<T>(pt, pt2);
  }

  template <class T, class A, class... Args>
  typename ceto::detail::sp_if_not_array<T>::type allocate_shared(A const &a, Args &&... args) {
    typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;
    A2 a2(a);

    typedef ceto::detail::sp_as_deleter<T, A2> D;

    ceto::local_shared_ptr<T> pt(static_cast<T *>(0), ceto::detail::sp_inplace_tag<D>(), a2);

    D *pd = static_cast<D *>(pt._internal_get_untyped_deleter());
    void *pv = pd->address();

    std::allocator_traits<A2>::construct(a2, static_cast<T *>(pv),
                                         ceto::detail::sp_forward<Args>(args)...);

    pd->set_initialized();

    T *pt2 = static_cast<T *>(pv);

    ceto::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
    return ceto::local_shared_ptr<T>(pt, pt2);
  }

#undef CETO_SP_MSD

} // namespace ceto

#endif // #ifndef CETO_MAKE_SHARED_HPP_INCLUDED
