
struct _linkedlistItem
{
    unsigned int id;
    struct _linkedlist *nextItem;
};

typedef struct _linkedlistItem LinkedListItem;

struct _linkedlist
{
    LinkedListItem *ListHead;
    LinkedListItem *ListTail;
    unsigned int    itemCnt;
};

typedef struct _linkedlist LinkedList;

LinkedList *NewList();
LinkedListItem *AppendListItem(LinkedList *, LinkedListItem *);
LinkedListItem *FindListItemPlusPrevious(LinkedList *, unsigned int, LinkedListItem **);
LinkedListItem *FindListItem(LinkedList *, unsigned int);
LinkedListItem *RemovelistItem(LinkedList *, unsigned int);
LinkedListItem *RemoveListHead(LinkedList *);