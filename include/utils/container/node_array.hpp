
#include "vector"

// Node based array container
// Wrapper of std::vector (for simplicity')
// Iterator only get invalidated when it's value gets deleted
// No random access and no random insertion
template <class T>
class NodeArray
{
	struct Iterator
	{
		
		
		
	};
	
	
	typedef T value_type;
	typedef std::ptrdiff_t difference_type;
	typedef std::size_t size_type;
	typedef Iterator iterator;
	
	
	

	struct Node
	{
		bool active = true;
		size_t next = 0;
		T value;
	};

	std::vector<Node> data;

	void push_back(const T &value)
	{
		Node n = {true};
		n.value = value;
		if (!data.empty())
		{
			data.back().next = data.size() - 1u;
		}
		data.push_back(value);
	}
	
	void pop_back()
	{
		data.pop_back();
		while (data.front().active)
		{
			data.pop_back();
		}
	}
	
	void insert(const T& value)
	{
		if (!data.empty() && !data.front().active)
		{
			size_type index = 1;
			for (; index < data.size(); ++index)
			{
				if (data[index].active)
				  break;
			}
			data.front().value = value;
			data.front().next = index;
		}
		
		for (size_t i = 1; i < data.size(); ++i)
		{
			if (!data[i].active)
			{
				data[i-1].next = i;
				Node& node = data[i];
				node.active = true;
				node.next = i;
				node.value = value;
				return;
			}
		}
	}
	
	
	
	T& back()
	{
		return data.back().value;
	}
	
	T& front()
	{
		return data.front().value;
	}

	void reserve(size_t x)
	{
		data.reserve(x);
	}

	size_type capacity()
	{
		return data.capacity();
	}
};