# 从垃圾值到根因：一次 C 内存错误的逐层诊断

## 背景

这是一次真实调试过程的完整记录。场景是给练习题 `24_prev_binary_tree` 补齐实现，包括三个任务：用层序遍历数组重建二叉树（`build_tree_by_level`），以及递归和迭代两种前序遍历。

涉及的代码在 `exercises/24_prev_binary_tree/simple_tree.c`，其中 `primary_tree.h` 定义了 `TreeNode` 和 `Queue`，`simple_tree.c` 包含了队列的全套操作（`create_queue`、`enqueue`、`dequeue`、`is_empty`、`free_queue`）和待补齐的三个函数。

测试用例输入：

```c
int level_order1[] = {1, 2, 3, INT_MIN, INT_MIN, 4, 5};
// 对应树:
//     1
//    / \
//   2   3
//      / \
//     4   5
// 预期前序: 1 2 3 4 5

int level_order2[] = {1, INT_MIN, 2, 3, 4, INT_MIN, 5};
// 对应树:
//     1
//      \
//       2
//      / \
//     3   4
//      \
//       5
// 预期前序: 1 2 3 5 4
```

## 第一轮修复：建树和前序遍历

建树函数最初是一个循环每次只处理一个数组元素。二叉树分支因子为 2，这种一次一个的结构与其不匹配——在前一篇文章中已经分析过。改成了标准写法：每次循环从队列取一个父节点，分配左右两个孩子，都入队。同时处理 `INT_MIN` 作为空节点的占位符。

两个前序遍历一并实现：递归版和显式栈版。

提交后编译通过，运行输出：

```
1 2 511218512 511218576 3
1 2 511218512
```

预期是：

```
1 2 3 4 5
1 2 3 5 4
```

不是全错。不是后半全错。而是对一段、错几个、再对一个。

## 第一步：模式识别——规律性 vs 随机破坏

把错误输出逐位对比：

```
预期： 1     2     3        4        5
实际： 1     2     511218512 511218576 3

位置 0: 匹配
位置 1: 匹配
位置 2: 垃圾值
位置 3: 垃圾值
位置 4: 匹配（但值 3 跑到了原本 5 的位置）
```

如果建树的算法逻辑写错了——比如左右孩子挂反、或者遍历顺序不对——那么错误应该是规律性的。比如所有右孩子都丢掉了，或者所有节点的 children 都是 NULL。

但这里是"一部分正确、一部分垃圾、然后 3 还在但挪了位"。这种模式意味着**不是算法逻辑写错，而是内存被人踩了**。有人把已经初始化好的内存改写了。

在 Go 或 Rust 里，你设 `left = NULL` 之后它不可能凭空变成别的值。在 C 里，另一段代码完全可以通过一条你不知道的路径修改你已经初始化好的内存。所以这一步的结论是：**先怀疑有一个独立的破坏源，而不是回头改算法。**

为什么不是回头改算法？不是因为建树逻辑就是对的——那个时候我还没证据。而是因为"改算法"是一个代价很高的下一步：它会把我带回一段已经手推过的代码，花很长时间去验证一个大概率不成立的新假设。而"找破坏源"的方向不需要动建树代码，只需要观察更多数据——代价小得多。**在两种解释都成立时，先验证代价低的那一个。**

这里有一个隐含的二分类判断。如果一个 C 程序的结果部分是正确的，那么有两种可能性：

- **算法错但内存完好**：比如遍历顺序写错了、条件判断漏了分支。这种错误的特征是"全部输出都在预期集合内，但排列不对"——值是正确的，只是出现在了错误的位置。比如中序输出跑来当前序，里面不会有 500,000,000 这种值。
- **内存被踩但算法正确**：一部分值是合理的（1、2、3），但夹杂了不在输入中出现过的巨量值和明显是地址格式的值。值的范围超出了预期域。

511218512 不属于这个程序的任何一个可能输出值。它不在 `{1, 2, 3, 4, 5}` 里。**不在预期值域内的输出是内存破坏的最强信号。**

## 第二步：隔离被破坏的内存区域

选什么诊断工具是有讲究的。那时候我手上有几个选项：

- **GDB**：能设断点、单步、看调用栈。但需要停下来在某一步检查，而我还不知道哪一步是分叉点——在未知位置设断点是碰运气。
- **单节点 printf**：给每个新创建的节点打印 `val` 和 `left/right`，看哪个节点在什么时刻被污染。但这需要我把打印语句嵌回原代码，然后在命令行输出里大海捞针。
- **完整树结构打印**：把整棵树在建造完成后一次性打出来，然后静态对比预期结构。好处是：一次运行就能看到完整的破坏形态，包括污染和丢失的分布。

