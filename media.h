
#ifndef _media_
#define _media_
#include <stdint.h>

#define MAX_MEDIA_DESC_LEN (60)

typedef struct media_entry_t media_entry_t;

struct  media_entry_t
{
	int32_t name_len;
	char name[MAX_MEDIA_DESC_LEN];
};

typedef enum media_res_t media_res_t;
enum media_res_t {
	MEDIA_RES_IDLE = 0,
	MEDIA_RES_MINOR_UPDATE_NEEDED,
	MEDIA_RES_MAJOR_UPDATE_NEEDED
};

int32_t media_init(const int32_t max_entries,const int32_t max_entries_per_page);
int32_t media_import_list(const char* path,int32_t do_sort);
void media_get_partition(media_entry_t** me,int32_t* start,int32_t* end,int32_t* selected);
media_res_t media_next_page();
media_res_t media_prev_page();
media_res_t media_prev_item();
media_res_t media_next_item();
void media_close();
void media_save_state();
void media_load_state();
void media_dirty_state() ;
#endif

