#include <utility>
#include <stdexcept>

#include "container.h"

namespace library
{

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

// template<typename T>
// const T &one_indexed_container<T>::at( const std::size_t index ) const
// {
//    return at( index );
// }

// template<typename T>
// const T &one_indexed_container<T>::operator[]( const std::size_t index ) const
// {
//    return at( index );
// }

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
