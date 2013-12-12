#include <map>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;

typedef std::map<int, int> EVENT_MAP;
typedef std::map<int, int>::iterator EVENT_MAP_IT;
typedef boost::posix_time::ptime Time;
typedef boost::minstd_rand base_generator_type;
