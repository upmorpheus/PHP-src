/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

#ifndef ST_INCLUDED

#define ST_INCLUDED

typedef unsigned long st_data_t;
#define ST_DATA_T_DEFINED

typedef struct st_table st_table;

struct st_hash_type {
    int (*compare)();
    int (*hash)();
    int (*key_free)();
    st_data_t (*key_clone)();
};

struct st_table {
    struct st_hash_type *type;
    int num_bins;
    int num_entries;
    struct st_table_entry **bins;
};

typedef struct {
  unsigned char* s;
  unsigned char* end;
} st_strend_key;

#define st_is_member(table,key) st_lookup(table,key,(st_data_t *)0)

enum st_retval {ST_CONTINUE, ST_STOP, ST_DELETE, ST_CHECK};

#ifndef _
# define _(args) args
#endif
#ifndef ANYARGS
# ifdef __cplusplus
#   define ANYARGS ...
# else
#   define ANYARGS
# endif
#endif

st_table *st_init_table _((struct st_hash_type *));
st_table *st_init_table_with_size _((struct st_hash_type *, int));
st_table *st_init_numtable _((void));
st_table *st_init_numtable_with_size _((int));
st_table *st_init_strtable _((void));
st_table *st_init_strtable_with_size _((int));
st_table *st_init_strend_table_with_size _((int));
int st_delete _((st_table *, st_data_t *, st_data_t *));
int st_delete_safe _((st_table *, st_data_t *, st_data_t *, st_data_t));
int st_insert _((st_table *, st_data_t, st_data_t));
int st_insert_strend _((st_table *, const unsigned char*, const unsigned char*, st_data_t));
int st_lookup _((st_table *, st_data_t, st_data_t *));
int st_lookup_strend _((st_table *, const unsigned char*, const unsigned char*, st_data_t*));
void st_foreach _((st_table *, int (*)(ANYARGS), st_data_t));
void st_add_direct _((st_table *, st_data_t, st_data_t));
void st_add_direct_strend _((st_table *, const unsigned char*, const unsigned char*, st_data_t));
void st_free_table _((st_table *));
void st_cleanup_safe _((st_table *, st_data_t));
st_table *st_copy _((st_table *));

extern st_data_t st_nothing_key_clone _((st_data_t key));
extern int st_nothing_key_free _((st_data_t key));

#define ST_NUMCMP	((int (*)()) 0)
#define ST_NUMHASH	((int (*)()) -2)

#define st_numcmp	ST_NUMCMP
#define st_numhash	ST_NUMHASH

#endif /* ST_INCLUDED */
