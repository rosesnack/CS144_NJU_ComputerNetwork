#include "reassembler.hh"

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t last_index = first_index + data.length() - 1;
  if (data.empty() || last_index < begin_i_() || first_index > end_i_()) {
    if (is_last_substring && bytes_pending() == 0) output_.writer().close();
    return;
  }
  if (is_last_substring && last_index <= end_i_())
    has_end = true;

  if (first_index <= begin_i_()) {
    data = data.substr(begin_i_() - first_index);
    output_.writer().push(data);
  }
  else if (first_index > begin_i_()) {
    if (last_index > end_i_()) 
      data = data.substr(0, end_i_() - first_index + 1);
    mergeString(first_index, data);
  }
  
  auto x = myMap.begin();
  while (x != myMap.end() && x->first <= begin_i_()) {
    uint64_t i = x->first;
    string d = x->second;
    x = myMap.erase(x);
    if (i + d.length() - 1 < begin_i_())
      continue;
    d = d.substr(begin_i_() - i);
    output_.writer().push(d);
  }
  if (has_end && myMap.empty()) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  uint64_t bp = 0;
  for (const auto& pair : myMap) {
      bp += pair.second.length();
  }
  return bp;
}

void Reassembler::mergeString(uint64_t first_index, string data) {
    auto it = myMap.lower_bound(first_index);
    auto next = it;
    if (it != myMap.begin() && (it == myMap.end() || it->first > first_index))
        it--;
    int newStart = first_index;
    string newStr = data;

    if (it != myMap.end() && it->first <= first_index && it->first + it->second.size() - 1 >= first_index - 1) {
        newStart = it->first;
        string existingStr = it->second;
        if (it->first + it->second.size() < first_index + data.length())
            newStr = existingStr.substr(0, first_index - newStart) + data;
        else
            newStr = existingStr;
    }
    myMap[newStart] = newStr;
    
    while (next != myMap.end() && newStart + newStr.size() >= next->first) {
        if (newStart + newStr.size() < next->first + next->second.size()) {
            newStr += next->second.substr(newStart + newStr.size() - next->first);
        }
        next = myMap.erase(next);
    }
    myMap[newStart] = newStr;
}