选了第三个。因为当前已知信息只有"输出有垃圾值"，我不知道破坏发生在建树的哪一步、不知道是哪个字段、也不知道是否还有其他被破坏的结构。**完整结构打印在信息密度上是最高的——它把所有"对/错"的信息浓缩成一张图，供静态推理。**

写了一个独立调试程序（附录一），完整拷贝了队列和建树的代码，加上了 `print_tree` 函数递归打印整棵树的节点值、地址、和 children 状态。独立程序的好处是改动不影响原文件，可以大胆加调试代码而不担心引入新的干扰。

为了看清到底哪些节点的哪些字段被污染了，写了一个独立的调试程序。这个程序完整拷贝了 `simple_tree.c` 中的队列和建树代码，然后逐节点打印整棵树：

这个调试脚本的完整代码贴在附录一。

运行的结果：

```
=== 输入 ===
size = 7
  level_order[0] = 1
  level_order[1] = 2
  level_order[2] = 3
  level_order[3] = -2147483648
  level_order[4] = -2147483648
  level_order[5] = 4
  level_order[6] = 5

=== 建树结果 ===
root->val  = 1
root->left = 0xc8da360  val=2
root->right= 0xc8da3a0  val=3

=== 树结构 ===
val=1 (addr=0xc8da340)
left:
  val=2 (addr=0xc8da360)
  left:
    val=210608992 (addr=0xc8da380)
    left:
      val=210609056 (addr=0xc8da3c0)
      left:
        NULL
      right:
        NULL
    right:
      NULL
  right:
    NULL
right:
  val=3 (addr=0xc8da3a0)
  left:
    NULL
  right:
    NULL
```

从这张图可以提取出三个不对称：

**不对称一**：node2（`0xc8da360`）的 `left` 是垃圾值 `210608992`，但 `right` 是 `NULL`。污染只打中了一个字段，不是整个节点被炸毁。

**不对称二**：node3（`0xc8da3a0`）的 `left` 和 `right` 都是 `NULL`——它们是"没被分配"，不是"被污染"。按照输入数组，node3 应该获得孩子 4 和 5。

**不对称三**：垃圾值 `210608992` 约三亿量级。在 x86-64 上它的十六进制是 `0xC8DA360`，和同一片输出中的地址 `0xc8da360`（node2 的地址）结构相似——它不是一个随机数，更像一个**被错误解释为整数的指针**。

三个不对称共同指向：**一个操作既做了多余的事（往 node2 的 `left` 里写了不该写的内容），又漏掉了该做的事（node3 的 children 没被分配）。** 两种性质不同的错误同时出现在同一个输出里，说明它们来自同一个操作的两条分支。

## 第三步：枚举所有持有指针的嫌疑人，逐项排除

node2 的 `left` 被写入了垃圾值。谁有能力做这件事？列出所有在 node2 被创建后、仍然持有**可以修改 node2 或其内存**的指针的结构：

**候选一：`build_tree_by_level` 栈上的局部变量。** 函数内部的 `left` 和 `right` 变量在分配完孩子后就离开了作用域。它们指向新创建的 TreeNode，但没有再被代码路径使用——C 不会在栈变量回收后自动往原来指向的地址写东西。

**候选二：父节点 node1 的 `left` 字段。** `node1->left = left` 存了 node2 的地址。但纵观整个建树循环，没有代码会通过 `node1->left` 再写入 node2 的任何字段。父节点只负责"挂"，不负责"改"。

**候选三：全局变量或静态存储。** 这个程序没有全局变量。队列是 `malloc` 在堆上分配的，不在全局区。

**候选四：队列。** node2 在被创建后，通过 `enqueue(q, left)` 存入了队列。队列持有 `TreeNode*` 指针，且在后续循环中不断地 `dequeue` 取出节点——被取出的节点可能再被 `enqueue` 入队其 children。队列是唯一一个**在 node2 被创建后仍然可能写 node2 相关内存**的结构。而且队列是一个链式容器，其自身内部状态（front/rear 指针）也是动态管理在堆上的。如果队列自身的链被打断或者指向了错误地址，它返回给建树代码的 `parent` 指针就会指向已释放或已被其他对象占用的内存。而建树代码会沿着这个指针做 `parent->left = left` 这类写操作——这正是"把垃圾值写入别人结构"的路径。

