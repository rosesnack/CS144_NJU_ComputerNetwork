#include "reassembler.hh"
#include <map>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  cout << first_index << " " << first_unassembled_index_ << endl;
  // Your code here.
  if (first_index > first_unassembled_index_ && first_index < first_unassembled_index_ + output_.writer().available_capacity()) {
    if (is_last_substring) {
      has_end = true;
      end = first_index;
    }
    index.push(first_index);
    if (first_index + data.length() > first_unassembled_index_ + output_.writer().available_capacity()) {
      data = data.substr(0, first_unassembled_index_ + output_.writer().available_capacity() - first_index);
      has_end = false;
    }
    myMap[first_index] = data;
    bytes_pending_ += data.length();
    return;
  }
  if (first_index == first_unassembled_index_) {
    cout << data << endl;
    cout << output_.writer().available_capacity() << endl;
    first_unassembled_index_ += min(output_.writer().available_capacity(), data.length());
    output_.writer().push(data);
    cout << "f " << first_unassembled_index_ << endl;
  }
  else if (first_index < first_unassembled_index_) {
    if (first_index + data.length() <= first_unassembled_index_)
      return;
    data = data.substr(first_unassembled_index_ - first_index);
    first_unassembled_index_ += min(output_.writer().available_capacity(), data.length());
    output_.writer().push(data);
  }
  if (is_last_substring) {
    output_.writer().close();
  }
  while (!index.empty() && index.top() <= first_unassembled_index_) {
    uint64_t i = index.top();
    index.pop();
    string d = myMap[i];
    myMap.erase(i);
    bytes_pending_ -= d.length();
    bool flag = false;
    if (has_end && end == i) flag = true;
    insert(i, d, flag);
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
