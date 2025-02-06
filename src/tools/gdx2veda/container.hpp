#ifndef GDX_CONTAINER_H
#define GDX_CONTAINER_H

#include <vector>
#include <utility>
#include <stdexcept>

namespace library
{

template<typename T>
class Container_t
{
   std::vector<T> data;

public:
   Container_t();
   Container_t( std::size_t size );
   Container_t( const std::vector<T> &data );

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
Container_t<T>::Container_t()
    : data()
{}

template<typename T>
Container_t<T>::Container_t( const std::size_t size )
    : data( size )
{}

template<typename T>
Container_t<T>::Container_t( const std::vector<T> &data )
    : data( std::move( data ) )
{}

template<typename T>
T &Container_t<T>::at( const std::size_t index )
{
   if( index < 1 || index > data.size() )
      throw std::out_of_range( "Index out of range" );
   return data.at( index - 1 );
}

template<typename T>
T &Container_t<T>::operator[]( const std::size_t index )
{
   return at( index );
}

template<typename T>
const T &Container_t<T>::at( const std::size_t index ) const
{
   if( index < 1 || index > data.size() )
      throw std::out_of_range( "Index out of range" );
   return data.at( index - 1 );
}

template<typename T>
const T &Container_t<T>::operator[]( const std::size_t index ) const
{
   return at( index );
}

template<typename T>
std::size_t Container_t<T>::size() const
{
   return data.size();
}

template<typename T>
void Container_t<T>::resize( const std::size_t size )
{
   data.resize( size );
}

template<typename T>
void Container_t<T>::push_back( const T &element )
{
   data.push_back( element );
}

template<typename T>
void Container_t<T>::emplace_back( const T &element )
{
   data.emplace_back( element );
}

}// namespace library

#endif// GDX_CONTAINER_H
