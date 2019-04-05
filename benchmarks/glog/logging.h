#pragma once

#include <vector>

#define GOOGLE_STRIP_LOG 2

struct NullStream {};

constexpr NullStream nullStream;

template<class T>
const NullStream& operator<<(const NullStream &ns, const T& /*unused*/) {
    return ns;
}

template<class T>
const NullStream& operator<<(const NullStream &ns, T /*unused*/[]) {
    return ns;
}

template<class T>
const NullStream& operator<<(const NullStream &ns, const T* /*unused*/) {
    return ns;
}

#define LOG(level) nullStream

