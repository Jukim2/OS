//----------------------------------------------------------------
//
//  4190.307 Operating Systems (Spring 2024)
//
//  Project #4: KSM (Kernel Samepage Merging)
//
//  May 7, 2024
//
//  Dept. of Computer Science and Engineering
//  Seoul National University
//
//----------------------------------------------------------------

// #ifdef SNU

#define MAX_PAGE 32768

uint64 xxh64(void *input, unsigned int len);

typedef struct m_page
{
  pte_t *first_pte;
  uint64 pa;
  uint64 hash;
  int cnt;
  int is_shared;
} m_page;

extern uint64 zero_page;
extern uint64 zero_hash;
extern int freemem;
extern m_page m_pages[MAX_PAGE];


int find_same_hash(uint64 hash_value);

void clean_m_pages();

void put_page(pte_t *pte, uint64 pa, uint64 hash_value);

void merge_pte(pte_t *pte, uint64 old_pa, uint64 new_pa);

uint64 pte2pa(pte_t *pte);

void print_process_page(struct proc *p);

void print_m_pages();

void point_zero_page(pte_t *pte, uint64 old_pa);
// #endif
