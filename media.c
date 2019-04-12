/*Just a basic media list
	TODO maybe wrap everything around struct contexts later..
*/
#include "media.h"
#include "filesystem/filesystem.h"
#include <string.h>
#include <malloc.h>
#include <libdragon.h>

#define unc_addr(_addr) ((void *)(((unsigned long)(_addr))|0x20000000))

static int32_t max_media_entries_per_page = 15;

static media_entry_t* media_entries;
static int32_t media_entry_ptr = 0;
static int32_t media_entry_size = 0;
static int32_t media_entry_highlighted = 0;
static int32_t media_entry_start = 0;
static int32_t media_entry_end = 0;
static const int media_state_id = 0xc0de;

void media_dirty_state() {	/*"Destroy" state file*/
	fs_handle_t* f;
	int32_t media_state_buffer[512/4];
 
	if (!(f = fs_open("gnuboy/res/menu.state", "wb"))) { return; }
	media_state_buffer[0] = ~media_state_id; 
	fs_write(f,(void*)media_state_buffer,sizeof(media_state_buffer));
	fs_close(f);
}

void media_save_state() {
	fs_handle_t* f;
	int32_t media_state_buffer[512/4];

	if ((!media_entries) || (!media_entry_size)) { return; }
 
	if (!(f = fs_open("gnuboy/res/menu.state", "wb"))) { return; }
	media_state_buffer[0] = media_state_id; /*hdr*/
	media_state_buffer[1] = media_entry_ptr; /*Ptr hash*/
	media_state_buffer[2] = media_entry_size; /*Ptr hash*/
	media_state_buffer[3] = media_entry_start; /*S*/
	media_state_buffer[4] = media_entry_end; /*E*/
	media_state_buffer[5] = media_entry_highlighted; /*HL*/
	media_state_buffer[6] = strlen(media_entries[media_entry_start+media_entry_highlighted].name); /*len HL*/
	fs_write(f,(void*)media_state_buffer,sizeof(media_state_buffer));
	fs_close(f);
}

void media_load_state() {
	fs_handle_t* f;
	int32_t media_state_buffer[512/4];

	if ((!media_entries) || (!media_entry_size)) { return; }

	if (!(f = fs_open("gnuboy/res/menu.state", "rb"))) {return; }
	fs_read(f,(void*)media_state_buffer,sizeof(media_state_buffer));

	if (media_state_buffer[0] != media_state_id) { fs_close(f); return; }
	if (media_state_buffer[1] != media_entry_ptr){ fs_close(f); return; } /*Invalid state*/
	if (media_state_buffer[2] != media_entry_size) { fs_close(f); return; } /*Invalid state*/
	if (media_entry_ptr > media_entry_size) { fs_close(f); return; } /*Invalid state*/

	/*Restore*/
	media_entry_start = media_state_buffer[3];
	media_entry_end = media_state_buffer[4];
	media_entry_highlighted = media_state_buffer[5];
	if (media_state_buffer[6] != strlen(media_entries[media_entry_start+media_entry_highlighted].name)) {
		/*FAIL - Directory structure changed*/
		media_entry_highlighted = 0;
		media_entry_start = 0;
		media_entry_end = (max_media_entries_per_page > media_entry_ptr) ? media_entry_ptr : max_media_entries_per_page;
	}
	fs_close(f);
}


int32_t media_init(const int32_t max_entries,const int32_t max_entries_per_page)
{
	max_media_entries_per_page = max_entries_per_page;
	media_entry_size = max_entries;
	media_entry_start = media_entry_end = media_entry_ptr = 0;
	media_entries = malloc(sizeof(media_entry_t) * max_entries);
	if (!media_entries) {
		return 0;
	}
	memset(media_entries,0,sizeof(media_entry_t) * max_entries);
	data_cache_hit_writeback_invalidate(media_entries,sizeof(media_entry_t) * max_entries);
	return 1;
}

void media_close() 
{
	media_entry_highlighted = media_entry_start = media_entry_end = media_entry_ptr = 0;
	if (media_entries) {
		free(media_entries);
		media_entries = 0;
	}
}

