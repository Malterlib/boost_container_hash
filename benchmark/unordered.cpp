// Copyright 2022 Peter Dimov.
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#define _CRT_SECURE_NO_WARNINGS

#include <boost/container_hash/hash.hpp>
#include <boost/unordered_set.hpp>
#include <boost/core/detail/splitmix64.hpp>
#include <boost/core/type_name.hpp>
#include <boost/config.hpp>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

// mul31_hash

class mul31_hash
{
public:

    std::size_t operator()( std::string const& st ) const BOOST_NOEXCEPT
    {
        char const * p = st.data();
        std::size_t n = st.size();

#if SIZE_MAX > UINT32_MAX
        std::size_t h = 0xCBF29CE484222325ull;
#else
        std::size_t h = 0x811C9DC5u;
#endif

        for( std::size_t i = 0; i < n; ++i )
        {
            h = h * 31 + static_cast<unsigned char>( p[i] );
        }

        return h;
    }
};

// mul31_unrolled_hash

template<int Bits> struct mul31_unrolled_hash_impl;

template<> struct mul31_unrolled_hash_impl<32>
{
    std::size_t operator()( std::string const& st ) const BOOST_NOEXCEPT
    {
        char const * p = st.data();
        std::size_t n = st.size();

        std::size_t h = 0x811C9DC5u;

        while( n >= 4 )
        {
            h = h * (31u * 31u * 31u * 31u)
                + static_cast<unsigned char>( p[0] ) * (31u * 31u * 31u)
                + static_cast<unsigned char>( p[1] ) * (31u * 31u)
                + static_cast<unsigned char>( p[2] ) * 31u
                + static_cast<unsigned char>( p[3] );

            p += 4;
            n -= 4;
        }

        while( n > 0 )
        {
            h = h * 31u + static_cast<unsigned char>( *p );

            ++p;
            --n;
        }

        return h;
    }
};

template<> struct mul31_unrolled_hash_impl<64>
{
    std::size_t operator()( std::string const& st ) const BOOST_NOEXCEPT
    {
        char const * p = st.data();
        std::size_t n = st.size();

        std::size_t h = 0xCBF29CE484222325ull;

        while( n >= 8 )
        {
            h = h * (31ull * 31ull * 31ull * 31ull * 31ull * 31ull * 31ull * 31ull)
                + static_cast<unsigned char>( p[0] ) * (31ull * 31ull * 31ull * 31ull * 31ull * 31ull * 31ull)
                + static_cast<unsigned char>( p[1] ) * (31ull * 31ull * 31ull * 31ull * 31ull * 31ull)
                + static_cast<unsigned char>( p[2] ) * (31ull * 31ull * 31ull * 31ull * 31ull)
                + static_cast<unsigned char>( p[3] ) * (31ull * 31ull * 31ull * 31ull)
                + static_cast<unsigned char>( p[4] ) * (31ull * 31ull * 31ull)
                + static_cast<unsigned char>( p[5] ) * (31ull * 31ull)
                + static_cast<unsigned char>( p[6] ) * 31ull
                + static_cast<unsigned char>( p[7] );

            p += 8;
            n -= 8;
        }

        while( n > 0 )
        {
            h = h * 31u + static_cast<unsigned char>( *p );

            ++p;
            --n;
        }

        return h;
    }
};

struct mul31_unrolled_hash: mul31_unrolled_hash_impl< std::numeric_limits<std::size_t>::digits > {};

// fnv1a_hash

template<int Bits> struct fnv1a_hash_impl;

template<> struct fnv1a_hash_impl<32>
{
    std::size_t operator()( std::string const& s ) const
    {
        std::size_t h = 0x811C9DC5u;

        char const * first = s.data();
        char const * last = first + s.size();

        for( ; first != last; ++first )
        {
            h ^= static_cast<unsigned char>( *first );
            h *= 0x01000193ul;
        }

        return h;
    }
};

