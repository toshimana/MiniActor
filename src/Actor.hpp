/*

MIT License

Copyright (c) 2016 toshimana

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include <mutex>
#include <queue>
#include <atomic>

// ActorBase : nonthreading actor
template <typename Message_>
class ActorBase
{
public:
	using Message = Message_;

	ActorBase(void)
		: halt_flag(false)
		, message_queue()
	{}

	virtual ~ActorBase(void)
	{}

	template <typename T>
	void send(const T& msg)
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (halt_flag) return;
		message_queue.push(msg);
	}

	void receive(void)
	{
		Message msg;
		{
			std::lock_guard<std::mutex> lock(mtx);

			if (halt_flag) return;
			if (message_queue.empty()) return;

			msg = message_queue.front();
			message_queue.pop();
		}

		process_message(msg);
	}

protected:
	std::atomic_bool halt_flag;

	void to_halt(void)
	{
		std::lock_guard<std::mutex> lock(mtx);
		halt_flag = true;
	}

private:
	std::mutex mtx;
	std::queue<Message> message_queue;

	virtual void process_message(const Message& msg) = 0;
};


#include <thread>
#include <chrono>

// Actor : running ActorBase on unique thread
template <typename Message_>
class Actor : public ActorBase <Message_>
{
public:
	Actor( void )
		: ActorBase()
		, th( &Actor<Message_>::exec, this )
	{
	}

	virtual ~Actor( void )
	{
		if (!halt_flag) halt();
	}

	void halt() { 
		to_halt(); 
		th.join();
	}

private:
	std::thread th;

	void exec( void )
	{
		while ( !halt_flag ) {
			receive();
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}
	}
};