排查的推进逻辑：

```
有写操作的 → 候选一（栈变量）→ 写完就释放，排除
            候选二（父节点指针）→ 只挂不改，排除
            候选四（队列）→ 还在活跃改，嫌疑最大

有链式状态的 → 候选四（队列）

自身状态也在动态变化的 → 候选四（队列）
```

三个条件取交集，只有队列同时具备。**所以下一步不看别的，只看队列的状态一致性。**

这一层的通用判断是：**当你要找谁写了不该写的东西，先列出所有有写权限的指针持有者，再按"是否仍在活跃操作"排序，最后选择"自身也在动态变化"的那个——因为这种结构最容易自己被破坏后去破坏别人。**

## 第四步：追队列生命周期中的不变量分裂

已经锁定了队列。但队列是一个多方法操作的结构——`enqueue`、`dequeue`、`is_empty` 都在读写它。要找到具体哪一步出错，需要一个分析框架。我选择了不变量分析。

**为什么是不变量分析？** 不变量是"无论怎么操作都必须成立的属性"。链式结构的不变量通常不是"所有字段都正确"——那是最终目标——而是"构成结构一致性的基本关系"。对于链式队列，基本关系只有一条：**空队列的表征必须一致**，即 `front` 和 `rear` 必须同时为 `NULL`。找到不变量后，就可以逐个方法检查：这个方法进来时和出去后，不变量还在吗？哪一步是第一个破坏它的？

**怎么找到不变量？** 对于任何链式结构，找到不变量只需要回答一个问题：**"如果这个结构是空的，哪几个字段必须统一反映出这个事实？"** 对于单链表，空 = `head == NULL`（只有一个字段）。对于双链表，空 = `head == NULL && tail == NULL`。对于这个链式队列，空 = `front == NULL && rear == NULL`。如果只有 `front == NULL` 但 `rear != NULL`，结构内部矛盾了——从前面看是空的，从后面看不是空的，而队列是同一个对象。

**不变量分析为什么对链式结构特别有效。** 链式结构的正确性不是单个字段的"值对不对"，而是整张指针图的拓扑是否正确。直接验证指针图需要遍历所有节点，代价等于把整个结构走一遍。不变量把"指针图正确"拆成一小组局部的判定规则：每个方法的入口和出口处，这些规则是否还成立？一个全局问题被分解成 O(1) 个方法上的 O(1) 次检查。

**这里的不变量和 TDD 中常说的"不变量断言"有本质区别。** TDD 里的不变量通常是领域约束——"账户余额永远不能为负"、"订单状态只能沿特定路径流转"。这些是业务规则，在当前系统之外不成立（另一个系统的账户可能允许透支）。这里讨论的是**结构不变量**：它不表达任何业务语义，只表达数据结构本身作为容器的完整性条件。换一个程序，只要是链式队列，不变量仍然是"front 和 rear 对空/非空状态的判断一致"。结构不变量比领域不变量更底层——前者被破坏后，即使业务逻辑正确，也是跑在错误的内存布局上。

结构不变量的另一个实用属性是：它是一个**二值传感器**。不变量不会被"部分违反"——它要么成立，要么不成立。诊断时你不需要判断"破坏到多严重"，只需要找到第一个让它从 `true` 翻转为 `false` 的操作。这比逐行阅读代码、手工模拟执行精确得多。

队列有空和非空两种状态。对于空队列，不变量就是这一条：`front == NULL && rear == NULL`。两个指针必须同时持有一致的判断。

单独写了一个程序验证队列在"空→有→空→有"循环中的行为。选择这个场景不是因为"猜到了 bug 在这里"，而是因为：**建树程序里队列被反复填满和清空（每个父节点出队，两个孩子入队），而只有当队列清空后再入队才会触发 `rear` 相关的分支判断。** 这个场景是嫌疑人最可能在"清空"和"再入队"之间暴露不一致的。完整代码贴在附录二。

运行结果：

