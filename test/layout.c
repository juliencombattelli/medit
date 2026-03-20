#include <stdlib.h>

typedef struct {
    int i;
} LayoutElement;

typedef struct {
    LayoutElement* element;
} LayoutElementTreeNode;

typedef struct {
    LayoutElement* items;
    size_t count;
    size_t capacity;
} LayoutElements;

typedef struct {
    LayoutElements elements;
    LayoutElementTreeNode* root;
} Layout;

int main(void)
{
}
