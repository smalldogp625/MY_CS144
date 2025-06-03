#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
_unassemble_str(), _unassembled_bytes_num(0), _eof_index(-1),_next_assemble_idx(0),
_output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);
    /*
    对重叠部分做出处理：{
    1. 子串前面
    2. 子串后面
    }
    */
    auto pos_iter = _unassemble_str.upper_bound(index);
    if (pos_iter != _unassemble_str.begin()) {
        pos_iter--;
    }
    size_t new_idx = index;
    /*
    如果前面有子串
    */
    if(pos_iter!=_unassemble_str.end()&&pos_iter->first<=index){
        const size_t up_index = pos_iter->first;
        if(index < up_index+pos_iter->second.size())
            new_idx = up_index+pos_iter->second.size();
    }
    else if(index<_next_assemble_idx) 
        new_idx = _next_assemble_idx;
    const size_t data_start_pos = new_idx - index;

    ssize_t data_size = data.size() - data_start_pos;
    //unassembled_bytes()后半截处理
    pos_iter = _unassemble_str.lower_bound(new_idx);
    while(pos_iter!=_unassemble_str.end()&& new_idx <= pos_iter->first){
        //有重叠
        const size_t data_end_pos = new_idx + data_size;
        if(pos_iter->first < data_end_pos){//部分重叠
            if(data_end_pos < pos_iter->first + pos_iter->second.size()){
                data_size = pos_iter->first - new_idx;
                break;
            }
            else{//全部重叠
                _unassembled_bytes_num -=pos_iter->second.size();
                pos_iter = _unassemble_str.erase(pos_iter);
                continue;
            }
        }
        else 
            break;
    }

    //字串处理完毕，准备插入_output

    size_t _first_unaccepted_idx = _capacity -_output.buffer_size() + _next_assemble_idx;
    if(_first_unaccepted_idx<=new_idx) // 接收到的index放不进去
    return ;

    //
    if(data_size>0){
        const string new_data = data.substr(data_start_pos,data_size);
        //可以直接插入
        if(new_idx ==  _next_assemble_idx){
            const size_t write_num = _output.write(new_data);
            _next_assemble_idx += write_num;
            //写不完，到达容量了
            if(write_num<new_data.size()){
                const string data2store = new_data.substr(write_num,new_data.size()-write_num);
                _unassembled_bytes_num += data2store.size();
                _unassemble_str.insert(make_pair(_next_assemble_idx,std::move(data2store)));
            }
        }
        else{
             const string data2store = new_data.substr(0,new_data.size());
                _unassembled_bytes_num += data2store.size();
                _unassemble_str.insert(make_pair(new_idx,std::move(data2store)));
        }
    }

    //
    for(auto iter = _unassemble_str.begin();iter!=_unassemble_str.end();/**/){
        
        if(iter->first==_next_assemble_idx){
            const size_t write_num = _output.write(iter->second);
            _next_assemble_idx += write_num;
            if(write_num < iter->second.size())
            {
                 _unassembled_bytes_num += iter->second.size() - write_num;
                const string new_data = iter->second.substr(write_num,
                    iter->second.size()-write_num);

                _unassemble_str.insert(make_pair(_next_assemble_idx,std::move(new_data)));
                _unassembled_bytes_num -= iter->second.size();
                iter = _unassemble_str.erase(iter); 
                break;
            }
            _unassembled_bytes_num -= write_num;
            iter = _unassemble_str.erase(iter);

        }
        else 
            break;
    }

    if(eof)
        _eof_index = index +data.size();
    if(_eof_index<=_next_assemble_idx)
        _output.end_input();
}
size_t StreamReassembler::unassembled_bytes() const { return {_unassembled_bytes_num}; }

bool StreamReassembler::empty() const { return {_unassembled_bytes_num==0}; }
