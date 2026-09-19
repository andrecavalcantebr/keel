/* lst.type.h — gerado de lst.k pelo cgen, perfil C23. */
#ifndef LST_TYPE_H
#define LST_TYPE_H
#include "keel.type.h"

#line 8 "lst.k"
typedef struct lst_Node { i32 v; struct lst_Node *link; } lst_Node;
#line 9 "lst.k"
typedef struct lst_List { lst_Node *head; } lst_List;
#line 10 "lst.k"
typedef struct lst_cursor { lst_Node *current; } lst_cursor;
#endif
