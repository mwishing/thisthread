#pragma once

#include <cstdint>
#include <vector>
#include <zmq.hpp>

namespace zmq
{
    typedef std::vector<std::string> Frames;

    class Socket : public socket_t
    {
    public:
        Socket(context_t& context, int type) : socket_t(context, type) {}

        Socket(context_t& context, int type, const std::string& id) : socket_t(context, type)
        {
            zmq_setsockopt(this->operator void *(), ZMQ_IDENTITY, id.c_str(), id.length());
        }

        bool sendSeparator()
        {
            zmq::message_t msg;
            return socket_t::send(msg, ZMQ_SNDMORE);
        }

        bool send(const std::string& frame, int flags =0)
        {
            zmq::message_t msg(frame.length());
            memcpy(msg.data(), frame.c_str(), frame.length());
            return socket_t::send(msg, flags);
        }

        bool send(const Frames& frames)
        {
            // trivial cases
            if(!frames.size())
                return false;
            if(frames.size() == 1)
                return send(frames[0]);

            // more than one frame
            for(unsigned int i = 0; i < frames.size() - 1; ++i)
            {
                if(!send(frames[i], ZMQ_SNDMORE))
                    return false;
                if(!sendSeparator())
                    return false;
            }
            // last frame
            return send(frames.back());
        }

        /*
            n: expected number of frames
         */
        Frames blockingRecv(int n, bool checked =true)
        {
            Frames frames;
            frames.reserve(n);

            int currentFrame = 1;
            do {
                zmq::message_t message;
                if(!socket_t::recv(&message, 0))
                    throw error_t();

                if(!(currentFrame++ % 2))
                { // skipping separators
                    if(message.size())
                        throw error_t();
                }
                else
                {
                    const char* base = static_cast<const char*>(message.data());
                    frames.push_back(std::string(base, base + message.size()));
                }
            } while(sockopt_rcvmore());

            if(checked && frames.size() != n)
                throw error_t();

            return frames;
        }

        std::string recvAsString(int flags =0)
        {
            zmq::message_t message;
            if(!socket_t::recv(&message, flags))
                throw error_t();

            const char* base = static_cast<const char*>(message.data());
            return std::string(base, base + message.size());
        }
    private:
        int sockopt_rcvmore()
        {
            int64_t rcvmore;
            size_t type_size = sizeof (int64_t);
            getsockopt(ZMQ_RCVMORE, &rcvmore, &type_size);
            return static_cast<int>(rcvmore);
        }
    };
}
