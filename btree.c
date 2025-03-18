#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define M 5  // B树的阶数，表示每个节点最多有M个子节点和M-1个键值

// B树节点结构
typedef struct BTreeNode {
    int keys[M-1];           // 存储键值
    struct BTreeNode* children[M];  // 子节点指针
    int n;                   // 当前节点中键值的数量
    bool leaf;               // 是否为叶子节点
} BTreeNode;

// B树结构
typedef struct BTree {
    BTreeNode* root;         // 根节点
} BTree;

// 创建新的B树节点
BTreeNode* createNode(bool leaf) {
    BTreeNode* newNode = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (!newNode) {
        printf("内存分配失败\n");
        exit(1);
    }
    
    newNode->leaf = leaf;
    newNode->n = 0;
    
    // 初始化子节点指针为NULL
    for (int i = 0; i < M; i++) {
        newNode->children[i] = NULL;
    }
    
    return newNode;
}

// 创建空的B树
BTree* createBTree() {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (!tree) {
        printf("内存分配失败\n");
        exit(1);
    }
    
    tree->root = createNode(true);  // 初始时根节点也是叶子节点
    return tree;
}

// 在节点中查找键值的位置
int findKey(BTreeNode* node, int key) {
    int idx = 0;
    
    // 找到第一个大于等于key的位置
    while (idx < node->n && node->keys[idx] < key) {
        idx++;
    }
    
    return idx;
}

// 在B树中查找键值
bool search(BTreeNode* root, int key) {
    if (!root) {
        return false;
    }
    
    // 找到第一个大于等于key的位置
    int i = findKey(root, key);
    
    // 如果找到了键值
    if (i < root->n && root->keys[i] == key) {
        return true;
    }
    
    // 如果是叶子节点且未找到，则不存在
    if (root->leaf) {
        return false;
    }
    
    // 递归搜索合适的子树
    return search(root->children[i], key);
}

