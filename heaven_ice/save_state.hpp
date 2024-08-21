#pragma once

#include "bee/reader.hpp"
#include "bee/writer.hpp"

namespace heaven_ice {

template <class T> void save_state_gen(const T& v, bee::Writer& writer)
{
  must_unit(writer.write(reinterpret_cast<const std::byte*>(&v), sizeof(v)));
}

template <class T> void load_state_gen(T& v, bee::Reader& reader)
{
  must_unit(reader.read(reinterpret_cast<std::byte*>(&v), sizeof(v)));
}

template <class T, size_t S>
void save_state_array(const std::array<T, S>& ar, bee::Writer& writer)
{
  must_unit(writer.write(
    reinterpret_cast<const std::byte*>(ar.data()), ar.size() * sizeof(T)));
}

template <class T, size_t S>
void load_state_array(std::array<T, S>& ar, bee::Reader& reader)
{
  must_unit(reader.read(
    reinterpret_cast<std::byte*>(ar.data()), ar.size() * sizeof(T)));
}

} // namespace heaven_ice
