#include "tcp_minnow_socket.hh"
// #include "socket.hh"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

void errorhandling( const char* message, int error )
{
  fprintf( stderr, "%s : %d \n", message, error );
};

void get_URL( const string& host, const string& path )
{
  CS144TCPSocket sock {};
  sock.connect( Address( host, "http" ) );

  sock.write( string_view( "GET " + path + " HTTP/1.1\r\n" ) );
  sock.write( string_view( "Host: " + host + "\r\n" ) );
  sock.write( string_view( "Connection: close\r\n\r\n" ) );

  while ( not sock.eof() ) {
    string buffer;
    sock.read( buffer );
    cout << buffer;
  }
  sock.close();

  // passed too
  // sock.wait_until_closed();

  // Below is a implement use syscall which will make checkpoint4 useless
  // struct addrinfo hints, *res, *p;
  // int status;
  // char ipstr[INET6_ADDRSTRLEN];
  // memset( &hints, 0, sizeof hints );
  // hints.ai_family = AF_UNSPEC; // IPv4 或 IPv6
  // hints.ai_socktype = SOCK_STREAM;
  // if ( ( status = getaddrinfo( host.c_str(), NULL, &hints, &res ) ) != 0 ) {
  //   fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( status ) );
  //   return;
  // }
  // for ( p = res; p != NULL; p = p->ai_next ) {
  //   void* addr;
  //   // 获取地址结构体中的 IP 地址
  //   if ( p->ai_family == AF_INET ) { // IPv4
  //     struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
  //     addr = &( ipv4->sin_addr );
  //     inet_ntop( p->ai_family, addr, ipstr, sizeof ipstr );
  //     // printf( "ip: %s\n", ipstr );
  //     break;
  //   } else { // IPv6
  //     struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
  //     addr = &( ipv6->sin6_addr );
  //   }
  //   // 将二进制地址转换成文本表示，并打印

  //   // printf("%s: %s\n", ipver, ipstr);
  // }
  // freeaddrinfo( res );

  // int so = socket( AF_INET, SOCK_STREAM, 0 );
  // if ( so == -1 ) {
  //   errorhandling( "invalid socket: ", errno );
  //   return;
  //   std::cout << path;
  // }
  // // errorhandling( "invalid socket: ", errno );

  // struct sockaddr_in serveraddr;
  // memset( &serveraddr, 0, sizeof( serveraddr ) );
  // serveraddr.sin_family = AF_INET;
  // serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );
  // serveraddr.sin_port = htons( 7688 );
  // if ( bind( so, (struct sockaddr*)&serveraddr, sizeof( serveraddr ) ) == -1 ) {
  //   printf( "bind fail, error code: %d\n", errno );
  // }
  // // errorhandling( "invalid socket: ", errno );

  // struct sockaddr_in serveaddr;
  // memset( &serveaddr, 0, sizeof( serveaddr ) );
  // serveaddr.sin_family = AF_INET;
  // serveaddr.sin_addr.s_addr = inet_addr( host.c_str() );
  // serveaddr.sin_addr.s_addr = inet_addr( ipstr );
  // short PORT = 80;
  // serveaddr.sin_port = htons( PORT );

  // if ( connect( so, (struct sockaddr*)&serveaddr, sizeof( serveaddr ) ) == -1 )
  //   errorhandling( "connect fail: ", errno );
  // // errorhandling( "invalid socket: ", errno );

  // char buffer[BUFSIZ] = { 0 };
  // strcpy( buffer, "GET " );
  // strcat( buffer, path.c_str() );
  // strcat( buffer,
  //         " HTTP/1.1\r\n"
  //         "Host: cs144.keithw.org\r\n"
  //         "Connection: close\r\n\r\n\0" );
  // // std::cout << buffer << std::endl;
  // if ( write( so, buffer, strlen( buffer ) ) == -1 )
  //   errorhandling( "write fail: ", errno );
  // // errorhandling( "invalid socket: ", errno );
  // char recvbuf[BUFSIZ] = { 0 };
  // int len = 0, offset = 0;
  // std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
  // while ( ( offset = read( so, recvbuf + len, BUFSIZ ) ) > 0 ) {
  //   len += offset;
  //   if ( std::chrono::duration_cast<std::chrono::seconds>( std::chrono::system_clock::now() - start ).count() <
  //   10 )
  //     continue;
  // }
  // if ( len == -1 )
  //   errorhandling( "read fail: ", errno );
  // cout << recvbuf;
  // 不要在后面跟'\n'，会不通过。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
