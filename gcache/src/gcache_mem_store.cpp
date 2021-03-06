/*
 * Copyright (C) 2010-2014 Codership Oy <info@codership.com>
 */

#include "gcache_mem_store.hpp"
#include "gcache_page_store.hpp"

namespace gcache
{

bool
MemStore::have_free_space (ssize_t size)
{
    while ((size_ + size > max_size_) && !seqno2ptr_.empty())
    {
        /* try to free some released bufs */
        seqno2ptr_iter_t const i  (seqno2ptr_.begin());
        BufferHeader*    const bh (ptr2BH (i->second));

        if (BH_is_released(bh)) /* discard buffer */
        {
            seqno2ptr_.erase(i);
            bh->seqno_g = SEQNO_ILL;

            switch (bh->store)
            {
            case BUFFER_IN_MEM:
                discard(bh);
                break;
            case BUFFER_IN_RB:
                bh->ctx->discard(bh);
                break;
            case BUFFER_IN_PAGE:
            {
                Page*      const page (static_cast<Page*>(bh->ctx));
                PageStore* const ps   (PageStore::page_store(page));
                ps->discard(bh);
                break;
            }
            default:
                log_fatal << "Corrupt buffer header: " << bh;
                abort();
            }
        }
        else
        {
            break;
        }
    }

    return (size_ + size <= max_size_);
}

void
MemStore::seqno_reset()
{
    for (std::set<void*>::iterator buf(allocd_.begin()); buf != allocd_.end();)
    {
        std::set<void*>::iterator tmp(buf); ++buf;

        BufferHeader* const bh(ptr2BH(*tmp));

        if (bh->seqno_g != SEQNO_NONE)
        {
            assert (BH_is_released(bh));

            allocd_.erase (tmp);

            size_ -= bh->size;
            ::free (bh);
        }
    }
}

} /* namespace gcache */
