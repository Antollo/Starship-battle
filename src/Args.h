#ifndef ARGS_H_
#define ARGS_H_

#include <string>
#include <map>

class Args
{
public:
	Args(int argc, const char* argv[])
	{
		for (int i=1; i+1 < argc; i+=2)
		{
			m.emplace(argv[i], argv[i+1]);
		}
	}
	const std::string& operator[](const std::string& key)
	{
		return m[key];
	}
private:
	std::map<std::string, std::string> m;
};

#endif
