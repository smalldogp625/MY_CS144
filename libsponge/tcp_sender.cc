#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _outgoing_bytes; }

void TCPSender::fill_window() {
    size_t curr_window_size = _last_window_size?_last_window_size:1;
    
    //循环填充
    while (curr_window_size>_outgoing_bytes)
    {
        //构建单个TCPsegment
        TCPSegment segment;
        if(!_set_syn_flag)
        {
            segment.header().syn = true;
            _set_syn_flag =true;
        }
        segment.header().seqno = next_seqno();
        //从bytestream中读取数量的字节作为playload;
        //
        const size_t play_load_size = 
        min(TCPConfig::MAX_PAYLOAD_SIZE,curr_window_size-_outgoing_bytes-segment.header().syn);
        string playload = _stream.read(play_load_size);
        /*
         * 读取好后，如果满足以下条件，则增加 FIN
        *  1. 从来没发送过 FIN
        *  2. 输入字节流处于 EOF
        *  3. window 减去 payload 以及正在传输的大小后，仍然可以存放下 FIN(正在传输+将要传输<window)
        */
       if(!_set_fin_flag&&_stream.eof()&&_outgoing_bytes+playload.size()<curr_window_size)
       {
            _set_fin_flag = 
            segment.header().fin =true;
       }
       segment.payload() = Buffer(move(playload));
       //如果没有任何数据，则停止数据包的发送
       if(segment.length_in_sequence_space()==0)
        break;
        // 如果没有正在等待的数据包，则重设更新时间
       if(_outgoing_map.empty())
       {
        _timeout = _initial_retransmission_timeout;
        _timecount = 0;
       }
        // 发送
        _segments_out.push(segment);


        //追踪数据包
        _outgoing_bytes+=segment.length_in_sequence_space();
        _outgoing_map.insert(make_pair(_next_seqno,segment));//1
        //更新待发送的abs——segno
        _next_seqno +=segment.length_in_sequence_space();//2

         // 如果设置了 fin，则直接退出填充 window 的操作
         if (segment.header().fin)
         {
             break;
         }
        

    }
    
    

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { DUMMY_CODE(ackno, window_size); 
    size_t abs_seqno = unwrap(ackno,_isn,_next_seqno);//2

    if(abs_seqno>_next_seqno)
        return ;
    //遍历输出的map
    for(auto iter = _outgoing_map.begin();iter!=_outgoing_map.end();)
    {
        const TCPSegment& seg = iter->second;
        //如果已经接受完毕，则重置超时时间
        if(seg.length_in_sequence_space()+iter->first<=abs_seqno)
        {
            _outgoing_bytes-=seg.length_in_sequence_space();
            iter = _outgoing_map.erase(iter);
            //
            _timeout = _initial_retransmission_timeout;
            _timecount = 0;

        }
        //未接受直接返回
        else
            break;
       

    } 
    _consecutive_retransmissions_count =0;
        _last_window_size = window_size;
        fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); 
    _timecount+=ms_since_last_tick;
    auto iter =_outgoing_map.begin();
    //超时
    if(iter!=_outgoing_map.end()&&_timecount>=_timeout)
    {
         if(_last_window_size>0)
         _timeout*= 2;
        _timecount=0;
        _segments_out.push(iter->second);
         // 连续重传计时器增加
         ++_consecutive_retransmissions_count;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions_count; }

void TCPSender::send_empty_segment() {

    TCPSegment segment;
    segment.header().seqno=next_seqno();
    _segments_out.push(segment);
}
