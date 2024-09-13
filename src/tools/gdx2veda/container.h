#ifndef GDX_CONTAINER_H
#define GDX_CONTAINER_H

#include <vector>

namespace library
{

template<typename T>
class one_indexed_container
{
private:
   std::vector<T> data;

public:
   one_indexed_container();
   one_indexed_container( std::size_t size );
   one_indexed_container( const std::vector<T> &data );

   T &at( std::size_t index );
   T &operator[]( std::size_t index );

   // const T &at( std::size_t index ) const;
   // const T &operator[]( std::size_t index ) const;

   std::size_t size() const;
   void resize( std::size_t size );

   void push_back( const T &element );
   void emplace_back( const T &element );
};

}// namespace library

#endif// GDX_CONTAINER_H