```
=== Step A: create empty queue ===
  front=(nil)  rear=(nil)

=== Step B: enqueue(n1) ===
    enqueue: rear=(nil), rear==NULL? YES
    -> take front branch, front=0x27c38340
  front=0x27c38340  rear=0x27c38340

=== Step C: dequeue(n1) -> queue becomes empty ===
  returned val=1
  front=(nil)  rear=0x27c38340  <-- rear still points to freed memory!

=== Step D: enqueue(n2) onto 'empty' queue ===
    enqueue: rear=0x27c38340, rear==NULL? NO
    -> take rear branch, rear->next=0x27c38340
  front=(nil)  rear=0x27c38340
  NOTE: front is still NULL! enqueue wrote to freed memory via rear

=== Step E: enqueue(n3) ===
    enqueue: rear=0x27c38340, rear==NULL? NO
    -> take rear branch, rear->next=0x27c38360
  front=(nil)  rear=0x27c38360

=== Step F: is the queue empty? ===
  is_empty(q) = true (empty)
  BUT queue actually contains n2 and n3 -- front/rear disagree!
```

这张表就是整个问题的解剖图。一行一行看：

**Step B**：第一个元素正常入队。`front` 和 `rear` 都指向同一个 `QueueNode`。

**Step C**：`dequeue` 弹出唯一元素。`front` 设为 `NULL`（正确），但 `rear` 还指着已经被 free 的 `0x27c38340`。此时队列的 `front` 和 `rear` 已经对"队列空不空"这个基本事实产生了分歧：`front` 说空，`rear` 说不空。

**Step D**：`enqueue(n2)` 进来。它用 `rear` 判断队列状态——发现 `rear` 不是 `NULL`，就认为队列不空，走了"接到队尾"的分支：`rear->next = new_node`。但 `rear = 0x27c38340` 是已经 free 过的地址。这一行往系统已经回收的内存里写了东西。

更致命的是，走这个分支就不会更新 `front`（因为"接到队尾"不改变队头）。所以 `front` 仍然是 `NULL`。

**Step E**：`enqueue(n3)` 再来一轮，同样走错分支。

**Step F**：`is_empty(q)` 只看 `front`——它说队列是空的。但队列里实际有两个元素。所有依赖 `is_empty` 的逻辑都会被误导。

回到建树的代码：

```c
while (!is_empty(q) && i < size) {
    TreeNode *parent = dequeue(q);
    // ...分配左右孩子...
}
```

`is_empty` 在第一轮之后返回 `true`，循环直接退出。所以只有根节点的左右孩子（node2 和 node3）被分配，node3 的 children 全丢了。而 node2 的 `left` 被污染，是因为 `rear->next = new_node` 往已 free 的内存写时，恰好那片内存已经被 malloc 分给了新建的 TreeNode——在它的某个字段上踩了个脚印。

## 第五步：定位失职者

谁该维护 `front == NULL && rear == NULL` 这个不变量？

`enqueue` 不负责这个——它进来时信任当前 `rear` 的值反映队列真实状态，然后只管往正确的地方接。

`dequeue` 是破坏不变量的那个操作。它动了 `front`，但没有在 `front` 变成 `NULL` 时同步 `rear`。这是一个不完整的状态转移：`dequeue` 知道"弹出最后一个元素后队列为空"，但它只告诉了 `front`，没告诉 `rear`。

修复一行：

```c
TreeNode* dequeue(Queue *q) {
    if (is_empty(q)) return NULL;
    QueueNode *old_front = q->front;
    TreeNode *to_pop = old_front->tree_node;
    q->front = old_front->next;
    if (q->front == NULL) {
        q->rear = NULL;         // <-- 加这一行
    }
    free(old_front);
    return to_pop;
}
```

修复后的输出：

```
1 2 3 4 5
1 2 3 5 4
```

`make check 24` 全部通过。

## 这个 bug 的分类：它为什么不是 double free 或 buffer overflow

bug 修了、原因找到了，但值得退一步看它属于哪一类——这会影响以后遇到类似症状时怎么判断。

这个 bug 的核心是 **use-after-free**：`dequeue` 没把 `rear` 置为 `NULL`，导致 `rear` 成为悬空指针。后续 `enqueue` 通过它执行 `rear->next = new_node`，往已释放的内存写数据。然后 `is_empty` 基于前端判断，和后端实际状态脱节，循环提前退出——节点 4、5 根本没被分配。

和几类经典内存问题的区分：

**double free**：同一个指针被 `free` 两次。标志性信号是程序崩在 `free()` 调用处——大多数 `malloc` 实现会在第二次 free 时检测到堆元数据损坏，直接 abort。这个 bug 里没有 double free：每个 `QueueNode` 只被 free 了一次。如果崩溃点总是在 `free()` 而不是在使用数据时，优先怀疑 double free。

