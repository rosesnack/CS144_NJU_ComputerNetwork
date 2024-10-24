#include "byte_stream.hh"
#include <cassert>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // if (error_ || is_closed()) return ;
  uint64_t ac = available_capacity();
  if (data.length() <= ac) {
    queue_ += data;
    pushed_ += data.length();    
  }
  else {
    queue_ += data.substr(0, ac);
    pushed_ += ac;
  }
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - queue_.length();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && queue_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  return queue_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if (len > queue_.length())
    len = queue_.length();
  //queue_ = queue_.substr(len);
  queue_.erase(0, len);  //faster
  popped_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return pushed_ - popped_;
}
