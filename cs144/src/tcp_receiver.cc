#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    RST = true;
    reader().writer().set_error();
  }

  if ( !zero_point_flag_ ) {
    if ( !message.SYN ) {
      msg_buffer_.push_back( std::move( message ) );
      return;
    } else {
      zero_point_ = message.seqno;
      zero_point_flag_ = true;
    }
  }

  auto index { message.seqno.unwrap( zero_point_, check_point_ ) - !message.SYN };
  reassembler_.insert( index, message.payload, message.FIN );

  while ( !msg_buffer_.empty() ) {
    auto front { std::move( msg_buffer_.front() ) };
    auto first_index { front.seqno.unwrap( zero_point_, check_point_ ) };
    reassembler_.insert( first_index, front.payload, front.FIN );
    msg_buffer_.pop_front();
  }

  ackno_ = zero_point_.wrap( writer().bytes_pushed() + ( zero_point_flag_ + writer().is_closed() ), zero_point_ );
}

TCPReceiverMessage TCPReceiver::send() const
{
  return { ackno_,
           (uint16_t)std::min( (uint64_t)UINT16_MAX, writer().available_capacity() ),
           RST || writer().has_error() };
}
