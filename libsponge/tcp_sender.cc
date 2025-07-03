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

uint64_t TCPSender::bytes_in_flight() const { return {_outgoing_bytes}; }
/**
 *  尽可能地多装填string
 *  
 */
void TCPSender::fill_window() {
    size_t curr_window_size = _last_window_size ? _last_window_size:1;
    /*
    开始装填
    */
    while(curr_window_size>_outgoing_bytes){
        /**
         * 构建segment
         * 1.syn设置，seqno设置， payload_size取最大装填值和剩余空间的最小值
         * 2.装填payload,从_stream读取相印大小；
         * 3.设置fin
         *  1. 从来没发送过 FIN
         *  2. 输入字节流处于 EOF
         *  3. window 减去 payload 大小后，仍然可以存放下 FIN
         * 
        */
        TCPSegment segment;
        if(!_set_syn_flag){
            segment.header().syn = true;
            _set_syn_flag = true;
        }
        segment.header().seqno = next_seqno();

        const size_t payload_size = 
        min(TCPConfig::MAX_PAYLOAD_SIZE, curr_window_size-_outgoing_bytes - segment.header().syn);
        string payload = _stream.read(payload_size);

        if(!_set_fin_flag && _stream.eof() &&
            payload.size() + _outgoing_bytes < curr_window_size)
            _set_fin_flag = segment.header().fin = true;
        
        segment.payload() = Buffer(move(payload));

        if(segment.length_in_sequence_space() == 0)
            break;
        /**
         * 如果没有sent过，重置timeout,_tinmecount;
         * 将segment放入发射队列中
         * 
        */
        if(_outgoing_map.empty()){
            _timeout = _initial_retransmission_timeout;
            _timecount = 0;

        }

        _segments_out.push(segment);
        /**
         * 统计sent的字节数
         * 统计sent过的segment;
         * 下一个seqno；
        */
        _outgoing_bytes += segment.length_in_sequence_space();
        _outgoing_map.insert(make_pair(_next_seqno, segment));
        _next_seqno += segment.length_in_sequence_space();

        if(segment.header().fin)
            break;

    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { DUMMY_CODE(ackno, window_size);
    size_t abs_seqno = unwrap(ackno, _isn, _next_seqno);
    if(abs_seqno>_next_seqno) 
        return;
    for(auto iter = _outgoing_map.begin(); iter != _outgoing_map.end();){
        const TCPSegment& seg = iter->second;

        if(iter->first + seg.length_in_sequence_space()<= abs_seqno){
            _outgoing_bytes -= seg.length_in_sequence_space();
            iter = _outgoing_map.erase(iter);

            //代表已被接受，重置超时
            _timeout = _initial_retransmission_timeout;
            _timecount = 0;
        }
        else 
            break;
        
    }
        _consecitive_retransmissions_count = 0;
        _last_window_size = window_size;
        fill_window();

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick);
    _timecount += ms_since_last_tick;
    auto iter = _outgoing_map.begin();
    if(iter != _outgoing_map.end() && _timecount >= _timeout){
        if(_last_window_size > 0) _timeout *=2;

        _timecount = 0;
        _segments_out.push(iter->second);
        ++_consecitive_retransmissions_count;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecitive_retransmissions_count; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    _segments_out.push(segment);
}
