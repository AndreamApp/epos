/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

struct chunk {
    char signature[4];  /* "OSEX" */
    struct chunk *next; /* ptr. to next chunk */
    int state;          /* 0 - free, 1 - used */
#define FREE   0
#define USED   1

    int size;           /* size of this chunk */
};

// TODO: Thread safe

#define CHUNK_SIZE sizeof(struct chunk)

static struct chunk *chunk_head;

void *g_heap;
void *tlsf_create_with_pool(uint8_t *heap_base, size_t heap_size)
{
    chunk_head = (struct chunk *)heap_base;
    strncpy(chunk_head->signature, "OSEX", 4);
    chunk_head->next = NULL;
    chunk_head->state = FREE;
    chunk_head->size  = heap_size;

    return NULL;
}

void debug(){
	printf("\n");
	struct chunk * chk = chunk_head;
	while(chk != NULL){
		printf("%s(%d) ", chk->state == FREE ? "FREE" : "USED", chk->size);
		chk = chk->next;
	}
	printf("\n");
}

// ? CHUNK_SIZE是否计算在size中？看来是要计算的
void *malloc(size_t size)
{
	if(size == 0){
		return NULL;
	}
	int find = 0;
	struct chunk * chk = chunk_head;
	while(chk != NULL){
		if(chk->state == FREE){
			// 找到一块足够大的连续的空闲内存
			if(chk->size >= (size + CHUNK_SIZE*2)){
				struct chunk * nxt = (struct chunk *) ((uint8_t *)chk + size + CHUNK_SIZE);
				memcpy(nxt, chk, CHUNK_SIZE);
				nxt->size -= size + CHUNK_SIZE;
				
				chk->next = nxt;
				chk->state = USED;
				chk->size = size + CHUNK_SIZE;
				find = 1;
				break;
			}
			else if(chk->size == (size + CHUNK_SIZE)){
				chk->state = USED;
				find = 1;
				break;
			}
		}
		chk = chk->next;
	}
	
	if(find){
		//debug();
		return (void *)(chk + 1);
	}
	else{
		return NULL;
	}
}

// ? 不能合并前一个空闲块吗？不是双向链表，只能从头遍历了
void free(void *ptr)
{
	if(ptr == NULL){
		return;
	}
	struct chunk * chk = ((struct chunk *) ptr) - 1;
	if(strncmp(chk->signature, "OSEX", 4) == 0){
		chk->state = FREE;
		// 如果下一块也是FREE，合并
		chk = chunk_head;
		while(chk->next != NULL){
			if(chk->state == FREE && chk->next->state == FREE){
				chk->size += chk->next->size;
				chk->next = chk->next->next;
			}
			else{
				chk = chk->next;
			}
		}
	}
	//debug();
}

void *calloc(size_t num, size_t size)
{
	void * ptr = malloc(num * size);
	memset(ptr, 0, num * size);
    return ptr;
}

void *realloc(void *oldptr, size_t size)
{
	void * ptr = NULL;
	if(oldptr != NULL){
		free(oldptr);
	}
	if(size > 0){
		ptr = malloc(size);
	}
    return ptr;
}

/*************D O  N O T  T O U C H  A N Y T H I N G  B E L O W*************/
static void tsk_malloc(void *pv)
{
  int i, c = (int)pv;
  char **a = malloc(c*sizeof(char *));
  for(i = 0; i < c; i++) {
	  a[i]=malloc(i+1);
	  a[i][i]=17;
  }
  for(i = 0; i < c; i++) {
	  free(a[i]);
  }
  free(a);

  task_exit(0);
}

