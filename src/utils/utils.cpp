#include "utils/utils.hpp"

// Definitions of non‑template, non‑inline functions declared in utils.hpp

std::string str_replace(
	std::string& str,
	const std::string& from,
	const std::string& to)
{
	std::string::size_type pos = 0;
	while ((pos = str.find(from, pos)) != std::string::npos) {
		str.replace(pos, from.length(), to);
		pos += to.length();
	}
	return str;
}

std::string str_unfold(const std::string &str)
{
	char *buf = new char[str.size() + 2];
	int counter = 0;
	int j = 0;
	bool quoted = false;
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str.at(i) == '\"')
			quoted ^= 1;
		else if (str.at(i) == '}')
			--counter;

		if (counter == 0)
		{
			// Don't write char
		}
		else
		{
			if (str[i] == ',' && !quoted && counter == 1)
				buf[j++] = '\n';
			else
				buf[j++] = str.at(i);
		}

		if (str.at(i) == '{')
			++counter;
	}
	buf[j] = '\0';
	std::string ret = std::string(buf);
	delete[] buf;
	return ret;
}

int str_to_int(std::string &str)
{
	char *p;
	long val = strtol(str.c_str(), &p, 10);
	return (p[0] == '\0' ? val : -1);
}

sf::Vector2i string_to_ivec(const std::string &str)
{
	auto strings = str_split(str, ",");
	sf::Vector2i ret = {1, 1};
	const char *err = "Can't convert string to int";
	if (strings.size() > 0)
	{
		try
		{
			ret.x = std::stoi(strings[0]);
		}
		catch (const std::invalid_argument &e)
		{
			WARNING("%s: %s", err, e.what());
		}
		catch (const std::out_of_range &e)
		{
			WARNING("%s: %s", err, e.what());
		}
	}
	if (strings.size() > 1)
	{
		try
		{
			ret.y = std::stoi(strings[1]);
		}
		catch (const std::invalid_argument &e)
		{
			WARNING("%s: %s", err, e.what());
		}
		catch (const std::out_of_range &e)
		{
			WARNING("%s: %s", err, e.what());
		}
	}

	return ret;
}

std::string str_enclose(const std::string &str)
{
	std::string ret;
	ret.reserve(str.size() + 4ul);
	ret += "{ ";
	ret += str;
	ret += " }";
	return ret;
}

std::string str_unenclose(const std::string &str)
{
	std::string ret = str_trim(str);
	if (ret.front() == '{' && ret.back() == '}')
	{
		ret.erase(ret.begin());
		ret.erase(ret.end() - 1);
	}
	return ret;
}