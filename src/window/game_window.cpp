#include "window/game_window.hpp"


#include <nlohmann/json.hpp>
#include "utils/class/logger.hpp"


bool json_file_load(nlohmann::json& json, const std::string& filePath)
{

	FILE* file = fopen(filePath.c_str(), "r");

	if (!file)
	{
		LOG_ERROR("%s not found: %s", 
			filePath.c_str(),
			strerror(errno));
		return false;
	}

	try
	{
		json = nlohmann::json::parse(file, nullptr, true, true);
	}
	catch (nlohmann::detail::parse_error& e)
	{
		LOG_ERROR("%s parsing error %s", filePath.c_str(), e.what());
		fclose(file);
		return false;
	}

	if (json.is_discarded())
	{
		LOG_ERROR("%s parse error", filePath.c_str());
		fclose(file);
		return false;
	}
	else
		LOG("%s parsed succesfully", filePath.c_str());

	fclose(file);
	return true;
}

bool json_file_write(
	const nlohmann::json& json, 
	const std::string& filePath)
{

	FILE* file = fopen(filePath.c_str(), "w");
	if (!file)
	{
		LOG_ERROR("%s not found: %s",
			filePath.c_str(),
			strerror(errno));
		return false;
	}

	std::string jsonString = json.dump(4); // Pretty-print with 4 spaces
	if (fputs(jsonString.c_str(), file) == EOF)
	{
		LOG_ERROR("Failed to write data to file %s",
			filePath.c_str());
		return false;
	}

	if (fclose(file) == EOF)
	{
		LOG_ERROR("Failed to close file %s",
			filePath.c_str());
		return false;
	}

	return true;
}