template<> struct fnv1a_hash_impl<64>
{
    std::size_t operator()( std::string const& s ) const
    {
        std::size_t h = 0xCBF29CE484222325ull;

        char const * first = s.data();
        char const * last = first + s.size();

        for( ; first != last; ++first )
        {
            h ^= static_cast<unsigned char>( *first );
            h *= 0x00000100000001B3ull;
        }

        return h;
    }
};

struct fnv1a_hash: fnv1a_hash_impl< std::numeric_limits<std::size_t>::digits > {};

//

template<class V, class S> void test4( int N, V const& v, char const * hash, S s )
{
    typedef std::chrono::steady_clock clock_type;

    clock_type::time_point t1 = clock_type::now();

    for( int i = 0; i < N; ++i )
    {
        s.insert( v[ i * 16 ] );
    }

    clock_type::time_point t2 = clock_type::now();

    std::size_t q = 0;

    for( int i = 0; i < 16 * N; ++i )
    {
        q += s.count( v[ i ] );
    }

    clock_type::time_point t3 = clock_type::now();

    long long ms1 = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    long long ms2 = std::chrono::duration_cast<std::chrono::milliseconds>( t3 - t2 ).count();

    std::size_t n = s.bucket_count();
    std::size_t m = 0;

    for( std::size_t i = 0; i < n; ++i )
    {
        std::size_t k = s.bucket_size( i );

        if( k > m )
        {
            m = k;
        }
    }

#if defined( _MSC_VER )

    std::printf( "%25s : n=%Iu, m=%Iu, q=%Iu, %lld + %lld ms\n", hash, n, m, q, ms1, ms2 );

#else

    std::printf( "%25s : n=%zu, m=%zu, q=%zu, %lld + %lld ms\n", hash, n, m, q, ms1, ms2 );

#endif
}

template<class K, class H, class V> void test3( int N, V const& v )
{
    boost::unordered_set<K, H> s( 0 );
    test4( N, v, boost::core::type_name<H>().c_str(), s );
}

template<class H, class V> void test2( int N, V const& v )
{
    typedef std::chrono::steady_clock clock_type;

    clock_type::time_point t1 = clock_type::now();

    std::size_t q = 0;

    H const h;

    for( int i = 0; i < N; ++i )
    {
        q += h( v[i] );
    }

    clock_type::time_point t2 = clock_type::now();

    long long ms1 = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();

    std::string hash = boost::core::type_name<H>();

#if defined( _MSC_VER )

    std::printf( "%25s : q=%20Iu, %lld ms\n", hash.c_str(), q, ms1 );

#else

    std::printf( "%25s : q=%20zu, %lld ms\n", hash.c_str(), q, ms1 );

#endif
}

int main()
{
    int const N = 1048576;

    std::vector<std::string> v;

    {
        v.reserve( N * 16 );

        boost::detail::splitmix64 rnd;

        for( int i = 0; i < 16 * N; ++i )
        {
            char buffer[ 64 ];

            unsigned long long k = rnd();

            if( k & 1 )
            {
                sprintf( buffer, "prefix_%llu_suffix", k );
            }
            else
            {
                sprintf( buffer, "{%u}", static_cast<unsigned>( k ) );
            }

            v.push_back( buffer );
        }
    }

    typedef std::string K;

    std::puts( "Hash speed test:\n" );

    test2<mul31_hash>( N * 16, v );
    test2<mul31_unrolled_hash>( N * 16, v );
    test2<fnv1a_hash>( N * 16, v );
    test2<boost::hash<std::string> >( N * 16, v );
    test2<std::hash<std::string> >( N * 16, v );

    std::puts( "" );

    std::puts( "Container speed test:\n" );

    test3<K, mul31_hash>( N, v );
    test3<K, mul31_unrolled_hash>( N, v );
    test3<K, fnv1a_hash>( N, v );
    test3<K, boost::hash<std::string> >( N, v );
    test3<K, std::hash<std::string> >( N, v );

    std::puts( "" );
}
