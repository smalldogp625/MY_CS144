#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
            :_capacity(capacity), 
            written_cnt(0),
            read_cnt(0),
            end_input_flag(false),
            _byte_stream()

{ DUMMY_CODE(capacity); }

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t delda_cnt = data.length();
    if(delda_cnt>_capacity-_byte_stream.size())
    {
        delda_cnt = _capacity-_byte_stream.size();
    }
    for(size_t i=0;i<delda_cnt;++i)
    {
        _byte_stream.push_back(data[i]);
    }
    written_cnt +=delda_cnt;
    return {delda_cnt};
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    size_t delta_cnt = min(len,_byte_stream.size());
    return {string(_byte_stream.begin(),_byte_stream.begin()+delta_cnt)};
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len); 
    size_t delta_cnt = min(len,_byte_stream.size());
    _byte_stream.erase(_byte_stream.begin(),_byte_stream.begin()+delta_cnt);
    read_cnt+=delta_cnt;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    string data = peek_output(len);
    pop_output(len);
    return {data};
}

void ByteStream::end_input() {end_input_flag =true;}

bool ByteStream::input_ended() const { return {end_input_flag}; }

size_t ByteStream::buffer_size() const { return {_byte_stream.size()}; }

bool ByteStream::buffer_empty() const { return {_byte_stream.empty()}; }

bool ByteStream::eof() const { return end_input_flag&&buffer_empty(); }

size_t ByteStream::bytes_written() const { return {written_cnt}; }

size_t ByteStream::bytes_read() const { return {read_cnt}; }

size_t ByteStream::remaining_capacity() const { return {_capacity-_byte_stream.size()}; }
