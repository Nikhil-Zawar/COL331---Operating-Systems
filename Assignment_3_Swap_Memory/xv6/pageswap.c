#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

struct swap_slot_struct swap_table[800];
extern struct superblock sb;

#define LIMIT 100
#define INIT_TH 100
#define INIT_NPG 4

int Th = INIT_TH;
int Npg = INIT_NPG;

struct spinlock s_table_lock;

char *disk;

void init_swap_slots()
{
    initlock(&s_table_lock, "swap lock");

    cprintf("swap slots initialized\n");
    for (int i = 0; i < SWAP_PAGES; i++)
    {
        swap_table[i].is_free = 1;
        swap_table[i].page_perm = 0;
    }
}

int find_free_swap_slot(void)
{
    acquire(&s_table_lock);
    for (int i = 0; i < SWAP_PAGES; i++)
    {
        if (swap_table[i].is_free == 1)
        {
            swap_table[i].is_free = 0;
            release(&s_table_lock);
            return i;
        }
    }
    release(&s_table_lock);

    // No free swap slot found
    return -1;
}

pte_t *walkpgdir_pageswap(pde_t *pgdir, uint va)
{
    pde_t *pde = &pgdir[PDX(va)];

    if (!(*pde & PTE_P))
        return 0;

    pte_t *pgtab = (pte_t *)P2V(PTE_ADDR(*pde));
    return &pgtab[PTX(va)];
}

int count_free_slots()
{
    int count = 0;
    acquire(&s_table_lock);
    for (int i = 0; i < SWAP_PAGES; i++)
    {
        if (swap_table[i].is_free == 1)
        {
            count++;
        }
    }
    release(&s_table_lock);

    return count;
}

int free_swap_slot(int slot)
{
    if (slot > 0 && slot < SWAP_PAGES)
    {
        acquire(&s_table_lock);
        swap_table[slot].page_perm = 0;
        swap_table[slot].is_free = 1;
        release(&s_table_lock);
        return 0;
    }
    else
    {
        return -1;
    }
}

void free_swap_spaces(struct proc *p)
{
    uint va;
    pte_t *pgdir = p->pgdir;

    for (va = 0; va < KERNBASE; va += PGSIZE)
    {
        pte_t *pte = walkpgdir_pageswap(pgdir, (void *)va);

        if (!pte)
        {
            continue;
        }

        if ((!(*pte & PTE_P)) && (*pte >> 12) != 0)
        {
            int slot_num = *pte >> 12;
            int can_free = free_swap_slot(slot_num);

            if (can_free < 0)
            {
                cprintf("cannnot free swap slot\n");
            }
        }
    }
}

void swap_out_page(struct proc *victim_proc, uint va)
{
    int swap_slot_index = find_free_swap_slot();
    if (swap_slot_index == -1)
    {
        cprintf("No free swap slots available\n");
        return;
    }

    uint victim_page_va = va;
    pte_t *pte = walkpgdir_pageswap(victim_proc->pgdir, victim_page_va);

    if (!pte)
    {
        cprintf("Invalid PTE for VA %p\n", (void *)victim_page_va);
        return;
    }

    if (*pte & PTE_P)
    {
        char *page = (char *)P2V(PTE_ADDR(*pte));
        for (int i = 0; i < SWAP_BLOCKS_PER_PAGE; i++)
        {
            int blockno = sb.swap_start + (swap_slot_index * SWAP_BLOCKS_PER_PAGE) + i;

            struct buf *disk_buf = bread(0, blockno);
            memmove(disk_buf->data, page + i * BSIZE, BSIZE);
            bwrite(disk_buf);
            brelse(disk_buf);
        }

        swap_table[swap_slot_index].page_perm = PTE_FLAGS(*pte) & 0xFFF;
        swap_table[swap_slot_index].is_free = 0;

        int perm = *pte & (PTE_W | PTE_U);

        *pte = (swap_slot_index << 12) | perm;

        kfree(page);
        lcr3(V2P(victim_proc->pgdir));
    }
    else
    {
        cprintf("Page not present in memory\n");
    }
}

int swap_in_page(struct proc *p, pte_t *pte, uint va)
{
    uint slot_num = ((uint)(*pte)) >> 12;

    // Invalid slot num
    if (slot_num < 0 || slot_num >= SWAP_PAGES)
    {
        return -1;
    }

    // Check for slot is free or not
    acquire(&s_table_lock);
    if (swap_table[slot_num].is_free)
    {
        release(&s_table_lock);
        return -1;
    }
    release(&s_table_lock);

    // Check for page replacement before allocating any new page
    adaptive_page_swap();

    char *mem = kalloc();
    if (mem == 0)
    {
        return -1;
    }

    memset(mem, 0, PGSIZE);

    for (int i = 0; i < SWAP_BLOCKS_PER_PAGE; i++)
    {
        int block = sb.swap_start + (slot_num * SWAP_BLOCKS_PER_PAGE) + i;
        struct buf *buf = bread(0, block);
        memmove(mem + i * BSIZE, buf->data, BSIZE);
        brelse(buf);
    }

    acquire(&s_table_lock);
    *pte = V2P(mem) | swap_table[slot_num].page_perm;
    swap_table[slot_num].page_perm = 0;
    swap_table[slot_num].is_free = 1;
    release(&s_table_lock);

    lcr3(V2P(p->pgdir));

    return 0;
}

void adaptive_page_swap(void)
{
    int free_pages = count_free_pages();

    if (free_pages > Th)
    {
        return;
    }

    cprintf("Current Threshold = %d, Swapping %d pages\n", Th, Npg);

    for (int i = 0; i < Npg; i++)
    {
        struct proc *victim_proc;
        pte_t *pte;
        uint va;

        victim_proc = get_victim_proc_page(&pte, &va);

        if (!victim_proc)
        {
            break;
        }

        swap_out_page(victim_proc, va);
    }

    Th = (Th * (100 - BETA)) / 100;
    Npg = (Npg * (100 + ALPHA)) / 100;

    if (Npg >= LIMIT)
    {
        Npg = LIMIT;
    }
}