void media_invalidate_all()
{
	media_entry_highlighted = media_entry_start = media_entry_end = media_entry_ptr = 0;
	data_cache_hit_writeback_invalidate(media_entries,sizeof(media_entry_t) * media_entry_size);
}

static int media_add(const char* in_name,fs_file_type_t type)
{
	const char* name = in_name;
	media_entry_t* me;
	int32_t len;
	
#if 0
	int32_t i;
	int32_t delim;
#endif

	if (media_entry_ptr >= media_entry_size)
		return 0;
	else if (type == FSFT_DIR)
		return 1;/*skip*/ 

	len = strlen(name);

	if ( (len > MAX_MEDIA_DESC_LEN) || (len < 1) )
		return 1; /*skip*/ 

	me = &media_entries[media_entry_ptr++];
#if 0
	delim = -1;
	for(i = len - 1;i > 0;--i) {
		if(name[i] == '.') {
			delim = i;
			break;
		}
	}

	if(delim != -1) {
		me->name_len = delim;
		strncpy(me->name,name,delim);
		return 1;
	}
#endif
	
	me->name_len = len;
	strcpy(me->name,name);
	
	return 1;
}

static int cmp_qs(const void* a, const void* b) {
	const media_entry_t* pa = (const media_entry_t*)a;
	const media_entry_t* pb = (const media_entry_t*)b;
	return strcmp(pa->name,pb->name);
}

int32_t media_import_list(const char* path,int32_t do_sort)
{
	media_invalidate_all();
	fs_list_files(path,media_add);

	if (media_entry_ptr && do_sort) {	
	extern void stack_based_quicksort (void *const pbase, size_t total_elems, size_t size,size_t stride,
	   int (*cmp)(const void*,const void*));


		stack_based_quicksort(media_entries,media_entry_ptr,sizeof(media_entry_t),sizeof(media_entry_t),cmp_qs);
	}
	media_entry_highlighted = 0;
	media_entry_start = 0;
	media_entry_end = (max_media_entries_per_page > media_entry_ptr) ? media_entry_ptr : max_media_entries_per_page;
	return media_entry_ptr != 0;
}
 
void media_get_partition(media_entry_t** me,int32_t* start,int32_t* end,int32_t* selected) 
{
	*me = media_entries;
	*start = media_entry_start;
	*end = media_entry_end;
	*selected = media_entry_highlighted;
}

media_res_t media_next_page() 
{
	if(media_entry_end + max_media_entries_per_page <= media_entry_ptr) {
		media_entry_highlighted = 0;
		media_entry_start = media_entry_end;
		media_entry_end =  media_entry_start + max_media_entries_per_page;
		return MEDIA_RES_MAJOR_UPDATE_NEEDED;
	} else if(media_entry_end != media_entry_ptr) {
		media_entry_highlighted = 0;
		media_entry_start = media_entry_end;
		media_entry_end = media_entry_ptr;	
		return MEDIA_RES_MAJOR_UPDATE_NEEDED;	
	}
	return MEDIA_RES_IDLE;
}

media_res_t media_prev_page() 
{
	if(media_entry_start >= max_media_entries_per_page) {
		media_entry_start -= max_media_entries_per_page;

		if(media_entry_start < 0)
			media_entry_start = 0;
		
		media_entry_end = media_entry_start + max_media_entries_per_page;
		if(media_entry_end > media_entry_ptr)
			media_entry_end = media_entry_ptr;

		//media_entry_highlighted = media_entry_end - media_entry_start;
		//media_entry_highlighted -= media_entry_highlighted != 0;
		return MEDIA_RES_MAJOR_UPDATE_NEEDED;
	}
	return MEDIA_RES_IDLE;
}

media_res_t media_prev_item() 
{
	if(--media_entry_highlighted < 0) {
		media_entry_highlighted = 0;
		return media_prev_page();
	}
	return MEDIA_RES_MINOR_UPDATE_NEEDED;
}

media_res_t media_next_item() 
{
	if(++media_entry_highlighted >= media_entry_end - media_entry_start) {
		media_entry_highlighted = (media_entry_end - media_entry_start) - 1;
		return media_next_page();
	}
	return MEDIA_RES_MINOR_UPDATE_NEEDED;
}
 
