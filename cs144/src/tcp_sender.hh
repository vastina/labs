#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

typedef struct msg_buffer
{
  TCPSenderMessage msg;
  uint64_t time;
  // uint64_t index;
  uint8_t wait;
  bool zero_mark;
} msg_buffer;

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // uint64_t bytes_sent() const;

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  // Retransmission TimeOut
  uint64_t initial_RTO_ms_;

  std::deque<msg_buffer> buffer_;
  bool window_zero_flag_ {};
  bool fuck_you_ {};
  uint64_t bytes_sent_ {};

  uint64_t last_tick_ {};  // time
  bool syn_ { true };      // syn
  bool fin_ {};            // fin
  uint64_t next_index_ {}; // ack

  bool window_got_ { false };
  uint16_t window_size_ {};

  constexpr static uint64_t check_point_ { 0 };
};
