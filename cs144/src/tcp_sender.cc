#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return window_got_ ? bytes_sent_ - next_index_ : 1;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return buffer_.front().wait;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if ( !buffer_.empty() ) {
    if ( window_got_ and buffer_.front().wait > TCPConfig::MAX_RETX_ATTEMPTS ) {
      if ( window_zero_flag_ ) {
        if ( buffer_.front().msg.payload.empty() ) {
          if ( reader().is_finished() and !fin_ ) {
            buffer_.front().wait = 0;
            buffer_.front().zero_mark = false;
            buffer_.front().msg.FIN = true;
          } else
            buffer_.pop_front();
        } else {
          auto str1 { buffer_.front().msg.payload.substr( 0, 1 ) };
          auto str2 { buffer_.front().msg.payload.substr( 1 ) };
          auto to_send1 { buffer_.front() };
          auto to_send2 { buffer_.front() };
          buffer_.pop_front();
          to_send2.msg.seqno = to_send2.msg.seqno + 1;
          // to_send2.index++;
          to_send2.msg.payload = str2;
          buffer_.push_front( to_send2 );
          to_send1.wait = 0;
          to_send1.zero_mark = false;
          to_send1.msg.payload = str1;
          buffer_.push_front( to_send1 );
        }
      } else {
        if ( buffer_.front().msg.sequence_length() > window_size_ && !fuck_you_ ) {
          auto str1 { buffer_.front().msg.payload.substr( 0, window_size_ ) };
          auto str2 { buffer_.front().msg.payload.substr( window_size_ ) };
          auto to_send1 { buffer_.front() };
          auto to_send2 { buffer_.front() };
          buffer_.pop_front();

          // to_send2.index += window_size_;
          to_send2.msg.seqno = to_send2.msg.seqno + 1;
          to_send2.msg.payload = str2;
          buffer_.push_front( to_send2 );
          buffer_.front().msg.FIN = reader().is_finished() and ( window_size_ > 0 ) and window_got_ and !fin_;
          if ( !fin_ )
            fin_ = buffer_.front().msg.FIN;

          to_send1.msg.payload = str1;
          buffer_.push_front( to_send1 );
          buffer_.front().wait = 0;

          window_size_ = 0;
        } else {
          buffer_.front().wait = 0;
          buffer_.front().msg.FIN = reader().is_finished() and ( window_size_ > 0 ) and window_got_ and !fin_;
          if ( !fin_ )
            fin_ = buffer_.front().msg.FIN;
        }
      }
      if ( !buffer_.empty() )
        transmit( buffer_.front().msg );
      bytes_sent_ += buffer_.front().msg.sequence_length();
    }
  }

  std::string str {};
  auto bytes_popped { input_.reader().bytes_popped() };
  uint16_t window_mark { window_got_ ? window_size_ : (uint16_t)UINT16_MAX };
  while ( window_mark ) {
    auto str_ { input_.reader().peek() };
    if ( str_.empty() )
      break;
    if ( str_.size() > window_mark )
      str_ = str_.substr( 0, window_mark );
    str += str_;
    window_mark -= str_.size();
    writer().reader().pop( str_.size() );
  }

  if ( !str.empty() || syn_ ) { // not empty or the first one
    for ( auto i { 0u }; i < str.size() || syn_; i += TCPConfig::MAX_PAYLOAD_SIZE ) {
      TCPSenderMessage to_send { isn_.wrap( bytes_popped + !syn_ + i, isn_ ),
                                 syn_,
                                 str.substr( i, std::min( TCPConfig::MAX_PAYLOAD_SIZE, str.size() - i ) ),
                                 false,
                                 input_.has_error() };

      if ( window_got_ )
        window_size_ -= to_send.sequence_length();

      to_send.FIN = reader().is_finished() and ( i + TCPConfig::MAX_PAYLOAD_SIZE >= str.size() )
                    and ( window_size_ > 0 ) and window_got_ and !fin_;
      buffer_.push_back( { to_send, 0, /*!syn_ + bytes_popped + to_send.FIN + i,*/ 0, true } );
      if ( window_got_ || syn_ ) {
        transmit( to_send );
        bytes_sent_ += to_send.sequence_length();
      } else {
        buffer_.back().wait = TCPConfig::MAX_RETX_ATTEMPTS + 1;
      }
      if ( !fin_ )
        fin_ = to_send.FIN;
      if ( syn_ )
        syn_ = false;
    }
  } else if ( reader().is_finished() and ( window_size_ > 0 ) and window_got_ and !fin_ and !window_zero_flag_ ) {
    TCPSenderMessage to_send { isn_.wrap( bytes_popped + !syn_, isn_ ), syn_, {}, true, input_.has_error() };
    fin_ = true;

    buffer_.push_back( { to_send, 0, /*!syn_ + bytes_popped + fin_,*/ 0, true } );
    transmit( to_send );
    bytes_sent_ += to_send.sequence_length();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // isn_.wrap(window_got_ ? bytes_sent_:1, isn_)
  return { isn_.wrap( window_got_ ? bytes_sent_ : 1, isn_ ), syn_, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( input_.has_error() )
    return;
  if ( msg.RST ) {
    input_.set_error();
    return;
  }
  window_size_ = msg.window_size;
  window_got_ = true;
  window_zero_flag_ = ( window_size_ == 0 );

  auto bytes_out { !syn_ + reader().bytes_popped() + fin_ };
  auto _next_index_ { msg.ackno->unwrap( isn_, check_point_ ) };
  if ( _next_index_ <= bytes_out && _next_index_ >= next_index_ )
    next_index_ = _next_index_;
  while ( !buffer_.empty() ) {
    if ( buffer_.front().msg.seqno.unwrap( isn_, check_point_ ) + buffer_.front().msg.sequence_length()
           <= next_index_
         && buffer_.front().wait < TCPConfig::MAX_RETX_ATTEMPTS ) {
      buffer_.pop_front();
      last_tick_ = 0;
    } else
      break;
  }

  if ( !window_zero_flag_ && window_size_ >= bytes_out - next_index_ ) {
    window_size_ -= ( bytes_out - next_index_ );
    fuck_you_ = true;
  } else
    fuck_you_ = false;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  last_tick_ += ms_since_last_tick;
  if ( !buffer_.empty() ) {
    if ( last_tick_ - buffer_.front().time >= initial_RTO_ms_ * ( 1 << buffer_.front().wait ) ) {
      buffer_.front().time = last_tick_;
      transmit( buffer_.front().msg );
      if ( buffer_.front().zero_mark )
        buffer_.front().wait++;
    }
  }
}
