#ifndef GDX_CONTAINER_H
#define GDX_CONTAINER_H

#include <vector>
#include <utility>
#include <stdexcept>

namespace library
{

template<typename T>
class one_indexed_container
{
   std::vector<T> data;

public:
   one_indexed_container();
   one_indexed_container( std::size_t size );
   one_indexed_container( const std::vector<T> &data );

   T &at( std::size_t index );
   T &operator[]( std::size_t index );

   const T &at( std::size_t index ) const;
   const T &operator[]( std::size_t index ) const;

   std::size_t size() const;
   void resize( std::size_t size );

   void push_back( const T &element );
   void emplace_back( const T &element );
};

template<typename T>
one_indexed_container<T>::one_indexed_container()
    : data()
{}

template<typename T>
one_indexed_container<T>::one_indexed_container( const std::size_t size )
    : data( size )
{}

template<typename T>
one_indexed_container<T>::one_indexed_container( const std::vector<T> &data )
    : data( std::move( data ) )
{}

template<typename T>
T &one_indexed_container<T>::at( const std::size_t index )
{
   if( index < 1 || index > data.size() )
      throw std::out_of_range( "Index out of range" );
   return data.at( index - 1 );
}

template<typename T>
T &one_indexed_container<T>::operator[]( const std::size_t index )
{
   return at( index );
}

template<typename T>
const T &one_indexed_container<T>::at( const std::size_t index ) const
{
   if( index < 1 || index > data.size() )
      throw std::out_of_range( "Index out of range" );
   return data.at( index - 1 );
}

template<typename T>
const T &one_indexed_container<T>::operator[]( const std::size_t index ) const
{
   return at( index );
}

template<typename T>
std::size_t one_indexed_container<T>::size() const
{
   return data.size();
}

template<typename T>
void one_indexed_container<T>::resize( const std::size_t size )
{
   data.resize( size );
}

template<typename T>
void one_indexed_container<T>::push_back( const T &element )
{
   data.push_back( element );
}

template<typename T>
void one_indexed_container<T>::emplace_back( const T &element )
{
   data.emplace_back( element );
}

}// namespace library

#endif// GDX_CONTAINER_H
