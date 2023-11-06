#pragma once

#include <condition_variable>
#include <thread>
#include <mutex>
#include <map>
#include <stdint.h>
#include <vector>

class CAsyncMessage
{
	class Message
	{
		enum MsgStats_e
		{
			SLEEP,
			WAKE,
		};
			
		private:
			MsgStats_e m_status{SLEEP};
			uint8_t m_msg;
			std::vector<uint8_t> m_data;
			std::condition_variable m_cond;
			std::mutex m_mutex;
			
		public:
			Message(uint8_t msg):
				m_msg(msg)
			{
				
			}

			uint8_t Msg()
			{
				return m_msg;
			}

			void Wake(uint8_t* data,int len)
			{
				std::unique_lock<std::mutex> loc(m_mutex);
				m_data = std::move(std::vector<uint8_t>(data,data+len));
				m_status = WAKE;
				m_cond.notify_one();
			}

			int Wait(std::vector<uint8_t>& data,uint32_t timeout)
			{
				std::unique_lock<std::mutex> loc(m_mutex);
				if(WAKE == m_status)
				{
					m_status = SLEEP;
					data = std::move(m_data);
					return 0;
				}
				else
				{
					if(0 == timeout)
					{
						m_cond.wait(loc);
					}
					else
					{
						if(std::cv_status::timeout == m_cond.wait_for(loc,std::chrono::milliseconds(timeout)))
						{
							return -1;
						}						
					}
					
					if(WAKE == m_status)
					{
						m_status = SLEEP;
						data = std::move(m_data);
						return 0;
					}
				}
				
				return -1;
			}
			
			std::vector<uint8_t>& Data()
			{
				return m_data;
			}

	};

	private:
		std::map<uint8_t, std::shared_ptr<Message>> m_map;
		std::mutex m_mutex;
		

	public:
		CAsyncMessage() = default;
		~CAsyncMessage() = default;
		
		int Wait(uint8_t msg, std::vector<uint8_t>& data , uint32_t timeout = 0);
		void WakeUp(uint8_t msg,uint8_t* data, int len);
		int	AddMsg(uint8_t msg);


};