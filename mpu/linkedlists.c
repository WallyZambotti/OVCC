#include <stddef.h>
#include "linkedlists.h"

LinkedList *NewList()
{
    LinkedList *newlist = malloc(sizeof(LinkedList));
    newlist->ListHead = newlist->ListTail = NULL;
    newlist->itemCnt = 0;
    return newlist;
}

LinkedListItem *AppendListItem(LinkedList *list, LinkedListItem *item)
{
    if (item == NULL || list == NULL) { return NULL; };

    if (list->ListTail == NULL)
    {
        list->ListTail = list->ListHead = item;
    }
    else
    {
        list->ListTail = list->ListTail->nextItem = item;
    }

    list->itemCnt++;

    return item;
}

LinkedListItem *FindListItemPlusPrevious(LinkedList *list, unsigned int id, LinkedListItem **previous)
{
    if (list == NULL || previous == NULL) { return NULL; };

    *previous = NULL;

    for(LinkedListItem *item = list->ListHead ; item != NULL ; item = item->nextItem)
    {
        if (item->id == id) return item;
        *previous = item;
    }

    *previous = NULL;
    return NULL;
}

LinkedListItem *FindListItem(LinkedList *list, unsigned int id)
{
    if (list == NULL) { return NULL; };

    for(LinkedListItem *item = list->ListHead ; item != NULL ; item = item->nextItem)
    {
        if (item->id == id) return item;
    }

    return NULL;
}


LinkedListItem *RemovelistItem(LinkedList *list, unsigned int id)
{
    LinkedListItem *previous;

    if (list == NULL) { return NULL; };

    LinkedListItem *item = FindListItemPlusPrevious(list, id, &previous);

    if (item == NULL) { return NULL; };

    if (previous != NULL) 
    {
        previous->nextItem = item->nextItem;
    }
    else
    {
        list->ListHead = item->nextItem;
    }
    
    if (item->nextItem == NULL)
    {
        list->ListTail = previous;
    }

    list->itemCnt--;

    return item;
}

LinkedListItem *RemoveListHead(LinkedList *list)
{
    if (list == NULL) return NULL;
    LinkedListItem *item = list->ListHead;
    if (item == NULL) return NULL;
    list->ListHead = list->ListHead->nextItem;
    list->itemCnt--;
    if (list->ListHead == NULL) list->ListTail = NULL;
    return item;
}
