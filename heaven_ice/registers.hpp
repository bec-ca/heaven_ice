#include "size_kind.hpp"
#include "types.hpp"

namespace heaven_ice {

template <class R> struct BaseRegister {
 public:
  inline sbyte_t b() const { return _value; }
  inline sword_t w() const { return _value; }
  inline slong_t l() const { return _value; }

  inline void b(sbyte_t v) { _value = R::ub(_value, v); }
  inline void w(sword_t v) { _value = R::uw(_value, v); }
  inline void l(slong_t v) { _value = R::ul(_value, v); }

  template <class T> inline T get() const;
  template <> inline sbyte_t get<sbyte_t>() const { return b(); }
  template <> inline sword_t get<sword_t>() const { return w(); }
  template <> inline slong_t get<slong_t>() const { return l(); }

  template <class T> inline void set(T v);
  template <> inline void set<sbyte_t>(sbyte_t v) { b(v); }
  template <> inline void set<sword_t>(sword_t v) { w(v); }
  template <> inline void set<slong_t>(slong_t v) { l(v); }

  template <class T> inline void inc(T v) { set<T>(get<T>() + v); }
  template <class T> inline void dec(T v) { set<T>(get<T>() - v); }

  inline slong_t get_s(SizeKind size)
  {
    switch (size) {
    case SizeKind::Byte:
      return b();
    case SizeKind::Word:
      return w();
    case SizeKind::Long:
      return l();
    }
  }

  inline void set_s(SizeKind size, slong_t v)
  {
    switch (size) {
    case SizeKind::Byte:
      b(v);
      break;
    case SizeKind::Word:
      w(v);
      break;
    case SizeKind::Long:
      l(v);
      break;
    }
  }

 private:
  slong_t _value = 0;
};

struct DataRegisterSpec {
  static inline ulong_t ub(ulong_t r, ubyte_t v) { return (r & ~0xff) | v; }
  static inline ulong_t uw(ulong_t r, uword_t v) { return (r & ~0xffff) | v; }
  static inline ulong_t ul(ulong_t, ulong_t v) { return v; }
};

using DataRegister = BaseRegister<DataRegisterSpec>;

struct AddrRegister : public BaseRegister<AddrRegister> {
  static inline slong_t ub(slong_t, sbyte_t v) { return v; }
  static inline slong_t uw(slong_t, sword_t v) { return v; }
  static inline slong_t ul(slong_t, slong_t v) { return v; }

  inline AddrRegister& operator=(slong_t v)
  {
    l(v);
    return *this;
  }

  inline AddrRegister& operator+=(slong_t v)
  {
    inc(v);
    return *this;
  }

  inline AddrRegister& operator-=(slong_t v)
  {
    dec(v);
    return *this;
  }

  inline operator slong_t() const { return l(); }
};

} // namespace heaven_ice
