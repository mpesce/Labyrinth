/*
 * lablist.h -- the tiny intrusive list used throughout Labyrinth.
 *
 * Reconstructed from LABYRINTH.EXE (code at 0x412960-0x412990 and
 * 0x413490-0x413540).  Objects that live on a list embed (or derive from) a
 * LabLink; a LabList just remembers the first and last links.  Lists are
 * append-only; destroying a list frees every link with operator delete.
 */
#ifndef LABLIST_H
#define LABLIST_H

class LabLink {
public:
    LabLink() : next(0), prev(0) {}                 /* 412960 */
    LabLink *Next() { return next; }                /* 412970 */
    LabLink *Prev() { return prev; }                /* 412980 */

    /* Link this node in after 'after' (412990).  Note: the original never
     * fixes up the old successor's back pointer, which is harmless because
     * nodes are only ever appended at the tail. */
    void LinkAfter(LabLink *after)
    {
        if (after) {
            if (after->next)
                next = after->next;
            after->next = this;
            prev = after;
        }
    }

private:
    LabLink *next;
    LabLink *prev;
};

class LabList {
public:
    LabList() : head(0), tail(0) {}                 /* 413490 */

    /* The original destructor (4134a0) freed every link with operator
     * delete.  That is right for lists of model objects (the link is the
     * start of the allocation) but wrong for the window lists, whose links
     * are embedded 4 bytes into each window -- a latent heap corruption that
     * never fired in 1995 because Labyrinth exits via exit(0).  The freeing
     * is therefore explicit here, and only the model lists use it. */
    void DeleteLinks()
    {
        LabLink *l = First();
        while (l) {
            LabLink *n = Next(l);
            ::operator delete(l);
            l = n;
        }
        head = tail = 0;
    }
    void Append(LabLink *l)                         /* 4134d0 */
    {
        if (!tail) {
            head = tail = l;
        } else {
            l->LinkAfter(tail);
            tail = l;
        }
    }
    LabLink *First() { return head; }               /* 413500 */
    LabLink *Last() { return tail; }                /* 413510 */
    static LabLink *Next(LabLink *l) { return l->Next(); }  /* 413520 */
    static LabLink *Prev(LabLink *l) { return l->Prev(); }  /* 413530 */

private:
    LabLink *head;
    LabLink *tail;
};

#endif /* LABLIST_H */
