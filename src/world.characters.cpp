#include "world.characters.hpp"

#include "config.hpp"
#include "logger.hpp"

Characters character_list;	// global container of chars

void Characters::push_front(const CHAR_DATA::shared_ptr& character)
{
	m_list.push_front(character);
	m_object_raw_ptr_to_object_ptr[character.get()] = m_list.begin();
}

void Characters::foreach_on_copy(const foreach_f function) const
{
	const list_t list = get_list();
	std::for_each(list.begin(), list.end(), function);
}

bool Characters::erase_if(const predicate_f predicate)
{
	bool result = false;

	auto i = m_list.begin();
	while (i != m_list.end())
	{
		if (predicate(*i))
		{
			i = m_list.erase(i);
			result = true;
		}
		else
		{
			++i;
		}
	}

	return result;
}

void Characters::remove(const CHAR_DATA* character)
{
	const auto index_i = m_object_raw_ptr_to_object_ptr.find(character);
	if (index_i == m_object_raw_ptr_to_object_ptr.end())
	{
		const size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		snprintf(buffer, BUFFER_SIZE,
			"Character at address %p requested to remove not found in the world.", character);
		mudlog(buffer, BRF, LVL_IMPL, SYSLOG, TRUE);

		return;
	}

	m_list.erase(index_i->second);
	m_object_raw_ptr_to_object_ptr.erase(index_i);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
