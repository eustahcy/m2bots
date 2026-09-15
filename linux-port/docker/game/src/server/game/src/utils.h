#ifndef __INC_METIN_II_UTILS_H__
#define __INC_METIN_II_UTILS_H__

#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include "../../common/length.h"

#define IS_SET(flag, bit)		((flag) & (bit))
#define SET_BIT(var, bit)		((var) |= (bit))
#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)	((var) = (var) ^ (bit))

struct SParsedTime {
	DWORD days;
	DWORD hours;
	DWORD minutes;
	DWORD seconds;
};


inline float DISTANCE_SQRT(long dx, long dy)
{
    return ::sqrt((float)dx * dx + (float)dy * dy);
}

inline int DISTANCE_APPROX(int dx, int dy)
{
	int min, max;

	if (dx < 0)
		dx = -dx;

	if (dy < 0)
		dy = -dy;

	if (dx < dy)
	{
		min = dx;
		max = dy;
	}
	else
	{
		min = dy;
		max = dx;
	}

    // coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
    return ((( max << 8 ) + ( max << 3 ) - ( max << 4 ) - ( max << 1 ) +
		( min << 7 ) - ( min << 5 ) + ( min << 3 ) - ( min << 1 )) >> 8 );
}

#ifndef __WIN32__
inline WORD MAKEWORD(BYTE a, BYTE b)
{
	return static_cast<WORD>(a) | (static_cast<WORD>(b) << 8);
}
#endif

extern void set_global_time(time_t t);
extern time_t get_global_time();

std::string mysql_hash_password(const char* tmp_pwd);

extern int	dice(int number, int size);
extern size_t str_lower(const char * src, char * dest, size_t dest_size);

extern void	skip_spaces(char **string);

extern const char *	one_argument(const char *argument, char *first_arg, size_t first_size);
extern const char *	two_arguments(const char *argument, char *first_arg, size_t first_size, char *second_arg, size_t second_size);
extern const char *	first_cmd(const char *argument, char *first_arg, size_t first_arg_size, size_t *first_arg_len_result);

extern void split_argument(std::string_view stArg, std::vector<std::string> & vecArgs);
extern std::vector<std::string> split_arguments(std::string_view stArg);

extern int CalculateDuration(int iSpd, int iDur);

extern float gauss_random(float avg = 0, float sigma = 1);

extern int parse_time_str(const char* str);

extern bool WildCaseCmp(const char *w, const char *s);

extern PIXEL_POSITION get_pos_on_circle(long center_x, long center_y, long radius, float t);
extern const char* seconds_to_smart_time(DWORD second);

extern bool is_time_between_hours(DWORD fromHour, DWORD toHour);
extern std::vector<VECTOR> find_intermediate_points(VECTOR pos1, VECTOR pos2, int count);
extern DWORD get_time_until(DWORD targetHour, DWORD targetMinutes=0, DWORD baseTimestamp=0);
extern std::vector<int> generate_unique_numbers(DWORD number_count, DWORD max_number_count, DWORD seed);
extern bool is_empty_page_grid(BYTE* grid, WORD bCell, BYTE bSize, int iExceptionCell, DWORD dwGridWidth, DWORD dwGridHeight, BYTE bPageCount, bool check_inventory=false, BYTE customGridSize=INVENTORY_DEFAULT_MAX_NUM);
extern std::string decrypt_aes_hash(const std::string& cipherText, const std::string& key);
#endif /* __INC_METIN_II_UTILS_H__ */
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
