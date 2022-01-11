#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <string>
#include <iostream>
#include <ctime>
#include <tuple>
#include <thread>
#include <numeric>
#include <mutex>


template<int SN, typename T>
T get_local_static(T v)
{
	(void)SN;
	static T t = v;

	return t;
}

using namespace boost::interprocess;

class ttt
{
public:
	ttt() {
		std::cout << "Constructor\n";
	}
	~ttt() {
		std::cout << "Destructor\n";
	}
};

shared_memory_object& get_shmobject()
try
{
	static shared_memory_object mtx(open_only, "WWWWWWW", read_only);
	static ttt t;
	

	return mtx;
}
catch (interprocess_exception& ex)
{
	auto error_code = ex.get_error_code();
	static shared_memory_object mtx;

	return mtx;
}


int main(int argc, char ** argv)
{
	double d = get_local_static<0>(3.);
	double dd = get_local_static<1>(6.);
	double ddd = get_local_static<2>(9.);

	std::cout << d << " " << dd << " " << ddd << '\n';
#ifndef _DEBUG
	if (argc != 2) {
		std::cout << "Usage...\n";
		return 1;
	}
	const std::string opt = argv[1];
#else
	const std::string opt = "else";
#endif
	
	const std::string shm_name = "wwwwwwwwow";

	try
	{
		if (opt == "open")
		{
			shared_memory_object shm(open_only, shm_name.data(), read_only);
			mapped_region region(shm, read_only);
			int* ptr = reinterpret_cast<int*>(region.get_address());

			shm.truncate(11);
		}
		else if (opt == "staticopen")
		{
			shared_memory_object& shm1 = get_shmobject();
			shared_memory_object mtx(open_or_create, "WWWWWWW", read_only);
			shared_memory_object& shm2 = get_shmobject();
		}
		else {
			;//
		}
	}
	catch (interprocess_exception& ex)
	{
		auto t = ex.get_error_code();
	}


	return 0;
}