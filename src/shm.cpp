#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>


#include <string>
#include <iostream>
#include <ctime>
#include <tuple>
#include <thread>
#include <numeric>

using namespace boost::interprocess;


int binary_search(const std::vector<int>& values, const int target)
{
	int head = 0;
	int tail = values.size() - 1;

	while (head <= tail)
	{
		int mid = (head + tail) / 2;

		if (values[mid] == target)
			return mid;
		else if (values[mid] < target)
			head = mid + 1;
		else
			tail = mid - 1;
	}
	return -1;
}



int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage ...\n";
		return 1;
	}

	const std::string opt = argv[1];
	const std::string mtx_name = "Hello";
	const std::string shm_name = "WOW";
	const int length = 1000000;
	const int size = 100000;
	std::vector<int> no_shm(length);

	std::iota(no_shm.begin(), no_shm.end(), 1);

	named_sharable_mutex gmtx(open_or_create, mtx_name.data());

	try
	{
		if (opt == "delete")
		{
			{
				named_sharable_mutex mtx(open_or_create, mtx_name.data());
				scoped_lock< named_sharable_mutex> lock(mtx);

				shared_memory_object::remove(shm_name.data());
				std::cout << "Delete shared memory\n";
			}

			named_sharable_mutex::remove(mtx_name.data());
			std::cout << "Delete mutex\n";
		}
		else if (opt == "force")
		{
			shared_memory_object::remove(shm_name.data());
			named_sharable_mutex::remove(mtx_name.data());
			std::cout << "Force delete mutex\n";
		}
		else if (opt == "init")
		{
			named_sharable_mutex mtx(open_or_create, mtx_name.data());
			scoped_lock<named_sharable_mutex> lock(mtx);

			shared_memory_object shm(create_only, shm_name.data(), read_write);
			shm.truncate(sizeof(int) * length);
			mapped_region region(shm, read_write);

			int* ptr = reinterpret_cast<int*>(region.get_address());
			for (int i = 0; i < length; i++)
			{
				ptr[i] = i + 1;
			}

			std::cout << "Allocate shared memory\n";
		}
		else if (opt == "check")
		{
			for (int i = 0; i < 80; i++)
			{
				named_sharable_mutex mtx(open_only, mtx_name.data());
				scoped_lock<named_sharable_mutex> lock(mtx);

				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				std::cout << "Wake up\n";
			}
		}
		else if (opt == "watch")
		{
			named_sharable_mutex mtx(open_only, mtx_name.data());
			sharable_lock<named_sharable_mutex> lock(mtx);

			shared_memory_object shm(open_only, shm_name.data(), read_write);
			mapped_region region(shm, read_write);

			int* ptr = reinterpret_cast<int*>(region.get_address());
			std::cout << "ptr[20]: " << ptr[20] << " | ptr[30]: " << ptr[30] << '\n';
		}
		else if (opt == "simul" || opt == "noshm" || opt == "empty" || opt == "global")
		{
			time(NULL);

			const auto get_array = [&]() {
				try {
					if (opt == "noshm")
					{
						return no_shm;
					}
					if (opt == "empty")
					{
						return std::vector<int>();
					}

					named_sharable_mutex mtx(open_only, mtx_name.data());
					sharable_lock<named_sharable_mutex> lock(mtx);

					shared_memory_object shm(open_only, shm_name.data(), read_write);
					mapped_region region(shm, read_write);

					int* ptr = reinterpret_cast<int*>(region.get_address());
					std::vector<int> ret;

					for (int i = 0; i < length; i++) {
						ret.push_back(ptr[i]);
					}

					return ret;
				}
				catch (interprocess_exception& ex)
				{
					std::cout << "thread error code: " << ex.get_error_code() << '\n';
					return std::vector<int>();
				}
			};

			const auto get_array_by_g = [&]() {
				sharable_lock<named_sharable_mutex> lock(gmtx);
				
				shared_memory_object shm(open_only, shm_name.data(), read_write);
				mapped_region region(shm, read_write);

				int* ptr = reinterpret_cast<int*>(region.get_address());
				std::vector<int> ret;

				for (int i = 0; i < length; i++) {
					ret.push_back(ptr[i]);
				}

				return ret;
			};

			double avg = 0.;

			const auto rt = [&]() {
				std::this_thread::sleep_for(std::chrono::microseconds(10));

				auto start = std::chrono::high_resolution_clock::now();

				const auto& shm_array = (opt != "global") ? get_array() : get_array_by_g();
				if (shm_array.empty())
				{
					goto LAST;
				}
					

				for (int i = 0; i < size; i++)
				{
					const int target = rand() % length;
					const int offset = binary_search(shm_array, target);
					if (offset + 1 != target) {
						std::cout << "Search fail\n";
					}
					const double ld = 3.14 / offset;
					(void)ld;
				}

				LAST:
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> diff = end - start;
				{
					named_sharable_mutex mtx(open_or_create, mtx_name.data());
					scoped_lock<named_sharable_mutex> lock(mtx);
					avg += diff.count();
					std::cout << "Duration " << diff.count() << " msec\n";
				}
			};

			std::vector<std::thread> ths;
			for (int i = 0; i < 48; i++) {
				ths.push_back(std::thread(rt));
			}
			for (int i = 0; i < ths.size(); i++) {
				ths[i].join();
				//std::cout << "Terminated " << i << '\n';
			}
			std::cout << "Completed. Avg: "<< avg / ths.size() << " msec\n";
		}
		else
		{
			std::cout << "invalid\n";
		}
		return 0;

	}
	catch (interprocess_exception& ex)
	{
		std::cerr << "error: " << ex.get_error_code() << std::endl;
		return 1;
	}

	return 0;

}




