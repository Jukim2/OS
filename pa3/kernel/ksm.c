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

#include "param.h" // for NPROC
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"
#include "ksm.h"

int print_mode = 0;

extern struct proc proc[NPROC];

m_page m_pages[MAX_PAGE];

int find_same_hash(uint64 hash_value)
{
  for (int i = 0; i < MAX_PAGE; i++)
  {
    if (hash_value == m_pages[i].hash && m_pages[i].cnt < 16)
      return (i);
  }
  return (-1);
}

void clean_m_pages()
{
  for (int idx = 0; idx < MAX_PAGE; idx++)
  {
    if (m_pages[idx].cnt == 1 && !m_pages[idx].is_shared)
      memset(&m_pages[idx], 0, sizeof(m_page));
  }
}

void put_page(pte_t *pte, uint64 pa, uint64 hash_value)
{
  int next_idx = 0;
  for (int idx = 0; idx < MAX_PAGE; idx++)
  {
    if (m_pages[idx].cnt == 0 && m_pages[idx].first_pte == 0)
    {
      next_idx = idx;
      break;
    }
  }

  if (print_mode)
    printf("-> put page %p at m_pages[%d]\n", (void *)pa, next_idx);
  m_pages[next_idx].first_pte = pte; 
  m_pages[next_idx].pa = pa;
  m_pages[next_idx].hash = hash_value;
  m_pages[next_idx].cnt = 1;
  next_idx++;
}

void merge_pte(pte_t *pte, uint64 old_pa, uint64 new_pa)
{
  kfree((void *)old_pa);
  *pte = PA2PTE(new_pa) | PTE_FLAGS_NO_W(*pte);
  if (print_mode)
    printf("+ Merged from old pa : %p to new pa : %p\n", old_pa, new_pa);
}

uint64 pte2pa(pte_t *pte)
{
  uint64 pa;

  if(pte == 0)
    return 0;
  if((*pte & PTE_V) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

void print_process_page(struct proc *p)
{
  printf("\n---------------------------process : %d-----------------------\n\n", p->pid);
  pagetable_t ptb = p->pagetable;

  for (int i = 0; i < p->sz; i+= PGSIZE)
  {
    uint64 pa = walkaddr(ptb, i);
    if (pa)
      printf("va : %p pa : %p hash : %d\n", i, (void *)pa, xxh64((void *)pa, PGSIZE));
  }
}

void print_m_pages()
{
  printf("---------------- m_pages -------------------\n");
  for (int idx = 0; idx < MAX_PAGE; idx++)
  {
    if (m_pages[idx].cnt)
      printf("idx : %d cnt : %d hash : %d\n", idx, m_pages[idx].cnt, m_pages[idx].hash);
  }
  printf("--------------------------------------------\n");
}

void point_zero_page(pte_t *pte, uint64 old_pa)
{
  if (old_pa == zero_page)
    return;
  if (print_mode)  
    printf("- page %p point to zero_page : %p\n", old_pa, zero_page);
  kfree((void *)old_pa);
  *pte = PA2PTE(zero_page) | PTE_V | PTE_U | PTE_R;
}

uint64
sys_ksm(void)
{
  printf("----------------------------------KSM-----------------------------------------\n");
  // 모든 프로세스 대상으로
  struct proc *p;
  int my_pid = myproc()->pid;
  
  for(p = proc; p < &proc[NPROC]; p++) 
  {
    if (!p->pid || p->pid == 1 || p->pid == 2 || p->pid == my_pid)
      continue;

    if (print_mode)
    print_process_page(p);
    // print_m_pages();

    for (int i = 0; i < p->sz; i+= PGSIZE)
    {
      pte_t *pte = walk(p->pagetable, i, 0);
      uint64 pa = pte2pa(pte);
      if (!pa)
        continue;
      uint64 hash = xxh64((void *)pa, PGSIZE);
      int idx = find_same_hash(hash);

      if (hash == zero_hash)
        point_zero_page(pte, pa);
      else if (idx == -1)
        put_page(pte, pa, hash);
      else if (idx != -1 && pa != m_pages[idx].pa)
      {
        merge_pte(pte, pa, m_pages[idx].pa);
        if (++(m_pages[idx].cnt) == 2)
        {
          *(m_pages[idx].first_pte) = PA2PTE(m_pages[idx].pa) | PTE_FLAGS_NO_W(*pte); 
          m_pages[idx].is_shared = 1;
        }
      }
    }
  }
  clean_m_pages();
  
  printf("----------------------------------KSM End-----------------------------------------\n\n");
  return freemem;
}

