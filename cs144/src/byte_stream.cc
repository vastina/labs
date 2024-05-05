#include "byte_stream.hh"
#include <cstdint>
#include <fcntl.h>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), primary_capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if ( capacity_ == 0 || data.empty() )
    return;
  uint64_t size = min( data.size(), capacity_ );
  if ( size == capacity_ )
    data = data.substr( 0, size );
  capacity_ -= size;
  data_queue_.push_back( std::move( data ) );
  // view_queue_.emplace_back( data_queue_.back().c_str(), size);
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  return ( primary_capacity_ - capacity_ ) + poped;
}

bool Reader::is_finished() const
{
  return closed_ && data_queue_.empty();
}

uint64_t Reader::bytes_popped() const
{
  return poped;
}

string_view Reader::peek() const
{
  if ( data_queue_.empty() ) {
    return {};
  }
  return static_cast<std::string_view>( data_queue_.front() );
  // return view_queue_.front();
  // return static_cast<std::string_view>(data_);
}

void Reader::pop( uint64_t len )
{
  capacity_ += len;
  poped += len;
  // data_ = data_.substr( len );
  while ( len > 0 && !data_queue_.empty() ) {
    auto& front = data_queue_.front();
    if ( front.size() <= len ) {
      len -= front.size();
      data_queue_.pop_front();
      // view_queue_.pop_front();
    } else {
      front = front.substr( len );
      // view_queue_.pop_front();
      // view_queue_.push_front(front);
      len = 0;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  return primary_capacity_ - capacity_;
}