**buffer overflow**：数组越界写。标志性信号是被破坏的数据紧挨着"数组尾部"——比如栈上两个相邻局部变量，后者被前者的越界写覆盖了；或者堆上两个相邻分配块，后者元数据被前者碾压。这个 bug 不是溢出：被写地址（已释放的 QueueNode 内存）和被破坏对象（TreeNode）之间没有"紧挨着"的空间关系——它们是被分配器复用机制联系起来的。如果在 dump 里看到"数组 A 正常，数组 B 尾部出现不属于任何逻辑来源的值"，overflow 概率很高；如果看到"A 的某个字段出了 B 的内容"，考虑分配器复用。

**dangling pointer / use-after-free**：这和本 bug 是同一种类。信号是：
1. 破坏地址是已释放对象的地址（不容易直接从输出去判断，需要通过指针值推断）
2. 被破坏对象和悬空指针的"原主"在代码上没有邻近关系——它们通过分配器复用关联
3. 症状在不同运行之间不完全一致（ASLR 下分配地址每次不同，复用的对象也不同）

但这个 bug 在 use-after-free 大类里有一个显著特征：**根因不是单纯的"忘了指针已经 free 了"，而是结构内部多字段状态不一致**。`dequeue` 更新了 `front` 但没更新 `rear`，造成两个字段对"队列是否为空"给出了相反判断。悬空指针是表象，状态转移不完整才是真正失职。

这个区分有实操意义：如果你定位到一个容器结构，问自己的第一个问题不应该是"哪一行 free 错了"，而是"这个结构的所有字段是否在所有操作中保持了一致"。后者覆盖的范围更大，能抓到那些不是"多 free"而是"少更新"的 bug。

## 这个诊断流程的通用版

这五步不依赖任何 C 的工具链知识。面对任何"输出对错交替"的内存 bug，固定按这个顺序排查。

但步骤顺序比步骤内容更重要——它是按**排除成本从低到高**排的。每一步的错误都会让你在前面浪费的时间成倍增加。

| 步骤 | 问题                                   | 如果跳过这一层会怎样？                                                 | 这次对应的答案                                   |
| ---- | -------------------------------------- | ---------------------------------------------------------------------- | ------------------------------------------------ |
| 一   | 错误有没有规律？对称的还是随机的？     | 可能花几个小时改算法，但 bug 在别的地方。模式识别是最便宜的过滤器       | 半对半错、不对称 → 排除算法错，怀疑独立破坏源   |
| 二   | 哪块内存被破坏了？破坏的特征是什么？   | 直接跳到看代码会漫无目的地读几千行。先画出破坏图才能知道往哪看           | node2 `left` 被污染、node3 孩子丢失；多写+漏写   |
| 三   | 谁持有指向被破坏内存的指针，仍在活跃？ | 错误分析会掉进"所有代码都可能出错"的泛化陷阱。必须收敛到单个候选结构     | 队列——它在增删改、持有指针且链式耦合             |
| 四   | 这个结构的不变量是什么？是否被违反？   | 知道是队列的问题但仍不知道哪个方法、哪个时刻出错。不变量是最精确的探针   | `front==NULL && rear==NULL`；Step C 之后就分裂了  |
| 五   | 谁应该维护不变量？在哪一步失职？       | 修错了位置：在 `enqueue` 里加 workaround，而不是在 `dequeue` 里修复根因 | `dequeue` 动了 `front` 没同步 `rear`              |

为什么这个顺序不能变？

第一步用"输出中有不在预期值域内的值"直接二分了问题是算法还是内存。代价为零（只需要看一眼输出）。如果这一步判断错了——比如以为是内存问题但实际是算法错——你会被带着看一堆无关代码，但至少这个判断是可逆的（几分钟后就能发现不对劲）。

第二步是在第一步排除了算法错之后，用一次完整的结构打印替代手工逐个节点跟踪。这块投入是写一个独立调试程序（附录一大约 75 行），但换回了一张能静态推理的全局图。如果没有这张图，下一步"找谁持有被污染内存的指针"就成了盲猜——你连哪些节点被污染了、污染了多少、污染的是什么都不知道。

第三步的候选枚举和排查是最累的一步，因为它要求你把程序里所有"有可能写"的路径都列出来。但这正是让问题从"不知道在哪"收敛到"只有一个结构需要看"的关键。跳过它就等于放弃收敛。