#define MESSAGE(foo) printf("%s, line %d: %s", __FILE__, __LINE__, foo)
void test_allocator()
{
  char *p, *q, *t;

  MESSAGE("[1] Test malloc/free for unusual situations\r\n");

  MESSAGE("  [1.1]  Allocate small block ... ");
  p = malloc(17);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  p[0] = p[16] = 17;
  printf("PASSED\r\n");

  MESSAGE("  [1.2]  Allocate big block ... ");
  q = malloc(4711);
  if (q == NULL) {
    printf("FAILED\r\n");
	return;
  }
  q[4710] = 47;
  printf("PASSED\r\n");

  MESSAGE("  [1.3]  Free big block ... ");
  free(q);
  printf("PASSED\r\n");

  MESSAGE("  [1.4]  Free small block ... ");
  free(p);
  printf("PASSED\r\n");

  MESSAGE("  [1.5]  Allocate huge block ... ");
  q = malloc(32*1024*1024-sizeof(struct chunk));
  if (q == NULL) {
    printf("FAILED\r\n");
	return;
  }
  q[32*1024*1024-sizeof(struct chunk)-1]=17;
  free(q);
  printf("PASSED\r\n");

  MESSAGE("  [1.6]  Allocate zero bytes ... ");
  if ((p = malloc(0)) != NULL) {
    printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [1.7]  Free NULL ... ");
  free(p);
  printf("PASSED\r\n");

  MESSAGE("  [1.8]  Free non-allocated-via-malloc block ... ");
  int arr[5] = {0x55aa4711,0x5a5a1147,0xa5a51471,0xaa551741,0x5aa54171};
  free(&arr[4]);
  if(arr[0] == 0x55aa4711 &&
     arr[1] == 0x5a5a1147 &&
	 arr[2] == 0xa5a51471 &&
	 arr[3] == 0xaa551741 &&
	 arr[4] == 0x5aa54171) {
	  printf("PASSED\r\n");
  } else {
	  printf("FAILED\r\n");
	  return;
  }

  MESSAGE("  [1.9]  Various allocation pattern ... ");
  int i;
  size_t pagesize = sysconf(_SC_PAGESIZE);
  for(i = 0; i < 7411; i++){
    p = malloc(pagesize);
	p[pagesize-1]=17;
    q = malloc(pagesize * 2 + 1);
	q[pagesize*2]=17;
    t = malloc(1);
	t[0]=17;
    free(p);
    free(q);
    free(t);
  }

  char **a = malloc(2741*sizeof(char *));
  for(i = 0; i < 2741; i++) {
	  a[i]=malloc(i+1);
	  a[i][i]=17;
  }
  for(i = 0; i < 2741; i++) {
	  free(a[i]);
  }
  free(a);

  if(chunk_head->next != NULL || chunk_head->size != 32*1024*1024) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [1.10] Allocate using calloc ... ");
  int *x = calloc(17, 4);
  for(i = 0; i < 17; i++)
	  if(x[i] != 0) {
		  printf("FAILED\r\n");
		  return;
	  } else
	      x[i] = i;
  free(x);
  printf("PASSED\r\n");

  MESSAGE("[2] Test realloc() for unusual situations\r\n");

  MESSAGE("  [2.1]  Allocate 17 bytes by realloc(NULL, 17) ... ");
  p = realloc(NULL, 17);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  p[0] = p[16] = 17;
  printf("PASSED\r\n");
  MESSAGE("  [2.2]  Increase size by realloc(., 4711) ... ");
  p = realloc(p, 4711);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  if ( p[0] != 17 || p[16] != 17 ) {
    printf("FAILED\r\n");
	return;
  }
  p[4710] = 47;
  printf("PASSED\r\n");

  MESSAGE("  [2.3]  Decrease size by realloc(., 17) ... ");
  p = realloc(p, 17);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  if ( p[0] != 17 || p[16] != 17 ) {
    printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [2.4]  Free block by realloc(., 0) ... ");
  p = realloc(p, 0);
  if (p != NULL) {
	printf("FAILED\r\n");
    return;
  } else
	printf("PASSED\r\n");

  MESSAGE("  [2.5]  Free block by realloc(NULL, 0) ... ");
  p = realloc(realloc(NULL, 0), 0);
  if (p != NULL) {
    printf("FAILED\r\n");
    return;
  } else
	printf("PASSED\r\n");

  MESSAGE("[3] Test malloc/free for thread-safe ... ");

  int t1, t2;
  char *s1 = malloc(1024*1024),
       *s2 = malloc(1024*1024);
  t1=task_create(s1+1024*1024, tsk_malloc, (void *)5000);
  t2=task_create(s2+1024*1024, tsk_malloc, (void *)5000);
  task_wait(t1, NULL);
  task_wait(t2, NULL);
  free(s1);
  free(s2);

  if(chunk_head->next != NULL || chunk_head->size != 32*1024*1024) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");
}
/*************D O  N O T  T O U C H  A N Y T H I N G  A B O V E*************/
