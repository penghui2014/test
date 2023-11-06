#include "async_message.hpp"

int CAsyncMessage::Wait(uint8_t msg,std::vector<uint8_t>& data, uint32_t timeout)
{
	std::shared_ptr<Message> message;
	
	{
		std::unique_lock<std::mutex> loc(m_mutex);
		if(0 == m_map.count(msg))
		{
			message = std::make_shared<Message>(msg);
			m_map[msg] = message;
		}
		else
		{
			message = m_map[msg];
		}
	}

	int ret = message->Wait(data,timeout);

	{
		std::unique_lock<std::mutex> loc(m_mutex);
		m_map.erase(msg);
	}

	return ret;
	
}

void CAsyncMessage::WakeUp(uint8_t msg,uint8_t* data, int len)
{
	std::unique_lock<std::mutex> loc(m_mutex);
	if(0 != m_map.count(msg))
	{
		m_map[msg]->Wake(data,len);
	}
}

int CAsyncMessage::AddMsg(uint8_t msg)
{
	std::shared_ptr<Message> message = std::make_shared<Message>(msg);
	int ret = 0;
	std::unique_lock<std::mutex> loc(m_mutex);
	if(0 != m_map.count(msg))
	{
		//printf("AddMsg %02x error\n",msg);
		ret = -1;
	}
	m_map[msg] = message;
	return ret;
}