第四步把一个结构的问题进一步收敛到"哪个方法破坏了不变量"。不变量分析的好处是它把时间维度纳入了——不是看某个时刻的状态对不对，而是看"在哪个操作前后，状态从正确变成了错误"。

第五步最容易。前四步做到了精确指向一行代码，这一步就是加回去。

这里的每一步都在缩小嫌疑范围：第一步把问题从"算法"排除到"内存破坏"，第二步把破坏源从"全数据结构"缩小到"一个结构的一个字段和一个未执行的路径"，第三步锁定单个嫌疑结构（队列），第四步在这个结构内找到一个被违反的不变量，第五步找到那个不更新其他字段的操作。

**这个模板怎么套到其他场景。** 两个简要示例：

- **double free**：第一步看崩溃点——如果死在 `free()` 内部而不是在用数据时，那就是 free 相关的管理问题（等价于本文"不在预期值域内"的判断）。第二步 dump 释放历史（哪些指针被谁 free 过）→ 第三步枚举持有重复指针的代码路径 → 第四步查"每个指针只有一个 deallocator"这个不变量在哪被重复 free 打破 → 第五步定位第二个 free 的调用者。

- **buffer overflow**：第一步看被破坏数据的空间关系——和数组尾部是不是"紧挨着"（等价于本文观察垃圾值的分布模式）。是的话第二步检查相邻分配块的大小和写入量。不变量从"front/rear 一致"换成了"写入量 ≤ 分配量"，第五步找到那个用 `<=` 代替 `<` 或者漏了 `+1` 的操作。

## 附录一：树结构调试脚本

文件：`/tmp/test_tree.c`（临时代码，用于复现）

```c
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

typedef struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

typedef struct QueueNode {
    TreeNode *tree_node;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    QueueNode *front;
    QueueNode *rear;
} Queue;

Queue* create_queue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(Queue *q, TreeNode *tree_node) {
    QueueNode *rear = q->rear;
    QueueNode *new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->tree_node = tree_node;
    new_node->next = NULL;
    if (rear == NULL) q->front = new_node;
    else rear->next = new_node;
    q->rear = new_node;
}

/* buggy dequeue: no rear reset */
TreeNode* dequeue(Queue *q) {
    if (q->front == NULL) return NULL;
    QueueNode *old_front = q->front;
    TreeNode *to_pop = old_front->tree_node;
    q->front = old_front->next;
    free(old_front);
    return to_pop;
}

bool is_empty(Queue *q) { return q->front == NULL; }

TreeNode* build_tree_by_level(int *level_order, int size) {
    if (size <= 0) return NULL;
    Queue *q = create_queue();
    TreeNode *root = (TreeNode*)malloc(sizeof(TreeNode));
    root->val = level_order[0];
    root->left = root->right = NULL;
    enqueue(q, root);
    int i = 1;
    while (!is_empty(q) && i < size) {
        TreeNode *parent = dequeue(q);
        if (level_order[i] != INT_MIN) {
            TreeNode *left = (TreeNode*)malloc(sizeof(TreeNode));
            left->val = level_order[i];
            left->left = left->right = NULL;
            parent->left = left;
            enqueue(q, left);
        }
        i++;
        if (i < size) {
            if (level_order[i] != INT_MIN) {
                TreeNode *right = (TreeNode*)malloc(sizeof(TreeNode));
                right->val = level_order[i];
                right->left = right->right = NULL;
                parent->right = right;
                enqueue(q, right);
            }
            i++;
        }
    }
    return root;
}

void print_tree(TreeNode *r, int depth) {
    if (!r) { printf("%*sNULL\n", depth*2, ""); return; }
    printf("%*sval=%d (addr=%p)\n", depth*2, "", r->val, (void*)r);
    printf("%*sleft:\n", depth*2, "");
    print_tree(r->left, depth+1);
    printf("%*sright:\n", depth*2, "");
    print_tree(r->right, depth+1);
}

int main() {
    int arr[] = {1, 2, 3, INT_MIN, INT_MIN, 4, 5};
    int size = sizeof(arr)/sizeof(arr[0]);

    printf("=== 输入 ===\n");
    printf("size = %d\n", size);
    for (int i = 0; i < size; i++)
        printf("  level_order[%d] = %d\n", i, arr[i]);

    TreeNode *r = build_tree_by_level(arr, size);

    printf("\n=== 建树结果 ===\n");
    printf("root->val  = %d\n", r->val);
    printf("root->left = %p", (void*)r->left);
    if (r->left) printf("  val=%d\n", r->left->val); else printf("  NULL\n");
    printf("root->right= %p", (void*)r->right);
    if (r->right) printf("  val=%d\n", r->right->val); else printf("  NULL\n");

    printf("\n=== 树结构 ===\n");
    print_tree(r, 0);
    return 0;
}
```

