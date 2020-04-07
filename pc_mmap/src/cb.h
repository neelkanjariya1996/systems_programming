#define SUCCESS 0
#define FAILURE !SUCCESS

extern int num_records;
extern int num_of_users;
extern char file_name[15];

typedef struct record_t_ {

  bool is_dummy;
  char user_id[5];
  char topic[16];
  int score;
} record_t;

typedef struct buffer_t_ {

  pthread_cond_t read;
  pthread_cond_t write;
  pthread_mutex_t mutex;
  int front;
  int rear;
  int size;
  int num_of_elements;
  record_t arr[];
} buffer_t;

typedef struct buffer_array_t_ {

  int users;
  pthread_mutex_t super_mutex;
  buffer_t arr[];
} buffer_array_t;

static inline buffer_t*
get_user_buffer (buffer_array_t *buf_arr, int user)
{

  long single_buf_size = sizeof(buffer_t) + (num_records * sizeof(record_t));
  long offset = (user * single_buf_size);
  return (buffer_t *) ((char *)buf_arr->arr + offset);
}

extern buffer_array_t*
buffer_array_init ();

extern void
buffer_array_cleanup (buffer_array_t *buf_arr);

extern void
print_record (record_t record);

extern void
print_buffer_array (buffer_array_t *buf);

extern bool
is_buffer_full (buffer_t* buf);

extern bool
is_buffer_empty (buffer_t* buf);

extern void
enqueue (buffer_t* buf, record_t record);

extern int
dequeue (buffer_t* buf, record_t* record);