/*int main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cout << "Usage...\n";
		return 0;
	}

	const std::string opt = argv[1];
	std::srand(std::time(nullptr));

	try
	{
		if (opt == "create")
		{
			shared_memory_object shm(create_only, "MySharedMemory", read_write);
			shm.truncate(1000);
			mapped_region region(shm, read_write);

			std::pair<int, int>* arr = reinterpret_cast<std::pair<int, int>*>(region.get_address());
			for (int i = 0; i < 20; i++) {
				arr[i].first = std::rand();
				arr[i].second = std::rand();
			}
		}
		else if (opt == "delete")
		{
			shared_memory_object::remove("MySharedMemory");
		}
		else if (opt == "check")
		{
			shared_memory_object shm(open_only, "MySharedMemory", read_only);
			mapped_region region(shm, read_only);

			std::pair<int, int>* arr = reinterpret_cast<std::pair<int, int>*>(region.get_address());

			for (int i = 0; i < 20; i++)
			{
				std::cout << arr[i].first << ":" << arr[i].second << "\n";
			}
		}
		else if (opt == "sort")
		{
			shared_memory_object shm(open_only, "MySharedMemory", read_write);
			mapped_region region(shm, read_write);

			std::pair<int, int>* arr = reinterpret_cast<std::pair<int, int>*>(region.get_address());

			std::qsort(arr, 20, 8, [](const void* lhs, const void* rhs) {
				const std::pair<int, int> &left = *reinterpret_cast<const std::pair<int, int>*>(lhs);
				const std::pair<int, int> &right = *reinterpret_cast<const std::pair<int, int>*>(rhs);

				const bool cmp1 = (std::make_tuple(left.first, left.second) < std::make_tuple(right.first, right.second));
				const bool cmp2 = (std::make_tuple(left.first, left.second) == std::make_tuple(right.first, right.second));

				if (cmp1)
					return -1;
				if (cmp2)
					return 0;
				return 1;
			});
		}
	}
	catch (interprocess_exception& ex)
	{
		std::cerr << ex.get_error_code() << std::endl;
		return ex.get_error_code();
	}

	return 0;
}*/
