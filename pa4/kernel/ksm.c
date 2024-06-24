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

int print_mode = 1;
int scanned_val, merged_val;
uint64 scanned, merged;

extern struct proc proc[NPROC];

m_page m_pages[MAX_PAGE];

int find_same_pa(uint64 pa)
{
  for (int i = 0; i < MAX_PAGE; i++)
  {
    if (pa == m_pages[i].pa)
      return (i);
  }
  return (-1);
}

int find_idx_to_put(uint64 hash_value)
{
  for (int i = 0; i < MAX_PAGE; i++)
  {
    if (hash_value == m_pages[i].hash && m_pages[i].cnt < 16)
    {
      return (i);
    }
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
    if (m_pages[idx].cnt == 0)
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
}

void merge_page(pte_t *pte, uint64 old_pa, uint64 new_pa)
{
  kfree((void *)old_pa);

  if (HAD_WRITE_FLAG(pte))
  {
    *pte = PA2PTE(new_pa) | PTE_FLAGS_NO_W(*pte) | PTE_OW;
  }
  else
    *pte = PA2PTE(new_pa) | PTE_FLAGS_NO_W(*pte);

  merged_val++;

  if (print_mode)
    printf("+ Merged from old pa : %p to new pa : %p\n", old_pa, new_pa);
}

void print_process_page(struct proc *p)
{
  printf("\n---------------------------process : %d-----------------------\n\n", p->pid);
  pagetable_t ptb = p->pagetable;

  for (int i = 0; i < p->sz; i+= PGSIZE)
  {
    pte_t *pte = walk(ptb, i, 0);
    uint64 pa = PTE2PA(*pte);
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
  // if (old_pa == zero_page)
  //   return;
  if (print_mode)  
    printf("- page %p point to zero_page : %p\n", old_pa, zero_page);

  kfree((void *)old_pa);
  
  if (HAD_WRITE_FLAG(pte))
    *pte = PA2PTE(zero_page) | PTE_FLAGS_NO_W(*pte) | PTE_OW;
  else
    *pte = PA2PTE(zero_page) | PTE_FLAGS_NO_W(*pte);

  merged_val++;
}

int return_args(pagetable_t ptb)
{
  if(copyout(ptb, scanned, (char *)&scanned_val, sizeof(int)) < 0)
    return -1;
    
  if(copyout(ptb, merged, (char *)&merged_val, sizeof(int)) < 0)
    return -1;
    
  return 0;
}

uint64 pte2pa(pte_t *pte)
{
  if (pte == 0)
    return 0;
  else if (!(*pte & PTE_V))
    return 0;
  return (PTE2PA(*pte));
}

uint64
sys_ksm(void)
{
  // clean_m_pages();
  if (print_mode)
  printf("----------------------------------KSM---------------------------------------------\n");
  
  scanned_val = 0; 
  merged_val = 0;
  argaddr(0, &scanned);
  argaddr(1, &merged);
  
  struct proc *p = myproc();
  pagetable_t ptb = p->pagetable;
  int my_pid = p->pid;

  for(p = proc; p < &proc[NPROC]; p++) 
  {
    if (!p->pid || p->pid == 1 || p->pid == 2 || p->pid == my_pid)
      continue;

    if (print_mode)
      print_process_page(p);
    if (print_mode)
      print_m_pages();

    for (int i = 0; i < PGROUNDUP(p->sz)/PGSIZE * PGSIZE; i += PGSIZE)
    {
      pte_t *pte = walk(p->pagetable, i, 0);
      uint64 pa = pte2pa(pte);
      if (!pa)
        continue;
      uint64 hash = xxh64((void *)pa, PGSIZE);
      int idx = find_idx_to_put(hash);
      scanned_val++;

      if (hash == zero_hash && pa != zero_page)
        point_zero_page(pte, pa);
      else if (idx == -1)
        put_page(pte, pa, hash);
      else if (idx != -1 && pa != m_pages[idx].pa)
      {
        if (++(m_pages[idx].cnt) == 2 && !m_pages[idx].is_shared)
        {
          if (HAD_WRITE_FLAG(m_pages[idx].first_pte))      
            *(m_pages[idx].first_pte) = PA2PTE(m_pages[idx].pa) | PTE_FLAGS_NO_W(*m_pages[idx].first_pte) | PTE_OW;
          else
            *(m_pages[idx].first_pte) = PA2PTE(m_pages[idx].pa) | PTE_FLAGS_NO_W(*m_pages[idx].first_pte);
          m_pages[idx].is_shared = 1;
        }
        merge_page(pte, pa, m_pages[idx].pa);
      }
    }
  }
  clean_m_pages();
  return_args(ptb);
  if (print_mode)
  printf("----------------------------------KSM End-----------------------------------------\n\n");
  return freemem;
}

