#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);
    //传入的数据三种情况构成数据块
    //index>capacity+_head_index; [index                                 ]
    
    //1：包含关系;
    //2: [                              _head_index]
    //               [index                ]
    //3:[       ]
    if(index>=_capacity+_head_index)
    {
        return;//越界了，无法接受数据
    }
    block_note elm;
    //构建数据块
    if(index+data.length()<=_head_index)
    {
        goto JUDGE_EOF;
    }
    else if(index<_head_index)
    {
        size_t offset = _head_index-index;
        
        elm.data.assign(data.begin()+offset,data.end());
        elm.begin = index + offset;
        elm.length = elm.data.length(); 
    }
    else{
        elm.begin = index;
        elm.data = data;
        elm.length = data.length();
    }
    unassembled_byte_num +=elm.length; //未重组字节数


     // merge substring
     do{
        //合并_block中elem和ta之后的数据块
        long merged_bytes = 0;
        auto iter = _blocks.lower_bound(elm);

        while(iter!=_blocks.end()&&(merged_bytes=merge_block(elm,*iter))>=0)
        {
            unassembled_byte_num -=merged_bytes;
            _blocks.erase(iter);
            iter = _blocks.lower_bound(elm);
        }
        if(iter == _blocks.begin())
        {
            break;
        }
        //合并_block中elem和之前的数据块
        iter--;
        while((merged_bytes=merge_block(elm,*iter))>=0)
        {
            unassembled_byte_num-=merged_bytes;
            _blocks.erase(iter);
            iter = _blocks.lower_bound(elm);
            if(iter ==_blocks.begin())
            {
                break;
            }
            iter --;

        }

     }while(false);
     _blocks.insert(elm);
     //合并数据块之后，加入到out_put中去
    //_block_index:{0,1,2,3,...}
    //_output.write: return 传入的字节数，比如传入2字节，那么_head_index 为0+2 =2;
    //_head_index:下一个要传入的index
    if(!_blocks.empty()&&_blocks.begin()->begin == _head_index)
    {
        const block_note head_node = *_blocks.begin();
        size_t wirtten_bytes = _output.write(head_node.data);

        _head_index+=wirtten_bytes;
        unassembled_byte_num-=wirtten_bytes;
        _blocks.erase(_blocks.begin());

    }

    JUDGE_EOF:
    if (eof) {
        _eof_flag = true;
    }
    if (_eof_flag && empty()) {
        _output.end_input();
    }
}
//! elm：传入的数据块，1代表合并后的数据块
//！return 合并增加的数据长度 失败返回-1；
long StreamReassembler::merge_block(block_note &elm1 ,const block_note &elm2) //
{
    block_note x,y;
    //选取X作为第一个元素下表
    if(elm1.begin>elm2.begin)
    {
        x = elm2;
        y = elm1;
    }
    else
    {
        x = elm1;
        y = elm2;
    }
    if(x.begin+x.length<y.begin)
    {
        return -1; //合并失败
    }
    else if(x.begin+x.length>y.begin+y.length)
    {
        elm1 =x;
        return y.length;
    }
    else{
        elm1.begin = x.begin;
        elm1.data = x.data+y.data.substr(x.begin+x.length-y.begin);
        elm1.length = elm1.data.length();
        return x.begin+x.length-y.begin;

    }

}
size_t StreamReassembler::unassembled_bytes() const { return {unassembled_byte_num}; }

bool StreamReassembler::empty() const { return {unassembled_byte_num==0}; }