// 分裂子节点
void splitChild(BTreeNode* parent, int index, BTreeNode* child) {
    // 创建新节点存储分裂后的右半部分
    BTreeNode* newNode = createNode(child->leaf);
    newNode->n = M/2 - 1;  // 对于5阶B树，右节点将有2个键值
    
    // 复制child节点的右半部分键值到newNode
    for (int j = 0; j < M/2 - 1; j++) {
        newNode->keys[j] = child->keys[j + M/2];
    }
    
    // 如果不是叶子节点，复制对应的子节点指针
    if (!child->leaf) {
        for (int j = 0; j < M/2; j++) {
            newNode->children[j] = child->children[j + M/2];
        }
    }
    
    // 更新child节点的键值数量
    child->n = M/2 - 1;
    
    // 在父节点中为新键值腾出空间
    for (int j = parent->n; j > index; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    
    // 将新节点链接到父节点
    parent->children[index + 1] = newNode;
    
    // 为新键值腾出空间
    for (int j = parent->n - 1; j >= index; j--) {
        parent->keys[j + 1] = parent->keys[j];
    }
    
    // 将中间键值复制到父节点
    parent->keys[index] = child->keys[M/2 - 1];
    
    // 增加父节点的键值数量
    parent->n++;
}

// 在非满节点中插入键值
void insertNonFull(BTreeNode* node, int key) {
    int i = node->n - 1;
    
    // 如果是叶子节点，直接插入
    if (node->leaf) {
        // 找到合适的插入位置
        while (i >= 0 && node->keys[i] > key) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        
        node->keys[i + 1] = key;
        node->n++;
    } else {
        // 找到合适的子树
        while (i >= 0 && node->keys[i] > key) {
            i--;
        }
        i++;
        
        // 如果子节点已满，需要分裂
        if (node->children[i]->n == M - 1) {
            splitChild(node, i, node->children[i]);
            
            // 分裂后确定要插入的子树
            if (node->keys[i] < key) {
                i++;
            }
        }
        
        // 递归插入到子树
        insertNonFull(node->children[i], key);
    }
}

// 在B树中插入键值
void insert(BTree* tree, int key) {
    BTreeNode* root = tree->root;
    
    // 如果根节点已满，需要分裂
    if (root->n == M - 1) {
        BTreeNode* newRoot = createNode(false);
        tree->root = newRoot;
        newRoot->children[0] = root;
        
        // 分裂原根节点
        splitChild(newRoot, 0, root);
        
        // 在新的根节点中插入键值
        int i = 0;
        if (newRoot->keys[0] < key) {
            i++;
        }
        insertNonFull(newRoot->children[i], key);
    } else {
        // 根节点未满，直接插入
        insertNonFull(root, key);
    }
}

// 从节点中删除键值
void removeFromNode(BTreeNode* node, int idx) {
    // 移动键值
    for (int i = idx + 1; i < node->n; i++) {
        node->keys[i - 1] = node->keys[i];
    }
    
    // 减少键值数量
    node->n--;
}

// 合并节点
void merge(BTreeNode* parent, int idx, BTreeNode* left, BTreeNode* right) {
    // 将父节点的键值移到左子节点
    left->keys[left->n] = parent->keys[idx];
    left->n++;
    
    // 将右子节点的所有键值复制到左子节点
    for (int i = 0; i < right->n; i++) {
        left->keys[left->n + i] = right->keys[i];
    }
    
    // 如果不是叶子节点，复制子节点指针
    if (!left->leaf) {
        for (int i = 0; i <= right->n; i++) {
            left->children[left->n + i] = right->children[i];
        }
    }
    
    // 更新左子节点的键值数量
    left->n += right->n;
    
    // 在父节点中移除对应的键值和右子节点指针
    for (int i = idx + 1; i < parent->n; i++) {
        parent->keys[i - 1] = parent->keys[i];
        parent->children[i] = parent->children[i + 1];
    }
    
    // 减少父节点的键值数量
    parent->n--;
    
    // 释放右子节点的内存
    free(right);
}

// 从B树中删除键值
BTreeNode* removeKey(BTreeNode* node, int key) {
    int idx = findKey(node, key);
    
    // 如果找到键值
    if (idx < node->n && node->keys[idx] == key) {
        // 情况1：在叶子节点中
        if (node->leaf) {
            removeFromNode(node, idx);
        } else {
            // 情况2：在内部节点中
            BTreeNode* pred = node->children[idx];
            
            // 找到前驱（左子树的最右节点）
            while (!pred->leaf) {
                pred = pred->children[pred->n];
            }
            
            // 用前驱替换要删除的键值
            if (pred->n >= M/2) {
                node->keys[idx] = pred->keys[pred->n - 1];
                // 递归删除前驱
                removeKey(node->children[idx], pred->keys[pred->n - 1]);
            } else {
                // 情况3：前驱节点键值不足，尝试后继
                BTreeNode* succ = node->children[idx + 1];
                
                // 找到后继（右子树的最左节点）
                while (!succ->leaf) {
                    succ = succ->children[0];
                }
                
                // 用后继替换要删除的键值
                if (succ->n >= M/2) {
                    node->keys[idx] = succ->keys[0];
                    // 递归删除后继
                    removeKey(node->children[idx + 1], succ->keys[0]);
                } else {
                    // 情况4：前驱和后继节点键值都不足，合并节点
                    merge(node, idx, node->children[idx], node->children[idx + 1]);
                    // 递归删除键值
                    removeKey(node->children[idx], key);
                }
            }
        }
    } else {
        // 键值不在当前节点中
        if (node->leaf) {
            printf("键值 %d 不存在于B树中\n", key);
            return node;
        }
        
        // 确定要递归的子树
        bool flag = (idx == node->n);
        
        // 如果子节点键值数量不足
        if (node->children[idx]->n < M/2) {
            // 尝试从左兄弟借
            if (idx > 0 && node->children[idx - 1]->n >= M/2) {
                BTreeNode* child = node->children[idx];
                BTreeNode* leftSibling = node->children[idx - 1];
                
                // 为新键值腾出空间
                for (int i = child->n - 1; i >= 0; i--) {
                    child->keys[i + 1] = child->keys[i];
                }
                
                // 如果不是叶子节点，移动子节点指针
                if (!child->leaf) {
                    for (int i = child->n; i >= 0; i--) {
                        child->children[i + 1] = child->children[i];
                    }
                }
                
                // 将父节点的键值下移到子节点
                child->keys[0] = node->keys[idx - 1];
                
                // 如果不是叶子节点，移动左兄弟的最右子节点
                if (!child->leaf) {
                    child->children[0] = leftSibling->children[leftSibling->n];
                }
                
                // 将左兄弟的最右键值上移到父节点
                node->keys[idx - 1] = leftSibling->keys[leftSibling->n - 1];
                
                // 更新节点键值数量
                child->n++;
                leftSibling->n--;
            }
            // 尝试从右兄弟借
            else if (idx < node->n && node->children[idx + 1]->n >= M/2) {
                BTreeNode* child = node->children[idx];
                BTreeNode* rightSibling = node->children[idx + 1];
                
                // 将父节点的键值下移到子节点
                child->keys[child->n] = node->keys[idx];
                
                // 如果不是叶子节点，移动右兄弟的最左子节点
                if (!child->leaf) {
                    child->children[child->n + 1] = rightSibling->children[0];
                }
                
                // 将右兄弟的最左键值上移到父节点
                node->keys[idx] = rightSibling->keys[0];
                
                // 在右兄弟中移除最左键值
                for (int i = 1; i < rightSibling->n; i++) {
                    rightSibling->keys[i - 1] = rightSibling->keys[i];
                }
                
                // 如果不是叶子节点，移动子节点指针
                if (!rightSibling->leaf) {
                    for (int i = 1; i <= rightSibling->n; i++) {
                        rightSibling->children[i - 1] = rightSibling->children[i];
                    }
                }
                
                // 更新节点键值数量
                child->n++;
                rightSibling->n--;
            }
            // 如果左右兄弟都不能借，合并节点
            else {
                if (idx < node->n) {
                    merge(node, idx, node->children[idx], node->children[idx + 1]);
                } else {
                    merge(node, idx - 1, node->children[idx - 1], node->children[idx]);
                    idx--;
                }
            }
        }
        
        // 如果根节点变为空节点
        if (flag && idx > node->n) {
            return removeKey(node->children[idx - 1], key);
        } else {
            return removeKey(node->children[idx], key);
        }
    }
    
    return node;
}

// 从B树中删除键值的入口函数
void removeFromBTree(BTree* tree, int key) {
    if (!tree->root) {
        printf("B树为空\n");
        return;
    }
    
    BTreeNode* newRoot = removeKey(tree->root, key);
    
    // 如果根节点变为空节点且有子节点
    if (tree->root->n == 0 && !tree->root->leaf) {
        BTreeNode* oldRoot = tree->root;
        tree->root = tree->root->children[0];
        free(oldRoot);
    }
}

// 中序遍历B树
void inorderTraversal(BTreeNode* node) {
    if (node) {
        int i;
        for (i = 0; i < node->n; i++) {
            // 先访问左子树
            if (!node->leaf) {
                inorderTraversal(node->children[i]);
            }
            // 访问当前键值
            printf("%d ", node->keys[i]);
        }
        
        // 访问最右子树
        if (!node->leaf) {
            inorderTraversal(node->children[i]);
        }
    }
}

// 释放B树节点内存
void freeBTreeNode(BTreeNode* node) {
    if (!node) return;
    
    // 如果不是叶子节点，递归释放所有子节点
    if (!node->leaf) {
        for (int i = 0; i <= node->n; i++) {
            freeBTreeNode(node->children[i]);
        }
    }
    
    // 释放当前节点
    free(node);
}

// 释放整个B树
void freeBTree(BTree* tree) {
    if (tree) {
        freeBTreeNode(tree->root);
        free(tree);
    }
}

// 打印B树结构（层次遍历）
void printBTree(BTreeNode* root, int level) {
    if (root) {
        printf("Level %d: ", level);
        for (int i = 0; i < root->n; i++) {
            printf("%d ", root->keys[i]);
        }
        printf("\n");
        
        if (!root->leaf) {
            for (int i = 0; i <= root->n; i++) {
                printBTree(root->children[i], level + 1);
            }
        }
    }
}

// 主函数测试B树操作
int main() {
    BTree* tree = createBTree();
    
    // 插入测试数据
    insert(tree, 10);
    insert(tree, 20);
    insert(tree, 5);
    insert(tree, 6);
    insert(tree, 12);
    insert(tree, 30);
    insert(tree, 7);
    insert(tree, 17);
    
    printf("B树中序遍历: ");
    inorderTraversal(tree->root);
    printf("\n\n");
    
    printf("B树结构:\n");
    printBTree(tree->root, 0);
    printf("\n");
    
    // 查找测试
    int searchKey = 6;
    if (search(tree->root, searchKey)) {
        printf("键值 %d 存在于B树中\n", searchKey);
    } else {
        printf("键值 %d 不存在于B树中\n", searchKey);
    }
    
    searchKey = 15;
    if (search(tree->root, searchKey)) {
        printf("键值 %d 存在于B树中\n", searchKey);
    } else {
        printf("键值 %d 不存在于B树中\n", searchKey);
    }
    printf("\n");
    
    // 删除测试
    int deleteKey = 6;
    printf("删除键值 %d\n", deleteKey);
    removeFromBTree(tree, deleteKey);
    
    printf("删除后的B树中序遍历: ");
    inorderTraversal(tree->root);
    printf("\n\n");
    
    printf("删除后的B树结构:\n");
    printBTree(tree->root, 0);
    
    // 释放B树
    freeBTree(tree);
    
    return 0;
}