## 附录二：队列不变量分裂验证脚本

文件：`/tmp/test_queue.c`（临时代码，用于复现）

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

typedef struct QueueNode {
    TreeNode *tree_node;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    QueueNode *front;
    QueueNode *rear;
} Queue;

Queue* create_queue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

TreeNode* dequeue_buggy(Queue *q) {
    if (q->front == NULL) return NULL;
    QueueNode *old_front = q->front;
    TreeNode *to_pop = old_front->tree_node;
    q->front = old_front->next;
    free(old_front);
    return to_pop;
}

void enqueue(Queue *q, TreeNode *tree_node) {
    QueueNode *rear = q->rear;
    QueueNode *new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->tree_node = tree_node;
    new_node->next = NULL;
    printf("    enqueue: rear=%p, rear==NULL? %s\n",
           (void*)rear, rear == NULL ? "YES" : "NO");
    if (rear == NULL) {
        q->front = new_node;
        printf("    -> take front branch, front=%p\n", (void*)q->front);
    } else {
        rear->next = new_node;
        printf("    -> take rear branch, rear->next=%p\n", (void*)rear->next);
    }
    q->rear = new_node;
}

bool is_empty(Queue *q) { return q->front == NULL; }

int main() {
    TreeNode n1 = {1,NULL,NULL};
    TreeNode n2 = {2,NULL,NULL};
    TreeNode n3 = {3,NULL,NULL};

    printf("=== Step A: create empty queue ===\n");
    Queue *q = create_queue();
    printf("  front=%p  rear=%p\n", (void*)q->front, (void*)q->rear);

    printf("\n=== Step B: enqueue(n1) ===\n");
    enqueue(q, &n1);
    printf("  front=%p  rear=%p\n", (void*)q->front, (void*)q->rear);

    printf("\n=== Step C: dequeue(n1) -> queue becomes empty ===\n");
    TreeNode *ret = dequeue_buggy(q);
    printf("  returned val=%d\n", ret->val);
    printf("  front=%p  rear=%p  <-- rear still points to freed memory!\n",
           (void*)q->front, (void*)q->rear);

    printf("\n=== Step D: enqueue(n2) onto 'empty' queue ===\n");
    enqueue(q, &n2);
    printf("  front=%p  rear=%p\n", (void*)q->front, (void*)q->rear);
    printf("  NOTE: front is still NULL! enqueue wrote to freed memory via rear\n");

    printf("\n=== Step E: enqueue(n3) ===\n");
    enqueue(q, &n3);
    printf("  front=%p  rear=%p\n", (void*)q->front, (void*)q->rear);

    printf("\n=== Step F: is the queue empty? ===\n");
    printf("  is_empty(q) = %s\n", is_empty(q) ? "true (empty)" : "false (not empty)");
    printf("  BUT queue actually contains n2 and n3 -- front/rear disagree!\n");
    return 0;
}
```

## 修复后的代码

最终版 `dequeue`（文件：`exercises/24_prev_binary_tree/simple_tree.c`）：

```c
TreeNode* dequeue(Queue *q) {
    if (is_empty(q)) {
        return NULL;
    }
    QueueNode *old_front = q->front;
    TreeNode *to_pop = old_front->tree_node;
    q->front = old_front->next;
    if (q->front == NULL) {
        q->rear = NULL;  // 队列变空时，rear 也要跟着置空
    }
    free(old_front);
    return to_pop;
}
```

最终 `check 24` 输出：

```
🧪 测试练习题: 24_prev_binary_tree
✅ PASS: 练习题应该移除 'I AM NOT DONE' 标记
✅ PASS: 程序应该能够成功编译和运行 (expected: 0, actual: 0)
✅ PASS: 程序输出应该包含'1 2 3 4 5'
✅ PASS: 程序输出应该包含'1 2 3 5 4'
✅ 程序正确输出了两行前序遍历结果：
1 2 3 4 5
1 2 3 5 4

总测试数: 4
通过: 4
失败: 0
```
