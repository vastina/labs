#include "reassembler.hh"
#include <cstdint>
#include <deque>
#include <netinet/in.h>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto max_endindex = writer().bytes_pushed() + writer().available_capacity();
  auto endpoint = first_index + data.size();
  if ( is_last_substring ) {
    end_index = endpoint; // std::max(end_index ,endpoint);
    last_ = true;
    if ( end_index == writen_size_ )
      output_.writer().close();
  }
  if ( endpoint > max_endindex )
    data.resize( std::max( (int64_t)0, (int64_t)max_endindex - (int64_t)first_index ) );
  // close part

  if ( endpoint <= writer().bytes_pushed() || // already writen
       first_index >= max_endindex )          // max_endindex
    return;
  if ( data.empty() )
    return;
  // return part

  uint64_t i = 0;
  for ( i = 0; i < index_queue_.size(); i++ ) {
    if ( index_queue_.at( i ) > first_index )
      break;
  }
  index_queue_.insert( index_queue_.begin() + i, first_index );
  buffer_queue_.insert( buffer_queue_.begin() + i, data );
  // insert part

  i = 0;
  for ( auto&& buf : buffer_queue_ ) {
    auto index = index_queue_.at( i );
    if ( writen_size_ >= index ) {
      i++;
      if ( writen_size_ >= index + buf.size() )
        continue;
      auto start_pos = writen_size_ - index;
      writen_size_ += std::min( writer().available_capacity(), std::max( (uint64_t)0, buf.size() - start_pos ) );
      output_.writer().push( buf.substr( start_pos ) );

    } else
      break;
  }
  // push part

  while ( i-- ) {
    index_queue_.pop_front();
    buffer_queue_.pop_front();
  }
  // pop part

  if ( last_ && end_index == writen_size_ )
    output_.writer().close();
  ;
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t result = 0;
  if ( index_queue_.empty() )
    return result;
  else {
    auto index = index_queue_.at( 0 );
    const auto size = index_queue_.size();
    for ( uint64_t i = 0; i < size; i++ ) {
      if ( index > index_queue_.at( i ) ) {
        auto temp = std::max( (int64_t)0,
                              (int64_t)buffer_queue_.at( i ).size() - (int64_t)( index - index_queue_.at( i ) ) );
        result += temp;
        index += temp;
      } else {
        result += buffer_queue_.at( i ).size();
        index += buffer_queue_.at( i ).size();
      }
    }
    return result;
  }
}
